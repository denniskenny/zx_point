# They That Go Down To The Sea In Ships

A ZX Spectrum 48K game where the player controls a deep-sea diver exploring a 3D underwater world. Navigate using the minimap and sound cues to find treasure and avoid predatory sea creatures, then return to the surface before your oxygen runs out.

## Current State

### Implemented
- 3D starfield renderer with depth-dependent particle counts
- State machine: Title, Intro (level briefing), Game, Summary, Game Over
- Player movement with inertia and boundary bounce across a 32x3x32 grid (QAOP + WS / Kempston)
- Surfacing ends the level: success if all relics collected, game over if not
- Three depth layers with animated colour transitions (~0.15s)
- Treasure system: archaeological (depth 2) and flotsam (depth 0) with PRNG placement
- Treasure rendering: 32x32 sprites positioned relative to diver's sub-cube coordinates
- Predator system: rays (depth 0), sharks (depth 1), GOOs (depth 2, invisible/instant death)
- Predator rendering: 32x32 sprites with range-based visibility and relative positioning (green ink)
- Proximity-based damage: 1 HP lost per second of continuous predator contact
- Minimap: 32x32 pixel overhead view showing player (yellow), treasures (red), predators (green) at all depths
- Depth indicator bar adjacent to minimap
- HUD: oxygen bar (white) and health bar (red)
- Sound: sonar ping with echo, collection jingle (Taps arpeggio), low-pitched damage jingle
- Floating bus vsync with HALT fallback
- Direct screen RAM rendering (no SP1 dependency)

### Not Yet Implemented
- Sea line / sea floor rendering
- Boat/crane intro and level completion animations
- Large graphics (GOO, boat, title logo) with ZX0 compression
- Sea shanty music (Tritone beeper / AY)
- 128K enhancements
- Ship's log and lore tablet text

## Build

Requires z88dk with SDCC.

```
make clean && make        # builds starfield.tap
make run                  # launches in emulator
```

## Controls

- Q / Joystick Up: Ascend
- A / Joystick Down: Descend
- O / Joystick Left: Move left
- P / Joystick Right: Move right
- W / Fire 1: Move forward (into screen)
- S / Fire 2: Move backward (out of screen)
