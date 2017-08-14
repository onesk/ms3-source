;*********************************************************************
; ISR_SPI
;*********************************************************************
; $Id: isr_spi.s,v 1.20.8.1 2015/01/23 15:44:31 jsmcortina Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; * Origin: James Murray
; * Majority: James Murray
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

.sect .text3
.globl ISR_SPI
             nolist               ;turn off listing
             include "ms3h.inc"
             list                 ;turn listing back on
;*********************************************************************

; This is the number of fetches we make for retrieving status bytes before
; flagging an error. Depends on baud rate.
.equ    WAIT_LOOP,    10000 ;  timeout

; Here we execute SD card commands step by step
; sd_int_cmd specifies the command sequence
; 0 = init1
; 1 = init2
; 2 = read byte
; 4 = write block
; 5 = read block
; 10 = send_command
; ff = final read byte


ISR_SPI:
    brset   SPI1SR, #0x20, spi_t_ok ; should always be set
    bra     dummy_read ; always do this for now
spi_t_ok:
    ldab    SPI1SR ; part of flag clearing process

    ldab    sd_int_phase
    beq     dummy_read  ; shouldn't be here

    brclr   flagbyte19, #FLAGBYTE19_SPI_RB27, ispi_normal
    bra     spi_rb27qk    ; jump right in there to save a few cycles (5%)

ispi_normal:    
    ldaa    sd_int_cmd
    cmpa    #5
    beq     spi_read_block
    cmpa    #4
    beq     spi_write_block
    cmpa    #10
    beq     spi_send_cmd
    tsta
    beq     spi_sd_init
    cmpa    #1
    beq     spi_sd_init2
    cmpa    #2
    beq     spi_sd_rd

    bra     dummy_read  ; shouldn't be here

;--------------
spi_sd_init:
    cmpb    #9 ; 1,2,3,4,5,6,7,8,9
    ble     spi_in1
    cmpb    #10
    beq     spi_in10
    cmpb    #16 ;11,12,13,14,15,16
    ble     spi_in11
    cmpb    #17
    beq     spi_in17

    bra     dummy_read  ; shouldn't be here

;--------------
spi_sd_init2:
    ldaa    SPI1DRL
    staa    sd_rx
    movb    #0x00,SPI1DRL   ; send a second zero
    clr     sd_int_phase
    bra     isr_spi_end
;--------------
spi_sd_rd:
    ldab    SPI1DRL ; read the data
    stab    sd_rx
    clr     sd_int_phase
    bra     isr_spi_end
;--------------
spi_write_block:
    cmpb    #22
    beq     spi_wb22
    cmpb    #17
    beq     spi_wb17 ; writing data

    cmpb    #4
    bls     spi_wb1 ;1,2,3,4
    cmpb    #5
    beq     spi_wb5
    cmpb    #6
    beq     spi_wb6
    cmpb    #16
    bls     spi_wb7; 7,8,9,10,11,12,13,14,15,16

    cmpb    #20
    ble     spi_wb18 ;18,19,20
    cmpb    #21
    beq     spi_wb21
    cmpb    #23
    beq     spi_wb23
    bra     dummy_read  ; shouldn't be here

;--------------
spi_read_block:
    cmpb    #27
    beq     spi_rb27 ; actual data read

    cmpb    #4
    bls     spi_rb1 ;1,2,3,4
    cmpb    #5
    beq     spi_rb5
    cmpb    #6
    beq     spi_rb6
    cmpb    #16
    bls     spi_rb7; 7,8,9,10,11,12,13,14,15,16
    cmpb    #26
    bls     spi_rb17 ; 17,18,19,20,21,22,23,24,25,26
    cmpb    #29
    bls     spi_rb28 ; 28,29
    cmpb    #30
    beq     spi_rb30
    cmpb    #31
    beq     spi_rb31

    bra     dummy_read  ; shouldn't be here

;--------------
spi_send_cmd:
    cmpb    #4
    bls     spi_sc1 ;1,2,3,4
    cmpb    #5
    beq     spi_sc5
    cmpb    #20
    bls     spi_sc6; 6,7,8,9,10,11,12,13,14,15

;--------------
dummy_read:
    ; not expecting to get here, but do a dummy read if something is there waiting
    ldab    SPI1SR ; part of flag clearing process
    ldab    SPI1DRL
    stab    sd_rx
    clr     sd_int_phase

    bra     isr_spi_end

;--------------
spi_in1:
    ldaa    SPI1DRL ; dummy read the data
    movb    #0xff, SPI1DRL
    bra     spi_inc_exit

spi_in10:
    ldaa    SPI1DRL ; dummy read the data
    bset    PTH, #8         ; disable SPI
    movb    #0xff, SPI1DRL
    bra     spi_inc_exit

spi_in11:
    ldaa    SPI1DRL ; dummy read the data
    movb    #0xff, SPI1DRL
    bra     spi_inc_exit

spi_in17:
    ldaa    SPI1DRL ; dummy read the data
    bclr    SPI1CR1, #0x80 ; turn off ints
    movb    #0xff, SPI1DRL
    clr     sd_int_phase
    bra     isr_spi_end
;--------------
spi_wb1:
    ldaa    SPI1DRL ; dummy read the data
    ldx     #sd_int_sector-1
    abx
    movb    0,x, SPI1DRL ; send the byte
    bra     spi_inc_exit

spi_wb5:
    movb    sd_baud_write, SPI1BR ;try changing SPI speed here.
    ldaa    SPI1DRL ; dummy read the data
    movb    #0x95, SPI1DRL ; send the byte
    bra     spi_inc_exit

spi_wb6:
    ldaa    SPI1DRL ; dummy read the data
    movb    #0xff, SPI1DRL ; Equates to part of get byte for first check in wb7
    clrw    sd_int_cnt     ; loop counter
    bra     spi_inc_exit

spi_wb7:
    ldaa    SPI1DRL         ; read the data
    bmi     spi_wb7_3       ; top bit set, no R1 byte yet
    beq     spi_wb7_done    ; SD_OK = 0
    ;wasn't zero
    movb    #1, sd_int_error
    bra     spi_dis_exit

spi_wb7_3:
    incw    sd_int_cnt
    ldd     sd_int_cnt
    cpd     #WAIT_LOOP
    blo     spi_wb7_2
    
    movb    #2, sd_int_error    ; error code
    bra     spi_dis_exit

spi_wb7_2:
    movb    #0xff, SPI1DRL  ; dummy write
    bra     isr_spi_end     ; keep looping - count above

spi_wb7_done:
    movb    #0xfe, SPI1DRL  ; send the start token byte for single blocks
    movb    #17, sd_int_phase
    ldd     sd_int_addr
    std     sdrw_addr
    addd    sd_sectsize
    std     sdrw_targ
    bra     isr_spi_end

spi_wb17:
; This is where we write the data.
; address = sdrw_addr
; target address = sdrw_targ
    ldaa    SPI1DRL         ; dummy read
    ldx     sdrw_addr
    movb    1,x+, SPI1DRL

    stx     sdrw_addr
    cpx     sdrw_targ
    bne     isr_spi_end
    bra     spi_inc_exit

spi_wb18:
    ldaa    SPI1DRL         ; dummy read
    movb    #0xff, SPI1DRL  ; send the byte
    bra     spi_inc_exit

spi_wb21: ; wait until data response == 5 "accepted" is received
    ldaa    SPI1DRL
    staa    sd_rx

    movb    #0xff, SPI1DRL  ; send a byte for next read
    anda    #0x1f
    cmpa    #0x05           ; return status
    beq     spi_inc_exit
    movb    #3, sd_int_error    ; error code
    bra     spi_dis_exit

spi_wb22:           ; need timeout to prevent getting stuck at this phase
    ldaa    SPI1DRL
    bne     spi_wb22_ok     ; wait for BUSY to clear
    movb    #0xff, SPI1DRL  ; send a byte for next read
    bra     isr_spi_end

spi_wb22_ok:
    bset    PTH, #8
    movb    #0xff, SPI1DRL  ; send a byte for next read
    bra     spi_inc_exit

spi_wb23:
    movb    sd_baud_norm, SPI1BR ;try changing SPI speed here.
    ldab    SPI1DRL
    clr     sd_int_phase
    bra     isr_spi_end

;--------------

spi_rb1:
    ldaa    SPI1DRL ; dummy read the data
    ldx     #sd_int_sector-1
    abx
    movb    0,x, SPI1DRL ; send the byte
    bra     spi_inc_exit

spi_rb5:
    ldaa    SPI1DRL ; dummy read the data
    movb    #0x95, SPI1DRL ; send the byte
    bra     spi_inc_exit

spi_rb6:
    ldaa    SPI1DRL         ; read the data
    movb    #0xff, SPI1DRL  ; dummy write
    clrw    sd_int_cnt     ; loop counter
    bra     spi_inc_exit
    
spi_rb7:
    ldaa    SPI1DRL         ; read the data
    staa    sd_rx
    bmi     spi_rb7_3       ; top bit set, no R1 byte yet
    beq     spi_rb7_done    ; SD_OK = 0
    ;wasn't zero
    movb    #4, sd_int_error
    bra     spi_dis_exit

spi_rb7_3:
    incw    sd_int_cnt
    ldd     sd_int_cnt
    cpd     #WAIT_LOOP
    blo     spi_rb7_2
    
    movb    #5, sd_int_error    ; error code
    bra     spi_dis_exit

spi_rb7_2:
    movb    #0xff, SPI1DRL  ; dummy write
    bra     isr_spi_end     ; keep looping - count above

spi_rb7_done:
    movb    #0xff, SPI1DRL  ; send the byte
    movb    #17, sd_int_phase
    clrw    sd_int_cnt     ; loop counter
    bra     isr_spi_end

spi_rb17:
    ldaa    SPI1DRL         ; read the data
    cmpa    #0xfe           ; start token
    beq     spi_rb17_done
    staa    sd_rx
    bita    #0xe0
    bne     spi_rb17_3      ; not an error token
    ;some error token
    bita    #0x1f
    beq     spi_rb17_3      ; top 3 bits clear saying error token, but no error reported. Ignore.
    movb    #6, sd_int_error
    bra     spi_dis_exit

spi_rb17_3:
    incw    sd_int_cnt
    ldd     sd_int_cnt
    cpd     #WAIT_LOOP
    blo     spi_rb17_2
    
    movb    #7, sd_int_error    ; error code
    bra     spi_dis_exit

spi_rb17_2:
    movb    #0xff, SPI1DRL  ; dummy write
    bra     isr_spi_end     ; keep looping - count above

spi_rb17_done:
    movb    #0xff, SPI1DRL  ; send the byte to get one in next phase
    movb    #27, sd_int_phase
    ldd     sd_int_addr
    std     sdrw_addr
    addd    sd_sectsize
    std     sdrw_targ
    bra     isr_spi_end

spi_rb27:
    bset    flagbyte19, #FLAGBYTE19_SPI_RB27 ; done with speedup
spi_rb27qk:
; This is where we read the data.
; address = sdrw_addr
; target address = sdrw_targ
    ldx     sdrw_addr
    movb    SPI1DRL, 1,x+    ; store in requested location

    movb    #0xff, SPI1DRL  ; send to receive

    stx     sdrw_addr
    cpx     sdrw_targ
    bne     isr_spi_end
    bclr    flagbyte19, #FLAGBYTE19_SPI_RB27 ; done with speedup
    bra     spi_inc_exit

spi_rb28:
    ldaa    SPI1DRL         ; dummy read
    movb    #0xff, SPI1DRL  ; send the byte
    bra     spi_inc_exit

spi_rb30:
    ldab    SPI1DRL
    bset    PTH, #8
    movb    #0xff, SPI1DRL  ; send a byte for next read
    bra     spi_inc_exit

spi_rb31:
    ldab    SPI1DRL
    clr     sd_int_phase
    bra     isr_spi_end

;--------------
spi_sc1:
    ldaa    SPI1DRL ; dummy read the data
    ldx     #sd_int_sector-1
    abx
    movb    0,x, SPI1DRL ; send the byte
    bra     spi_inc_exit

spi_sc5:
    ldaa    SPI1DRL ; dummy read the data
    movb    sd_rx, SPI1DRL ; send checksum the byte
    clrw    sd_int_cnt     ; loop counter
    clr     sd_int_error
    bra     spi_inc_exit

spi_sc6:
    ldaa    SPI1DRL         ; read the data
    staa    sd_rx   ; response

    bmi     sc6_noresp ; top bit is 1, nothing yet
    anda    sd_match_mask
    cmpa    sd_match        ; check for response code
    beq     sc6_done
    bra     sc6_nomatch

sc6_noresp:
    incw    sd_int_cnt
    ldd     sd_int_cnt
    cpd     #WAIT_LOOP            ; arbitrary loop timeout. FS code was 20.
    blo     sc6_2
    
sc6_nomatch:
    movb    #8, sd_int_error    ; error code
    clr     sd_int_phase
    bra     isr_spi_end

sc6_2:
    movb    #0xff, SPI1DRL  ; dummy write
    bra     isr_spi_end     ; keep looping

sc6_done:
    clr     sd_int_phase
    bra     isr_spi_end

;--------------

spi_dis_exit:
    clr     sd_int_phase
    bset    PTH, #8         ; disable SPI
    bra     isr_spi_end
     
spi_inc_exit:
    inc     sd_int_phase

isr_spi_end:
   rti


