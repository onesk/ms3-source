;*********************************************************************
; Configure XGATE
;*********************************************************************
; $Id: xg_conf.s,v 1.32 2013/06/01 17:02:12 jsmcortina Exp $
; * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; * Origin: James Murray / Kenneth Culver / Freescale MC9S12XEP100 datasheet
; * Majority: James Murray / Kenneth Culver
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

.sect .textfa ; (far - same as xgate code)
.globl config_xgate

             nolist               ;turn off listing
             include "ms3h.inc"
             list                 ;turn listing back on

;**************************************************************************
; XGATE configuration symbols
                        ;###########################################
                        ;#                   SYMBOLS                   #
                        ;###########################################
.equ TC0_VEC,           0xEE             ;TC0 vector number
.equ TC1_VEC,           0xEC             ;TC1 vector number
.equ TC2_VEC,           0xEA             ;TC2 vector number
.equ TC3_VEC,           0xE8             ;TC3 vector number
.equ TC4_VEC,           0xE6             ;TC4 vector number
.equ TC5_VEC,           0xE4             ;TC5 vector number
.equ TC6_VEC,           0xE2             ;TC6 vector number
.equ TC7_VEC,           0xE0             ;TC7 vector number
.equ TIMTC0_VEC,        0x54             ;TIMTC0 vector number
.equ TIMTC1_VEC,        0x52             ;TIMTC1 vector number
.equ TIMTC2_VEC,        0x50             ;TIMTC2 vector number
.equ TIMTC3_VEC,        0x4E             ;TIMTC3 vector number
.equ TIMTC4_VEC,        0x4C             ;TIMTC4 vector number
.equ TIMTC5_VEC,        0x4A             ;TIMTC5 vector number
.equ TIMTC6_VEC,        0x48             ;TIMTC6 vector number
.equ TIMTC7_VEC,        0x46             ;TIMTC7 vector number
.equ XGATE0_VEC,        0x72             ;XGATE0 vector number
.equ XGATE1_VEC,        0x70             ;XGATE1 vector number
.equ XGATE2_VEC,        0x6e             ;XGATE2 vector number
.equ XGATE3_VEC,        0x6c             ;XGATE3 vector number
.equ PIT2_VEC,          0x76             ;PIT2 vector number
.equ RQST,              0x80             ;RQST bit mask
.equ RAM_SIZE,          32*0x400          ;32k RAM
.equ RAM_START,         0x1000
.equ RAM_START_XG,      0x10000-RAM_SIZE
.equ RAM_START_GLOB,    0x100000-RAM_SIZE
.equ XGATE_VECTORS,     RAM_START
.equ XGATE_VECTORS_XG,  RAM_START_XG
.equ XGATE_CODE,        RAM_START+(4*128) ; allows space for XGATE vector table
.equ XGATE_CODE_XG,     RAM_START_XG+(4*128)
.equ XGMCTL_CLEAR,      0xFA02          ;Clear all XGMCTL bits
.equ XGMCTL_ENABLE,     0xA383          ;Enable XGATE + software interrupts + clear DEBUG bit. Was 0x8383

config_xgate:
                        ;###########################################
                        ;#           INITIALIZE S12X_INT           #
                        ;###########################################
INIT_INT:
                        ; always use XGATE for spark TC1
                        MOVB #(TC1_VEC&0xF0), INT_CFADDR      ; switch TC1 interrupts to XGATE if we are otherwise HC12 ISR is used
                        MOVB #0x81, INT_CFDATA0+((TC1_VEC&0x0F)>>1) ; 

                        movb #(XGATE0_VEC&0xF0), INT_CFADDR             ; software trigger 0
                        movb #0x81, INT_CFDATA0+((XGATE0_VEC&0x0F)>>1)

                        movb #(XGATE1_VEC&0xF0), INT_CFADDR             ; software trigger 1
                        movb #0x81, INT_CFDATA0+((XGATE1_VEC&0x0F)>>1)

                        movb #(XGATE2_VEC&0xF0), INT_CFADDR             ; software trigger 2
                        movb #0x81, INT_CFDATA0+((XGATE2_VEC&0x0F)>>1)

                        movb #(XGATE3_VEC&0xF0), INT_CFADDR             ; software trigger 3
                        movb #0x81, INT_CFDATA0+((XGATE3_VEC&0x0F)>>1)

                        movb #(PIT2_VEC&0xF0), INT_CFADDR               ; PIT2
                        movb #0x81, INT_CFDATA0+((PIT2_VEC&0x0F)>>1)

                        brclr   flagbyte9, #FLAGBYTE9_CAS360_ON, no_cas360
                        movb #(TC0_VEC&0xF0), INT_CFADDR             ; crank -> XGATE
                        movb #0x81, INT_CFDATA0+((TC0_VEC&0x0F)>>1)

                        brset   ram4.hardware, #HARDWARE_CAM, cas360tc4
                        movb #(TC5_VEC&0xF0), INT_CFADDR             ; cam (JS10) -> XGATE
                        movb #0x81, INT_CFDATA0+((TC5_VEC&0x0F)>>1)
                        bra     no_cas360
cas360tc4:
                        movb #(TC4_VEC&0xF0), INT_CFADDR             ; cam (PT4) -> XGATE
                        movb #0x81, INT_CFDATA0+((TC4_VEC&0x0F)>>1)

no_cas360:
                        movb #(TC3_VEC&0xF0), INT_CFADDR             ; rotary spark on TC3
                        movb #0x81, INT_CFDATA0+((TC3_VEC&0x0F)>>1)

                        brset    ram4.EngStroke, #2, use_tc6_1
                        ldaa    spkmode
                        cmpa    #14
                        beq     use_tc6_1
                        bra     not_rotary1
use_tc6_1:
                        movb #(TC6_VEC&0xF0), INT_CFADDR             ; rotary dwell on TC6
                        movb #0x81, INT_CFDATA0+((TC6_VEC&0x0F)>>1)
not_rotary1:
                        movb #(TC7_VEC&0xF0), INT_CFADDR             ; dwell on TC7
                        movb #0x81, INT_CFDATA0+((TC7_VEC&0x0F)>>1)

seq_setup:
                        movb #(TIMTC0_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC0_VEC&0x0F)>>1)

                        movb #(TIMTC1_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC1_VEC&0x0F)>>1)

                        movb #(TIMTC2_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC2_VEC&0x0F)>>1)

                        movb #(TIMTC3_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC3_VEC&0x0F)>>1)

                        movb #(TIMTC4_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC4_VEC&0x0F)>>1)

                        movb #(TIMTC5_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC5_VEC&0x0F)>>1)

                        movb #(TIMTC6_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC6_VEC&0x0F)>>1)

                        movb #(TIMTC7_VEC&0xF0), INT_CFADDR
                        movb #0x81, INT_CFDATA0+((TIMTC7_VEC&0x0F)>>1)

continue_xg:
                        MOVB #(RAM_START_GLOB>>12), RPAGE ;set RAM page window to point to XGATE RAM
                        ;###########################################
                        ;#             INITIALIZE XGATE                #
                        ;###########################################
INIT_XGATE:             MOVW #XGMCTL_CLEAR, XGMCTL               ;clear all XGMCTL bits
INIT_XGATE_BUSY_LOOP:   TST  XGCHID           ;wait until current thread is finished
                        BNE  INIT_XGATE_BUSY_LOOP
                        LDX  #XGIF78            ;clear all channel interrupt flags
                        LDD  #0xFFFF
                        STD  2,X+
                        STD  2,X+
                        STD  2,X+
                        STD  2,X+
                        STD  2,X+
                        STD  2,X+
                        STD  2,X+
                        STD  2,X+
                        CLR XGISPSEL          ;set vector base register
                        MOVW #XGATE_VECTORS_XG, XGVBR
                        MOVW #0xFF00, XGSWTM    ;clear all software triggers
                        ;###########################################
                        ;#      INITIALIZE XGATE VECTOR TABLE          #
                        ;###########################################
                        LDAA #128             ;build XGATE vector table
                        LDY #XGATE_VECTORS
; assembler doesn't want to do the following:
;INIT_XGATE_VECTAB_LOOP: MOVW #(XGATE_DUMMY_ISR_FLASH-XGATE_CODE_FLASH)+XGATE_CODE_XG, 4,Y+
; so do it on the chip instead
                        ldx     #XGATE_DUMMY_ISR_FLASH
                        subx    #XGATE_CODE_FLASH
                        addx    #XGATE_CODE_XG
INIT_XGATE_VECTAB_LOOP: stx 4,Y+
                        DBNE A, INIT_XGATE_VECTAB_LOOP

                        ; always doing spark on XGATE on TC1
                        ldx #XGATE_CODE_XG
                        addx #xgss_tc1
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TC1_VEC)
                        MOVW #0, RAM_START+(2*TC1_VEC)+2  ; data points to zero

                        ;xgate0
                        ldx #XGATE_CODE_XG
                        addx #xgss_xgate0
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*XGATE0_VEC)
                        movw #0, RAM_START+(2*XGATE0_VEC)+2

                        ;xgate1
                        ldx #XGATE_CODE_XG
                        addx #xgss_xgate1
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*XGATE1_VEC)
                        movw #0, RAM_START+(2*XGATE1_VEC)+2

                        ;xgate2
                        ldx #XGATE_CODE_XG
                        addx #xgss_xgate2
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*XGATE2_VEC)
                        movw #0, RAM_START+(2*XGATE2_VEC)+2

                        ;xgate3
                        ldx #XGATE_CODE_XG
                        addx #xgss_xgate3
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*XGATE3_VEC)
                        movw #0, RAM_START+(2*XGATE3_VEC)+2

                        ;PIT2
                        ldx #XGATE_CODE_XG
                        addx #xgss_pit2
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*PIT2_VEC)
                        movw #0, RAM_START+(2*PIT2_VEC)+2

                        brclr   flagbyte9, #FLAGBYTE9_CAS360_ON, set_tc3
                        ;TC0
                        ldx #XGATE_CODE_XG
                        addx #crank_tc0
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TC0_VEC)
                        movw #0, RAM_START+(2*TC0_VEC)+2

                        brset   ram4.hardware, #HARDWARE_CAM, set_cam_tc4
                        ;TC5 cam
                        ldx #XGATE_CODE_XG
                        addx #cam_tc5
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TC5_VEC)
                        movw #0, RAM_START+(2*TC5_VEC)+2
                        bra     set_tc3
set_cam_tc4:
                        ;TC4 cam
                        ldx #XGATE_CODE_XG
                        addx #cam_tc4
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TC4_VEC)
                        movw #0, RAM_START+(2*TC4_VEC)+2
set_tc3:

                        ;TC3 rotary spark
                        ldx #XGATE_CODE_XG
                        addx #xgss_tc3
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TC3_VEC)
                        movw #0, RAM_START+(2*TC3_VEC)+2

                        brset    ram4.EngStroke, #2, use_tc6_2
                        ldaa    spkmode
                        cmpa    #14
                        beq     use_tc6_2
                        bra     not_rotary2
use_tc6_2:
                        ;TC6
                        ldx #XGATE_CODE_XG
                        addx #xgss_tc6
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TC6_VEC)
                        movw #0, RAM_START+(2*TC6_VEC)+2
not_rotary2:

                        ;TC7
                        ldx #XGATE_CODE_XG
                        addx #xgss_tc7
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TC7_VEC)
                        movw #0, RAM_START+(2*TC7_VEC)+2

seq_setup_2:
                        ;inj1
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj1
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC0_VEC)
                        movw #0, RAM_START+(2*TIMTC0_VEC)+2

                        ;inj2
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj2
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC1_VEC)
                        movw #0, RAM_START+(2*TIMTC1_VEC)+2

                        ;inj3
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj3
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC2_VEC)
                        movw #0, RAM_START+(2*TIMTC2_VEC)+2

                        ;inj4
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj4
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC3_VEC)
                        movw #0, RAM_START+(2*TIMTC3_VEC)+2

                        ;inj5
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj5
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC4_VEC)
                        movw #0, RAM_START+(2*TIMTC4_VEC)+2

                        ;inj6
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj6
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC5_VEC)
                        movw #0, RAM_START+(2*TIMTC5_VEC)+2

                        ;inj7
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj7
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC6_VEC)
                        movw #0, RAM_START+(2*TIMTC6_VEC)+2

                        ;inj8
                        ldx #XGATE_CODE_XG
                        addx #xgss_inj8
                        subx #XGATE_CODE_FLASH
                        stx RAM_START+(2*TIMTC7_VEC)
                        movw #0, RAM_START+(2*TIMTC7_VEC)+2

                        ;###########################################
                        ;#              COPY XGATE CODE                #
                        ;###########################################
COPY_XGATE_CODE:        LDX #XGATE_CODE_FLASH
COPY_XGATE_CODE_LOOP:   MOVW 2,X+, 2,Y+
                        cpy #0x2000
                        bne cxcl2
                        ldy #0x1000
                        inc RPAGE
cxcl2:
                        CPX  #XGATE_CODE_FLASH_END
                        BLS  COPY_XGATE_CODE_LOOP
                        ;###########################################
                        ;#               START XGATE               #
                        ;###########################################
START_XGATE:            MOVW #XGMCTL_ENABLE, XGMCTL          ;enable XGATE
                        rtc ; far

