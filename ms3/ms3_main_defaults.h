/* $Id: ms3_main_defaults.h,v 1.293.4.7 2015/04/10 14:41:10 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 *
    Origin: Al Grippo
    Major: James Murray / Kenneth Culver
    Majority: Al Grippo / James Murray / Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#ifndef _MS3_MAIN_DEFAULTS
#define _MS3_MAIN_DEFAULTS
const page4_data dflash4 CNFDATA2_ATTR = {
    4,                          // no_cyl (1-12)
    3,                          // no_skip_pulses, don't need if wheel decode mode
    0x11,                       // ICIgnOption,
    30, 10, 9,                  // max_coil_dur,max_spk_dur,DurAcc   msx10

    // For EDIS keep total adv (incl. offset & cold adv) < 60 deg
    700,                        // rpm at which cranking through

    0,                          // adv_offset,   deg x 10
    900, 500,                   // TpsBypassCLTRevlim , RevLimRpm2

    93,                         // map0,         kPa x 10, value @ 0 ADC counts (MPXH6400 values)
    2609,                       // mapmax,       kPa x 10, value @ max(1023) ADC counts
    0,                          // clt0,         deg (C or F) x 10
    100,                        // cltmult,      %
    0,                          // mat0,         deg (C or F) x 10
    100,                        // matmult,      %
    0,                          // tps0,         adc counts
    1023,                       // tpsmax,       adc counts
    1,                          // batt0,        v x 10
    297,                        // battmax,      v x 10
    0,                          // ego0,         afr x 10
    100,                        // egomult,      %
    93,                         // baro0,        kPa x 10
    2609,                       // baromax,      kPa x 10
    0, 0,                       // bcor0,bcormult  kpax10, slope - default data cancels this old method. Use curve instead.
    250,                        // iacfullopen
    4000,                        // vss reluctorteeth1 x 0.01
    50, 70, 25,                 // PulseTol,     % tolerance for next input pulse timing during
    // cranking, after start/ warmup, normal running
    0,                          // IdleCtl, idle: 0 = none
    3,                         // IACtstep,  1 ms units (3 gives pulse freq of 333 Hz)
    5,                         // IAC_tinitial_step
    2,                          // IACminstep
    127,                        // dwell duty%
    250,                        // IACStart,  no. of steps to send at startup to put stepper
    //    motor at reference (wide open) position
    50,                         // IdleHyst amount (degx10)
    100, 5,                     // > IAC opening (< steps) during cranking and few secs after
    0,                          // iaccurlim
    0,                          // spare
    4000,                         // vss reluctorteeth2 x 0.01
    400,                        // boosttol
    1000,                       // OverBoostKpa2
    1100,                       // fc_rpm_lower
    0,                          //overboostoption
    0x7a,                       // hardware. MS3X fuel, MS3X spark, MS3X cam, H3+4 for injI/J
    1000,
    100,
    1500,                       // N2Olaunchmaxmap
    2000,                       // TpsThresh, tpsdot threshhold for accel enrichment(change in %x10 per .1 s)
    100,                        // MapThresh, mapdot threshhold for accel enrichment(change in kPax10 per .1 s)
    20,                         // Tpsacold,  cold (-40F) accel amount in %reqfuel units
    130,                        // AccMult,   cold (-40F) accel multiply factor (%)
    20,                          // TpsAsync,  clock duration (in .01 sec tics) for accel enrichment
    100,                         // TPSDQ,  deceleration fuel cut option (%)
    700,                        // TPSWOT, TPS value at WOT (for flood clear), %x10
    700,                        // TPSOXLimit,  Max tps value (%x10) where O2 closed loop active
    100,                        // Tps_acc_wght, weight to be given to tpsdot for accel enrichment.
    //  100 - Tps_acc_wght will then be given to mapdot.
    0,                          // baroCorr BaroOption,  0=no baro, 1=baro is 1st reading of map (before cranking),
    //   2=independent barometer
    1,                          // 0 = no ego;1= nb o2;2=2 nb o2;3=single wbo2;4=dual wbo2.
    16,                         // EgoCountCmp,  Ign Pulse counts between when EGO corrections are made
    1,                          // EgoStep,   % step change for EGO corrections
    15,                         // EgoLimit,  Upper/Lower rail limit (egocorr inside 100 +/- limit)
    92,                        // EGOVtarget,  NBO2 afr (afrx10) determining rich/ lean
    0,                          // Temp_Units,    0= coolant & mat in deg F; 1= deg C
    1, 0,                       // 
    1400,                       // FastIdle, fast idle temperature (degx10) (idle_ctl = 1 only)
    1600,                       // EgoTemp,  min clt temp where ego active, degx10
    1300,                       // RPMOXLimit,  Min rpm where O2 closed loop is active
    8000,                       // ReqFuel;  fuel pulsewidth (usec) at wide open throttle
    2,                          // Divider,   divide factor for input tach pulses
    1,                          // Alternate,   option to alternate injector banks
    8,                          // InjPWMTim,   Time (.128 ms units) after opening to start pwm
    66,                         // InjPWMPd,    Injector PWM period (us)
    75,                         // InjPWMDty,   Injector PWM duty cycle (%)
    0,                          // EngStroke,  0 = 4 stroke,  1 = 2 stroke
    0,                          // InjType,  0 = port injection,  1 = throttle body
    4,                          // NoInj,    no. of injectors (1-12)
    900,                        // OddFire smaller angle between firings
    50, 50, 50,                 // Lag filter coefficients (1-100%) for Rpm,Map,Tps,
    60,                         // ego1,2
    50,                         // Lag filter coefficients for other adc(clt,mat,batt)
    7,                          // knock window pin
    50,                         // mafLF
    0,                          //
    1,                          // fuel alpha-N, map blend option
    1,                          // ign alpha-N, map blend option
    1,                          // WBO2 AFR alpha-N, map blend option
    10,                         // dwell time in 0.1ms
    500,                        // trigger to return angle
    1,                          // RevLimOption:0,none; 1,retard spk; 2,fuel cut + more (see ini)
    120,                        // RevLimMaxRtd,   deg x 10
    30,                         // ego_startdelay
    8,                          // can_poll2_ego
#ifdef MS3PRO
    1,                          // opt142 internal RTC on
#else
    0,                          // opt142 RTC off
#endif
    8, 66, 75,                  // InjPWMTim2, InjPWMPd2, InjPWMDty2.  injector channel 2 parameters
    5, 7, 153,                       // can_ego
    1050, 800, 1000,            // baro upper, lower, default
    6000,                       // RevLimTPSbypassRPM
    200,                        // launchcutzone
    6000,                       // RevLimNormal2

    0,                          //hw latency
    0xd,                        //loadopts, ve2=multiplicative, multiply MAP=on, includeAFR=on
    115200,                     // baud rate
    900,                        // MAPOXLimit, Max MAP value (kPax10) where O2 closed loop active
    5, 0,                       // poll_id_rtc, mycan_id
    50,                         // map sample percent
    5, 5,                       // remote can ids
    0,                          // accel tail duration (0.01 sec),  TpsAsync2
    0,                         // accel end pwidth enrichment (%ReqFuel) TpsAccel2
    0,                          // EgoAlg, 0=simple algorithm; 1=Prop err alg;2=PID w. Smith pred
    100, 20, 0,                 // KP,KI,KD, PID coefficients in %
    178, 22,                   // ac_idleup_vss_offpoint, ac_idleup_vss_hyst
    //   delay (ms) = Kdly1 + Kdly2*120000 / (map(kPax10)*rpm).
    //   Defaults based on xpt delay of .1 s at wot, 1 s at idle
    4,                          // Flex fuel option - modifies pw and spk adv based on freq signal
    //  for % alcohol
    {50, 150},                  // Table of fuel sensor freq(Hz) vs
    {100, 163},                 // fuel pw corr in %; 1st element is pure gas, 2nd is pure Alcohol.
    0,                          // dwell mode   "std dwell", "fixed duty", "time after spk", "chg at trigger"
    9000,                       // pwmidle_shift_lower_rpm. Really high number disables
    700,                        // ac_idleup_tps_offpoint
    100,                        // ac_idleup_tps_hyst
    700,                        // fan_idleup_tps_offpoint
    100,                        // fan_idleup_tps_hyst
    179,                        // fan_idleup_vss_offpoint
    0x40,                       // knk_option: Bits 0-3: 0=no knock detection;1=operate at table value or 1
    // step below knock; 2=operate at table value or edge of knock.
    // Bits 4-7: 0/1 = knock signal < / > knk_thresh indicates knock occurred.
    100,                        // knk_maxrtd, max total retard when knock, (degx10).
    30, 10,                     // knk_step1, _step2, ign retard/ adv steps when 1st knock or after stopped,
    // (degx10); step1 large to quickly retard/ stop knock
    2, 20,                      // knk_trtd,_tadv, time between knock retard, adv corrections, (secx10);
    // allows short time step to quickly retard, longer to try advancing.
    30,                         // knk_dtble_adv, change in table advance required to restart adv til knock
    // or reach table value (0 knock retard) process, deg x10.
    // This only applies with knk_option = 1.
    2,                          // knk_ndet, number of knock detects required for valid detection; pad byte.
    0,                          //EAE option
    6,                          // knk_port
    700,                        // knk_maxmap, no knock retard above this map (kPax10).
    700, 3500,                  // knk_lorpm,knk_hirpm,  no knock retard below, above these rpms.

    36,                         // No_Teeth, nominal (include missing) teeth for wheel decoding.
    1,                          // No_Miss_Teeth, number of consecutive missing teeth.
    0,                          // pwmidle_shift_open_time
    800,                        // Angle of missing tooth BTDC
    01,                         // ICISR_tmask, time (msx10) after tach input capture during which further
    // interrupts are inhibited to mask coil ring or VR noise.
    10,                         // ICISR_pmask, % of dtpred after tach input capture during which further
    // interrupts are inhibited to mask coil ring or VR noise.
    6000, 6000,                 // ae_lorpm,ae_hirpm, lorpm is rpm at which normal accel enrichment just
    // starts to scale down, and is reduced to 0 at ae_hirpm. To omit scaling, set
    //  _lorpm = _hirpm= very large number.
    {0, 35},                  // ffSpkDel fuelSpkDel flex fuel spk corr (degx10); 1st element for pure gasoline, 2nd for pure
    // ethanol; advance spk since E85 burns slower.
    0,                          // spk_conf2
    0x24,                       // spk_config  crank wheel. single wheel
    0x04,                       // spk_mode, "trigger wheel"
    0x40,                       // wasted spark
    1,                          // rtbaroport
    0,                          // ego2port
    0,                          // mapport defaults to mainboard
    3,                          // knport_an

    0,                          // OvrRunC
    1,                          // poll_level_tooth
    0,                          // use simple false trig as should be working now

    2, 60,                      // timing_flags, crank_dwell
    1,                          // table switching pin PE1 by default
    100, 100,                   // crank_timing, fixed_timing

    6000, 1000, 800,            // fuel table switch rpm, load, tps
    6001, 1001, 801,            // spark table switch rpm, load, tps

    0,                          // feature5_0 defaults to zero
    1,                          // tsw_pin_s
    5,                          // crank taper time
    20,                         // knk_step_adv
    1500,                       // fc >rpm
    400,                        // fc <kpa
    50,                         // fc <tps
    900,                        // fc >clt
    15,                         // fc >time
    7,                          // tacho out off, defaults to 'tacho'
    0,                          // fc_ego_delay
    0,                          // tacho opt2
    1000,                       // tacho scale
    0, // spare
    0,                          // feature3
    0, 200, 10, 4000, 100, 1, // launch_opt, launch_sft_zone, launch_sft_deg, launch_hrd_lim, launch_tps, launchlimopt
    10, // launchvsstime
    6,  // launchvss_maxgear
    9,                          // launch_opt_pins
    3000, 10, 50, 4800,       //flats_arm, launchvss_minvss, flats_deg, flats_hrd
    180, 600, 0, 0, 0, 0, 0, 0,     // staging
    0x03, 0x1c, 3000, 6000, 800, 1500, 100, 6000, 3000, 0, 0, 0,   // nitrous N2Oopt
    50, 5000, 6000, 10, 2000, 1000,     // nitrous stage 2
    0,                          //rotary split mode
    4,                          // dlyct delay/noise filtering 0.5us
    30,                         // dwelltime_trl
    8,                          // N2Oopt_pins
    10,                         // rev lim
    0,                          // vss4_can_offset
    200,                        // more rev limiter fields
    0,                          // pwmidleset_inv 
    1,                          // tooth_init for injector sequencing
    200,                        // pwmidle_ms
    3,                          // pwmidle_close_delay
    153,                         // pwmidle_open_duty
    38,                         // pwmidle_closed_duty
    30,                         // pwmidle_pid_wait_timer
    0x80,                       // pwmidle_freq_pin3 + koeo
    300,                        // ac_idleup_min_rpm
    10,                         // pwmidle_tps_thresh
    3,                          // pwmidle_dp_adder
    10,                         // pwmidle_rpmdot_threshold (/ 10)
    250,                        // pwmidle_decelload_threshold
    1000,                       // pwmidle_Kp
    1000,                         // pwmidle_Ki
    0,                          // pwmidle_Kd,
    0x59,                       // pwmidle_freq
    0x1, 9,                       // boost_ctl_settings
    100, 100, 100, 0, 100, 10,          // boost_ctl_Kp,Ki,Kd, closed,open,ms
    0x2f,                       // boost_ctl_pwm
    0,                          // NoiseFilterOpts
    90,                        // launchcuttiming
    500,                      // pwmidle_max_rpm
    2,                          // pwmidle_targ_ramptime
    0,                          // inj time mask

    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0}, /* spare */
    0,                          //secondtrigopts
    416,                        //TC5_required_width
    150,                        //EgoLimit
    147,                        //stoich
    200,                        //MAPOXMin
    02, 20,                     //IC2ISR_tmask, IC2ISR_pmask - cam input mask time and %age settings
    0,                          // pad437
    0,                        // airdensity scaling

    22,                         // fan_idleup_vss_hyst
    4,                          // SDcard log_style_led (D15)
    0,                          // prime delay
    0,
    10,                          // boost_ctl_flags : basic mode for everything
    0,
    2000, 2000, // boost_ctl_sensitivity
    100,  50, // fuelcut_fuelon_
    0,                          //staged_secondary_enrichment
    0,                          //staged_primary_delay
    0,                          // idle_special_ops
    20,                         // idleadvance_tps
    1000,                       // idleadvance_rpm
    400,                        // idleadvance_load
    1400,                       // idleadvance_clt
    2,                          // idleadvance_delay
    {150, 150, 150, 150},       // idleadvance_curve
    {300, 320, 340, 380},       // idleadvance_loads
    10,                         // log_style2
    0,                          // log_style 64 byte. Log disabled.
    0,                          // more log settings
    0x02,                       // more log settings ADC stream port
    10,                          // log length 10 minutes
    156,                         // datalog sample interval 20ms (0.128ms units)

    {  0,   2,   4,   6,   8,  11,  12,  16,  18,  20,  22,  18,  24,  26,  28,  38,    //16  16bit log offsets
      40,  48,  50,  54,  58,  62,  66,  70,  94, 142, 252, 300,   0,   0,   0,   0,    //32  SD card
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    //48
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},   //64

    {2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2,    //16 // 8bit log entry size
     2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 2, 0, 0, 0, 0,    //32
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //48
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   //64

    {1, 3, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // engine firing order (4 cyl)
    4,                          // sequential setting
    0, 100, 0,                  // boost opts, boost switch pin
    0,
    3000, 5000,              // variable launch
    3000, //vssout_scale
    50, 4000,             // 3 step
    100,                        // map_sample_duration
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     // opentime_opt
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     // smallopen_opt
    0, 0, 1000, 2500, 500, 500, 200, 950, 2000, // max afr settings
    0,                          // launch fuel addition
    656, 411,                   // wheel dia in mm, final drive ratio
    656,
    0,0,                        // vss_opt
    0,                       // flats_minvss
    0, 0, 7,                    // VSS options
    0,                          // overboost switch
    4, 4,                       // VSS options
    40, 60,
    0, 0,                       // spare
    0, 0,                 // Shaft speed
    1, 1, 0,                    // options
    0x4, 0,                       // mapsample_opt, map2port
    5, 5, 6, 6,                 // nitrous pins
    669, 669,                   // analogue VSS
    0, 0, 0, 0, 15000, 147,     // reqfuel, afr, stoich switching
    0, 0, 0,                    // water_pins_pump, water_pins_valve, water_pins_in
    0x1c,                       // water_freq
    900,                        // boost_vss_tps
    700, 1500, 900, 600,        // water_tps, water_rpm, water_map, water_mat
    100,                        // pwmidle_rpmdot_disablepid
    1000,                       // boost_ctl_lowerlimit
        0, //enable_poll bit 0: ADC ; bit 1&2: enable PWM; bit 3: enable digital I/O ports
        {7,7,7}, //poll_tables[3] Remote table numbers for ADC, PWM and ports data defaults to I/O Extender tables
        {110,58,77,75}, //poll_offset[3] Offset in the table (ADC, PWM, ports). Defaults to I/O Extender offsets
    0, //egt_num
    0,0,0, //accX,Y,Z ports
    217,483,217,483,217,483, // accelerometer calibration (raw counts for -/+ 1g)
    50, // lag
    {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31}, // EGT ports
    0x0c, // egt_conf
    320, 22820, // EGT calibration. ADC, ADC, degF, degF to match rest of Megasquirt.
    16200, 17100, 20,
    10000, 10000,
    {1, 2},
    0x43, //MAFOption
    2000, // engine size
    0, 0, //VSSout
    3, 24, 16, 3, 7, /* vss_can_size, canpwm_clk */
    0x80, // feature7. Crank taper uses ignition events by default
    50,50,50,50, // VSS,SS lags
    0, // EGT addfuel
    9000, // launch_fcut_rpm
    {332,209,140,100,83,50}, // gear ratios
    0, // gear method
    0, // AN port
    {489, 178, 225, 296, 364, 433, 472}, // gear voltages
    5, // num gears
    50, // 50% lag
    20, // 200ms interval
    1, // ac_idleup
    0,
    100,
    8,
    1, //fanctl
    100,
    4,
    2000,
    1800,
    0, 0,
    {5,5,5,5,5,5},
    {7,7,7,7,7,7},
    {2,10,18,26,34,42},
    300,
    0,
    1700,
    0,
    200, 90, // ego_upper_bound, ego_lower_bound
    0,
    894,  // launch_maxvss
    0, // maf_range
    5, // ac_delay_since_last_on
    1, 1, // shaft speed reluctorteeth3, 4
    3, // boost_gear_switch
    0, // staged_extended_opts
    5, 7, 94, // CAN pwm out
    // idleve
    10,
    1100,
    350,
    1400,
    2,
    0
};

const page5_data dflash5 CNFDATA2_ATTR = {
    1, 25,                          // test mode pwms
    0,                          // no coils, no injectors
    30, 1562,                    // 3.0ms 200ms interval
    0,                          // mode 0
    0,                          // zero pw
    0,                          // zero count
    8, 66, 30,                  // PWM params
    0,                          // injector A
    0,                          //iacpostest
    0,                          // tooth
    250, 0,
    0,                          //flashlock
    0, 5,                       // boost_ctl_settings2, pins
    100, 100, 100,                    // boost_ctl_Kp,Ki,Kd,
    0,0,                        // closed,open,ms
    0x09,                       // boost_ctl_pwm
    1000,                       // lower limit
    0,                          // sensor port
    0x19, 3, 4, 1 , 1,          // VSS3, VSS4 
    0, 0, // spare

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // EGO port
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // EGO xref

    0xc0, 5,                       // TC
    9, 400, 200,
    0,

    32, 0, 15, // knock_bpass etc.
    20, 100,
    -400, 2570,
    0,
    0,
    150, 25000,
    160, 375,
    {14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14}, // knock_gain
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, // knock_sens
    0, // s16_debug
    0x00, // u08_debug134
    0, // AE_options
    1000, // accel_blend_percent
    500, // accel_tpsdot_threshold (0.1% units)
    100, // accel_mapdot_threshold (1 kPa units)
    6001, 6001,                 // ae_lorpm2,ae_hirpm2,
    1000, // blend2
    501, // accel_tpsdot_threshold2 (0.1% units)
    101, // accel_mapdot_threshold2 (1 kPa units)
    2000, 100, 0, /* time based AE 2 */
    20, 130, 20, 100, 0, 100, /* do */
    1000, /* do */
    0,
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //1 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //2 16
     0, 0, 0, 0, 0, 0, 0},    //4 8
    1000,
    {-400, -300, -200, -100, 0, 100, 200, 300},
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    0, 1, 30, 0, 1000, // ltt on
    0, // tc_minmap
    0, 0, 1953, // can_bcast
    0, 6, 500, // TB delay
    0, // throttle stop
    1,  /* can_enable */
    1000, 500, // throttle stop
    {1800,1800,1800,1800,600,600}, // oddfireangs
    0, 0, // CEL
    85, 220, 2000, 10, // AFR
    0, 1023, 7000, // MAP
    5, 1018, 1500, // MAT
    5, 1018, 1000, // CLT
    0, 1023, 7000, // TPS
    70, 230, 15, // Batt
    0, 0,
    -400, 2120,
    300,
    1000,
    3000, 1050,
    0,0,1,0,
    0,
    0,
    -400, 27000,
    10000, 0,
    20, 0,
    {0,0,0,0,0,0,0},
    0, // gap
    0,0,2959,    // fuel pump and pressure
    100,50,30,
    100,0,7, 0,
    0, 3, 5, 6, // alternator control
    200, 100, 50,
    30, 50,
    850,
    125, 148, 20, 0, 0, 142,
    156, 156,
    0, 15, 132, 10, 90,
    0,      // hpte
    900, 900,
    0, 5,
    {6200, 6500, 6500, 6500, 6500, 6500},
    30, 50, // ltt_samp_time, ltt_agg
    0,0, // oil pressure
    7912, // fuelcalctime
    {0, 0}, 200, // alternator min/max/sens
    0, // idleminvss
    0, // baseline eth% for backwards compat
    5,0,0,0, // fc_transition fc_ae
    0,0,255, // FP off/min/max duties
    0, 1, 50, 100, // TCS turbo car staging
    {1000,1000,1000,1000,1000,1000}, // boostgeartarg

    300, 900, 1500, 1000, // fuel pressure safety fp_drop
    {18,18,18,18,18,18}, // pwm_opt_load_offset
    {2,2,2,2,2,2}, //pwm_opt_load_size
    5,7,128, /* CAN GPS data */
    20,-150, /* SpkAdj max/min adjustments */

    1520, /* can_outpc_msg */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* can_outpc_gp00 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

    0,
    400, 80, -400, -80, 10, 200, 1200,
    0,
    1000, /* vss_samp_int */

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //1 16   big pad big gap
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //2 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //3 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //4 16
     0, 0, 0, 0                               //5 16
     },
    0, 4, // staged_out2, timer
    0, // debug640
    0, // staged_out1
    0, 1200, 50, // log_trig
    115200, // baudhigh
    163,    // fuelCorr_default
    0,      // spare
    0,    // fuelSpkDel_default
    700,    // map_phase_thresh
    {0,1000}, // flex_pct
#ifdef MS3PRO
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // generic sensors 1-16 source 
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7}, // transform
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-110}, // val0
#else
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, // generic sensors 1-16 source 
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, // transform
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, // val0
#endif
    {1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023}, // valmax
    {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100},
    0, // sensor temp units
    0, // pad789
    {1000,1000,1000,1000,1000,1000,1000,1000, // opening/dead time. MS3X inj1-8
    1000,1000, // V3 inj 1,2
    1000,1000,1000,1000,1000,1000,1000,1000}, // MS3X inj9-16

    0, 1, 1, 7000, 750, 10, 8, // shift_cut, shift_cut_in, shift_cut_out, shift_cut_rpm, shift_cut_tps, shift_cut_delay, shift_cut_time
    {3, 0, 0, 0, 0}, // shift_cut_add
    10, // shift_cut_soldelay
    {6000, 6000, 6000, 6000, 6000}, // shift_cut_rpm
    50, // shift_cut_reshift
    {0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c}, // pwm_opt
    {1, 1, 1, 1, 1, 1}, // pwm_opt2
    {60,60,60,60,60,60}, // pwm_onabove
    {40,40,40,40,40,40}, // pwm_offbelow
    0,0, // dual fuel sw
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, //opentime2
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, //smallpw2
    1,0, //dualfuel_pin, dualfuel_opt
    {1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000}, //opentimes2
        8, 66, 75,                  // Inj2PWM..A
        8, 66, 75,                  // Inj2PWM..B
    0,0,
    0, 0, 0, 0,                  // FuelAdj etc adjusters for GPIO - not used
    0, 0, // dualfuel_pri, dualfuel_sec - neither actually used
    900,10,
    {90,105,120,135,150,160},
    {5, 3, 1, 0, -1, -3},
    5000, //       ac_idleup_max_rpm
    0,0, // line-lock staging
    1000, // kick delay
    1200, // log_trig_map
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
};

const page8_data dflash8 CNFDATA_ATTR = {
    {{1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},  //boost_ctl_load_targets etc.
     {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
     {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
     {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
     {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
     {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
     {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
     {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000}},
    {0, 200, 400, 600, 700, 800, 900, 1000},    //tps
    {200, 400, 600, 800, 1000, 2000, 3000, 4000},       //load
    {{0, 0, 0, 0, 0, 0, 0, 0},  //boost_ctl_pwm_targets
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}},
    {0, 200, 400, 600, 700, 800, 900, 1000},    //tps
    {500, 1000, 2000, 3000, 4000, 5000, 6000, 7000},    //rpm

    {60, 56, 52, 48, 44, 40, 36, 32, 26, 20},   // CWPrime, (msx10)
    {325, 300, 275, 250, 225, 200, 175, 150, 125, 100}, // CrankPctTable,   (%age)
    {45, 43, 41, 39, 37, 35, 33, 31, 28, 25},   // CWAWEV,  %
    {350, 330, 310, 290, 270, 250, 230, 210, 180, 150}, // CWAWC,   cycles
    {1600, 1800, 2000, 2200, 2400, 2600},       // MatTemps, degx10 F or C
    {0, 0, 20, 40, 60, 80},     // MatSpkRtd, degx10 matretard
    {800, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000},    // AWCRPM
    {800, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000},    // SOCRPM
    {300, 350, 400, 450, 500, 550, 600, 650, 700, 800, 900, 1000},      //AWCKPA
    {300, 350, 400, 450, 500, 550, 600, 650, 700, 800, 900, 1000},      //SOCKPA
    {22, 28, 30, 32, 38, 40, 42, 48, 50, 52, 58, 60},   //BAWC in 1% units
    {22, 28, 30, 32, 38, 40, 42, 48, 50, 52, 58, 60},   //BSOC in 0.1% units
    {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45},  //AWN
    {45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100},  //SWN
    {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
    {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
    {                           // BaroVals[NO_BARS]: barometric pressures, (kPa x 10)) for baro correction table
     650, 700, 750, 800, 850, 900, 950, 1000, 1050},
    {                           // MatVals[NO_MATS]: air temperatures (degx10) for air density correction table
     -400, -40, 320, 590, 860, 1130, 1400, 1940, 2480},

    {100, 300, 500, 700, 900, 1100, 1300, 1500, 1700, 1800},    /* temp_table_p5 */
    {0, 200, 400, 600, 800, 1000, 1200, 1300, 1400, 1600, 1700, 1800},  // for EAE
    {0, 200, 400, 600, 800, 1000, 1200, 1300, 1400, 1600, 1700, 1800},  // for EAE

    {1500, 1400, 1300, 1200, 1100, 1000, 900, 800},     // pwmidle_target_rpms
    {700, 800, 900, 1000, 1100, 1200, 1300, 1400},      // pwmidle_clt_temps

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     // 10 spare ints (20 bytes)

    {320, 680, 1000, 1200, 1400, 1600, 1800, 2000},     // RevLimLookup
    {2500, 3000, 4000, 4500, 5000, 5500, 6000, 6000},   // RevLimRpm1
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 24 spares

    {{0, 0, 0, 0, 0, 0, 0, 0},  //rotary split table
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}},
    {300, 400, 500, 600, 700, 800, 900, 1000},
    {800, 1200, 2000, 3000, 4000, 5000, 6000, 7000},
    {500, 2500, 5500, 7500},    // NoiseFilterRpm
    {200, 40, 18, 13},          // NoiseFilterLen - default values based on 15% of a 60-2 tooth

    {3, 10, 20, 50},            //VariableLagTPSBins
    {50, 50, 50, 50},           //VariableLagMapLags
    {153, 140, 127, 102},       //pwmidle_bias_vals
    {300, 1000, 1300, 1750},    //pwmidle_bias_rpms

//    {85, 69, 53, 37, 21, 5},         // mat/clt vs flow correction
    {0, 0, 0, 0, 0, 0},         // mat/clt vs flow correction
    {500, 1700, 2900, 4100, 5300, 6500}, // flow/100
    /* user defined initial configuration data */
    0, 0, 0,
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    0,

//    {1165, 1141, 1118, 1094, 1071, 1047, 1024, 1000, 977}, // baro correction barocor - exposes default calc
    {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000}, // baro correction barocor - zero adjustment, 100%
    {1257, 1158, 1073, 1017,  967,  921,  880,  807, 746}, // MAT correction aircor - exposes default calc

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // spares
};

const page9_data dflash9 CNFDATA_ATTR = {
    {                           //inj trim tables (inj_trim)
     {                          //inj a
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj b
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj c
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj d
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj e
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj f
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj g
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj h
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj 1/i
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj 2/j
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj k
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj L
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj m
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj n
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj o
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //inj p
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      }
     },
    
     {500, 1000, 2000, 4000, 6000, 7000}, // shared rpm bins

     {200, 500, 800, 1000, 1500, 2000}, // shared load bins

    {{3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, // inj timing table
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600},
     {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600,
      3600, 3600}},
    {300, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000, 6000, 6500,
     7000},
    {200, 250, 300, 400, 450, 500, 600, 700, 750, 800, 900, 1000},

    {850, 900, 1500, 2000, 3000, 4000}, // max afr table1
    {500, 1000, 1500, 2000, 2500, 7000},
    {{10, 10, 10, 10, 10, 10},
     {7, 7, 7, 7, 7, 7},
     {5, 5, 5, 5, 5, 5},
     {3, 3, 3, 3, 3, 3},
     {3, 3, 3, 3, 3, 3},
     {3, 3, 3, 3, 3, 3}},
    {30, 30, 30, 30, 30, 30},
    {500, 1000, 2000, 3000, 5000, 8000},

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

};

const page10_data dflash10 CNFDATA_ATTR = {
    {{100, 100, 100, 100, 100, 100, 100, 100},  //boost_ctl_load_targets2 etc.
     {100, 100, 100, 100, 100, 100, 100, 100},
     {100, 100, 100, 100, 100, 100, 100, 100},
     {100, 100, 100, 100, 100, 100, 100, 100},
     {100, 100, 100, 100, 100, 100, 100, 100},
     {100, 100, 100, 100, 100, 100, 100, 100},
     {100, 100, 100, 100, 100, 100, 100, 100},
     {100, 100, 100, 100, 100, 100, 100, 100}},
    {0, 200, 400, 600, 700, 800, 900, 1000},    //tps
    {200, 400, 600, 800, 1000, 2000, 3000, 4000},       //load
    {{0, 0, 0, 0, 0, 0, 0, 0},  //boost_ctl_pwm_targets2
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0}},
    {0, 200, 400, 600, 700, 800, 900, 1000},    //tps
    {500, 1000, 2000, 3000, 4000, 5000, 6000, 7000},    //rpm

    {0, 20, 40, 60, 80, 100},   // boost_timed_pct
    {0, 1000, 2000, 3000, 4000, 5000},  // boost_timed_time (ms)

    {0, 0, 0, 0, 0, 0},         // boost vs vss
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},

    {800, 1500, 3000, 4000, 5000, 6000, 7000, 8000},    // map_sample
    {450, 450, 450, 450, 450, 450, 450, 450},

    /* AFR table 1 */
    {{{130, 135, 160, 160, 160, 149, 143, 132, 131, 132, 131, 130},
      {134, 139, 155, 155, 154, 149, 141, 130, 129, 128, 127, 127},
      {135, 140, 147, 146, 145, 143, 136, 130, 129, 128, 127, 126},
      {136, 141, 143, 142, 141, 139, 134, 129, 128, 127, 126, 126},
      {134, 134, 139, 137, 136, 136, 131, 127, 126, 126, 126, 126},
      {130, 130, 131, 130, 130, 129, 130, 125, 125, 125, 125, 125},
      {130, 129, 129, 128, 128, 127, 126, 125, 125, 125, 125, 124},
      {130, 129, 129, 128, 128, 127, 126, 125, 125, 125, 123, 123},
      {129, 128, 128, 127, 127, 126, 125, 125, 125, 122, 122, 122},
      {123, 123, 123, 122, 122, 121, 121, 120, 120, 118, 118, 118},
      {117, 116, 116, 116, 116, 116, 115, 115, 115, 114, 114, 114},
      {110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110}
     },

    /* AFR table 2 */
    {{130, 135, 160, 160, 160, 149, 143, 132, 131, 132, 131, 130},
      {134, 139, 155, 155, 154, 149, 141, 130, 129, 128, 127, 127},
      {135, 140, 147, 146, 145, 143, 136, 130, 129, 128, 127, 126},
      {136, 141, 143, 142, 141, 139, 134, 129, 128, 127, 126, 126},
      {134, 134, 139, 137, 136, 136, 131, 127, 126, 126, 126, 126},
      {130, 130, 131, 130, 130, 129, 130, 125, 125, 125, 125, 125},
      {130, 129, 129, 128, 128, 127, 126, 125, 125, 125, 125, 124},
      {130, 129, 129, 128, 128, 127, 126, 125, 125, 125, 123, 123},
      {129, 128, 128, 127, 127, 126, 125, 125, 125, 122, 122, 122},
      {123, 123, 123, 122, 122, 121, 121, 120, 120, 118, 118, 118},
      {117, 116, 116, 116, 116, 116, 115, 115, 115, 114, 114, 114},
      {110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110}
     }},

    {180,                       // warmen_table[TEMP no = 0],   % enrichment vs temp
     180, 160, 150, 135, 125, 113, 108, 102, 100},
    {0, 0, 0, 0, 0, 0, 0, 0},   // pad spare
    {160, 150, 140, 130, 120, 105, 90, 75, 60, 40},  // iacstep_table[TEMP no = 0] (correct order)
    {{500,                      // frpm_table1[RPM no = 0] , use in  AFR tables
      800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000},
     {500,                      // frpm_table2[RPM no = 0] , use in AFR table
      800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000}},
    {{300, 400, 500, 550, 600, 700, 800, 900, 1000, 2000, 3000, 4000}, // fmap_table[MAP/tps no = 0], kPa x 10 , use for AFR
     {300, 400, 500, 550, 600, 700, 800, 900, 1000, 2000, 3000, 4000}}, // fmap_table2[MAP/tps no = 0], kPa x 10 , use for AFR
    {-400,                      // temp_table[TEMP no = 0],  deg x 10
     -200, 0, 200, 400, 600, 800, 1000, 1300, 1600},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // pad spare

    {60, 80, 100, 120, 140, 160},       // deltaV_table[], Vx10 = batt_voltx10 - 120
    {250, 124, 84, 64, 51, 44}, // deltaDur_table[], %age correction for batt_volt. 50 = 100%
// MS1/Extra uses these
//dwelltv: db     51T,68T,85T,102T,119T,136T    ; 6v,8v,10v,12v,14v,16v
//dwelltf: db     250T,124T,84T,64T,51T,44T
//Values in table are /4 (i.e. 250 = 250/256*4 = x 3.9)

    {60,                        // cold_adv_table[TEMP no = 0], deg x 10
     50, 40, 30, 20, 10, 0, 0, 0, 0},

    {60, 54, 49, 44, 38, 32, 27, 21, 13, 5},    // pwm idle table

    {{2200, 1720, 1240, 760, 280, 0},   // opentime0
     {2200, 1720, 1240, 760, 280, 0},
     {2200, 1720, 1240, 760, 280, 0},
     {2200, 1720, 1240, 760, 280, 0}},
    {72, 96, 120, 144, 168, 192},       //opentimev

    {{0, 400, 800, 1200, 1600, 2000},   // smallopen
     {0, 400, 800, 1200, 1600, 2000},
     {0, 400, 800, 1200, 1600, 2000},
     {0, 400, 800, 1200, 1600, 2000}},
    {0, 400, 800, 1200, 1600, 2000},    //smallopenpw

    {1000, 2000, 2500, 3000, 3500, 4000, 5000, 6000},   // water injection
    {1000, 2000, 3000, 4000},
    {{16, 32, 40, 48, 56, 64, 80, 100},
     {16, 32, 40, 48, 56, 64, 80, 100},
     {16, 32, 40, 48, 56, 64, 80, 100},
     {16, 32, 40, 48, 56, 64, 80, 100}},

    {0, 0}                      //2 spares
};

const page11_data dflash11 CNFDATA_ATTR = {
    {                           //spk trim tables (spk_trim)
     {                          //spk a
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk b
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk c
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk d
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk e
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk f
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk g
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk h
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk i
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk j
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk k
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk l
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk m
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk n
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk o
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      },
     {                          //spk o
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0}
      }
     },

//rpm bins
     {500, 1000, 2000, 4000, 6000, 7000},

//load bins
     {200, 500, 800, 1000, 1500, 2000},

    {300, 300, 300, 300, 300},  // XAccTable[NO_XTRPMS], %x10
    {410, 400, 390, 370, 350},  // TauAccTable[NO_XTRPMS], ms
    {200, 200, 200, 200, 200},  // XDecTable[NO_XTRPMS], %x10
    {510, 500, 490, 470, 450},  // TauDecTable[NO_XTRPMS], ms
    {600, 1000, 2000, 3000, 6000},      // XTrpms[NO_XTRPMS]:rpms for above tables (idle to max)
    {160, 150, 140, 130, 125, 120, 115, 110, 105, 100}, // XClt[NO_TEMPS], %       
    {200, 190, 180, 180, 170, 160, 140, 125, 110, 100}, // TauClt[NO_TEMPS], %
    {-400, -200, 0, 200, 400, 600, 800, 1200, 1400, 1600},
    {-400, -200, 0, 200, 400, 600, 800, 1200, 1400, 1600},

    {0,  1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000},  /* Launch retard time */
    {100,  75,   50,   37,   25,    0,    0,    0,    0,    0},  /* retard */

    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0}, /* spare */

    {{0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 5, 5, 10, 20, 30},
     {0, 0, 0, 5, 10, 20, 30, 40},
     {0, 0, 0, 10, 20, 30, 40, 50},
     {0, 0, 10, 20, 30, 40, 50, 60},
     {0, 0, 10, 30, 50, 70, 90, 100},
     {0, 0, 10, 30, 60, 80, 100, 100},
     {0, 0, 10, 30, 60, 80, 100, 100}},
    {500, 1000, 2000, 3000, 4000, 5000, 6000, 7000},
    {300, 400, 500, 600, 700, 800, 900, 1000},
    {    // MAFFlow[NO_MAFS], MAF flows (mg/ secx10) for below corrections.
  100, 2000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 18000, 20000, 25000 }, 
    {    // MAFCor[NO_MAFS], Corrections to maf factor table (%) for real time tuning/ 
  100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
    {0, 1000, 2000, 3000, 4000, 5000},  // Launch retard time - obsolete
    {100, 50, 25, 0, 0, 0},         // retard - obsolete
    {0, 20, 40, 60, 80, 120},  // cranktaper_time
    {1000, 1000, 1000, 1000, 1000, 1000},         // cranktaper_pct

//pad bytes
    {0, 0, 0, 0, 0, 0}
};

const page12_data dflash12 CNFDATA_ATTR = {
    // ** VE TABLE 1 ** //
    {{260, 260, 290, 340, 400, 460, 500, 550, 580, 610, 610, 600, 590, 590, 580, 580},
     {380, 370, 390, 440, 530, 610, 650, 670, 690, 730, 710, 660, 640, 620, 600, 580},
     {430, 420, 430, 450, 540, 630, 660, 690, 710, 750, 730, 670, 650, 630, 610, 590},
     {480, 460, 480, 500, 580, 650, 690, 710, 730, 770, 750, 690, 670, 650, 630, 610},
     {520, 510, 520, 550, 620, 670, 710, 730, 750, 790, 770, 710, 690, 670, 650, 630},
     {570, 590, 610, 650, 690, 720, 760, 780, 810, 850, 850, 800, 780, 760, 740, 720},
     {610, 620, 650, 690, 720, 750, 790, 820, 850, 890, 880, 840, 820, 800, 780, 760},
     {650, 660, 690, 730, 760, 780, 820, 860, 900, 930, 920, 880, 860, 840, 820, 800},
     {680, 700, 730, 780, 810, 830, 860, 900, 940, 980, 970, 930, 910, 890, 870, 850},
     {720, 770, 820, 870, 900, 930, 950, 1000, 1050, 1090, 1080, 1030, 1010, 990, 970, 950},
     {740, 820, 870, 920, 960, 980, 1010, 1060, 1120, 1150, 1140, 1090, 1070, 1050, 1030, 1010},
     {750, 830, 880, 930, 970, 990, 1020, 1070, 1130, 1160, 1150, 1100, 1080, 1060, 1040, 1020},
     {780, 850, 900, 950, 990, 1010, 1040, 1090, 1150, 1180, 1170, 1120, 1100, 1080, 1060, 1004},
     {820, 890, 945, 1000, 1040, 1060, 1090, 1145, 1210, 1240, 1230, 1180, 1155, 1135, 1115, 1054},
     {860, 935, 945, 1000, 1040, 1110, 1090, 1200, 1265, 1300, 1290, 1230, 1210, 1190, 1165, 1105},
     {900, 980, 1035, 1095, 1140, 1160, 1200, 1255, 1325, 1360, 1345, 1290, 1265, 1240, 1220, 1155}
      },

    // ** VE TABLE 2 ** //
    {{260, 260, 290, 340, 400, 460, 500, 550, 580, 610, 610, 600, 590, 590, 580, 580},
     {380, 370, 390, 440, 530, 610, 650, 670, 690, 730, 710, 660, 640, 620, 600, 580},
     {430, 420, 430, 450, 540, 630, 660, 690, 710, 750, 730, 670, 650, 630, 610, 590},
     {480, 460, 480, 500, 580, 650, 690, 710, 730, 770, 750, 690, 670, 650, 630, 610},
     {520, 510, 520, 550, 620, 670, 710, 730, 750, 790, 770, 710, 690, 670, 650, 630},
     {570, 590, 610, 650, 690, 720, 760, 780, 810, 850, 850, 800, 780, 760, 740, 720},
     {610, 620, 650, 690, 720, 750, 790, 820, 850, 890, 880, 840, 820, 800, 780, 760},
     {650, 660, 690, 730, 760, 780, 820, 860, 900, 930, 920, 880, 860, 840, 820, 800},
     {680, 700, 730, 780, 810, 830, 860, 900, 940, 980, 970, 930, 910, 890, 870, 850},
     {720, 770, 820, 870, 900, 930, 950, 1000, 1050, 1090, 1080, 1030, 1010, 990, 970, 950},
     {740, 820, 870, 920, 960, 980, 1010, 1060, 1120, 1150, 1140, 1090, 1070, 1050, 1030, 1010},
     {750, 830, 880, 930, 970, 990, 1020, 1070, 1130, 1160, 1150, 1100, 1080, 1060, 1040, 1020},
     {780, 850, 900, 950, 990, 1010, 1040, 1090, 1150, 1180, 1170, 1120, 1100, 1080, 1060, 1004},
     {820, 890, 945, 1000, 1040, 1060, 1090, 1145, 1210, 1240, 1230, 1180, 1155, 1135, 1115, 1054},
     {860, 935, 945, 1000, 1040, 1110, 1090, 1200, 1265, 1300, 1290, 1230, 1210, 1190, 1165, 1105},
     {900, 980, 1035, 1095, 1140, 1160, 1200, 1255, 1325, 1360, 1345, 1290, 1265, 1240, 1220, 1155}
    },
};

const page13_data dflash13 CNFDATA_ATTR = {
    //spark table 1
    {{200, 200, 310, 335, 350, 360, 370, 380, 380, 380, 380, 380, 380, 380, 380, 380},
     {190, 200, 300, 330, 350, 340, 370, 380, 380, 380, 380, 380, 380, 380, 380, 380},
     {185, 200, 295, 325, 340, 340, 360, 370, 370, 370, 370, 370, 370, 370, 370, 370},
     {180, 200, 290, 320, 330, 340, 350, 360, 360, 360, 360, 360, 360, 360, 360, 360},
     {170, 200, 280, 300, 310, 320, 330, 340, 340, 340, 340, 340, 340, 340, 340, 340},
     {160, 185, 270, 280, 290, 300, 310, 320, 320, 320, 320, 320, 320, 320, 320, 320},
     {150, 180, 250, 260, 270, 280, 290, 300, 300, 300, 300, 300, 300, 300, 300, 300},
     {140, 170, 230, 240, 250, 260, 270, 280, 280, 280, 280, 280, 280, 280, 280, 280},
     {135, 165, 213, 227, 237, 247, 257, 267, 268, 269, 270, 271, 272, 273, 273, 274},
     {130, 159, 196, 215, 225, 235, 245, 255, 255, 260, 260, 262, 264, 266, 267, 268},
     {120, 148, 162, 186, 196, 206, 216, 227, 232, 236, 240, 245, 249, 252, 254, 256},
     {110, 134, 156, 193, 203, 213, 223, 234, 236, 238, 240, 243, 245, 246, 247, 248},
     {100, 120, 150, 200, 210, 220, 230, 240, 240, 240, 240, 240, 240, 240, 240, 240},
     {85, 100, 108, 135, 143, 150, 158, 165, 165, 165, 165, 165, 165, 165, 165, 165},
     {50, 50, 65, 70, 75, 80, 85, 90, 90, 90, 90, 90, 90, 90, 90, 90},
     {50, 50, 65, 70, 75, 80, 85, 90, 90, 90, 90, 90, 90, 90, 90, 90}
    },

    //spark table 2
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};

const page18_data dflash18 CNFDATA_ATTR = {
    // ** VE TABLE 3 ** //
    {{260, 260, 290, 340, 400, 460, 500, 550, 580, 610, 610, 600, 590, 590, 580, 580},
     {380, 370, 390, 440, 530, 610, 650, 670, 690, 730, 710, 660, 640, 620, 600, 580},
     {430, 420, 430, 450, 540, 630, 660, 690, 710, 750, 730, 670, 650, 630, 610, 590},
     {480, 460, 480, 500, 580, 650, 690, 710, 730, 770, 750, 690, 670, 650, 630, 610},
     {520, 510, 520, 550, 620, 670, 710, 730, 750, 790, 770, 710, 690, 670, 650, 630},
     {570, 590, 610, 650, 690, 720, 760, 780, 810, 850, 850, 800, 780, 760, 740, 720},
     {610, 620, 650, 690, 720, 750, 790, 820, 850, 890, 880, 840, 820, 800, 780, 760},
     {650, 660, 690, 730, 760, 780, 820, 860, 900, 930, 920, 880, 860, 840, 820, 800},
     {680, 700, 730, 780, 810, 830, 860, 900, 940, 980, 970, 930, 910, 890, 870, 850},
     {720, 770, 820, 870, 900, 930, 950, 1000, 1050, 1090, 1080, 1030, 1010, 990, 970, 950},
     {740, 820, 870, 920, 960, 980, 1010, 1060, 1120, 1150, 1140, 1090, 1070, 1050, 1030, 1010},
     {750, 830, 880, 930, 970, 990, 1020, 1070, 1130, 1160, 1150, 1100, 1080, 1060, 1040, 1020},
     {780, 850, 900, 950, 990, 1010, 1040, 1090, 1150, 1180, 1170, 1120, 1100, 1080, 1060, 1004},
     {820, 890, 945, 1000, 1040, 1060, 1090, 1145, 1210, 1240, 1230, 1180, 1155, 1135, 1115, 1054},
     {860, 935, 945, 1000, 1040, 1110, 1090, 1200, 1265, 1300, 1290, 1230, 1210, 1190, 1165, 1105},
     {900, 980, 1035, 1095, 1140, 1160, 1200, 1255, 1325, 1360, 1345, 1290, 1265, 1240, 1220, 1155}
    },
    //spark table 3
    {{200, 200, 310, 335, 350, 360, 370, 380, 380, 380, 380, 380, 380, 380, 380, 380},
     {190, 200, 300, 330, 350, 340, 370, 380, 380, 380, 380, 380, 380, 380, 380, 380},
     {185, 200, 295, 325, 340, 340, 360, 370, 370, 370, 370, 370, 370, 370, 370, 370},
     {180, 200, 290, 320, 330, 340, 350, 360, 360, 360, 360, 360, 360, 360, 360, 360},
     {170, 200, 280, 300, 310, 320, 330, 340, 340, 340, 340, 340, 340, 340, 340, 340},
     {160, 185, 270, 280, 290, 300, 310, 320, 320, 320, 320, 320, 320, 320, 320, 320},
     {150, 180, 250, 260, 270, 280, 290, 300, 300, 300, 300, 300, 300, 300, 300, 300},
     {140, 170, 230, 240, 250, 260, 270, 280, 280, 280, 280, 280, 280, 280, 280, 280},
     {135, 165, 213, 227, 237, 247, 257, 267, 268, 269, 270, 271, 272, 273, 273, 274},
     {130, 159, 196, 215, 225, 235, 245, 255, 255, 260, 260, 262, 264, 266, 267, 268},
     {120, 148, 162, 186, 196, 206, 216, 227, 232, 236, 240, 245, 249, 252, 254, 256},
     {110, 134, 156, 193, 203, 213, 223, 234, 236, 238, 240, 243, 245, 246, 247, 248},
     {100, 120, 150, 200, 210, 220, 230, 240, 240, 240, 240, 240, 240, 240, 240, 240},
     {85, 100, 108, 135, 143, 150, 158, 165, 165, 165, 165, 165, 165, 165, 165, 165},
     {50, 50, 65, 70, 75, 80, 85, 90, 90, 90, 90, 90, 90, 90, 90, 90},
     {50, 50, 65, 70, 75, 80, 85, 90, 90, 90, 90, 90, 90, 90, 90, 90}
    }
};

const page19_data dflash19 CNFDATA_ATTR = {
    {500, 800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // VE1 rpms
    {500, 800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // VE2 rpms
    {500, 800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // VE3 rpms

    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},  // VE1 loads
    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},  // VE2 loads
    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},  // VE3 loads

    {700, 900, 1200, 1500, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // adv1 rpms
    {700, 900, 1200, 1500, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // adv2 rpms
    {700, 900, 1200, 1500, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // adv3 rpms

    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},    // adv1 loads
    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},    // adv2 loads
    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},    // adv3 loads
    {800, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000},    // AWCRPM
    {800, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000},    // SOCRPM
    {300, 350, 400, 450, 500, 550, 600, 650, 700, 800, 900, 1000},      //AWCKPA
    {300, 350, 400, 450, 500, 550, 600, 650, 700, 800, 900, 1000},      //SOCKPA
    {22, 28, 30, 32, 38, 40, 42, 48, 50, 52, 58, 60},   //BAWC in 1% units
    {22, 28, 30, 32, 38, 40, 42, 48, 50, 52, 58, 60},   //BSOC in 0.1% units
    {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45},  //AWN
    {45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100},  //SWN
    {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
    {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
    {0, 200, 400, 600, 800, 1000, 1200, 1300, 1400, 1600, 1700, 1800},  // for EAE
    {0, 200, 400, 600, 800, 1000, 1200, 1300, 1400, 1600, 1700, 1800},  // for EAE
    {500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500}, /* ITB_load_loadvals */
    {50, 77, 104, 158, 185, 236, 266, 293, 320, 347, 370, 390}, /* ITB_load_switchpoints */
    {800,1454,2109,3418,4072,5381,6036,6690,7345,8000,8500,9000}, /* ITB_load_rpms */
    {{300,320,340,360},
     {320,340,360,380},
     {340,360,380,400},
     {360,380,400,420}},
    {{300,320,340,360},
     {320,340,360,380},
     {340,360,380,400},
     {360,380,400,420}},
    {{300,350,400,450},
     {300,350,400,450}},
    {{800,900,1000,1100},
     {800,900,1000,1100}},

    {0,0,0,0,0,0,0,0,0,0}, // TC
    {0,0,0,0,0,0,0,0,0,0},
    {250,500,750,1000},
    {30,60,120,200},
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0},
    {25,50,75,100},

    {500,1000,1500,2000,2500,3000,4000,5000,6000,7000},
    {500,500,500,500,500,500,500,500,500,500},
    {0,0,0,0,0,0,0,0,0,0},
    {200,200,200,200,200,200,200,200,200,200},
    {800, 1000, 1200, 1400, 1600},
    {200, 500, 800, 1100, 1400},
    {{0,0,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    0, // align
    {400, 800, 1200, 1600}, // knock_clt
    {2500, 2000, 1500, 1000}, // knock_upscale
    {0,0,0,0,0,0,0,0,0,0}, // TC
    {-5,-10,-15,-20},

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}    // 18


};
const page21_data dflash21 CNFDATA_ATTR = {
   {{{500,1000,2000,4000,6000,8000}, {20,40,60,80,90,100}},
    {{500,1000,2000,4000,6000,8000}, {20,40,60,80,90,100}},
    {{500,1000,2000,4000,6000,8000}, {20,40,60,80,90,100}},
    {{500,1000,2000,4000,6000,8000}, {20,40,60,80,90,100}},
    {{500,1000,2000,4000,6000,8000}, {20,40,60,80,90,100}},
    {{500,1000,2000,4000,6000,8000}, {20,40,60,80,90,100}}},
    {{{0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0}},
     {{0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0}},
     {{0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0}},
     {{0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0}},
     {{0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0}},
     {{0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0},
      {0,0,0,0,0,0}}},
    {60, 56, 52, 48, 44, 40, 36, 32, 26, 20},   // CWPrime2, (msx10)
    {325, 300, 275, 250, 225, 200, 175, 150, 125, 100}, // CrankPctTable2,   (%age)
    {45, 43, 41, 39, 37, 35, 33, 31, 28, 25},   // CWAWEV2,  %
    {350, 330, 310, 290, 270, 250, 230, 210, 180, 150}, // CWAWC2,   cycles
    {180, 180, 160, 150, 135, 125, 113, 108, 102, 100}, // warmen_table2[TEMP no = 0],   % enrichment vs temp
    {100, 300, 500, 700, 900, 1100, 1300, 1500, 1700, 1800},    /* temp_table_p21 */
    {500, 800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // VE4 rpms
    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},  // VE4 loads
    {700, 900, 1200, 1500, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000, 6500, 7000, 7200, 7500},     // adv4 rpms
    {300, 400, 450, 500, 550, 600, 650, 700, 750, 800, 900, 950, 1000, 2000, 3000, 4000},    // adv4 loads
    {100, 300, 500, 700, 900, 1100, 1300, 1500, 1700, 1800}, // dualfuel temp
    {0,0,0,0,0,0,0,0,0,0}, // adj
    {2200,2400,2600,2800,3000,3200,3400,3600,3800,4000}, // pressure
    {0,0,0,0,0,0,0,0,0,0}, // adj
    {{3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600},
    {3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600, 3600}},
    {200, 250, 300, 400, 450, 500, 600, 700, 750, 800, 900, 1000}, 
    {300, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000, 6000, 6500,
     7000},

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // 10 spares 
};
const page22_data dflash22 CNFDATA_ATTR = {
    // ** VE TABLE 4 ** //
    {{260, 260, 290, 340, 400, 460, 500, 550, 580, 610, 610, 600, 590, 590, 580, 580},
     {380, 370, 390, 440, 530, 610, 650, 670, 690, 730, 710, 660, 640, 620, 600, 580},
     {430, 420, 430, 450, 540, 630, 660, 690, 710, 750, 730, 670, 650, 630, 610, 590},
     {480, 460, 480, 500, 580, 650, 690, 710, 730, 770, 750, 690, 670, 650, 630, 610},
     {520, 510, 520, 550, 620, 670, 710, 730, 750, 790, 770, 710, 690, 670, 650, 630},
     {570, 590, 610, 650, 690, 720, 760, 780, 810, 850, 850, 800, 780, 760, 740, 720},
     {610, 620, 650, 690, 720, 750, 790, 820, 850, 890, 880, 840, 820, 800, 780, 760},
     {650, 660, 690, 730, 760, 780, 820, 860, 900, 930, 920, 880, 860, 840, 820, 800},
     {680, 700, 730, 780, 810, 830, 860, 900, 940, 980, 970, 930, 910, 890, 870, 850},
     {720, 770, 820, 870, 900, 930, 950, 1000, 1050, 1090, 1080, 1030, 1010, 990, 970, 950},
     {740, 820, 870, 920, 960, 980, 1010, 1060, 1120, 1150, 1140, 1090, 1070, 1050, 1030, 1010},
     {750, 830, 880, 930, 970, 990, 1020, 1070, 1130, 1160, 1150, 1100, 1080, 1060, 1040, 1020},
     {780, 850, 900, 950, 990, 1010, 1040, 1090, 1150, 1180, 1170, 1120, 1100, 1080, 1060, 1004},
     {820, 890, 945, 1000, 1040, 1060, 1090, 1145, 1210, 1240, 1230, 1180, 1155, 1135, 1115, 1054},
     {860, 935, 945, 1000, 1040, 1110, 1090, 1200, 1265, 1300, 1290, 1230, 1210, 1190, 1165, 1105},
     {900, 980, 1035, 1095, 1140, 1160, 1200, 1255, 1325, 1360, 1345, 1290, 1265, 1240, 1220, 1155}
    },
    //spark table 4
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};
const page23_data dflash23 CNFDATA_ATTR = {
//generic spare ports
/*
    char spr_port[NPORT], condition1[NPORT], condition2[NPORT], cond12[NPORT],
         init_val[NPORT], port_val[NPORT],
         out_byte1[NPORT], out_byte2[NPORT]; // These are all a gross waste of memory
    int out_offset1[NPORT], out_offset2[NPORT],
        thresh1[NPORT], thresh2[NPORT], hyst1[NPORT], hyst2[NPORT];
*/
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},   // spr_port
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
     '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
     '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
     '>', '>', '>'},  // condition1
    {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
     '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
     '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>',
     '>', '>', '>'},  // condition2
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
     ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
     ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
     ' ', ' ', ' '},   // cond12
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // init_val
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // port_val
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // out_byte1
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     2, 2, 2},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // out_byte2
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},
    {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // out_offset1
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     6, 6, 6},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // out_offset2
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // thresh1
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // thresh2
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // hyst1
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // hyst2
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0},   // end of spare ports

    {0, 0, 0, 0 }    // spares
};
const page24_data dflash24 CNFDATA_ATTR = {
    //narrowband_tgts
    {{102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102},
     {102,102,102,102,102,102,102,102,102,102,102,102}},
    {800,1400,2000,2600,3200,3800,4400,5000,5600,6200,6800,7500},
    {200,272,346,419,491,563,635,707,780,852,924,1000},

    0, 40, 0, 300, 150, 0, 0, 75, // ALS anti-lag  (3rd one was als_min_tps)
    100,
    20, // vvt_slew
    1220, 2300, // 
    {1300, 1600, 1900, 2200, 2500, 2800},
    {0,40, 80, 120, 160, 200},
    {{-300, -280, -260, -240, -220, -200}, // ALS timing
     {-300, -280, -260, -240, -220, -200},
     {-300, -280, -260, -240, -220, -200},
     {-300, -280, -260, -240, -220, -200},
     {-300, -280, -260, -240, -220, -200},
     {-300, -280, -260, -240, -220, -200}},
    {{250, 250, 250, 250, 250, 250}, // ALS fuel add
     {250, 250, 250, 250, 250, 250},
     {250, 250, 250, 250, 250, 250},
     {250, 250, 250, 250, 250, 250},
     {250, 250, 250, 250, 250, 250},
     {250, 250, 250, 250, 250, 250}},
    {{80, 80, 80, 80, 80, 80}, // ALS cyclic fuel cut
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80}},
    {{80, 80, 80, 80, 80, 80}, // ALS cyclic spark cut
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80},
     {80, 80, 80, 80, 80, 80}},
    500, 3000, 1700, // mix/max RPM, max MAT

    0,
    0,127,{0,0,0,0}, // VVT
    10,
    50,50,50, // VVTint PID
    127,
    {0,0,0,0}, {0,0,0,0},
    0x6,0,
    {200,400,500,600,700,800,900,1000},
    {500,1000,2000,3000,4000,5000,6000,7000},
    {{{0,0,0,0,0,0,0,0}, // VVT timing table 1
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0}},
     {{0,0,0,0,0,0,0,0}, // VVT timing table 2
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0}}},
    {1,1,1,1}, // VVT teeth
    {0,0}, // vvt_coldpos
    0, 0, 0, 0, 4, 40,    // TCLU torque convertor lockup
    135, 50, 800, 300, 900,
    0, 0, //vvt_opt4,5
    127, // vvt_hold_duty_exh
    50, 50, 50, // vvt_ctl_Kp_exh ...
    0,0,0,0,0,0,0,0, // start teeth
    8, 0, // vvt_opt6, 7
    {{60,50,40,20,0,0}, // ALS roving idle
     {30,40,30,10,0,0},
     {20,30,20,10,0},
     {10,20,10,0,0,0},
     {0,0,0,0,0,0},
    {0,0,0,0,0,0}},
    {800, 1000, 1200, 1400, 1600, 1800},
    {0,20, 40, 60, 80, 100},
    0x1c, 1,    // ALS PWM
    -400, // vvt_minclt
    0, 255  // min/max duty
};

const page25_data dflash25 CNFDATA2_ATTR = {  // NB cnfdata2

    /* MAF calib volts *1000 mafflow */
    {0, 80, 160, 230, 310, 390, 470, 550,
     630, 700, 780, 860, 940, 1020, 1090, 1170,
     1250, 1330, 1410, 1490, 1560, 1640, 1720, 1800,
     1880, 1960, 2030, 2110, 2190, 2270, 2350, 2420,
     2500, 2580, 2660, 2740, 2820, 2890, 2970, 3050,
     3130, 3210, 3280, 3360, 3440, 3520, 3600, 3680,
     3750, 3830, 3910, 3990, 4070, 4140, 4220, 4300,
     4380, 4460, 4540, 4610, 4690, 4770, 4850, 4930},
    /* MAF calib flow in g/sec * 1000 */
/*    {0, 50, 100, 151, 202, 254, 307, 361,
     417, 477, 549, 632, 721, 813, 909, 1008,
     1102, 1193, 1292, 1426, 1616, 1865, 2101, 2276,
     2422, 2602, 2816, 3051, 3310, 3592, 3895, 4233,
     4630, 5049, 5427, 5805, 6215, 6649, 7099, 7565,
     8055, 8574, 9119, 9687, 10271, 10869, 11479, 12101,
     12737, 13387, 14053, 14739, 15451, 16195, 16978, 17790,
     18612, 19420, 20214, 21056, 22017, 23168, 24556, 26122},
*/
    /* MAF calib flow in g/sec * 1000. Zero offset is 8192. */
    {8192, 8242, 8292, 8343, 8394, 8446, 8499, 8553,
     8609, 8669, 8741, 8824, 8913, 9005, 9101, 9200,
     9294, 9385, 9484, 9618, 9808, 10057, 10293, 10468,
     10614, 10794, 11008, 11243, 11502, 11784, 12087, 12425,
     12822, 13241, 13619, 13997, 14407, 14841, 15291, 15757,
     16247, 16766, 17311, 17879, 18463, 19061, 19671, 20293,
     20929, 21579, 22245, 22931, 23643, 24387, 25170, 25982,
     26804, 27612, 28406, 29248, 30209, 31360, 32748, 34314},

    {{1000,400,350,300,250,200}, // alpha_map_table[tps no =0][RPM nos 0-5] (kpa x 10)
     {1000,600,400,300,250,250},  // alpha_map_table[tps no =1][RPM nos 0-5]
     {1000,1000,800,600,500,400},  // alpha_map_table[tps no =2][RPM nos 0-5]
     {1000,1000,1000,900,750,600},  // alpha_map_table[tps no =3][RPM nos 0-5]
     {1000,1000,1000,1000,1000,800},  // alpha_map_table[tps no =4][RPM nos 0-5]
     {1000,1000,1000,1000,1000,1000}}, // alpha_map_table[tps no =5][RPM nos 0-5]
    {0,200,400,600,800,1000},     // amap_tps[0 - 5] (% x 10);
    {0,1000,2000,3000,4000,6000},  // amap_rpm[0 - 5]

    {100,200,400,600,800,1000},     // tpswot_tps[0 - 5] (% x 10);
    {0,1000,2000,3000,4000,6000},  // tpswot_rpm[0 - 5]
    { 400, 600, 1000, 1300, 1600, 1800},  // MatVals[NO_MATS]: air temperatures (degx10) for air density correction table. old version.
    {0, 0, 0, 0, 0, 0}, // MAT correction aircor. old version     
    {{100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100}},
    {{100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100},
     {100,100,100,100,100,100,100,100}},
    {1500, 2500, 3500, 4500, 5000, 5500, 6000, 6500},
    {1500, 2500, 3500, 4500, 5000, 5500, 6000, 6500},
    {1000, 1100, 1300, 1400, 1500, 1600, 1700, 1800},
    {1000, 1100, 1300, 1400, 1500, 1600, 1700, 1800},
    {{0, 12, 25, 38, 50, 62, 75, 88, 100}, // blendx
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {100, 194, 288, 382, 475, 569, 663, 757, 850}},
    {{0, 12, 25, 38, 50, 62, 75, 88, 100}, // blendy
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100},
     {0, 12, 25, 38, 50, 62, 75, 88, 100}},
    
    {{64, 64, 64, 64, 64, 64}, // fpd_duty
     {64, 64, 64, 64, 64, 64},
     {64, 64, 64, 90, 100, 100},
     {80, 100, 130, 150, 150, 200},
     {100, 150, 170, 255, 255, 255},
     {128, 200, 200, 255, 255, 255}},

    {100,200,400,600,800,1000},     // fpd_load[0 - 5] (% x 10);
    {0,1000,2000,3000,4000,6000},  // fpd_rpm[0 - 5]

    {100, 300, 500, 700, 900, 1100, 1300, 1500, 1700, 1800}, // fp_temps
    {0,0,0,0,0,0,0,0,0,0}, // adj
    {2200,2400,2600,2800,3000,3200,3400,3600,3800,4000}, // fp_presss
    {0,0,0,0,0,0,0,0,0,0}, // adj

    {0, 70, 100, 150, 200, 250}, // hpte time
    {0,  0,   5,  10,  13,  18}, // 0.1 AFR increase

    {0,500,1000,2000,5000,8000}, // oil pressure rpms
    {0,0,0,0,0,0}, // oil min
    {0,0,0,0,0,0}, // oil max
    {0,16,33,50,66,83,100},
    {0,16,33,50,66,83,100},

    {0}
};

const page27_data dflash27 CNFDATA2_ATTR = {
    /* ego_auth_table */
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
     {0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
     {0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
     {0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
     {0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
     {0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
     {0, 0, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70},
     {0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},


    {500, 800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000}, // ego_auth_rpms
    {300, 400, 500, 550, 600, 700, 800, 900, 1000, 2000, 3000, 4000}, // ego_auth_loads
    {0,8,16,24,32,40,48,56,64,72}, // vsslaunch_vss
    {7000,7000,7000,7000,7000,7000,7000,7000,7000,7000}, // vsslaunch_rpm
    {0,0,0,0,0,0,0,0,0,0}, // vsslaunch_retard
    {{10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},// ego_delay_table
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10}},
    {500, 800, 1100, 1400, 2000, 2600, 3100, 3700, 4300, 4900, 5400, 6000}, // ego_delay_rpms
    {300, 400, 500, 550, 600, 700, 800, 900, 1000, 2000, 3000, 4000}, // ego_delay_loads
    25, // ego_sensor_delay
    {2,2}, // generic_pid_flags
    {30,30}, // generic_pid_pwm_opts
    {5,6}, // generic_pid_pwm_outs
    {24,24}, // generic_pid_load_offsets
    {2,2}, // generic_pid_load_sizes
    {1000,1000}, // generic_pid_upper_inputlims
    {0,0}, // generic_pid_lower_inputlims
    {252,252}, // generic_pid_output_upperlims
    {3,3}, // generic_pid_output_lowerlims
    {104,106}, // generic_pid_PV_offsets
    {2,2}, // generic_pid_PV_sizes
    {{{500,1000,1500,2000,3000,4000,5000,6000}, // generic_pid_axes3d
      {0,200,300,400,500,600,800,1000}}, 
     {{500,1000,1500,2000,3000,4000,5000,6000},
      {0,200,300,400,500,600,800,1000}}},
    {{{0,0,0,0,0,0,0,0}, // generic_pid_targets
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0}},
     {{0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0}}},
     {100,100}, // generic_pid_control_intervals
     {100,100},
     {100,100},
     {100,100},
    0,
    {0, 128, 256, 384, 511, 634, 768, 900, 1023}, // tcslipx
    {0, 12, 25, 38, 50, 62, 75, 88, 100}, // tcslipy
    0,
    {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  // 16 offsets; Message 1
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}, // 16 sizes
     {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  // 16 offsets; Message 2
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}, // 16 sizes
     {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  // 16 offsets; Message 3
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}, // 16 sizes
     {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  // 16 offsets; Message 4
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}}, // 16 sizes
};

const page28_data dflash28 CNFDATA3_ATTR = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //1 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //2 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //3 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //4 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //5 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //6 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //7 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //8 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //9 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //10 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //11 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //12 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //13 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //14 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //15 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //16 16
     0, 0, 0, 0, 0, 0 }, /* reserved 262 bytes */

    {{25,25,25,25,25,25,25,25}, /* dwell_table_value */
     {25,25,25,25,25,25,25,25},
     {25,25,25,25,25,25,25,25},
     {25,25,25,25,25,25,25,25},
     {25,25,25,25,25,25,25,25},
     {25,25,25,25,25,25,25,25},
     {25,25,25,25,25,25,25,25},
     {25,25,25,25,25,25,25,25}},
    {500, 1000, 1500, 2000, 3000, 4000, 6000, 8000},
    {200, 400, 600, 800, 1000, 1500, 2000, 2550},

     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* can_rcv_id[] */
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* can_rcv_var[] */
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* can_rcv_off[] */
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* can_rcv_size[] */
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* can_rcv_mult[] */
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* can_rcv_div[] */
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* can_rcv_add[] */
    1234, {0,0,0,0,0,0,0,0}, /* can_bcast_user */

    {0x00,0x54,0xa8,0xe8}, /* IO-box configuration */
    {0x54,0x14,0x14,0x14},
    {512,544,576,608},
    0, 0, /* Pit limiter */
    45, 300, 100, 50, 200,
    {6,                         // was tpsen_table[TPS_DOT no = 0], enrichment in .1ms vs tpsdot
     15, 30, 40},
    {0,                         // was mapen_table[TPS_DOT no = 0], enrichment in .1ms vs mapdot
     0, 0, 0},
    {2100,                      // tpsdot_table[TPS_DOT no =0],
     4000, 8000, 10000},        //    change in % x 10 per .1 sec
    {0,                         //  mapdot_table[TPS_DOT no =0],
     0, 0, 0},                  //    change in kPa x 10 per .1 sec
    {6,                         // was tpsen_table[TPS_DOT no = 0], enrichment in .1ms vs tpsdot
     15, 30, 40},
    {0,                         // was mapen_table[TPS_DOT no = 0], enrichment in .1ms vs mapdot
     0, 0, 0},
    {2100,                      // tpsdot_table[TPS_DOT no =0],
     4000, 8000, 10000},        //    change in % x 10 per .1 sec
    {0,                         //  mapdot_table[TPS_DOT no =0],
     0, 0, 0},                  //    change in kPa x 10 per .1 sec
    {-500, -200, -100, -50, 50, 200, 500, 1000}, //accel_mapdots
    {-5000, -2000, -1000, -500, 500, 2000, 5000, 10000}, // accel_tpsdots
    {0, 0, 0, 0, 10, 100, 1000, 1200}, // accel_mapdot_amts2
    {0, 0, 0, 0, 10, 100, 1000, 1200}, // accel_tpsdot_amts2
    {-500, -200, -100, -50, 50, 200, 500, 1000}, //accel_mapdots2
    {-5000, -2000, -1000, -500, 500, 2000, 5000, 10000}, // accel_tpsdots2
    {0, 0, 0, 0, 10, 100, 1000, 1200}, // accel_mapdot_amts2
    {0, 0, 0, 0, 10, 100, 1000, 1200}, // accel_tpsdot_amts2
    1512, 0x20, /* dashbcast */

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* spares */
     0 },
    
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //1 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //2 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //3 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //4 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //5 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //6 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //7 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //8 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //9 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //10 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //11 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //12 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //13 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //14 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //15 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //16 16
    } /* Reserved 256 bytes */
};

const page29_data dflash29 CNFDATA3_ATTR = {
    {{{1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000}, // boost_dome_targets
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000}},
      {{1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000},
       {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000}}},
    {{1500, 2180, 2860, 3540, 4220, 4900, 5580, 6260}, // boost_dome_target_rpms
     {1500, 2180, 2860, 3540, 4220, 4900, 5580, 6260}},
    {{0, 140, 280, 420, 560, 700, 840, 1000}, // boost_dome_target_tps
     {0, 140, 280, 420, 560, 700, 840, 1000}},
    {1000,1000},{1000,1000},{1000,1000}, // boost_dome_Kp ...
    {0, 0}, // boost_dome_outputs_fill
    {0x2,0x2}, // boost_dome_settings
    {0, 0}, {1000, 1000}, // boost_dome_empty_out_min/max
    200, // boost_dome_sensitivity
    {0,    1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000}, /* Progressive nitrous 1 time */
    {3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500}, /* rpm */
    {0,     100,  200,  300,  400, 500,   600,  700,  800,  900}, /* vss */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* duty */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* pw */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* retard */
    {5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000, 9500}, /* Progressive nitrous 2 time */
    {3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500}, /* rpm */
    {0,     100,  200,  300,  400, 500,   600,  700,  800,  900}, /* vss */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* duty */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* pw */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* retard */

    {130, 133, 136, 140, 143, 147}, // alternator 
    {250, 190, 154, 125, 110, 102}, // Ford freq defaults. 13.0V=250Hz, 14.0V=125Hz, 14.7V=102Hz
    {-400, -120, 160, 440, 720, 1000},
    {148, 146, 144, 142, 140, 138},

    {110, 118, 128, 137, 146, 155}, // GM RVC L wire duty cycles. (0-5V signal @128Hz)
    {10, 26, 42, 58, 74, 90},

    {2000, 2000}, // boost_dome_sensitivities
    {0, 0}, {1000, 1000}, //boost_dome_out_mins, boost_dome_out_maxs
    {0, 0}, {0, 0}, {0, 0}, //boost_dome_freqs, boost_dome_inputs, boost_dome_outputs_empty


    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //1 16 pad
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //2 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //3 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //4 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //5 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //6 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //7 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //8 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //9 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //10 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //11 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //12 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //13 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //14 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //15 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //16 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //17 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //18 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //19 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //20 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //21 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //22 16
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //23 16
     0, 0, 0, 0, 0, 0, 0, 0}
};
#endif
