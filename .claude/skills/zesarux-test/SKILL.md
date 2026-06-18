---
name: zesarux-test
description: Launch ZEsarUX headless, profile frame timing, inspect screen memory, and simulate input via ZRCP. Use when testing, profiling, debugging rendering, or checking screen output of a ZX Spectrum program.
when_to_use: "profile the frame" or "check the screen" or "test in emulator" or "inspect attributes" or "run the profiler" or "screenshot"
allowed-tools: Bash Read Write Edit
effort: high
---

# ZEsarUX Testing, Profiling & Screen Inspection

You have access to ZEsarUX 13.0 with ZRCP (remote control protocol) for automated testing of ZX Spectrum programs. Use these capabilities to test, profile, and debug any program loaded as a .tap file.

## Environment

- **ZEsarUX binary**: `/usr/local/bin/zesarux` (built from source at `/Users/Kennyd/projects/zesarux/`)
- **48K ROM**: `~/projects/zesarux/src/48.rom`
- **ZRCP port**: TCP 10000 (localhost)
- **Profiler script**: `tools/profile_zrcp.py` (project-specific, uses map file symbols)

## 1. Launching ZEsarUX Headless

Always launch with `--vo null --ao null` for headless operation and `--enable-remoteprotocol` for ZRCP access. Use **absolute paths** for the .tap file — relative paths fail in headless mode:

```bash
zesarux --vo null --ao null --enable-remoteprotocol --machine 48k \
  --noconfigfile --quickexit \
  --romfile ~/projects/zesarux/src/48.rom \
  /Users/Kennyd/projects/zx_point/downship.tap &
```

Launch as a background process with `&`. Wait **6 seconds** before connecting to ZRCP — the emulator needs time to start up. Always clean up with `pkill -f zesarux` when done.

### Smartload alternative

After connecting via ZRCP, you can also load a .tap file dynamically:
```
smartload /Users/Kennyd/projects/zx_point/downship.tap
```
Then wait **8–10 seconds** for the tape to autoload before interacting.

## 2. ZRCP Connection

Connect via Python socket. The protocol is line-based: send command + newline, read until `command>` prompt.

### Python connection template
```python
import socket

def zrcp():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(15)
    s.connect(('localhost', 10000))
    buf = b''
    while b'command>' not in buf:
        buf += s.recv(4096)
    return s

def cmd(s, c):
    s.sendall((c + '\n').encode())
    buf = b''
    while b'command>' not in buf:
        buf += s.recv(4096)
    text = buf.decode('latin-1', errors='replace')
    idx = text.rfind('command>')
    if idx >= 0:
        text = text[:idx]
    return text.strip()
```

### Key ZRCP commands

| Command | Purpose |
|---------|---------|
| `get-registers` | Dump all Z80 registers (parse `PC=`, `SP=`, etc.) |
| `get-tstates-partial` | T-states since last reset (for profiling) |
| `reset-tstates-partial` | Reset the partial T-state counter |
| `read-memory <addr> <len>` | Read memory as hex string |
| `write-memory <addr> <byte> [<byte>...]` | Write bytes to memory (space-separated) |
| `write-memory-raw <addr> <hexbytes>` | Write bytes to memory (no separators) |
| `hexdump <addr> <len>` | Hex + ASCII dump at address |
| `set-breakpoint <idx> PC=<addr>h` | Set a PC breakpoint |
| `enable-breakpoints` | Enable breakpoint system |
| `run` | Resume execution (returns on breakpoint hit) |
| `enter-cpu-step` | Pause execution |
| `cpu-step` | Single-step one instruction (must `enter-cpu-step` first) |
| `set-ui-io-ports <18 hex chars>` | Set keyboard/joystick state |
| `smartload <path>` | Load a .tap/.tzx file |
| `save-screen <path>` | Save screen as .scr file |

## 3. Memory Read/Write — Address Formats

**IMPORTANT**: `read-memory` and `write-memory` use **decimal** addresses and lengths:
```
read-memory 16384 6144      # 0x4000, 6144 bytes — correct
read-memory 0x4000 0x1800   # WRONG — don't use hex
```

`hexdump` uses **hex** addresses and **decimal** lengths:
```
hexdump 4000 16             # hex address, decimal length
hexdump EB9B 15             # hex address, 15 bytes
```

### Reading memory in Python
```python
def read_bytes(s, addr_decimal, n):
    """Read n bytes from a decimal address. Returns bytes object."""
    resp = cmd(s, f'read-memory {addr_decimal} {n}')
    cleaned = resp.replace(' ', '').replace('\n', '').replace('\r', '')
    return bytes.fromhex(cleaned[:n * 2])
```

The response from `read-memory` is hex bytes (space-separated or continuous depending on length). Strip whitespace before parsing.

### Writing memory
```
write-memory 60315 16       # write byte 16 to decimal address 60315
write-memory 60315 0 0 17   # write three bytes starting at 60315
write-memory-raw EC15 0011  # hex address, hex bytes without spaces
```

`write-memory` takes decimal addresses and decimal byte values. `write-memory-raw` takes hex addresses and hex byte values.

**Caution**: The game loop runs continuously at 50fps. Values written to game variables may be overwritten on the next frame. For reliable modification:
- Write values that the game loop doesn't actively recompute (e.g., entity positions between grid-move intervals)
- Or hold the write in a loop / accept that it's a one-frame poke

## 4. Reading Screen Memory

ZX Spectrum screen layout:
- **Pixel RAM**: `0x4000`-`0x57FF` (6144 bytes)
- **Attribute RAM**: `0x5800`-`0x5AFF` (768 bytes, 32x24 grid)

Read via ZRCP (decimal addresses):
```
read-memory 16384 6144    # pixels (0x4000)
read-memory 22528 768     # attributes (0x5800)
```

### Screen address calculation
The pixel address for screen coordinate (x, y) where x is in columns (0-31) and y is in pixels (0-191):
```
third = y >> 6                    # 0, 1, or 2
char_row = (y >> 3) & 0x07       # 0-7 within third
pixel_row = y & 0x07             # 0-7 within char cell
addr = 0x4000 | (third << 11) | (pixel_row << 8) | (char_row << 5) | column
```

### Attribute byte format
```
Bit 7: Flash
Bit 6: Bright
Bits 5-3: Paper colour (0-7)
Bits 2-0: Ink colour (0-7)
```
Colours: 0=Black 1=Blue 2=Red 3=Magenta 4=Green 5=Cyan 6=Yellow 7=White

### Rendering screen to PNG
```python
from PIL import Image

def render_screen_png(pixels, attrs, path):
    PALETTE = [
        (0,0,0), (0,0,0xCD), (0xCD,0,0), (0xCD,0,0xCD),
        (0,0xCD,0), (0,0xCD,0xCD), (0xCD,0xCD,0), (0xCD,0xCD,0xCD),
        (0,0,0), (0,0,0xFF), (0xFF,0,0), (0xFF,0,0xFF),
        (0,0xFF,0), (0,0xFF,0xFF), (0xFF,0xFF,0), (0xFF,0xFF,0xFF),
    ]
    img = Image.new('RGB', (256, 192))
    for py in range(192):
        t = py >> 6; cr = (py >> 3) & 7; pr = py & 7
        for col in range(32):
            a = (t << 11) | (pr << 8) | (cr << 5) | col
            byte = pixels[a]; attr = attrs[(py >> 3) * 32 + col]
            ink = attr & 7; paper = (attr >> 3) & 7
            bright = (attr >> 6) & 1
            if bright: ink += 8; paper += 8
            for bit in range(8):
                color = PALETTE[ink] if byte & (0x80 >> bit) else PALETTE[paper]
                img.putpixel((col * 8 + bit, py), color)
    img.save(path)
```

### Detecting XOR sprites via multi-sampling

XOR sprites are drawn and erased each frame, so a single attribute read may miss them. Sample attributes multiple times and take the union of all non-background cells:
```python
def sample_attr_cells(s, ink_value, n_samples=10):
    """Return set of (row, col, attr) tuples where ink matches across n_samples."""
    found = set()
    for _ in range(n_samples):
        at = read_bytes(s, 22528, 768)
        for r in range(20):
            for c in range(32):
                if (at[r * 32 + c] & 0x07) == ink_value:
                    found.add((r, c, at[r * 32 + c]))
    return sorted(found)
```

### Attribute map (text dump)
For quick debugging without PIL, dump attributes as a text grid showing ink/paper per cell. Flag unexpected colours to identify rendering artifacts (e.g., attribute bleed from sprites).

## 5. Profiling Frame Timing

The 48K ZX Spectrum runs at 50Hz with **69,888 T-states per frame**. The game loop must fit within this budget.

### Using profile_zrcp.py
```bash
python3 tools/profile_zrcp.py --frames 5 --mapfile downship.map
```

Options:
- `--frames N` — number of frames to measure (default 3)
- `--mapfile FILE` — symbol map from z88dk build (build with `-m` flag)
- `--tap FILE` — .tap file to load (default: downship.tap)
- `--settle N` — seconds to let the game reach the main loop (default 8)
- `--motion HH` — hold Kempston joystick byte (hex) during test
- `--screenshot PATH` — save screen as PNG before profiling

The profiler sets breakpoints at known function entry points (waypoints) and measures T-states between them. It reports per-segment costs and total frame utilisation.

### Building with map file
```bash
make clean && make USER_CFLAGS="-m"
```
This produces `downship.map` with symbol addresses (prefixed with `_` for C symbols, e.g., `_player`, `_predators`).

### Manual profiling via ZRCP
1. `enter-cpu-step` — pause
2. `set-breakpoint 1 PC=<addr>h` — set breakpoint at function entry
3. `reset-tstates-partial` — zero the counter
4. `run` — execute until breakpoint
5. `get-tstates-partial` — read elapsed T-states

## 6. Simulating Input

### Always prefer Kempston joystick over keyboard

Kempston input via `set-ui-io-ports` is reliable and works for both gameplay and menu navigation (any game that checks `has_kempston`). Keyboard half-rows are harder to get right and less reliable in headless mode. **Default to Kempston for all input.**

### Kempston joystick (port 0x001F, active-high)
| Bit | Direction | Hex |
|-----|-----------|-----|
| 0 | Right | 01 |
| 1 | Left | 02 |
| 2 | Down | 04 |
| 3 | Up | 08 |
| 4 | Fire 1 | 10 |
| 5 | Fire 2 | 20 |

Use `set-ui-io-ports` with 9 hex bytes: 8 keyboard half-rows (all `FF` = no keys) + 1 Kempston byte:
```
set-ui-io-ports ffffffffffffffff08    # Up (forward)
set-ui-io-ports ffffffffffffffff04    # Down (backward)
set-ui-io-ports ffffffffffffffff02    # Left
set-ui-io-ports ffffffffffffffff01    # Right
set-ui-io-ports ffffffffffffffff10    # Fire1 (descend)
set-ui-io-ports ffffffffffffffff20    # Fire2 (ascend)
set-ui-io-ports ffffffffffffffff18    # Up+Fire1 (forward+descend)
set-ui-io-ports ffffffffffffffff00    # Release all
```

**Always release** (`ffffffffffffffff00`) when done or between distinct actions.

### Button debounce

Many game screens use debounce: they ignore held keys until a release is seen. To reliably press a button:
```python
cmd(s, 'set-ui-io-ports ffffffffffffffff00')  # release
time.sleep(0.5)                                # let debounce clear
cmd(s, 'set-ui-io-ports ffffffffffffffff10')  # press
time.sleep(1.0)                                # hold for the game to see it
```

### Keyboard (active-low half-rows) — fallback only

The 8 keyboard bytes in `set-ui-io-ports` correspond to port addresses 0xFEFE through 0x7FFE. Set a bit to 0 to press that key. Prefer Kempston instead.

## 7. Navigating Game Screens

The game has multiple states: TITLE → INTRO → GAME → SUMMARY/GAMEOVER. You must navigate through them before testing gameplay.

### Startup sequence
```python
# After smartload + 10s wait:
cmd(s, 'set-ui-io-ports ffffffffffffffff00')  # ensure released
time.sleep(1)
cmd(s, 'set-ui-io-ports ffffffffffffffff10')  # Fire: past title
time.sleep(1)
cmd(s, 'set-ui-io-ports ffffffffffffffff00')  # release for debounce
time.sleep(0.5)
cmd(s, 'set-ui-io-ports ffffffffffffffff10')  # Fire: past intro → GAME + descend
time.sleep(3)                                  # dive below surface
```

### Verifying game state

Check whether you're actually in gameplay:
- **Attribute test**: In-game screens have many cyan (0x29) attr cells. Title/summary screens are mostly black (0x00/0x38):
  ```python
  at = read_bytes(s, 22528, 768)
  cyan_count = sum(1 for b in at if b == 0x29)
  in_game = cyan_count > 200
  ```
- **Player health/oxygen**: Read the player struct — `health > 0` and `oxygen > 0` means the game is running.
- **Screenshot**: Render to PNG and visually inspect via the Read tool (which can display images).

### Surfacing trap

At depth gy=0, if `player.at_bound_y == 1` (player at the very top of the grid), the game immediately transitions to SUMMARY. This creates a rapid GAME→SUMMARY→GAME loop. To stay in GAME state at the surface:
- Hold Kempston Fire (0x10 = descend) to keep the player diving
- Or write `player.sub_z` to a non-zero value and `player.at_bound_y` to 0

### Using map file addresses
Build with `make USER_CFLAGS="-m"` to produce `downship.map`. Look up symbol addresses:
```bash
grep '_player\b' downship.map    # → _player = $EB9B (= 60315 decimal)
grep '_predators\b' downship.map # → _predators = $EC15 (= 60437 decimal)
```
Convert hex to decimal for `read-memory`/`write-memory`.

## 8. Typical Test Workflow

1. **Build** with map file: `make clean && make USER_CFLAGS="-m"`
2. **Launch** ZEsarUX headless in background with absolute .tap path
3. **Wait 6 seconds** for emulator startup
4. **Connect** to ZRCP via Python socket
5. **Smartload** the .tap file (or pass it on the command line)
6. **Wait 8–10 seconds** for tape autoload to complete
7. **Navigate** past title/intro with Kempston Fire (release/press cycle for debounce)
8. **Hold Fire** for 3+ seconds to dive below the surface
9. **Verify** game state via attrs or player struct
10. **Test** — inspect screen, profile, write memory to set up test conditions
11. **Cleanup** — release joystick, close socket, `pkill -f zesarux`

**Do all work in a single Python script** to maintain the ZRCP connection and input state across steps. Splitting across multiple script invocations loses `set-ui-io-ports` state.

## 9. Diagnosing Common Issues

- **Attribute artifacts**: Dump attribute map, look for unexpected ink/paper values outside sprite areas. Usually caused by screen address calculation bugs at third boundaries (y=64, y=128).
- **Frame overrun**: Profile shows >100% budget. Identify the most expensive segment and optimise.
- **Vsync glitch**: Tearing or flicker. Check that vsync wait happens before drawing, and that draw order matches the expected beam position.
- **Input not working**: Use Kempston (`set-ui-io-ports` with joystick byte), not keyboard. If Kempston Fire doesn't advance screens, try release/press cycle for debounce. Verify `has_kempston` is set in the game.
- **Blank/wrong screen after smartload**: The tape hasn't finished loading. Wait longer (10+ seconds). Check `get-registers` — if PC is in ROM (< 0x8000), the game hasn't started yet.
- **Game keeps returning to summary**: The player is at the surface with `at_bound_y=1`. Hold Fire to descend, or write `sub_z` > 0 and `at_bound_y = 0`.
- **XOR sprites invisible in screenshots**: XOR sprites exist for only half each frame (between draw and next-frame erase). Use multi-sampling (read attrs 10-15 times) to reliably detect them.
- **write-memory seems to have no effect**: Check that you're using decimal addresses. The game may also overwrite the value on the next frame — verify by reading back immediately.
