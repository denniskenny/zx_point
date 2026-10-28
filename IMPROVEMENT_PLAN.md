# Improvement Plan — Code Review vs design.md

Findings from a full review of the implementation against `design.md` (July 2026).
Verified against a fresh build: binary spans **0x8000–0xF99E** (31,134 B), leaving
~1.6 KB stack headroom. Frame budget 69,888 T-states at 50Hz; draw phase should fit
the ~14,336 T top-border window after vsync.

Complements `PLAN.md` (feature phases). This plan covers: spec-compliance fixes,
performance optimisations, memory savings, and doc drift.

---

## A. Spec-compliance fixes (bugs vs design — do these first)

- [ ] **A1. Remove TEMP spawn hacks** — `src/predators.c:92-99`: the `level_num < 2`
  guard is commented out and `if (n_goos == 0) n_goos = 1;` forces a GOO on every
  level. Violates the level table (design lines 46-51) on *every* level.
- [ ] **A2. RAY_CONSTANT** — `config/game_config.h:157` is `1`; design (line 53) says
  `10` (rays = level × constant). Decide the intended value and align code + doc.
- [ ] **A3. Shark count off by one** — `n_sharks = level_num - 2` gives 3 sharks at
  level 5; design says 2. Use `level_num - 3` from level 4, or update the doc.
- [ ] **A4. Minimap depth filtering** — design (lines 78, 100, 406) says all treasures
  and predators show at **all** depths; `src/minimap.c:140,152` filter to the player's
  depth. This also hides the GOO's flashing dot except at depth 2.
- [ ] **A5. Minimap stale predator dots** — redraw only triggers on player cell/depth
  change (`src/minimap.c:119-124`); design (line 412) says sharks/rays move around the
  map. Also redraw when any predator crosses a cell (cheap: set a dirty flag in
  `predators_update` when a grid coord changes).
- [ ] **A6. HUD gauges ignore `depth_get_paper()`** — design (line 16) says both bars
  use the depth paper colour; `src/hud.c:18-21` hardcodes bright-black paper. Same for
  the depth bar (`src/minimap.c:190,211-212` — also yellow ink vs spec'd white, 8px
  wide vs spec'd 4px).
- [ ] **A7. Sonar silent at distance 0** — `src/sound.c:161` treats `dist == 0` as no
  target, so the ping stops exactly when standing on the target — worst for invisible
  GOOs, whose sonar is the only warning before instant death. Clamp dist to 1 instead.
- [ ] **A8. Proximity counter wasted during invulnerability** —
  `src/predators.c:295-297` resets `proximity_ctr` even when `state.c:789` discards
  the damage due to `invuln_timer`, silently extending the damage interval. Either
  skip accrual while invulnerable or don't reset on discarded hits.
- [ ] **A9. Map/Log flotsam have no effect** — `src/treasure.c:139-148` handles only
  FIRSTAID and OXYGEN. Design (line 65): maps add one flotsam treasure + minimap dot;
  logs feed the summary-screen lore display (see B3).
- [ ] **A10. Fragile predator depth = type identity** — depth checks use
  `predators[i].type != player.gy` (`state.c:835`, `predators.c:170,282`,
  `minimap.c:152`), relying on RAY/SHARK/GOO == 0/1/2. Introduce a real `gy` field or
  at least route everything through `pred_depth(t)` so A4 can be fixed safely.
- [ ] **A11. GOO not clipped to sea floor** — design (line 386) says the GOO draws
  only down to the sea-floor scanline; `write_crop_to_screen` (`state.c:1015-1098`)
  writes the full crop height.
- [ ] **A12. Design self-contradictions to resolve** (pick one, fix the doc):
  start position bottom-vs-centre of first subcube (lines 33 vs 271); rays/sharks
  "no sonar" (88, 94) vs medium-pitch sonar (201) — code follows the latter;
  GOO 3 frames (102) vs 4-frame reveal pipeline (438+) — code follows the latter.

## B. Missing features (design gaps, roughly by value)

- [ ] **B1. Title/summary screens: sea line + bobbing boat** — both screens are
  text-only (`state.c:130-148`, `901-956`); the boat/sea-line code already exists in
  `STATE_LEVEL_INTRO` and can be reused.
- [ ] **B2. Summary screen: treasure list + game-over totals** — only counts are
  printed; `level.total_collected` is tracked but never displayed.
- [ ] **B3. Log/lore entries** — the 7 log + 7 lore strings from design exist nowhere
  in the codebase. Add as const string tables (≈450 B — see memory items first) and
  show a random entry on the summary when a LOG/TABLET was collected.
- [ ] **B4. Non-blocking music** — interrupts are off (`-startup=31`), so all shanties
  block; design says melodies play under interrupts so input/animation continue.
  Options: an IM2 path for melodies, or frame-paced tick playback like `music.c`.
- [ ] **B5. Sonar priority** — design: sonar always overrides other SFX. Blocking
  jingles currently pre-empt it (also a frame-drop issue — see C7).
- [ ] **B6. Predator direction frames** — `pred_pick_frame` (`predators.c:197-216`)
  ignores `gdx`; mirrored frames are spec'd (design 90/96/424) and mirroring-at-init
  infra already exists (`sprites_mirror_h`).
- [ ] **B7. 128K layer** — AY shanties (.pt3 + player), AY sonar ping, beeper muting,
  banked memory map (PLAN.md Phase 9). `is_128k` detection exists; nothing uses it
  except vsync.
- [ ] **B8. Smaller items**: flotsam tracking the undulating sea line at depth 0
  (design 339); predator respawn on leaving grid (232) — currently they bounce and
  never leave, decide which is intended; intro crane + pixel-line descent (134-138);
  title text wording/order (111); GOO random walk vs current diagonal bounce (100);
  the unused `CUBE_DISTANCE` constant (273) — wire it in or delete it.

## C. Performance optimisations (ranked; est. combined ceiling 25-40k T/frame)

- [ ] **C1. Kill per-row/per-byte `scr_off()` calls** *(biggest win: 10-25k T)* —
  `scr_off` (`gfx.c:10-16`) costs ~150-250 T per call and is called up to 64×/frame in
  `sealine_update` (`sealine.c:53,78,84`), 16× per `xor_sprite_16` (`gfx.c:194`, same
  in `xor_sprite_8`/`xor_block_4`), per row in `write_blit`/`clear_blit`
  (`gfx.c:261,290`), and per **byte** in `write_blit_px` (`gfx.c:324`).
  Fix: a 384-byte `uint16_t scr_row_addr[192]` table built at init in a low-RAM hole
  (see D2 pattern), and/or `inc h` address stepping as `xor_sprite_32_fast` already
  does (`gfx.c:117-130`).
- [ ] **C2. Asm `xor_sprite_16`** *(3-10k T when 16-scale entities visible)* — clone
  `xor_sprite_32_fast` (`gfx.c:43-180`) down to 2 bytes/row × 16 rows. This is the
  common predator/treasure LOD.
- [ ] **C3. Move draw-phase math into the logic phase** *(2-4k T off the post-vsync
  critical path)* — `state.c:576-637` does two/three `__divuint` divisions by
  CUBE_SUB_Z=300 plus 16-bit muls before any pixel is written; `sfy` is computed twice
  (`state.c:598-600` vs `626-628`). Compute once per frame in logic (positions are
  last-frame anyway), ideally incrementally (sub_z changes ≤2/frame).
- [ ] **C4. Stagger the O(64) scans** *(5-10k T on populated levels)* — three full
  MAX_PREDATORS passes per frame: `predators_update` (`predators.c:122-184`), sonar
  scan (`state.c:814-847`), collision (`predators.c:264-305`). Fold into one pass
  and/or update ¼ of predators per frame; run the sonar scan every 4-8 frames (its
  minimum interval is 12 frames anyway). Shrinking MAX_PREDATORS (D3) helps too.
- [ ] **C5. Early-out caching for HUD/depth-bar/player-colour** *(2-3k T, trivial)* —
  `hud_draw` (`hud.c:55-74`) does a 16-bit mul + div-by-100 and rewrites 18 attr bytes
  every frame though oxygen changes every 90 frames; `depth_indicator_draw`
  (`minimap.c:183-213`) and `sprites_player_set_colour` (`sprites.c:75-92`) likewise.
  Cache previous values and skip when unchanged.
- [ ] **C6. Replace runtime `%` with masks** *(0.5-1k T, trivial)* —
  `% diver_frame_count` at `state.c:643,454` hits a library modulo because
  `diver_frame_count` is a const *variable*; use `& (DIVER_FRAMES - 1)`.
- [ ] **C7. Non-blocking collect/damage jingles** — `sfx_collect_jingle` ≈2 frames,
  `sfx_damage_jingle` ≈5-6 frames of blocking beeper (`sound.c:127-147`), visible
  hitch on every pickup/hit. Convert to a per-frame state machine like the existing
  sonar ping (`sound.c:149-157`). Also solves B5.
- [ ] **C8. `set_attr_rect` pointer walk** *(0.5-2k T, trivial)* — `gfx.c:24-37`
  recomputes `(row+r)*32+col+c` per cell; walk a pointer with `+= 32` per row.
- [ ] **C9. Adjacent erase+draw per entity + beam-ordered drawing** — erase happens
  right after vsync (`state.c:573-574`) but redraw only at `state.c:640-642`, leaving
  a flicker window; pair erase/draw per entity (as the intro diver does). Draw sealine
  before bubbles (currently after, `state.c:601-609`) so top-of-screen writes beat the
  beam.
- [ ] **C10. Micro-wins** — `__z88dk_fastcall` on single-arg hot functions
  (`sprites_player_draw`, `sealine_update`, `seafloor_update`, `sfx_play_tone`…);
  globals-as-params for `write_blit`/`clear_blit` (pattern already used by
  `xor32_*`); bubble-loop asm hoists (`bubblefield.c:228-233,470-475`);
  `dx_sub * 3 / 4` → `dx_sub - (dx_sub >> 2)` (`entity_render.c:130`);
  duplicate `depth_is_transitioning()` calls (`state.c:577,639,763`).
- [ ] **C11. Profile before/after** — border-colour timing (per `debugging.md`) around
  C1-C3 to confirm the draw phase fits the top-border window.

## D. Memory savings (~2.5 KB low-risk; up to ~4.5 KB total)

Current layout: rodata 9,486 B (compressed assets 7,959 B), BSS 1,547 B, ~1.6 KB
stack headroom. Low-RAM holes free: 0x5B00-0x5FFF (1,280 B), 0x675C-0x68FF (420 B),
0x78E8-0x7FFF (1,816 B) ≈ 3.5 KB.

- [ ] **D1. Disable stdio console** *(~1,360 B, trivial — biggest single win)* —
  nothing calls printf, yet `fputc_cons_generic` (~491 B), `font_4x8` (768 B, game
  uses the ROM font via `gfx.c:330`) and `_sgoioblk` (100 B) are linked. Add
  `#pragma output CRT_ENABLE_STDIO = 0` in main.c. Also drop the no-op `-lm`.
- [ ] **D2. Build `recip_z` at init in a low-RAM hole** *(~480 B net)* — the 512 B
  const table (`bubblefield.c:125`) is `32768/z`; generate it at boot at e.g. 0x7A00,
  exactly like `init_bit_rev` already does. Same trick for the C1 row-address table.
- [ ] **D3. MAX_PREDATORS 64 → 16** *(528 B BSS, trivial)* — the spawn formula never
  approaches 64 in realistic play; this is also the cheapest stack-headroom insurance
  and speeds up the C4 scans.
- [ ] **D4. Prune dead sprite data from the packed blob** *(~500 B)* — only
  `diver_attr` and `minimap_grid_attr` are consumed; 9 other `*_attr` blocks (32 B
  each), `boat_attr` (24 B) and `minimap_grid_f2` (64 B) are packed but never read.
  Filter in `tools/pack_sprites.py`.
- [ ] **D5. Move ZX0 blobs into low-RAM holes** *(up to ~2 KB more, moderate)* — the
  compressed assets are only read at non-critical moments (boot/title/death), so
  contended RAM is fine. Easiest: `sprites_zx0` (977 B) is dead after boot-time
  unpack — it can even overlay the GOO scratch region. Needs a second ORG'd CODE
  block on the tape.
- [ ] **D6. Small table dedup/derivation** *(~150 B)* — `lintro_sine` (`state.c:252`)
  duplicates `sine_tab` (`sealine.c:21`); `depth_scr[32]`, `player_scr[16]` are
  init-derivable via `scr_off`; `death_borders[8]` is `death_attrs[i]>>3 & 7`.
- [ ] **D7. Optional: drop a GOO reveal frame** *(~1,062 B)* — use 3 of the 6 dither
  steps instead of 4 (Makefile:75-77). Visual tradeoff; decide after D1-D5.
- [ ] **D8. Optional: selective `--opt-code-speed`** *(several hundred B)* — compile
  only bubblefield/gfx/entity_render/vsync for speed, the rest for size. Requires
  per-file rules in the Makefile.
- [ ] **D9. Guardrails** — assert `arena_end < 0x8000` in `tools/pack_sprites.py`;
  a `#if` guard on `GOO_CROP_SIZE` vs the bit-rev table at 0x6900 (headroom is only
  420 B); comment at `state.c:985` that SCRATCH_BUF must never exceed 0x900 bytes.

## E. Doc drift — update design.md

- Boat intro animation and GOO death sequence exist; remove the "not yet implemented"
  caveats (lines 102, 128-129, 155).
- State machine has 7 states (adds STATE_LEVEL_INTRO, STATE_GOO_DEATH).
- Document: invulnerability flash, PRNG seeding from title music ticks, predator
  grid-move intervals (250/300/750), spawn margins, the entity-slot cap
  (`MAX_ENTITY_SLOTS` / `MAX_VISIBLE_PREDS 2`) vs the "no sprite cap" claim (line 429),
  bubble counts 45/35/29 vs doc's 100/50/30, sea floor drawn as filled region vs
  "single line", surfaced-early re-entry at bottom of depth-0 cube.

## Suggested order

1. **A1-A3** (spec-breaking spawn/count bugs — one small session)
2. **D1 + D3 + C5 + C6 + C8** (trivial wins: ~2 KB RAM + ~4k T/frame)
3. **C1** (row-address table; biggest frame-budget win) then **C11** profile
4. **A4-A8** (minimap/HUD/sonar compliance) + **C4** (single predator pass)
5. **C2, C3, C7/B5, C9** (asm sprite16, math hoisting, non-blocking jingles)
6. **D2, D4, D6, D9** (remaining memory items + guardrails)
7. **B1-B3** (screens/log/lore — needs the RAM freed above)
8. **B4, B6-B8, D5, D7-D8, E** (larger features + doc pass)
