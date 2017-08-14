;*********************************************************************
; ISR_TimerOverflow
;*********************************************************************
; $Id: isr_timerovf.s,v 1.8 2012/05/25 14:13:46 jsmcortina Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; * Origin: James Murray / Kenneth Culver
; * Majority: James Murray / Kenneth Culver
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

.sect .text
.globl ISR_TimerOverflow, ISR_TIMTimerOverflow
             nolist               ;turn off listing
             include "ms3h.inc"
             list                 ;turn listing back on
;*********************************************************************

ISR_TimerOverflow:
;  acknowledge interrupt and clear flag
   movb	#128, TFLG2

   incw    swtimer  ; // top word to make 32 bit timer. software:hardware
   bclr   flagbyte1, #flagbyte1_ovfclose

   rti


ISR_TIMTimerOverflow:
;  acknowledge interrupt and clear flag
   movb	#128, TIMTFLG2

   incw    swtimer_inj
   bclr    flagbyte6, #FLAGBYTE6_INJ_OVFCLOSE

   rti
