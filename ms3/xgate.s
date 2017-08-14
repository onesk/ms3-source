;*********************************************************************
; XGATE code
;$Id: xgate.s,v 1.121.4.3 2015/05/19 20:40:10 jsmcortina Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; * Origin: James Murray / Kenneth Culver
; * Majority: James Murray / Kenneth Culver
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

;*********************************************************************

.sect .textfa  ; so addresses are in 0xXX8000 range to make sense
.globl XGATE_CODE_FLASH, XGATE_CODE_FLASH_END, XGATE_DUMMY_ISR_FLASH, xgss_tc1, xgss_inj1, xgss_inj2, xgss_inj3, xgss_inj4, xgss_inj5, xgss_inj6, xgss_inj7, xgss_inj8, xgss_xgate0, xgss_xgate1, xgss_tc6, xgss_xgate2, xgss_tc3, xgss_xgate3, xgss_tc7, cam_tc4, cam_tc5, crank_tc0, xgss_pit2
.globl random_no

.equ next_inj_0, next_inj
.equ next_inj_1, next_inj+6
.equ next_inj_2, next_inj+12
.equ next_inj_3, next_inj+18
.equ next_inj_4, next_inj+24
.equ next_inj_5, next_inj+30
.equ next_inj_6, next_inj+36
.equ next_inj_7, next_inj+42
.equ next_inj_8, next_inj+48
.equ next_inj_9, next_inj+54
.equ next_inj_10, next_inj+60
.equ next_inj_11, next_inj+66
.equ next_inj_12, next_inj+72
.equ next_inj_13, next_inj+78
.equ next_inj_14, next_inj+84
.equ next_inj_15, next_inj+90
.equ inj1_cnt, inj_cnt
.equ inj2_cnt, inj_cnt+2
.equ inj3_cnt, inj_cnt+4
.equ inj4_cnt, inj_cnt+6
.equ inj5_cnt, inj_cnt+8
.equ inj6_cnt, inj_cnt+10
.equ inj7_cnt, inj_cnt+12
.equ inj8_cnt, inj_cnt+14
.equ inj9_cnt, inj_cnt+16
.equ inj10_cnt, inj_cnt+18
.equ inj11_cnt, inj_cnt+20
.equ inj12_cnt, inj_cnt+22
.equ inj13_cnt, inj_cnt+24
.equ inj14_cnt, inj_cnt+26
.equ inj15_cnt, inj_cnt+28
.equ inj16_cnt, inj_cnt+30

             nolist               ;turn off listing
             include "hcs12xedef.inc"
             include "ms3_structs.inc"
             include "ms3h.inc"
             list                 ;turn listing back on
.org 0x200
; code starts at 0x8200 in XGATE space. Force that offset within section
; so that jump tables work and disassembled addresses make sense.
;*********************************************************************
XGATE_CODE_FLASH:
xgate_isr_start:
;forced spark output by software trigger
xgss_xgate0:
    ssem    #0              ; attempt to set semaphore 0
    bcc     xgss_xgate0

    ldw     r2, #XGSWTM
    ldw     r1,#0x0100      ; opposite of other flags, writing a 0 clears it.
    stw     r1,(r0,r2)      ; clear int flag
    bra     xg_ig_com

;spark output on TC1
xgss_tc1:
    ssem    #0              ; attempt to set semaphore 0
    bcc     xgss_tc1
    ldw     r2, #TFLG1      ; load interrupt flag address
    ldl     r3, #0x02
    stb     r3,(r0,r2)      ; clear interrupt flag

    ldw     r2, #TIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xFD        ; clear bit 0x02
    stb     r3,(r0,r2)      ; clear interrupt enable

    bra     xg_ig_com

xg_ig_com:
    ldw     r4,#xgate_deadman
    stb     r0,(r0,r4)      ; clear the deadman

    ldw     r4,#flagbyte10
    ldb     r6,(r0,r4)      ; set R6 to flagbyte10 for ICIgnOption spark hi/lo

    ldw     r4,#coilsel  ; set R4 to &coilsel
    ldw     r5,(r0,r4)      ; set R5 to coilsel data
    tst     r5              ; required on XGATE before bne/beq
    bne     xgigcm2
    bra     xgdn            ; if coilsel zero, just exit

xgigcm2:

; Clear overdwell timers

;    ldw     r5,(r0,r4) ; set just above

cod1:
    bitl    r5,#1
    beq     cod2
    ldw     r3,#dwl
    stb     r0,(r0,r3)

cod2:
    bitl    r5,#2
    beq     cod3
    ldw     r3,#dwl+1
    stb     r0,(r0,r3)

cod3:
    bitl    r5,#4
    beq     cod4
    ldw     r3,#dwl+2
    stb     r0,(r0,r3)

cod4:
    bitl    r5,#8
    beq     cod5
    ldw     r3,#dwl+3
    stb     r0,(r0,r3)

cod5:
    bitl    r5,#0x10
    beq     cod6
    ldw     r3,#dwl+4
    stb     r0,(r0,r3)

cod6:
    bitl    r5,#0x20
    beq     cod7
    ldw     r3,#dwl+5
    stb     r0,(r0,r3)

cod7:
    bitl    r5,#0x40
    beq     cod8
    ldw     r3,#dwl+6
    stb     r0,(r0,r3)

cod8:
    bitl    r5,#0x80
    beq     cod9
    ldw     r3,#dwl+7
    stb     r0,(r0,r3)

cod9:
    bith    r5,#1
    beq     cod10
    ldw     r3,#dwl+8
    stb     r0,(r0,r3)

cod10:
    bith    r5,#2
    beq     cod11
    ldw     r3,#dwl+9
    stb     r0,(r0,r3)

cod11:
    bith    r5,#4
    beq     cod12
    ldw     r3,#dwl+10
    stb     r0,(r0,r3)

cod12:
    bith    r5,#8
    beq     cod13
    ldw     r3,#dwl+11
    stb     r0,(r0,r3)

cod13:
    bith    r5,#0x10
    beq     cod14
    ldw     r3,#dwl+12
    stb     r0,(r0,r3)

cod14:
    bith    r5,#0x20
    beq     cod15
    ldw     r3,#dwl+13
    stb     r0,(r0,r3)

cod15:
    bith    r5,#0x40
    beq     cod16
    ldw     r3,#dwl+14
    stb     r0,(r0,r3)

cod16:
    bith    r5,#0x80
    beq     cod_done
    ldw     r3,#dwl+14
    stb     r0,(r0,r3)
cod_done:

    stw     r0,(r0,r4)  ; clear coilsel - we have the data in R5

; By clearing this here and having the setbit data in R5, should be able to use
; a common bit flipping code section for spark & dwell.

;Check spkmode
    ldw     r4,#spkmode
    ldb     r2,(r0,r4)
    cmpl    r2,#14
    beq     is14
    andl    r2,#0xfe
    cmpl    r2,#2
    bne     cktas   ; not modes 2 or 3, check for TAS
is14:
    ldw     r4,#dwell_us
    ldw     r2,(r0,r4)          ; dwell in tics
    cmp     r2,#0
    beq     ck_dli               ; zero so skip
    ldw     r4,#dwellsel_next
    ldw     r5,(r0,r4)
    bra     dwell_back          ; set dwell timer off back of this spark

;Check dwell mode
cktas:
    ldw     r4,#ram4.dwellmode
    ldb     r2,(r0,r4)
    andl    r2,#3
    cmpl    r2,#2
    bne     ck_dli  ; not mode 2
    
dwell_back:
; Distributor dwell timed off back of spark or
; time after spark. If we are running this wacky mode, then set up 'dwell' right now
    ldw     r4,#dwellsel     ; set R4 to &coilsel ??
    stw     r5,(r0,r4)          ; dwellsel = coilsel

    ldw     r4,#dwell_us
    ldw     r2,(r0,r4)          ; dwell in tics
    tst     r2
    beq     ck_dli              ; not for zero dwell
    ldw     r4,#TC1
    ldw     r1,(r0,r4)          ; timer value
    add     r1,r1,r2
    ldw     r4,#TC7
    stw     r1,(r0,r4)
    ldw     r4,#TIE
    ldb     r1,(r0,r4)
    orl     r1,#0x80            ; bset    TIE,#0x80
    stb     r1,(r0,r4)
    ldw     r4,#TFLG1
    ldl     r1,#0x80
    stb     r1,(r0,r4)          ; movb    #0x80,TFLG1

ck_dli:
    ldw     r2,#ram4.hardware
    ldb     r7,(r0,r2)

;check for DLI
    ldw     r4,#flagbyte11
    ldb     r2,(r0,r4)
    bitl    r2,#FLAGBYTE11_DLI4 | FLAGBYTE11_DLI6
    beq     xdc10 ; not DLI
    bra     dli_spk

xdc10:

    bitl    r7,#HARDWARE_MS3XSPK
    beq     xg_ig_ug  ; LEDs or JS10 for spark
;*********************************************************************
; This is the spark output interrupt for 'MS3X' spark pins
;*********************************************************************
xg_ig_ms3x:
    bitl    r6,#FLAGBYTE10_SPKHILO        ; inverted or non-inverted outputs
    beq     xghi

xglo:  
; this is mode where 5v = dwell, 0v = spark
; coilsel is inverse of AND mask
    ldw     r2,#PORTB       ; set R2 to &PORTB

xgl_norm:
    xnorl   r5,#0  ; inverse, r5 is coilsel
    ldb     r3,(r0,r2)    ; grab PORT data to R3... r2 is PORTB
    and     r3,r3,r5      ; clear bits as per coilsel
    stb     r3,(r0,r2)    ; store data to PORT

    andh    r5, #0xff
    beq     xgdn        ; <9 spark outputs / top byte is clear
    lsr     r5, #8       ; move upper byte to lower
    bra     xg_uglo     ; use LED code
;    bra xgdn

xghi:
; this is mode where 5v = spark, 0v = dwell
; coilsel is OR mask
    ldw     r2,#PORTB       ; set R2 to &PORTB

xghi_norm:
    ldb     r3,(r0,r2)    ; grab PORT data to R3
    or      r3,r3,r5      ; set bits as per coilsel
    stb     r3,(r0,r2)    ; store data to PORT

    andh    r5, #0xff
    beq     xgdn          ; <9 spark outputs / top byte is clear
    lsr     r5, #8       ; move upper byte to lower
    bra     xg_ughi     ; use LED code
;    bra xgdn

;*********************************************************************
; This is the spark output interrupt for 'upgrade' spark pins
;*********************************************************************
xg_ig_ug:
    bitl    r6,#FLAGBYTE10_SPKHILO        ; inverted or non-inverted outputs
    beq     xg_ughi

; ----- "inverted" -----
; inverted mode sets the CPU output pin to 0 for spark
; labels have a 0 in them. S12X code used 1 in labels.
xg_uglo:

xgigug0c1:
    bitl    r5, #0x01
    beq     xgigug0c2 ; not 1
;uses port/pin method to save lots of tests and branches
;load pin port/pin and mask out bit

    ldw     r1, #port_spki
    ldw     r1, (r0,r1)    ; load in the port address
    ldw     r2, #pin_spki
    ldb     r2, (r0,r2)    ; load in the pin value
    xnorl   r2, #0          ; flip it to make a mask
    ldb     r3, (r0,r1)     ; load in the port value
    and     r3, r2, r3      ; mask out that bit
    stb     r3, (r0,r1)     ; save back the value

xgigug0c2:
    bitl    r5, #0x02
    beq     xgigug0c3 ; not 2

;uses port/pin method to save lots of tests and branches
;load pin port/pin and mask out bit
    ldw     r1, #port_spkj
    ldw     r1, (r0,r1)    ; load in the port address
    ldw     r2, #pin_spkj
    ldb     r2, (r0,r2)    ; load in the pin value
    xnorl   r2, #0          ; flip it to make a mask
    ldb     r3, (r0,r1)     ; load in the port value
    and     r3, r2, r3      ; mask out that bit
    stb     r3, (r0,r1)     ; save back the value

xgigug0c3:
    bitl    r5, #0x04
    beq     xgigug0c4 ; not 3
    ldw     r1, #PTM
    ldb     r3, (r0,r1)
    andl    r3, #0xdf ; clear 0x20
    stb     r3, (r0,r1)

xgigug0c4:
    bitl    r5, #0x08
    beq     xgigug0c5 ; not 4
    ldw     r1, #PTJ
    ldb     r3, (r0,r1)
    andl    r3, #0x7f ; clear 0x80
    stb     r3, (r0,r1)

xgigug0c5:
    bitl    r5, #0x10
    beq     xgigug0c6 ; not 5
    ldw     r1, #PTAD0L
    ldb     r3, (r0,r1)
    andl    r3, #0xbf ; clear 0x40
    stb     r3, (r0,r1)

xgigug0c6:
    bitl    r5, #0x20
    beq     xgdn ; not 6
    ldw     r1, #PTAD0L
    ldb     r3, (r0,r1)
    andl    r3, #0x7f ; clear 0x80
    stb     r3, (r0,r1)
    bra      xgdn

;---------- "normal" -------
; CPU pin set to a 1 on spark
xg_ughi:

xgigug1c1:
    bitl    r5, #0x01
    beq     xgigug1c2 ; not 1
;uses port/pin method to save lots of tests and branches
;load pin port/pin and mask out bit
    ldw     r1, #port_spki
    ldw     r1, (r0,r1)    ; load in the port address
    ldw     r2, #pin_spki
    ldb     r2, (r0,r2)    ; load in the pin value
    ldb     r3, (r0,r1)     ; load in the port value
    or      r3, r2, r3      ; mask out that bit
    stb     r3, (r0,r1)     ; save back the value

xgigug1c2:
    bitl    r5, #0x02
    beq     xgigug1c3 ; not 2

;uses port/pin method to save lots of tests and branches
;load pin port/pin and mask out bit
    ldw     r1, #port_spkj
    ldw     r1, (r0,r1)    ; load in the port address
    ldw     r2, #pin_spkj
    ldb     r2, (r0,r2)    ; load in the pin value
    ldb     r3, (r0,r1)     ; load in the port value
    or      r3, r2, r3      ; mask out that bit
    stb     r3, (r0,r1)     ; save back the value

xgigug1c3:
    bitl    r5, #0x04
    beq     xgigug1c4 ; not 3
    ldw     r1, #PTM
    ldb     r3, (r0,r1)
    orl     r3, #0x20
    stb     r3, (r0,r1)

xgigug1c4:
    bitl    r5, #0x08
    beq     xgigug1c5 ; not 4
    ldw     r1, #PTJ
    ldb     r3, (r0,r1)
    orl     r3, #0x80
    stb     r3, (r0,r1)

xgigug1c5:
    bitl    r5, #0x10
    beq     xgigug1c6 ; not 5
    ldw     r1, #PTAD0L
    ldb     r3, (r0,r1)
    orl     r3, #0x40
    stb     r3, (r0,r1)

xgigug1c6:
    bitl    r5, #0x20
    beq     xgdn ; not 6
    ldw     r1, #PTAD0L
    ldb     r3, (r0,r1)
    orl     r3, #0x80
    stb     r3, (r0,r1)

xgdn:
    csem    #0          ; clear semaphore
    rts

;*********************************************************************
; DLI spark
;*********************************************************************
dli_spk:
    bitl    r7,#HARDWARE_MS3XSPK
    beq     dli_ug ; LEDs or JS10 for spark
;*********************************************************************
; DLI MS3X
;*********************************************************************
dli_ms3x:
    ldw     r1,#PORTB       ; set R2 to &PORTB

    bitl    r6,#FLAGBYTE10_SPKHILO        ; inverted or non-inverted outputs
    beq     dlixhi

dlixlo:  
; this is mode where 5v = dwell, 0v = spark
;clear IGt = spark
    ldb     r3, (r0,r1)
    andl    r3, #0xfe

    bitl    r2,#FLAGBYTE11_DLI4
    bne     dli4xlo
    bitl    r2,#FLAGBYTE11_DLI6
    bne     dli6xlo
    bra     xgdn ; just in case
    
; DLI4
dli4xlo:
    bitl    r5,#1
    bne     dli4xloa
    bitl    r5,#2
    bne     dli4xlob
    bra     xgdn

dli4xloa:
    andl    r3, #0xfd
    bra     dliug_dn

dli4xlob:
    orl     r3, #0x02
    bra     dliug_dn

;DLI6
dli6xlo:
    bitl    r5,#1
    bne     dli6xloa
    bitl    r5,#2
    bne     dli6xlob
    bitl    r5,#4
    bne     dli6xloc
    bra     xgdn

;IGdA = 0x02, IGdB = 0x04
dli6xloa:
;IGdA = 0 , IGdB = 0
    andl    r3, #0xf9
    bra     dliug_dn

dli6xlob:
;IGdA = 1 , IGdB = 0
    orl     r3, #0x02
    andl    r3, #0xfb
    bra     dliug_dn

dli6xloc:
;IGdA = 0 , IGdB = 1
    andl    r3, #0xfd
    orl     r3, #0x04
    bra     dliug_dn


dlixhi:
; this is mode where 0v = dwell, 5v = spark (at CPU)
;set IGt = spark
    ldb     r3, (r0,r1)
    orl     r3, #0x01

    bitl    r2,#FLAGBYTE11_DLI4
    bne     dli4xhi
    bitl    r2,#FLAGBYTE11_DLI6
    bne     dli6xhi
    bra     xgdn ; just in case
    
; DLI4
dli4xhi:
    bitl    r5,#1
    bne     dli4xhia
    bitl    r5,#2
    bne     dli4xhib
    bra     xgdn

dli4xhia:
    orl     r3, #0x02
    bra     dliug_dn

dli4xhib:
    andl    r3, #0xfd
    bra     dliug_dn

;DLI6
dli6xhi:
    bitl    r5,#1
    bne     dli6xhia
    bitl    r5,#2
    bne     dli6xhib
    bitl    r5,#4
    bne     dli6xhic
    bra     xgdn

;IGdA = 0x02, IGdB = 0x04
dli6xhia:
;IGdA = 0 , IGdB = 0
    orl     r3, #0x06
    bra     dliug_dn

dli6xhib:
;IGdA = 1 , IGdB = 0
    andl    r3, #0xfd
    orl     r3, #0x04
    bra     dliug_dn

dli6xhic:
;IGdA = 0 , IGdB = 1
    orl     r3, #0x02
    andl    r3, #0xfb
    bra     dliug_dn

;*********************************************************************
; DLI UG
;*********************************************************************
dli_ug:
    ldw     r1,#PORTM       ; set R1 to &PORTM

    bitl    r6,#FLAGBYTE10_SPKHILO        ; inverted or non-inverted outputs
    beq     dliughi

dliuglo:
; this is mode where 5v = dwell, 0v = spark (at CPU)
;clear IGt = spark
    ldb     r3, (r0,r1)
    andl    r3, #0xf7

    bitl    r2,#FLAGBYTE11_DLI4
    bne     dli4uglo
    bitl    r2,#FLAGBYTE11_DLI6
    bne     dli6uglo
    bra     xgdn ; just in case
    
; DLI4
dli4uglo:
    bitl    r5,#1
    bne     dli4ugloa
    bitl    r5,#2
    bne     dli4uglob
    bra     xgdn

dli4ugloa:
    andl    r3, #0xef
    bra     dliug_dn

dli4uglob:
    orl     r3, #0x10
    bra     dliug_dn

;DLI6
dli6uglo:
    bitl    r5,#1
    bne     dli6ugloa
    bitl    r5,#2
    bne     dli6uglob
    bitl    r5,#4
    bne     dli6ugloc
    bra     xgdn

;IGdA = 0x10, IGdB = 0x20
dli6ugloa:
;IGdA = 0 , IGdB = 0
    andl    r3, #0xcf
    bra     dliug_dn

dli6uglob:
;IGdA = 1 , IGdB = 0
    orl     r3, #0x10
    andl    r3, #0xdf
    bra     dliug_dn

dli6ugloc:
;IGdA = 0 , IGdB = 1
    andl    r3, #0xef
    orl     r3, #0x20
    bra     dliug_dn


dliughi:
; this is mode where 0v = dwell, 5v = spark (at CPU)
;set IGt = spark
    ldb     r3, (r0,r1)
    orl     r3, #0x08

    bitl    r2,#FLAGBYTE11_DLI4
    bne     dli4ughi
    bitl    r2,#FLAGBYTE11_DLI6
    bne     dli6ughi
    bra     xgdn ; just in case
    
; DLI4
dli4ughi:
    bitl    r5,#1
    bne     dli4ughia
    bitl    r5,#2
    bne     dli4ughib
    bra     xgdn

dli4ughia:
    orl     r3, #0x10
    bra     dliug_dn

dli4ughib:
    andl    r3, #0xef
    bra     dliug_dn

;DLI6
dli6ughi:
    bitl    r5,#1
    bne     dli6ughia
    bitl    r5,#2
    bne     dli6ughib
    bitl    r5,#4
    bne     dli6ughic
    bra     xgdn

;IGdA = 0x10, IGdB = 0x20
dli6ughia:
;IGdA = 0 , IGdB = 0
    orl     r3, #0x30
    bra     dliug_dn

dli6ughib:
;IGdA = 1 , IGdB = 0
    andl    r3, #0xef
    orl     r3, #0x20
    bra     dliug_dn

dli6ughic:
;IGdA = 0 , IGdB = 1
    orl     r3, #0x10
    andl    r3, #0xdf
    bra     dliug_dn

dliug_dn:
    stb     r3, (r0,r1)
    bra     xgdn

;*********************************************************************
; Dwell
;*********************************************************************
xgss_tc7:
    ssem    #0
    bcc     xgss_tc7

    ldw     r2, #TFLG1      ; load interrupt flag address
    ldl     r3, #0x80
    stb     r3,(r0,r2)      ; clear interrupt flag

    ldw     r2, #TIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0x7F        ; clear bit 0x80
    stb     r3,(r0,r2)      ; clear interrupt enable

xgtc7_com:
    ldw     r4,#dwellsel
    ldw     r5,(r0,r4)      ; dwellsel into R5
    stw     r0,(r0,r4)      ; clear dwellsel

; set dwell timers for overdwell
; dwl[dwellsel] = maxdwl ;was 1
    ldw     r4,#maxdwl
    ldb     r3,(r0,r4)
od1:
    bitl    r5,#1
    beq     od2
    ldw     r4,#dwl
    stb     r3,(r0,r4)

od2:
    bitl    r5,#2
    beq     od3
    ldw     r4,#dwl+1
    stb     r3,(r0,r4)

od3:
    bitl    r5,#4
    beq     od4
    ldw     r4,#dwl+2
    stb     r3,(r0,r4)

od4:
    bitl    r5,#8
    beq     od5
    ldw     r4,#dwl+3
    stb     r3,(r0,r4)

od5:
    bitl    r5,#0x10
    beq     od6
    ldw     r4,#dwl+4
    stb     r3,(r0,r4)

od6:
    bitl    r5,#0x20
    beq     od7
    ldw     r4,#dwl+5
    stb     r3,(r0,r4)

od7:
    bitl    r5,#0x40
    beq     od8
    ldw     r4,#dwl+6
    stb     r3,(r0,r4)

od8:
    bitl    r5,#0x80
    beq     od9
    ldw     r4,#dwl+7
    stb     r3,(r0,r4)

od9:
    bith    r5,#1
    beq     od10
    ldw     r4,#dwl+8
    stb     r3,(r0,r4)

od10:
    bith    r5,#2
    beq     od11
    ldw     r4,#dwl+9
    stb     r3,(r0,r4)

od11:
    bith    r5,#4
    beq     od12
    ldw     r4,#dwl+10
    stb     r3,(r0,r4)

od12:
    bith    r5,#8
    beq     od13
    ldw     r4,#dwl+11
    stb     r3,(r0,r4)

od13:
    bith    r5,#0x10
    beq     od14
    ldw     r4,#dwl+12
    stb     r3,(r0,r4)

od14:
    bith    r5,#0x20
    beq     od15
    ldw     r4,#dwl+13
    stb     r3,(r0,r4)

od15:
    bith    r5,#0x40
    beq     od16
    ldw     r4,#dwl+14
    stb     r3,(r0,r4)

od16:
    bith    r5,#0x80
    beq     od_done
    ldw     r4,#dwl+15
    stb     r3,(r0,r4)

od_done:
    
; random number table spark cut
    ldw     r4,#spkcut_thresh
    ldw     r6,(r0,r4)
    tst     r6
    beq     do_dwell_coil   ; no spark cut
    ldw     r4,#rand_ptr
    ldw     r3,(r0,r4)
    addl    r3,#1       ; rand_ptr++
    ldh     r3,#0       ; top byte always zero
    stw     r3,(r0,r4)
;    ldw     r2,#random_no - 0x4000 ; Data is in PPAGEe0. Appears in XGATE memory at 0x800
    ldw     r2,#random_no_ptr
    ldw     r2,(r0,r2)
    ldb     r2,(r2,r3)    ; r2 = random[rand_ptr]
    ldh     r2,#0
    cmp     r6,r2
    bhs     j_xgdn  ; above threshold, skip dwell
    
do_dwell_coil:
    ldw     r4,#flagbyte10
    ldb     r6,(r0,r4)      ; set R6 to flagbyte10 for ICIgnOption spark hi/lo

;check for DLI
    ldw     r4,#flagbyte11
    ldb     r2,(r0,r4)
    bitl    r2,#FLAGBYTE11_DLI4 | FLAGBYTE11_DLI6
    beq     ddc2 ; not DLI
    ldw     r5, #1  ; Force dwellsel to 1 to flip IGt

ddc2:
    ldw     r2,#ram4.hardware
    ldb     r7,(r0,r2)
    bitl    r7,#HARDWARE_MS3XSPK
    bne     dwell_ms3x  ; LEDs or JS10 for spark

;Use opposite spark code to do the dwell
dwell_ug:
    bitl    r6,#FLAGBYTE10_SPKHILO        ; inverted or non-inverted outputs
    bne     jxg_ughi
    bra     xg_uglo

jxg_ughi:
    bra     xg_ughi

dwell_ms3x:
    bitl    r6,#FLAGBYTE10_SPKHILO        ; inverted or non-inverted outputs
    bne     j_xghi
    bra     xglo

j_xghi: bra xghi
;rts is at xgdn above

j_xgdn: bra xgdn
j_ck_dli:    bra     ck_dli
;*********************************************************************
; DWELL_COIL
;*********************************************************************
xgss_xgate1:
    ssem    #0
    bcc     xgss_xgate1

    ldw     r2, #XGSWTM
    ldw     r1,#0x0200      ; opposite of other flags, writing a 0 clears it.
    stw     r1,(r0,r2)      ; clear int flag

    bra     xgtc7_com

;*********************************************************************
; FIRE_COIL_ROTARY
;*********************************************************************
xgss_xgate2:
    ssem    #0
    bcc     xgss_xgate2

    ldw     r2, #XGSWTM
    ldw     r1,#0x0400      ; opposite of other flags, writing a 0 clears it.
    stw     r1,(r0,r2)      ; clear int flag
    bra     xgtc3_com

;*********************************************************************
; Rotary spark
;*********************************************************************
xgss_tc3:
    ssem    #0
    bcc     xgss_tc3

    ldw     r2, #TFLG1      ; load interrupt flag address
    ldl     r3, #0x08
    stb     r3,(r0,r2)      ; clear interrupt flag

    ldw     r2, #TIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xf7        ; clear bit 0x08
    stb     r3,(r0,r2)      ; clear interrupt enable

xgtc3_com:
    ldw     r4,#flagbyte10
    ldb     r6,(r0,r4)      ; set R6 to flagbyte10 for ICIgnOption spark hi/lo

    ldw     r4,#rotaryspksel
    ldb     r5,(r0,r4)      ; set R5 to rotaryspksel
    stb     r0,(r0,r4)      ; clear rotaryspksel

;rotary overdwell
rcod1:
    bitl    r5,#1
    beq     rcod2
    ldw     r3,#rdwl
    stb     r0,(r0,r3)

rcod2:
    bitl    r5,#2
    beq     rcod3
    ldw     r3,#rdwl+1
    stb     r0,(r0,r3)

rcod3:
    bitl    r5,#4
    beq     rcod4
    ldw     r3,#rdwl+2
    stb     r0,(r0,r3)

rcod4:
    bitl    r5,#8
    beq     rcod5
    ldw     r3,#rdwl+3
    stb     r0,(r0,r3)

rcod5:
    bitl    r5,#0x10
    beq     rcod6
    ldw     r3,#rdwl+4
    stb     r0,(r0,r3)

rcod6:
    bitl    r5,#0x20
    beq     rcod7
    ldw     r3,#rdwl+5
    stb     r0,(r0,r3)

rcod7:
    bitl    r5,#0x40
    beq     rcod8
    ldw     r3,#rdwl+6
    stb     r0,(r0,r3)

rcod8:
    bitl    r5,#0x80
    beq     rcod_done
    ldw     r3,#rdwl+7
    stb     r0,(r0,r3)

rcod_done:

    ldw     r4,#spkmode
    ldb     r3,(r0,r4)
    cmpl    r3,#14          ; twin trigger
    beq     j_tt2s
    
    ldw     r4, #ram4.RotarySplitMode
    ldb     r3,(r0,r4)      ; set R3 to RotarySplitMode

;LEDs or MS3X
    ldw     r2,#ram4.hardware
    ldb     r7,(r0,r2)
    bitl    r7,#HARDWARE_MS3XSPK
    bne     rotspk_ms3x

;*******************************
;Rotary spark on LEDs
;*******************************

    ldw     r4,#PTM

    bitl    r6,#FLAGBYTE10_SPKHILO
    bne     rotspkhi
;*******************************
;Rotary spark on LEDs, 5v = fire
;*******************************
;rotspklow:
    bitl    r3,#0x01
    bne     FDFire
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    beq     FDFire ; RX8 mode if 4 outputs

;FCFire:
    bitl    r5,#0x04
    beq     FCFireD
    ldb     r1,(r0,r4)
    andl    r1,#0xdf    ; PTM &= ~0x20
    stb     r1,(r0,r4)
    orl     r1,#0x10    ; PTM |= 0x10
    stb     r1,(r0,r4)

FCFireD:
    bitl    r5,#0x08
    beq     rot_spk_end
    ldb     r1,(r0,r4) 
    orl     r1,#0x20    ; PTM |= 0x20
    stb     r1,(r0,r4)
    orl     r1,#0x10    ; PTM |= 0x10
    stb     r1,(r0,r4)
    bra     rot_spk_end

FDFire:
    bitl    r5,#0x04
    beq     FDFireD
    ldb     r1,(r0,r4) 
    orl     r1,#0x10    ; PTM |= 0x10
    stb     r1,(r0,r4)

FDFireD:
    bitl    r5,#0x08
    beq     rot_spk_end
    ldb     r1,(r0,r4) 
    orl     r1,#0x20    ; PTM |= 0x20
    stb     r1,(r0,r4)
    bra     rot_spk_end

;*******************************
;Rotary spark on LEDs, 0v = fire
;*******************************

rotspkhi:
    bitl    r3,#0x01
    bne     FDFirehi
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    beq     FDFire ; RX8 mode if 4 outputs

;FCFire:
    bitl    r5,#0x04
    beq     FCFireDhi
    ldb     r1,(r0,r4)
    orl     r1,#0x20    ; PTM |= 0x20
    stb     r1,(r0,r4)
    andl    r1,#0xef    ; PTM &= ~0x10
    stb     r1,(r0,r4)

FCFireDhi:
    bitl    r5,#0x08
    beq     rot_spk_end
    ldb     r1,(r0,r4) 
    andl    r1,#0xdf    ; PTM &= ~0x20
    stb     r1,(r0,r4)
    andl    r1,#0xef    ; PTM &= ~0x10
    stb     r1,(r0,r4)
    bra     rot_spk_end

FDFirehi:
    bitl    r5,#0x04
    beq     FDFireDhi
    ldb     r1,(r0,r4) 
    andl    r1,#0xef    ; PTM &= ~0x10
    stb     r1,(r0,r4)

FDFireDhi:
    bitl    r5,#0x08
    beq     rot_spk_end
    ldb     r1,(r0,r4) 
    andl     r1,#0xdf    ; PTM &= ~0x20
    stb     r1,(r0,r4)
    bra     rot_spk_end

;*******************************
;Rotary spark on MS3X
;*******************************
rotspk_ms3x:
    ldw     r4,#PORTB

    bitl    r6,#FLAGBYTE10_SPKHILO
    bne     rotspkhi_ms3x
;*******************************
;Rotary spark on MS3X, 5v = fire
;*******************************
;rotspklow_ms3x:
    bitl    r3,#0x01
    bne     normFire_ms3x
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    bhs     normFire_ms3x

;FCFire_ms3x:
    bitl    r5,#0x04
    beq     FCFireD_ms3x
    ldb     r1,(r0,r4)
    andl    r1,#0xfb    ; clear spkC
    stb     r1,(r0,r4)
    orl     r1,#0x02    ; set spkB
    stb     r1,(r0,r4)

FCFireD_ms3x:
    bitl    r5,#0x08
    beq     rot_spk_end
    ldb     r1,(r0,r4) 
    orl     r1,#0x04    ; set spkC
    stb     r1,(r0,r4)
    orl     r1,#0x02    ; set spkB
    stb     r1,(r0,r4)
    bra     rot_spk_end

normFire_ms3x:
    ldb     r3,(r0,r4)    ; grab PORT data to R3
    or      r3,r3,r5      ; set bits as per coilsel
    stb     r3,(r0,r4)    ; store data to PORT
    bra     rot_spk_end

;*******************************
;Rotary spark on MS3X, 0v = fire
;*******************************

rotspkhi_ms3x:
    bitl    r3,#0x01
    bne     invFirehi_ms3x
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    bhs     invFirehi_ms3x

;FCFire_ms3x:
    bitl    r5,#0x04
    beq     FCFireDhi_ms3x
    ldb     r1,(r0,r4)
    orl     r1,#0x04    ; set spkC
    stb     r1,(r0,r4)
    andl    r1,#0xfd    ; clr spkB
    stb     r1,(r0,r4)

FCFireDhi_ms3x:
    bitl    r5,#0x08
    beq     rot_spk_end
    ldb     r1,(r0,r4) 
    andl    r1,#0xfb    ; clr spkC
    stb     r1,(r0,r4)
    andl    r1,#0xfd    ; clr spkB
    stb     r1,(r0,r4)
    bra     rot_spk_end

invFirehi_ms3x:
    xnorl   r5,#0  ; inverse, r5 is coilsel
    ldb     r3,(r0,r4)    ; grab PORT data to R3... r4 is PORTB
    and     r3,r3,r5      ; clear bits as per coilsel
    stb     r3,(r0,r4)    ; store data to PORT

rot_spk_end:
    csem    #0
    rts

j_tt2s: bra tt2s
;*********************************************************************
; DWELL_COIL_ROTARY
;*********************************************************************
xgss_xgate3:
    ssem    #0
    bcc     xgss_xgate3

    ldw     r2, #XGSWTM
    ldw     r1,#0x0800      ; opposite of other flags, writing a 0 clears it.
    stw     r1,(r0,r2)      ; clear int flag
    bra     xgtc6_com

;*********************************************************************
; Rotary dwell
;*********************************************************************
xgss_tc6:
    ssem    #0
    bcc     xgss_tc6

    ldw     r2, #TFLG1      ; load interrupt flag address
    ldl     r3, #0x40
    stb     r3,(r0,r2)      ; clear interrupt flag

    ldw     r2, #TIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xbf        ; clear bit 0x40
    stb     r3,(r0,r2)      ; clear interrupt enable

xgtc6_com:
    ldw     r4,#flagbyte10
    ldb     r6,(r0,r4)      ; set R6 to flagbyte10 for ICIgnOption spark hi/lo

    ldw     r4,#rotarydwlsel
    ldb     r5,(r0,r4)      ; set R5 to rotarydwlsel
    stb     r0,(r0,r4)      ; clear rotarydwlsel

;rotary overdwell
    ldw     r4,#maxdwl
    ldb     r3,(r0,r4)
;    ldl     r3,#1
rod1:
    bitl    r5,#1
    beq     rod2
    ldw     r4,#rdwl
    stb     r3,(r0,r4)

rod2:
    bitl    r5,#2
    beq     rod3
    ldw     r4,#rdwl+1
    stb     r3,(r0,r4)

rod3:
    bitl    r5,#4
    beq     rod4
    ldw     r4,#rdwl+2
    stb     r3,(r0,r4)

rod4:
    bitl    r5,#8
    beq     rod5
    ldw     r4,#rdwl+3
    stb     r3,(r0,r4)

rod5:
    bitl    r5,#0x10
    beq     rod6
    ldw     r4,#rdwl+4
    stb     r3,(r0,r4)

rod6:
    bitl    r5,#0x20
    beq     rod7
    ldw     r4,#rdwl+5
    stb     r3,(r0,r4)

rod7:
    bitl    r5,#0x40
    beq     rod8
    ldw     r4,#rdwl+6
    stb     r3,(r0,r4)

rod8:
    bitl    r5,#0x80
    beq     rod_done
    ldw     r4,#rdwl+7
    stb     r3,(r0,r4)

rod_done:

    ldw     r4,#spkmode
    ldb     r3,(r0,r4)
    cmpl    r3,#14
    beq     tt2d

    ; spkcut_thresh for trailing handled in ign_in

    ldw     r4, #ram4.RotarySplitMode
    ldb     r3,(r0,r4)      ; set R3 to RotarySplitMode

;LEDs or MS3X
    ldw     r2,#ram4.hardware
    ldb     r7,(r0,r2)
    bitl    r7,#HARDWARE_MS3XSPK
    bne     rotdwl_ms3x

;*******************************
;Rotary dwell on LEDs
;*******************************
    ldw     r4,#PTM

    bitl    r6,#FLAGBYTE10_SPKHILO
    bne     rotdwllo
;rotdwlhi:
    bitl    r3,#0x01
    bne     FDFirehi
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    beq     FDFirehi

;FCDwl:
    ldb     r1,(r0,r4)
    andl    r1,#0xef    ;   bclr	   PTM, #0x10
    stb     r1,(r0,r4)
    bra     rot_timerout_end

rotdwllo:
    bitl    r3,#0x01
    bne     FDFire
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    beq     FDFire

;FCDwllo:
    ldb     r1,(r0,r4)
    orl     r1,#0x10    ;   bset	   PTM, #0x10
    stb     r1,(r0,r4)
    bra     rot_timerout_end

;*******************************
;Rotary dwell on MS3X
;*******************************
rotdwl_ms3x:
    ldw     r4,#PORTB

    bitl    r6,#FLAGBYTE10_SPKHILO
    bne     rotdwllo_ms3x
;rotdwlhi_ms3x:
    bitl    r3,#0x01
    bne     invFirehi_ms3x
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    bhs     invFirehi_ms3x


;FCDwl:
    ldb     r1,(r0,r4)
    andl    r1,#0xfd    ;   clr	spkB
    stb     r1,(r0,r4)
    bra     rot_timerout_end

rotdwllo_ms3x:
    bitl    r3,#0x01
    bne     normFire_ms3x
    ldw     r1,#num_spk
    ldb     r3,(r0,r1)
    cmpl    r3,#4
    bhs     normFire_ms3x

;FCDwllo:
    ldb     r1,(r0,r4)
    orl     r1,#0x02    ;   set	spkB
    stb     r1,(r0,r4)
;    bra     rot_timerout_end

rot_timerout_end:
    csem    #0
    rts
;*********************************************************************
; Twin trigger channel 2
;*********************************************************************
tt2s:
    cmpl    r5,#0               ; rotaryspksel = 0
    beq     rot_timerout_end    ; exit

; see if we need to schedule the dwell from here too
    ldw     r4,#dwell_us2
    ldw     r2,(r0,r4)          ; dwell in tics
    cmp     r2,#0
    beq     tt2s_choice         ; zero so skip

; Distributor dwell timed off back of spark or
; time after spark. If we are running this wacky mode, then set up 'dwell' right now
    ldw     r4,#rotarydwlsel     ; set R4 to &coilsel
    ldl     r1,#2
    stb     r1,(r0,r4)          ; rotdwlsel = 2

    ldw     r4,#TC3
    ldw     r1,(r0,r4)          ; timer value
    add     r1,r1,r2
    ldw     r4,#TC6
    stw     r1,(r0,r4)
    ldw     r4,#TIE
    ldb     r1,(r0,r4)
    orl     r1,#0x40            ; bset    TIE,#0x40
    stb     r1,(r0,r4)
    ldw     r4,#TFLG1
    ldl     r1,#0x40
    stb     r1,(r0,r4)          ; movb    #0x40,TFLG1

tt2s_choice:
    ldh     r5, #0
    bra     j_ck_dli

tt2d:
    cmpl    r5,#0               ; rotarydwlsel = 0
    beq     rot_timerout_end    ; exit

; random number table spark cut
    ldw     r4,#spkcut_thresh
    ldw     r6,(r0,r4)
    tst     r6
    beq     tt2d_choice     ; no spark cut
    ldw     r4,#rand_ptr
    ldw     r3,(r0,r4)
    addl    r3,#1       ; rand_ptr++
    ldh     r3,#0       ; top byte always zero
    stw     r3,(r0,r4)
;    ldw     r2,#random_no - 0x4000; Data is in PPAGEe0. Appears in XGATE memory at 0x800
    ldw     r2,#random_no_ptr
    ldw     r2,(r0,r2)
    ldb     r2,(r2,r3)    ; r2 = random[rand_ptr]
    ldh     r2,#0
    cmp     r6,r2
    bhs     rot_timerout_end ; above threshold, skip dwell

tt2d_choice:
    bra     do_dwell_coil

;*********************************************************************
; Injector outputs
;*********************************************************************

xgss_inj1:
    ; get semaphore
    ssem    #0
    bcc     xgss_inj1    

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x1
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xFE           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x01
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#0
    bra     inj_sel
    
xgss_inj2:
; get semaphore
    ssem    #0
    bcc     xgss_inj2

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x2
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xFD           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x02
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#1
    bra     inj_sel

xgss_inj3:
    ; get semaphore
    ssem    #0
    bcc     xgss_inj3

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x4
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xFB           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x04
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#2
    bra     inj_sel

xgss_inj4:
    ; get semaphore
    ssem    #0
    bcc     xgss_inj4

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x8
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xF7           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x08
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#3
    bra     inj_sel

xgss_inj5:
    ; get semaphore
    ssem    #0
    bcc     xgss_inj5

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x10
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xEF           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x10
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#4
    bra     inj_sel

xgss_inj6:
    ; get semaphore
    ssem    #0
    bcc     xgss_inj6

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x20
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xDF           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x20
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#5
    bra     inj_sel

xgss_inj7:
    ; get semaphore
    ssem    #0
    bcc     xgss_inj7

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x40
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0xBF           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x40
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#6
    bra     inj_sel

xgss_inj8:
    ; get semaphore
    ssem    #0
    bcc     xgss_inj8

    ; Clear timer interrupt flag
    ldw     r2,#TIMTFLG1
    ldl     r3,#0x80
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIMTIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    andl    r3,#0x7F           ; and turn it off
    stb     r3,(r0,r2)

    ldw     r2,#timer_usage
    ldb     r3,(r0,r2)
    orl     r3, #0x80
    stb     r3,(r0,r2) ; timer bit marked free

    ldl     r3,#7
    bra     inj_sel

;--------------------
inj_sel:
    ldh     r3,#0
    ldw     r4,#inj_chan ; array
    ldh     r5,#0
    ldb     r5,(r3,r4)      ; load from downcounter / channel array
    ldw     r4,#inj_status
    stb     r0,(r5,r4)   ; mark injector as off
    lsl     r5,#1
    addl    r5,#2   ; add on two to skip the 'jal'
    tfr     r1,pc
    add     r1,r1,r5        ; get new PC + offset
; PC points *HERE*
    jal     r1              ; jump into jump table
; +2
    bra     inj1_off
    bra     inj2_off
    bra     inj3_off
    bra     inj4_off
    bra     inj5_off
    bra     inj6_off
    bra     inj7_off
    bra     inj8_off
    bra     inj9_off
    bra     inj10_off
    bra     inj11_off
    bra     inj12_off
    bra     inj13_off
    bra     inj14_off
    bra     inj15_off
    bra     inj16_off

inj1_off:
    ; toggle the pin off
    ldw     r2,#next_inj_0 ; r2 = &inj_event
    bra     inj_off_com

inj2_off:
    ; toggle the pin off
    ldw     r2,#next_inj_1 ; r2 = &inj_event
    bra     inj_off_com

inj3_off:
    ; toggle the pin off
    ldw     r2,#next_inj_2 ; r2 = &inj_event
    bra     inj_off_com

inj4_off:
    ; toggle the pin off
    ldw     r2,#next_inj_3 ; r2 = &inj_event
    bra     inj_off_com

inj5_off:
    ; toggle the pin off
    ldw     r2,#next_inj_4 ; r2 = &inj_event
    bra     inj_off_com

inj6_off:
    ; toggle the pin off
    ldw     r2,#next_inj_5 ; r2 = &inj_event
    bra     inj_off_com

inj7_off:
    ; toggle the pin off
    ldw     r2,#next_inj_6 ; r2 = &inj_event
    bra     inj_off_com

inj8_off:
    ; toggle the pin off
    ldw     r2,#next_inj_7 ; r2 = &inj_event
    bra     inj_off_com

inj9_off:
    ldw     r2,#PWME
    ldb     r3,(r0,r2)
    andl    r3,#0xFE
    stb     r3,(r0,r2)

    ; toggle the pin off
    ldw     r2,#next_inj_8 ; r2 = &inj_event
    bra     inj_off_com
    
inj10_off:
    ldw     r2,#PWME
    ldb     r3,(r0,r2)
    andl    r3,#0xFD
    stb     r3,(r0,r2)

    ; toggle the pin off
    ldw     r2,#next_inj_9 ; r2 = &inj_event
    bra     inj_off_com

inj11_off:
    ; toggle the pin off
    ldw     r2,#next_inj_10 ; r2 = &inj_event
    bra     inj_off_com

inj12_off:
    ; toggle the pin off
    ldw     r2,#next_inj_11 ; r2 = &inj_event
    bra     inj_off_com

inj13_off:
    ldw     r2,#next_inj_12 ; r2 = &inj_event
    bra     inj_off_com

inj14_off:
    ldw     r2,#next_inj_13 ; r2 = &inj_event
    bra     inj_off_com

inj15_off:
    ldw     r2,#next_inj_14 ; r2 = &inj_event
    bra     inj_off_com

inj16_off:
    ldw     r2,#next_inj_15 ; r2 = &inj_event
;    bra     inj_off_com

inj_off_com:
    ldw     r3,(r2,#2) ; r3 = inj_event.port (which is &of the port reg)
    ldb     r4,(r0,r3) ; r4 = *inj_event.port
    ldb     r6,(r2,#4)     ; r6 = inj_event.pin
    xnorl   r6,#0          ; invert the pin selection
    and     r4,r4,r6       ; r4 |= r6
    stb     r4,(r0,r3)     ; port = r4
    
inj_off_done:
    csem    #0
    rts

; **********************************************************

; Cam (hi-res) input for CAS360 modes
; on TC4 (i.e. PT4, not normal TC2 MS3X cam input)
cam_tc4:
    ssem    #0              ; attempt to set semaphore 0
    bcc     cam_tc4

    ldw     r2, #TFLG1      ; load interrupt flag address
    ldl     r3, #0x10
    stb     r3,(r0,r2)      ; clear interrupt flag

    ldw     r2, #TIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    orl     r3,#0x10        ; set bit 0x10
    stb     r3,(r0,r2)      ; ensure interrupt still enabled

    bra     xg_cas360

; on TC5
cam_tc5:
    ssem    #0              ; attempt to set semaphore 0
    bcc     cam_tc5

    ldw     r2, #TFLG1      ; load interrupt flag address
    ldl     r3, #0x20
    stb     r3,(r0,r2)      ; clear interrupt flag
    ldw     r2, #TIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    orl     r3,#0x20        ; set bit 0x20
    stb     r3,(r0,r2)      ; ensure interrupt still enabled

xg_cas360:
; are we doing composite log?
    ldw     r2, #flagbyte0
    ldb     r3,(r0,r2)
    bitl    r3,#FLAGBYTE0_COMPLOG
    beq     xgc3nolog
; comp log
    ldw     r2,#TCNT
    ldw     r3,(r0,r2)
    ldw     r4,#IC_last
    ldw     r5,(r4,#2)
    sub     r5,r3,r5    ; calc delta time
    stw     r3,(r4,#2) ; store TCNT in low word
    stw     r0,(r4,#0)  ; clear top word
    ldw     r4,#xgcamTC
    stw     r5,(r0,r4)  ; store delta time
    ;fire off an S12X interrupt
    ldw     r3, #0x4040 ; XGATE6 interrupt -> xg_camlogger
    ldw     r2, #XGSWTM
    stw     r3,(r0,r2)

xgc3nolog:
    ; automatic noise filter, <20us is noise
    ldw     r2,#TCNT
    ldw     r3,(r0,r2)
    ldw     r4,#xgcam_last
    ldw     r5,(r0,r4)
    sub     r5,r3,r5    ; calc delta time
    cmp     r5,#20
    bhi     xgc3_ok
    bra     xg_cas360_exit    ; must be noise, so bail

xgc3_ok:
    stw     r3,(r0,r4) ; store TCNT to last

xg_cas360_countonly:
;rolling tooth no. counter 0-359
    ldw     r4, #cas_tooth
    ldw     r5,(r0,r4)
    add     r5,#1

    ldw     r2, #xg_teeth
    ldw     r7,(r0,r2)

    cmp    r5,r7
    bcs     xg_cas360t
    ldw     r5,#0
xg_cas360t:
    stw     r5,(r0,r4)

    ldw     r2, #spkmode
    ldb     r3, (r0,r2)
    cmpl    r3, #47
    beq     fly_hires

;Cranking tooth down counter (Nissan CAS + Optispark)
; This is used to fire the coil based on hi-res teeth during cranking
; count to give more stable spark when engine speed is unstable.
; Out of crank mode, will be zero and skipped.

;xgdwlq[0]
    ldw     r4, #xgdwellq+2
    ldw     r5,(r0,r4) ; .cnt
    tst     r5
    beq     dc2             ; already zero, next test
    sub     r5,#1
    stw     r5,(r0,r4)
    bne     dc2             ; not yet zero, next test
    ldw     r4, #xgdwellq
    ldw     r5,(r0,r4) ; .sel
    stw     r0,(r0,r4)
    bra     dc_dc

dc2:
;xgdwlq[1]
    ldw     r4, #xgdwellq+6
    ldw     r5,(r0,r4) ; .cnt
    tst     r5
    beq     dc3             ; already zero, next test
    sub     r5,#1
    stw     r5,(r0,r4)
    bne     dc3             ; not yet zero, next test
    ldw     r4, #xgdwellq+4
    ldw     r5,(r0,r4) ; .sel
    stw     r0,(r0,r4)
    bra     dc_dc

dc3:
;xgspkq[0]
    ldw     r4, #xgspkq+2
    ldw     r5,(r0,r4) ; .cnt
    tst     r5
    beq     dc4             ; already zero, next test
    sub     r5,#1
    stw     r5,(r0,r4)
    bne     dc4             ; not yet zero, next test
    ldw     r4, #xgspkq
    ldw     r5,(r0,r4) ; .sel
    stw     r0,(r0,r4)
    bra     dc_fc

dc4:
;xgspkq[0]
    ldw     r4, #xgspkq+6
    ldw     r5,(r0,r4) ; .cnt
    tst     r5
    beq     xg_cas360_exit  ; already zero, exit
    sub     r5,#1
    stw     r5,(r0,r4)
    bne     xg_cas360_exit  ; not yet zero, exit
    ldw     r4, #xgspkq+4
    ldw     r5,(r0,r4) ; .sel
    stw     r0,(r0,r4)
    bra     dc_fc

dc_fc:
    ldw     r4, #coilsel
    stw     r5,(r0,r4)
    ldw     r4,#0x0101  ; XGATE0 interrupt -> FIRE COIL (once we exit)
    ldw     r5,#XGSWTM
    stw     r4,(r0,r5)  ; cause S12X interrupt, which may then call ign_reset if needed
    bra     xg_cas360_exit

dc_dc:
    ldw     r4, #dwellsel
    stw     r5,(r0,r4)
    ldw     r4, #coilsel ; for fallback
    ldw     r3,(r0,r4)
    or      r5,r3,r5
    stw     r5,(r0,r4)  ; store ORed coilsel (in case one already active)
    ldw     r4,#0x0202  ; XGATE1 interrupt -> DWELL COIL (once we exit)
    ldw     r5,#XGSWTM
    stw     r4,(r0,r5)  ; cause S12X interrupt, which may then call ign_reset if needed

;set spark timer as a fallback
    ld		r4, #dwell_us
    ldw     r5,(r0,r4)
    ld		r4, #TCNT   ; calc timer target
    ldw     r3,(r0,r4)
    add		r3,r3,r5
    ld		r4, #TC1    ; set timer
    stw		r3,(r0,r4)

    ld		r4, #TFLG1  ; clear flag
    ldl		r3,#2
    stb		r3,(r0,r4)

    ld		r4, #TIE    ; enable int
    ldb		r3,(r0,r4)
    orl		r3,#2
    stb		r3,(r0,r4)

    bra     xg_cas360_exit

fly_hires:
;flywheel divider mode
;0,1,2 divider
    ldw     r4, #cas_tooth_div
    ldw     r5,(r0,r4) ; this could likely be U08 not U16
    add     r5,#1

    ldw     r2, #fly_div
    ldb     r7,(r0,r2)
    ldh     r7,#0

    cmp     r5,r7
    bcs     xg_cas360_store ; (blo) teeth 1,2 fall through
    andl    r3,#1
    beq     xg_cas360_store ; if not synced just keep counting

    ;Xth tooth fire off an S12X interrupt
    ldw     r3, #0x1010 ; XGATE4 interrupt -> exec_timerin
    ldw     r2, #XGSWTM
    stw     r3,(r0,r2)
    ldw     r5,#0
xg_cas360_store:
    stw     r5,(r0,r4)

xg_cas360_exit:
    csem    #0
    rts

; crank (low-res) input

crank_tc0:
    ssem    #0              ; attempt to set semaphore 0
    bcc     crank_tc0
    ldw     r2, #TFLG1      ; load interrupt flag address
    ldl     r3, #0x01
    stb     r3,(r0,r2)      ; clear interrupt flag

    ldw     r2, #TIE        ; load interrupt enable address
    ldb     r3, (r0,r2)
    orl     r3,#0x01        ; set bit 0x01
    stb     r3,(r0,r2)      ; ensure interrupt still enabled

; are we doing composite log?
    ldw     r2, #flagbyte0
    ldb     r3,(r0,r2)
    bitl    r3,#FLAGBYTE0_COMPLOG
    beq     xgcrk
; comp log
    ldw     r2,#TCNT
    ldw     r3,(r0,r2)
    ldw     r4,#IC_last
    ldw     r5,(r4,#2)
    sub     r5,r3,r5    ; calc delta time
    stw     r3,(r4,#2) ; store TCNT in low word
    stw     r0,(r4,#0)  ; clear top word
    ldw     r4,#xgcrkTC
    stw     r5,(r0,r4)  ; store delta time
    ;fire off an S12X interrupt
    ldw     r3, #0x2020 ; XGATE5 interrupt -> xg_crank_logger
    ldw     r2, #XGSWTM
    stw     r3,(r0,r2)

xgcrk:
    ldw     r2, #spkmode
    ldb     r3, (r0,r2)
    cmpl    r3, #47
    bne     iscas_crk
    bra     fly_pin

iscas_crk:
;edge detect on crank input
    ldl     r4,#0 ; edge * was 1
    ldw     r2, #ram4.ICIgnOption
    ldb     r3, (r0,r2) ; r3 contains ICIgnOption
    ldw     r5, #PTT
    ldb     r6, (r0,r5) ; r6 contains PTT

    bitl    r3,#1
    beq     ct0_0
    bitl    r6,#1
    beq     ct0_setzero
    bra     ct0_done
ct0_0:
    bitl    r6,#1
    beq     ct0_done
ct0_setzero:
    ldl     r4,#1 ;  was 0
ct0_done:

    ldw     r2, #synch
    ldb     r3, (r0,r2)
    bitl    r3,#1
    bne     ct0_synced  ; go check re-sync

;keep stall timer happy ; ltch_lmms = lmms
    ldw     r5, #lmms
    ldw     r7, #ltch_lmms
xgss_1:
    ssem    #1              ; attempt to set semaphore 1, small lock in rtc.s
    bcc     xgss_1
    ldw     r6,(r0,r5) 
    stw     r6,(r0,r7) 
    ldw     r5, #lmms+2
    ldw     r7, #ltch_lmms+2
    ldw     r6,(r0,r5) 
    stw     r6,(r0,r7) 
    csem    #1

    ldw     r5, #cas_tooth
    ldw     r6,(r0,r5)

    cmpl    r4,#1       ; check edge
    beq     ct0_cksemi

;start of slot
    cmp     r6,#5
    bcs     jct0_x   ; blo  - not really turning yet
    stw     r0,(r0,r5) ; clear it
    orl     r3,#0x08   ; set SYNC_SEMI
    stb     r3, (r0,r2)
    bra     ct0_x
    
; end of slot
ct0_cksemi:
    bitl    r3,#0x08    ; check for semi
    beq     jct0_x       ; exit
    ldw     r4,#spkmode
    ldb     r1, (r0,r4)
    cmpl    r1,#32
    beq     xg_cs32
    cmpl    r1,#33
    beq     xg_cs33
    cmpl    r1,#34
    beq     xg_cs34
    cmpl    r1,#35
    beq     xg_cs35
; etc.
jct0_x:
    bra     ct0_x

;*********** optispark sync *************
; Equidistant start of slot
; was check for 7,12,17,22
xg_cs32:
;revised method, this is END of slot, so don't send tach signal to S12 here
; only send tach signal on start of slot for Optispark
;7,12,17,22
;on entry r6 is no. hi-res teeth in this low-res window
;on exit r6 is S12 tooth no.

    cmp     r6,#6
    bcs     xg_cs32_not6 ; blo
    cmp     r6,#8
    bhi     xg_cs32_not6
    ldl     r6,#2
    bra     xg_com_sync

xg_cs32_not6:
    cmp     r6,#11
    bcs     xg_cs32_not11
    cmp     r6,#13
    bhi     xg_cs32_not11
    ldl     r6,#4
    bra     xg_com_sync

xg_cs32_not11:
    cmp     r6,#16
    bcs     xg_cs32_not16
    cmp     r6,#18
    bhi     xg_cs32_not16
    ldl     r6,#6
    bra     xg_com_sync

xg_cs32_not16:
    cmp     r6,#21
    bcs     xg_cs32_logfail   ; likely a short indeterminate one, try another
    cmp     r6,#23
    bhi     xg_cs32_logfail   ; likely a short indeterminate one, try another
    ldl     r6,#0
    bra     xg_com_sync

xg_cs32_logfail:
;do we know it is synced?
    andl    r3,#0xf7   ; clear SYNC_SEMI 0x08
    stb     r3, (r0,r2)
;    ldw     r2,#outpc.status4    ;status4 = tooth count we didn't like
;    stb     r6,(r0,r2)
    bra     ct0_x

;*********** 16,12,8,4 *************
; equidistant start of slot SR20
xg_cs33:
    cmp     r6,#15
    bcs     xg_cs33_not16
    cmp     r6,#17
    bhi     xg_cs33_not16
    ldl     r6,#0
    bra     xg_com_sync

xg_cs33_not16:
    cmp     r6,#11
    bcs     xg_cs33_not12
    cmp     r6,#13
    bhi     xg_cs33_not12
    ldl     r6,#1
    bra     xg_com_sync

xg_cs33_not12:
    cmp     r6,#7
    bcs     xg_cs33_not8
    cmp     r6,#9
    bhi     xg_cs33_not8
    ldl     r6,#2
    bra     xg_com_sync

xg_cs33_not8:
    cmp     r6,#3
    bcs     xg_cs33_logfail
    cmp     r6,#5
    bhi     xg_cs33_logfail
    ldl     r6,#3
    bra     xg_com_sync

xg_cs33_logfail:
;do we know it is synced?
    andl    r3,#0xf7   ; clear SYNC_SEMI 0x08
    stb     r3, (r0,r2)
;    ldw     r2,#outpc.status4    ;status4 = tooth count we didn't like
;    stb     r6,(r0,r2)
    bra     ct0_x

;*********** RB25 24,20,16,12,8,4 *************
; equidistant start of slot
xg_cs34:
    cmp     r6,#23
    bcs     xg_cs34_not24
    cmp     r6,#25
    bhi     xg_cs34_not24
    ldl     r6,#0
    bra     xg_com_sync

xg_cs34_not24:
    cmp     r6,#19
    bcs     xg_cs34_not20
    cmp     r6,#21
    bhi     xg_cs34_not20
    ldl     r6,#1
    bra     xg_com_sync

xg_cs34_not20:
    cmp     r6,#15
    bcs     xg_cs34_not16
    cmp     r6,#17
    bhi     xg_cs34_not16
    ldl     r6,#2
    bra     xg_com_sync

xg_cs34_not16:
    cmp     r6,#11
    bcs     xg_cs34_not12
    cmp     r6,#13
    bhi     xg_cs34_not12
    ldl     r6,#3
    bra     xg_com_sync

xg_cs34_not12:
    cmp     r6,#7
    bcs     xg_cs34_not8
    cmp     r6,#9
    bhi     xg_cs34_not8
    ldl     r6,#4
    bra     xg_com_sync

xg_cs34_not8:
    cmp     r6,#3
    bcs     xg_cs34_logfail
    cmp     r6,#5
    bhi     xg_cs34_logfail
    ldl     r6,#5
    bra     xg_com_sync

xg_cs34_logfail:
;do we know it is synced?
    andl    r3,#0xf7   ; clear SYNC_SEMI 0x08
    stb     r3, (r0,r2)
;    ldw     r2,#outpc.status4    ;status4 = tooth count we didn't like
;    stb     r6,(r0,r2)
    bra     ct0_x


xg_cs35:
    bra     ct0_x
; --------- common sync -------------
xg_com_sync:
    ldw     r5,#tooth_no ; store S12 tooth no. (r6)
    stb     r6,(r0,r5)

;keep stall timer happy ; ltch_lmms = lmms
    ldw     r5, #lmms
    ldw     r7, #ltch_lmms
xgss_1a:
    ssem    #1              ; attempt to set semaphore 1, small lock in rtc.s
    bcc     xgss_1a
    ldw     r6,(r0,r5) 
    stw     r6,(r0,r7) 
    ldw     r5, #lmms+2
    ldw     r7, #ltch_lmms+2
    ldw     r6,(r0,r5) 
    stw     r6,(r0,r7) 
    csem    #1

;say we are synced
    ldw     r2, #synch
    ldl     r3,#0x03    ;SYNC_SYNCED | SYNC_FIRST
    stb     r3,(r0,r2)

    bra     ct0_x

;-------------

ct0_synced:
    ;re-sync code

    ldw     r5,#cas_tooth
    ldw     r6,(r0,r5)          ;cas_tooth in r6

    ldw     r5,#tooth_no
    ldb     r7,(r0,r5)          ;s12 tooth in r7

    ldw     r5,#spkmode
    ldb     r1, (r0,r5)
    cmpl    r1,#32
    beq     xg_rs32
    cmpl    r1,#33
    beq     xg_rs33
    cmpl    r1,#34
    beq     xg_rs34
    cmpl    r1,#35
    beq     xg_rs35

xg_rs32:
    cmpl    r4,#0       ; check edge. Start/0 = tach. End/1 = check
    bne     xgrs32ck
    ldw     r5,#cas_tooth
    stw     r0,(r0,r5)          ;reset cas_tooth to count size of slot
    bra     call_s12x           ;active edge, call S12

xgrs32ck:
;check on end

    cmpl    r7,#1
    beq     xgrs32_1
    cmpl    r7,#3
    beq     xgrs32_3
    cmpl    r7,#5
    beq     xgrs32_5
    cmpl    r7,#7
    beq     xgrs32_7
    bra     ct0_x

; allow +/- tooth tolerance
xgrs32_1:
    cmpl    r6,#6
    bcs     lose_sync
    cmpl    r6,#8
    bhi     lose_sync
    bra     ct0_x

xgrs32_3:
    cmpl    r6,#11
    bcs     lose_sync
    cmpl    r6,#13
    bhi     lose_sync
    bra     ct0_x

xgrs32_5:
    cmpl    r6,#16
    bcs     lose_sync
    cmpl    r6,#18
    bhi     lose_sync
    bra     ct0_x

xgrs32_7:
    cmpl    r6,#21
    bcs     lose_sync
    cmpl    r6,#23
    bhi     lose_sync
    bra     ct0_x

xg_rs33:
    cmpl    r4,#0       ; check edge. Start/0 = tach. End/1 = check
    bne     xgrs33ck
    ldw     r5,#cas_tooth
    stw     r0,(r0,r5)          ;reset cas_tooth to count size of slot
    bra     call_s12x           ;active edge, call S12

xgrs33ck:
;check on end
    cmpl    r7,#3
    beq     xgrs33_1
    cmpl    r7,#4
    beq     xgrs33_2
    cmpl    r7,#1
    beq     xgrs33_3
    cmpl    r7,#2
    beq     xgrs33_4
    bra     ct0_x

; allow +/- tooth tolerance
xgrs33_1:
    cmpl    r6,#15
    bcs     lose_sync
    cmpl    r6,#17
    bhi     lose_sync
    bra     ct0_x

xgrs33_2:
    cmpl    r6,#11
    bcs     lose_sync
    cmpl    r6,#13
    bhi     lose_sync
    bra     ct0_x

xgrs33_3:
    cmpl    r6,#7
    bcs     lose_sync
    cmpl    r6,#9
    bhi     lose_sync
    bra     ct0_x

xgrs33_4:
    cmpl    r6,#3
    bcs     lose_sync
    cmpl    r6,#5
    bhi     lose_sync
    bra     ct0_x

xg_rs34:
    cmpl    r4,#0       ; check edge. Start/0 = tach. End/1 = check
    bne     xgrs34ck
    ldw     r5,#cas_tooth
    stw     r0,(r0,r5)          ;reset cas_tooth to count size of slot
    bra     call_s12x           ;active edge, call S12

xgrs34ck:
;check on end, only half of teeth
    cmpl    r7,#1
    beq     xgrs34_3
    cmpl    r7,#3
    beq     xgrs34_5
    cmpl    r7,#5
    beq     xgrs34_1

    bra     ct0_x

; allow +/- tooth tolerance
xgrs34_1:
    cmpl    r6,#23
    bcs     lose_sync
    cmpl    r6,#25
    bhi     lose_sync
    bra     ct0_x

xgrs34_3:
    cmpl    r6,#15
    bcs     lose_sync
    cmpl    r6,#17
    bhi     lose_sync
    bra     ct0_x

xgrs34_5:
    cmpl    r6,#7
    bcs     lose_sync
    cmpl    r6,#9
    bhi     lose_sync
    bra     ct0_x

xg_rs35:
    bra     ct0_x

lose_sync:
    ldw     r2, #synch
    stb     r0,(r0,r2)  ; clear synch
    ldw     r2,#xg_debug    ;store the offending tooth
    stw     r6,(r0,r2)

call_s12x:
    ldw     r2,#0x1010  ; XGATE4 interrupt -> exec_timerin
    ldw     r3,#XGSWTM
    stw     r2,(r0,r3)  ; cause S12X interrupt, which may then call ign_reset if needed

ct0_x:
    csem    #0
    rts

; ------- flywheel tri-tach input ------------
; flywheel handled along with CAS360

; this is the reset pin and poll cam
fly_pin:
.if 0
;debug - save a copy of the crank tooth no. at the reset pin
    ldw     r1, #tooth_no
    ldb     r3, (r0,r1)

    ldw     r1, #cas_tooth
    ldw     r3, (r0,r1)
    ldw     r1, #outpc.status4
    stb     r3, (r0,r1)

; end debug
.endif
    ldw     r2, #synch
    ldb     r3, (r0,r2)
    bitl    r3,#1
    bne     fly_resync  ; go check re-sync

; need to poll the cam and set phase accordingly
; if cam high then r6 = num_teeth >> 1
    ldw     r1, #PORTT
    ldb     r4,(r0,r1)
    ldw     r1, #pin_xgcam
    ldb     r1,(r0,r1)
    ldh     r1,#0
    ldh     r4,#0
    and     r0,r4,r1
    beq     fly_camlow
;cam high, second rev
    ldw     r1, #no_teeth
    ldb     r6,(r0,r1)
    ldh     r6,#0
    lsr     r6,#1   ; >> 1
    bra     fly_settooth

fly_camlow:
; cam low, first rev
    ldw     r6,#0

fly_settooth:
    ldw     r4,#0
    ldw     r7,#0
    bra     xg_com_sync

fly_resync:

;    check that (cas_tooth +1) here is zero
    ldw     r5,#0  ; compare to zero
    ldw     r3, #xg_teeth
    ldb     r1, (r0,r3)
    ldh     r1, #0
    cmp     r1, r0
    bne     lose_sync

; Validate S12 tooth no.
    ldw     r1, #tooth_no
    ldb     r3, (r0,r1)
    ldh     r3, #0

    ldw     r1, #no_teeth
    ldb     r4, (r0,r1)
    ldh     r4, #0

; grab engine crank status
    ldw     r7, #outpc.engine
    ldb     r7,(r0,r7)
    andl    r7,#2
    ; r7 is 0 for not cranking
    beq     fly_ck_ph1  ; no point in calculating pin again if not going to check it
    ldw     r1, #PORTT
    ldb     r6,(r0,r1)
    ldw     r1, #pin_xgcam
    ldb     r1,(r0,r1)
    ldh     r1,#0
    ldh     r6,#0
    and     r6,r6,r1
    ; here r6 is cam pin level

fly_ck_ph1:
; tooth no. = no_teeth ?
    cmp     r3,r4
    bne     fly_ck_ph2  ; wasn't, so see if other phase

;check cam is correct phase
    cmpl    r7,#0
    beq     ct0_x       ; not cranking so no need to check

    cmpl    r6,#0
    bne     lose_sync   ; wanted a high cam
    bra     ct0_x       ; was ok
    
fly_ck_ph2:
; tooth no. = (no_teeth >> 1) ?
    lsr     r4, #1
    cmp     r3,r4
    bne     lose_sync

;check cam is correct phase
    cmpl    r7,#0
    beq     ct0_x       ; not cranking so no need to check

    cmpl    r6,#0
    beq     lose_sync   ; wanted a low cam
    bra     ct0_x       ; was ok

; ---------------------------------------------

; PIT2 - Injector ON
xgss_pit2:
    ssem    #0              ; attempt to set semaphore 0
    bcc     xgss_pit2

;clear PIT flag
    ldw     r1,#PITTF
    ldb     r2, (r0,r1)
    ldl     r2,#4
    stb     r2, (r0,r1)

; miata "bang bang" alt ctl.
    ldw	    r1, #flagbyte23
    ldb     r6, (r0,r1) ; leave flagbyte23 in R6
    bitl    r6,#FLAGBYTE23_MIATA_ALT
    beq     miata_skip    

    ldw	    r1, #ATD0DR4
    ldw	    r2, (r0,r1)

    ldw	    r3, #alt_min_adc
    ldw	    r4, (r0,r3)

    cmp     r4, r2
    blo	    miata_on

    ldw	    r3, #alt_max_adc
    ldw	    r4, (r0,r3)

    cmp     r4, r2
    bhi     miata_off
    bra     miata_skip

miata_on:
    bitl    r6,#FLAGBYTE23_MIATAALT_INV ; inverted?
    beq     toggle_miataout_off
toggle_miataout_on:
    ldw     r1,#port_alt_out
    ldw     r1,(r0,r1) ; get address
    ldb     r2,(r0,r1) ; get current port value
    ldw     r3,#pin_alt_out
    ldb     r4,(r0,r3)
    or      r2,r2,r4
    bra     miata_set

miata_off:
   bitl     r6,#FLAGBYTE23_MIATAALT_INV ; inverted?
   beq      toggle_miataout_on
toggle_miataout_off:
    ldw     r1,#port_alt_out
    ldw     r1,(r0,r1) ; get address
    ldb     r2,(r0,r1) ; get current port value
    ldw     r3,#pin_alt_out
    ldb     r4,(r0,r3)
    xnorl   r4,#0
    and     r2,r2,r4

miata_set:
    stb     r2,(r0,r1) ; save port value

miata_skip:

; down count on each injector and start when it reaches zero. Then schedule off event

;Check inj1
    ldw     r1,#inj1_cnt
    ldw     r2, (r1,r0)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj1_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r1,r0)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj1_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_0 ; r2 = &inj_event
    ldl     r1,#0

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj1_dc_end:

;Check inj2
    ldw     r1,#inj2_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj2_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj2_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_1 ; r2 = &inj_event
    ldl     r1,#1

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj2_dc_end:

;Check inj3
    ldw     r1,#inj3_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj3_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj3_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_2 ; r2 = &inj_event
    ldl     r1,#2

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj3_dc_end:

;Check inj4
    ldw     r1,#inj4_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj4_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj4_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_3 ; r2 = &inj_event
    ldl     r1,#3

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj4_dc_end:

;Check inj5
    ldw     r1,#inj5_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj5_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj5_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_4 ; r2 = &inj_event
    ldl     r1,#4

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj5_dc_end:

;Check inj6
    ldw     r1,#inj6_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj6_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj6_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_5 ; r2 = &inj_event
    ldl     r1,#5

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj6_dc_end:

;Check inj7
    ldw     r1,#inj7_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj7_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj7_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_6 ; r2 = &inj_event
    ldl     r1,#6

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj7_dc_end:

;Check inj8
    ldw     r1,#inj8_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj8_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj8_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_7 ; r2 = &inj_event
    ldl     r1,#7

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj8_dc_end:
    ldw     r1,#inj9_cnt
    ldw     r2, (r1,r0)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj9_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r1,r0)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj9_dc_end
;has reached zero, turn on inj and schedule off event
   
    ; This is an old-style output, we have to turn on PWM
    ldw     r2,#PWMPER0
    stb     r0,(r0,r2)
    ldw     r2,#PWME
    ldb     r3,(r0,r2)
    orl     r3,#1
    stb     r3,(r0,r2)

    ldw     r2,#InjPWMTim1
    ldb     r3,(r0,r2)
    ldw     r4,#pwm1_on
    stb     r3,(r0,r4)

cont_inj9_sched:
    ldw     r2,#next_inj_8 ; r2 = &inj_event
    ldl     r1,#8

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj9_dc_end:

    ldw     r1,#inj10_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj10_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj10_dc_end
;has reached zero, turn on inj and schedule off event

    ; This is an old-style output, we have to turn on PWM
    ldw     r2,#PWMPER1
    stb     r0,(r0,r2)
    ldw     r2,#PWME
    ldb     r3,(r0,r2)
    orl     r3,#2
    stb     r3,(r0,r2)

    ldw     r2,#InjPWMTim2
    ldb     r3,(r0,r2)
    ldw     r4,#pwm2_on
    stb     r3,(r0,r4)

    bitl    r6,#0x2
    beq     cont_inj10_sched

    ; If we get here both pins were set, PWM for both.
    ldw     r2,#PWMPER0
    stb     r0,(r0,r2)
    ldw     r2,#PWME
    ldb     r3,(r0,r2)
    orl     r3,#1
    stb     r3,(r0,r2)

    ldw     r2,#InjPWMTim1
    ldb     r3,(r0,r2)
    ldw     r4,#pwm1_on
    stb     r3,(r0,r4)

   
cont_inj10_sched:
    ldw     r2,#next_inj_9 ; r2 = &inj_event
    ldl     r1,#9

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj10_dc_end:

;Check inj11
    ldw     r1,#inj11_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj11_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj11_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_10 ; r2 = &inj_event
    ldl     r1,#10

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj11_dc_end:

;Check inj12
    ldw     r1,#inj12_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj12_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj12_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_11 ; r2 = &inj_event
    ldl     r1,#11

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj12_dc_end:

;Check inj13
    ldw     r1,#inj13_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj13_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj13_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_12 ; r2 = &inj_event
    ldl     r1,#12

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj13_dc_end:

;Check inj14
    ldw     r1,#inj14_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj14_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj14_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_13 ; r2 = &inj_event
    ldl     r1,#13

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj14_dc_end:

;Check inj15
    ldw     r1,#inj15_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj15_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj12_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_14 ; r2 = &inj_event
    ldl     r1,#14

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj15_dc_end:

;Check inj16
    ldw     r1,#inj16_cnt
    ldw     r2, (r0,r1)
    sub     r0,r2,r0 ; compare R2 to zero
    beq     inj16_dc_end

    subl    r2,#1 ; works
    subh    r2,#0
    stw     r2, (r0,r1)
;see if we reached zero
    sub     r0,r2,r0 ; compare R2 to zero
    bne     inj16_dc_end
;has reached zero, turn on inj and schedule off event

    ldw     r2,#next_inj_15 ; r2 = &inj_event
    ldl     r1,#15

    tfr     r7, pc
    bra     j_grab_timer
    ;returns here

inj16_dc_end:

    bra     ckpool

j_grab_timer:
    bra     grab_timer

; Just-in-time timer schedulers
ckpool:
; now check the pool counters - one per injector channel
; when it is just-in-time, code is called to pick a timer
    ldw     r4,#TIMTCNT
    ldw     r6,(r0,r4)      ; r6 = TIMTCNT
ckpool1:
    ldw     r4,#timerpool_flag+0
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool2         ; off so skip

    ldw     r4,#timerpool+0
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool2         ; distance > 256us, skip

    ldl     r1,#0
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool2:
    ldw     r4,#timerpool_flag+1
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool3         ; off so skip

    ldw     r4,#timerpool+2
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool3         ; distance > 256us, skip

    ldl     r1,#2
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool3:
    ldw     r4,#timerpool_flag+2
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool4         ; off so skip

    ldw     r4,#timerpool+4
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool4         ; distance > 256us, skip

    ldl     r1,#4
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool4:
    ldw     r4,#timerpool_flag+3
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool5         ; off so skip

    ldw     r4,#timerpool+6
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool5         ; distance > 256us, skip

    ldl     r1,#6
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool5:
    ldw     r4,#timerpool_flag+4
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool6         ; off so skip

    ldw     r4,#timerpool+8
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool6         ; distance > 256us, skip

    ldl     r1,#8
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool6:
    ldw     r4,#timerpool_flag+5
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool7         ; off so skip

    ldw     r4,#timerpool+10
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool7         ; distance > 256us, skip

    ldl     r1,#10
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool7:
    ldw     r4,#timerpool_flag+6
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool8         ; off so skip

    ldw     r4,#timerpool+12
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool8         ; distance > 256us, skip

    ldl     r1,#12
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool8:
    ldw     r4,#timerpool_flag+7
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool9         ; off so skip

    ldw     r4,#timerpool+14
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool9         ; distance > 256us, skip

    ldl     r1,#14
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool9:
    ldw     r4,#timerpool_flag+8
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool10         ; off so skip

    ldw     r4,#timerpool+16
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool10         ; distance > 256us, skip

    ldl     r1,#16
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool10:
    ldw     r4,#timerpool_flag+9
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool11         ; off so skip

    ldw     r4,#timerpool+18
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool11         ; distance > 256us, skip

    ldl     r1,#18
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool11:
    ldw     r4,#timerpool_flag+10
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool12         ; off so skip

    ldw     r4,#timerpool+20
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool12         ; distance > 256us, skip

    ldl     r1,#20
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool12:
    ldw     r4,#timerpool_flag+11
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool13         ; off so skip

    ldw     r4,#timerpool+22
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool13         ; distance > 256us, skip

    ldl     r1,#22
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool13:
    ldw     r4,#timerpool_flag+12
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool14         ; off so skip

    ldw     r4,#timerpool+24
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool14         ; distance > 256us, skip

    ldl     r1,#24
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool14:
    ldw     r4,#timerpool_flag+13
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool15         ; off so skip

    ldw     r4,#timerpool+26
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool15         ; distance > 256us, skip

    ldl     r1,#26
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool15:
    ldw     r4,#timerpool_flag+14
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool16         ; off so skip

    ldw     r4,#timerpool+28
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool16         ; distance > 256us, skip

    ldl     r1,#28
    tfr     r7,pc
    bra     set_timer
    ;returns here

ckpool16:
    ldw     r4,#timerpool_flag+15
    ldb     r3,(r0,r4)      ; r3=timerpool_flag[0]
    cmpl    r3,#0
    beq     ckpool17         ; off so skip

    ldw     r4,#timerpool+30
    ldw     r5,(r0,r4)      ; timer target
    sub     r3,r5,r6        ; r3 = target - current
    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    bne     ckpool17         ; distance > 256us, skip

    ldl     r1,#30
    tfr     r7,pc
    bra     set_timer
    ;returns here
ckpool17:

    csem    #0
    rts
;-----------------------------

grab_timer:
; injector grabbing subroutine
; on entry r1 low byte = injector to schedule 0-15
; r2 is pointer to next_inj element

; first set the output pin from the array
    ldw     r3,(r2,#2) ; r3 = inj_event.port (which is &of the port reg)
    ldb     r4,(r0,r3) ; r4 = *inj_event.port
    ldb     r6,(r2,#4)     ; r6 = inj_event.pin
    or      r4,r4,r6       ; r4 |= r6
    stb     r4,(r0,r3)     ; port = r4

    ldh     r1, #0
;see if injector is already on - this can happen ~100% duty
    ldw     r4,#inj_status
    ldb     r3,(r1,r4)
    bitl    r3,#0x80
    beq     gt_cksemi ; ok, isn't on at the moment
    ; injector currently on, figure out what's controlling it and close them down
    bitl    r3,#0x40
    beq     gt_onhires  ; hi-res timer in control
    ; pooled timer in control
    ldw     r4,#timerpool_flag
    ldl     r5,#0
    stb     r5,(r1,r4)     ; timerpool_flag[r1] = 0 ; kill it
    bra     gt_cksemi

gt_onhires:
    ; hi-res hardware timer is on - turn it off
    ;r3 low bits contain hardware timer no.
    andl    r3,#7 ; clear others
    ldl     r5,#1
    lsl     r5,r3 ; gives us a bit
    xnorl   r5,#0  ; invert to give mask
    ldw     r4, #TIMTIE
    ldb     r2, (r0,r4)
    and     r2, r5, r2 ; mask out that bit
    stb     r2, (r0,r4)

    xnorl   r5,#0  ; invert again to give bit
    ldw     r4, #TIMTFLG1
    stb     r5, (r0,r4) ; ack any pending interrupt
    
    ; set bit in timer_usage to show free
    ldw     r4,#timer_usage
    ldb     r2,(r0,r4)
    or      r2,r2,r5 ; set that one bit
    stb     r2,(r0,r4) ; timer bit marked free

    ; In semi-sequential mode, one channel controls two outputs
    ; and one output is controlled by two channels, so need to check the
    ; sister channel also.
gt_cksemi:
    ldw     r4,#sister_inj
    ldb     r2,(r1,r4)
    cmpl    r2,#0
    beq     gt_nowoff ; no sister channel
; there is one, so check it
    ldh     r2,#0
    subl    r2,#1   ; use r2 as the new index
    ldw     r4,#inj_status
    ldb     r3,(r2,r4)
    bitl    r3,#0x80
    beq     gt_nowoff ; ok, isn't on at the moment
    ; injector currently on, figure out what's controlling it and close them down
    bitl    r3,#0x40
    beq     gt_onhires_sem  ; hi-res timer in control
    ; pooled timer in control
    ldw     r4,#timerpool_flag
    ldl     r5,#0
    stb     r5,(r2,r4)     ; timerpool_flag[r2] = 0 ; kill it
    bra     gt_nowoff

gt_onhires_sem:
    ; hi-res hardware timer is on - turn it off
    ;r3 low bits contain hardware timer no.
    andl    r3,#7 ; clear others
    ldl     r5,#1
    lsl     r5,r3 ; gives us a bit
    xnorl   r5,#0  ; invert to give mask
    ldw     r4, #TIMTIE
    ldb     r3, (r0,r4)
    and     r3, r5, r3 ; mask out that bit
    stb     r3, (r0,r4)

    xnorl   r5,#0  ; invert again to give bit
    ldw     r4, #TIMTFLG1
    stb     r5, (r0,r4) ; ack any pending interrupt
    
    ; set bit in timer_usage to show free
    ldw     r4,#timer_usage
    ldb     r2,(r0,r4)
    or      r2,r2,r5 ; set that one bit
    stb     r2,(r0,r4) ; timer bit marked free

    ; have now cleared any pending 100% duty timers, go ahead and set this one
gt_nowoff:
    ; here r1 is 0-15 for desired inj channel
    lsl     r1, #1  ; double it as address offset
    
;now work out off time
    ; get seq_pw[r1]
    ldw     r4,#seq_pw  ; r4 = &seq_pw[]
    ldw     r3,(r1,r4)     ; r3 = PW  ( seq_pw[r1] )

    ldw     r4,#TIMTCNT    ; r4 = &TIMTCNT ; calculate target timer value
    ldw     r5,(r0,r4)     ; r5 = TIMTCNT
    add     r5,r3,r5       ; r5 = TIMTCNT + PW (target time)

    ldl     r4,#0x04        ; (N=0, Z=1, V=0, C=0)
    tfr     ccr,r4          ; R4 => CCR
    cpch    r3,#0           ; needs Z=1 on only
    beq     set_timer       ; PW < 256us, go and set timer now

    ; long pulsewidth, use timer pool
    ; timerpool_flag[r1]
    ; timerpool[r1] = target time
    
    ldw     r4,#timerpool
    stw     r5,(r1,r4)     ; timerpool[r1] = r5 = target time
;ok up to here
    lsr     r1,#1
    ldw     r4,#timerpool_flag
    ldl     r5,#1
    stb     r5,(r1,r4)     ; timerpool_flag[r1] = 1

    ; update status for this injector
    ldw     r4,#inj_status
    ldl     r5, #0xc0   ; on and using jit timer
    stb     r5,(r1,r4)  ; inj_status[r1] = 0xc0
  
    jal     r7  ; "RTS"

set_timer:
    ; on entry r5 = target timer value
    ; r1 = 2 * injector channel i.e. 0,2,4,6

;
; Section of code here needed to find r2, offset to free timer
;
    ldw     r4,#timer_usage
    ldb     r3,(r0,r4)

    bffo    r2, r3  ; find the free timer bit

    lsr     r1,#1   ; restore r1 to 0,1,2

    ; update status for this injector
    ldw   r4,#inj_status
    ldl   r3,#0x80
    or    r3,r2,r3
    stb   r3,(r1,r4)  ; set status as on, using timer, timer r2

    lsl     r2,#1   ; convert r2 to 0,2,4

    ;r2 = timer offset
    ;r1 = injector no.

    ; Set timer to proper value
    ldw     r4,#TIMTC0     ; r6 = &TIMTC0
    stw     r5,(r2,r4)     ; TIMTC0[r1] = r5 (Uses r1 as 0,2,4)

    
    ; calculate r5 is bitfield/mask of timer with a single 1 bit
    lsr     r2,#1   ; restore r2 to 0,1,2
    ldl     r5,#1
    lsl     r5,r2  ; r5 = (1 << r2)

    ldw     r4,#inj_chan ; store inj no. into timer array - presently 1:1 mapping
    stb     r1,(r4,r2)      ; inj_chan[timer] = injector no.


; update timerusage
    xnorl   r5,#0  ; invert
    ldw     r4,#timer_usage
    ldb     r3,(r0,r4)
    and     r3,r3,r5 ; clear that one bit
    stb     r3,(r0,r4) ; timer bit marked used
    xnorl   r5,#0  ; invert again

    ; enable the interrupt
    ldw     r4,#TIMTFLG1
    stb     r5,(r0,r4)     ; clear interrupt flag with r6
    ldw     r4,#TIMTIE     ; r2 = &TIMTIE
    ldb     r3,(r0,r4)     ; r3 = TIMTIE
    or      r3,r3,r5       ; r3 |= r5
    stb     r3,(r0,r4)     ; TIMTIE = r3

    ldw     r4,#timerpool_flag
    stb     r0,(r1,r4)     ; timerpool_flag[r1] = 0  ; mark jit element as unused

    jal     r7  ; "RTS" see datasheet


XGATE_DUMMY_ISR_FLASH:
  rts
  nop
XGATE_CODE_FLASH_END:
xgate_isr_end:

.sect .texte0
.org 0x800 ; wastes first 2k of this flash page
; this is in flash @ 0x0800
random_no:
; actual random
    fcb 0xb2, 0x8a, 0x29, 0x54, 0x48, 0x9a, 0x0a, 0xbc, 0xd5, 0x0e, 0x18, 0xa8, 0x44, 0xac, 0x5b, 0xf3
    fcb 0x8e, 0x4c, 0xd7, 0x2d, 0x9b, 0x09, 0x42, 0xe5, 0x06, 0xc4, 0x33, 0xaf, 0xcd, 0xa3, 0x84, 0x7f 
    fcb 0x2d, 0xad, 0xd4, 0x76, 0x47, 0xde, 0x32, 0x1c, 0xec, 0x4a, 0xc4, 0x30, 0xf6, 0x20, 0x23, 0x85 
    fcb 0x6c, 0xfb, 0xb2, 0x07, 0x04, 0xf4, 0xec, 0x0b, 0xb9, 0x20, 0xba, 0x86, 0xc3, 0x3e, 0x05, 0xf1 
    fcb 0xec, 0xd9, 0x67, 0x33, 0xb7, 0x99, 0x50, 0xa3, 0xe3, 0x14, 0xd3, 0xd9, 0x34, 0xf7, 0x5e, 0xa0 
    fcb 0xf2, 0x10, 0xa8, 0xf6, 0x05, 0x94, 0x01, 0xbe, 0xb4, 0xbc, 0x44, 0x78, 0xfa, 0x49, 0x69, 0xe6 
    fcb 0x23, 0xd0, 0x1a, 0xda, 0x69, 0x6a, 0x7e, 0x4c, 0x7e, 0x51, 0x25, 0xb3, 0x48, 0x84, 0x53, 0x3a 
    fcb 0x94, 0xfb, 0x31, 0x99, 0x90, 0x32, 0x57, 0x44, 0xee, 0x9b, 0xbc, 0xe9, 0xe5, 0x25, 0xcf, 0x08 
    fcb 0xf5, 0xe9, 0xe2, 0x5e, 0x53, 0x60, 0xaa, 0xd2, 0xb2, 0xd0, 0x85, 0xfa, 0x54, 0xd8, 0x35, 0xe8 
    fcb 0xd4, 0x66, 0x82, 0x64, 0x98, 0xd9, 0xa8, 0x87, 0x75, 0x65, 0x70, 0x5a, 0x8a, 0x3f, 0x62, 0x80 
    fcb 0x29, 0x44, 0xde, 0x7c, 0xa5, 0x89, 0x4e, 0x57, 0x59, 0xd3, 0x51, 0xad, 0xac, 0x86, 0x95, 0x80 
    fcb 0xec, 0x17, 0xe4, 0x85, 0xf1, 0x8c, 0x0c, 0x66, 0xf1, 0x7c, 0xc0, 0x7c, 0xbb, 0x22, 0xfc, 0xe4 
    fcb 0x66, 0xda, 0x61, 0x0b, 0x63, 0xaf, 0x62, 0xbc, 0x83, 0xb4, 0x69, 0x2f, 0x3a, 0xfd, 0xaf, 0x27 
    fcb 0x16, 0x93, 0xac, 0x07, 0x1f, 0xb8, 0x6d, 0x11, 0x34, 0x2d, 0x8d, 0xef, 0x4f, 0x89, 0xd4, 0xb6 
    fcb 0x63, 0x35, 0xc1, 0xc7, 0xe4, 0x24, 0x83, 0x67, 0xd8, 0xed, 0x96, 0x12, 0xec, 0x45, 0x39, 0x02 
    fcb 0xd8, 0xe5, 0x0a, 0xf8, 0x9d, 0x77, 0x09, 0xd1, 0xa5, 0x96, 0xc1, 0xf4, 0x1f, 0x95, 0xaa, 0x82
; 5/7
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0
    fcb 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff
    fcb 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0
    fcb 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0
    fcb 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff
    fcb 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0
    fcb 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xff
; 4/5
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0
    fcb 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0
    fcb 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0
    fcb 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0
    fcb 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0
    fcb 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0
    fcb 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0
    fcb 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0
    fcb 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff
    fcb 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xff, 0xe0, 0xe0, 0xff

;.equ XGATE_DUMMY_ISR_XG, (XGATE_CODE_FLASH_END-XGATE_CODE_FLASH)+XGATE_CODE_XG
.equ XGATE_SIZE, XGATE_CODE_FLASH_END-XGATE_CODE_FLASH+0x200
.if XGATE_SIZE > 0x1c00
.error "XGATE code will overflow onto vars2"
.endif
.nolist
