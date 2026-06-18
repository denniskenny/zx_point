# They That Go Down To The Sea In Ships : The Game

## Description
They That Go Down is a ZX Spectrum game where the player controls a deep sea diver in a 3d space. The player must use the minimap and sound cues to find treasure and avoid predatory fish. 

There is a time limit within which the diver must find as much treasure as possible and return to the surface. 

Each succesful trip leads to a further trip with more treasure items to collect and more predatory fish to avoid. Levels have unique titles that reveal more information about the source of the treasure.

## Heads Up Display

The time limit is represented by a white percentage bar shaped like a horizontal oxygen tank in the bottom left corner of the screen. The time limit should be configurable, initially using a value of 3 minutes.

Health is indicated by a red bar shaped like a horizontal gauge next to the time limit bar. It has five positions.

Both bars use the current depth paper colour as their background (via `depth_get_paper()`), ensuring they blend with the starfield at all depths.

The blue and red bars should leave enough room for the 32x32 minimap in the bottom right corner of the screen.

## Controls

The player controls the diver with either keys or joystick, moving the player left, right, forward, and backward. The fire button causes the player to descend, allowing for full navigation of the 3d space.

The controls are

Q / Joystick Up     - Move Forward (into screen)
A / Joystick Down   - Move Backward (out of screen)
O / Joystick Left   - Move Left
P / Joystick Right  - Move Right
Z / Fire Button 1   - Descend
X / Fire Button 2   - Ascend

By default the diver will gradually increase velocity to a maximum speed vertically. This can be countered by pressing Descend. To avoid ending the game instantly, the diver must begin the game at the bottom of the first subcube.

Movement has inertia: holding a key accelerates the diver up to maximum speed; releasing the key causes the diver to decelerate gradually to a stop (friction applied every 8th frame). The starfield scrolls to match the diver's velocity, creating the illusion of movement. When the diver reaches a world boundary, the starfield velocity reverses on that axis, creating a bounce effect that decays with the normal friction.

The fire button / Z control behaves differently; releasing this will cause the diver to slow down and then begin to float upwards. 

## Gameplay
The game begins with a short animation that shows the player descending from their ship and. their oxygen gauge filling up.

The mini-map is a grid in the lower right hand corner of the screen. It shows the player's location in relation to the ship and the treasure. The player must use the minimap to navigate to the treasure.

Predators also randomly appear in the 3d space and must be avoided. 

* Level One "The Deep": One piece of treasure, no predators.
* Level Two "The Reef": Two pieces of treasure, rays.
* Level Three "The Shipwreck": Three pieces of treasure, rays, a single shark.
* Level Four "The Ruins": Four pieces of treasure, rays, two sharks.
* Level Five "Five Fathoms Deep": Five pieces of treasure, rays, two sharks, A Great Old One.
* Further Levels "Descent into Madness": increment +1 the pieces of treasure to collect, increment +1 the number of creatures (rays, sharks and old ones) at each depth.

In the context of a level, 'rays' refers to the level number multiplied by a rays constant. The constant is currently defined as 10 but will be configurable.

### Treasure
Treasure is indicated on the 2d minimap, but it is up to the player to guess the depth it is at. Treasure can be collected by contact with the player. Contact is defined as within contact_distance of the player sprite, where contact_distance is a configurable parameter that applies to the X and Y coordinates. The Z coordinate is not checked as the plane is 2d.

Initially there is one piece of treasure to collect but each subsequent level will have an additional piece of treasure.

## Flotsam Treasure: 32x32 pixel bitmaps
Floating treasure consists of parts of boats that previously sank. These are optional. 

* First aid kits - fully replenish the diver's health
* Oxygen tanks - fully replenish the diver's oxygen
* Maps - add one additional flotsam treasure and display on the minimap
* Ships Logs - additional story and lore

## Archaeological Treasure: 32x32 pixel bitmaps
These are ancient artifacts from the sunken city of R'lyeh. All archaeological treasure must be collected to progress to the next level. 

* Statues : A greek statue of a man
* Tablets : A stele with cuneiform script
* Altars : A Roman altar.

### Predators
There are three types of predators. Rays and sharks are rendered as 32x32 sprites in green ink (brightness inherited from the current depth layer) when within PRED_VISIBLE_RANGE (4 grid cells) at the same depth as the player. Their screen position is computed relative to the player's sub-cube coordinates, using the same dynamic positioning system as treasures.

All predators appear on the minimap at all depths as green dots.

Damage is proximity-based: when a ray or shark remains within CONTACT_DISTANCE of the player for PRED_PROXIMITY_FRAMES (50 frames = 1 second), 1 HP is subtracted. A low-pitched Taps jingle plays on damage. The player is then invulnerable for invulnerable_constant frames (default 60).

* Predator Constants
invulnerable_constant : The number of frames the player is invulnerable after taking damage. Default to 60.
PRED_VISIBLE_RANGE : Grid cells within which predator sprites are rendered. Default 4.
PRED_PROXIMITY_FRAMES : Consecutive frames of contact before damage. Default 50 (1 second).

* Rays : 32x32 pixel bitmap of a ray fish
These appear on the minimap as green dots, have no sonar sound and move in a random direction. They only exist at depth 0 (shallow water). They are slower than the player.

Rays are rendered from the side and should have frames for both left and right directions. They traverse the grid diagonally, bouncing at grid boundaries.

* Sharks : 32x32 pixel bitmap of a shark

Sharks appear on the minimap as green dots, have no sonar sound but will move towards the player once the player is within PRED_SHARK_PURSUE_RANGE (2 cubes). They only exist at depth 1. They are slower than the player.

Sharks are rendered from the side and should have frames for both left and right directions. They traverse the grid diagonally, bouncing at grid boundaries, and pursue the player when nearby.

* Great Old Ones : 20x20 character bitmap of a great old one

Great Old Ones appear on the minimap at all depths as a green dot with the FLASH attribute set (hardware flash, not programmatic). They have a sonar sound and move in a random direction. They are very slow but cause instant death if the player enters their grid cell at depth 2.

Great Old Ones are rendered as a large angler fish face approaching the screen. The graphic will be 20x20 characters in size and have three frames; approaching, open maw and closed swallow. If the player enters a frame with an old one, the game will play this animation and end (see Level Failure). (GOO sprite not yet implemented.)

### Game Title screen
Displays a "They That Go Down To The Sea" 256x100 pixel logo at the top of the screen.

Displays the undulating sea line at the centre of the screen with the large boat graphic bobbing on it

Plays a sea shanty in a loop on the beeper (see 'Sea Shanties')

A the bottom the text should say "Copyright Actual Size 2026 \n Press Fire or Any Key to Start"

### Level Summary screen
The summary screen will display the undulating sea line in the top third with the large boat graphic bobbing on it.

The beeper will loop through a sea shanty (see 'Sea Shanties').

A list of collected treasures will appear  underneath the sea line.

If one of the treasures is a ships log, a random log entry will appear from the list.
If one of the treasures is a tablet, a random lore entry will appear from the list.

If the game is over, the list of treasures will be a total from all the levels.

### Game Intro
Spawning predators and placing treasure happens here. The intro screen displays a level briefing showing the level number, level name, treasure count, relic count, and predator counts (rays, sharks, GOOs — only shown if > 0). The player presses any key to begin.

The title sea shanty should continue playing here (not yet implemented).
The full animation sequence (below) is not yet implemented. Currently a text briefing screen is shown.
The game opens with the sea level set to Depth 1 (Cyan and White). Player controls are disabled.
The sea line is initially set at two characters above the centre of the viewport.
A large 100x100 pixel graphic of a boat with a crane will appear in profile on the left of the screen. it will be rendered at a point of the undulating surface line, bobbing up and down.

The crane will extend from the bow of the ship to the centre of the screen

A pixel line will extend from the end of the crane with the player character rendered at the end of the line.

Once the pixel line descends far enough that the player character is at the centre of the screen, the boat disappears and player control is enabled.

### On Surfacing
When the player returns to the surface (depth 0, top of sub-cube):

* **All relics collected**: Level Complete — player advances to the next level via the summary screen.
* **Relics missing**: Surfaced Early — the summary screen shows "Surfaced Early" with collected/total counts. Pressing any key (or Kempston fire) returns the player to the game with replenished oxygen to continue the same level. The player's grid position, health, and collected treasures are preserved; only oxygen is refilled. The diver re-enters at the top of depth 0.

The player does not need to collect all treasures (flotsam is optional), but all archaeological relics must be recovered for a successful completion.

Future implementation: the ship and crane will reappear, a pixel line will descend, and the diver will be lifted out of the water.

### On Level Failure
1. **Surfacing without relics**: the player is shown the summary screen and may continue the level (see On Surfacing above).

2. **Health or oxygen depleted**: the beeper emits a continuous high tone and the screen colour-cycles through all attributes from white to black over approximately 1 second (8 steps, 48 frames). The player is taken to the summary screen showing "Game Over", then any key returns to the title screen.

3. **Great Old One collision**: instant death at depth 2. The death animation plays. (GOO-specific death animation with sonar not yet implemented.) The player is taken to the summary screen then the title screen.

## Technical Implementation
The game will be implemented using the ZX Spectrum 48k. The game will be written in C with assembly language routines for the critical sections.

###  Architecture
* The game should maintain a simple state to switch between
    - Title Screen
    - Game Intro
    - Game 
    - Level Summary
    - Game Over
* The main loop should be written in C
* Variables, data structures and graphics should be global where possible.
* Features like the Sea Line and Sea Floor should be implemented in assembly and extracted as functions
* The game should use the Floating Bus port to avoid flicker. This should be 48k/128k safe and have a HALT fallback in case the game is in an emulator that doesn't support the Floating Bus trick.
* Use direct writes for the starfield, Boat, Great Old One,and sprites (Player, Rays, Sharks and Treasures).
* Plan to use Einar's compression library for the Great Old One and the Boat graphics;  https://github.com/einar-saukas/ZX0

### Sound
All beeper effects should be very short to minimise frame drops.
Use Shiru's Tritone engine for the beeper melodies (to emulate the AY 3 channels), Sonar Ping and effects.
Use the Vortex Tracker II → .pt3 format with an existing Z88DK player routine, for AY music.

#### Sonar Ping
This is the most important sound effect in the game. 

It will use the ZX Spectrum 48k built-in sound chip with Shiru's Engine to generate the sonar sound for predators or the 128k AY chip with Vortex Tracker II if it is available. The code for this  should be stored in its own file.

The AY sound should have an envelope that muffles the high end and has a reverb effect to simulate the sound of the sonar ping.

The Sonar Ping should always override any other sound effects.

Music is never played in the game state, when the Sonar Ping effect is used.

##### Sonar Targeting
The sonar tracks only the single closest object to the player at the same depth level. Objects on different depth levels are ignored. If a treasure is 3 cubes away on depth 1 and a shark is 5 cubes away on depth 1, only the treasure is tracked. If the shark is on depth 2 and the player is on depth 1, the shark is not tracked at all.

The target type determines the sonar pitch:
* Treasure — highest pitch (delay 15). The player's primary navigation aid.
* Predator (ray or shark) — medium pitch (delay 25). Warning of nearby danger.
* Great Old One — lowest pitch (delay 40). Ominous warning of lethal threat.

The pitch change allows the player to distinguish what they are approaching by ear alone, which is critical since GOOs are invisible.

##### Distance Mapping
Sonar ping frequency increases as the player approaches the target using a quadratic curve. The range in cubes is configurable (SONAR_RANGE_FAR, default 10). The ping interval ranges from every 2 seconds at 10 cubes away (SONAR_INTERVAL_FAR=100 frames) to every 0.25 seconds at 1 cube away (SONAR_INTERVAL_NEAR=12 frames). 


#### Additional sound effects
- Collecting treasure: Fast Taps-inspired bugle arpeggio (G-G-C, G-C-E at G5/C6/E6)
- Taking damage: Low-pitched Taps jingle (same pattern, ~1 octave lower, delays 255/206/164)

#### 48k Melodies
These will be played using interrupts so that keyboard polling, sea line animation and sprite updates can continue.
- Level completion: simulated trumpet fanfare
- Game over: simulated funeral march
- Title Screen: looping sea shanty

#### 128K support
The game should have a start up function to detect if it is running in 128k mode. If it is
- It should use the 128k memory map to access the additional memory.
- The sea shanties should be played in a 3-channel AY arrangement (harmony/bass/lead), muting the beeper versions
- An AY version of the sonar ping should be used instead of the beeper version.
- 48k treasure and damage sound effects should still be played, while the AY music plays

### Spawning & AI
* There must always be at least one archaeological treasure per level.
* If there are multiple treasures, they will be randomly flotsam or archaeological.
* Flotsam items are randomly selected from the 4 types.
* Shark proximity trigger — The shark will start pursuing the player if they are in adjacent cubes
* Predator spawning — Predators start at random positions within their depth level. They respawn if they leave the grid.

### Screen Space
The 3d space will consist of a starfield background and animated sprite of the player in the centre of the screen. 

The player will be rendered as a 2d 16x16 pixel sprite in the centre of the screen. 

When the player moves, the player sprite stays in the same position and the starfield and other sprites move to create the illusion of movement.

The 3d space occupies the top 160 pixel rows of the screen (char rows 0-19). The bottom 32 rows (char rows 20-23, pixels y 160-191) are reserved for the HUD gauges and minimap. The starfield engine viewport height is capped at 160 so no per-pixel bounds testing is needed for the HUD/minimap region.

#### Treasure and Predator 3D Positioning
Treasures and predators are rendered using all three world axes mapped to screen coordinates:

* **X axis** (grid X distance from player) → **Screen X position**. Objects to the player's left appear on the left of the screen and vice versa. Computed from the sub-cube X offset between the player and the object. This is the same as the existing behaviour.

* **Y axis** (depth) → **Screen Y position**. Objects are pinned to their depth-level environmental feature:
  - Depth 0 (surface): Flotsam treasures and rays render on or near the sea line Y position.
  - Depth 1 (mid-water): Sharks render at a screen Y derived from the player's sub_z position within the current cube, centred vertically in the viewport.
  - Depth 2 (sea floor): Archaeological treasures and GOOs render at the sea floor line Y position.

* **Z axis** (grid Z distance, forward/backward into screen) → **Sprite scale**. Objects further away in Z appear smaller, using pre-calculated scaled sprite variants. There are four scale levels, selected by Chebyshev distance in Z from the player:
  - **Close** (0-1 cubes): Full 32x32 sprite — maximum detail.
  - **Medium** (2 cubes): 16x16 scaled sprite — half size.
  - **Small** (3 cubes): 8x8 scaled sprite — quarter size.
  - **Far** (4+ cubes): 4x4 pixel block — a single coloured block using the appropriate attribute (green for predators, white for treasure). This is a common graphic shared by all predator and treasure types.

This replaces the previous system where Z distance was mapped to screen Y position. The new model gives a clearer sense of depth: objects approach from the distance and grow larger, matching the bubblefield's existing perspective model.

#### Sprite Scaling Pipeline
The 16x16 and 8x8 scaled variants are pre-calculated at build time from the 32x32 source sprites using nearest-neighbour downsampling. This is a Makefile task (`make assets`) that runs a Python tool over each sprite's .zxp asset:

* Input: 32x32 sprite frames (from .zxp files)
* Output: Additional frame arrays in the generated C headers (e.g. `statue_f1_16`, `statue_f1_8`)
* The 4x4 far block requires no per-sprite asset — it is a hardcoded pixel pattern with the appropriate colour attribute set at runtime.

The scaling tool must preserve the 1-pixel mask border at each scale level to prevent pixel bleeding against the background.

### Game Space
The world space will be a 3d grid of 32x3x32 (Width x Depth x Height) cubes. The player will always start in the top centre subcube, grid position (16,16). To avoid ending the game instantly, the diver must begin the game at the bottom of the this subcube.

Each minimap cell (8 cubes) should take about 5 seconds to traverse horizontally. It should take approximately 6 seconds to descend one depth level (CUBE_SUB_Z=300). The player always starts at the centre of the grid in the top layer (grid position 16,16). Once the player speed per cube has been calculated, it should be extracted as a cube_distance constant. This can also be used for the Sonar Ping distance mapping, as well as the Sea Line and Sea Floor scrolling.

### The Sea Line
When the player is in the top cube, the Sea Line is represented by a sinewave line of pixels. The sinewave is animated to create an undulating effect.

The sea line should never descend below the player's position (e.g. the centre of the screen space).

The sea line will scroll off the screen as the player descends.

* Starfield bubbles should be culled above the sea line (which can be treated as a single horizontal coordinate value for the purpose of culling).
* Rays should also be culled above the sea line. 
* Sharks, Great Old Ones and Archaeological Treasures do not appear at this level.
* Flotsam will appear centred on the seal line and should not be culled

## The Sea Floor
The sea floor is a flat horizontal line of pixels that is rendered  when the player is in the bottom cube. 

The sea floor should never ascend above the player's position (e.g. the centre of the screen space).

* Artifacts should appear a pixel above the sea floor and should not be culled
* Rays, Sharks and Flotsam Treasures do not appear at this level.
* The Great Old One graphic should only be drawn to the scanline where the sea floor appears.

## Minimap
The 2d minimap is an overhead (top-down) view displayed in the bottom right hand corner of the screen, using XOR writes. The X axis (left/right) maps horizontally and the Z axis (forward/backward) maps vertically; the depth axis (Y) is used only for filtering.

The minimap will be 32x32 pixels in size so that the 32x32 horizontal grid is fully visible, with each grid position mapping 1:1 to a minimap pixel. It will be a white grid divided into 4x4 squares of 8px width and height.

Each grid square represents an 8x8 cube area in the world space (consistent with the 32x32 grid / 4 cells = 8 cubes per cell). Treasure (both flotsam and archaeological) is indicated by red pixels at their grid position. Predators (rays, sharks, and GOOs) are indicated by green pixels. All items are shown at all depths (no depth filtering on the minimap).

A yellow 2x2 pixel represents the player's exact grid position.

If the player occupies the same minimap grid cell as a predator, the grid attribute is set to red flash. GOOs always display with the FLASH attribute.

The minimap will update each time the player crosses a cell boundary. Treasure is static but Sharks and Rays will move around the minimap.

The minimap overlaps the play area, so it should be drawn last to avoid flicker. Dots should be added and removed with XOR writes.

A vertical depth bar (4 pixels wide, 32 pixels tall) is displayed in the character column immediately to the left of the minimap (col 27). It shows a filled bar that is full height at the surface and shrinks to 1 pixel at the sea floor. The bar uses white ink on the current depth paper colour and updates every frame for smooth feedback.

The grid image for the minimap (with colour attributes) is stored at assets/minimap_grid.zxp

## Sprites
* Sprites should be stored in the asset directory as a ZX-Paintbrush .zxp bitmap and converted to a C header file with static const unsigned char at build time. 
* Sprites should be masked with a 1-pixel border to prevent pixel bleeding. The mask can be generated at game initialisation by using a 1px border expansion.
* Sprites should be animated with XOR writes.
* Predator Sprites should have two frames of animation and be mirrored horizontally. Mirroring should be done at game initialisation.
* Great Old One Sprites should have three frames of animation. This large sprite required no mask and does not use XOR writes.
* Archaeological Treasure Sprites should have two frames, the second of which has every second scanline offset by 1 pixel to create an underwater effect.
* Flotsam Treasure Sprites should have two frames, the second of which has every second scanline offset by 1 pixel.
* Sprite attributes will be determined by the depth level, except for the player sprite, which will always have yellow ink, and yellow ink bright in the top left character.
* There is no sprite cap.
* Each predator and treasure sprite has three scale variants generated at build time from the 32x32 source: full (32x32), medium (16x16), and small (8x8). A fourth common 4x4 block is used for all objects at maximum visible range. See "Sprite Scaling Pipeline" in Screen Space for details.

## Graphics

* The Boat and Title Logo graphics should be stored as ZX-Paintbrush ZXP files and converted to C header files with static const unsigned char at build time. The attribute values should also be stored. This requires the ZX0 compressor binary at build time
* The standard pipeline is: .zxp → raw bytes → ZX0 compress → C array
* The same Boat graphic will be used on all screens, positioned in the correct part of the screen.

### Great Old One (GOO) Rendering Pipeline

The GOO is a large angler fish face (approximately 20×20 characters) rendered as a death animation when the player enters its grid cell at depth 2. Because of its size, it uses a dedicated asset pipeline that stores only the left half of each frame and reconstructs the right half at runtime via bit-reversal mirroring.

#### Build-time pipeline

1. **Source:** `assets/angler_5.scr` — a full 256×192 ZX Spectrum screen file containing the GOO artwork, vertically symmetric about a centre column.
2. **Dithered reveal frames:** `tools/scr_dither_reveal.py` generates 6 progressively-dithered frames from the source, simulating the GOO emerging from darkness.
3. **Crop and compress:** `tools/scr_crop_zx0.py --mirror` processes each frame:
   - Auto-detects the bounding box of non-zero pixel data
   - Stores only the **left half** of the bounding box (columns left of the mirror axis)
   - ZX0-compresses each cropped half-frame
4. **Generated header:** 4 of the 6 frames are selected and output as `include/goo_data.h`, containing:
   - 4 ZX0-compressed byte arrays (one per animation frame)
   - Crop constants: `GOO_CROP_COL`, `GOO_CROP_ROW`, `GOO_CROP_W`, `GOO_CROP_H`, `GOO_CROP_SIZE`
   - Mirror column: `GOO_MIRROR_COL` (character column of the symmetry axis)
   - Draw position: `GOO_DRAW_COL`, `GOO_DRAW_ROW`, `GOO_DRAW_END_Y`

#### Runtime rendering (in `src/state.c`)

1. **Decompress:** `dzx0_decompress()` expands the current frame's ZX0 data into a scratch buffer at `0x6000` (below the code origin at `0x8000`).
2. **Bit-reversal table:** `init_bit_rev()` builds a 256-byte lookup table at `0x6900` that maps each byte to its bit-reversed equivalent (e.g. `0b10110000` → `0b00001101`). Built once at the start of the death animation.
3. **XOR blit:** `write_crop_to_screen()` is a naked assembly routine that:
   - Writes the decompressed left half directly to screen RAM using `scr_off()` for address calculation
   - For each row, also writes the **bit-reversed** bytes to the corresponding right-half columns, producing a mirrored copy
   - Uses XOR writes so the same routine can erase the previous frame before drawing the next

This half-storage approach halves the compressed data size for the GOO frames while producing the full symmetric face at runtime. The bit-reversal XOR blit is fast enough to animate the 4-frame death sequence without visible delay.

### The 3 Layers of Depth

Although the sea is wide, it is only 3 levels deep. However the cubes in the grid are much deeper than they are wide. It should take approximately 10 seconds to descend each level.

A single master paper colour (read via `depth_get_paper()`) is used for the border, starfield attributes, diver sprite, predator sprites, HUD gauges, and depth bar. The beeper sound routine preserves the current border colour when toggling the speaker bit on port 254.

When the player descends or ascends a depth level, the paper/ink colors change rapidly (~0.15 seconds) using the following cycle;

1. Depth 1 -> Depth 2: 
Ink changes from white bright -> white -> yellow bright -> yellow -> cyan bright -> cyan -> green bright
Paper changes from cyan -> green bright -> green  -> magenta light -> magenta -> red  bright -> blue bright

2. Depth 2 -> Depth 3:
Ink changes from green bright -> white  
Paper changes from  blue bright -> blue -> black

3. Depth 3 -> Depth 2:
Ink changes from white bright -> white -> green bright
Paper changes from black -> blue -> blue bright

4. Depth 2 -> Depth 1:
Ink changes from green bright -> cyan -> cyan bright -> yellow -> yellow bright -> white -> white bright
Paper changes from blue bright -> red -> red bright -> magenta -> magenta bright -> cyan 

### Depth 1

At the top, the Sea Line is rendered by a single sinewave line of pixels. The sinewave is animated to create an undulating effect. 

Bubbles above this line are culled. This line quickly scrolls off the screen as the player descends.

Paper colour is cyan (non-bright, matching the border), Ink colour is white. This level has 100 starfield bubbles. Only Rays Exist at this level.

If the current cube contains flotsam, the treasure is rendered at a random position on the surface line. It should be updated as the pixel line undulates.

### Depth 2

Paper colour is blue, Ink colour is green. This level has 50 starfield bubbles. Only Sharks exist at this level.

This level contains no treasure.

### Depth 3

Paper colour is black, Ink colour is white with no brightness. This level has 15 starfield bubbles. Only Great Old Ones exist at this level.

This level contains archaeological treasure. The bottom of the lowest cube should be rendered as a single dark blue line of pixels (the sea floor). 

If treasure is present, it should be rendered at a random position on this line.

## Log Entries

1. SS Marigold : Unexpected Storms, the sailors dream of an ancient city.
2. HMS Endeavour : Divers have returned with unusual statues.
3. USS Challenger : Do not stay here! The sea is full of danger.
4. HMS Beagle : A strange seawood-covered island has appeared.
5. MV Dawn Trader : Abandon hope, all ye who enter here!
6. EV Intrepid : Flotsam marked on our maps
7. SMS Gefahrlich : Hilfe! Mein Gott!

## Lore Entries

1. These statues are humanoid, but not human.
2. This altar is unpleasantly stained.
3. This ruined city is the wrong scale for people.
4. These tablets foretell the city's doom.
5. We should never have come here, we are lost.
6. This architecture is neither Greek nor Roman.
7. There is an eerie glow in the depths

## Sea Shanties

ABC notation is used below; see https://abcnotation.com for the standard.

Each tune has a single-melody beeper version (Channel A only, for Shiru's Beeper 48K) and a 3-channel arrangement (for Shiru's Tritone on 48k or AY on 128k). The AY arrangements should be authored in Vortex Tracker II and exported as .pt3 files. The ABC below serves as the reference for each channel.

### Title Screen: Óró Sé do Bheatha ‘Bhaile
Irish rebel song, rousing and march-like. Loops cleanly on the title screen.

Source: https://thesession.org/tunes/7480
Chords (per bar): |: Gm | Gm | F | F | Gm | F | Dm | F | Gm :|

Channel A — Lead:
```abc
X: 1
T: Óró Sé do Bheatha ‘Bhaile - Lead
M: 2/4
L: 1/8
K: Gdor
|:A4 A2 GA|B2 A2 G2 E2|G4 G2 G>A|G2 D2 E2 G2|
A3 A A2 G>A|B2 A2 B2 d2-|d6 d2|e2 B2 d2 B2|A2- A>B A4:|
```

Channel B — Harmony (3rds below lead, adjusted to chord tones):
```abc
X: 1
T: Óró Sé do Bheatha ‘Bhaile - Harmony
M: 2/4
L: 1/8
K: Gdor
|:F4 F2 EF|G2 F2 E2 C2|E4 E2 E>F|C2 A,2 C2 E2|
F3 F F2 E>F|G2 F2 G2 B2-|F6 F2|c2 G2 B2 G2|F2- F>G F4:|
```

Channel C — Bass (root-5th alternation):
```abc
X: 1
T: Óró Sé do Bheatha ‘Bhaile - Bass
M: 2/4
L: 1/8
K: Gdor
|:G,2D,2 G,2D,2|G,2D,2 G,2D,2|F,2C,2 F,2C,2|F,2C,2 F,2C,2|
G,2D,2 G,2D,2|F,2C,2 F,2C,2|D,2A,2 D,2A,2|F,2C,2 F,2C,2|G,4 z4:|
```

### Level Completion: Lowlands Away
Haunting sea shanty about a drowned sailor’s ghost. Plays on the Level Summary screen.

Source: https://abcnotation.com/tunePage?a=trillian.mit.edu%2F~jc%2Fmusic%2Fabc%2Fmirror%2Ffolkinfo.org%2FLowlands_Away_a_%2F0000
Chords (per bar): Cm | Cm | Gm | Bb | F | Cm | Cm (repeated)

Channel A — Lead:
```abc
X: 2
T: Lowlands Away - Lead
M: C|
L: 1/8
Q: 1/4=100
K: Cdor
c4 G4|cded c2B2|G6 F2|
B3c d2B2|c2B2 F2BA|G4 F2E2|C6||
C2|
EFGE FEC2|c4 G4|cded c2B2|G6 F2|
B3c d2B2|c2B2 F2BA|G4 F2E2|C6||
```

Channel B — Harmony (sustained chord tones, sparse and haunting):
```abc
X: 2
T: Lowlands Away - Harmony
M: C|
L: 1/8
Q: 1/4=100
K: Cdor
E4 E4|E2G2 E2G2|D6 D2|
F3G B2F2|A2G2 D2DC|E4 D2C2|E6||
C2|
C2E2 C2E2|E4 E4|E2G2 E2G2|D6 D2|
F3G B2F2|A2G2 D2DC|E4 D2C2|E6||
```

Channel C — Bass (root-5th, half notes for slow haunting feel):
```abc
X: 2
T: Lowlands Away - Bass
M: C|
L: 1/8
Q: 1/4=100
K: Cdor
C,4 G,4|C,4 G,4|G,4 D,4|
B,4 F,4|F,4 C,4|C,4 G,4|C,8||
z2|
C,4 C,4|C,4 G,4|C,4 G,4|G,4 D,4|
B,4 F,4|F,4 C,4|C,4 G,4|C,8||
```

### Game Over: Spanish Ladies (Farewell To Spain) — Slowed
Waltz in E minor, played at half tempo (~60 BPM) as a funeral dirge. Play the verse once then silence.

Source: https://thesession.org/tunes/6519
Chords (per bar): Em | Em | C | D | Em | Am | D | D | G | Am | Em | Em | C | D | Bm | Em

Channel A — Lead:
```abc
X: 3
T: Farewell To Spain - Lead
M: 3/4
L: 1/8
Q: 1/4=60
K: Emin
B,2|E2 E2 F2|E4 EF|G2 F2 E2|ED B,2 B,2|
E2 E2 F2|E4 F2|G2 A2 G2|F4 F2|
G2 F2 G2|A2 G2 AA|BA G2 E2|ED B,2 BA|
G2 E2 E2|ED B,2 A,2|B,2 G2 F2|E4||
```

Channel B — Harmony (3rds below lead, adjusted to chord tones):
```abc
X: 3
T: Farewell To Spain - Harmony
M: 3/4
L: 1/8
Q: 1/4=60
K: Emin
G,2|C2 C2 D2|C4 CD|E2 D2 C2|CB, G,2 G,2|
C2 C2 D2|C4 D2|E2 F2 E2|D4 D2|
E2 D2 E2|F2 E2 FF|GF E2 C2|CB, G,2 GF|
E2 C2 C2|CB, G,2 F,2|G,2 D2 D2|C4||
```

Channel C — Bass (waltz pattern: root-5th-5th):
```abc
X: 3
T: Farewell To Spain - Bass
M: 3/4
L: 1/8
Q: 1/4=60
K: Emin
z2|E,2 B,2 B,2|E,2 B,2 B,2|C,2 G,2 G,2|D,2 A,2 A,2|
E,2 B,2 B,2|A,2 E,2 E,2|D,2 A,2 A,2|D,2 A,2 A,2|
G,2 D,2 D,2|A,2 E,2 E,2|E,2 B,2 B,2|E,2 B,2 B,2|
C,2 G,2 G,2|D,2 A,2 A,2|B,2 F,2 F,2|E,4||
```

### Trumpet Fanfare (Level Complete Trigger)
A short 4-note ascending motif (C-E-G-C’) played before Lowlands Away begins on the Level Summary screen.