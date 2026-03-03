# Implementation Plan: They That Go Down To The Sea In Ships

Ordered task list. Each task builds on previous ones. Check off as completed.

---

## Phase 1: Foundation

- [ ] **1.1 Project structure & build system**
  Reorganise into multi-file build. Create `src/` for C files, keep `include/` for headers. Update Makefile to compile and link multiple .c/.asm files. Add `config/game_config.h` with all configurable constants (NUM_STARS per depth, cube_distance, contact_distance, invulnerable_constant, ray_constant, time_limit, sonar ranges, etc.).

- [ ] **1.2 Game state machine**
  Create `src/state.c` / `include/state.h`. Implement the top-level state enum (STATE_TITLE, STATE_INTRO, STATE_GAME, STATE_SUMMARY, STATE_GAMEOVER) and a `state_run()` dispatcher in main(). Each state gets an `init()` and `tick()` function. Move the existing starfield/input loop into `state_game_tick()` as the starting point.

- [ ] **1.3 Game data structures**
  Create `include/game.h` with global game state: player position (grid x, y, z + sub-cube offsets), velocity, health (5 steps), oxygen (percentage), current depth layer (1-3), current level number, collected treasure counts, level treasure list, total treasure list. Define the 64x3x64 grid as a flat array of flags (treasure present, predator slot index).

- [ ] **1.4 128K detection stub**
  Create `src/hw_detect.c`. Write a 128K detection routine (page bank 7 and check) that sets a global `is_128k` flag. Call it once at startup. All 128K-conditional code will check this flag. No AY code yet — just the detection.

---

## Phase 2: Rendering Core

- [ ] **2.1 Depth layer attribute system**
  Create `src/depth.c` / `include/depth.h`. Implement the 3 depth layer colour palettes (Depth 1: cyan paper/white bright ink, Depth 2: blue/green, Depth 3: black/white). Implement the transition animation (colour cycling over 3 seconds = ~150 frames at 50Hz) for all 4 direction changes as specified in the design. Expose `depth_set(layer)` and `depth_transition(from, to)`.

- [ ] **2.2 Adapt starfield for depth layers**
  Refactor the existing starfield code into `src/starfield.c`. Make star count configurable per depth (100/50/15). Stars must be reseeded when changing depth. Ensure starfield is updated last in the frame (after sprites) as per design.

- [ ] **2.3 Sea line (sinewave)**
  Create `src/sealine.c` with an assembly-optimised sinewave renderer. The sea line appears when the player is in a Depth 1 cube. It animates (phase shifts each frame) and scrolls off-screen as the player descends. Stars and rays are culled above this line. The line position is a single y-coordinate for culling purposes. Flotsam treasure renders on this line.

- [ ] **2.4 Sea floor (flat line)**
  Add sea floor rendering to `src/sealine.c` (or a new file). A flat horizontal line visible when the player is in a Depth 3 cube. Scrolls up as the player descends toward the bottom. Archaeological treasure renders just above this line. The Great Old One is clipped to the sea floor scanline.

- [ ] **2.5 Floating bus flicker avoidance**
  Create `src/vsync.c` with an assembly routine that uses the floating bus technique (read port 0xFF, wait for non-contended period). Include a HALT fallback for emulators that don't support floating bus. This replaces the bare `halt` in the current main loop. Must be 48K/128K safe.

---

## Phase 3: Sprite System

- [ ] **3.1 SP1 library integration**
  Add SP1 to the Z88DK build flags. Configure SP1's invalidated-tiles region to cover the play area (rows 0-22, leaving row 23 for HUD). Create `src/sprites.c` / `include/sprites.h` with init/update/draw functions. SP1 will handle Player, Rays, Sharks, and Treasure sprites only.

- [ ] **3.2 Sprite mask generation at init**
  Write a utility function in `src/sprites.c` that takes a sprite bitmap and generates a 1-pixel-border mask at game initialisation. This is used by SP1 for masked sprite rendering.

- [ ] **3.3 Sprite horizontal mirroring at init**
  Write a utility function that mirrors a sprite bitmap horizontally. Called at init for predator sprites (rays, sharks) to generate left-facing versions from the right-facing .zxp source.

- [ ] **3.4 Sprite underwater shimmer effect**
  Write a utility function that generates the "shimmer" frame for treasure sprites: every second scanline offset by 1 pixel. Called at init for both archaeological and flotsam treasure sprites.

- [ ] **3.5 Player sprite via SP1**
  Move the diver from direct writes to SP1. The diver stays at screen centre (120, 88). Two animation frames, alternating every 8 frames. Always yellow ink, yellow bright in the top-left character cell. Sprite attributes are independent of depth layer.

- [ ] **3.6 Create all sprite .zxp assets**
  Using ZX-Paintbrush or manually, create the following .zxp files in `assets/`:
  - `ray.zxp` — 32x32, 2 frames (side view, right-facing)
  - `shark.zxp` — 32x32, 2 frames (side view, right-facing)
  - `statue.zxp` — 32x32, 2 frames (normal + shimmer)
  - `tablet.zxp` — 32x32, 2 frames (normal + shimmer)
  - `altar.zxp` — 32x32, 2 frames (normal + shimmer)
  - `firstaid.zxp` — 32x32, 2 frames (normal + shimmer)
  - `oxygen_tank.zxp` — 32x32, 2 frames (normal + shimmer)
  - `map.zxp` — 32x32, 2 frames (normal + shimmer)
  - `log.zxp` — 32x32, 2 frames (normal + shimmer)
  Update Makefile with all new asset rules and GENERATED_HEADERS list.

- [ ] **3.7 Update zxp2header.py for SP1 column-major output**
  Add a `--sp1` flag to `tools/zxp2header.py` that outputs sprite data in SP1's column-major format (columns of 8-pixel-wide strips) instead of row-major. Use this flag for all SP1 sprites.

---

## Phase 4: Gameplay Systems

- [ ] **4.1 Player movement & cube traversal**
  Implement player movement within the 64x3x64 grid. Movement keys/joystick adjust sub-cube position; when the player crosses a cube boundary, update grid coordinates. Starfield velocity is driven by player movement direction. Extract `cube_distance` constant (10 seconds horizontal, 20 seconds vertical). Depth changes trigger the depth transition from 2.1.

- [ ] **4.2 Camera / perspective system**
  When in-cube sprites (treasure, predators) share the player's grid cube, project them from their 3D sub-cube position to screen coordinates using the existing perspective projection. Sprites scale based on Z distance (or just appear at fixed 32x32 when in same cube, as per design). Sprites not in the current cube are invisible (only shown on minimap).

- [ ] **4.3 Treasure spawning & collection**
  Implement treasure placement at level start (in `state_intro_init()`). At least one archaeological treasure per level; extras randomly flotsam or archaeological. Flotsam sub-types are random. Place treasures at random grid positions respecting depth rules (flotsam at Depth 1 sea line, archaeological at Depth 3 sea floor). Collection triggers on contact_distance proximity (X/Y only). Collected treasures are added to the level list.

- [ ] **4.4 HUD: oxygen bar**
  Create `src/hud.c` / `include/hud.h`. Draw the oxygen bar as a blue horizontal tank shape in the bottom-left of the last character row. The bar depletes over the configurable time limit (default 3 minutes). When oxygen reaches 0, trigger level failure.

- [ ] **4.5 HUD: health bar**
  Draw the health bar as a red horizontal gauge next to the oxygen bar. 5 positions. Decreased by 1 on ray/shark contact. Replenished fully by first aid kit flotsam. When health reaches 0, trigger level failure.

- [ ] **4.6 Minimap**
  Create `src/minimap.c` / `include/minimap.h`. A 32x32 pixel XOR-drawn map in the bottom-right corner. White grid lines. Red dot for player (centre). White dots for treasure, rays, and sharks (not GOO). Updates every frame as entities move. Drawn last to overlay the play area. Maps the full 64x64 horizontal grid (depth is not shown).

---

## Phase 5: Predators

- [ ] **5.1 Rays — AI & rendering**
  Create `src/predators.c` / `include/predators.h`. Rays exist only at Depth 1. Swim diagonally, reverse at screen edges. Slower than the player. Show on minimap, no sonar sound. Rendered via SP1 when in the player's cube. 2-frame animation + horizontal mirror. Spawned at random positions within their depth at level start. Respawn if they leave the grid.

- [ ] **5.2 Sharks — AI & rendering**
  Sharks exist only at Depth 2. Swim diagonally by default but pursue the player when in adjacent cubes. Slower than the player. Show on minimap, no sonar sound. Rendered via SP1 when in the player's cube. 2-frame animation + horizontal mirror. Spawned at random positions within their depth.

- [ ] **5.3 Collision detection & damage**
  Implement overlap detection between player sprite and predator sprites (SP1 bounding box or pixel check). On collision: decrement health by 1, set invulnerability timer (invulnerable_constant frames, default 60). Play damage sound effect. Flash player sprite during invulnerability.

- [ ] **5.4 Great Old One — AI, rendering & death sequence**
  GOOs exist only at Depth 3. Move randomly, very slow. Do NOT appear on minimap. Have a sonar sound (see 6.2). Rendered via direct screen writes (not SP1) — 20x20 character bitmap. 3 frames: approaching, open maw, closed swallow. On collision: play the 3-frame death animation, trigger game over sequence. Clipped to sea floor scanline.

---

## Phase 6: Sound

- [ ] **6.1 Integrate Shiru's Tritone engine**
  Add Shiru's Tritone beeper engine to the project. Create `src/sound.c` / `include/sound.h` with `sound_init()`, `sound_play_music(track)`, `sound_play_sfx(id)`, `sound_stop()`. The engine runs via interrupts so gameplay continues during music. Music only plays in non-game states (title, summary, gameover). Sound effects play in all states.

- [ ] **6.2 Sonar ping system**
  Implement the sonar ping in `src/sonar.c`. Distance-based: ping every 2 seconds at 10 cubes, increasing to every 0.25 seconds at 1 cube. Uses an exponential curve. Volume/pitch increases with proximity. Uses cube_distance for calculation. Always overrides other sound effects. Used for Great Old Ones only. In 128K mode, uses AY instead of beeper (implemented in Phase 8).

- [ ] **6.3 Sound effects**
  Implement short beeper effects:
  - Treasure collection: short ascending Zelda-style jingle
  - Taking damage: short aggressive white noise burst
  - Level completion: 4-note trumpet fanfare (C-E-G-C')
  - Game over / funeral march: short descending tone
  All effects must be very short to minimise frame drops.

- [ ] **6.4 Sea shanties — 48K beeper versions**
  Convert the 3-channel ABC arrangements from Design.md into Shiru's Tritone format:
  - Óró Sé do Bheatha 'Bhaile (title screen, looping)
  - Lowlands Away (level summary, looping)
  - Spanish Ladies / Farewell To Spain (game over, play once at half tempo)
  Each uses all 3 Tritone channels (lead/harmony/bass).

---

## Phase 7: Game Screens

- [ ] **7.1 Title screen**
  Implement `state_title_init()` / `state_title_tick()`. Display the 256x100 "They That Go Down To The Sea" logo at screen top (decompressed from ZX0). Render the undulating sea line at centre. Render the boat graphic bobbing on it. Play Óró Sé do Bheatha 'Bhaile in a loop. Display "Copyright Actual Size 2026 / Press Fire or Any Key to Start" at the bottom. On any key/fire: transition to STATE_INTRO.

- [ ] **7.2 Game intro sequence**
  Implement `state_intro_init()` / `state_intro_tick()`. Continue playing the title shanty. Set depth to 1 (cyan/white). Sea line 2 character rows above centre. Render boat graphic on left of sea line. Crane extends from bow to centre. Pixel line descends from crane tip with diver sprite attached. When diver reaches screen centre: boat disappears, sea line/starfield activate, enable player controls, transition to STATE_GAME. Spawn predators and place treasure during init.

- [ ] **7.3 Level summary screen**
  Implement `state_summary_init()` / `state_summary_tick()`. Sea line in top third with boat bobbing. Loop Lowlands Away shanty. Display list of collected treasures below sea line. If a ship's log was collected, show a random log entry. If a tablet was collected, show a random lore entry. On key press: advance to next level (STATE_INTRO) or return to title (STATE_TITLE) if game over.

- [ ] **7.4 Game over sequence**
  Implement within `state_game_tick()` and transition to STATE_GAMEOVER.
  - Health/oxygen death: continuous high beeper tone, colour-cycle all attributes white→black over ~1 second (50 frames, cycle through intermediate colours).
  - Great Old One death: final GOO frame displayed, colour-cycle attributes, sonar continues for 1 second after final frame.
  Then transition to STATE_SUMMARY (which shows total treasures across all levels), then STATE_TITLE.

- [ ] **7.5 Level progression**
  Implement level data table: level number → name, treasure count, predator counts (rays = level × ray_constant, sharks, GOOs). Levels 1-5 as per design, level 6+ follow the "Descent into Madness" formula (+1 each type per level). On level completion (all archaeological treasure collected, player at top-centre cube, one char below sea line): disable controls, crane/ship appear, diver lifted out, transition to STATE_SUMMARY.

---

## Phase 8: Large Graphics & Compression

- [ ] **8.1 ZX0 compression pipeline**
  Integrate Einar Saukas' ZX0 compressor into the build system. Extend `tools/zxp2header.py` (or create a new `tools/zxp2zx0.py`) to output raw bytes, then pipe through `zx0` binary to produce compressed data, then wrap in a C array. Update Makefile with the pipeline: `.zxp → raw → ZX0 → .h`. Add a runtime `dzx0_standard` decompression routine (from ZX0 repo) in assembly.

- [ ] **8.2 Boat graphic**
  Create `assets/boat.zxp` — a ~100x100 pixel boat with crane, side profile. Convert via ZX0 pipeline. Write a `draw_boat()` function that decompresses to a screen-aligned buffer and blits to the correct position on the sea line. Used on title screen, intro, level completion, and summary.

- [ ] **8.3 Great Old One graphic**
  Create `assets/goo.zxp` — 20x20 character (160x160 pixel) angler fish face with 3 frames stacked vertically. Convert via ZX0 pipeline. Decompress frame-by-frame during the GOO encounter. Direct screen writes (no SP1).

- [ ] **8.4 Title logo**
  Create `assets/logo.zxp` — 256x100 pixel game title logo. Convert via ZX0 pipeline. Decompress to top of screen on title screen.

---

## Phase 9: 128K Enhancements

- [ ] **9.1 128K memory banking**
  Extend `src/hw_detect.c` with bank-switching routines. Use extra RAM banks to store compressed music data and large graphics that don't fit in 48K. Define a memory map for which banks hold what.

- [ ] **9.2 AY sea shanties**
  Author the 3 sea shanties as 3-channel AY arrangements in Vortex Tracker II, export as .pt3 files. Integrate a Z88DK .pt3 player routine. When `is_128k` is true, play AY music instead of Tritone beeper music. Mute beeper melodies but keep beeper SFX (treasure, damage).

- [ ] **9.3 AY sonar ping**
  Create an AY version of the sonar ping with envelope shaping (muffled high-end, reverb effect). Use instead of the beeper sonar when `is_128k` is true.

---

## Phase 10: Polish & Integration

- [ ] **10.1 Floating bus testing**
  Test the floating bus routine on real hardware and multiple emulators (Fuse, SpectacUm, ZX Bare Metal). Verify the HALT fallback works correctly when floating bus is unsupported.

- [ ] **10.2 Gameplay balancing**
  Tune all configurable constants via `config/game_config.h`: time limit, cube traversal speed, predator speeds, sonar curve, invulnerability duration, ray multiplier. Playtest each level 1-5 and verify the difficulty curve.

- [ ] **10.3 Memory audit**
  Profile RAM usage. Verify everything fits in 48K (with ZX0 compression for large graphics). Ensure 128K banks are used efficiently. Check stack depth. Trim any unnecessary data.

- [ ] **10.4 Final build & packaging**
  Ensure `make clean && make` produces a working .tap file. Test on Fuse and at least one other emulator. Create a 128K .tap variant. Verify all game states, all 5+ levels, all sound, all transitions work end to end.

---

## Dependency Summary

```
Phase 1 (Foundation) → everything
Phase 2 (Rendering) → Phase 3, 4, 5, 7
Phase 3 (Sprites)   → Phase 4.2, 4.3, 5.x
Phase 4 (Gameplay)  → Phase 5, 7
Phase 5 (Predators) → Phase 7
Phase 6 (Sound)     → Phase 7
Phase 7 (Screens)   → Phase 8 (large graphics needed for title/intro/summary)
Phase 8 (Graphics)  → Phase 10
Phase 9 (128K)      → Phase 10 (can be done in parallel with Phase 7-8)
Phase 10 (Polish)   → ship it
```
