# --- Toolchain ---
Z88DK ?= $(HOME)/z88dk
ZCC ?= $(Z88DK)/bin/zcc
ZCCCFG ?= $(Z88DK)/lib/config
HOSTCC ?= cc
PYTHON ?= python3
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
FUSE ?= open -a Fuse
FUSE_RUN = defaults write FusePreferences machine -string "48" 2>/dev/null; $(FUSE) downship.tap
else
FUSE ?= fuse-sdl
FUSE_RUN = $(FUSE) --machine 48 downship.tap &
endif

# --- Config ---
CONFIG_MK ?= config/basic_config.mk
include $(CONFIG_MK)

CFLAGS=+zx -vn -SO3 -zorg=32768 -startup=31 --opt-code-speed -compiler=sdcc -mz80 \
       --reserve-regs-iy --allow-unsafe-read -Cc--max-allocs-per-node=50000
USER_CFLAGS ?=
LDFLAGS=-lm -create-app

# --- Asset pipeline ---
ZXP2HEADER = $(PYTHON) tools/zxp2header.py
ZX0 ?= /tmp/ZX0/src/zx0
SCR_CROP_ZX0 = $(PYTHON) tools/scr_crop_zx0.py

# Row-major sprites (direct write — diver stays row-major for legacy compat)
include/diver.h: assets/diver3.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 4 --horizontal --name diver

# Row-major sprites (32x32, 2 frames each, with 16x16 and 8x8 downscaled)
include/ray.h: assets/ray.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name ray --downscale

include/shark.h: assets/shark.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name shark --downscale

include/statue.h: assets/statue.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name statue --downscale

include/tablet.h: assets/tablet.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name tablet --downscale

include/altar.h: assets/altar.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name altar --downscale

include/firstaid.h: assets/firstaid.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name firstaid --downscale

include/oxygen_tank.h: assets/oxygen_tank.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name oxygen_tank --downscale

include/map_item.h: assets/map.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name map_item --downscale

include/log_item.h: assets/log.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --horizontal --name log_item --downscale

# Ship (48x32, single frame, for level intro animation)
include/boat.h: assets/ship3.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 1 --name boat

# Minimap grid (32x32, blit source for the minimap background)
include/minimap_grid.h: assets/minimap_grid.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name minimap_grid

# GOO anglerfish: dither source → 4 reveal frames → crop + ZX0 compress
GOO_SRC = assets/angler_5.scr
include/goo_data.h: $(GOO_SRC) tools/scr_dither_reveal.py tools/scr_crop_zx0.py
	$(PYTHON) tools/scr_dither_reveal.py $(GOO_SRC) assets/goo
	$(SCR_CROP_ZX0) --mirror $@ $(ZX0) \
		goo_frame1:assets/goo_1.scr goo_frame2:assets/goo_3.scr \
		goo_frame3:assets/goo_5.scr goo_frame4:assets/goo_6.scr

# Vignette (full 6912-byte .scr → ZX0 compressed C header)
include/vignette.h: assets/vignette.scr tools/zx0_to_header.py
	rm -f /tmp/vignette.zx0
	$(ZX0) $< /tmp/vignette.zx0
	$(PYTHON) tools/zx0_to_header.py $@ vignette_zx0:/tmp/vignette.zx0

GENERATED_HEADERS = include/diver.h \
    include/ray.h include/shark.h \
    include/statue.h include/tablet.h include/altar.h \
    include/firstaid.h include/oxygen_tank.h include/map_item.h include/log_item.h \
    include/boat.h \
    include/minimap_grid.h \
    include/goo_data.h \
    include/vignette.h

# --- Source files (multi-file build) ---
SRCS = src/main.c src/state.c src/bubblefield.c src/gfx.c src/input.c src/sound.c \
       src/music.c src/hw_detect.c src/depth.c src/sealine.c src/vsync.c src/sprites.c \
       src/player.c src/treasure.c src/hud.c src/minimap.c src/predators.c \
       src/entity_render.c src/dzx0.c

HEADERS = config/game_config.h include/state.h include/game.h include/hw.h \
          include/gfx.h include/input.h include/sound.h include/music.h include/depth.h \
          include/sealine.h include/vsync.h include/sprites.h \
          include/player.h include/treasure.h include/hud.h include/minimap.h \
          include/predators.h include/entity_render.h include/dzx0.h \
          include/goo_data.h $(GENERATED_HEADERS)

# --- Tritone music -----------------------------------------------------------
# Pipeline: transcription (.txt) --txt2tritone.py--> Tritone (.asm)
#           --gen_tritone_module.py--> per-tune data module (_NAME_play).
# The ~300-line Tritone ENGINE is factored into ONE shared module
# (tritone_engine.asm, PUBLIC TRI_PLAY); each tune module holds only its song
# data and CALLs it, so the engine is in the binary once regardless of tune
# count.  To add a tune NAME: author assets/music/NAME.txt, append
# assets/music/NAME_linkable.asm to MUSIC_LINKABLE, and call NAME_play().
# (See .claude/skills/tritone-music.)
MUSIC_ENGINE = assets/music/tritone_engine.asm
MUSIC_LINKABLE = $(MUSIC_ENGINE) \
                 assets/music/oro_linkable.asm \
                 assets/music/lowlands_linkable.asm \
                 assets/music/spanish_linkable.asm

# shared engine (extracted once from the Óró export template)
$(MUSIC_ENGINE): assets/music/oro\ se\ do\ bheatha.asm tools/gen_tritone_module.py
	$(PYTHON) tools/gen_tritone_module.py "assets/music/oro se do bheatha.asm" -o $@ --engine

# transcription -> Tritone assembly
assets/music/%.asm: assets/music/%.txt tools/txt2tritone.py
	$(PYTHON) tools/txt2tritone.py $< -o $@

# Tritone assembly -> per-tune data module (symbol prefix = filename stem)
assets/music/%_linkable.asm: assets/music/%.asm tools/gen_tritone_module.py
	$(PYTHON) tools/gen_tritone_module.py $< -o $@ --name $*

# Óró (title) is a hand-authored Beepola export (spaces in the name, no .txt),
# so it needs an explicit rule with an explicit symbol name.
assets/music/oro_linkable.asm: assets/music/oro\ se\ do\ bheatha.asm tools/gen_tritone_module.py
	$(PYTHON) tools/gen_tritone_module.py "assets/music/oro se do bheatha.asm" -o $@ --name oro

# keep generated .asm intermediates from being auto-deleted
.SECONDARY:

# --- Top-level targets ---
all: downship.tap

.PHONY: all run clean assets test-legacy test

assets: $(GENERATED_HEADERS)

run: downship.tap
	$(FUSE_RUN)

# --- Compile, link & package (multi-file) ---
downship.tap: $(SRCS) $(HEADERS) $(MUSIC_LINKABLE)
	PATH=$(Z88DK)/bin:$$PATH Z88DK=$(Z88DK) ZCCCFG=$(ZCCCFG) $(ZCC) $(CFLAGS) $(USER_CFLAGS) -o downship $(SRCS) $(MUSIC_LINKABLE) $(LDFLAGS)

# --- Automated tests (run on ZEsarUX headless) ---
TEST_CFLAGS=+zx -vn -SO3 -zorg=32768 -startup=31 --opt-code-speed -compiler=sdcc -mz80 \
            --reserve-regs-iy --allow-unsafe-read

tests/test_orientation.tap: tests/test_orientation.c config/game_config.h
	PATH=$(Z88DK)/bin:$$PATH Z88DK=$(Z88DK) ZCCCFG=$(ZCCCFG) $(ZCC) $(TEST_CFLAGS) -o tests/test_orientation tests/test_orientation.c -create-app

test: tests/test_orientation.tap
	$(PYTHON) tests/run_tests.py tests/test_orientation.tap

# --- Legacy single-file build (regression reference) ---
test-legacy: starfield.c include/diver.h
	PATH=$(Z88DK)/bin:$$PATH Z88DK=$(Z88DK) ZCCCFG=$(ZCCCFG) $(ZCC) $(CFLAGS) $(USER_CFLAGS) -o downship starfield.c -lm -create-app

# --- Clean ---
clean:
	rm -f downship downship.tap downship_CODE.bin downship_data_user.bin downship_code.tap *.o *.map
	rm -f $(GENERATED_HEADERS)
	rm -f $(MUSIC_LINKABLE) assets/music/*.o assets/music/lowlands.asm $(MUSIC_ENGINE)
	rm -f tests/test_orientation tests/test_orientation.tap tests/test_orientation_CODE.bin tests/test_orientation_data_user.bin tests/test_orientation_code.tap
