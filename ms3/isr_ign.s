; $Id: isr_ign.s,v 1.88.4.1 2015/01/27 14:26:15 jsmcortina Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; * Origin: James Murray / Kenneth Culver
; * Majority: James Murray / Kenneth Culver
; * 
; * 
; * xg_loggers: James Murray
; * cam/vvt: James Murray
; * frequency MAF: James Murray
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

;Cam interrupts and some other low-level S12X ignition code.

  .sect .text3      ; was .text
  .globl ISR_TC6, ISR_TC5, ISR_TC4, ISR_TC2, do_complog_pri
  .globl    xg_crank_logger, xg_cam_logger
  
               nolist               ;turn off listing
               include "ms3h.inc"
               include "ms3_structs.inc"
               list                 ;turn listing back on on
;*********************************************************************
xg_crank_logger:
    ; gets executed from XGATE5 interrupt
    movw    #0x2000, XGSWTM ; clear int flag
    clra
    clrb
    ldx     xgcrkTC
    jsr     do_complog_pri_b ; jump in partway
    rti
;*********************************************************************
xg_cam_logger:
    ; gets executed from XGATE6 interrupt
    movw    #0x4000, XGSWTM ; clear int flag
    clra
    clrb
    ldx     xgcamTC

; this code is a near-duplicate of the normal S12 cam logger
    ldy	    #TRIGLOGBASE
    addy	log_offset

;set RPAGE
    ldaa    RPAGE
    cpy     #0x1ffd
    bhi     xcl2fin

    movb    #TRIGLOGPAGE, RPAGE

    ; ignore top byte in A
    andb	#0xf
    brclr	synch, #0x01, xcl2not_sync   ; HARDCODING
    orab	#0x10

xcl2not_sync:
    orab	#0x20 ; always tach2

    brclr   PTT,#1,xcl2low1
    orab	#0x40   ; store pri trig pin level in 0x40

xcl2low1:
    brset   ram4.hardware, #HARDWARE_CAM, xcl2low1_tc2
    brclr   PTT,#0x20,xcl2low2
    bra xcl2low1_or
xcl2low1_tc2:
    brclr   PTT,#0x10,xcl2low2 ; checks TC4 now
xcl2low1_or:
    orab	#0x80   ; store 2nd trig pin level in 0x80

xcl2low2:
    stab	0,Y ; top byte of time + 4 special bits
    stx	    1,Y ; low word

;restore RPAGE
    staa    RPAGE

    ldy     log_offset

    ldab    page
    cmpb    #0xf2
    beq     xcl2end1
;comp log loop mode

    movb    #TRIGLOGPAGE,RPAGE
    sty     TRIGLOGBASE + 0x3fd ; store current log_offset as pointer to next data point
    staa    RPAGE
    brclr   synch, #1, xcl2fin ; no sync.. we are done

xcll2inc:
    addy	#3
    sty	    log_offset
    cpy	    #0x3fc   ; three less
    bls	    xcl2done
;start again
    clrw    log_offset
    bra     xcl2done

xcl2end1:
    addy	#3
    sty	    log_offset
    cpy	    #0x3ff
    bls	    xcl2done

xcl2fin:
    ; we've reached the end of the buffer, stop
    bclr	flagbyte0, #FLAGBYTE0_COMPLOG
    movb    #TRIGLOGPAGE,RPAGE
    movb	#1, TRIGLOGBASE + 0x3ff ; HARDCODED version
    staa    RPAGE
    bset    outpc.status3, #STATUS3_DONELOG

xcl2done:

    rti
;*********************************************************************

j_str_maf_freq2:
    ldd     TC2
    jmp     str_maf_freq
j_str_map_freq2:
    ldd     TC2
    jmp     str_map_freq
j_str_ss1_freq2:
    jmp     str_ss1_freq
j_str_ss2_freq2:
    jmp     str_ss2_freq
;*********************************************************************
ISR_TC2:
    movb    #0x04, TFLG1 ; clear interrupt flag

    brset   flagbyte13, #FLAGBYTE13_MAF_PT2, j_str_maf_freq2
    brset   flagbyte13, #FLAGBYTE13_MAP_PT2, j_str_map_freq2
    brset   flagbyte18, #FLAGBYTE18_SS1_PT2, j_str_ss1_freq2
    brset   flagbyte18, #FLAGBYTE18_SS2_PT2, j_str_ss2_freq2

    brclr   ram4.hardware, #HARDWARE_CAM, no_ISR_cam
    bra     ISR_cam

no_ISR_cam:
; most of this next section will be redundant with angle-clock
    ldaa    vvt_intx
    anda    #0x0c          ; bits 2,3 are TC2 setbits
    beq     false_tc2

log_vvt_tc2:
    ldx     TC2
    movw    #no_vvt_tc2, 2,-sp ; put return address onto the stack
    cmpa    #0x04
    beq     str_cam2
    cmpa    #0x08
    beq     str_cam3
    cmpa    #0x0c
    beq     str_cam4
    ; fall through - not expecting to set primary one from here
    pulx     

no_vvt_tc2:
    rti

false_tc2:
   ; shouldn't be here, turn off the interrupt enable
SSEM0
   bclr    TIE, #0x04 ;  clr TIE
CSEM0
   rti

j_str_maf_freq5:
    ldd     TC5
    jmp     str_maf_freq
j_str_map_freq5:
    ldd     TC5
    jmp     str_map_freq
j_str_ss1_freq5:
    jmp     str_ss1_freq
j_str_ss2_freq5:
    jmp     str_ss2_freq
;*********************************************************************
ISR_TC5:
SSEM0
   movb    #0x20, TFLG1 ; clear interrupt flag
CSEM0

    brset   flagbyte13, #FLAGBYTE13_MAF_PT5, j_str_maf_freq5
    brset   flagbyte13, #FLAGBYTE13_MAP_PT5, j_str_map_freq5
    brset   flagbyte18, #FLAGBYTE18_SS1_PT5, j_str_ss1_freq5
    brset   flagbyte18, #FLAGBYTE18_SS2_PT5, j_str_ss2_freq5

   brclr   ram4.hardware, #HARDWARE_CAM, ISR_cam

; OEM specific wheel decoding to be added here

; most of this next section will be redundant with angle-clock
    ldaa    vvt_intx
    anda    #0x30          ; bits 4,5 are TC5 setbits
    beq     false_tc5

log_vvt_tc5:
    ldx     TC5
    movw    #no_vvt_tc5, 2,-sp ; put return address onto the stack
    cmpa    #0x10
    beq     str_cam2
    cmpa    #0x20
    beq     str_cam3
    cmpa    #0x30
    beq     str_cam4
    ; fall through - not expecting to set primary one from here
    pulx     

no_vvt_tc5:
    rti

false_tc5:
   ; shouldn't be here, turn off the interrupt enable
SSEM0
   bclr    TIE, #0x20 ;  clr TIE
CSEM0
   rti
;***********
   
ISR_cam:
; this is   IOC5 in UG mode   IOC2 when using MS3X card

; second trigger input

; The input capture setup determines when we arrive here

   brset    ram4.hardware, #HARDWARE_CAM, isc_tc2
SSEM0
   bset    TIE, #0x20  ; ensure TC5 IC still enabled
CSEM0
   bra     isc_ok
isc_tc2:
SSEM0
   bset    TIE, #0x04  ; ensure TC2 IC still enabled
CSEM0
isc_ok:
   brclr   flagbyte2,#flagbyte2_twintrig,tc5notwin
   bset    flagbyte2,#flagbyte2_twintrignow
   jmp     ISR_Ign_TimerIn   ; CAUTION! Jumping to timerin
tc5notwin:


;*************************************************************************
; Start of primary cam interrupt handler
;*************************************************************************

    leas    -5,sp       ; grab 5 bytes on the stack
; Stack :
;  edge is 0,sp (1 byte)
;  tooth_diff_this is 1,sp (4 bytes)
;

; Determine edge. 1 = active. 0 = inactive
    clr     0,sp

    brset   ram4.hardware, #HARDWARE_CAM, edge_pt2
;pt5 (JS10)
    brset   flagbyte10, #FLAGBYTE10_CAMPOL, edge_rise_tc5
    bra     edge_fall_tc5

edge_pt2: ; (MS3X cam)
    brset   flagbyte10, #FLAGBYTE10_CAMPOL, edge_rise_tc2
    bra     edge_fall_tc2

edge_fall_tc5:
    brclr   PORTT, #0x20, edgeis1 ; input pin is low
    bra     edgeis0
edge_fall_tc2:
    brclr   PORTT, #0x04, edgeis1 ; input pin is low
    bra     edgeis0

edge_rise_tc5:
    brset   PORTT, #0x20, edgeis1 ; input pin is high
    bra     edgeis0
edge_rise_tc2:
    brset   PORTT, #0x04, edgeis1 ; input pin is high
    bra     edgeis0

edgeis1:
    movb    #1, 0,sp

edgeis0:

   ; As with the crank input tach path is
   ;  * 1. if cam tach masking is enabled, we might not even have reached this ISR
   ;  * (composite tooth logger happens here)
   ;  * 2. 2nd trig noise filter
   ;  * 3. if noise filter off, polarity check
   ;  * 4. cam period filtering

   ; noise filter

; Need 32-bit TC5, so generate that ; can be TC2 if using MS3X

   movw    swtimer,TC5_32bits
   brset   ram4.hardware, #HARDWARE_CAM, nf_s_tc2
   movw    TC5,TC5_32bits+2
   bra     nf_s
nf_s_tc2:
   movw    TC2,TC5_32bits+2
nf_s:

   brclr   flagbyte1,#flagbyte1_ovfclose,ovf_check_nointyet
   ldd     TC5_32bits+2
   cmpd    #0x1000
   bhs     ovf_check_nointyet
   bra     extra_inc_TC532bits

ovf_check_nointyet:
   ldd     TC5_32bits+2
   cmpd     TC5_last
   bhs     done_ovf_tc5
   ldd     swtimer
   cmpd     swtimer_TC5_last
   bne     done_ovf_tc5

extra_inc_TC532bits:
   incw    TC5_32bits

done_ovf_tc5:
   movw    TC5_32bits+2,TC5_last
   movw    swtimer,swtimer_TC5_last

; Calculate tooth time since last cam tooth
   ldd     TC5_32bits+2
   subd    TC5_trig_firstedge+2
   ldx     TC5_32bits
   sbex    TC5_trig_firstedge

; tooth_this on stack
    stx     1,sp
    std     3,sp

; save timer for next interrupt
   movw    TC5_32bits,TC5_trig_firstedge
   movw    TC5_32bits+2,TC5_trig_firstedge+2

;composite tooth logger
; top part is common for noise filter, comp logger and double-edge cam decoding
;(IC_last is last timer on any tach input)

    ldx     TC5_32bits
    ldd     TC5_32bits+2
    subd    IC_last+2
    xgdx
    sbcb    IC_last+1
    sbca    IC_last

;D contains high word of time since last edge (pri or sec)
;X contains low word

   brset   flagbyte0, #FLAGBYTE0_COMPLOG, do_ic2complog   ; doing comp logger
   brclr   flagbyte5,FLAGBYTE5_CAM_NOISE,j_skip_nf        ; not comp log or noise filter
   bra     cl2done

j_skip_nf:
   jmp   skip_nf ; neither comp log nor polarity check - go see if we should do a polarity check

do_ic2complog:
    movw    TC5_32bits, IC_last ; save this timer time
    movw    TC5_32bits+2, IC_last+2

    ldy	    #TRIGLOGBASE
    addy	log_offset

;set RPAGE
    ldaa    RPAGE
    cpy     #0x1ffd
    bhi     cl2fin

    movb    #TRIGLOGPAGE, RPAGE

    ; ignore top byte in A
    andb	#0xf
    brclr	synch, #0x01, cl2not_sync   ; HARDCODING
    orab	#0x10

cl2not_sync:
    orab	#0x20 ; always tach2

    brclr   PTT,#1,cl2low1
    orab	#0x40   ; store pri trig pin level in 0x40

cl2low1:
    brset   ram4.hardware, #HARDWARE_CAM, cl2low1_tc2
    brclr   PTT,#0x20,cl2low2
    bra cl2low1_or
cl2low1_tc2:
    brclr   PTT,#0x04,cl2low2
cl2low1_or:
    orab	#0x80   ; store 2nd trig pin level in 0x80

cl2low2:
    stab	0,Y ; top byte of time + 4 special bits
    stx	    1,Y ; low word

;restore RPAGE
    staa    RPAGE

    ldy     log_offset

    ldab    page
    cmpb    #0xf2
    beq     cl2end1
;comp log loop mode

    movb    #TRIGLOGPAGE,RPAGE
    sty     TRIGLOGBASE + 0x3fd ; store current log_offset as pointer to next data point
    staa    RPAGE
    brclr   synch, #1, cl2fin ; no sync.. we are done

cll2inc:
    addy	#3
    sty	    log_offset
    cpy	    #0x3fc   ; three less
    bls	    cl2done
;start again
    clrw    log_offset
    bra     cl2done

cl2end1:
    addy	#3
    sty	    log_offset
    cpy	    #0x3ff
    bls	    cl2done

cl2fin:
    ; we've reached the end of the buffer, stop
    bclr	flagbyte0, #FLAGBYTE0_COMPLOG
    movb    #TRIGLOGPAGE,RPAGE
    movb	#1, TRIGLOGBASE + 0x3ff ; HARDCODED version
    staa    RPAGE
    bset    outpc.status3, #STATUS3_DONELOG

cl2done:
   ldd   outpc.rpm
   cmpd  #10 ; check over 10rpm
   blo   skip_nf
   brset flagbyte5,#FLAGBYTE5_CAM_NOISE,real_noise_filter

skip_nf:
   brclr flagbyte5,#FLAGBYTE5_CAM_POLARITY,no_noise_filter ; polarity check off

; polarity check
    tst     0,sp
    bne     no_noise_filter   ; edge==1 so ok
cpol_bail:
;input pin was wrong polarity - bail out
   bra     ign_rti

; ok, now figure out what edge we're on, and what edge we're looking for
real_noise_filter:
    tst     0,sp
    bne     triggering  ; edge==1

not_triggering:
   ; Not triggering exit the interrupt (time stored above)
   bra     ign_rti

triggering:
   ; triggering, see if the time is large enough to not be noise
    ldx     1,sp
    bne     no_noise_filter
    ldd     3,sp
    cmpd    ram4.TC5_required_width
    bhi     no_noise_filter    ;filter passes, actual length is more than required

unset_bit_rti:
   bclr    flagbyte1,#flagbyte1_trig2active
   bra     ign_rti

no_noise_filter:
   movw    lmms, ltch_lmms2 ; save lmms value for false trig detection
   movw    lmms+2, ltch_lmms2+2 ; not really needed as lmms will not change while we are in here

    ldd     false_period_cam_tix
    beq     no_false_cam   ; skip if zero

    cmpd    3,sp    ; low word of cam tooth time
    blo     no_false_cam
    bclr    flagbyte1,#flagbyte1_trig2active
    bra	    ign_rti     ; assume this is a false trigger - bail out  

no_false_cam:
    brset   flagbyte5, #FLAGBYTE5_CAM_DOUBLE, nfc_real ; trigger on both edges
    tst     0,sp
    beq     nfc2    ; edge==0
nfc_real:
; If we got here this is considered a real pulse
   bset    flagbyte1,#flagbyte1_trig2active
   inc     trig2cnt
   movw    TC_trig2_last, TC_trig2_last2
   ; X = TCvalue
   stx     TC_trig2_last
;the primary tach ISR might decide that this was actually a false pulse and cancel the false trigger time
nfc2:

;do VVT logging
;        /* to enable VVT for a mode, needs to be added to ms3_init.c, isr_ign.s and ms3_ign.c */
    ldab    spkmode
    cmpb    #4
    beq     vvt_4
    cmpb    #7
    beq     log_vvt
    cmpb    #9
    beq     vvt_9
    cmpb    #19
    beq     log_vvt
    cmpb    #20
    beq     log_vvt
    cmpb    #25
    beq     log_vvt
    cmpb    #28
    beq     vvt_28
    cmpb    #43
    beq     vvt_43
    cmpb    #46
    beq     vvt_46
    cmpb    #48
    beq     log_vvt
    cmpb    #49
    beq     vvt_49
    cmpb    #50
    beq     log_vvt
    cmpb    #55
    beq     vvt_55
    cmpb    #56
    beq     log_vvt
    cmpb    #57
    beq     log_vvt
    bra     no_vvt

vvt_4:
    ldab    vvt_decoder
    cmpb    #3 ; vvt hemi
    bne     log_vvt
    ldab    trig2cnt
    cmpb    #1 ; only use the 1st tooth for VVT
    bhi     no_vvt
    bra     log_vvt

vvt_9:
    ldab    tooth_no
    cmpb    #8      ; 1 cam tooth between 8&1, so grab it
    beq     log_vvt
    ldab    trig2cnt    ; 2 cam teeth between 4&5, only grab 2nd
    cmpb    #2
    bne     no_vvt
    bra     log_vvt

vvt_28:
vvt_43:
    ldab    trig2cnt
    cmpb    #1      ; only record 1st cam tooth in each sequence
    beq     log_vvt
    bra     no_vvt

vvt_46:
;Zetec VCT, avoid the +1 tooth.
; tooth numbers might be the same on I4, V6, but unsure at this time, so just
; coded for V8
    ldab    num_cyl
    cmpb    #8
    bne     log_vvt
    ldab    tooth_no
    cmpb    #10
    blo     log_vvt
    cmpb    #17
    bhi     log_vvt
    bra     no_vvt

vvt_49:
    ldab    tooth_no
    cmpb    #4      ; ignore first short cam tooth
    bhs     log_vvt
    bra     no_vvt

vvt_55:
    ldab    tooth_no
    cmpb    #34      ; 1 cam tooth on first phase, so grab it
    beq     log_vvt
    ldab    trig2cnt    ; 2 cam teeth between 4&5, only grab 2nd
    cmpb    #2
    bne     no_vvt
    bra     log_vvt

log_vvt:
;    bset    DDRK,#1 ; DEBUG
;    bset    PORTK,#1
    ldy     TC5_32bits
    ldx     TC5_32bits+2
    bsr     str_cam1

no_vvt:
;   bra     gol1

;-----------------

gol1:
    movw    cam_last_tooth, cam_last_tooth_1        ; save last tooth time
    movw    cam_last_tooth+2, cam_last_tooth_1+2
    movw    1,sp, cam_last_tooth        ; save tooth time
    movw    3,sp, cam_last_tooth+2
    
;   jmp     L1 ; goes to rti

L1:
   ldd     false_mask_cam
   beq     ign_no_false
; cam sensor false trigger protection
   addd	   ltch_lmms2+0x2
   ldx     ltch_lmms2
   bcc	   L1carry
   inx
L1carry:
   stx     t_enable_IC2
   std     t_enable_IC2+0x2

   brset   ram4.hardware, #HARDWARE_CAM, L1_tc2
SSEM0
   bclr    TIE, #0x20
CSEM0
   bra     L1_tc_ok
L1_tc2:
SSEM0
   bclr    TIE, #0x04
CSEM0
L1_tc_ok:
   movb	   TFLG_trig2, TFLG1 ; clear any pending interrups
   bra	   ign_rti

ign_no_false:
    movw	#0xffff, t_enable_IC2+0x2
    movw	#0xffff, t_enable_IC2
    brset   ram4.hardware, #HARDWARE_CAM, ignf_tc2
SSEM0
    bset    TIE, #0x20 ; ensure it is enabled
CSEM0
    bra     ign_rti
ignf_tc2:
SSEM0
    bset    TIE, #0x04 ; ensure it is enabled
CSEM0

ign_rti:

; unstack any local variables here
    leas    5,sp       ; de-allocate stack

   rti

; composite logger moved to here from ign_in.c due to gcc mess up

do_complog_pri:
; function call does this, we arrive here with this data
;    ldx     TC0_32bits 
;    ldd     TC0_32bits+2
    subd    IC_last+2
    xgdx
    sbcb    IC_last+1
;    sbca    IC_last not needed as we ignore A

;D contains high word of time since last edge (pri or sec)
;X contains low word
;    movw    TC0_32bits, IC_last ; save this timer time ; do this in the C code
;    movw    TC0_32bits+2, IC_last+2
do_complog_pri_b:

    ldy	    #TRIGLOGBASE
    addy	log_offset

;swap page in ram window
    ldaa    RPAGE
    cpy     #0x1ffd
    bhi     cl1fin

    movb    #TRIGLOGPAGE, RPAGE

    ; ignore top byte that was in A
    andb	#0xf
    brclr	synch, #0x01, cl1not_sync   ; HARDCODING
    orab	#0x10

cl1not_sync:
    brclr   flagbyte2, #flagbyte2_twintrignow, cl1ns2
    orab	#0x20 ; if tach2 from twin trigger?

cl1ns2:
    brclr   PTT,#1,cl1low1
    orab	#0x40   ; store pri trig pin level in 0x40

cl1low1:
    brset   ram4.hardware, #HARDWARE_CAM, cl1low1_tc2
    brclr   PTT,#0x20,cl1low2
    bra cl1low1_or
cl1low1_tc2:
    brclr   PTT,#0x04,cl1low2
cl1low1_or:
    orab	#0x80   ; store 2nd trig pin level in 0x80

cl1low2:
    stab	0,Y ; top byte of time + 4 special bits
    stx	    1,Y ; low word

;restore RPAGE
    staa    RPAGE
    ldy     log_offset

    ldab    page
    cmpb    #0xf2
    beq     cl1end1
;comp log loop mode
    movb    #0xf0, RPAGE
    sty     TRIGLOGBASE + 0x3fd ; store current log_offset as pointer to next data point
    staa    RPAGE
    brclr   synch, #1, cl1fin  ; no sync.. we are done

cll1inc:
    addy	#3
    sty	    log_offset
    cpy	    #0x3fc   ; three less
    bls	    cl1done
;start again
    clrw    log_offset
    bra     cl1done

cl1end1:
    addy	#3
    sty	    log_offset
    cpy	    #0x3ff
    bls	    cl1done

cl1fin:
    ; we've reached the end of the buffer, stop
    bclr	flagbyte0, #FLAGBYTE0_COMPLOG
    movb    #TRIGLOGPAGE, RPAGE
    movb	#1, TRIGLOGBASE + 0x3ff ; HARDCODED version
    staa    RPAGE
    bset    outpc.status3, #STATUS3_DONELOG

cl1done:
    rts

j_str_maf_freq4:
    ldd     TC4
    jmp     str_maf_freq
j_str_map_freq4:
    ldd     TC4
    jmp     str_map_freq
j_str_ss1_freq4:
    jmp     str_ss1_freq
j_str_ss2_freq4:
    jmp     str_ss2_freq
;------------------------------------------------------------------------
ISR_TC4:
    movb    #0x10, TFLG1 ; clear interrupt flag

    brset   flagbyte13, #FLAGBYTE13_MAF_PT4, j_str_maf_freq4
    brset   flagbyte13, #FLAGBYTE13_MAP_PT4, j_str_map_freq4
    brset   flagbyte18, #FLAGBYTE18_SS1_PT4, j_str_ss1_freq4
    brset   flagbyte18, #FLAGBYTE18_SS2_PT4, j_str_ss2_freq4

; most of this next section will be redundant with angle-clock
    ldaa    vvt_intx
    anda    #3          ; bits 0,1 are TC4 setbits
    beq     false_tc4

log_vvt_tc4:
    ldx     TC4
    movw    #no_vvt_tc4, 2,-sp ; put return address onto the stack
    cmpa    #1
    beq     str_cam2
    cmpa    #2
    beq     str_cam3
    cmpa    #3
    beq     str_cam4
    ; fall through - not expecting to set primary one from here
    pulx     

no_vvt_tc4:
    rti

false_tc4:
   ; shouldn't be here, turn off the interrupt enable
SSEM0
   bclr    TIE, #0x10 ;  clr TIE
CSEM0
   rti

j_str_maf_freq6:
    ldd TC6
    jmp str_maf_freq
j_str_map_freq6:
    ldd TC6
    jmp str_map_freq
j_str_ss1_freq6:
    jmp     str_ss1_freq
j_str_ss2_freq6:
    jmp     str_ss2_freq
;------------------------------------------------------------------------
ISR_TC6:
    movb    #0x40, TFLG1 ; clear interrupt flag

    brset   flagbyte13, #FLAGBYTE13_MAF_PT6, j_str_maf_freq6
    brset   flagbyte13, #FLAGBYTE13_MAP_PT6, j_str_map_freq6
    brset   flagbyte18, #FLAGBYTE18_SS1_PT6, j_str_ss1_freq6
    brset   flagbyte18, #FLAGBYTE18_SS2_PT6, j_str_ss2_freq6

; most of this next section will be redundant with angle-clock
    ldaa    vvt_intx
    anda    #0xc0          ; bits 4,5 are TC6 setbits
    beq     false_tc6

log_vvt_tc6:
    ldx     TC6
    movw    #no_vvt_tc6, 2,-sp ; put return address onto the stack
    cmpa    #0x40
    beq     str_cam2
    cmpa    #0x80
    beq     str_cam3
    cmpa    #0xc0
    beq     str_cam4
    ; fall through - not expecting to set primary one from here
    pulx     

no_vvt_tc6:
    rti

false_tc6:
   ; shouldn't be here, turn off the interrupt enable
SSEM0
   bclr    TIE, #0x40 ;  clr TIE
CSEM0
   rti
;------------------------------------------------------------------------

str_cam1:
    brclr   flagbyte12, #FLAGBYTE12_CAM1ARM, str_ret
    bclr    flagbyte12, #FLAGBYTE12_CAM1ARM
    movb    tooth_no, cam_tooth ; save the last tooth we saw before this cam input cam_tooth[1]
    movb    trig2cnt, cam_trig
    subx    TC_crank+2
    stx     cam_time+2  ; cam_time[0] number of ticks since that tooth. Convert to angle in mainloop
    sbey    TC_crank  ; only handle high word for cam 1
    sty     cam_time
    bset    flagbyte11,#FLAGBYTE11_CAM1
str_ret:
    rts

str_cam2:
;decode here if needed
    inc     trig3cnt
    ldab    spkmode
    cmpb    #43
    beq     str_cam2_43
    bra     str_cam2a
str_cam2_43:
    ldab    trig3cnt
    cmpb    #1
    beq     str_cam2a
    bra     str_cam2b

str_cam2a:
    brclr   flagbyte12, #FLAGBYTE12_CAM2ARM, str_ret
    bclr    flagbyte12, #FLAGBYTE12_CAM2ARM
    movb    tooth_no, cam_tooth+1 ; save the last tooth we saw before this cam input cam_tooth[1]
    movb    trig3cnt, cam_trig+1
    subx    TC_crank+2
    stx     cam_time+6  ; cam_time[1] number of ticks since that tooth. Convert to angle in mainloop
    clrw    cam_time+4
    bset    flagbyte11,#FLAGBYTE11_CAM2
str_cam2b:
    rts

str_cam3:
    inc     trig4cnt
    brclr   flagbyte12, #FLAGBYTE12_CAM3ARM, str_ret
    bclr    flagbyte12, #FLAGBYTE12_CAM3ARM
    movb    tooth_no, cam_tooth+2 ; save the last tooth we saw before this cam input cam_tooth[2]
    movb    trig4cnt, cam_trig+2
    subx    TC_crank+2
    stx     cam_time+10  ; cam_time[2] number of ticks since that tooth. Convert to angle in mainloop
    clrw    cam_time+8
    bset    flagbyte11,#FLAGBYTE11_CAM3
    rts

str_cam4:
    inc     trig5cnt
    brclr   flagbyte12, #FLAGBYTE12_CAM4ARM, str_ret
    bclr    flagbyte12, #FLAGBYTE12_CAM4ARM
    movb    tooth_no, cam_tooth+3 ; save the last tooth we saw before this cam input cam_tooth[3]
    movb    trig5cnt, cam_trig+3
    subx    TC_crank+2
    stx     cam_time+14  ; cam_time[3] number of ticks since that tooth. Convert to angle in mainloop
    clrw    cam_time+12
    bset    flagbyte11,#FLAGBYTE11_CAM4
    rts

;---------------------------
str_maf_freq:
    brset   flagbyte12, #FLAGBYTE12_MAF_FSLOW, smf_slow
    tfr     d,x
    subd    maf_tim_l+2
    std     mafperiod_accum
    stx     maf_tim_l+2
    rti

smf_slow:
   ldx     swtimer

   brclr   flagbyte1,#flagbyte1_ovfclose,smf_s2
   cmpd    #0x1000
   bhs     smf_s2
   bra     smf_s3

smf_s2:
   cmpd    maf_tim_l+2
   bhs     smf_s4
   cmpx    maf_tim_l
   bne     smf_s4

smf_s3:
   inx

smf_s4:
    ; here X:D contain 32bit timer
    pshd
    pshx
    subd    maf_tim_l+2
    sbex    maf_tim_l
    ; divide by 4 to allow down to ~4Hz
    lsrx
    rora
    rorb
    lsrx
    rora
    rorb
    std     mafperiod_accum
    ; pull saved values back off stack and store
    movw    2,sp+, maf_tim_l
    movw    2,sp+, maf_tim_l+2
    rti

;---------------------------
str_map_freq:
    ; enter with captured timer in D
    brset   flagbyte12, #FLAGBYTE12_MAP_FSLOW, smp_slow
    tfr     d,x
    subd    map_tim_l+2
    std     mapperiod_accum
    stx     map_tim_l+2
    rti

smp_slow:
   ldx     swtimer

   brclr   flagbyte1,#flagbyte1_ovfclose,smp_s2
   cmpd    #0x1000
   bhs     smp_s2
   bra     smp_s3

smp_s2:
   cmpd    map_tim_l+2
   bhs     smp_s4
   cmpx    map_tim_l
   bne     smp_s4

smp_s3:
   inx

smp_s4:
    ; here X:D contain 32bit timer
    pshd
    pshx
    subd    map_tim_l+2
    sbex    map_tim_l
    ; divide by 4 to allow down to ~4Hz
    lsrx
    rora
    rorb
    lsrx
    rora
    rorb
    std     mapperiod_accum
    ; pull saved values back off stack and store
    movw    2,sp+, map_tim_l
    movw    2,sp+, map_tim_l+2
    rti
;---------------------------
str_ss1_freq:
   ldd     pit_16bits
   subd    ss1_last
; D may well be zero at high speeds on some teeth, but the sum over many teeth is correct
    incw    ss1_teeth
    std     ss1_time
    addd    ss1_time_sum
    std     ss1_time_sum
    cmpd    #2000   ; 20ms
    blo     idone_record3
    bset    flagbyte8, #FLAGBYTE8_SAMPLE_SS1   ; reached at least Xms of data
idone_record3:
   movw    pit_16bits,ss1_last
   clrw    ss1_stall

    rti

;---------------------------
str_ss2_freq:
   ldd     pit_16bits
   subd    ss2_last
; D may well be zero at high speeds on some teeth, but the sum over many teeth is correct
    incw    ss2_teeth
    std     ss2_time
    addd    ss2_time_sum
    std     ss2_time_sum
    cmpd    #2000   ; 20ms
    blo     idone_record4
    bset    flagbyte8, #FLAGBYTE8_SAMPLE_SS2   ; reached at least Xms of data
idone_record4:
   movw    pit_16bits,ss2_last
   clrw    ss2_stall
    rti

