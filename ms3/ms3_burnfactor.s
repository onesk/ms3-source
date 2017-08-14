; $Id: ms3_burnfactor.s,v 1.15 2013/01/23 20:32:26 jsmcortina Exp $
; * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; * Origin: James Murray
; * Majority: James Murray
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

.sect .textfc
.globl erasefactor

;.nolist
include "ms3h.inc"
;.list

;*********************************************************************
; burn a lookup table from SCI
; first step erase sector(s)
;
;*********************************************************************
erasefactor:
       pshd
       pshx
       pshy
       ldaa    flocker
       cmpa    #0xcc
       bne     erend

       call     Flash_Init ; ensure initialised

       ldaa    burn_idx

       cmpa    #4
       bhs     erend

       ldab    #8
       mul
       tfr     d,x
       ldd    tables+4,x   ; HARDCODING - size tables array
       ldx    tables+2,x   ; HARDCODING - d-flash low word address from tables array

       brclr   FSTAT,#CCIF,.   ; loop until no command in action

        tfr     a,b
;   B now contains no. 256 byte sectors to erase

;erase the 256b sectors

burnerase:
       movb    #0,FCCOBIX
       movb    #0x12,FCCOBHI
       movb    #0x10,FCCOBLO   ; global address of all D-flash

       movb    #1,FCCOBIX
       stx     FCCOBHI         ; X set above, points to start of sector to erase

       movb    #CCIF, FSTAT
       nop
       brclr   FSTAT, #CCIF, . ; wait until command has completed

       ldaa    FSTAT
       bita    #0x30
       bne     erend        ; received error - simply bail out

       movb    #ACCERR|PVIOL, FSTAT
       addx    #0x100
       dbne    b,burnerase ; loop until required sectors erased

erend:
       clr     flocker
       puly
       pulx
       puld
       rtc    ; in banked mem


