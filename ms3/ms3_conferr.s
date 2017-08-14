;*********************************************************************
; config error
;*********************************************************************
; $Id: ms3_conferr.s,v 1.145.4.1 2014/12/25 01:07:22 jsmcortina Exp $
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

.sect .textf4
.globl configerror

             nolist               ;turn off listing
             include "ms3h.inc"
             list                 ;turn listing back on

;kill just about everything and send config error down serial port,
; then sit in loop turning fuel pump on and off
;*********************************************************************
; on entry A contains the config error
;*********************************************************************
configerror:

            bset        outpc.status1, #STATUS1_CONFERR;
            sei                        ; disable ints until we've sent the message
            movb        #0x00,SCI0CR1
            movb        #0x08,SCI0CR2  ; disable SCI ints, enable tx
                        ldx                     #20
                        bsr                     delaylp1                ; short delay to ensure everything is ready
;send CR,NL as pre-amble to avoid missed first bytes
            movb        #0x13, GPAGE    ; global address
            ldy         #0xf700         ; of conf error buffer
; set that space to zeros
            clra
            clrb
ce_zl1:
            gstaa       1,y+
            dbne        b, ce_zl1

            ldy         #0xf700
            ldx         #cr
            bsr         sendstr        ; uses X

            ldy         #0xf700         ; forget that CR though

            ldab        conf_err
            decb
            clra
            tfr         d,x
            lslx
            ldx         errtbl,x

            bsr         sendstr        ; uses X

            ldaa        conf_err
            cmpa        #43            ; special message for ports clash
            bne         conf_norm
            ldab        #'P'
            bsr         sendchar
            ldab        conf_err_port
            sex         b,x
            ldab        ce_port,x
            bsr         sendchar
            ldab        conf_err_pin
            addb        #48  ; ascii '0'
            bsr         sendchar
            ldx         #cr
            bsr         sendstr
                        
conf_norm:
            ldab        conf_err_feat
            beq         conf_f_done

            ldx         #who
            bsr         sendstr

            clra
            tfr         d,x
            lslx
            ldx         rtbl,x
            bsr         sendstr        ; uses X

conf_f_done:

            clra
            gstaa       0,y         ; record terminating zero in conf error buffer

            movb        #0x00,SCI0CR1
            movb        #0x24,SCI0CR2  ; set back to normal

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SCI1 ?
            ldx         0xfefe
            cpx         #0x380
            blo         ce_rpm
            cpx         #0x3ff
            bhi         ce_rpm
; YES... now send it to SCI1 as well

            movb        #0x00,SCI1CR1
            movb        #0x08,SCI1CR2  ; disable SCI ints, enable tx
;send CR,NL as pre-amble to avoid missed first bytes
            ldx         #cr
            bsr         sendstr1        ; uses X

            ldab        conf_err
            decb
            clra
            tfr         d,x
            lslx
            ldx         errtbl,x

            bsr         sendstr1        ; uses X

            ldaa        conf_err
            cmpa        #43            ; special message for ports clash
            bne         conf_norm1
            ldab        #'P'
            bsr         sendchar1
            ldab        conf_err_port
            sex         b,x
            ldab        ce_port,x
            bsr         sendchar1
            ldab        conf_err_pin
            addb        #48  ; ascii '0'
            bsr         sendchar1
            ldx         #cr
            bsr         sendstr1
                        
conf_norm1:
            ldab        conf_err_feat
            beq         conf_f_done1

            ldx         #who
            bsr         sendstr1

            clra
            tfr         d,x
            lslx
            ldx         rtbl,x
            bsr         sendstr1        ; uses X

conf_f_done1:

            movb        #0x00,SCI1CR1
            movb        #0x24,SCI1CR2  ; set back to normal

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ce_rpm:
            cli
;fake rpm with conf_err in it
            clra
            addd        #65000   ; rpm = 65000 + conf_err
            std         outpc.rpm

            leas        -1,sp
            movb        #3, 0,sp
celoop:
; pulse fuel pump
            call fuelpump_prime
            bsr         delay       ; on for ~1s
            call fuelpump_off
            bsr         delay       ; off for ~3s
            bsr         delay
            bsr         delay
            dec         0,sp
            bne         celoop
            leas        +1,sp
            ldaa        conf_err
            cmpa        #190
            blo         dead_loop
            clr         conf_err
            bclr        outpc.status1, #STATUS1_CONFERR
            rtc                     ; if a soft config error then we _can_ return
dead_loop:
            bsr         delay
            bra         dead_loop

;*********************************************************************
;sendstr:      ; send text string down serial port
; on entry X points to start of zero terminated string
;*********************************************************************
sendstr:
            ldaa        SCI0SR1    ; ensure stat reg read as part of sequence
sendstrlp:
            ldaa        1,x+
            beq         ssdone
            staa        SCI0DRL
            gstaa       1,y+         ; record to conf error buffer
;            iny
            cmpy        #0xf800     ; prevent overflow
            blo         sswait
            ldy         #0xf700     ; start at beginning again
sswait:
            brset       SCI0SR1,#0x80,sendstrlp ; wait for byte to be sent
            movb        #0x55,ARMCOP      ; keep COP happy
            movb        #0xAA,ARMCOP
            bra         sswait
ssdone:
            rts

;*********************************************************************
;sendchar:      ; send charactor in B down serial port
;*********************************************************************
sendchar:
            ldaa        SCI0SR1    ; ensure stat reg read as part of sequence
            stab        SCI0DRL
scwait:
            brset       SCI0SR1,#0x80,scdone ; bail if done
            movb        #0x55,ARMCOP      ; keep COP happy
            movb        #0xAA,ARMCOP
            bra         scwait
scdone:
            rts
;*********************************************************************
;sendstr:      ; send text string down serial port
; on entry X points to start of zero terminated string
;*********************************************************************
sendstr1:
            ldaa        SCI1SR1    ; ensure stat reg read as part of sequence
sendstrlp1:
            ldaa        1,x+
            beq         ssdone1
            staa        SCI1DRL
;(message already in buffer from SCI0 pass)
sswait1:
            brset       SCI1SR1,#0x80,sendstrlp1 ; wait for byte to be sent
            movb        #0x55,ARMCOP      ; keep COP happy
            movb        #0xAA,ARMCOP
            bra         sswait1
ssdone1:
            rts

;*********************************************************************
;sendchar1:      ; send charactor in B down serial port
;*********************************************************************
sendchar1:
            ldaa        SCI1SR1    ; ensure stat reg read as part of sequence
            stab        SCI1DRL
scwait1:
            brset       SCI1SR1,#0x80,scdone1 ; bail if done
            movb        #0x55,ARMCOP      ; keep COP happy
            movb        #0xAA,ARMCOP
            bra         scwait1
scdone1:
            rts
;*********************************************************************
;delay:      ; busy wait
;*********************************************************************
delay:
            ldx         #0
delaylp1:
            ldy         #32
delaylp2:
            movb        #0x55,ARMCOP      ; keep COP happy
            movb        #0xAA,ARMCOP
            nop
            nop
            nop
            nop
            dbne        y,delaylp2
            pshx
            pshy
            call        serial       ; be prepared to send realtime data
            call        dribble_burn

            puly
            pulx
            dbne        x,delaylp1
            rts

;*********************************************************************
errtbl:
            fdb         err1
            fdb         err2
            fdb         err3
            fdb         err4
            fdb         err5
            fdb         err6
            fdb         err7
            fdb         err8
            fdb         err9
            fdb         err10
            fdb         err11
            fdb         err12
            fdb         err13
            fdb         err14
            fdb         err15
            fdb         err16
            fdb         err17
            fdb         err18
            fdb         err19
            fdb         err20
            fdb         err21
            fdb         err22
            fdb         err23
            fdb         err24
            fdb         err25
            fdb         err26
            fdb         err27
            fdb         err28
            fdb         err29
            fdb         err30
            fdb         err31
            fdb         err32
            fdb         err33
            fdb         err34
            fdb         err35
            fdb         err36
            fdb         err37
            fdb         err38
            fdb         err39
            fdb         err40
            fdb         err41
            fdb         err42
            fdb         err43
            fdb         err44
            fdb         err45
            fdb         err46
            fdb         err47
            fdb         err48
            fdb         err49
            fdb         err50
            fdb         err51
            fdb         err52
            fdb         err53
            fdb         err54
            fdb         err55
            fdb         err56
            fdb         err57
            fdb         err58
            fdb         err59
            fdb         err60
            fdb         err61
            fdb         err62
            fdb         err63
            fdb         err64
            fdb         err65
            fdb         err66
            fdb         err67
            fdb         err68
            fdb         err69
            fdb         err70
            fdb         err71
            fdb         err72
            fdb         err73
            fdb         err74
            fdb         err75
            fdb         err76
            fdb         err77
            fdb         err78
            fdb         err79
            fdb         err80
            fdb         err81
            fdb         err82
            fdb         err83
            fdb         err84
            fdb         err85
            fdb         err86
            fdb         err87
            fdb         err88
            fdb         err89
            fdb         err90
            fdb         err91
            fdb         err92
            fdb         err93
            fdb         err94
            fdb         err95
            fdb         err96
            fdb         err97
            fdb         err98
            fdb         err99
            fdb         err100
            fdb         err101
            fdb         err102
            fdb         err103
            fdb         err104
            fdb         err105
            fdb         err106
            fdb         err107
            fdb         err108
            fdb         err109
            fdb         err110
            fdb         err111
            fdb         err112
            fdb         err113
            fdb         err114
            fdb         err115
            fdb         err116
            fdb         err117
            fdb         err118
            fdb         err119
            fdb         err120
            fdb         err121
            fdb         err122
            fdb         err123
            fdb         err124
            fdb         err125
            fdb         err126
            fdb         err127
            fdb         err128
            fdb         err129
            fdb         err130
            fdb         err131
            fdb         err132
            fdb         err133
            fdb         err134
            fdb         err135
            fdb         err136
            fdb         err137
            fdb         err138
            fdb         err139
            fdb         err140
            fdb         err141
            fdb         err142
            fdb         err143
            fdb         err144
            fdb         err145
            fdb         err146
            fdb         err147
            fdb         err148
            fdb         err149
            fdb         err150
            fdb         err151
            fdb         err152
            fdb         err153
            fdb         err154
            fdb         err155
            fdb         err156
            fdb         err157
            fdb         err158
            fdb         err159
            fdb         err160
            fdb         err161
            fdb         err162
            fdb         err163
            fdb         err164
            fdb         err165
            fdb         err166
            fdb         err167
            fdb         err168
            fdb         err169
            fdb         err170
            fdb         err171
            fdb         err172
            fdb         err173
            fdb         err174
            fdb         err175
            fdb         err176
            fdb         err177
            fdb         err178
            fdb         err179
            fdb         err180
            fdb         err181
            fdb         err182
            fdb         err183
            fdb         err184
            fdb         err185
            fdb         err186
            fdb         err187
            fdb         err188
            fdb         err189
            fdb         err190
            fdb         err191
            fdb         err192
            fdb         err193
            fdb         err194
            fdb         err195
            fdb         err196
            fdb         err197
            fdb         err198
            fdb         err199

cr:         fcb         0x0d,0x0a,0
err1:       fcc         "Number of teeth/cylinders conflict\r\0"
err2:       fcc         "Conflict on realtime baro input pin\r\0"
err3:       fcc         "Conflict on 2nd ego input pin\r\0"
err4:       fcc         "Conflict on knock sensor input pin\r\0"
err5:       fcc         "Conflict on flex fuel input pin\r\0"
err6:       fcc         "Conflict with Tacho Output\r\0"
err7:       fcc         "Conflict with Launch input\r\0"
err8:       fcc         "7, 9, 11, 13+ cyl not supported\r\0"
err9:       fcc         "Trigger return needs larger trigger angle e.g. 60BTDC\r\0"
err10:      fcc         "Staged injection primary or secondary injector size zero\r\0"
err11:      fcc         "Staged triggers cannot be the same\r\0"
err12:      fcc         "Staged trigger param 1 or 2 zero\r\0"
err13:          fcc         "Second parameter enabled but with zero parameter\r\0"
err14:      fcc         "Staged transition enabled with zero events\r\0"
err15:      fcc         "Staged injection can only be used with simultaneous\r\0"
err16:      fcc         "Dual table and secondary load cannot be enabled together\r\0"
err17:      fcc         "Spark mode requires 4 cylinders\r\0"
err18:      fcc         "4G63 requires 4 cylinders\r\0"
err19:      fcc         "6G72 requires 6 cylinders\r\0"
err20:      fcc         "Daihatsu 3 cyl requires 3 cylinders\r\0"
err21:      fcc         "Daihatsu 4 cyl requires 4 cylinders\r\0"
err22:      fcc         "Vitara 2.0 requires 4 cylinders\r\0"
err23:      fcc         "Suzuki Swift requires 4 cylinders\r\0"
err24:      fcc         "IAW Weber Marelli requires 4 cylinders\r\0"
err25:      fcc         "Failed to calculate trigger teeth, check settings\r\0"
err26:      fcc         "Staging and Dual Table cannot be used together\r\0"
err27:      fcc         "Conflict with boost control pins\r\0"
err28:      fcc         "Conflict with nitrous pins\r\0"
err29:      fcc         "Twin Trigger mode is designed for bike type ignition - invalid settings\r\0"
err30:      fcc         "Only two spark outputs allowed on Microsquirt\r\0"
err31:          fcc                     "Dizzy mode with negative angle not allowed\r\0"
err32:      fcc         "COP set, but second trigger resets every crank rotation. Try wasted spark or change your 2nd trigger hardware to every cam rotation\r\0"
err33:      fcc         "Oddfire not allowed in this ignition mode\r\0"
err34:      fcc         "VTR1000 mode requires 2 cyl and oddfire and wasted spark\r\0"
err35:      fcc         "Spark mode requires 4cyl and single coil\r\0"
err36:      fcc             "Nitrous input conflict\r\0"
err37:      fcc             "Too many spark outputs required with your settings\r\0"
err38:      fcc         "COP set, but crank speed trigger wheel. Try wasted spark, install a cam trigger or mount your wheel on the cam\r\0"
err39:      fcc         "Conflict with nitrous stage 1 pins\r\0"
err40:      fcc         "Conflict with nitrous stage 2 pins\r\0"
err41:      fcc         "Oddfire required\r\0"
err42:      fcc         "EDIS cam input and spark output on same pin JS10\r\0"
err43:      fcc         "Conflict on programmable on/off outputs pin \0" ; this is a special message
err44:      fcc         "Rotary or 2-stroke do not need cam speed reset pulse - check wheel decoder settings\r\0"
err45:      fcc         "Conflict with JS10 / cam input\r\0"
err46:      fcc         "Logging LED conflict\r\0"
err47:      fcc         "Logging button conflict\r\0"
err48:      fcc         "Cam input required to run sequential with EDIS\r\0"
err49:      fcc         "SERIOUS ERROR - configuration flash in erased state! Try reloading all settings or reflashing firmware without preserving settings.\r\0"
err50:      fcc         "SERIOUS ERROR - flash controller busy! New CPU may be required?!\r\0"
err51:      fcc         "Logging stream conflict\r\0"
err52:      fcc         "Fuel table switch conflict\r\0"
err53:      fcc         "Spark table switch conflict\r\0"
err54:      fcc         "Boost table switch conflict\r\0"
err55:      fcc         "Variable launch conflict\r\0"
err56:      fcc         "3-Step conflict\r\0"
err57:      fcc         "MaxAFR output conflict\r\0"
err58:      fcc         "VSS1 conflict\r\0"
err59:      fcc         "VSS2 conflict\r\0"
err60:      fcc         "Shaft Speed 1 conflict\r\0"
err61:      fcc         "Shaft Speed 2 conflict\r\0"
err62:      fcc         "ReqFuel switch pin conflict\r\0"
err63:      fcc         "AFR table switch pin conflict\r\0"
err64:      fcc         "Stoich switch pin conflict\r\0"
err65:      fcc         "Water inj pump pin conflict\r\0"
err66:      fcc         "Water inj valve pin conflict\r\0"
err67:      fcc         "Battery voltage too low\r\0"
err68:      fcc         "WRONG CODE LOADED? Or unsupported/invalid/corrupted monitor.\r\0"
err69:      fcc         "MAF pin/port conflict\r\0"
err70:      fcc         "MAFMAP enabled when MAF is not\r\0"
err71:      fcc         "'Toothed wheel' required for wasted spark.\r\0"
err72:      fcc         "More 'squirts' required\r\0"
err73:      fcc         "AC pin/port conflict\r\0"
err74:      fcc         "Fan control pin/port conflict\r\0"
err75:      fcc         "Too many triggers\r\0"
err76:      fcc         "Accelerometer input conflict\r\0"
err77:      fcc         "Gear sensor input conflict\r\0"
err78:      fcc         "VSS out conflict\r\0"
err79:      fcc         "LS1 requires even-fire V8\r\0"
err80:      fcc         "YZF1000 only supported on even-fire 4cyl\r\0"
err81:      fcc         "Conflict on shifter input\r\0"
err82:      fcc         "Conflict on shifter output\r\0"
err83:      fcc         "Conflict on one of the Generic PWM outputs\r\0"
err84:      fcc         "Conflict on Dual Fuel input pin\r\0"
err85:      fcc         "SERIOUS ERROR - flash/ram verification failed 8\r\0"
err86:      fcc         "SERIOUS ERROR - flash/ram verification failed 9\r\0"
err87:      fcc         "SERIOUS ERROR - flash/ram verification failed 10\r\0"
err88:      fcc         "SERIOUS ERROR - flash/ram verification failed 11\r\0"
err89:      fcc         "SERIOUS ERROR - flash/ram verification failed 12\r\0"
err90:      fcc         "SERIOUS ERROR - flash/ram verification failed 13\r\0"
err91:      fcc         "SERIOUS ERROR - flash/ram verification failed 18\r\0"
err92:      fcc         "SERIOUS ERROR - flash/ram verification failed 19\r\0"
err93:      fcc         "SERIOUS ERROR - flash/ram verification failed 20\r\0"
err94:      fcc         "SERIOUS ERROR - flash/ram verification failed 21\r\0"
err95:      fcc         "SERIOUS ERROR - flash/ram verification failed 22\r\0"
err96:      fcc         "SERIOUS ERROR - flash/ram verification failed 23\r\0"
err97:      fcc         "SERIOUS ERROR - flash/ram verification failed 4\r\0"
err98:      fcc         "SERIOUS ERROR - flash/ram verification failed 5\r\0"
err99:      fcc         "SERIOUS ERROR - skip pulses set to invalid count\r\0"
err100:     fcc         "Overboost switch conflict\r\0"
err101:     fcc         "Cannot use trim with V3 outputs\r\0" ; not used
err102:     fcc         "Invalid combination of semi-seq and num squirts/alternating\r\0"
err103:     fcc         "Cannot run sequential without knowing engine phase (e.g. a cam sensor)\r\0"
.ifdef MS3PRO
err104:     fcc         "Cannot run sequential on InjI/J outputs for this many cylinders\r\0"
.else
err104:     fcc         "Cannot run sequential on V3 outputs for this many cylinders\r\0"
.endif
.ifdef MS3PRO
err105:     fcc         "Cannot run semi-sequential on InjI/J outputs for this many cylinders\r\0"
.else
err105:     fcc         "Cannot run semi-sequential on V3 outputs for this many cylinders\r\0"
.endif
err106:     fcc         "Water inj input pin conflict\r\0"
err107:     fcc         "Only single coil or COP allowed with this many cylinders\r\0"
err108:     fcc         "Only 'single coil' allowed in this spark mode (EDIS or Basic Trigger)\r\0"
err109:     fcc         "This combination of semi-sequential, staging, dual fuel options is not supported presently.\r\0"
err110:     fcc         "Your MS3 needs a monitor upgrade to run 4x pulsewidths\r\0"
err111:     fcc         "Fan Control on temperature lower than off temperature\r\0"
.ifdef MS3PRO
err112:     fcc         "Off or Semi-sequential does not make sense on rotary or 2-stroke - use full sequential\r\0."
.else
err112:     fcc         "Off or Semi-sequential does not make sense on rotary or 2-stroke - use full sequential with MS3X outputs\r\0"
.endif
err113:     fcc         "Anti-lag input pin conflict\r\0"
err114:     fcc         "Anti-lag output pin conflict\r\0"
err115:     fcc         "Anti-lag PWM output pin conflict\r\0"
err116:     fcc         "Boost 2 output pin conflict\r\0"
err117:     fcc         "EGT input pin conflict\r\0"
err118:     fcc         "Generic Sensor input pin conflict\r\0"
err119:     fcc         "VVT output pin conflict\r\0"
err120:     fcc         "VVT is not supported on this spark/wheel mode\r\0"
err121:     fcc         "You must run Alternating and Alternating cranking for more than 8 cylinders non-sequential\r\0"
err122:     fcc         "More than 2 rotors only supported with MS3X outputs\r\0"
err123:     fcc         "Conflict on VVT cam sensor 2/3/4 input\r\0"
err124:     fcc         "Conflict on Real Time Clock pins\r\0"
err125:     fcc         "Conflict on Torque convertor lockup Input/outputs\r\0"
err126:     fcc         "Conflict on Traction control input\r\0"
err127:     fcc         "Conflict on realtime baro input pin\r\0"
err128:     fcc         "Toyota DLI only permitted on 4 or 6cyl even fire with wasted spark\r\0"
err129:     fcc         "Invalid MAP sampling angles. Needs to be less than 180deg for a 4cyl or 90deg for a V8 etc.\r\0"
err130:     fcc         "Conflict on MAP input.\r\0"
err131:     fcc         "Conflict on MAF frequency input.\r\0"
err132:     fcc         "Conflict on knock sensor SPI pins\r\0"
err133:     fcc         "Cannot run knock control per-cylinder unless running C.O.P. or sequential capable crank/cam triggers.\r\0"  
err134:     fcc         "Internal knock sensor not supported in this firmware\r\0"
err135:     fcc         "Fault with internal knock sensor hardware - is it installed?\r\0"
err136:     fcc         "TPS calibrated backwards\r\0"
err137:     fcc         "Internal SPI RAM/Wideband pins busy\r\0"
err138:     fcc         "Conflict on 2nd MAP input.\r\0"
err139:     fcc         "Conflict on Long Term Trim button\r\0"
err140:     fcc         "Staging and dual fuel using same outputs.\r\0"
err141:     fcc         "Too many teeth!\r\0"
.ifdef MS3PRO
err142:     fcc         "ALS fuel cut only allowed with sequential\r\0"
.else
err142:     fcc         "ALS fuel cut only allowed with MS3X sequential\r\0"
.endif
err143:     fcc         "Trigger angle/offset only allowed maximum +/-20 deg\r\0"
err144:     fcc         "Conflict on launch timed in/out\r\0"
err145:     fcc         "Conflict on throttle stop output\r\0"
err146:     fcc         "MAF or MAFMAP enabled. You need to enable a MAF sensor port!\r\0"
.ifdef MS3PRO
err147:     fcc         "PWM group A : High Current 1/2, InjI/J must use same frequency.\r\0"
.else
err147:     fcc         "PWM group A : Nitrous1/2, Inj1/2 must use same frequency.\r\0"
.endif
.ifdef MS3PRO
err148:     fcc         "PWM group B : High Current Out 3, PWM Out 2, PWM / Idle Out 1, PWM Out 3 must use same frequency.\r\0"
.else
err148:     fcc         "PWM group B : Boost, Idle, FIDLE, VVT must use same frequency.\r\0"
.endif
err149:     fcc         "Custom oddfire angles must add up to 720.0 degrees.\r\0"
err150:     fcc         "Twin trigger does not support sequential.\r\0"
err151:     fcc         "Conflict on Check Engine Light output\r\0"
err152:     fcc         "You have set your 'Cam input' to 'MAP sensor'. Did you mean to?\rRead the tooltip and change the 'Cam input' setting if you don't want MAP phase detection. It requires 1,2 cyl, dual+missing, wasted-COP and 'Timed-min' MAP sampling.\r\0"
err153:     fcc         "SERIOUS CODE PROBLEM - stack overflow - contact support immediately.\r\0"
err154:     fcc         "VSS3 conflict\r\0"
err155:     fcc         "VSS4 conflict\r\0"
err156:     fcc         "Too many software PWMs configured.\r\0"
err157:     fcc         "Staged switched output 1 conflict\r\0"
err158:     fcc         "Staged switched output 1 not allowed with table based or fully transition.\r\0"
err159:     fcc         "Fuel pump output conflict\r\0"
err160:     fcc         "Alternator control output conflict\r\0"
err161:     fcc         "Alternator load monitor input conflict\r\0"
err162:     fcc         "Alternator current monitor input conflict\r\0"
err163:     fcc         "Alternator lamp output conflict\r\0"
err164:     fcc         "Staged switched output 2 conflict\r\0"
err165:     fcc         "Shift light output conflict\r\0"
err166:     fcc         "Oil pressure light output conflict\r\0"
err167:     fcc         "TCS input conflict\r\0"
err168:     fcc         "VSS required to use VSS launch\r\0"
err169:     fcc         "HD32-2 with MAP phase sensing requires that you set : 2 cyl. Oddfire. Wasted COP.\r\0"
err170:     fcc         "MAP sensor is required, but none enabled. (e.g. fuel, spark algorithm, multiply map)\r\0"
err171:     fcc         "Conflict on PWM idle output.\r\0"
err172:     fcc         "Conflict on PWM idle 3-wire output.\r\0"
err173:     fcc         "Wasted COP does not work on odd-fire or odd cyl counts\r\0"
err174:     fcc         "Error in PWM output setup for generic PID\r\0"
err175:     fcc         "Maximum duty must be > minimum duty for boost control.\r\0"
err176:     fcc         "Maximum duty must be > minimum duty for idle speed control.\r\0"
err177:     fcc         "Invalid min or max duty setting for boost control.\r\0"
err178:     fcc         "Invalid min or max duty setting for idle speed control.\r\0"
err179:     fcc         "VVT On/Off mode is only supported with 1 VVT.\r\0"
err180:     fcc         "Conflict on line-lock staging input.\r\0"
err181:     fcc         "Conflict on line-lock staging output.\r\0"
err182:     fcc         "Conflict on SDcard trigger output.\r\0"
err183:     fcc         "Page 28 != VARS1.\r\0"
err184:     fcc         "Conflict on pit limiter input.\r\0"
err185:     fcc         "Conflict on TC LED.\r\0"
err186:     fcc         "Conflict on dome fill output\r\0"
err187:     fcc         "Conflict on dome empty output\r\0"
err188:
err189:     fcc         "undefined error 189\r\0" ; replace with real error
err190:     fcc         "CAN transceiver broken!\r\0"
err191:
err192:
err193:
err194:
err195:
err196:
err197:
err198:
err199:     fcc         "Undefined config error\r\0"

; ports 0 = PM, 1 = PJ, 2 = PP, 3 = PT, 4 = PA, 5 = PB, 6 = PK
ce_port:   fcc         "MJPTABK"

rtbl:
            fdb         r0
            fdb         r1
            fdb         r2
            fdb         r3
            fdb         r4
            fdb         r5
            fdb         r6
            fdb         r7
            fdb         r8
            fdb         r9
            fdb         r10
            fdb         r11
            fdb         r12
            fdb         r13
            fdb         r14
            fdb         r15
            fdb         r16
            fdb         r17
            fdb         r18
            fdb         r19
            fdb         r20
            fdb         r21
            fdb         r22
            fdb         r23
            fdb         r24
            fdb         r25
            fdb         r26
            fdb         r27
            fdb         r28
            fdb         r29
            fdb         r30
            fdb         r31
            fdb         r32
            fdb         r33
            fdb         r34
            fdb         r35
            fdb         r36
            fdb         r37
            fdb         r38
            fdb         r39
            fdb         r40
            fdb         r41
            fdb         r42
            fdb         r43
            fdb         r44
            fdb         r45
            fdb         r46
            fdb         r47
            fdb         r48
            fdb         r49
            fdb         r50
            fdb         r51
            fdb         r52
            fdb         r53
            fdb         r54
            fdb         r55
            fdb         r56
            fdb         r57
            fdb         r58
            fdb         r59
            fdb         r60
            fdb         r61
            fdb         r62
            fdb         r63
            fdb         r64
            fdb         r65
            fdb         r66
            fdb         r67
            fdb         r68
            fdb         r69
            fdb         r70
            fdb         r71
            fdb         r72
            fdb         r73
            fdb         r74
            fdb         r75
            fdb         r76
            fdb         r77
            fdb         r78
            fdb         r79
            fdb         r80
            fdb         r81
            fdb         r82
            fdb         r83
            fdb         r84
            fdb         r85
            fdb         r86
            fdb         r87
            fdb         r88
            fdb         r89
            fdb         r90
            fdb         r91
            fdb         r92
            fdb         r93
            fdb         r94
            fdb         r95
            fdb         r96
            fdb         r97
            fdb         r98
            fdb         r99
            fdb         r100
            fdb         r101
            fdb         r102
            fdb         r103
            fdb         r104
            fdb         r105
            fdb         r106
            fdb         r107
            fdb         r108
            fdb         r109
            fdb         r110
            fdb         r111
            fdb         r112
            fdb         r113
            fdb         r114
            fdb         r115
            fdb         r116
            fdb         r117
            fdb         r118
            fdb         r119
            fdb         r120
            fdb         r121
            fdb         r122
            fdb         r123
            fdb         r124
            fdb         r125
            fdb         r126
            fdb         r127
            fdb         r128
            fdb         r129
            fdb         r130
            fdb         r131
            fdb         r132
            fdb         r133
            fdb         r134
            fdb         r135
            fdb         r136
	    fdb		r137
	    fdb		r138

who:        fcc         " - pin already in use by - "
            fcb         0
r0:       fcc         "unknown feature\r\0"
r1:       fcc         "injection LED\r\0"
r2:       fcc         "warmup LED\r\0"
r3:       fcc         "squirt LED\r\0"
r4:       fcc         "fuel pump\r\0"
r5:       fcc         "PWM or on/off idle\r\0"
r6:       fcc         "stepper idle\r\0"
r7:       fcc         "primary injectors\r\0"
r8:       fcc         "secondary injectors\r\0"
r9:       fcc         "a programmable on/off output\r\0"
r10:      fcc         "spark outputs\r\0"
r11:      fcc         "HEI bypass\r\0"
r12:      fcc         "knock\r\0"
r13:      fcc         "MAF\r\0"
r14:      fcc         "MAP\r\0"
r15:      fcc         "baro\r\0"
r16:      fcc         "EGO\r\0"
r17:      fcc         "tacho out\r\0"
r18:      fcc         "A/C idleup out\r\0"
r19:      fcc         "A/C idleup in\r\0"
r20:      fcc         "fan out\r\0"
r21:      fcc         "boost out\r\0"
r22:      fcc         "N2O stage 1 nitrous\r\0"
r23:      fcc         "N2O stage 1 fuel\r\0"
r24:      fcc         "N2O stage 2 nitrous\r\0"
r25:      fcc         "N2O stage 2 fuel\r\0"
r26:      fcc         "N2O in\r\0"
r27:      fcc         "launch in\r\0"
r28:      fcc         "variable launch in\r\0"
r29:      fcc         "3 step in\r\0"
r30:      fcc         "datalog button\r\0"
r31:      fcc         "datalog LED\r\0"
r32:      fcc         "bike shift in\r\0"
r33:      fcc         "bike shift out\r\0"
r34:      fcc         "generic PWM A\r\0"
r35:      fcc         "generic PWM B\r\0"
r36:      fcc         "generic PWM C\r\0"
r37:      fcc         "generic PWM D\r\0"
r38:      fcc         "generic PWM E\r\0"
r39:      fcc         "generic PWM F\r\0"
r40:      fcc         "datalog stream\r\0"
r41:      fcc         "dual fuel in\r\0"
r42:      fcc         "fuel tableswitch\r\0"
r43:      fcc         "spark tableswitch\r\0"
r44:      fcc         "boost tableswitch\r\0"
r45:      fcc         "overboot tableswitch\r\0"
r46:      fcc         "ReqFuel switch\r\0"
r47:      fcc         "AFR tableswitch\r\0"
r48:      fcc         "stoich switch\r\0"
r49:      fcc         "max AFR safety switch\r\0"
r50:      fcc         "accelerometer X\r\0"
r51:      fcc         "accelerometer Y\r\0"
r52:      fcc         "accelerometer Z\r\0"
r53:      fcc         "vss out\r\0"
r54:      fcc         "water inj pump\r\0"
r55:      fcc         "water inj valve\r\0"
r56:      fcc         "water inj in\r\0"
r57:      fcc         "ignition trigger LED\r\0"
r58:      fcc         "flex fuel in\r\0"
r59:      fcc         "cam input\r\0"
r60:      fcc         "vss1 in\r\0"
r61:      fcc         "vss2 in\r\0"
r62:      fcc         "speed sensor 1 in\r\0"
r63:      fcc         "speed sensor 2 in\r\0"
r64:      fcc         "gear position in\r\0"
r65:      fcc         "anti-lag in\r\0"
r66:      fcc         "anti-lag out\r\0"
r67:      fcc         "anti-lag PWM out\r\0"
r68:      fcc         "Boost 2 out\r\0"
r69:      fcc         "EGT 1 in\r\0"
r70:      fcc         "EGT 2 in\r\0"
r71:      fcc         "EGT 3 in\r\0"
r72:      fcc         "EGT 4 in\r\0"
r73:      fcc         "EGT 5 in\r\0"
r74:      fcc         "EGT 6 in\r\0"
r75:      fcc         "EGT 7 in\r\0"
r76:      fcc         "EGT 8 in\r\0"
r77:      fcc         "EGT 9 in\r\0"
r78:      fcc         "EGT 10 in\r\0"
r79:      fcc         "EGT 11 in\r\0"
r80:      fcc         "EGT 12 in\r\0"
r81:      fcc         "EGT 13 in\r\0"
r82:      fcc         "EGT 14 in\r\0"
r83:      fcc         "EGT 15 in\r\0"
r84:      fcc         "EGT 16 in\r\0"
r85:      fcc         "Generic Sensor 1 in\r\0"
r86:      fcc         "Generic Sensor 2 in\r\0"
r87:      fcc         "Generic Sensor 3 in\r\0"
r88:      fcc         "Generic Sensor 4 in\r\0"
r89:      fcc         "Generic Sensor 5 in\r\0"
r90:      fcc         "Generic Sensor 6 in\r\0"
r91:      fcc         "Generic Sensor 7 in\r\0"
r92:      fcc         "Generic Sensor 8 in\r\0"
r93:      fcc         "Generic Sensor 9 in\r\0"
r94:      fcc         "Generic Sensor 10 in\r\0"
r95:      fcc         "Generic Sensor 11 in\r\0"
r96:      fcc         "Generic Sensor 12 in\r\0"
r97:      fcc         "Generic Sensor 13 in\r\0"
r98:      fcc         "Generic Sensor 14 in\r\0"
r99:      fcc         "Generic Sensor 15 in\r\0"
r100:     fcc         "Generic Sensor 16 in\r\0"
r101:     fcc         "VVT 1 output\r\0"
r102:     fcc         "VVT 2 output\r\0"
r103:     fcc         "VVT 3 output\r\0"
r104:     fcc         "VVT 4 output\r\0"
r105:     fcc         "VVT cam 2/3/4 input\r\0"
r106:     fcc         "Real Time Clock\r\0"
r107:     fcc         "Torque Convertor Lockup\r\0"
r108:     fcc         "Traction control\r\0"
r109:     fcc         "Rotary trailing dwell timer\r\0"
r110:     fcc         "Internal SPI2\r\0"
r111:     fcc         "2nd MAP\r\0"
r112:     fcc         "Long Term Trim button\r\0"
r113:     fcc         "dual-fuel injectors\r\0"
r114:     fcc         "launch timed in\r\0"
r115:     fcc         "launch timed out\r\0"
r116:     fcc         "throttle stop\r\0"
r117:     fcc         "check engine light\r\0"
r118:     fcc         "vss3 in\r\0"
r119:     fcc         "vss4 in\r\0"
r120:     fcc         "Staged inj switch output 1\r\0"
r121:     fcc         "Alternator control output\r\0"
r122:     fcc         "Alternator load monitor input\r\0"
r123:     fcc         "Alternator current monitor input\r\0"
r124:     fcc         "Alternator lamp output\r\0"
r125:     fcc         "Staged inj switch output 1\r\0"
r126:     fcc         "Shift light output\r\0"
r127:     fcc         "Oil pressure warning output\r\0"
r128:     fcc         "TCS input\r\0"
r129:     fcc         "MAP2\r\0"
r130:     fcc         "Generic CL Out 1\r\0"
r131:     fcc         "Generic CL Out 2\r\0"
r132:     fcc         "Line-lock staging input\r\0"
r133:     fcc         "Line-lock staging output\r\0"
r134:     fcc         "SDcard trigger output\r\0"
r135:     fcc         "Pit limiter input\r\0"
r136:     fcc         "TC LED out\r\0"
r137:     fcc         "Dome fill out\r\0"
r138:     fcc         "Dome empty out\r\0"

.nolist                      ;skip the symbol table
