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

# Sprite assets: .zxp -> .h (add new sprites here)
include/diver.h: assets/diver.zxp tools/zxp2header.py
	$(ZXP2HEADER) $< $@ --frames 2 --name diver

GENERATED_HEADERS = include/diver.h

# --- Top-level targets ---
all: starfield.tap

.PHONY: all run clean assets

assets: $(GENERATED_HEADERS)

run: starfield.tap
	$(FUSE_RUN)

# --- Compile, link & package ---
starfield.tap: starfield.c $(GENERATED_HEADERS)
	PATH=$(Z88DK)/bin:$$PATH Z88DK=$(Z88DK) ZCCCFG=$(ZCCCFG) $(ZCC) $(CFLAGS) $(USER_CFLAGS) -o starfield starfield.c $(LDFLAGS)

# --- Clean ---
clean:
	rm -f starfield starfield.tap starfield_CODE.bin starfield_data_user.bin starfield_code.tap *.o *.map
	rm -f $(GENERATED_HEADERS)
