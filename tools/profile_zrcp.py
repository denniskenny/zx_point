#!/usr/bin/env python3
"""
profile_zrcp.py — Profile the ZX Point main loop via ZEsarUX ZRCP.

Measures T-states between successive function entry breakpoints to
determine how long each section of the main loop takes.

Usage:
    python3 tools/profile_zrcp.py [--frames N] [--mapfile FILE]
"""

import socket
import subprocess
import time
import sys
import os
import argparse
import re
from collections import defaultdict


def parse_map_file(path):
    symbols = {}
    with open(path) as f:
        for line in f:
            m = re.match(r"(\S+)\s+=\s+\$([0-9A-Fa-f]+)\s*;", line)
            if m:
                symbols[m.group(1)] = int(m.group(2), 16)
    return symbols


class ZRCPConnection:
    def __init__(self, host="localhost", port=10000):
        self.sock = None
        self.host = host
        self.port = port

    def connect(self, retries=20, delay=0.5):
        for i in range(retries):
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(5)
                self.sock.connect((self.host, self.port))
                self._recv()  # banner
                return True
            except (ConnectionRefusedError, OSError):
                if self.sock:
                    self.sock.close()
                self.sock = None
                time.sleep(delay)
        return False

    def cmd(self, c):
        self.sock.sendall((c + "\n").encode())
        return self._recv()

    def _recv(self):
        data = b""
        self.sock.settimeout(3)
        while True:
            try:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                data += chunk
                if b"command>" in data or b"command@" in data:
                    break
            except socket.timeout:
                break
        return data.decode("utf-8", errors="replace")

    def get_pc(self):
        resp = self.cmd("get-registers")
        m = re.search(r"PC=([0-9A-Fa-f]+)", resp)
        return int(m.group(1), 16) if m else None

    def get_tstates(self):
        resp = self.cmd("get-tstates-partial")
        m = re.search(r"(\d+)", resp)
        return int(m.group(1)) if m else None

    def set_bp(self, index, condition):
        """Enable breakpoints, then set one."""
        self.cmd("enable-breakpoints")
        return self.cmd(f"set-breakpoint {index} {condition}")

    def clear_bp(self, index):
        self.cmd(f"set-breakpoint {index}")

    def run_to_break(self, timeout=15):
        """Run and wait for breakpoint hit."""
        self.sock.sendall(b"run\n")
        data = b""
        self.sock.settimeout(timeout)
        while True:
            try:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                data += chunk
                text = data.decode("utf-8", errors="replace")
                if "command@cpu-step>" in text or "command>" in text:
                    break
            except socket.timeout:
                break
        return data.decode("utf-8", errors="replace")

    def close(self):
        if self.sock:
            try:
                self.cmd("quit")
            except Exception:
                pass
            self.sock.close()


# Waypoints in execution order within state_game_tick.
WAYPOINTS = [
    "_player_update",
    "_predators_update",
    "_depth_transition_tick",
    "_treasure_check_collection",
    "_vsync_wait",
    "_predators_render",
    "_update_and_draw_stars",
    "_sprites_player_draw",
    "_hud_draw",
    "_minimap_draw",
    "_depth_indicator_draw",
]

TSTATES_PER_FRAME = 69888


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--frames", type=int, default=3)
    parser.add_argument("--mapfile", default="starfield.map")
    parser.add_argument("--tap", default="starfield.tap")
    parser.add_argument("--settle", type=float, default=8.0)
    args = parser.parse_args()

    # Use absolute paths since ZEsarUX changes cwd
    tap_path = os.path.abspath(args.tap)
    map_path = os.path.abspath(args.mapfile)

    symbols = parse_map_file(map_path)
    waypoints = [(l, symbols[l]) for l in WAYPOINTS if l in symbols]

    print(f"Waypoints ({len(waypoints)}):")
    for l, a in waypoints:
        print(f"  ${a:04X}  {l}")

    tick_addr = symbols.get("_state_game_tick")
    if tick_addr:
        print(f"  ${tick_addr:04X}  _state_game_tick (frame start)")

    # Locate the 48.rom for headless mode
    rom_candidates = [
        os.path.expanduser("~/projects/zesarux/src/48.rom"),
        "/usr/local/share/zesarux/48.rom",
    ]
    rom_path = None
    for r in rom_candidates:
        if os.path.isfile(r):
            rom_path = r
            break
    if not rom_path:
        print("ERROR: Cannot find 48.rom for ZEsarUX")
        return 1

    print(f"\nLaunching ZEsarUX (ROM: {rom_path})...")
    emu = subprocess.Popen(
        ["zesarux", "--vo", "null", "--ao", "null",
         "--enable-remoteprotocol", "--machine", "48k",
         "--noconfigfile", "--quickexit",
         "--romfile", rom_path, tap_path],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    z = ZRCPConnection()
    if not z.connect():
        print("ERROR: Cannot connect to ZRCP")
        emu.terminate()
        return 1

    print(f"Connected. Settling {args.settle}s...")
    z.cmd("run")
    time.sleep(args.settle)

    # Pause
    z.cmd("enter-cpu-step")
    time.sleep(0.3)

    segment_costs = defaultdict(list)

    for frame in range(args.frames):
        print(f"\nFrame {frame + 1}/{args.frames}:")

        # Sync to frame start via _state_game_tick
        if tick_addr:
            z.set_bp(1, f"PC={tick_addr:04X}h")
            z.run_to_break()

            pc = z.get_pc()
            if pc != tick_addr:
                print(f"  WARNING: expected PC=${tick_addr:04X}, got PC=${pc:04X}" if pc else "  WARNING: no PC")
                continue

        z.cmd("reset-tstates-partial")

        # Run to each waypoint in sequence
        prev_label = "_state_game_tick"
        prev_t = 0
        ok = True

        for label, addr in waypoints:
            z.set_bp(1, f"PC={addr:04X}h")
            z.run_to_break()

            pc = z.get_pc()
            t = z.get_tstates()

            if pc == addr and t is not None:
                cost = t - prev_t
                segment_costs[prev_label].append(cost)
                print(f"  {prev_label:<35} {cost:>8} T")
                prev_t = t
                prev_label = label
            else:
                actual = f"${pc:04X}" if pc else "None"
                print(f"  MISSED {label} (got PC={actual})")
                ok = False
                break

        if not ok:
            continue

        # Measure remainder: last waypoint to next frame start
        if tick_addr:
            z.set_bp(1, f"PC={tick_addr:04X}h")
            z.run_to_break()

            t = z.get_tstates()
            if t is not None:
                cost = t - prev_t
                segment_costs[prev_label].append(cost)
                print(f"  {prev_label:<35} {cost:>8} T")
                print(f"  FRAME TOTAL: {t} T-states "
                      f"({100.0 * t / TSTATES_PER_FRAME:.1f}% of budget)")

    # Cleanup
    print("\nShutting down...")
    z.close()
    emu.terminate()
    try:
        emu.wait(timeout=5)
    except subprocess.TimeoutExpired:
        emu.kill()

    # Summary
    print(f"\n{'='*70}")
    print(f"PROFILE SUMMARY (avg of {args.frames} frames)")
    print(f"Frame budget: {TSTATES_PER_FRAME} T-states (48K @ 50Hz)")
    print(f"{'='*70}")
    print(f"{'Segment':<40} {'Avg T':>10} {'%':>7}  Visual")
    print(f"{'-'*40} {'-'*10} {'-'*7}  {'-'*20}")

    grand = 0
    all_labels = ["_state_game_tick"] + [l for l, _ in waypoints]
    for label in all_labels:
        if label in segment_costs and segment_costs[label]:
            vals = segment_costs[label]
            avg = sum(vals) / len(vals)
            pct = 100.0 * avg / TSTATES_PER_FRAME
            grand += avg
            bar = "#" * max(1, int(pct))
            print(f"{label:<40} {avg:>10.0f} {pct:>6.1f}%  {bar}")

    print(f"{'-'*40} {'-'*10} {'-'*7}")
    pct_used = 100.0 * grand / TSTATES_PER_FRAME
    idle = TSTATES_PER_FRAME - grand
    pct_idle = 100.0 * idle / TSTATES_PER_FRAME
    print(f"{'TOTAL ACTIVE':<40} {grand:>10.0f} {pct_used:>6.1f}%")
    print(f"{'IDLE / VSYNC WAIT':<40} {idle:>10.0f} {pct_idle:>6.1f}%")

    return 0


if __name__ == "__main__":
    sys.exit(main())
