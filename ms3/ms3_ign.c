/* $Id: ms3_ign.c,v 1.246.4.6 2015/05/08 14:46:00 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * ign_reset()
    Origin: Al Grippo
    Major: James Murray / Kenneth Culver
    Majority: Al Grippo / James Murray / Kenneth Culver
 * ign_kill
    Origin: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * coil_dur_table
    Origin: Al Grippo
    Majority: Al Grippo
 * wheel_fill_event_array
    Origin: Kenneth Culver
    Moderate: James Murray
    Majority: James Murray / Kenneth Culver
 * syncfirst
    Origin: Kenneth Culver
    Minor: James Murray
    Majority: Kenneth Culver
 * do_knock
    Origin: Al Grippo
    Moderate: Make digi, add internal knock. James Murray.
    Majority: Al Grippo / James Murray
 * calc_advance
    Origin: Al Grippo
    Major: James Murray / Kenneth Culver
    Major: VVT - James Murray / Kenneth Culver
    Majority: Al Grippo / James Murray / Kenneth Culver
 * do_everytooth_calcs
    Origin: Kenneth Culver
    Major: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * do_tach_mask
    Origin: Al Grippo
    Major: Large re-write. James Murray
    Majority: Al Grippo / James Murray
 * calc_spk_trims
    Origin: James Murray
    Majority: James Murray
 * knock_spi
    Origin: James Murray
    Majority: James Murray
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#include "ms3.h"

void ign_reset(void)
{
    int a;

    SSEM0;
    TIE = tim_mask;                    // disable selected IC/OC ints. Re-enable as required in a mo
    CSEM0;
    TSCR1 = 0x88;               // timer enabled, precision timer
    // reinitialize

    if (outpc.rpm != 0) {
        flagbyte6 |= FLAGBYTE6_DELAYEDKILL;
        // this ASM allows us to preserve sei/cli status when called from an interrupt
        __asm__ __volatile__("tfr ccr, b\n"
                             "pshb\n"
                             "sei\n");
        sync_loss_stamp = lmms;
        __asm__ __volatile__("pulb\n"
                             "tfr b, ccr\n");
        sync_loss_time = 390;  // 50ms time
    } else {
        ign_kill();
    }

    tooth_diff_this = 0;
    tooth_diff_last = 0;
    tooth_diff_last_1 = 0;
    tooth_diff_last_2 = 0;
    cam_last_tooth = 0;
    cam_last_tooth_1 = 0;
    pulse_no = 0;               // This will inhibit overflow counters
    t_enable_IC = 0xFFFFFFFF;
    t_enable_IC2 = 0xFFFFFFFF;
    igncount = 0;
    altcount = 0;
    egocount = 0;
    egopstat[0] = egopstat[1] = 0;
    tpsaclk = 0;
    outpc.status3 &= ~STATUS3_CUT_FUEL;
    flagbyte17 &= ~(FLAGBYTE17_OVERRUNFC | FLAGBYTE17_REVLIMFC | FLAGBYTE17_STATCAM);

    spark_events = spark_events_a;
    dwell_events = dwell_events_a;
    inj_events = inj_events_a;

    SSEM0;
    TIMTIE = 0; // disable all TIM injector timers
    TIMTFLG1 = 0xff; // clear all interrupt flags
    TIE &= tim_mask; // disable all timing timers (but not MAF/MAP)
    TFLG1 = ~tim_mask; // clear flags for same
    CSEM0;

    // Turn Off injectors
    if (!(ram4.hardware & HARDWARE_MS3XFUEL)) {
        SSEM0;
        PORTT &= ~0x0A;
        CSEM0;
    } else {
       // using MS3X for fuel, turn them off
        SSEM0;
        PORTA &= ~0x01;
        if (num_inj > 1) {
            PORTA &= ~0x02;
        }
        if (num_inj > 2) {
            PORTA &= ~0x04;
        }
        if (num_inj > 3) {
            PORTA &= ~0x08;
        }
        if (num_inj > 4) {
            PORTA &= ~0x10;
        }
        if (num_inj > 5) {
            PORTA &= ~0x20;
        }
        if (num_inj > 6) {
            PORTA &= ~0x40;
        }
        if (num_inj > 7) {
            PORTA &= ~0x80;
        }
        if ((ram4.staged & 0x7) && (num_cyl > 4) && !(glob_sequential & SEQ_SEMI)) {
            PORTT &= ~0x0A;
        }
        CSEM0;
    }
    SSEM0;
    *pPTMpin3 &= ~0x08;       // turn off inj led
    CSEM0;
    outpc.squirt = 0;           // injectors off
    // enable timer & IC interrupt
    synch = SYNC_FIRST;
    if (outpc.status1 & STATUS1_SYNCOK) {       // was synced
        outpc.synccnt++;
        outpc.status1 &= ~(STATUS1_SYNCOK | STATUS1_SYNCLATCH | STATUS1_SYNCFULL);
    }
    dtpred_adder = 0;

    if (flagbyte5 & FLAGBYTE5_CAM) {
        flagbyte1 &= ~flagbyte1_trig2active;    // reset software flag
        flagbyte2 &= ~flagbyte2_twintrignow;    // reset software flag
    }
    SSEM0;
    TFLG1 = tim_mask_run;   // clear any pending ignition ints
    TIE |= tim_mask_run;    // enable all ignition input timer ints
    CSEM0;

    flagbyte24 |= FLAGBYTE24_KNOCK_INIT;
    for (a = 0; a <= 15; a++) {
        dwl[a] = 0;                 // 0 means not in use, so count starts at 1
    }

    trig2cnt = 0;
    tooth_no = 0;
    flagbyte3 &= ~flagbyte3_toothinit;
    flagbyte1 &= ~flagbyte1_trig2active;        // clear 2nd trig
    tooth_counter = 0;
    tooth_counter_main = 0;
    syncerr = 0;
    flagbyte15 &= ~FLAGBYTE15_FIRSTRPM;
    NoiseFilterMin = 0;
    dwell_long = 0;
    dwell_us = 0;
    dwell_us2 = 0;
    cas_tooth = 0;
    cas_tooth_div = 0;
    last_edge = 255;
    outpc.water_duty = 0;
    outpc.nitrous1_duty = 0;
    outpc.nitrous2_duty = 0;
    dwellq[0].sel = 0;
    dwellq[1].sel = 0;
    xgcam_last = TCNT;
    /* if not running then zero out knock data */
    outpc.knock = 0;
    {
        int x;
        for (x = 0; x < 16 ; x++) {
            outpc.knock_cyl[x] = 0;
        }
    }
    tcrank_done = 0xffff;
    flagbyte2 |= flagbyte2_crank_ok;
    flagbyte17 &= ~(FLAGBYTE17_BFC_CRANKSET | FLAGBYTE17_BFC_RUNSET); // start with these off
    flow_int = 0;
    nextcyl_cnt = 0;
    for (a = 0 ; a < no_triggers ; a++) {
        skipdwell[a] = 0; // don't skip this dwell
    }
    skipinj = 0;
    skipinj_revlim = 0;
    skipinj_test = 0;
    vvt_run = 0;
    vvt_calc = 0;
    vvt_filter[0] = 200;
    vvt_filter[1] = 200;
    vvt_filter[2] = 200;
    vvt_filter[3] = 200;
    return;
}

void ign_kill()
{
    flagbyte6 &= ~FLAGBYTE6_DELAYEDKILL;
    // Turn off all spark outputs - typically a short delay after losing sync

    dwellsel = 0;
    coilsel |= 0x01;
    if (num_spk > 1) {
        coilsel |= 0x02;
    }
    if (num_spk > 2) {
        coilsel |= 0x04;
    }
    if (num_spk > 3) {
        coilsel |= 0x08;
    }
    if (num_spk > 4) {
        coilsel |= 0x10;
    }
    if (num_spk > 5) {
        coilsel |= 0x20;
    }
    if (num_spk > 6) {
        coilsel |= 0x40;
    }
    if (num_spk > 7) {
        coilsel |= 0x80;
    }
    if (num_spk > 8) {
        coilsel |= 0x0100;
    }
    if (num_spk > 9) {
        coilsel |= 0x0200;
    }
    if (num_spk > 10) {
        coilsel |= 0x0400;
    }
    if (num_spk > 11) {
        coilsel |= 0x0800;
    }
    if (num_spk > 12) {
        coilsel |= 0x1000;
    }
    if (num_spk > 13) {
        coilsel |= 0x2000;
    }
    if (num_spk > 14) {
        coilsel |= 0x4000;
    }
    if (num_spk > 15) {
        coilsel |= 0x8000;
    }
    if ((ram4.dwellmode & 3) == 2) {
        //Time after spark is weird. Normal state is "dwell"
        tmpdwellsel = coilsel;
        coilsel = 0;
        DWELL_COIL; // uses tmpdwellsel
    } else {
        FIRE_COIL;
    }

    if (((ram4.EngStroke & 0x03) == 0x03)) {
        if (ram4.RotarySplitMode & 0x1) {
            /* FD mode */
            rotaryspksel = 0x4;
        }
        rotaryspksel |= 0x8;
        FIRE_COIL_ROTARY;
    }
    fuelpump_off();             // Turn off fuel Pump. // try again here
    outpc.engine = 0;
    outpc.rpm = 0;
    flagbyte2 |= flagbyte2_crank_ok;
    if (((spkmode == 2) && ((ram4.spk_conf2 & 0x07) == 0x01)) // basic trigger + bypass
        || ((spkmode == 4) && ((ram4.spk_conf2 & 0x07) == 0x02))) { // toothed wheel + C3I
        // Set HEI bypass to 0v - requires that SpkA and SpkB are same polarity
        if (flagbyte10 & FLAGBYTE10_SPKHILO) { /* Going high */
            if (ram4.hardware & 0x02) {
                SSEM0SEI;
                PORTB &= ~0x02;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                PTM &= ~0x10;
                CSEM0CLI;
            }
        } else { /* going low */
            if (ram4.hardware & 0x02) {
                SSEM0SEI;
                PORTB |= 0x02;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                PTM |= 0x10;
                CSEM0CLI;
            }
        }
    }
//    asecount = 0; // doing this here causes ASE to restart on a sync-loss
}

int coil_dur_table(int delta_volt)
{
    RPAGE = tables[10].rpg;     // HARD CODED!
    int ix;
    long interp, interp3;
    // returns correction for coil duration vs battery voltage deviation
    //  from 12 V from table lookup in .1 ms units
    // bound input arguments
    if (delta_volt > ram_window.pg10.deltV_table[NO_COILCHG_PTS - 1]) {
        return ((int) ram_window.pg10.deltDur_table[NO_COILCHG_PTS - 1]);
    }
    if (delta_volt < ram_window.pg10.deltV_table[0]) {
        return ((int) ram_window.pg10.deltDur_table[0]);
    }
    // delta_volt in V x 10
    for (ix = NO_COILCHG_PTS - 2; ix > -1; ix--) {
        if (delta_volt > ram_window.pg10.deltV_table[ix]) {
            break;
        }
    }
    if (ix < 0)
        ix = 0;

    interp =
        ram_window.pg10.deltV_table[ix + 1] -
        ram_window.pg10.deltV_table[ix];
    if (interp != 0) {
        interp3 = (delta_volt - ram_window.pg10.deltV_table[ix]);
        interp3 = (100 * interp3);
        interp = interp3 / interp;
        if (interp < 0)
            interp = 0;
    }
    return ((int) (ram_window.pg10.deltDur_table[ix] +
                   interp * (ram_window.pg10.deltDur_table[ix + 1] -
                             ram_window.pg10.deltDur_table[ix]) / 100));
}

void wheel_fill_event_array(ign_event * spark_events_fill,
                            ign_event * dwell_events_fill, int spk_ang_in,
                            int dwl_ang_in, ign_time last_tooth_time,
                            unsigned int last_tooth_ang,
                            unsigned char rotary)
{
    char start, stop, iterate, tth, wfe_err;
    int tth_ang, tmp_ang, i, j, spk_ang, dwl_ang;
    ign_time spk_time, dwl_time;
    long tmptime;
    int trimtmp;

    if (rotary) {
        start = num_cyl;
        stop = num_cyl << 1; 
    } else {
        start = 0;
        stop = no_triggers;
    }
    for (i = start; i < stop; i++) {
        if (rotary) {
            j = i - num_cyl;
        } else {
            j = i;
        }

        trimtmp = 0;
        if (ram4.spk_mode3 & SPK_MODE3_SPKTRIM) { /* Spark Trim */
            trimtmp = spk_trim[(unsigned int)i];
        }

        if ((ram4.knk_option & 0x03) && (ram5.knock_conf & KNOCK_CONF_PERCYL)) { /* Knock per-cylinder */
            unsigned int cyl;
            cyl = ram4.fire[i];
            if (cyl > ram4.no_cyl) {
                cyl = 0; /* bogus */
            }
            
            RPAGE = RPAGE_VARS2;
            if (cyl && v2.knock_live & twopow[cyl - 1]) {  /* Per cylinder */
                /* Undo global knk_rtd that was already applied in calc_advance and apply per-cylinder */
                trimtmp += outpc.knk_rtd - v2.knk_rtd[cyl - 1];
            }
            /* else, No per-cylinder data available, leave the knk_rtd applied */
        }

        if (trimtmp != 0) { /* either trim or knock */

            spk_ang = spk_ang_in + trimtmp;

            // As we've changed the angle, need to re-check railing rules
            // Whenever railing spark, need to also adjust trimtmp
            // so correct value can be applied to dwell.
            if (spk_ang < MIN_IGN_ANGLE) {
                trimtmp = trimtmp - spk_ang + MIN_IGN_ANGLE;
                spk_ang = MIN_IGN_ANGLE;
            }
            if ((spkmode == 2) || (spkmode == 3) || (spkmode == 14)) { // dizzy or twin trigger
                if (ram4.adv_offset < 200) { // next-cyl
                    if (spk_ang < (ram4.adv_offset + 10)) {
                        trimtmp = trimtmp + (ram4.adv_offset + 10) - spk_ang;
                        spk_ang = ram4.adv_offset + 10;
                    }
                } else { // this cyl
                    if (spk_ang > (ram4.adv_offset - 50)) {
                        trimtmp = trimtmp + (ram4.adv_offset - 50) - spk_ang;
                        spk_ang = ram4.adv_offset - 50;
                    }
                }
            }
            dwl_ang = dwl_ang_in + trimtmp;
        } else {
            spk_ang = spk_ang_in;
            dwl_ang = dwl_ang_in;
        }

        wfe_err = 0;
        iterate = 0;
        tth_ang = trig_angs[j]; // trigger angle for this trigger to allow for oddfire
        tth = trigger_teeth[j];

        while (!iterate) {
            if (tth_ang > spk_ang) {    // must both be signed to get comparison to work
                // found a suitable tooth
                iterate = 1;
            } else {
                //how far do we step back in deg
                tth--;
                if (tth < 1) {
                    tth = last_tooth;
                    wfe_err++;
                    if (wfe_err > 1) {
                        iterate = 2;
                    }
                }
                tth_ang += deg_per_tooth[tth - 1];      // add on time ahead of the tooth we stepped back to
            }
        }                       // end while

        if (iterate == 2) {
            DISABLE_INTERRUPTS;
            asm("nop\n");       // something screwed up, place for breakpoint
            ENABLE_INTERRUPTS;
            // can't continue as didn't find a valid tooth
            return;
        }

        tmp_ang = tth_ang - spk_ang;    // angle after trigger tooth we found
        //convert this small angle to time
        spk_time.time_32_bits =
            (tmp_ang * last_tooth_time.time_32_bits) / last_tooth_ang;

        wfe_err = 0;
        while ((wfe_err < 2) && (spk_time.time_32_bits < SPK_TIME_MIN)) {
            // too soon after tooth, need to step back
            tth--;
            if (tth < 1) {
                tth = last_tooth;
                wfe_err++;
            }
            tth_ang += deg_per_tooth[tth - 1];  // add on time ahead of the tooth we stepped back to
            // recalc
            tmp_ang = tth_ang - spk_ang;
            spk_time.time_32_bits =
                (tmp_ang * last_tooth_time.time_32_bits) / last_tooth_ang;
        }

        if (wfe_err > 1) {
            DISABLE_INTERRUPTS;
            asm("nop\n");       // something screwed up, place for breakpoint
            ENABLE_INTERRUPTS;
            // can't continue as didn't find a valid tooth
            return;
        }

        spark_events_fill[(unsigned int) i].tooth = tth;
        spark_events_fill[(unsigned int) i].time = spk_time;
        spark_events_fill[(unsigned int) i].coil = i;

        // Code limits spark advance so spark point should always be ahead of or after
        // the trigger and not crossing it.
        // During next-cyl cranking we set this flag.
        // to force a spark at trigger.
        tmptime =
            (spk_ang * last_tooth_time.time_32_bits) / last_tooth_ang;

        if ((ram4.dwellmode & 3) == 3) {
            //if "charge on trigger", then dwell on same tooth as scheduling spark
            //might be ok for MSD in basic dizzy mode. Might give unpredictable short pulses
            // when combined with wheel mode.
            dwell_events_fill[(unsigned int) i].tooth =
                spark_events_fill[(unsigned int) i].tooth;
            dwell_events_fill[(unsigned int) i].time32 = 0;
        } else {
            // do normal dwell - assume that it must always start ahead of spark
            // so start from spark tooth

            wfe_err = 0;
            iterate = 0;
            while (!iterate) {
                if (tth_ang > dwl_ang) {
                    // found a suitable tooth
                    iterate = 1;
                } else {
                    //how far do we step back in deg
                    tth--;
                    if (tth < 1) {
                        tth = last_tooth;
                        wfe_err++;
                        if (wfe_err > 1) {
                            iterate = 2;
                        }
                    }
                    tth_ang += deg_per_tooth[tth - 1];  // add on time ahead of the tooth we stepped back to
                }
            }                   // end while

            if (iterate == 2) {
                DISABLE_INTERRUPTS;
                asm("nop\n");   // something screwed up, place for breakpoint
                ENABLE_INTERRUPTS;
                // can't continue as didn't find a valid tooth
                return;
            }

            tmp_ang = tth_ang - dwl_ang;        // angle after trigger tooth we found
            //convert this small angle to time
            dwl_time.time_32_bits =
                (tmp_ang * last_tooth_time.time_32_bits) / last_tooth_ang;

            wfe_err = 0;
            while ((wfe_err < 2) && (dwl_time.time_32_bits < SPK_TIME_MIN)) {
                // too soon after tooth, need to step back
                tth--;
                if (tth < 1) {
                    tth = last_tooth;
                    wfe_err++;
                }
                tth_ang += deg_per_tooth[tth - 1];      // add on time ahead of the tooth we stepped back to
                // recalc
                tmp_ang = tth_ang - dwl_ang;
                dwl_time.time_32_bits =
                    (tmp_ang * last_tooth_time.time_32_bits) /
                    last_tooth_ang;
            }

            if (wfe_err > 1) {
                DISABLE_INTERRUPTS;
                asm("nop\n");   // something screwed up, place for breakpoint
                ENABLE_INTERRUPTS;
                // can't continue as didn't find a valid tooth
                return;
            }

            dwell_events_fill[(unsigned int) i].tooth = tth;
            dwell_events_fill[(unsigned int) i].time = dwl_time;
        }

        dwell_events_fill[(unsigned int) i].coil = i;

        // if dwell has stepped back then force dwell on tooth just before spark
        // or if dwell is on same tooth but fairly short then force it
        if ((dwell_events_fill[(unsigned int) i].tooth !=
                    spark_events_fill[(unsigned int) i].tooth)
                ||
                ((dwell_events_fill[(unsigned int) i].tooth ==
                  spark_events_fill[(unsigned int) i].tooth)
                 && (dwell_events_fill[(unsigned int) i].time32 < 150)
                 && (dwell_events_fill[(unsigned int) i].time32 >= 0))) {
            spark_events_fill[(unsigned int) i].ftooth =
                spark_events_fill[(unsigned int) i].tooth;
        } else {
            spark_events_fill[(unsigned int) i].ftooth = 0;
        }
    }
}

#define SYNCFIRST_LIMIT 1000
/* timed at 339us on 8cyl fully seq. so allow for 16cyl and safety factor
 * (if this takes long than one tooth then things get nasty and code fails to sync)
 */

void syncfirst(void)
{
    signed int tmp_tooth;
    unsigned char i, start_val;

    if (flagbyte6 & FLAGBYTE6_DELAYEDKILL) {
        ign_kill();
    }

    // convenient common place to put this code
    if (((ram4.spk_mode3 & 0xe0) == 0x60) // this is permanent wasted-COP mode
        && (!(((ram4.hardware & 0xc0) == 0x80) // but not the things needed for MAP phase detection
            && (ram4.no_cyl <= 2) && ((ram4.spk_config & 0x0c) == 0x0c)
            && ((ram4.spk_mode3 & 0xe0) == 0x60)
            && ((ram4.mapsample_opt & 0x04) == 0)))) { 
        synch |= SYNC_WCOP2;
    }

    /* check if we are somehow already at high rpms
       in that case skip these checks as not enough CPU time
       was previously preventing re-sync at higher rpms */
    if ((tooth_diff_this && (tooth_diff_this < SYNCFIRST_LIMIT)) || (no_triggers == 1)) {
        start_val = 255;
    } else {
        start_val = 0;
    }

    // if we were holding off for a bit but now synced, had better fire right now
    if (flagbyte6 & FLAGBYTE6_DELAYEDKILL) {
        FIRE_COIL;
        if (((ram4.EngStroke & 0x03) == 0x03)) {
            if (ram4.RotarySplitMode & 0x1) {
                /* FD mode */
                rotaryspksel = 0x4;
            }
            rotaryspksel |= 0x8;
            FIRE_COIL_ROTARY;
        }
        flagbyte6 &= ~FLAGBYTE6_DELAYEDKILL;
    }

    /* find next dwell event so we spark ASAP */
    fuel_cntr = start_val;
    tmp_tooth = tooth_no;
    while (!fuel_cntr) {
        tmp_tooth++;
        if (tmp_tooth > last_tooth) {
            tmp_tooth = 0;
        }
        if (tmp_tooth == tooth_no) {
            // been all the way around without finding anything - oops!
            fuel_cntr = 255;
        }
        for (i = 0; i < no_triggers; i++) {
            if (dwell_events[i].tooth == tmp_tooth) {
                fuel_cntr = i;
            } 
        }
    }
    if (fuel_cntr == 255) {
        fuel_cntr = 0;
    }
    next_spark.time = spark_events[fuel_cntr].time;
    next_spark.tooth = spark_events[fuel_cntr].tooth;
    next_spark.coil = spark_events[fuel_cntr].coil;
    next_spark.ftooth = spark_events[fuel_cntr].ftooth;
    next_spark.fs = spark_events[fuel_cntr].fs;
    next_dwell.time = dwell_events[fuel_cntr].time;
    next_dwell.tooth = dwell_events[fuel_cntr].tooth;
    next_dwell.coil = dwell_events[fuel_cntr].coil;

    /* find next map event so we capture map ASAP */
    fuel_cntr = start_val;
    tmp_tooth = tooth_no;
    while (!fuel_cntr) {
        tmp_tooth++;
        if (tmp_tooth > last_tooth) {
            tmp_tooth = 0;
        }
        if (tmp_tooth == tooth_no) {
            // been all the way around without finding anything - oops!
            fuel_cntr = 255;
        }
        for (i = 0; i < no_triggers; i++) {
            if (map_start_event[i].tooth == tmp_tooth) {
                fuel_cntr = i;
            } 
        }
    }
    if (fuel_cntr == 255) {
        fuel_cntr = 0;
    }
    next_map_start_event.time = map_start_event[fuel_cntr].time;
    next_map_start_event.tooth = map_start_event[fuel_cntr].tooth;
    next_map_start_event.evnum = map_start_event[fuel_cntr].evnum;

    // don't use ftooth or fs in dwell array
    if (glob_sequential & 0x3) {
        int upperlim;
        if (glob_sequential & SEQ_SEMI) {
            upperlim = 2;
        } else {
            upperlim = num_cyl;
        }
        for (i = 0; i < upperlim; i++) {
            next_inj[i].time = inj_events[i].time;
            next_inj[i].tooth = inj_events[i].tooth;
            next_inj[i].port = inj_events[i].port;
            next_inj[i].pin = inj_events[i].pin;
        }
    }

    /* find next fuel event */
    fuel_cntr = start_val;
    tmp_tooth = tooth_no;
    while (!fuel_cntr) {
        tmp_tooth++;
        if (tmp_tooth > last_tooth) {
            tmp_tooth = 0;
        }
        if (tmp_tooth == tooth_no) {
            // been all the way around without finding anything - oops!
            fuel_cntr = 255;
        }
        for (i = 0; i < no_triggers; i++) {
            if (trigger_teeth[i] == tmp_tooth) {
                fuel_cntr = i;
            } 
        }
    }
    if (fuel_cntr == 255) {
        fuel_cntr = 0;
    }
    next_fuel = trigger_teeth[fuel_cntr];

    if ((next_spark.tooth != 0) && (next_dwell.tooth != 0)) {
        synch &= ~SYNC_FIRST;   // must have non zero values before declaring sync not first
    }

    if (((ram4.EngStroke & 0x03) == 0x03)) {
        int start = num_cyl;
        next_spk_trl.time = spark_events[start].time;
        next_spk_trl.tooth = spark_events[start].tooth;
        next_spk_trl.coil = spark_events[start].coil;
        next_dwl_trl.time = dwell_events[start].time;
        next_dwl_trl.tooth = dwell_events[start].tooth;
        next_dwl_trl.coil = dwell_events[start].coil;
        if ((next_spk_trl.tooth != 0) && (next_dwl_trl.tooth != 0)) {
            synch &= ~SYNC_FIRST;       // must have non zero values before declaring sync not first
        }
    }

    /* knock window */
    next_knock_start_event.time = map_start_event[0].time;
    next_knock_start_event.tooth = map_start_event[0].tooth;
    next_knock_start_event.evnum = map_start_event[0].evnum;

    outpc.status1 |= STATUS1_SYNCOK;
}

void do_knock(void)
{
    unsigned char uctmp;
    int tmp1, x;
    unsigned int cyl;
    unsigned int thresh;

    thresh = 0;
    cyl = 1;

    if (flagbyte24 & FLAGBYTE24_KNOCK_INIT) {
        flagbyte24 &= ~FLAGBYTE24_KNOCK_INIT;
        RPAGE = RPAGE_VARS2;
        for (x = 0; x < 16; x++) {
            v2.knk_clk[x] = 0;
            v2.knk_clk_test[x] = ram4.knk_trtd;
            v2.knk_stat[x] = 0;
            v2.knk_count[x] = 0;
            v2.knk_rtd[x] = 0;
            v2.knock_live = 0;
        }
        outpc.knk_rtd = 0;
    }

    if (!(ram4.knk_option & 0x03)) {   // no knock correction
        outpc.knk_rtd = 0;
        outpc.knock = 0;
        return;
    }

    if ( (knock_state == 255) && ((ram4.knk_option & 0x0c) == 0x0c)
        && (!knock_start_countdown) && (!knock_window_countdown) ) {
        /* avoid sending SPI when about to enter or in the window */
        knock_state = 1; // send init commands.
    }

    /* removed non-windowed on/off mode */

    RPAGE = RPAGE_VARS2;
    if ((outpc.map > ram4.knk_maxmap) || (outpc.rpm < ram4.knk_lorpm) || (outpc.rpm > ram4.knk_hirpm)) {   // no knock correction
        outpc.knk_rtd = 0;
        for (x = 0; x < 16; x++) {
            v2.knk_rtd[x] = 0;
        }
    }

    /* check flag */
    if (((flagbyte11 & FLAGBYTE11_KNOCK) == 0) || knock_state) {
        /* nothing new or still in an SPI transaction*/
        return;
    }

    flagbyte11 &= ~FLAGBYTE11_KNOCK; /* clear flag */

    if (ram4.knk_option & 0x04) {
        unsigned int upscale;
        /* analogue input or internal, grab threshold */
        thresh =  intrp_1ditable(outpc.rpm, 10,
            (int *) ram_window.pg19.knock_rpms, 1,
            (int *) ram_window.pg19.knock_thresholds, 19);

        /* upscale threshold based on CLT */
        upscale =  intrp_1ditable(outpc.clt, 4,
            (int *) ram_window.pg19.knock_clts, 1,
            (int *) ram_window.pg19.knock_upscale, 19);

        if (upscale != 1000) {
            unsigned long lth;
            lth = ((thresh * (unsigned long)upscale) / 1000);
            if (lth > 1023) {
              thresh = 1023;
            } else {
              thresh = (unsigned int)lth;
            }
        }

        RPAGE = RPAGE_VARS2; /* have to set again because the curve lookups changed it */

        if (ram5.knock_conf & KNOCK_CONF_PERCYL) {
            /* record which cylinder it was on */
            cyl = ram4.fire[knock_chan_sample] - 1;

            if (cyl < 16) { // bound array
                unsigned int a, b;
                a = (unsigned int)outpc.knock_cyl[cyl];
                b = knockin >> 2; // store digi input 0-102% as 0-255 raw
                if (a > b) {
                    //b = 87.5% a + 12.5% b
                    a = (a * 7) / 8;
                    b = a + (b / 8);
                }
                /* peak detect and decay */
                outpc.knock_cyl[cyl] = (unsigned char)b;
            }
        }
    } else {
        thresh = 500;
    }

    /* user output in outpc.knock */
    {
        unsigned int a, b;
        a = outpc.knock;
        b = knockin; // store digi input 0-100
        if (a > b) {
            //b = 75% a + 25% b
            a = (a * 7) / 8;
            b = a + (b / 8);
        }
        /* peak detect and decay */
        outpc.knock = b;
    }

    /* Are we set up to control per-cylinder? Need to consider if engine config actually allows it */
    if ((ram5.knock_conf & KNOCK_CONF_PERCYLACT) == 0) {
        cyl = 0; /* use #1 only */
    } else {
        v2.knock_live |= twopow[cyl];
    }

    /* Knock control algorithm from Al Grippo. */
    /* Only reach here after a knock window, original implementation was every mainloop pass. */ 
    /* Uses un-smoothed per-cylinder data. */
    if ((knockin > thresh)
        && (!((ram5.knock_conf & KNOCK_CONF_LAUNCH) // ignore if in launch or flat-shift
            && ((outpc.status2 & STATUS2_FLATSHIFT) || (outpc.status3 & STATUS3_LAUNCHON)))) ) {
       outpc.status7 |= STATUS7_KNOCK;
        // signal(Vx100) indicates knock occurred
        if (v2.knk_count[cyl] < 255) {       // don't want to overflow to 0
            v2.knk_count[cyl]++;
        }
        if ((v2.knk_stat[cyl] == 0) && (v2.knk_count[cyl] > ram4.knk_ndet)) {
            v2.knk_clk[cyl] = v2.knk_clk_test[cyl]; // just caught knock - force 1st retard now
        }
    } else {
        outpc.status7 &= ~STATUS7_KNOCK;
    }

    if (v2.knk_clk[cyl] >= v2.knk_clk_test[cyl]) {  // time to check for retard/ adv spark
        if (v2.knk_count[cyl] > ram4.knk_ndet) {    // got valid knock. req ndet consec
            // knocks to be sure
            v2.knk_clk_test[cyl] = ram4.knk_trtd;
            if (v2.knk_stat[cyl] == 4) {      // thought had enough retard, but need more
                v2.knk_stat[cyl] = 2;
            }
            if (v2.knk_stat[cyl] < 2) {
                uctmp = ram4.knk_step1;     // haven't stopped knock yet
            } else {
                uctmp = ram4.knk_step2;     // stopped once - use smaller step
            }
            uctmp = v2.knk_rtd[cyl] + uctmp;
            if (uctmp < ram4.knk_maxrtd) {
                v2.knk_rtd[cyl] = uctmp;
            } else {
                v2.knk_rtd[cyl] = ram4.knk_maxrtd;
            }

            if (v2.knk_stat[cyl] == 0) {     // 1st knock
                v2.knk_stat[cyl] = 1;
            } else if (v2.knk_stat[cyl] == 2) {       // this is knock returning due to too much adv
                if ((ram4.knk_option & 0x03) == 1) {  // operate one step below knock
                    v2.knk_stat[cyl] = 3;
                }
            }
        } else {            // did not get unambiguous knock
            v2.knk_clk_test[cyl] = ram4.knk_tadv;   // change time interval
            if ((v2.knk_stat[cyl] == 1) || (v2.knk_stat[cyl] == 2)) {
                if ((v2.knk_stat[cyl] == 2) && (outpc.knk_rtd == 0)) {
                    v2.knk_stat[cyl] = 0;   // back to table value & no knock- restart process
                } else {
                    if (v2.knk_rtd[cyl] >= ram4.knk_step2) {
                        v2.knk_rtd[cyl] -= ram4.knk_step2;    // remove some retard
                    } else {
                        v2.knk_rtd[cyl] = 0;
                    }
                    v2.knk_stat[cyl] = 2;   // had knock, eliminated it, now try get closer to threshhold
                }
            } else if (v2.knk_stat[cyl] == 3) { // had knock, eliminated it, couldn't get closer to thresh
                // This is as good as can get; save tps, rpm
                v2.knk_tble_advance[cyl] = outpc.adv_deg;
                v2.knk_stat[cyl] = 4;
            } else if (v2.knk_stat[cyl] == 4) { // maintain status until knock or change in tps or map
                tmp1 = outpc.adv_deg - v2.knk_tble_advance[cyl];
                if (tmp1 < 0) {
                    tmp1 = -tmp1;
                }
                if (tmp1 > ram4.knk_dtble_adv) {
                    v2.knk_rtd[cyl] = 0;      // conditions changed- restart process
                    v2.knk_stat[cyl] = 0;
                }
            }
        }
        v2.knk_count[cyl] = 0;
        v2.knk_clk[cyl] = 0;
    }

    /* Report largest retard */
    tmp1 = 0;
    for (x = 0; x < 16; x++) {
        if (v2.knk_rtd[x] > tmp1) {
            tmp1 = v2.knk_rtd[x];
        }
    }
    outpc.knk_rtd = tmp1;

    if (ram5.knock_conf & KNOCK_CONF_DEBUG) {
        /* for debug */
        for (x = 0; x < 16; x++) {
            outpc.sensors[x] = v2.knk_rtd[x];
        }
    }
}

void calc_advance(long *lsum)
{
    unsigned char uctmp, w;
    int base_timing, adv1 = 0, adv2 = 0, adv3 = 0, adv4 = 0, rl_rtd = 0, idlecor = 0;


    // Table switching
    uctmp = 0;                  //uctmp is local tmp var
    // nitrous control will also use this var as pin behaviour will be different ??
    if ((ram5.dualfuel_sw & 0x05) == 0x05) { // on + spark
        if (pin_tss && ((*port_tss & pin_tss) == pin_match_tss)) {       // Hardware table switching
            uctmp = 1;
        }
    } else {
        if (ram4.feature5_0 & TSW_S_ON) {       // table switching
            if ((ram4.feature5_0 & TSW_S_INPUTS) == 0) {
                if (pin_tss && ((*port_tss & pin_tss) == pin_match_tss)) {       // Hardware table switching
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_S_INPUTS) == TSW_S_RPM) {
                if (outpc.rpm > ram4.tss_rpm) {
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_S_INPUTS) == TSW_S_MAP) {
                if (outpc.map > ram4.tss_kpa) {
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_S_INPUTS) == TSW_S_TPS) {
                if (outpc.tps > ram4.tss_tps) {
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_S_INPUTS) == TSW_S_ONOFFVVT) {
                if (flagbyte15 & FLAGBYTE15_VVTON) {
                    uctmp = 1;
                }
            }
        }
    }

    if (uctmp) {
        outpc.status1 |= STATUS1_STBLSW;
    } else {
        outpc.status1 &= ~STATUS1_STBLSW;
    }

    serial();
    // Calculate ignition advance
    if (outpc.engine & ENGINE_CRANK) {
        if ((((spkmode & 0xfe) == 2) || (spkmode == 14)) && (ram4.adv_offset <= 200)) {    //2,3
            *lsum = ram4.adv_offset;    // next-cyl fire at trigger on in dizzy modes
        } else {
            *lsum = ram4.crank_timing;
        }
        base_timing = *lsum;
    } else if (ram4.timing_flags & TIMING_FIXED) {
        base_timing = ram4.fixed_timing;
        *lsum = ram4.fixed_timing;
    } else if (flagbyte23 & FLAGBYTE23_ALS_ON) {
        base_timing = outpc.als_timing;
        *lsum = outpc.als_timing;
    } else {
        long ign1 = 0, ign3 = 0;

        if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw & 0x04)
            && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
            w = 7;
        } else if ((ram4.feature5_0 & 0x10) && ((ram4.feature5_0 & 0x60) == 0) && (ram4.tsw_pin_s & 0x1f) == 14) { /* blend from Spk1+2 to Spk3+4 */
            w = 3;
        } else if (uctmp) {  /* table switching from Spk1+2 to Spk3+4 */
            w = 2;
        } else { /* single (up-down) table */
            w = 1;
        }

        if (w & 1) {           
            ign1 = intrp_2ditable(outpc.rpm, outpc.ignload, NO_EXSRPMS,
                       NO_EXSMAPS, &ram_window.pg19.srpm_table1[0],
                       &ram_window.pg19.smap_table1[0], (int *) &ram_window.pg13.adv_table1[0][0], 13);
            adv1 = ign1;
        }
        if (w & 2) {           
            ign3 = intrp_2ditable(outpc.rpm, outpc.ignload, NO_EXSRPMS,
                       NO_EXSMAPS, &ram_window.pg19.srpm_table3[0],
                       &ram_window.pg19.smap_table3[0], (int *) &ram_window.pg18.adv_table3[0][0], 18);
            adv3 = ign3;
        }

        if (w == 2) {
            ign1 = ign3; /* store switched result to ign1 */
        }
       /* No change for Single table unswitched, or spk1+2 -> spk3+4 blend (2) */

        /* If the second table is enabled, use it i.e. spk2 or spk2+4*/
        if (ram4.IgnAlgorithm & 0xF0) {
            long ign2 = 0, ign4 = 0;

            if (w & 1) {
                ign2 = intrp_2ditable(outpc.rpm, outpc.ignload2, NO_EXSRPMS,
                       NO_EXSMAPS, &ram_window.pg19.srpm_table2[0],
                       &ram_window.pg19.smap_table2[0], (int *) &ram_window.pg13.adv_table2[0][0], 13);
                adv2 = ign2;
            }
            if (w & 2) {
                ign4 = intrp_2ditable(outpc.rpm, outpc.ignload2, NO_EXSRPMS,
                       NO_EXSMAPS, &ram_window.pg21.srpm_table4[0],
                       &ram_window.pg21.smap_table4[0], (int *) &ram_window.pg22.adv_table4[0][0], 21);
                adv4 = ign4;
            }

            if (w == 2) {
                ign2 = ign4; /* store switched result to ign2 */
            }

            if (ram4.loadopts & 0x10) {
                /* blend from spk1+3 -> spk2+4 (2) */
                unsigned char blend;
                blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[1] & 0x1f), 9, 
                        (int *) ram_window.pg25.blendx[1], 0, 
                        (unsigned char *)ram_window.pg25.blendy[1], 25);
                ign1 = ((ign1 * (100 - blend)) + (ign2 * blend)) / 100;
                if (w & 2) {
                    ign3 = ((ign3 * (100 - blend)) + (ign4 * blend)) / 100;
                }
            } else {
                /* Ign only additive */
                ign1 += ign2;
                if (w & 2) {
                    ign3 += ign4;
                }
            }
        }

        if (w == 3) {
            /* spk1+2 -> spk3+4 blend (4) into single Spk value*/
            unsigned char blend;

            blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[3] & 0x1f), 9, 
                    (int *) ram_window.pg25.blendx[3], 0, 
                    (unsigned char *)ram_window.pg25.blendy[3], 25);
            *lsum = ((ign1 * (100 - blend)) + (ign3 * blend)) / 100;
        } else if (w == 7) {
            /* spk1+2 -> spk3+4 blend (4) into single Spk value*/
            *lsum = ((ign1 * (100 - flexblend)) + (ign3 * flexblend)) / 100;
        } else {
            *lsum = ign1;
        }

        /* Bare spark advance table lookup (if looked up) */
        outpc.adv1 = adv1;
        outpc.adv2 = adv2;
        outpc.adv3 = adv3;
        outpc.adv4 = adv4;

        /* Current base timing. Either table lookup, cranking, fixed etc. */
        base_timing = *lsum;

        /* idle advance takes precedence over previous calcs */
        if ((ram4.idle_special_ops & IDLE_SPECIAL_OPS_IDLEADVANCE) || 
            (ram4.idle_special_ops & IDLE_SPECIAL_OPS_CLIDLE_TIMING_ASSIST)) {
            if ((!(ram4.idleveadv_to_pid & IDLEVEADV_TO_PID_IDLEADV) && 
                (outpc.tps < ram4.idleadvance_tps) &&
                (outpc.rpm < ram4.idleadvance_rpm) &&
                (outpc.fuelload > ram4.idleadvance_load) &&
                (outpc.clt > ram4.idleadvance_clt)) ||
                ((ram4.idleveadv_to_pid & IDLEVEADV_TO_PID_IDLEADV) &&
                 (outpc.status2 & 0x80)))    {
                if (((idle_advance_timer >= ram4.idleadvance_delay) &&
                    (flagbyte4 & flagbyte4_idleadvreset)) ||
                    ((ram4.idleveadv_to_pid & IDLEVEADV_TO_PID_IDLEADV) &&
                    (outpc.status2 & 0x80))) {
                    outpc.status6 |= STATUS6_IDLEADV;
                    /* deg x 10 */

                    if (ram4.idle_special_ops & IDLE_SPECIAL_OPS_IDLEADVANCE) {
                        *lsum =
                            intrp_1ditable(outpc.fuelload, 4,
                                    (int *) ram4.idleadvance_loads, 1,
                                    (int *) ram4.idleadvance_curve, 4);
                        base_timing = *lsum;
                    }

                    if (ram4.idle_special_ops & IDLE_SPECIAL_OPS_CLIDLE_TIMING_ASSIST) {
                        idlecor = intrp_1ditable(outpc.rpm - outpc.cl_idle_targ_rpm, 
                                                8, (int *) ram5.cl_idle_timing_target_deltas, 1,
                                                (int *) ram5.cl_idle_timing_advance_deltas,
                                                5);
                        *lsum += idlecor;
                    }
                } else {
                    outpc.status6 &= ~STATUS6_IDLEADV;
                    if (!(flagbyte4 & flagbyte4_idleadvreset)) {
                        DISABLE_INTERRUPTS;
                        idle_advance_timer = 0;
                        ENABLE_INTERRUPTS;
                        flagbyte4 |= flagbyte4_idleadvreset;
                    }
                }
            } else {
                flagbyte4 &= ~flagbyte4_idleadvreset;
                outpc.status6 &= ~STATUS6_IDLEADV;
            }
        }

        /* now add and remove stuff */
        *lsum += outpc.cold_adv_deg - outpc.knk_rtd;    // degx 10
        // Subtract retard vs manifold air temp - now includes the MAT/CLT calc
        outpc.mat_retard = intrp_1dctable(outpc.airtemp, NO_MAT_TEMPS, (int *) ram_window.pg8.MatTemps, 0, (unsigned char *) ram_window.pg8.MatSpkRtd, 8);    // %
        *lsum -= outpc.mat_retard;
        // correct for flex fuel;
        *lsum += outpc.flex_advance;      // degx10

        // rev limit
        if (ram4.RevLimOption & 1) {
            if (outpc.rpm > RevLimRpm2) {
                rl_rtd = ram4.RevLimMaxRtd;
                outpc.status3 |= STATUS3_REVLIMSFT;
            } else if (outpc.rpm > RevLimRpm1) {
                rl_rtd = (((long) ram4.RevLimMaxRtd * (outpc.rpm - RevLimRpm1)) / (RevLimRpm2 - RevLimRpm1));   // deg x 10
                outpc.status3 |= STATUS3_REVLIMSFT;
            } else {
                outpc.status3 &= ~STATUS3_REVLIMSFT;
            }

            *lsum -= rl_rtd;     // deg x 10

        } else if (ram4.RevLimOption & 2) {
            if ((outpc.rpm > RevLimRpm1) && (*lsum > ram4.RevLimRtdAng)) {      // made sure we are going to retard.
                rl_rtd = *lsum - ram4.RevLimRtdAng; // calculate in retard terms
                *lsum = ram4.RevLimRtdAng;
                outpc.status3 |= STATUS3_REVLIMSFT;
            } else {
                outpc.status3 &= ~STATUS3_REVLIMSFT;
            }
        }
    }
    outpc.base_advance = base_timing;
    outpc.idle_cor_advance = idlecor;
    outpc.revlim_retard = rl_rtd;
}

void do_everytooth_calcs(long *lsum, long *lsum1, char *localflags)
{
    int utmp1, utmp2;
    signed int inttmp, latency_ang, last_tooth_ang,
               last_tooth_ang_prev, spk_req_ang, dwl_req_ang, map_req_ang;
    unsigned long max_dwl_time;
    unsigned long dtpred_local, dtpred_last_local, dtpred_last_local2,
                  dtpred_last_local3;
    ign_event *dwell_events_fill, *spark_events_fill;
    ign_time last_tooth_time, spk_time, dwl_time;
    ign_time last_tooth_time_prev;
    unsigned char last_tooth_no, last_tooth_no_prev;
    long longtmp, longtmp2;

    serial();

    DISABLE_INTERRUPTS;
    last_tooth_time = tooth_diff_rpm;
    last_tooth_time_prev = tooth_diff_rpm_last;
    dtpred_local = dtpred;
    dtpred_last_local = dtpred_last;
    dtpred_last_local2 = dtpred_last2;
    dtpred_last_local3 = dtpred_last3;
    last_tooth_no = tooth_no_rpm;
    ENABLE_INTERRUPTS;

    /* sometimes when reconfiguring spark modes, might get here with last_tooth == 0,
     * which is meaningless
     */
    if (last_tooth == 0) {
        goto END_WHEEL_DECODER;
    }
    // figure out angle between this tooth and previous one
    // array holds angle _ahead_ of the tooth, so step back a tooth
    last_tooth_no--;
    if ((char) last_tooth_no <= 0) {
        last_tooth_no = last_tooth;
    }
    // make sure we don't land on a zero one
    while ((last_tooth_ang = deg_per_tooth[last_tooth_no - 1]) == 0) {
        last_tooth_no--;
        if ((char) last_tooth_no <= 0) {
            last_tooth_no = last_tooth;
        }
    }

    /* see if we have new data to look at */
    if (tooth_counter_main != tooth_counter) {
        unsigned long ign_ultmp;
        /* Now do this calc
         * ticks per degree = last_tooth_time / last_tooth_ang
         * @ 50    rpm   5000    ticks per deg
         * @ 15000 rpm     16.66 ticks per deg
         * Would need to say *100 and use a ulong to retain precision
         * Use ultmp as ticks/deg * 1000 (degs is in 0.1deg)
         */

        ign_ultmp =
            (last_tooth_time.time_32_bits * 1000) /
            (unsigned long) last_tooth_ang;

        if (outpc.rpm == 1) {
            unsigned int newrpm;
            /* calc estimated rpm from the ticks/deg */
            newrpm = 16666667L / ign_ultmp;

            /* check for possible invalid high rpm caused by noise pulse on very first pulse */
            if (newrpm > ram4.crank_rpm) {
                outpc.rpm = ram4.crank_rpm >> 1; // use half of set cranking rpm instead just this time
            } else {
                outpc.rpm = newrpm;
            }
        }

        /* figure out how accurately we predicted ticks per deg  -12.7% to +12.7%   
         * - means current period is shorter, + means current period is longer
         * Can't easily compute as degree error like in MS2 because we are handling uneven
         * wheels and the degree error will depend on which tooth we are predicing over
         * %age should be more meaningful
         */
        longtmp2 = (long) ign_ultmp - (long) ticks_per_deg;
        longtmp2 = (longtmp2 * 100) / (long) ign_ultmp;
        if (longtmp2 < -127) {
            longtmp2 = -127;
        } else if (longtmp2 > 127) {
            longtmp2 = 127;
        }
        outpc.timing_err = (char) longtmp2;

        if (ram4.timing_flags & USE_1STDERIV_PREDICTION) {
            unsigned long ign_ultmp2;
            /* figure out angle between previous tooth and previous-previous one
             * array holds angle _ahead_ of the tooth, so step back a tooth
             */
            last_tooth_no_prev = last_tooth_no - 1;
            if ((char) last_tooth_no_prev <= 0) {
                last_tooth_no_prev = last_tooth;
            }
            /* make sure we don't land on a zero one */
            while ((last_tooth_ang_prev =
                        deg_per_tooth[last_tooth_no_prev - 1]) == 0) {
                last_tooth_no_prev--;
                if (last_tooth_no_prev == 0) {
                    last_tooth_no_prev = last_tooth;
                }
            }

            ign_ultmp2 =
                (last_tooth_time_prev.time_32_bits * 1000) /
                (unsigned long) last_tooth_ang_prev;
            ign_ultmp = (ign_ultmp << 1) - ign_ultmp2;
        }
        ticks_per_deg = ign_ultmp;
        /* fudge it to say we've done in next implementation
         * of code will use ring buffer
         */
        tooth_counter_main = tooth_counter;
    }

    if (synch & SYNC_SYNCED) {
        RPAGE = 0xfb; // HARDCODING for page 24
        // safe to alter RPAGE in mainloop
        /* to enable VVT for a mode, needs to be added to ms3_init.c, isr_ign.s and ms3_ign.c */
        if (ram_window.pg24.vvt_opt1 & 0x03) {
            volatile unsigned long cm, ck;
            volatile int t, t2;
            int ang, period, x, n, dm;
            n = ram_window.pg24.vvt_opt1 & 0x03;
            if (n == 3) {
                n = 4;
            }

            for (x = 0; x < n ; x++) {
                /* Only calculate new angle if new data available. */
                if (vvt_calc & twopow[x]) {
                    // synchronous tooth time sampling required
                    DISABLE_INTERRUPTS;
                    vvt_calc &= ~(twopow[x]);
                    vvt_run |= twopow[x];
                    cm = cam_time_sample[x];
                    ck = crank_time_sample[x];
                    t = cam_tooth_sample[x];
                    t2 = cam_trig_sample[x];
                    ENABLE_INTERRUPTS;

                    if ((signed long)cm < 0) {
                        /* Race condition caused negative time, ignore */
                        outpc.istatus5++;
                        goto VVT_SKIPONE;
                    }

                    cm = (cm * deg_per_tooth[t - 1]) / ck;

                    ang = tooth_absang[t - 1];
                    ang -= cm;

                    if ((signed long)ang < 0) {
                        /* Negative angle, ignore */
                        outpc.istatus5++;
                        goto VVT_SKIPONE;
                    }

                    // spark mode specific angle railing
                        
                    if (vvt_decoder == 4) { /* Coyote */
                        period = 3600;
                        if (((outpc.status1 & STATUS1_SYNCFULL) == 0) && 0) { /* Future - only calc angle when full sync */
                            /* Show min or max until fully synced and know phase */
                            if ((ram_window.pg24.vvt_opt5 & twopow[x + 4]) // this cam is exhaust
                                && (ram_window.pg24.vvt_opt1 & 0x80)) { // exhaust table is a retard table
                                /* convert to relative retard */
                                ang = ram_window.pg24.vvt_max_ang[x];
                            } else {
                                ang = ram_window.pg24.vvt_min_ang[x];
                            }
                        } else {
                            /* Special handling for Coyote, bring uneven teeth into usable range */
                            if (x == 0) {/* right intake cam */
                                if (t < 36) {
                                    /* Two groups of 2 */
                                    if (t2 == 1) {
                                        ang -= 3170;
                                    } else if (t2 == 2) {
                                        ang -= 2570;
                                    } else if (t2 == 3) {
                                        ang -= 1370;
                                    } else if (t2 == 4) {
                                        ang -= 770;
                                    }
                                } else {
                                    /* Group of 3 */
                                    if (t2 == 1) {
                                        ang -= 6170;
                                    } else if (t2 == 2) {
                                        ang -= 5270;
                                    } else {
                                        ang -= 4370;
                                        /* a3 */
                                    }
                                }
                            } else if (x == 1) { /* right exhaust cam */
                                if ((t < 21) || (t > 57)) {
                                    /* Two groups of 2 (perhaps) */
                                    if (t2 == 1) {
                                        ang -= 600;
                                    } else if (t2 == 2) {
                                        ang -= 0;
                                    } else if (t2 == 3) {
                                        ang -= 6000;
                                    } else if (t2 == 4) {
                                        ang -= 5400;
                                    }
                                } else {
                                    /* Group of 3 */
                                    if (t2 == 1) {
                                        ang -= 3600;
                                    } else if (t2 == 2) {
                                        ang -= 2700;
                                    } else {
                                        /* a3 */
                                        ang -= 1800;
                                    }
                                }
                            } else if (x == 2) {/* left intake cam */
                                if (t > 36) {
                                    /* Two groups of 2 */
                                    if (t2 == 1) {
                                        ang -= 7040;
                                    } else if (t2 == 2) {
                                        ang -= 6440;
                                    } else if (t2 == 3) {
                                        ang -= 5240;
                                    } else if (t2 == 4) {
                                        ang -= 4640;
                                    }
                                } else {
                                    /* Group of 3 */
                                    if (t2 == 1) {
                                        ang -= 2840;
                                    } else if (t2 == 2) {
                                        ang -= 1940;
                                    } else {
                                        ang -= 1040;
                                    }
                                }
                            } else { /* left exhaust cam */
                                if ((t < 21) || (t > 57)) {
                                    /* Group of 3 */
                                    if (t2 == 1) {
                                        ang -= 0;
                                    } else if (t2 == 2) {
                                        ang -= 6300;
                                    } else {
                                        /* a3 */
                                        ang -= 5400;
                                    }
                                } else {
                                    /* Two groups of 2 */
                                    if (t2 == 1) {
                                        ang -= 4200;
                                    } else if (t2 == 2) {
                                        ang -= 3600;
                                    } else if (t2 == 3) {
                                        ang -= 2400;
                                    } else if (t2 == 4) {
                                        ang -= 1800;
                                    }
                                }
                            }
                        }
                    } else if (vvt_decoder == 5) { /* Ford Duratec 4cyl */
                        period = 600; /* on 60 deg grid */
                    } else if (spkmode == 4) {
                        unsigned int opt;
                        opt = ram_window.pg24.vvt_tth[x];
                        if (opt && (opt < 25)) {
                            period = 7200 / opt;
                        } else {
                            period = 7200;
                        }
                    } else if ((spkmode == 9) || (spkmode == 55)) {
                        period = 3600;
                    } else if ((spkmode == 25) || (spkmode == 43)) {
                        period = 1200;
                    } else if (((spkmode == 7) || (spkmode == 57)) && (num_cyl == 6)) {
                        period = 1200;
                    } else if ((spkmode == 7) || (spkmode == 28) || (spkmode == 49) || (spkmode == 57)) {
                        period = 1800; // 28 applies to QR25DE and VK56
                    } else if (spkmode == 46) {
                        if (ram4.no_cyl == 6) {
                            period = 600;
                        } else {
                            period = 900;
                        }
                    } else if ((spkmode == 48) && (num_cyl == 4)) { /* 2zz-ge */
                        period = 1800;
                    } else if ((spkmode == 19) || (spkmode == 20) || (spkmode == 48) || (spkmode == 56)) {
                        period = 2400;
                    } else if (spkmode == 50) {
                        period = 600;
                    } else {
                        period = 7200;
                    }

                    ang -= (ram_window.pg24.vvt_min_ang[x] - 50); /* Bring into range to hopefully avoid a wraparound */
                    /* The additional 5.0 deg it to allow the -5 deg range check to work */

                    dm = 0;
                    while ((ang < 0) && (dm < 25)) {
                        ang += period;
                        dm++;
                    }

                    dm = 0;
                    while ((ang > period) && (dm < 25)) {
                        ang -= period;
                        dm++;
                    }

                    ang += (ram_window.pg24.vvt_min_ang[x] - 50); /* Return to absolute range */

                    /* apply +/- 5.0 degrees to tolerance and allow check to be disabled*/
                    if (((ang > (ram_window.pg24.vvt_max_ang[x] + 50)) || (ang < (ram_window.pg24.vvt_min_ang[x] - 50)))
                        && (ram_window.pg24.vvt_max_ang[x] != 0)
                        && (!(ram_window.pg24.vvt_opt5 & 0x08)) ) { /* final option is enable/disable */
                        outpc.status7 |= twopow[x]; // STATUS_VVT1ERR, STATUS_VVT2ERR, STATUS_VVT3ERR ,STATUS_VVT4ERR
                    } else {
                        int new_ang;
                        outpc.status7 &= ~twopow[x];
                        if (ram_window.pg24.vvt_max_ang[x] == 0) {
                            /* If max isn't set then report absolute angle */
                            new_ang = ang;
                        } else if ((ram_window.pg24.vvt_opt5 & twopow[x + 4]) // this cam is exhaust
                            && (ram_window.pg24.vvt_opt1 & 0x80)) { // exhaust table is a retard table
                            /* convert to relative retard */
                            new_ang = ram_window.pg24.vvt_max_ang[x] - ang;
                        } else {
                            new_ang = ang - ram_window.pg24.vvt_min_ang[x]; /* convert to relative advance */
                        }

                        if (ram_window.pg24.vvt_opt1 & VVT_OPT1_FILTER) {
                            int diff;
                            int filt;

                            filt = (int)vvt_filter[x];
                            diff = new_ang - outpc.vvt_ang[x];

                            if (diff < 0) {
                                diff = -diff;
                            }
                            if (diff < filt) { /* good - inside filter window */
                                filt -= 10;
                            } else { /* bad... outside window */
                                if (filt < 200) {
                                    /* generally, use previous value
                                        but at max window, do use new value in case of step changes in cam position */
                                    new_ang = outpc.vvt_ang[x];
                                }
                                filt += 20; /* open up the window for next time */
                            }
                            if (filt < 50) {
                                filt = 50; /* minimum 5 degrees */
                            } else if (filt > 200) {
                                filt = 200; /* max 20 degrees */
                            }
                            vvt_filter[x] = (unsigned char)filt; /* put it back */
                        }

                        /* Smooth it */
                        outpc.vvt_ang[x] = (new_ang + outpc.vvt_ang[x]) >> 1;
                    }
                }
                VVT_SKIPONE:; /* used to bypass VVT calcs on impossible time/angle */
            }
        }

        if (synch & SYNC_RPMCALC) {
            unsigned int rpm;

            /* oddfire dizzy + twin trigger + fuel only */
            if ((ram4.ICIgnOption & 0x8) && ((spkmode == 2) || (spkmode == 14) || (spkmode == 31))) { 
                unsigned long tdt, tdl;
                DISABLE_INTERRUPTS;
                tdt = tooth_diff_this;
                tdl = tooth_diff_last;
                ENABLE_INTERRUPTS;
                rpm = Rpm_Coeff / ((tdt + tdl) >> 1);
            } else {
                /* Avoid divide by 0 */
                if (dtpred_local < 2) {
                    rpm = 2;
                } else {
                    if (ram4.ICIgnOption & 0x8) {
                        /* always avg oddfire over 4 periods */
                        rpm = Rpm_Coeff /
                                ((dtpred_local + dtpred_last_local + dtpred_last_local2 +
                                  dtpred_last_local3) >> 2);
                    } else if ((ram4.no_cyl & 0x1f) == 1) { /* one cylinder averages over two revs */
                        rpm = Rpm_Coeff / ((dtpred_local + dtpred_last_local) >> 1);
                    } else {
                        rpm = Rpm_Coeff / dtpred_local;
                    }
                }
            }
            if ((flagbyte15 & FLAGBYTE15_FIRSTRPM) == 0) {
                flagbyte15 |= FLAGBYTE15_FIRSTRPM;
                if (outpc.rpm == 0) {
                    outpc.rpm = 1;
                }
            } else if (outpc.rpm < 3) {
                /* check for likely invalid high rpm caused by noise pulse on very first pulse */
                if (rpm > ram4.crank_rpm) {
                    outpc.rpm = ram4.crank_rpm >> 1; // use half of set cranking rpm instead just this time
                } else {
                    outpc.rpm = rpm;
                }
            } else if (outpc.engine & ENGINE_CRANK) {
                /* skip calc if cranking */
                outpc.rpm = rpm;
            } else {
                outpc.rpm +=
                    (int) ((ram4.rpmLF * ((long) rpm - outpc.rpm)) /
                            100);
            }

            RPAGE = RPAGE_VARS1;

            /* rpmdot calc - sliding window squares - variable dataset and max sample rate */
            v1.rpmdot_data[0][0] = (unsigned int)lmms; /* store actual 16 bit time to first pt in array */
            v1.rpmdot_data[0][1] = outpc.rpm/10; /* store rpm */

            /* minimum 10ms between samples to prevent high rpm jitter and cycletime cost */
            if ((v1.rpmdot_data[0][0] - v1.rpmdot_data[1][0]) > 78) {
                int rpmi;
                long rpmdot_sumx, rpmdot_sumx2, rpmdot_sumy, rpmdot_sumxy;
                long toprow, btmrow;
                unsigned long rpmdot_x; // was uint

                rpmdot_sumx = 0;
                rpmdot_sumx2 = 0;
                rpmdot_sumy = 0;
                rpmdot_sumxy = 0;

                rpmi = 0;
                /* only use ten datapoints for this. Array defined as larger */
                while ((rpmi < 10) && (rpmdot_sumxy < 10000000)) {
                    /* relative time to keep numbers smaller */
                    rpmdot_x = v1.rpmdot_data[0][0] - v1.rpmdot_data[rpmi][0];
                    rpmdot_sumx += rpmdot_x;
                    rpmdot_sumy += v1.rpmdot_data[rpmi][1];
                    rpmdot_sumx2 += (rpmdot_x * rpmdot_x); /* overflow if rpmdot_x > 65535 */
                    rpmdot_sumxy += (rpmdot_x * v1.rpmdot_data[rpmi][1]);
                    rpmi++;
                }

                /* divide sumy to help overflow */
                toprow = rpmdot_sumxy - (rpmdot_sumx * (rpmdot_sumy / rpmi)); 
                btmrow = rpmdot_sumx2 - (rpmdot_sumx * rpmdot_sumx / rpmi);

                btmrow = btmrow / 10; /* allows top row to be 10x less to reduce overflow. */
                toprow = (-toprow * 781) / btmrow;
                if (toprow > 32767) {
                    toprow = 32767;
                } else if (toprow < -32767) {
                    toprow = -32767;
                }

                /* very low rpmdot */
                if ((toprow > -150) && (toprow < 150)) {
                    long tmp_top, tmp_btm;
                    /* see how it compares to 20 positions */
                    tmp_btm = v1.rpmdot_data[0][0] - v1.rpmdot_data[RPMDOT_N-1][0];
                    tmp_top = (int)v1.rpmdot_data[0][1] - (int)v1.rpmdot_data[RPMDOT_N-1][1];
                    tmp_btm = tmp_btm / 10; /* allows top row to be 10x less to reduce overflow. */
                    tmp_top = (tmp_top * 781) / tmp_btm;
                    if (tmp_top > 32767) {
                        tmp_top = 32767;
                    } else if (tmp_top < -32767) {
                        tmp_top = -32767;
                    }
                    /* use lesser magnitude of two */
                    if (long_abs(tmp_top) < long_abs(toprow)) {
                        toprow = tmp_top;
                    }
                }

                toprow = 50L * (toprow - outpc.rpmdot); /* now 50% lag */
                if ((toprow > 0) && (toprow < 100)) {
                    toprow = 100;
                } else if ((toprow < 0) && (toprow > -100)) {
                    toprow = -100;
                }
                outpc.rpmdot += (int)(toprow / 100);

                /* shuffle data forwards by one */
                for (rpmi = RPMDOT_N - 1; rpmi > 0 ; rpmi--) {
                    v1.rpmdot_data[rpmi][0] = v1.rpmdot_data[rpmi - 1][0];
                    v1.rpmdot_data[rpmi][1] = v1.rpmdot_data[rpmi - 1][1];
                }
            }

            synch &= ~SYNC_RPMCALC;

            if (flagbyte0 & (FLAGBYTE0_MAPLOG | FLAGBYTE0_MAFLOG)) {
                /* Calculate how infrequently to sample at low revs. This aims
                   to give a full 720 deg of data when sampled every 0.128ms
                   and allowing for tooth data. */
                maplog_max = 1 + (unsigned char)(2344 / outpc.rpm);
            } else if ((flagbyte0 & FLAGBYTE0_ENGLOG) && (page >= 0xf6)) { // 4 byte
                // numbers chosen as a balance between precision and data points
                maplog_max = 1 + (unsigned char)(2900 / outpc.rpm);
            } else if (flagbyte0 & FLAGBYTE0_ENGLOG) { // 3 byte
                // numbers chosen as a balance between precision and data points
                maplog_max = 1 + (unsigned char)(2700 / outpc.rpm);
            }
        }
    } else {
        if (!(synch & SYNC_SEMI2)) {
            outpc.rpm = 0;
        }
    }

    /* if we're synced and/or we're supposed to calc */
    if ((last_tooth_time.time_32_bits != 0) && (dtpred_local)) {
        if (dwell_events == dwell_events_a) {
            spark_events_fill = spark_events_b;
            dwell_events_fill = dwell_events_b;
        } else {
            spark_events_fill = spark_events_a;
            dwell_events_fill = dwell_events_a;
        }

        spk_time.time_32_bits = 0;
        dwl_time.time_32_bits = 0;

        /* figure out based on period time and num_spk how much time we actually
         * have for dwell.
         */

        if ((((ram4.EngStroke & 0x03) == 0x03) &&
             (num_cyl == 2) && (ram4.spk_mode3 & 0x40))
             || (flagbyte11 & (FLAGBYTE11_DLI4 | FLAGBYTE11_DLI4))) {
            /* rotary 2-rotor single leading out mode or DLI, force 1 */
            max_dwl_time = dtpred_local;
        } else if ((ram4.ICIgnOption & 0x8)) {
            if (num_spk == 1) {
                /* in odd fire set max dwell for shorter period */
                if (dtpred_last_local < dtpred_local) {
                    max_dwl_time = dtpred_last_local;
                } else {
                    max_dwl_time = dtpred_local;
                }
            } else {
                /*  use average time.. might work or not */
                max_dwl_time = ((dtpred_local + dtpred_last_local) >> 1) * num_spk;
            }
        } else {
            max_dwl_time = dtpred_local * num_spk;
        }

        /* allow for known latency in bit-bash outputs */
        latency_ang =
            (unsigned int) (((OUTPUT_LATENCY + ram4.hw_latency) * 1000L) /
                         ticks_per_deg);
        inttmp = latency_ang + *lsum;

        if (spkmode < 2) { // EDIS
            unsigned long ltmp3;
            ltmp3 = 1536 - ((655 * *lsum) >> 8); /* 1536 - (adv*25.6) us //takes 9, still crappy asm */
            dwell_us = (unsigned int) ltmp3;
            utmp1 = dwell_us / 100;
            /* FIXME! Need to handle multi-spark */
        } else {
            if ((ram4.dwellmode & 3) == 0) {
                //normal dwell
                if ((outpc.engine & ENGINE_CRANK) || (outpc.rpm < 5)) {
                    utmp2 = ram4.crank_dwell;
                } else {
                    if (ram4.spk_conf2 & SPK_CONF2_DWELLTABLE) {
                        utmp2 = intrp_2dctable(outpc.rpm, outpc.ignload, 8, 8,
                            ram_window.pg28.dwell_table_rpms, ram_window.pg28.dwell_table_loads,
                            &ram_window.pg28.dwell_table_values[0][0], 0, 28);
                    } else {
                        utmp2 = ram4.max_coil_dur;
                    }
                }
                utmp1 = utmp2 * coil_dur_table(outpc.batt);
                __asm__ __volatile__("ldx #50\n" "idiv\n":"=x"(utmp1)
                        :"d"(utmp1) /* already in D from previous calc */
                        );

                /* prevent under or overflow */
                if (utmp2 == 0) {
                    utmp1 = utmp2;
                } else if (utmp2 > 255) {
                    utmp1 = 255;
                }

                /* Rotary trailing dwell */
                if (((ram4.EngStroke & 0x03) == 0x03)) {
                    utmp2 =
                        ram4.dwelltime_trl * coil_dur_table(outpc.batt);
                        __asm__ __volatile__("ldx #50\n" "idiv\n":"=x"(utmp2)
                            :"d"(utmp2)    // already in D from previous calc
                            );

                    /* prevent under or overflow */
                    if (utmp2 == 0) {
                        utmp2 = ram4.dwelltime_trl;
                    } else if (utmp2 > 255) {
                        utmp2 = 255;
                    }
                    outpc.dwell_trl = utmp2;
                }

            } else if ((ram4.dwellmode & 3) == 1) {
                /* dwell duty */
                if (dtpred_local > 1500000) {
                    utmp1 = 10000; /* top limit, prevents overflow */
                } else {
                    /* had to work to avoid overflows in here */
                    longtmp = (num_spk * dtpred_local) / 100;
                    longtmp = longtmp * ram4.dwellduty; /* /256 and convert ticks to 0.1ms */
                    utmp1 = longtmp / 256;
                }
            } else if ((ram4.dwellmode & 3) == 2) {
                /* time after spark (Saab trionic) */
                if (ram4.spk_conf2 & SPK_CONF2_DWELLCURVE) {
                    utmp1 =
                        intrp_1dctable(outpc.rpm, 6,
                                (int *) ram_window.pg9.dwellrpm_rpm,
                                0,
                                (unsigned char *) ram_window.
                                pg9.dwellrpm_dwell, 9);
                } else {
                    utmp1 = ram4.dwelltime;
                }
                if (utmp1 < 255) { /* 25.5ms max 'dwell' */
                    dwell_us = utmp1 * 100;
                } else {
                    dwell_us = 25500;
                }
                if (dwell_us > dtpred_local) {
                    dwell_us = dtpred_local - 200; /* at least (arbitrary) 0.2ms gap */
                }
            } else {
                utmp1 = 0;      /* unknown if dwell at trigger */
            }
        }

        longtmp = utmp1 * 100UL;

        if (((ram4.dwellmode & 3) == 0) && (spkmode >= 2)) {
            /* should not be possible to exceed time period in other modes as it has already been taken into account */
            unsigned long disch = ram4.max_spk_dur * 100UL;
            if ((longtmp + disch) > max_dwl_time) {
                /* if (charge + discharge) exceeds available time then scale back in proportion */
                longtmp = (longtmp * max_dwl_time) / (longtmp + disch);
            }
        }

        if ((ram4.dwellmode & 3) != 2) {
            DISABLE_INTERRUPTS;
            dwell_long = longtmp;
            ENABLE_INTERRUPTS;
        }
        outpc.coil_dur = longtmp / 100; /* shows dwell reduction due to spark duration */

        /* END OF DWELL STUFF. NOW DO TIME CALCS */

        if (((spkmode & 0xfe) == 2) || (spkmode == 14)) { /* dizzy mode or twin trig */
            /* check for out of range advance */
            if (ram4.adv_offset > 200) { /* this cyl */
                if (inttmp > (ram4.adv_offset - 10)) {
                    inttmp = ram4.adv_offset - 10; /* set to 1 deg less than trigger angle */
                }
            } else { /* next cyl */
                if (inttmp < (ram4.adv_offset + 10)) {
                    inttmp = ram4.adv_offset + 10; /* set to 1 deg more than trigger angle */
                }
            }
        }

        spk_req_ang = inttmp; /* deg*10 */

        /* only need spark time to get to dwell time */
        spk_time.time_32_bits = ((inttmp *  (long)ticks_per_deg) / 1000L);

        if ((ram4.dwellmode & 3) == 0) { /* normal dwell */
            /* have now calc target dwell. */
                    unsigned int md;
                /* use largest of primary or trailing dwell */
                if (outpc.coil_dur > outpc.dwell_trl) {
                    md = (unsigned int) (longtmp / 64); /* 200% convert timer ticks to 0.128ms units (max of 32ms)*/
                } else {
                    md = (unsigned int) ((outpc.dwell_trl * 100UL) / 64);
                }
                if (md > 255) {
                    md = 255;
                }
                maxdwl = md;
            dwl_time.time_32_bits = spk_time.time_32_bits + longtmp;
        } else if ((ram4.dwellmode & 3) == 2) {
            maxdwl = 0; // no overdwell protection desired with time-after-spark
            dwl_time.time_32_bits = spk_time.time_32_bits - longtmp;
        } else { // fixed duty or charge-at-trigger
            maxdwl = 0; // no overdwell protection
            dwl_time.time_32_bits = spk_time.time_32_bits + longtmp;
        }

        /* now convert dwell time to an angle using last tooth time */
        dwl_time.time_32_bits = dwl_time.time_32_bits * 1000;
        dwl_req_ang = (int) (dwl_time.time_32_bits / (long)ticks_per_deg); // without (long) cast this was using an unsigned divide in error

        while (dwl_req_ang < MIN_IGN_ANGLE) {
            dwl_req_ang += cycle_deg;
        }
        while (dwl_req_ang > cycle_deg) {
            dwl_req_ang -= cycle_deg;
        }

        wheel_fill_event_array(spark_events_fill, dwell_events_fill,
                spk_req_ang, dwl_req_ang,
                last_tooth_time, last_tooth_ang, 0);

        if ((spkmode == 2) || (spkmode == 3)) { /* basic trigger i.e. distributor */
            unsigned int delay_tmp;
            unsigned int ix;
            for (ix = 0 ; ix < no_triggers ; ix++) {
                if (ram4.adv_offset < 200) { /* next cyl */
                    if (ram4.ICIgnOption & 0x08) { /* oddfire next-cyl */
                        unsigned int iy;
                        iy = ix;
                        iy = ix + 1;
                        if (iy >= no_triggers) {
                            iy = 0;
                        }
                        if (spk_req_ang < ram4.adv_offset) {
                            delay_tmp = 0; /* can't go later than trigger */
                        } else {
                            delay_tmp = deg_per_tooth[iy] + ram4.adv_offset - spk_req_ang;
                            /* for oddfire the calc is divided by two tooth periods so that when
                             * use do the calc we can
                             * still use the last period but the 65536 scaling calc does not overflow
                             * e.g. if last was 90 deg, but we want to delay 140 deg forwards -> overflow
                             * now  (140/(90+150) * 65536) = 38229 
                             */
                            dizzy_scaler[ix] = (unsigned int)(((unsigned long)delay_tmp << 16) /
                                               (deg_per_tooth[0] + deg_per_tooth[1]));
                        }
                    } else { /* even fire next-cyl */
                        if (spk_req_ang < ram4.adv_offset) {
                            delay_tmp = 0; /* can't go later than trigger */
                        } else {
                            delay_tmp = deg_per_tooth[ix] + ram4.adv_offset - spk_req_ang;
                            dizzy_scaler[ix] = (unsigned int)(((unsigned long)delay_tmp << 16) /
                                                deg_per_tooth[ix]);
                        }
                    }
                } else { /* this cyl */
                    if (spk_req_ang > (ram4.adv_offset - 20)) {
                        delay_tmp = 20; /* minimum 2deg delay */
                    } else {
                        delay_tmp = ram4.adv_offset - spk_req_ang;
                    }
                    if (ram4.ICIgnOption & 0x08) { /* oddfire this-cyl */
                        dizzy_scaler[ix] = (unsigned int)(((unsigned long)delay_tmp << 16) /
                                                         (deg_per_tooth[0] + deg_per_tooth[1]));
                    } else { /* evenfire this-cyl */
                        dizzy_scaler[ix] = (unsigned int)(((unsigned long)delay_tmp << 16) /
                                                            deg_per_tooth[ix]);
                    }
                    if (spkmode == 3) {
                        /* for trigger return set spark timer to fire very late,
                         * should only happen if trigger return missed
                         * and possibly for one spark during crank/run transition
                         */
                        trigret_scaler = (unsigned int)(((unsigned long)ram4.adv_offset << 16) /
                                          deg_per_tooth[ix]);
                    }
                }
            }

        } else if (spkmode == 14) { /* twin trigger */
            unsigned int delay_tmp;
            unsigned int ix;
            for (ix = 0 ; ix < no_triggers ; ix++) {
                if (ram4.adv_offset < 200) { /* next cyl */
                    if (spk_req_ang < ram4.adv_offset) {
                        delay_tmp = 0; /* can't go later than trigger */
                    } else {
                        delay_tmp = (deg_per_tooth[0] + deg_per_tooth[1]) + ram4.adv_offset -
                                     spk_req_ang;
                        /* for oddfire the calc is divided by two tooth periods so that when use
                         * do the calc we can
                         * still use the last period but the 65536 scaling calc does not overflow
                         * e.g. if last was 90 deg, but we want to delay 140 deg forwards -> overflow
                         * now  (140/(90+150) * 65536) = 38229 
                         */
                        dizzy_scaler[ix] = (unsigned int)(((unsigned long)delay_tmp << 16) /
                                           (deg_per_tooth[0] + deg_per_tooth[1]));
                    }
                } else { /* this cyl */
                    if (spk_req_ang > (ram4.adv_offset - 20)) {
                        delay_tmp = 20; /* minimum 2deg delay */
                    } else {
                        delay_tmp = ram4.adv_offset - spk_req_ang;
                    }
                    dizzy_scaler[ix] = (unsigned int)(((unsigned long)delay_tmp << 16) /
                                       (deg_per_tooth[0] + deg_per_tooth[1]));
                }
            }
        } else if ((spkmode >= 32) && (spkmode <= 39) && ((ram4.dwellmode & 3) == 0)) { // Nissan CASes with normal dwell
            /* second generation of code defines wheel based on actual
               low-res wheel, even edges only.
               During cranking use down-counter on hi-res signal for dwell+spark */

            if (outpc.rpm < ram4.crank_rpm) { // for testing only
//            if (outpc.engine & ENGINE_CRANK) { // preferred method

                int an, an2, an3;

                /* this assumes that all CASes are next-cyl */
                if (spk_req_ang < (trig_angs[0] + 20)) {
                    an3 = trig_angs[0] + 20; /* minimum 2deg earlier */
                    dwl_req_ang += an3 - spk_req_ang; /* bump dwell by same */
                    spk_req_ang = an3;
                }

                an = 7200 / no_teeth;
                an += trig_angs[0];
                an2 = an;

                an -= spk_req_ang; // this is angular delay
                an /= 20; // each hi-res tooth is 2.0 deg
                an += 1;

                an2 -= dwl_req_ang; // this is angular delay
                an2 /= 20; // each hi-res tooth is 2.0 deg
                an2 += 1;

				/* safety timer, two milliseconds later than expected
				   but during uneven cranking, time is uncertain */
				dwell_us = 2000 + dwell_long;
                SSEM0;
                spk_crk_targ = an;
                dwl_crk_targ = an2;
                CSEM0;
            } else {
                SSEM0;
                spk_crk_targ = 0;
                dwl_crk_targ = 0;
                CSEM0;
            }
        }

        map_req_ang =
            intrp_1ditable(outpc.rpm, 8,
                          (int *) ram_window.pg10.map_sample_rpms, 1,
                          (int *) ram_window.pg10.map_sample_timing,
                           10);

        wheel_fill_map_event_array(map_start_event, map_req_ang,
                                   last_tooth_time, last_tooth_ang, ram4.map_sample_duration);

        if (pin_knock_out) {
            /* knock window output enabled - uses same type of fill function as map */
            map_req_ang = intrp_1ditable(outpc.rpm, 10,
                (int *) ram_window.pg19.knock_rpms, 1,
                (int *) ram_window.pg19.knock_starts, 19);

            wheel_fill_map_event_array(knock_start_event, map_req_ang,
                last_tooth_time, last_tooth_ang,
                intrp_1ditable(outpc.rpm, 10, (int *) ram_window.pg19.knock_rpms,
                1, (int *) ram_window.pg19.knock_durations, 19) );
        }

        if (((ram4.EngStroke & 0x03) == 0x03)) {
            long spk_time_tmp, dwl_time_tmp;

            inttmp = latency_ang + *lsum1;
            spk_req_ang = inttmp;   // deg*10

            spk_time_tmp = (inttmp * (long) ticks_per_deg) / 1000L;

            dwl_time_tmp = outpc.dwell_trl * 100L;
            if (dwl_time_tmp > (long)max_dwl_time) {
                dwl_time_tmp = max_dwl_time;
            }
            dwl_time_tmp += spk_time_tmp;

            dwl_req_ang =
                (dwl_time_tmp * 1000L) / (long) ticks_per_deg;

            wheel_fill_event_array(spark_events_fill,
                    dwell_events_fill, spk_req_ang,
                    dwl_req_ang, last_tooth_time,
                    last_tooth_ang, 1);
        }

        DISABLE_INTERRUPTS;
        dwell_events = dwell_events_fill;
        spark_events = spark_events_fill;
        ENABLE_INTERRUPTS;
    }

END_WHEEL_DECODER:;
}

void do_tach_mask(void)
{
    unsigned long ltmp1;
    unsigned long tmp_ticks;

    if ((ram4.NoiseFilterOpts & 0x06) || (ram4.secondtrigopts & 0x06)) {    // any of the filters/masks enabled?
        tmp_ticks = 25000;
        if (spkmode < 2) {   // EDIS only)
            DISABLE_INTERRUPTS;
            ltmp1 = dtpred;
            ENABLE_INTERRUPTS;
            ltmp1 *= 1000;
        } else { // everything else
            if (ticks_per_deg > 25000) {        // seems to be less than 1000rpm
                tmp_ticks = 25000;      // rail at equiv of 1000, in case we reset/key off etc and need to regain sync
            } else {
                tmp_ticks = ticks_per_deg;
            }

            // a wheel or pseudo wheel. Use smallest tooth angle to determine mask time
            // This percentage (equiv) in here could be made variable like MS2 base's various next pulse tolerance settings

            ltmp1 = tmp_ticks * smallest_tooth_crk;
            //                    ltmp1 = ltmp1 / 2864; // (equiv to * 35%)
        }
        ltmp1 *= (unsigned long) ram4.ICISR_pmask;
        ltmp1 /= 100000L;
        ltmp1 += (unsigned long) ram4.ICISR_tmask * 100;

        if ((ram4.NoiseFilterOpts & 0x02) && (ltmp1 < 65535)) {
            false_period_crk_tix = (unsigned int) ltmp1;
        } else {
            false_period_crk_tix = 0;
        }

        if ((ltmp1 < 576) || (ram4.NoiseFilterOpts & 0x01) || ((ram4.NoiseFilterOpts & 0x04) == 0)) {       // too short OR crank noise filter OR disabled
            false_mask_crk = 0;
        } else {
            ltmp1 = ltmp1 / 128;
            false_mask_crk = (unsigned int) ltmp1;
        }

        if (spkmode < 2) {   // EDIS only)
            DISABLE_INTERRUPTS;
            ltmp1 = dtpred;
            ENABLE_INTERRUPTS;
            ltmp1 *= 2000;
        } else { // everything else
            ltmp1 = tmp_ticks * smallest_tooth_cam;
        }
        //                    ltmp1 = ltmp1 / 2864; // (equiv to * 35% )
        ltmp1 *= (unsigned long) ram4.IC2ISR_pmask;
        ltmp1 /= 100000L;
        ltmp1 += (unsigned long) ram4.IC2ISR_tmask * 100;

        if ((ram4.secondtrigopts & 0x02) && (ltmp1 < 65535)) {
            false_period_cam_tix = (unsigned int) ltmp1;
        } else {
            false_period_cam_tix = 0;
        }

        if ((ltmp1 < 576) || (ram4.secondtrigopts & 0x01) || ((ram4.secondtrigopts & 0x04) == 0)) { // too short OR cam noise filter OR disabled
            false_mask_cam = 0;
        } else {
            ltmp1 = ltmp1 / 128;
            false_mask_cam = (unsigned int) ltmp1;
        }

    } else {
        false_mask_crk = 0;
        false_mask_cam = 0;
        false_period_crk_tix = 0;
        false_period_cam_tix = 0;
    }
}

void calc_spk_trims(void)
{
    char trimtmp;
    unsigned int fire;
    // update one trim on each pass
    if (spk_trim_cnt >= NUM_TRIGS) { // range check first
        spk_trim_cnt = 0;
    }
    fire = ram4.fire[(unsigned int)spk_trim_cnt];

    if ((fire == 0) || (fire > (unsigned int)(ram4.no_cyl & 0x1f))) {
        fire = 1; //failsafe
    }
    fire--;

    trimtmp = (char) intrp_2dcstable(outpc.rpm, outpc.ignload, 6, 6,
            &ram_window.pg11.spk_trim_rpm[0],
            &ram_window.pg11.spk_trim_load[0],
            &ram_window.pg11.spk_trim[fire][0][0],
            11);
    spk_trim[spk_trim_cnt] = trimtmp;
    // the array is per output and IS adjusted for firing order
    spk_trim_cnt++;
}

void knock_spi(void)
{
#define KNOCK_PRESCALE 0x46 // 8MHz, enable SDO
/* knock state machine - called from 0.128ms interrupt. */
    if (knock_state == 0) {
        /* either idle or awaiting send completion */
        return;

/* init stuff */
    } else if ((knock_state == 1) || (knock_state == 2)) {
        /* intentional delay before init */
        knock_state++;

    } else if (knock_state == 3) {
        SSEM0;
        *port_knock_out &= ~pin_knock_out; // Set INT/HOLD to 0 to allow comms
        CSEM0;
        (void)SPI0SR;
        (void)SPI0DRL;
        SPI0DRL = 0x71;
        knock_state++;
    } else if (knock_state == 4) {
        if (!(SPI0SR & 0x80)) {
            return;
        }     
        knock_state++;

    } else if (knock_state == 5) {
        (void)SPI0SR;
        (void)SPI0DRL;
        SPI0DRL = KNOCK_PRESCALE;
        knock_state++;
    } else if (knock_state == 6) {
        if (!(SPI0SR & 0x80)) {
            return;
        }     
        knock_state++;
    } else if (knock_state == 7) {
        unsigned char v;
        (void)SPI0SR;
        v = SPI0DRL;
        if ((v != 0x8e) && (v != KNOCK_PRESCALE)) { // this is the response to SPI advanced code (warm boot) or echo back (cold boot)
            stat_knock |= 1;
            knock_state = 50; // failed, abandon
        } else {
        SPI0DRL = (ram5.knock_bpass & 0x3f); // set bandpass (re-set after each conversion)
        knock_state++;
        }
    } else if (knock_state == 8) {
        if (!(SPI0SR & 0x80)) {
            return;
        }     
        knock_state++;
    } else if (knock_state == 9) {
        (void)SPI0SR;
        (void)SPI0DRL;
        SPI0DRL = 0xe0; // set channel 1
        knock_state++;
    } else if (knock_state == 10) {
        if (!(SPI0SR & 0x80)) {
            return;
        }     
        knock_state++;
    } else if (knock_state == 11) {
        (void)SPI0SR;
        (void)SPI0DRL;
        SPI0DRL = knock_gain | 0x80; // set gain (re-set after each conversion)
        knock_state++;
    } else if (knock_state == 12) {
        if (!(SPI0SR & 0x80)) {
            return;
        }   
        knock_state++;
    } else if (knock_state == 13) {
        (void)SPI0SR;
        (void)SPI0DRL;
        SPI0DRL = (ram5.knock_int & 0x1f) | 0xc0; // set integration time constant (once)
        knock_state++;
    } else if (knock_state == 14) {
        if (!(SPI0SR & 0x80)) {
            return;
        }
        knock_state++;
    } else if (knock_state == 15) {
        (void)SPI0SR;
        (void)SPI0DRL;
        SPI0DRL = 0x71; // set advanced mode
        knock_state++;

    } else if (knock_state == 16) {
        if (!(SPI0SR & 0x80)) {
            return;
        }
        knock_state = 0; // back to idle

/* fetch results realtime */
    } else if (knock_state == 20) {
        /* send cmd 1 and ignore result */
        (void)SPI0SR;
        (void)SPI0DRL;
        SPI0DRL = KNOCK_PRESCALE;
        knock_state++;
    } else if (knock_state == 21) {
        if (!(SPI0SR & 0x80)) {
            return;
        }
        (void)SPI0SR;
        (void)SPI0DRL; // first byte back is nothing */
        SPI0DRL = 0xe0 | knock_chan;
        knock_state++;
    } else if (knock_state == 22) {
        if (!(SPI0SR & 0x80)) {
            return;
        }     
        (void)SPI0SR;
        knock_res = SPI0DRL;
        /* send bandpass upper 2 bits of result */
        SPI0DRL = (ram5.knock_bpass & 0x3f); // set bandpass (re-set after each conversion)
        knock_state++;
    } else if (knock_state == 23) {
        if (!(SPI0SR & 0x80)) {
            return;
        }     
        (void)SPI0SR;
        unsigned char res2;

        res2 = SPI0DRL;
        if (res2 == (ram5.knock_bpass & 0x3f)) {
            /* invalid result that matches data we just sent - chip not talking to us */
            stat_knock |= 1;
        }

        res2 = res2 & 0xc0;
        knock_res |= (((unsigned int)res2) << 2) | knock_res;
        knockin = knock_res;
        SPI0DRL = knock_gain | 0x80; // set gain (re-set after each conversion)
        knock_state++;
    } else if (knock_state == 24) {
        if (!(SPI0SR & 0x80)) {
            return;
        }     
        (void)SPI0SR;
        (void)SPI0DRL;
        knock_state = 0; // back to idle
    } else if (knock_state == 50) {
        stat_knock |= 1; // "it's bost"
        knockin = outpc.knock = 1023;

    } else if (knock_state == 100) {
      /* this is part of the retry process */
        knock_retry++;
        if (knock_retry > 3) {
            knock_state = 102; // get stuck
            knockin = outpc.knock = 1023;
        } else {
            (void)SPI0SR;
            (void)SPI0DRL;
            knock_state++;
        }
    } else if (knock_state == 101) {
        (void)SPI0SR;
        (void)SPI0DRL;
        knock_state = 1; // REDO from start
   }
}
