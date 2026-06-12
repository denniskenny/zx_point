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

Always launch with `--vo null --ao null` for headless operation and `--enable-remoteprotocol` for ZRCP access:

```bash
zesarux --vo null --ao null --enable-remoteprotocol --machine 48k \
  --noconfigfile --quickexit \
  --romfile ~/projects/zesarux/src/48.rom \
  program.tap
```

Launch as a background process. Connect to ZRCP on `localhost:10000` after a short settle delay (~3s). Always clean up with `pkill -f zesarux` when done.

## 2. ZRCP Connection

Connect via Python socket or netcat. The protocol is line-based: send command + newline, read until `command>` prompt.

### Key ZRCP commands

| Command | Purpose |
|---------|---------|
| `get-registers` | Dump all Z80 registers (parse `PC=`, `SP=`, etc.) |
| `get-tstates-partial` | T-states since last reset (for profiling) |
| `reset-tstates-partial` | Reset the partial T-state counter |
| `read-memory <addr> <len>` | Read memory as hex string |
| `write-memory <addr> <bytes>` | Write bytes to memory |
| `set-breakpoint <idx> PC=<addr>h` | Set a PC breakpoint |
| `enable-breakpoints` | Enable breakpoint system |
| `run` | Resume execution (returns on breakpoint hit) |
| `cpu-step` | Single-step one instruction |
| `enter-cpu-step` | Pause execution |
| `set-ui-io-ports <18 hex chars>` | Set keyboard/joystick state |
| `send-keys-string <ms> <chars>` | Type characters with delay |
| `save-screen <path>` | Save screen as .scr file |

## 3. Reading Screen Memory

ZX Spectrum screen layout:
- **Pixel RAM**: `0x4000`-`0x57FF` (6144 bytes)
- **Attribute RAM**: `0x5800`-`0x5AFF` (768 bytes, 32x24 grid)

Read via ZRCP:
```
read-memory 16384 6144    # pixels
read-memory 22528 768     # attributes
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
Use the `dump_screen_png()` method in `tools/profile_zrcp.py` or replicate it:
1. Read 6144 bytes from 0x4000 and 768 bytes from 0x5800
2. For each 8x8 character cell, look up ink/paper/bright from the attribute byte
3. For each pixel bit, choose ink (1) or paper (0) colour
4. Apply bright flag (adds 8 to colour index for brighter palette)

The ZX Spectrum palette (normal / bright):
```
0: (0,0,0)/(0,0,0)       Black
1: (0,0,0xCD)/(0,0,0xFF)  Blue
2: (0xCD,0,0)/(0xFF,0,0)  Red
3: (0xCD,0,0xCD)/(0xFF,0,0xFF) Magenta
4: (0,0xCD,0)/(0,0xFF,0)  Green
5: (0,0xCD,0xCD)/(0,0xFF,0xFF) Cyan
6: (0xCD,0xCD,0)/(0xFF,0xFF,0) Yellow
7: (0xCD,0xCD,0xCD)/(0xFF,0xFF,0xFF) White
```

### Attribute map (text dump)
For quick debugging without PIL, dump attributes as a text grid showing ink/paper per cell. Flag unexpected colours to identify rendering artifacts (e.g., attribute bleed from sprites).

## 4. Profiling Frame Timing

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
This produces `downship.map` with symbol addresses.

### Manual profiling via ZRCP
1. `enter-cpu-step` — pause
2. `set-breakpoint 1 PC=<addr>h` — set breakpoint at function entry
3. `reset-tstates-partial` — zero the counter
4. `run` — execute until breakpoint
5. `get-tstates-partial` — read elapsed T-states

## 5. Simulating Input

### Kempston joystick (port 0x001F, active-high)
| Bit | Direction |
|-----|-----------|
| 0 | Right |
| 1 | Left |
| 2 | Down |
| 3 | Up |
| 4 | Fire |

Use `set-ui-io-ports` with 9 hex bytes: 8 keyboard half-rows (all 0xFF = no keys pressed) + 1 joystick byte:
```
set-ui-io-ports ffffffffffffffff08    # Up (forward)
set-ui-io-ports ffffffffffffffff04    # Down (backward)
set-ui-io-ports ffffffffffffffff02    # Left
set-ui-io-ports ffffffffffffffff01    # Right
set-ui-io-ports ffffffffffffffff10    # Fire (descend)
set-ui-io-ports ffffffffffffffff18    # Up+Fire (forward+descend)
set-ui-io-ports ffffffffffffffff00    # Release all
```

Always release joystick (`ffffffffffffffff00`) when done testing.

### Keyboard (active-low half-rows)
The 8 keyboard bytes in `set-ui-io-ports` correspond to the 8 keyboard half-rows. Set a bit to 0 to press that key. The order matches ZX Spectrum port addresses from 0xFEFE to 0x7FFE.

### Typing text
```
send-keys-string 200 a    # press 'a' with 200ms delay
```
Useful for navigating menus and title screens.

## 6. Typical Test Workflow

1. **Build** with map file: `make clean && make USER_CFLAGS="-m"`
2. **Launch** ZEsarUX headless in background
3. **Connect** to ZRCP, wait for settle
4. **Navigate** past title/intro screens with `send-keys-string`
5. **Simulate** input if needed with `set-ui-io-ports`
6. **Wait** for game state to stabilise
7. **Screenshot** — read screen RAM, render to PNG, inspect
8. **Profile** — set breakpoints, measure T-states between waypoints
9. **Inspect** attributes — dump attribute map to check for artifacts
10. **Cleanup** — release joystick, close connection, kill emulator

## 7. Diagnosing Common Issues

- **Attribute artifacts**: Dump attribute map, look for unexpected ink/paper values outside sprite areas. Usually caused by screen address calculation bugs at third boundaries (y=64, y=128).
- **Frame overrun**: Profile shows >100% budget. Identify the most expensive segment and optimise.
- **Vsync glitch**: Tearing or flicker. Check that vsync wait happens before drawing, and that draw order matches the expected beam position.
- **Input not working**: Verify joystick byte is correct (active-high). Check that the game reads the Kempston port. Try keyboard half-rows as fallback.
