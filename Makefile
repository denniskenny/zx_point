# --- Toolchain ---
Z88DK ?= $(HOME)/z88dk
ZCC ?= $(Z88DK)/bin/zcc
ZCCCFG ?= $(Z88DK)/lib/config
HOSTCC ?= cc
PYTHON ?= python3
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
FUSE ?= open -a Fuse
FUSE_RUN = $(FUSE) starfield.tap
else
FUSE ?= fuse-sdl
FUSE_RUN = $(FUSE) starfield.tap &
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

# Row-major sprites (direct write — diver stays row-major for legacy compat)
include/diver.h: assets/diver.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name diver

# Row-major sprites (32x32, 2 frames each)
include/ray.h: assets/ray.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name ray

include/shark.h: assets/shark.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name shark

include/statue.h: assets/statue.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name statue

include/tablet.h: assets/tablet.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name tablet

include/altar.h: assets/altar.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name altar

include/firstaid.h: assets/firstaid.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name firstaid

include/oxygen_tank.h: assets/oxygen_tank.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name oxygen_tank

include/map_item.h: assets/map.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name map_item

include/log_item.h: assets/log.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name log_item

# Minimap grid (32x32, blit source for the minimap background)
include/minimap_grid.h: assets/minimap_grid.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name minimap_grid

GENERATED_HEADERS = include/diver.h \
    include/ray.h include/shark.h \
    include/statue.h include/tablet.h include/altar.h \
    include/firstaid.h include/oxygen_tank.h include/map_item.h include/log_item.h \
    include/minimap_grid.h

# --- Source files (multi-file build) ---
SRCS = src/main.c src/state.c src/starfield.c src/gfx.c src/input.c src/sound.c \
       src/hw_detect.c src/depth.c src/sealine.c src/vsync.c src/sprites.c \
       src/player.c src/treasure.c src/hud.c src/minimap.c src/predators.c

HEADERS = config/game_config.h include/state.h include/game.h include/hw.h \
          include/gfx.h include/input.h include/sound.h include/depth.h \
          include/sealine.h include/vsync.h include/sprites.h \
          include/player.h include/treasure.h include/hud.h include/minimap.h \
          include/predators.h \
          $(GENERATED_HEADERS)

# --- Top-level targets ---
all: starfield.tap

.PHONY: all run clean assets test-legacy

assets: $(GENERATED_HEADERS)

run: starfield.tap
	$(FUSE_RUN)

# --- Compile, link & package (multi-file) ---
starfield.tap: $(SRCS) $(HEADERS)
	PATH=$(Z88DK)/bin:$$PATH Z88DK=$(Z88DK) ZCCCFG=$(ZCCCFG) $(ZCC) $(CFLAGS) $(USER_CFLAGS) -o starfield $(SRCS) $(LDFLAGS)

# --- Legacy single-file build (regression reference) ---
test-legacy: starfield.c include/diver.h
	PATH=$(Z88DK)/bin:$$PATH Z88DK=$(Z88DK) ZCCCFG=$(ZCCCFG) $(ZCC) $(CFLAGS) $(USER_CFLAGS) -o starfield starfield.c -lm -create-app

# --- Clean ---
clean:
	rm -f starfield starfield.tap starfield_CODE.bin starfield_data_user.bin starfield_code.tap *.o *.map
	rm -f $(GENERATED_HEADERS)
