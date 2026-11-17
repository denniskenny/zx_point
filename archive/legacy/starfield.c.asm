;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.0 #15242 (Mac OS X ppc)
;--------------------------------------------------------
; Processed by Z88DK
;--------------------------------------------------------

	EXTERN __divschar
	EXTERN __divschar_callee
	EXTERN __divsint
	EXTERN __divsint_callee
	EXTERN __divslong
	EXTERN __divslong_callee
	EXTERN __divslonglong
	EXTERN __divslonglong_callee
	EXTERN __divsuchar
	EXTERN __divsuchar_callee
	EXTERN __divuchar
	EXTERN __divuchar_callee
	EXTERN __divuint
	EXTERN __divuint_callee
	EXTERN __divulong
	EXTERN __divulong_callee
	EXTERN __divulonglong
	EXTERN __divulonglong_callee
	EXTERN __divuschar
	EXTERN __divuschar_callee
	EXTERN __modschar
	EXTERN __modschar_callee
	EXTERN __modsint
	EXTERN __modsint_callee
	EXTERN __modslong
	EXTERN __modslong_callee
	EXTERN __modslonglong
	EXTERN __modslonglong_callee
	EXTERN __modsuchar
	EXTERN __modsuchar_callee
	EXTERN __moduchar
	EXTERN __moduchar_callee
	EXTERN __moduint
	EXTERN __moduint_callee
	EXTERN __modulong
	EXTERN __modulong_callee
	EXTERN __modulonglong
	EXTERN __modulonglong_callee
	EXTERN __moduschar
	EXTERN __moduschar_callee
	EXTERN __mulint
	EXTERN __mulint_callee
	EXTERN __mullong
	EXTERN __mullong_callee
	EXTERN __mullonglong
	EXTERN __mullonglong_callee
	EXTERN __mulschar
	EXTERN __mulschar_callee
	EXTERN __mulsuchar
	EXTERN __mulsuchar_callee
	EXTERN __muluchar
	EXTERN __muluchar_callee
	EXTERN __muluschar
	EXTERN __muluschar_callee
	EXTERN __rlslonglong
	EXTERN __rlslonglong_callee
	EXTERN __rlulonglong
	EXTERN __rlulonglong_callee
	EXTERN __rrslonglong
	EXTERN __rrslonglong_callee
	EXTERN __rrulonglong
	EXTERN __rrulonglong_callee
	EXTERN ___mulsint2slong
	EXTERN ___mulsint2slong_callee
	EXTERN ___muluint2ulong
	EXTERN ___muluint2ulong_callee
	EXTERN ___sdcc_call_hl
	EXTERN ___sdcc_call_iy
	EXTERN ___sdcc_enter_ix
	EXTERN banked_call
	EXTERN _banked_ret
	EXTERN ___fs2schar
	EXTERN ___fs2schar_callee
	EXTERN ___fs2sint
	EXTERN ___fs2sint_callee
	EXTERN ___fs2slong
	EXTERN ___fs2slong_callee
	EXTERN ___fs2slonglong
	EXTERN ___fs2slonglong_callee
	EXTERN ___fs2uchar
	EXTERN ___fs2uchar_callee
	EXTERN ___fs2uint
	EXTERN ___fs2uint_callee
	EXTERN ___fs2ulong
	EXTERN ___fs2ulong_callee
	EXTERN ___fs2ulonglong
	EXTERN ___fs2ulonglong_callee
	EXTERN ___fsadd
	EXTERN ___fsadd_callee
	EXTERN ___fsdiv
	EXTERN ___fsdiv_callee
	EXTERN ___fseq
	EXTERN ___fseq_callee
	EXTERN ___fsgt
	EXTERN ___fsgt_callee
	EXTERN ___fslt
	EXTERN ___fslt_callee
	EXTERN ___fsmul
	EXTERN ___fsmul_callee
	EXTERN ___fsneq
	EXTERN ___fsneq_callee
	EXTERN ___fssub
	EXTERN ___fssub_callee
	EXTERN ___schar2fs
	EXTERN ___schar2fs_callee
	EXTERN ___sint2fs
	EXTERN ___sint2fs_callee
	EXTERN ___slong2fs
	EXTERN ___slong2fs_callee
	EXTERN ___slonglong2fs
	EXTERN ___slonglong2fs_callee
	EXTERN ___uchar2fs
	EXTERN ___uchar2fs_callee
	EXTERN ___uint2fs
	EXTERN ___uint2fs_callee
	EXTERN ___ulong2fs
	EXTERN ___ulong2fs_callee
	EXTERN ___ulonglong2fs
	EXTERN ___ulonglong2fs_callee
	EXTERN ____sdcc_2_copy_src_mhl_dst_deix
	EXTERN ____sdcc_2_copy_src_mhl_dst_bcix
	EXTERN ____sdcc_4_copy_src_mhl_dst_deix
	EXTERN ____sdcc_4_copy_src_mhl_dst_bcix
	EXTERN ____sdcc_4_copy_src_mhl_dst_mbc
	EXTERN ____sdcc_4_ldi_nosave_bc
	EXTERN ____sdcc_4_ldi_save_bc
	EXTERN ____sdcc_4_push_hlix
	EXTERN ____sdcc_4_push_mhl
	EXTERN ____sdcc_lib_setmem_hl
	EXTERN ____sdcc_ll_add_de_bc_hl
	EXTERN ____sdcc_ll_add_de_bc_hlix
	EXTERN ____sdcc_ll_add_de_hlix_bc
	EXTERN ____sdcc_ll_add_de_hlix_bcix
	EXTERN ____sdcc_ll_add_deix_bc_hl
	EXTERN ____sdcc_ll_add_deix_hlix
	EXTERN ____sdcc_ll_add_hlix_bc_deix
	EXTERN ____sdcc_ll_add_hlix_deix_bc
	EXTERN ____sdcc_ll_add_hlix_deix_bcix
	EXTERN ____sdcc_ll_asr_hlix_a
	EXTERN ____sdcc_ll_asr_mbc_a
	EXTERN ____sdcc_ll_copy_src_de_dst_hlix
	EXTERN ____sdcc_ll_copy_src_de_dst_hlsp
	EXTERN ____sdcc_ll_copy_src_deix_dst_hl
	EXTERN ____sdcc_ll_copy_src_deix_dst_hlix
	EXTERN ____sdcc_ll_copy_src_deixm_dst_hlsp
	EXTERN ____sdcc_ll_copy_src_desp_dst_hlsp
	EXTERN ____sdcc_ll_copy_src_hl_dst_de
	EXTERN ____sdcc_ll_copy_src_hlsp_dst_de
	EXTERN ____sdcc_ll_copy_src_hlsp_dst_deixm
	EXTERN ____sdcc_ll_lsl_hlix_a
	EXTERN ____sdcc_ll_lsl_mbc_a
	EXTERN ____sdcc_ll_lsr_hlix_a
	EXTERN ____sdcc_ll_lsr_mbc_a
	EXTERN ____sdcc_ll_push_hlix
	EXTERN ____sdcc_ll_push_mhl
	EXTERN ____sdcc_ll_sub_de_bc_hl
	EXTERN ____sdcc_ll_sub_de_bc_hlix
	EXTERN ____sdcc_ll_sub_de_hlix_bc
	EXTERN ____sdcc_ll_sub_de_hlix_bcix
	EXTERN ____sdcc_ll_sub_deix_bc_hl
	EXTERN ____sdcc_ll_sub_deix_hlix
	EXTERN ____sdcc_ll_sub_hlix_bc_deix
	EXTERN ____sdcc_ll_sub_hlix_deix_bc
	EXTERN ____sdcc_ll_sub_hlix_deix_bcix
	EXTERN ____sdcc_load_debc_deix
	EXTERN ____sdcc_load_dehl_deix
	EXTERN ____sdcc_load_debc_mhl
	EXTERN ____sdcc_load_hlde_mhl
	EXTERN ____sdcc_store_dehl_bcix
	EXTERN ____sdcc_store_debc_hlix
	EXTERN ____sdcc_store_debc_mhl
	EXTERN ____sdcc_cpu_pop_ei
	EXTERN ____sdcc_cpu_pop_ei_jp
	EXTERN ____sdcc_cpu_push_di
	EXTERN ____sdcc_outi
	EXTERN ____sdcc_outi_128
	EXTERN ____sdcc_outi_256
	EXTERN ____sdcc_ldi
	EXTERN ____sdcc_ldi_128
	EXTERN ____sdcc_ldi_256
	EXTERN ____sdcc_4_copy_srcd_hlix_dst_deix
	EXTERN ____sdcc_4_and_src_mbc_mhl_dst_deix
	EXTERN ____sdcc_4_or_src_mbc_mhl_dst_deix
	EXTERN ____sdcc_4_xor_src_mbc_mhl_dst_deix
	EXTERN ____sdcc_4_or_src_dehl_dst_bcix
	EXTERN ____sdcc_4_xor_src_dehl_dst_bcix
	EXTERN ____sdcc_4_and_src_dehl_dst_bcix
	EXTERN ____sdcc_4_xor_src_mbc_mhl_dst_debc
	EXTERN ____sdcc_4_or_src_mbc_mhl_dst_debc
	EXTERN ____sdcc_4_and_src_mbc_mhl_dst_debc
	EXTERN ____sdcc_4_cpl_src_mhl_dst_debc
	EXTERN ____sdcc_4_xor_src_debc_mhl_dst_debc
	EXTERN ____sdcc_4_or_src_debc_mhl_dst_debc
	EXTERN ____sdcc_4_and_src_debc_mhl_dst_debc
	EXTERN ____sdcc_4_and_src_debc_hlix_dst_debc
	EXTERN ____sdcc_4_or_src_debc_hlix_dst_debc
	EXTERN ____sdcc_4_xor_src_debc_hlix_dst_debc

;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	GLOBAL _update_and_draw_stars
	GLOBAL _init_stars
	GLOBAL _stars_erase_all
	GLOBAL _stars_set_count
;--------------------------------------------------------
; Externals used
;--------------------------------------------------------
	GLOBAL _set_attr_rect
	GLOBAL _erase_sprite_32
	GLOBAL _write_sprite_32
	GLOBAL _write_sprite
	GLOBAL _unplot
	GLOBAL _plot
	GLOBAL _scr_off
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	SECTION bss_compiler
_stars:
	DEFS 900
;--------------------------------------------------------
; ram data
;--------------------------------------------------------

IF 0

; .area _INITIALIZED removed by z88dk

_star_count:
	DEFS 1
_lfsr:
	DEFS 2
_weyl:
	DEFS 2

ENDIF

;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	SECTION IGNORE
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	SECTION code_crt_init
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	SECTION code_home
;--------------------------------------------------------
; code
;--------------------------------------------------------
	SECTION code_compiler
;	---------------------------------
; Function rng
; ---------------------------------
_rng:
	ld	bc, (_lfsr)
	ld	de, (_lfsr)
	srl	d
	rr	e
	srl	d
	rr	e
	ld	a, c
	xor	a, e
	ld	c, a
	ld	a, b
	xor	a, d
	ld	b, a
	ld	de, (_lfsr)
	ld	a,e
	srl	d
	rra
	srl	d
	rra
	srl	d
	rra
	ld	e,a
	ld	a, c
	xor	a, e
	ld	c, a
	ld	a, b
	xor	a, d
	ld	b, a
	ld	de, (_lfsr)
	ld	a,e
	srl	d
	rra
	srl	d
	rra
	srl	d
	rra
	srl	d
	rra
	srl	d
	rra
	ld	e,a
	ld	a, c
	xor	a, e
	ld	c, a
	ld	a, b
	xor	a, d
	ld	a, c
	and	a,0x01
	ld	e, a
	ld	bc, (_lfsr)
	srl	b
	rr	c
	ld	a, e
	rrca
	and	a,0x80
	ld	e, a
	ld	hl,_lfsr
	ld	(hl), c
	ld	a, e
	or	a, b
	inc	hl
	ld	(hl), a
	ld	hl,(_weyl)
	ld	de,0x9e35
	add	hl,de
	ld	(_weyl),hl
	ld	a,(_lfsr)
	ld	hl,_weyl
	xor	a, (hl)
	ld	c, a
	ld	a,(_lfsr + 1)
	ld	hl,_weyl + 1
	xor	a, (hl)
	ld	h, a
	ld	l, c
	ret
;	---------------------------------
; Function rand_xy
; ---------------------------------
_rand_xy:
	call	_rng
	ld	h,0x00
	ld	a, l
	add	a,0x80
	ld	l, a
	ld	a, h
	adc	a,0xff
	ld	h, a
	ret
;	---------------------------------
; Function rand_z
; ---------------------------------
_rand_z:
	call	_rng
	xor	a,a
	ld	c,l
	ld	e,l
	ld	d, a
	or	a, c
	jr	Z,l_rand_z_00103
	ex	de, hl
	jr	l_rand_z_00104
l_rand_z_00103:
	ld	hl,0x0001
l_rand_z_00104:
	ret
;	---------------------------------
; Function erase_star
; ---------------------------------
_erase_star:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	h,(ix+7)
	ld	l,(ix+6)
	push	hl
	ld	l,(ix+4)
	ld	h,(ix+5)
	push	hl
	call	_unplot
	pop	af
	pop	af
	ld	a,(ix+8)
	or	a, a
	jr	Z,l_erase_star_00103
	ld	c,(ix+6)
	inc	c
	push	bc
	ld	a,(ix+7)
	push	af
	inc	sp
	ld	a, c
	push	af
	inc	sp
	ld	l,(ix+4)
	ld	h,(ix+5)
	push	hl
	call	_unplot
	pop	af
	pop	af
	pop	bc
	ld	b,(ix+7)
	inc	b
	push	bc
	ld	c,(ix+6)
	push	bc
	ld	l,(ix+4)
	ld	h,(ix+5)
	push	hl
	call	_unplot
	pop	af
	pop	af
	ld	l,(ix+4)
	ld	h,(ix+5)
	push	hl
	call	_unplot
	pop	af
	pop	af
l_erase_star_00103:
	pop	ix
	ret
;	---------------------------------
; Function stars_set_count
; ---------------------------------
_stars_set_count:
	push	ix
	ld	ix,0
	add	ix,sp
	push	af
	ld	a,0x64
	sub	a,(ix+4)
	jr	NC,l_stars_set_count_00102
	ld	(ix+4),0x64
l_stars_set_count_00102:
	ld	c,(ix+4)
l_stars_set_count_00107:
	ld	a, c
	ld	hl,_star_count
	sub	a, (hl)
	jr	NC,l_stars_set_count_00105
	ld	b,0x00
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, bc
	ld	a, l
	add	a, +((_stars) & 0xFF)
	ld	(ix-2),a
	ld	a, h
	adc	a, +((_stars) / 256)
	ld	(ix-1),a
	ld	a,(ix-2)
	add	a,0x07
	ld	e, a
	ld	a,(ix-1)
	adc	a,0x00
	ld	d, a
	ld	a, (de)
	cp	a,0xff
	jr	Z,l_stars_set_count_00104
	pop	hl
	push	hl
	push	bc
	ld	bc,0x0008
	add	hl, bc
	pop	bc
	ld	b, (hl)
	pop	hl
	push	hl
	push	bc
	ld	bc,0x0006
	add	hl, bc
	pop	bc
	ld	h, (hl)
	push	bc
	push	de
	push	bc
	inc	sp
	ld	l,h
	ld	h,a
	push	hl
	ld	hl,0x4000
	push	hl
	call	_erase_star
	pop	af
	pop	af
	inc	sp
	pop	de
	pop	bc
l_stars_set_count_00104:
	ld	a,0xff
	ld	(de), a
	inc	c
	jr	l_stars_set_count_00107
l_stars_set_count_00105:
	ld	a,(ix+4)
	ld	(_star_count),a
	ld	sp, ix
	pop	ix
	ret
;	---------------------------------
; Function stars_erase_all
; ---------------------------------
_stars_erase_all:
	push	ix
	ld	ix,0
	add	ix,sp
	push	af
	ld	c,0x00
l_stars_erase_all_00105:
	ld	a, c
	ld	hl,_star_count
	sub	a, (hl)
	jr	NC,l_stars_erase_all_00107
	ld	b,0x00
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, bc
	ld	de,_stars
	add	hl,de
	ex	de,hl
	ld	hl,0x0007
	add	hl, de
	pop	af
	ld	a,(hl)
	push	hl
	cp	a,0xff
	jr	Z,l_stars_erase_all_00106
	ld	l, e
	ld	h, d
	push	bc
	ld	bc,0x0008
	add	hl, bc
	pop	bc
	ld	b, (hl)
	ld	hl,6
	add	hl, de
	ld	d, (hl)
	push	bc
	push	bc
	inc	sp
	ld	e,d
	ld	d,a
	push	de
	ld	hl,0x4000
	push	hl
	call	_erase_star
	pop	af
	pop	af
	inc	sp
	pop	bc
	pop	hl
	push	hl
	ld	(hl),0xff
l_stars_erase_all_00106:
	inc	c
	jr	l_stars_erase_all_00105
l_stars_erase_all_00107:
	ld	sp, ix
	pop	ix
	ret
;	---------------------------------
; Function init_stars
; ---------------------------------
_init_stars:
	push	ix
	ld	ix,0
	add	ix,sp
	dec	sp
	ld	(ix-1),0x00
l_init_stars_00103:
	ld	a,(ix-1)
	ld	hl,_star_count
	sub	a, (hl)
	jr	NC,l_init_stars_00105
	ld	l,(ix-1)
	ld	h,0x00
	ld	c,l
	ld	b,h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, bc
	ld	a, l
	add	a, +((_stars) & 0xFF)
	ld	c, a
	ld	a, h
	adc	a, +((_stars) / 256)
	ld	b, a
	push	bc
	call	_rand_xy
	ex	de, hl
	pop	hl
	ld	c,l
	ld	b,h
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, c
	ld	h, b
	inc	hl
	inc	hl
	push	hl
	push	bc
	call	_rand_xy
	ex	de, hl
	pop	bc
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl,0x0004
	add	hl, bc
	push	hl
	push	bc
	call	_rand_z
	ex	de, hl
	pop	bc
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl,0x0007
	add	hl, bc
	ld	(hl),0xff
	ld	hl,0x0008
	add	hl, bc
	ld	(hl),0x00
	inc	(ix-1)
	jr	l_init_stars_00103
l_init_stars_00105:
	inc	sp
	pop	ix
	ret
;	---------------------------------
; Function update_and_draw_stars
; ---------------------------------
_update_and_draw_stars:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	hl, -15
	add	hl, sp
	ld	sp, hl
	ld	(ix-1),0x00
l_update_and_draw_stars_00128:
	ld	a,(ix-1)
	ld	hl,_star_count
	sub	a, (hl)
	jp	NC, l_update_and_draw_stars_00130
	ld	l,(ix-1)
	ld	h,0x00
	ld	c,l
	ld	b,h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, bc
	ex	(sp), hl
	ld	l,(ix-15)
	ld	h,(ix-14)
	ld	bc,_stars
	add	hl,bc
	ld	(ix-13),l
	ld	(ix-12),h
	ld	bc,0x0007
	add	hl,bc
	ld	(ix-11),l
	ld	(ix-10),h
	ld	a, (hl)
	cp	a,0xff
	jr	Z,l_update_and_draw_stars_00102
	ld	c,(ix-13)
	ld	b,(ix-12)
	ld	hl,8
	add	hl, bc
	ld	d, (hl)
	ld	c,(ix-13)
	ld	b,(ix-12)
	ld	hl,6
	add	hl, bc
	ld	b, (hl)
	push	de
	inc	sp
	ld	c,b
	ld	b,a
	push	bc
	ld	hl,0x4000
	push	hl
	call	_erase_star
	pop	af
	pop	af
	inc	sp
l_update_and_draw_stars_00102:
	ld	a,(ix-13)
	ld	(ix-9),a
	ld	l, a
	ld	a,(ix-12)
	ld	(ix-8),a
	ld	h,a
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	a,(ix+4)
	ld	l, a
	rlca
	sbc	a, a
	ld	h, a
	add	hl, bc
	ex	de, hl
	ld	l,(ix-9)
	ld	h,(ix-8)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l,(ix-13)
	ld	h,(ix-12)
	ld	bc,0x0002
	add	hl,bc
	ld	(ix-7),l
	ld	(ix-6),h
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	a,(ix+5)
	ld	l, a
	rlca
	sbc	a, a
	ld	h, a
	add	hl, bc
	ex	de, hl
	ld	l,(ix-7)
	ld	h,(ix-6)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l,(ix-13)
	ld	h,(ix-12)
	ld	bc,0x0004
	add	hl,bc
	ld	(ix-5),l
	ld	(ix-4),h
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	a,(ix+6)
	ld	l, a
	rlca
	sbc	a, a
	ld	h, a
	add	hl, bc
	ex	de, hl
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	a, e
	sub	a,0x01
	ld	a, d
	rla
	ccf
	rra
	sbc	a,0x80
	jr	NC,l_update_and_draw_stars_00106
	call	_rand_xy
	ex	de, hl
	ld	l,(ix-9)
	ld	h,(ix-8)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	call	_rand_xy
	ex	de, hl
	ld	l,(ix-7)
	ld	h,(ix-6)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	(hl),0xff
	inc	hl
	ld	(hl),0x00
	jr	l_update_and_draw_stars_00107
l_update_and_draw_stars_00106:
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	a, (hl)
	ld	(ix-3),a
	inc	hl
	ld	a, (hl)
	ld	(ix-2),a
	ld	a,0xff
	cp	a,(ix-3)
	ld	a,0x00
	sbc	a,(ix-2)
	jp	PO, l_update_and_draw_stars_00241
	xor	a,0x80
l_update_and_draw_stars_00241:
	jp	P, l_update_and_draw_stars_00107
	call	_rand_xy
	ex	de, hl
	ld	l,(ix-9)
	ld	h,(ix-8)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	call	_rand_xy
	ex	de, hl
	ld	l,(ix-7)
	ld	h,(ix-6)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	(hl),0x01
	inc	hl
	ld	(hl),0x00
l_update_and_draw_stars_00107:
	ld	bc,_stars
	ld	l,(ix-15)
	ld	h,(ix-14)
	add	hl,bc
	ld	e, (hl)
	inc	hl
	ld	c, (hl)
	dec	hl
	ld	a, e
	sub	a,0x80
	ld	a, c
	rla
	ccf
	rra
	sbc	a,0x7f
	jr	NC,l_update_and_draw_stars_00111
	inc	d
	ld	(hl), e
	inc	hl
	ld	(hl), d
	jr	l_update_and_draw_stars_00112
l_update_and_draw_stars_00111:
	ld	a, e
	sub	a,0x80
	ld	a, c
	rla
	ccf
	rra
	sbc	a,0x80
	jr	C,l_update_and_draw_stars_00112
	ld	a,c
	add	a,0xff
	ld	(hl), e
	inc	hl
	ld	(hl), a
l_update_and_draw_stars_00112:
	ld	l,(ix-7)
	ld	h,(ix-6)
	ld	a,(hl)
	inc	hl
	ld	b,(hl)
	ld	c,a
	sub	a,0x80
	ld	a, b
	rla
	ccf
	rra
	sbc	a,0x7f
	jr	NC,l_update_and_draw_stars_00116
	inc	b
	ld	l,(ix-7)
	ld	h,(ix-6)
	ld	(hl), c
	inc	hl
	ld	(hl), b
	jr	l_update_and_draw_stars_00117
l_update_and_draw_stars_00116:
	ld	a, c
	sub	a,0x80
	ld	a, b
	rla
	ccf
	rra
	sbc	a,0x80
	jr	C,l_update_and_draw_stars_00117
	ld	a,b
	add	a,0xff
	ld	b, a
	ld	l,(ix-7)
	ld	h,(ix-6)
	ld	(hl), c
	inc	hl
	ld	(hl), b
l_update_and_draw_stars_00117:
	ld	l,(ix-9)
	ld	h,(ix-8)
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	xor	a, a
	rr	b
	ld	b, c
	rr	b
	rra
	ld	c, a
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	push	de
	push	de
	push	bc
	call	__divsint_callee
	pop	de
	ld	bc,0x0080
	add	hl,bc
	ld	c,l
	ld	l,(ix-7)
	ld	b,h
	ld	h,(ix-6)
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	xor	a, a
	rr	h
	ld	h, l
	rr	h
	rra
	ld	l, a
	push	bc
	push	de
	push	hl
	call	__divsint_callee
	pop	bc
	ld	a, l
	add	a,0x60
	ld	(ix-8),a
	ld	a, h
	adc	a,0x00
	ld	(ix-7),a
	bit	7, b
	jp	NZ, l_update_and_draw_stars_00121
	ld	a, b
	sub	a,0x01
	jp	NC, l_update_and_draw_stars_00121
	bit	7,(ix-7)
	jp	NZ, l_update_and_draw_stars_00121
	ld	a,(ix-8)
	sub	a,0xa0
	ld	a,(ix-7)
	sbc	a,0x00
	jp	NC, l_update_and_draw_stars_00121
	ld	e,(ix-8)
	ld	(ix-6),c
	push	bc
	push	de
	ld	a, e
	push	af
	inc	sp
	ld	a,(ix-6)
	push	af
	inc	sp
	ld	hl,0x4000
	push	hl
	call	_plot
	pop	af
	pop	af
	pop	de
	pop	bc
	ld	a,(ix-13)
	add	a,0x08
	ld	(ix-3),a
	ld	a,(ix-12)
	adc	a,0x00
	ld	(ix-2),a
	ld	l,(ix-5)
	ld	h,(ix-4)
	ld	d, (hl)
	inc	hl
	ld	l, (hl)
	ld	a, d
	sub	a,0x55
	ld	a, l
	rla
	ccf
	rra
	sbc	a,0x80
	jr	NC,l_update_and_draw_stars_00132
	ld	a,0x01
	jr	l_update_and_draw_stars_00133
l_update_and_draw_stars_00132:
	xor	a, a
l_update_and_draw_stars_00133:
	ld	l,(ix-3)
	ld	h,(ix-2)
	ld	(hl),a
	or	a, a
	jr	Z,l_update_and_draw_stars_00119
	inc	c
	ld	d, c
	push	de
	ld	a, e
	ld	e,d
	ld	d,a
	push	de
	ld	hl,0x4000
	push	hl
	call	_plot
	pop	af
	pop	af
	pop	de
	ld	a,(ix-8)
	inc	a
	ld	b, a
	push	bc
	push	de
	ld	c,(ix-6)
	push	bc
	ld	hl,0x4000
	push	hl
	call	_plot
	pop	af
	pop	af
	pop	de
	pop	bc
	push	de
	ld	c, d
	push	bc
	ld	hl,0x4000
	push	hl
	call	_plot
	pop	af
	pop	af
	pop	de
l_update_and_draw_stars_00119:
	ld	l,(ix-13)
	ld	h,(ix-12)
	ld	bc,0x0006
	add	hl, bc
	ld	a,(ix-6)
	ld	(hl), a
	ld	l,(ix-11)
	ld	h,(ix-10)
	ld	(hl), e
	jr	l_update_and_draw_stars_00129
l_update_and_draw_stars_00121:
	ld	l,(ix-11)
	ld	h,(ix-10)
	ld	(hl),0xff
l_update_and_draw_stars_00129:
	inc	(ix-1)
	jp	l_update_and_draw_stars_00128
l_update_and_draw_stars_00130:
	ld	sp, ix
	pop	ix
	ret
	SECTION data_compiler
_star_count:
	DEFB +0x64
_lfsr:
	DEFW +0xace1
_weyl:
	DEFW +0x0000
	SECTION IGNORE
