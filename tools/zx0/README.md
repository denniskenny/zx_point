# ZX0 v2 compressor (vendored)

Source from https://github.com/einar-saukas/ZX0 (commit `ecde3a2`, 2021-12-17),
BSD 3-Clause License, Copyright (c) 2021 Einar Saukas. See `LICENSE`.

Vendored so the build never depends on a copy in `/tmp` (macOS clears it).
`make` compiles `tools/zx0/zx0` on first use.

Must stay **v2 format** to match the decompressor in `src/dzx0.c`.
Do not substitute `z88dk-zx0`: it is ZX0 v1.5 (classic format) and its output
decompresses to garbage, corrupting RAM at boot.
