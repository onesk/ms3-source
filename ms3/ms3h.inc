;*********************************************************************
; MS3 equates for asm
;*********************************************************************
; $Id: ms3h.inc,v 1.73 2014/07/22 15:29:13 jsmcortina Exp $
; * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; *
;    Origin: James Murray
;    Major: James Murray / Kenneth Culver
;    Majority: James Murray / Kenneth Culver
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

include "ms3_structs.inc"
include "ms3h_defines.inc"
include "hcs12xedef.inc"

.macro SSEM0
    jsr do_ssem0
.endm

.macro CSEM0
movw #0x0100, XGSEMM
.endm

.macro SSEM0SEI
sei
    jsr do_ssem0
.endm

.macro CSEM0CLI
movw #0x0100, XGSEMM
cli
.endm

.macro SSEM1
    jsr do_ssem1
.endm

.macro CSEM1
movw #0x0200, XGSEMM
.endm

.macro FIRE_COIL
movw #0x0101, XGSWTM
.endm

.macro DWELL_COIL
movw #0x0202, XGSWTM
.endm

.macro FIRE_COIL_ROTARY
movw #0x0404, XGSWTM
.endm

.macro DWELL_COIL_ROTARY
movw #0x0808, XGSWTM
.endm

.macro STACK_SOFT
;these are the minimum to stack
movw 0x2800, 2, -SP
movw 0x2802, 2, -SP
movw 0x2804, 2, -SP
.endm

.macro UNSTACK_SOFT
;these are the minimum to stack
movw 2, SP+, 0x2804
movw 2, SP+, 0x2802
movw 2, SP+, 0x2800
.endm

