#!/usr/bin/env python3
"""
run_tests.py -- Run ZX Spectrum test binaries in ZEsarUX headless mode.

Launches ZEsarUX with --vo null --ao null --enable-remoteprotocol,
waits for the test program to write results to 0xD000, reads them
over ZRCP, and reports pass/fail.

Results format at 0xD000:
  byte 0  = total test count
  byte 1  = pass count
  bytes 2..N = per-test result (1=pass, 0=fail)
  byte N+1   = 0xAA sentinel

Environment variables:
  ZESARUX      Path to zesarux binary (default: zesarux)
  ZESARUX_ROM  Path to 48.rom (auto-detected if not set)
"""

import os
import socket
import subprocess
import sys
import time

ZRCP_HOST = "localhost"
ZRCP_PORT = 10000
RESULTS_ADDR = 0xD000
SENTINEL = 0xAA
MAX_WAIT = 10  # seconds


TEST_NAMES = {
    0:  "O key (left) increases gx",
    1:  "P key (right) decreases gx",
    2:  "Q key (forward) increases gz",
    3:  "A key (backward) decreases gz",
    4:  "Minimap: higher gx -> further left",
    5:  "Minimap: higher gz -> further up (north)",
    6:  "Screen: higher gx (west) -> left on screen",
    7:  "Screen X and minimap X agree",
    8:  "Z scale: close -> SCALE_32 (largest)",
    9:  "Z scale: far -> SCALE_4 (smallest)",
    10: "Z scale: monotonic with distance",
    11: "Z scale: beyond range -> SCALE_NONE",
    12: "Shark pursuit X: chases toward player gx",
    13: "Shark pursuit Z: chases toward player gz",
    14: "Pursuit X matches screen direction",
}


def find_rom():
    """Locate 48.rom for ZEsarUX."""
    env = os.environ.get("ZESARUX_ROM")
    if env and os.path.isfile(env):
        return env
    candidates = [
        os.path.expanduser("~/projects/zesarux/src/48.rom"),
        "/usr/local/share/zesarux/48.rom",
        "/usr/share/zesarux/48.rom",
    ]
    for p in candidates:
        if os.path.isfile(p):
            return p
    return None


def parse_hex_blob(raw):
    """Parse ZRCP read-memory response (continuous hex string)."""
    result = []
    for line in raw.splitlines():
        line = line.strip()
        if not line or line.startswith("command") or line.startswith("Welcome") \
                or line.startswith("Write "):
            continue
        # Continuous hex: "0F0F0101010101..."
        if all(c in "0123456789abcdefABCDEF" for c in line) and len(line) >= 2:
            for i in range(0, len(line), 2):
                result.append(int(line[i:i+2], 16))
    return result


def zrcp_read_memory(addr, length):
    """Read bytes from Spectrum memory via ZRCP."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    try:
        s.connect((ZRCP_HOST, ZRCP_PORT))
    except (ConnectionRefusedError, OSError):
        return None
    # Consume the welcome/prompt
    try:
        s.recv(4096)
    except socket.timeout:
        pass

    cmd = f"read-memory {addr} {length}\n"
    s.sendall(cmd.encode())

    chunks = []
    deadline = time.time() + 3
    while time.time() < deadline:
        try:
            data = s.recv(4096)
            if not data:
                break
            chunks.append(data.decode(errors="replace"))
            if "command>" in chunks[-1]:
                break
        except socket.timeout:
            break
    s.close()
    return parse_hex_blob("".join(chunks))


def wait_for_completion(timeout):
    """Poll 0xD000 until results are ready."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        data = zrcp_read_memory(RESULTS_ADDR, 64)
        if data and len(data) >= 3 and data[0] > 0:
            total = data[0]
            if len(data) >= total + 3 and data[total + 2] == SENTINEL:
                return data
        time.sleep(0.5)
    return None


def main():
    if len(sys.argv) < 2:
        print("Usage: run_tests.py <test.tap>")
        sys.exit(1)

    tap_file = os.path.abspath(sys.argv[1])
    if not os.path.exists(tap_file):
        print(f"FAIL: {tap_file} not found")
        sys.exit(1)

    zesarux_bin = os.environ.get("ZESARUX", "zesarux")
    rom = find_rom()
    if not rom:
        print("FAIL: cannot find 48.rom — set ZESARUX_ROM env var")
        sys.exit(1)

    cmd = [
        zesarux_bin,
        "--vo", "null",
        "--ao", "null",
        "--enable-remoteprotocol",
        "--machine", "48k",
        "--noconfigfile",
        "--romfile", rom,
        tap_file,
    ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    try:
        time.sleep(2)

        results = wait_for_completion(MAX_WAIT)
        if results is None:
            print("FAIL: timed out waiting for test completion")
            sys.exit(1)

        total = results[0]
        passed = results[1]
        per_test = results[2 : 2 + total]

        print(f"\n  Orientation tests: {passed}/{total} passed\n")
        all_ok = True
        for i, r in enumerate(per_test):
            name = TEST_NAMES.get(i, f"test {i}")
            status = "PASS" if r == 1 else "FAIL"
            if r != 1:
                all_ok = False
            print(f"  {status}  {i + 1:2d}. {name}")

        print()
        if all_ok:
            print("  All tests passed.")
        else:
            print(f"  {total - passed} test(s) FAILED.")
        sys.exit(0 if all_ok else 1)

    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()


if __name__ == "__main__":
    main()
