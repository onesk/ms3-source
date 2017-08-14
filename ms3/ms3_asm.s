;*********************************************************************
; Misc asm
;*********************************************************************
; $Id: ms3_asm.s,v 1.46 2014/07/22 15:29:13 jsmcortina Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; * Origin: James Murray / Kenneth Culver
; * Majority: James Murray / Kenneth Culver
; *
; * g_read/write: James Murray
; * cp2serial: James Murray
; *
; * MAP logger: James Murray 
; * 
; * int_sqrt: James Murray
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

.sect .text
.globl UnimplementedISR, set_coil, muldiv, xgswe_handler, memcmp
.globl g_read32, g_read16, g_read8, g_write32, g_write16, g_write8, g_read_copy
.globl cp2serialbuf, do_maplog1, do_maplog2, do_maplog_rpm, do_maplog_tooth, int_sqrt32
.globl do_englog
.globl do_ssem0, do_ssem1

             nolist               ;turn off listing
             include "ms3h.inc"
             list                 ;turn listing back on

;**************************************************************************
; Miscellaneous code in asm
; **************************************************************************
do_ssem0:
    pshy
    movw    TCNT, 2,-sp
dss0set:
    movw #0x0101, XGSEMM
    brset XGSEM, #0x01, dss0ex
    ldy TCNT
    suby    0,sp
    cpy #50 ; 50us timeout
    blo dss0set
    ; took too long, continue regardless
dss0ex:
    puly
    puly
    rts
;*********************************************************************
do_ssem1:
    pshy
    movw    TCNT, 2,-sp
dss1set:
    movw #0x0202, XGSEMM
    brset XGSEM, #0x02, dss1ex
    ldy TCNT
    suby    0,sp
    cpy #50 ; 50us timeout
    blo dss1set
    ; took too long, continue regardless
dss1ex:
    puly
    puly
    rts
;*********************************************************************

UnimplementedISR:
   ; Unimplemented ISRs trap
   rti

;void set_coil (unsigned int *coil, unsigned int *bits)

set_coil:
    tfr     d, x     ; coil
    ldy 	0x2, sp  ; bits

;    /* lookup from mapping array */
;    /* range checking/setting at boot time */

    ldaa     ram4.EngStroke
    cmpa     #3  ; Is it rotary?
    bne      nonrotary ; nope, go do the non rotary case
    ldaa     num_spk  
    cmpa     #4        ; We are rotary, but 4 or more coils is COP 2,3,
                       ; or 4 rotor, so handle it like non-rotary
    bhs      nonrotary ; anything 4 or more coils handled like normal

    ; Rotary FC
    ldx     0x0,X
    aslx
;            bitval = setcoil_array_fcfd[*coil];
    ldd     setcoil_array_fcfd, x
    bra     sc_store

nonrotary:
;        bitval = setcoil_array_norm[num_spk][*coil];
    ldaa    #16
    ldab	num_spk
    decb
    mul
    addd	0x0,X
    asld
    tfr     D,X
    ldd     setcoil_array_norm, x

sc_store:
    SSEM0
    oraa	0x0,Y
    orab	0x1,Y
    std     0x0,Y
    CSEM0
    rts

;----------------------------

.sect .text
;U32 x U16 / 65536
; put this in low mem, so jsr gets us here
; on entry:
; D is U16
; U32 on stack at 2,SP and 4,SP
; returns result in X:D
muldiv:
    pshd
    clrw 2,-sp ; lower word of U32 result 0,SP
;now U16 is 2,SP
;    U32 is 6,SP  8,SP

    ldd     8,SP
    ldy     2,SP
    emul
    sty     0,SP
    ldd     6,SP
    ldy     2,SP
    emul
    addd    0,SP
    std     0,SP
    bcc     noaddy
    iny
noaddy:
    tfr     y,x
    leas    4,SP
    rts

xgswe_handler:
; this interrupt happens if a software error occurs on the XGATE
    ldd     XGPC
    std     xgpc_copy ; save the XGATE PC
    inc     xgswe_count ; increment error counter
    STACK_SOFT
    call    config_xgate ; reconfigure the XGATE
    UNSTACK_SOFT
    rti

;*********************************************************************
memcmp:
; on entry D is s1 address
; first word off stack is 16bit s2 address
; second word off stack is number of bytes
; returns 0 if match or 1 if not (not correct to man page)
; destroys d,x,y

    tfr     d,x  ; S1
    ldy     2,sp ; S2
    ldd     4,sp ; count
memcmp_loop:
    pshd
    ldab    1,x+
    cmpb    1,y+
    bne     memcmp_different
    puld
    dbne    d,memcmp_loop
; D returns as zero
    rts
memcmp_different:
    puld
    ldd     #1 ; non-zero return
    rts
;*********************************************************************

;*********************************************************************
g_read32:
; Global read 32bits
; on entry D is 16bit address
; returns in X:D
    tfr     d,y
    gldx    2,y+
    gldd    0,y
	rts
;*********************************************************************

;*********************************************************************
g_read16:
; Global read 16 bits
; on entry D is 16bit address
; returns in  D
    tfr     d,y
    gldd    0,y
	rts
;*********************************************************************

;*********************************************************************
g_read8:
; Global read 8 bits
; on entry D is 16bit address
; returns in B
    tfr     d,y
    gldab   0,y
	rts
;*********************************************************************

;*********************************************************************
g_write32:
; Global read 32bits
; On entry X:D is 32bit value
; first word off stack is 16bit address
    ldy     2,sp
    gstx    2,y+
    gstd    0,y
	rts
;*********************************************************************

;*********************************************************************
g_write16:
; Global read 16 bits
; on entry D is 16bit value
; first word off stack is 16bit address
    ldy     2,sp
    gstd    0,y
	rts
;*********************************************************************

;*********************************************************************
g_write8:
; Global read 8 bits
; on entry B is 8bit value
; first word off stack is 16bit address
    ldy     2,sp
    gstab   0,y
	rts
;*********************************************************************

g_read_copy:
; (src, count, dest)
; copies from global memory to local memory
; on entry D is source address
; first word off stack is count in bytes
; second word off stack is dest address
; returns 1 if changed
; sets/clears interrupt flag
    tfr     d,x
    ldy     4,sp
    clr     5,sp
grc_lp:
    ldd     2,sp
    cmpd    #4
    blo     grc2

;4 bytes
    subd    #4
    std     2,sp
    gldd    2,x+
    cmpd    0,y
    beq     grc1k
    movb    #1,5,sp
grc1k:
    sei
    std     2,y+
    gldd    2,x+
    cmpd    0,y
    beq     grc1k2
    movb    #1,5,sp
grc1k2:
    std     2,y+
    cli
    bra     grc_lp
grc2:
    cmpd    #2
    blo     grc3

;2 bytes
    subd    #2
    std     2,sp
    gldd    2,x+
    cmpd    0,y
    beq     grc2k
    movb    #1,5,sp
grc2k:
    std     2,y+
    bra     grc_lp

grc3:
    cmpd    #1
    blo     grc4
;1 byte
    subd    #1
    std     2,sp
    gldaa   1,x+
    cmpa    0,y
    beq     grc3k
    movb    #1,5,sp
grc3k:
    staa    1,y+
    bra     grc_lp

grc4:
    ldab    5,sp
    rts


.sect .textf3
;-----------------------
cp2serialbuf:
;on entry D = target global address, 3,SP = source address, 5,SP = no. bytes
; clobbers all regs
; requires GPAGE set before entry
    tfr     d,y
    addy    5,sp
    sty     5,sp    ; holds new target
    tfr     d,y
    ldx     3,sp
cp2s1:
    ; copy in groups of 4 words - (even though this will overrun)
    ; matches CAN transfers and requires that data does not span 8 byte boundaries
    sei
    ldd     2,x+
    gstd    2,y+
    ldd     2,x+
    gstd    2,y+
    ldd     2,x+
    gstd    2,y+
    ldd     2,x+
    gstd    2,y+
    cli

    cpy     5,sp
    blo     cp2s1

    rtc

; --------------------------------
; map logging subroutine
; called from isr_rtc and from ign_in
; --------------------------------
.sect .text
do_maplog1:
    pshy
    pshx
    pshd
    bra     do_map_log_com_map

do_maplog2:
    pshy
    pshx
    pshd
    orx     #0x0400 ; map sensor 2 
;    bra     do_map_log_com_map

do_map_log_com_map:
; at low rpms, can end up logging too often
    dec     maplog_cnt
    bne     mapldone    ; not yet
    movb    maplog_max, maplog_cnt

do_map_log_com:
    brclr   flagbyte15, FLAGBYTE15_MAPWINDOW, do_map_log_com2
    orx     #0x4000
do_map_log_com2:
    ;on entry X contains MAP value to log
    ; bit 15 = 0 means MAP data
    ; MAP sensor no. in bits 12,11,10
    ; bits 9-0 are raw MAP ADC

    ldy	    #TRIGLOGBASE
    addy	log_offset

;swap page in ram window
    cpy     #0x1ffd ; shouldn't happen, but sanity check
    bhi     maplfin

    ldaa    RPAGE
    movb    #TRIGLOGPAGE, RPAGE
    stx	    0,y
    staa    RPAGE

    ldy     log_offset
    iny
    iny
    sty	    log_offset
    cpy	    #0x3fc   ; three less
    bls	    mapldone

maplfin:
    ; we've reached the end of the buffer, stop
    bclr	flagbyte0, #FLAGBYTE0_MAPLOG
    bset    outpc.status3, #STATUS3_DONELOG

mapldone:
    puld
    pulx
    puly

    rts
;----

do_maplog_rpm:
    pshy
    pshx
    pshd

    ldx     outpc.rpm
    cpx     #0x3fff
    bls     dmlrk
    ldx     #0x3fff  ; rail
dmlrk:
    orx     #0xc000 ; is rpm
    bra     do_map_log_com2
;----

do_maplog_tooth:
    pshy
    pshx
    pshd
    clra
    ldab    tooth_no ; byte
    tfr     d,x
    orx     #0x8000 ; is tooth no. (mainloop with convert to angle at end)
    bra     do_map_log_com2
; ---------------------

; --------------------------------
; engine logging subroutine
; called from isr_rtc
; --------------------------------
.sect .text
do_englog:
; at low rpms, can end up logging too often
    dec     maplog_cnt
    bne     engldone    ; not yet
    movb    maplog_max, maplog_cnt

    ldy	    #TRIGLOGBASE
    addy	log_offset

;swap page in ram window
    cpy     #0x1ffd ; shouldn't happen, but sanity check
    bhi     englfin

    movb    RPAGE, 1,-sp
    movb    #TRIGLOGPAGE, RPAGE
    ldab    page
    cmpb    #0xf7
    beq     engl_map
    cmpb    #0xf8
    beq     engl_maf

;no MAP or MAF, 3 bytes   
    movb	PTT, 1,y+
    movb	PORTB, 1,y+
    movb	PORTA, 0,y

    ldy     log_offset
    bra     englcom ; skips one iny

;MAP or MAF, 4 bytes
engl_map:
    ldx     mapport
    ldd     0,x
    bra     engl_mapmaf

engl_maf:
    ldx     mafport
    ldd     0,x

engl_mapmaf:
    lsra
    rorb
    lsra
    rorb
    stab    1,y+    ; MAPADC/4     
    movb	PTT, 1,y+
    movb	PORTB, 1,y+
    movb	PORTA, 0,y

    ldy     log_offset
    iny
englcom:
    movb    1,sp+, RPAGE
    iny
    iny
    iny

    sty	    log_offset
    cpy	    #0x3fc   ; three less
    bls	    engldone

englfin:
    ; we've reached the end of the buffer, stop
    bclr	flagbyte0, #FLAGBYTE0_ENGLOG
    bset    outpc.status3, #STATUS3_DONELOG

engldone:
    rts
;----

.sect .textf2
;unsigned int int_sqrt32(unsigned long x)
int_sqrt32:
    leas    -9,sp
    std     2,sp
    stx     0,sp

;    /* Derived from C code in http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2 */
    clrw	6,sp
    movw	#0x8000, 4,sp

    movb    #16,8,sp

LSM2515:
    ldy     6,sp
    ory     4,sp  ; temp = add | res 
    tfr     y,d
    tfr     y,x

    emul  ; g2 = temp ^2  (g2 = Y:D)

;    if (g2 < x) {
    cmpy    0,sp
    bhi     LSM2519
    blo     LSM2518
    cmpd    2,sp
    bhi     LSM2519    

LSM2518:
    stx     6,sp ; res = temp

LSM2519:
    lsrw    4,sp ; add >>= 1

LBE134:
    dec     8,sp
    bne     LSM2515

    ldd     6,sp
    leas    9,sp
 	rtc

