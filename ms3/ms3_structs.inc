;*********************************************************************
; MS3 equates for asm
;*********************************************************************
; $Id: ms3_structs.inc,v 1.56.4.1 2015/01/20 22:54:56 jsmcortina Exp $
; * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
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

.equ outpc.seconds,        outpc+0
.equ outpc.pw1,            outpc+2
.equ outpc.pw2,            outpc+4
.equ outpc.rpm,            outpc+6
.equ outpc.squirt,         outpc+10
.equ outpc.engine,         outpc+11
.equ outpc.mat,            outpc+20
.equ outpc.clt,            outpc+22
.equ outpc.knock,          outpc+32
.equ outpc.iacstep,        outpc+54
.equ outpc.sd_status,      outpc+70
.equ outpc.EAEfcor1,       outpc+72
.equ outpc.status1,        outpc+78
.equ outpc.status2,        outpc+79
.equ outpc.status3,        outpc+80
.equ outpc.status4,        outpc+81
.equ outpc.status6,        outpc+82
.equ outpc.istatus5,       outpc+84
.equ outpc.wallfuel1,      outpc+96
.equ outpc.wallfuel2,      outpc+100
.equ outpc.sensors0,       outpc+104
.equ outpc.sensors1,       outpc+106
.equ outpc.sensors2,       outpc+108
.equ outpc.sensors3,       outpc+110
.equ outpc.sensors4,       outpc+112
.equ outpc.sensors5,       outpc+114
.equ outpc.sensors6,       outpc+116
.equ outpc.sensors7,       outpc+118
.equ outpc.sensors8,       outpc+120
.equ outpc.sensors9,       outpc+122
.equ outpc.sensors10,      outpc+124
.equ outpc.sensors11,      outpc+126
.equ outpc.sensors12,      outpc+128
.equ outpc.sensors13,      outpc+130
.equ outpc.sensors14,      outpc+132
.equ outpc.sensors15,      outpc+134
.equ outpc.pwseq0,         outpc+144
.equ outpc.pwseq1,         outpc+146
.equ outpc.pwseq2,         outpc+148
.equ outpc.pwseq3,         outpc+150
.equ outpc.pwseq4,         outpc+152
.equ outpc.pwseq5,         outpc+154
.equ outpc.pwseq6,         outpc+156
.equ outpc.pwseq7,         outpc+158
.equ outpc.pwseq8,         outpc+160
.equ outpc.pwseq9,         outpc+162
.equ outpc.nitrous1_duty,  outpc+176
.equ outpc.nitrous2_duty,  outpc+177
.equ outpc.gpiopwmin0,     outpc+212
.equ outpc.gpiopwmin1,     outpc+214
.equ outpc.gpiopwmin2,     outpc+216
.equ outpc.gpiopwmin3,     outpc+218
.equ outpc.EAEfcor2,       outpc+224
.equ outpc.duty_pwm,       outpc+246
.equ outpc.stream_level,   outpc+332
.equ outpc.water_duty,     outpc+333
.equ outpc.vss1,           outpc+336
.equ outpc.vss2,           outpc+338
.equ outpc.ss1,            outpc+340
.equ outpc.ss2,            outpc+342
.equ outpc.vss3,           outpc+426
.equ outpc.vss4,           outpc+428
.equ outpc.vvt_duty,       outpc+372 ; array of 4

;these may need checking // FIXME
.equ ram4.ICIgnOption,   ram4+2
.equ ram4.adv_offset,    ram4+8
.equ ram4.IACtstep,      ram4+54
.equ ram4.hardware,      ram4+77
.equ ram4.InjPWMTim,     ram4+116
.equ ram4.InjPWMPd,      ram4+117
.equ ram4.InjPWMDty,     ram4+118
.equ ram4.EngStroke,     ram4+119
.equ ram4.dwelltime,     ram4+135
.equ ram4.InjPWMTim2,    ram4+143
.equ ram4.InjPWMPd2,     ram4+144
.equ ram4.InjPWMDty2,    ram4+145
.equ ram4.board_id_type, ram4+170
.equ ram4.mycan_id,      ram4+171
.equ ram4.FlexFuel,      ram4+186
.equ ram4.dwellmode,     ram4+191
.equ ram4.knk_option,    ram4+204
.equ ram4.EAEOption,     ram4+212
.equ ram4.No_Teeth,      ram4+220
.equ ram4.Miss_ang,      ram4+224
.equ ram4.spk_config,    ram4+237
.equ ram4.spk_mode,      ram4+238
.equ ram4.userlevel,     ram4+239
.equ ram4.feature4_0,    ram4+246
.equ ram4.pwmidleset,    ram4+269
.equ ram4.tacho_opt,     ram4+279
.equ ram4.tacho_opt2,    ram4+281
.equ ram4.fc_ego_delay,  ram4+280
.equ ram4.feature3,      ram4+286
.equ ram4.N2Oopt2,       ram4+323
.equ ram4.RotarySplitMode,  ram4+352
.equ ram4.pwmidle_ms,    ram4+364
.equ ram4.pwmidle_freq,  ram4+386
.equ ram4.boost_ctl_settings, ram4+387
.equ ram4.boost_ctl_pins, ram4+388
.equ ram4.boost_ctl_ms,  ram4+394
.equ ram4.boost_ctl_pwm,  ram4+396
.equ ram4.secondtrigopts, ram4+428
.equ ram4.TC5_required_width, ram4+429
.equ ram4.idle_special_ops, ram4+459
.equ IDLE_SPECIAL_OPS_IDLEADVANCE, 0x1
.equ IDLE_SPECIAL_OPS_IDLEVE, 0x2
.equ ram4.feature413,    ram4+485
.equ ram4.log_style,    ram4+486
.equ ram4.sequential,    ram4+700
.equ ram4.opentime_opta, ram4+728
.equ ram4.opentime_optb, ram4+729
.equ ram4.vss1_an,       ram4+785
.equ ram4.vss_opt0,      ram4+794
.equ ram4.vss2_an,       ram4+797
.equ ram4.mapsample_opt, ram4+802
.equ ram4.water_pins_in, ram4+822
.equ ram4.water_freq,    ram4+823
.equ ram4.MAFOption,     ram4+900
.equ ram4.feature7,      ram4+911
.equ ram4.ac_delay_since_last_on, ram4+1003

.equ ram5.pwm_testio,   ram5
.equ ram5.duty_testio,  ram5+1
.equ ram5.testop,       ram5+2
.equ ram5.testdwell,    ram5+3
.equ ram5.testint,      ram5+4 ; word
.equ ram5.testpw,       ram5+6 ; word
;.equ ram5.testinjcnt,   ram5+8 ; word
;.equ ram5.testmode,     ram5+10
.equ ram5.testInjPWMTim, ram5+11
.equ ram5.testInjPWMPd,  ram5+12
.equ ram5.testInjPWMDty, ram5+13
.equ ram5.testsel,  ram5+14
.equ ram5.iactest,  ram5+20
.equ ram5.flashlock,ram5+21
.equ ram5.boost_ctl_settings2, ram5+22
.equ ram5.vss3_an, ram5+34
.equ ram5.vss4_an, ram5+35
.equ ram5.pwm_opt,  ram5+838
.equ ram5.vss_samp_int, ram5+568

.equ ram_window.pg24, ram_window+0xc00
.equ ram_window.pg24.vvt_opt1, ram_window.pg24+0x258
.equ ram_window.pg24.vvt_ctl_ms, ram_window.pg24+0x25e

.equ ram_window.pg27, ram_window+0x400
.equ ram_window.pg27.generic_pid_flags1, ram_window.pg27+0x1bd
.equ ram_window.pg27.generic_pid_flags2,  ram_window.pg27+0x1be
.equ ram_window.pg27.generic_pid_control_intervals1, ram_window.pg27+0x31b
.equ ram_window.pg27.generic_pid_control_intervals2, ram_window.pg27+0x31c

.equ ram_window.vars2, ram_window+0xc00
.equ v2.knk_clk, ram_window.vars2+0
