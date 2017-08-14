/* $Id: ms3_main.c,v 1.387.4.5 2015/02/11 11:24:47 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * main()
    Origin: Al Grippo
    Major: Re-write, split up. James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
/*************************************************************************
 **************************************************************************
 **   M E G A S Q U I R T  II - 2 0 0 4 - V1.000
 **
 **   (C) 2003 - B. A. Bowling And A. C. Grippo
 **
 **   This header must appear on all derivatives of this code.
 **
 ***************************************************************************
 **************************************************************************/

/*************************************************************************
 **************************************************************************
 **   GCC Port
 **
 **   (C) 2004,2005 - Philip L Johnson
 **
 **   This header must appear on all derivatives of this code.
 **
 ***************************************************************************
 **************************************************************************/

/*************************************************************************
 **************************************************************************
 **   MS2/Extra
 **
 **   (C) 2006,2007,2008 - Ken Culver, James Murray (in either order)
 **
 **   This header must appear on all derivatives of this code.
 **
 ***************************************************************************
 **************************************************************************/

#include "ms3.h"
#include "cltfactor.inc"
#include "matfactor.inc"
#include "egofactor.inc"
#include "maffactor.inc"
#include "gmfactor.inc"

// the factor tables are only accessed via global calls which cause a compiler warning
// See memory.x for where these are. See ms3.h and ms3h.inc

#include "ms3_main_vectors.h"
#include "ms3_main_defaults.h"
#include "ms3_main_decls.h"

int main(void)
{
    unsigned int utmp1;
    long lsum_ign = 0, lsum_rot, lsum_fuel1 = 0, lsum_fuel2 = 0;
    char localflags = 0;

    tmp_pw1 = tmp_pw2 = 0;

    /* compile time checks for page sizes */
    ct_assert(sizeof(page4_data) == 1024);
    ct_assert(sizeof(page5_data) == 1024);
    ct_assert(sizeof(page8_data) == 1024);
    ct_assert(sizeof(page9_data) == 1024);
    ct_assert(sizeof(page10_data) == 1024);
    ct_assert(sizeof(page11_data) == 1024);
    ct_assert(sizeof(page12_data) == 1024);
    ct_assert(sizeof(page13_data) == 1024);
    ct_assert(sizeof(page18_data) == 1024);
    ct_assert(sizeof(page19_data) == 1024);
    ct_assert(sizeof(page21_data) == 1024);
    ct_assert(sizeof(page22_data) == 1024);
    ct_assert(sizeof(page23_data) == 1024);
    ct_assert(sizeof(page24_data) == 1024);
    ct_assert(sizeof(page25_data) == 1024);
    ct_assert(sizeof(page27_data) == 1024);
    ct_assert(sizeof(page28_data) == 1024);
    ct_assert(sizeof(page29_data) == 1024);
    /* end compile time checks */

    main_init();                // set up all timers, initialise stuff

    mltimestamp = TCNT;
    //  main loop
    for (;;) {
        if (conf_err != 0) {
            configerror();          // only returns for errors >= 190
        }

        ml_cnt++;
        ml_accum += TCNT - mltimestamp;
        mltimestamp = TCNT;
        if ((ml_accum >= 1000000) || (ml_cnt >= 1000)) {
            outpc.looptime = ml_accum / ml_cnt;    // avg over X loops
            ml_cnt = 0;
            ml_accum = 0;
        }

        stack_watch();

        //reset COP timer (That's Computer Operating Properly, not Coil On Plug...)
        ARMCOP = 0x55;
        ARMCOP = 0xAA;

        //check XGATE
        if ((unsigned char)XGMCTL != 0x81) {
            // XGATE went and did a silly thing - shouldn't happen any more
            XGMCTL = 0xA383; // same as XGMCTL_ENABLE in config_xgate()
        }

        // For debug - check if this setting got randomly changed
        if (ram4.no_skip_pulses > 10) {
            conf_err = 99;
        }

        if ((ram4.opt142 & 0x03) == 1) {
            poll_i2c_rtc();
        } else if ((ram4.opt142 & 0x03) == 0) {
            /* no real RTCC, but accept any set-time requests */
            if (datax1.setrtc_lock == 0x5a) {
                datax1.setrtc_lock = 0;

                datax1.rtc_sec = datax1.setrtc_sec;
                datax1.rtc_min = datax1.setrtc_min;
                datax1.rtc_hour = datax1.setrtc_hour;
                datax1.rtc_day = datax1.setrtc_day;
                datax1.rtc_date = datax1.setrtc_date;
                datax1.rtc_month = datax1.setrtc_month;
                datax1.rtc_year = datax1.setrtc_year;
            } else if (flagbyte9 & FLAGBYTE9_GETRTC) {
                flagbyte9 &= ~FLAGBYTE9_GETRTC;
                /* maintain some time locally */
                datax1.rtc_sec++;

                if (datax1.rtc_sec == 60) {
                    datax1.rtc_sec = 0;
                    datax1.rtc_min++;
                }

                if (datax1.rtc_min == 60) {
                    datax1.rtc_min = 0;
                    datax1.rtc_hour++;
                }

                if (datax1.rtc_hour == 24) {
                    datax1.rtc_hour = 0;
                    datax1.rtc_day++;
                }
                /* let days count up */
            }
        }

        serial();
        do_sdcard();

        if (flagbyte8 & FLAGBYTE8_MODE10) {
            if (((unsigned int)lmms - mode10_time) > 8000) { // 1 second
                flagbyte8 &= ~FLAGBYTE8_MODE10;
            }
        }

        if (flagbyte1 & FLAGBYTE1_EX) { // XGATE re-init
            flagbyte1 &= ~FLAGBYTE1_EX;
            DISABLE_INTERRUPTS;
            __asm__ __volatile__("pshy\n"
                                 "jsr 0xFEF0\n" "puly\n"::"y"(&buf2));
            XGMCTL = 0xA383;
            ENABLE_INTERRUPTS;
        }

        dribble_burn();

        /* Moved this section here 2014-12-28 so sensors do get updated in test mode */
        if (((unsigned int)lmms - adc_lmms) > 78) {   // every 10 ms (78 x .128 ms clk)
            adc_lmms = (unsigned int)lmms;
            // read 10-bit ADC results, convert to engineering units and filter
            next_adc++;
            if (next_adc > 7)
                next_adc = 1;
                do_sensors();
            if ((next_adc == 1) || (next_adc == 2) || (next_adc == 4)) {
                get_adc(next_adc, next_adc);    // get one channel on each pass
            } else if (next_adc >= 6) {
                get_adc(next_adc, next_adc);    // get one channel on each pass
            }
            fuel_sensors();

            if ((flagbyte1 & flagbyte1_tstmode) == 0) { /* Not in test mode. */
                gearpos(); // gear selection
                calcvssdot();
                oilpress();
            }
        }

        if (flagbyte1 & flagbyte1_tstmode) {
            injpwms();
            goto SKIP_THE_LOT;  // in special test mode don't do any of normal mainloop
        }

        generic_pwm_outs(); // placed here so it always runs (due to controlling fuel pump)

        sample_map_tps(&localflags);
        serial();

        /* Noise Filter Table Lookup */
        if (ram4.NoiseFilterOpts & 1) {
            unsigned int noiserpm;
            if (outpc.rpm < 10) {
                noiserpm = 32767;
            } else {
                noiserpm = outpc.rpm;
            }
            utmp1 = intrp_1ditable(noiserpm, 4,
                               (unsigned int *) ram_window.pg8.NoiseFilterRpm, 0,
                               (unsigned int *) ram_window.pg8.NoiseFilterLen, 8);

            DISABLE_INTERRUPTS;
            NoiseFilterMin = (unsigned long) utmp1;
            ENABLE_INTERRUPTS;
        }

        idle_ctl();
        alternator();
        flex_fuel_calcs();

        if ((localflags & LOCALFLAGS_RUNFUEL) || (outpc.engine & ENGINE_CRANK)) {
            calc_baro_mat_load();
            serial();

            // check for stall
            if (((outpc.engine & ENGINE_READY) == 0) || (outpc.rpm == 0)) {
                tacho_targ = 0;
                goto END_FUEL;
            }

            fuelpump_run();

            utmp1 = crank_calcs();

            if (utmp1 == 1) { // flood clear
                goto KNK;
            } else if (utmp1 == 2) { // normal cranking
                goto CHECK_STAGED;
            }

            warmup_calcs();
            serial();
            do_sdcard();
            if (!(ram5.AE_options & USE_NEW_AE)) {
                normal_accel();
            }
            serial();

            hpte();
            ego_get_targs_gl();
            ego_get_targs();
            ego_calc();
            serial();
            main_fuel_calcs(&lsum_fuel1, &lsum_fuel2); /* lsum_fuel1 and lsum_fuel2 set in here */
            if (ram4.FlexFuel & 0x01) {
                lsum_fuel1 = (lsum_fuel1 * outpc.fuelcor) / 100;
                lsum_fuel2 = (lsum_fuel2 * outpc.fuelcor) / 100;
            }
            if ((ram5.ltt_opt & 1) && ltt_fl_ad) { // on and has been initialised
                long_term_trim_in();
                long_term_trim_out(&lsum_fuel1, &lsum_fuel2);
            }
            dribble_burn();
            serial();

            if (glob_sequential & SEQ_TRIM) { // MS3X and/or V3
                calc_fuel_trims();
            }

            if (ram5.AE_options & USE_NEW_AE) {
                new_accel(&lsum_fuel1, &lsum_fuel2);
            }
            /**************************************************************************
             **
             ** Calculation of Fuel Pulse Width
             **
             **************************************************************************/
            if (!(ram5.AE_options & USE_NEW_AE)) {
                unsigned long lsumae;
                lsumae = (long) outpc.tpsaccel * 10000;       // .01 usec
                if (lsum_fuel1 > 0) {
                    lsum_fuel1 += lsumae;  // usec
                }
                if (lsum_fuel2 > 0) {
                    lsum_fuel2 += lsumae;  // usec
                }
            }

            if (fc_ae) { /* AE type event at end of fuel-cut */
                unsigned long fc_ae_tmp;
                fc_ae_tmp = fc_ae * (long)ReqFuel;

                lsum_fuel1 += fc_ae_tmp;
                /* bound to 0-65535 usec */
                if (lsum_fuel1 > 6553500) {
                    lsum_fuel1 = 6553500;
                }

                lsum_fuel2 += fc_ae_tmp;
                /* bound to 0-65535 usec */
                if (lsum_fuel2 > 6553500) {
                    lsum_fuel2 = 6553500;
                }
            }

            tmp_pw1 = lsum_fuel1;
            tmp_pw2 = lsum_fuel2;

        /* for fuel table blending could do the blend here if ve1+2 -> lsum_fuel1 and ve3+4 -> lsum_fuel2 in inj.c */

            do_overrun();

            if (outpc.status3 & STATUS3_CUT_FUEL) {
                tmp_pw1 = 0;
                tmp_pw2 = 0;
            }

            n2o_launch_additional_fuel();

CHECK_STAGED:
            global_base_pw1 = tmp_pw1; // store this away for anything that wants to figure out duty
            if (ram4.staged & 0x7) {
                calc_staged_pw(tmp_pw1);

                tmp_pw1 = pw_staged1;
                tmp_pw2 = pw_staged2;
            }
END_FUEL:;
        }
        /* fuel is finished off down below EVERY_TOOTH */

        serial();

        speed_sensors();

        water_inj();
        fan_ctl_idleup();

        // check for spark cut rev limiting
        flagbyte10 &= ~FLAGBYTE10_SPKCUTTMP; // status var set later
        flagbyte10 &= ~FLAGBYTE10_FUELCUTTMP;

        /* Shut it down... */
        if ((flagbyte22 & FLAGBYTE22_SHUTDOWNLOCKOUT) == 0) {
            if (datax1.shutdowncode == SHUTDOWN_CODE_ACTIVE) {
                flagbyte22 |= FLAGBYTE22_SHUTDOWNACTIVE;
            } else if (datax1.shutdowncode == SHUTDOWN_CODESPK_ACTIVE) {
                flagbyte22 |= FLAGBYTE22_SHUTDOWNSPKACTIVE;
            } else if (datax1.shutdowncode == (SHUTDOWN_CODE_ACTIVE | SHUTDOWN_CODESPK_ACTIVE)) {
                flagbyte22 |= FLAGBYTE22_SHUTDOWNACTIVE | FLAGBYTE22_SHUTDOWNSPKACTIVE;
            } else if (datax1.shutdowncode == SHUTDOWN_CODE_INACTIVE) {
                flagbyte22 &= ~(FLAGBYTE22_SHUTDOWNACTIVE | FLAGBYTE22_SHUTDOWNSPKACTIVE);
            } else {
                /* Received an invalid code, disable the feature */
                flagbyte22 |= FLAGBYTE22_SHUTDOWNLOCKOUT;
            }
        }

        // check for stall
        if (((outpc.engine & ENGINE_READY) == 0) || (outpc.rpm == 0)) {
            goto EVERY_TOOTH;
        }

        idle_ac_idleup();
        idle_target_lookup();

        /* BOOST CONTROL */
        if (ram4.boost_ctl_settings & BOOST_CTL_ON) {
            boost_ctl();
        }

        serial();
        do_sdcard();
        /**************************************************************************
         **
         ** Calculation of Knock retard, Distributor Advance & coil charge time correction
         **
         **************************************************************************/
      KNK:
        do_knock();
        calc_advance(&lsum_ign); /* lsum_ign is set in here */
        do_launch();
        if (outpc.status3 & STATUS3_3STEP) {
            lsum_ign = outpc.step3_timing;
        }
        if (outpc.status3 & STATUS3_LAUNCHON) {
            lsum_ign = outpc.launch_timing;
        }
        lsum_ign -= outpc.vsslaunch_retard;
        transbrake();

        dribble_burn();
        serial();
        nitrous();
        traction();
        linelock_staging();
        pitlim(); /* launch spark tweaked in here */

        if (fc_retard_time && (ram4.OvrRunC & (OVRRUNC_RETIGN | OVRRUNC_PROGIGN))) { // fuel cut timing - either into cut or out of cut
            long max_retard, act_retard;
            if (ram5.fc_timing < lsum_ign) { // check we aren't already more retarded
                max_retard = lsum_ign - ram5.fc_timing; // total amount of retard
                if (fc_phase == 5) {
                    act_retard = (max_retard * (long)fc_retard_time) / ram5.fc_trans_time_ret; // return
                } else {
                    act_retard = (max_retard * (long)fc_retard_time) / ram5.fc_transition_time; // cut
                }
                lsum_ign -= act_retard;
                outpc.fc_retard = act_retard;
            }
        } else {
            outpc.fc_retard = 0;
        }

        /* Is this enough for anti-lag ? */
        if (lsum_ign < -450) { // most retard allowed is to 45ATDC
            lsum_ign = -450;
        }

        lsum_ign -= outpc.tc_retard;
        lsum_ign -= outpc.n2o_retard;
        lsum_ign -= outpc.launch_retard;

        /* Apply sanity limits to remote adjustment */
        if (datax1.SpkAdj < ram5.spkadj_min) {
            outpc.ext_advance = ram5.spkadj_min;
        } else if (datax1.SpkAdj > ram5.spkadj_max) {
            outpc.ext_advance = ram5.spkadj_max;
        } else { /* within limits */
            outpc.ext_advance = datax1.SpkAdj;
        }
        lsum_ign += outpc.ext_advance;     // degx10 - adjustment from remote CAN device (dangerous)
    
        if (outpc.status7 & STATUS7_LIMP) {
            lsum_ign -= ram5.cel_retard;
            outpc.cel_retard = ram5.cel_retard;
        } else {
            outpc.cel_retard = 0;
        }

        /* now lsum_ign contains the final timing */

        if (ram4.spk_mode3 & SPK_MODE3_SPKTRIM) {
            calc_spk_trims();
        }
        /**************************************************************************/
        if (spkmode == 31) {
            lsum_ign = 0; // fuel only
        }

        if ((spkmode == 2) || (spkmode == 3) || (spkmode == 14)) { // dizzy or twin trigger
            if (ram4.adv_offset < 200) { // next cyl
                // allow at least 1 deg ahead of trigger
                if (lsum_ign < (ram4.adv_offset + 10)) {
                    lsum_ign = ram4.adv_offset + 10;
                }
            } else { // this cyl
                // allow at least 5 deg after trigger
                if (lsum_ign > (ram4.adv_offset - 50)) {
                    lsum_ign = ram4.adv_offset - 50;
                }
            }
        }

        if (lsum_ign < MIN_IGN_ANGLE) {
            lsum_ign = MIN_IGN_ANGLE;
        }

        outpc.adv_deg = (int) lsum_ign;     // Report advance before we fudge it

        /* check for difference between flash and ram trigger angles (trigger wizard?) for on-the-fly adjustment */
        if (spkmode != 4) {
            if (ram4.adv_offset != flash_adv_offset) {
                int local_offset;
                local_offset = ram4.adv_offset;
                if (spkmode > 3) {
                    /* only permit +/-20 here too */
                    if (local_offset < -200) {
                        local_offset = -200;
                    } else if (local_offset > 200) {
                        local_offset = 200;
                    }
                }
                lsum_ign += flash_adv_offset - local_offset;
            }
        }
        if (spkmode < 2) { // EDIS isn't adjusted in ign_wheel_init like other modes, do it here
            lsum_ign -= flash_adv_offset;
        }

        /* ONLY FOR TRIGGER WHEEL - check for difference between flash and ram tooth #1 angles for on-the-fly adjustment */
        if ((spkmode == 4) && ((int)ram4.Miss_ang != flash_Miss_ang)) {
            lsum_ign += flash_Miss_ang - ram4.Miss_ang;
        }

        /* ROTARY: No function here, very small so leave it here */
        if (((ram4.EngStroke & 0x03) == 0x03)) {
            if (outpc.engine & ENGINE_CRANK) {
                lsum_rot = ram4.crank_timing;
            } else if (ram4.timing_flags & TIMING_FIXED) {
                lsum_rot = ram4.fixed_timing;
            } else {
                lsum_rot =
                    intrp_2ditable(outpc.rpm, outpc.fuelload, 8, 8,
                                   &ram_window.pg8.RotarySplitRPM[0],
                                   &ram_window.pg8.RotarySplitMAP[0],
                                   (int *) &ram_window.pg8.
                                   RotarySplitTable[0][0], 8);
                if (!(ram4.RotarySplitMode & ROTARY_SPLIT_ALLOW_NEG_SPLIT)) {
                    if (lsum_rot < 0) {
                        lsum_rot = 0;
                    }
                }

                /* now since split is with reference to the leading timing, subtract split to leading timing */
                lsum_rot = lsum_ign - lsum_rot;
            }
        }

        /* --------------------------------------------------------------------------------------- */
        /* EVERY TOOTH WHEEL DECODER */
        /* --------------------------------------------------------------------------------------- */
EVERY_TOOTH:
        do_everytooth_calcs(&lsum_ign, &lsum_rot, &localflags);
        do_tach_mask();
        dribble_burn();

        // that's most of the spark calcs done
        serial();
        do_sdcard();

        do_revlim_overboost_maxafr();

        if ((localflags & LOCALFLAGS_RUNFUEL) || (outpc.engine & ENGINE_CRANK)) {
            do_final_fuelcalcs();
            /* Variable tacho */
            if ((ram4.tacho_opt2 & 0xc0) == 0xc0) {
                unsigned int f;
                f = ((unsigned long)outpc.rpm * (ram4.no_cyl & 0x1f)) / 6; /* 0.1Hz steps */
                if ((ram4.EngStroke & 1) == 0) {
                    f = f / 2;
                }
                f = ((unsigned long)f * ram4.tacho_scale) / 1000UL;
                tacho_targ = f;
            }
            localflags &= ~LOCALFLAGS_RUNFUEL;
        }

        do_sequential_fuel();
        dribble_burn();

        shifter();
        generic_pwm();
        generic_pid();
        antilag();
        vvt();
        tclu();

        if (flagbyte22 & FLAGBYTE22_SHUTDOWNSPKACTIVE) {
            outpc.status2 |= STATUS2_SPKCUT;
            spkcut_thresh = 255; /* full cut */
        } else if (!(flagbyte10 & FLAGBYTE10_SPKCUTTMP)) {
            outpc.status2 &= ~STATUS2_SPKCUT;
            spkcut_thresh = 0; // no cut
        } else {
            outpc.status2 |= STATUS2_SPKCUT;
            spkcut_thresh = spkcut_thresh_tmp;
        }
        if (spklimiter_type == 2) {
            random_no_ptr = (unsigned int)&random_no[1][0];
        } else if (spklimiter_type == 3) {
            random_no_ptr = (unsigned int)&random_no[2][0];
        } else {
            random_no_ptr = (unsigned int)&random_no[0][0];
        }
        spkcut_thresh_tmp = 0; // set to no cut for the next pass around mainloop
        spklimiter_type = 0;

        if (!(flagbyte10 & FLAGBYTE10_FUELCUTTMP)) {
            fuel_cutx = 0;
            fuel_cuty = 0;
        }

        handle_ovflo();


        /***************************************************************************
         **
         ** Check whether to burn flash
         ** (do this in SCI now)
         **************************************************************************/
        serial();
        ckstall();              // also calc PWM duties
        handle_spareports();

        /***************************************************************************
         **
         **  Check for CAN receiver timeout
         **
         **************************************************************************/
        if (flagbyte3 & flagbyte3_can_reset) {
            CanInit();          // start over again and clear flags
        }

        if ((!(flagbyte17 & FLAGBYTE17_CANSUSP)) && (ram5.can_enable & CAN_ENABLE_ON)) {
            if (!((flagbyte3 & (flagbyte3_sndcandat | flagbyte3_getcandat))
                || ((flagbyte4 & FLAGBYTE4_POLLOK) == 0) ))  {
                /* while doing a passthrough read/write, do not add any other requests to the queue */

                can_poll();
                if (ram5.can_bcast1 & 1) {
                    can_broadcast();
                }

                can_iobox();
                io_pwm_outs();
                can_dashbcast();
            }

#if 0
            if (outpc.seconds && (can_scanid < 15))  {
                DISABLE_INTERRUPTS;
                can_scan_prot();
                ENABLE_INTERRUPTS;
            }
#endif
        }
        /* Next is needed for passthrough also */
        can_rcv_process(); /* deal with any incoming broadcast messages */

        if (flagbyte21 & FLAGBYTE21_ONESEC) { // once per second
            flagbyte21 &= ~FLAGBYTE21_ONESEC;
            // check and decrement CAN error counters
            int x;
            for (x = 0 ; x < 15 ; x++) {
                if (can_err_cnt[x]) {
                    can_err_cnt[x]--;
                }
            }
        }

        do_egt();
        accelerometer();
        dribble_burn();
        shiftlight();

        // check for delayed ignition kill
        if (flagbyte6 & FLAGBYTE6_DELAYEDKILL) {
            unsigned long ult3;
            DISABLE_INTERRUPTS;
            ult3 = lmms;
            ENABLE_INTERRUPTS;
            if ((ult3 - sync_loss_stamp) > 39) {        // 5ms hardcoded max dwell before trying to fire coil
                FIRE_COIL;
            }
            if ((ult3 - sync_loss_stamp) > sync_loss_time) {
                ign_kill();
            }
        }

        /* if required, call read and compress file from sd */
        if (flagbyte7 & FLAGBYTE7_SENDFILE) {
            sd_timeout = 0;
            read_compress();    // inbuf, outbuf
            cp_flash_ram(); // re-copy data back again because compress used that space.
            flagbyte7 &= ~FLAGBYTE7_SENDFILE;
            outpc.sd_status |= 0x04; // ready again
        }

        calc_fuelflow();

        if (pin_tsw_rf && ((*port_tsw_rf & pin_tsw_rf) == pin_match_tsw_rf)) {  // Reqfuel switching
            if ((orig_ReqFuel != ram4.ReqFuel_alt) || (orig_divider != ram4.Divider) || (orig_alternate != ram4.Alternate)) {
                /* User changed ReqFuel, recalculate the one we actually use */
                calc_reqfuel(ram4.ReqFuel_alt);
            }
        } else {
            if ((orig_ReqFuel != ram4.ReqFuel) || (orig_divider != ram4.Divider) || (orig_alternate != ram4.Alternate)) {
                /* User changed ReqFuel, recalculate the one we actually use */
                calc_reqfuel(ram4.ReqFuel);
            }
        }

        if (flagbyte16 & FLAGBYTE16_CHKSENS) {
            flagbyte16 &= ~FLAGBYTE16_CHKSENS;
            if  (ram5.cel_opt & 0x01) {
                check_sensors();
            }
        }

        if (flagbyte20 & FLAGBYTE20_CCT) {
            flagbyte20 &= ~FLAGBYTE20_CCT;
    /* experi - record period times */
            if (ram5.u08_debug38 & 2) {
                int x;
                unsigned long sum = 0;
                for (x = 0; x < no_triggers; x++) {
                    sum += cyl_time[x];
                }
                sum /= no_triggers;

                for (x = 0; x < 8; x++) {
                    outpc.sensors[8 + x] =  (cyl_time[x] * 1000) / sum; // percentage.0 per of average
                }
            }
        }

        if (flagbyte21 & FLAGBYTE21_POPMAF) {
            populate_maf(); // this takes ~11ms
        }

        /* 'user defined' */

        user_defined(); // call the user defined function - put your code in there

        /* end user defined section */

SKIP_THE_LOT:;

        /* port status */
        outpc.porta = PORTA;
        outpc.portb = PORTB;
        outpc.porteh = (PORTE & 0x17) | (PTH & 0xc0);
        outpc.portk = PORTK;
        outpc.portmj = (PTM & 0x3c) | (PTJ & 0xc3);
        outpc.portp = PTP;
        outpc.portt = PTT;

/* putting debug code here allows for quicker reflash in dev mode */
/* end debug */
   } //  END Main while(1) Loop
}
