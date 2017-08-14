/* $Id: ms3_boost.c,v 1.15.4.4 2015/05/17 00:11:24 culverk Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * boost_ctl_init()
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * boost_ctl()
    Origin: Kenneth Culver
    Moderate: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * boost_ctl_cl()
    Origin: Kenneth Culver
    Moderate: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * boost_ctl_ol()
    Origin: Kenneth Culver
    Moderate: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
 */

#include "ms3.h"

void boost_ctl_init(void)
{
    unsigned int temp;
    
    boost_ctl_last_error[0] = 0;
    boost_ctl_last_error[1] = 0;
    boost_ctl_duty[0] = 0;
    outpc.boostduty = boost_ctl_duty[0];
    boost_ctl_duty[1] = 0;
    outpc.boostduty2 = boost_ctl_duty[1];
    boost_PID_enabled[0] = 0;
    boost_PID_enabled[1] = 0;
    boost_sensitivity_old[0] = ram4.boost_ctl_sensitivity;
    boost_sensitivity_old[1] = ram4.boost_ctl_sensitivity2;

    /* Set up the boost control interval based on frequency. Rail at 10 ms as the lowest interval */
    if ((ram4.boost_ctl_pwm & 0x30) == 0x20) {
        temp = ram4.boost_ctl_settings & 7;

        boost_ctl_ms = slow_boost_intervals[temp];
    } else {
        temp = ram4.boost_ctl_pwm & 0xF;

        boost_ctl_ms = fast_boost_intervals[temp];
    }

    boost_ctl_timer = boost_ctl_ms;
}

void boost_ctl_cl(int channel, int lowerlimit, int Kp, int Ki, int Kd, unsigned char closeduty,
                  unsigned char openduty, int sensitivity, char *outputptr)
{
    /* CLOSED LOOP BOOST */
    int targ_load, maxboost, tmp2;
    unsigned char flags = 0, start_duty = openduty;
    long tmp1;

    if ( (ram5.dualfuel_sw2 & 0x10) && (ram5.dualfuel_sw & 0x1)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
        maxboost = (int)((((long)ram4.OverBoostKpa * (100 - flexblend)) + ((long)ram4.OverBoostKpa2 * flexblend)) / 100);
    } else if (pin_tsw_ob && ((*port_tsw_ob & pin_tsw_ob) == pin_match_tsw_ob)) {
        maxboost = ram4.OverBoostKpa2;
    } else {
        maxboost = ram4.OverBoostKpa;
    }

    if (channel == 1) {
        targ_load = intrp_2ditable(outpc.rpm, outpc.tps, 8, 8,
                &ram_window.pg8.boost_ctl_loadtarg_rpm_bins[0],
                &ram_window.pg8.boost_ctl_loadtarg_tps_bins[0],
                &ram_window.pg8.boost_ctl_load_targets[0][0], 8);
        start_duty = intrp_2dctable(outpc.rpm, targ_load, 8, 8,
                &ram_window.pg25.boost_ctl_cl_pwm_rpms2[0],
                &ram_window.pg25.boost_ctl_cl_pwm_targboosts2[0],
                &ram_window.pg25.boost_ctl_cl_pwm_targs2[0][0], 0, 25);
        outpc.boost_targ_2 = targ_load;
    } else {
        if ((ram4.boost_feats & 0x20) && (outpc.status2 & STATUS2_LAUNCH)) { 
            targ_load = ram4.boost_launch_target;
        } else if ((ram4.boost_vss & 0x03) &&
                   (outpc.tps > ram4.boost_vss_tps)) {
            if ((ram4.boost_vss & 0x03) <= 2) {
                unsigned int tmp_vss;
                if ((ram4.boost_vss & 0x03) == 2) {
                    tmp_vss = outpc.vss2 / 10;
                } else {
                    tmp_vss = outpc.vss1 / 10;
                }
                targ_load =
                    intrp_1ditable(tmp_vss, 6,
                            (int *) ram_window.pg10.boostvss_speed, 0,
                            (int *) ram_window.pg10.boostvss_target, 10);
            } else { // by gear
                unsigned int tmp_gear;
                if (outpc.gear < 1) {
                    tmp_gear = 0;
                } else if (outpc.gear > 6) {
                    tmp_gear = 5;
                } else {
                    tmp_gear = outpc.gear - 1;
                }
                targ_load = ram5.boost_geartarg[tmp_gear];
            }

            /* traction control down-scaling */
            if (tc_boost != 100) {
            targ_load = outpc.baro + (int) (((targ_load - outpc.baro) * (long)tc_boost) / 100);
            }

            start_duty = intrp_2dctable(outpc.rpm, targ_load, 8, 8,
                     &ram_window.pg25.boost_ctl_cl_pwm_rpms1[0],
                     &ram_window.pg25.boost_ctl_cl_pwm_targboosts1[0],
                     &ram_window.pg25.boost_ctl_cl_pwm_targs1[0][0], 0, 25);
        } else {
            unsigned char w;
            unsigned char blend = 0;

            tmp2 = tmp1 = 0; // keep compiler happy
            if ( (ram5.dualfuel_sw2 & 0x20) && (ram5.dualfuel_sw & 0x1)
                && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
                w = 7;
            } else if ((pin_boost_tsw && ((*port_boost_tsw & pin_boost_tsw) == pin_match_boost_tsw)) ||
                (((ram4.boost_feats & 0x1f) == 15) &&
                 (outpc.gear >= ram4.boost_gear_switch))) {
                w = 2;
            } else if ((ram4.boost_feats & 0x1f) == 14) {
                w = 3;
            } else {
                w = 1;
            }

            if (w & 1) {
                tmp1 = intrp_2ditable(outpc.rpm, outpc.tps, 8, 8,
                        &ram_window.pg8.boost_ctl_loadtarg_rpm_bins[0],
                        &ram_window.pg8.boost_ctl_loadtarg_tps_bins[0],
                        &ram_window.pg8.boost_ctl_load_targets[0][0], 8);
            }
            if (w & 2) {
                tmp2 = intrp_2ditable(outpc.rpm, outpc.tps, 8, 8,
                        &ram_window.pg10.boost_ctl_loadtarg_rpm_bins2[0],
                        &ram_window.pg10.boost_ctl_loadtarg_tps_bins2[0],
                        &ram_window.pg10.boost_ctl_load_targets2[0][0], 10);
            }

            if (w == 2) {
                targ_load = tmp2;
            } else if (w == 3) { // blended
                blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[5] & 0x1f), 9, 
                        (int *) ram_window.pg25.blendx[5], 0, 
                        (unsigned char *)ram_window.pg25.blendy[5], 25);
                targ_load = (unsigned int)((((long)tmp1 * (100 - blend)) + ((long)tmp2 * blend)) / 100);
            } else if (w == 7) { /* flex blended */
                targ_load = (unsigned int)((((long)tmp1 * (100 - flexblend)) + ((long)tmp2 * flexblend)) / 100);
            } else {
                targ_load = tmp1;
            }

            if ((ram4.boost_feats & 0x40) && pin_launch) {  // launch delay and launch enabled
                unsigned char boost_pct;
                unsigned int timer;

                if (outpc.status2 & STATUS2_LAUNCH) {
                    timer = 0; // hold at start boost
                } else if (launch_timer < 32760) {
                    timer = launch_timer;
                } else {
                    goto LD_SKIP; // not in launch or timed, so skip down-scaling
                }
                boost_pct = intrp_1dctable(timer, 6, 
                                           (int *) ram_window.pg10.boost_timed_time,
                                           0, (unsigned char *)ram_window.pg10.boost_timed_pct,
                                           10);
                /* scale gauge boost, not absolute pressure */
                targ_load = outpc.baro + ((unsigned long) (targ_load - outpc.baro) * 
                             (unsigned int) boost_pct) / 100;
                LD_SKIP:;
            }
            /* traction control down-scaling */
            if (tc_boost != 100) {
                targ_load = outpc.baro + (int) (((targ_load - outpc.baro) * (long)tc_boost) / 100);
            }

            /* Always use bias table 1 (even when blending or switching) */
            start_duty = intrp_2dctable(outpc.rpm, targ_load, 8, 8,
                 &ram_window.pg25.boost_ctl_cl_pwm_rpms1[0],
                 &ram_window.pg25.boost_ctl_cl_pwm_targboosts1[0],
                 &ram_window.pg25.boost_ctl_cl_pwm_targs1[0][0], 0, 25);

        }

        outpc.boost_targ_1 = targ_load;
    }

    if (outpc.clt < ram4.boost_ctl_clt_threshold) {
        DISABLE_INTERRUPTS;
        boost_ctl_duty[channel] = 0;
        ENABLE_INTERRUPTS;
        boost_PID_enabled[channel] = 0;
        return;
    }

#define BOOST_MAP_HYST 50
    /* Do not try to control boost unless on top of target */
    {
        unsigned char ret;
        ret = 1;
        if ((outpc.map < (outpc.baro - BOOST_MAP_HYST))
            || (outpc.engine & ENGINE_CRANK)
            || (outpc.engine & ENGINE_ASE)) {
            DISABLE_INTERRUPTS;
            boost_ctl_duty[channel] = 0;
            ENABLE_INTERRUPTS;
        } else if (outpc.map >= (targ_load - lowerlimit)) {
            ret = 0;
        } else if (outpc.map >= outpc.baro) {
            DISABLE_INTERRUPTS;
            boost_ctl_duty[channel] = openduty;
            ENABLE_INTERRUPTS;
        } /* else no change */

        if (ret) {
            *outputptr = boost_ctl_duty[channel];
            boost_ctl_last_error[channel] = 0;
            boost_PID_enabled[channel] = 0;
            return;
        }
    }

    if (!boost_PID_enabled[channel]) {
        flags |= PID_INIT;
        boost_PID_enabled[channel] = 1;
        boost_ctl_duty[channel] = start_duty;
    }

    tmp1 = start_duty + (((long) (generic_ideal_pid_routine(0, sensitivity,
                        targ_load, outpc.map,
                        Kp, Ki, Kd, &boost_ctl_Iterm_sum[channel],
                        &boost_ctl_last_error[channel], start_duty,
                        closeduty, openduty, 100, 
                        flags))) / 100000L);


    if (tmp1 < closeduty) {
        tmp1 = closeduty;
    } else if (tmp1 > openduty) {
        tmp1 = openduty;
    }

    boost_ctl_duty[channel] = tmp1;
}

void boost_ctl_ol(int channel, unsigned char coldduty, char *outputptr)
{
    /* OPEN LOOP BOOST
     * lookup duty based on TPS,RPM
     */

    if (outpc.clt < ram4.boost_ctl_clt_threshold) {
        DISABLE_INTERRUPTS;
        boost_ctl_duty[channel] = coldduty;
        *outputptr = boost_ctl_duty[channel];
        ENABLE_INTERRUPTS;
        return;
    }

    if (channel == 1) {
        /* removed launch duty, vss, time based */
        boost_ctl_duty[1] = intrp_2dctable(outpc.rpm, outpc.tps, 8, 8,
                &ram_window.pg10.boost_ctl_pwmtarg_rpm_bins2[0],
                &ram_window.pg10.boost_ctl_pwmtarg_tps_bins2[0],
                &ram_window.pg10.boost_ctl_pwm_targets2[0][0],
                0, 10);
    } else {
        if ((ram4.boost_feats & 0x20) && pin_launch && (outpc.status2 & STATUS2_LAUNCH)) {
            boost_ctl_duty[channel] = ram4.boost_launch_duty;
        } else if ((ram4.boost_vss & 0x03)
                && (outpc.tps > ram4.boost_vss_tps)) {
            unsigned int tmp_vss;
            if ((ram4.boost_vss & 0x03) == 2) {
                tmp_vss = outpc.vss2;
            } else {
                tmp_vss = outpc.vss1;
            }
            boost_ctl_duty[channel] =
                intrp_1dctable(tmp_vss, 6,
                        (int *) ram_window.pg10.boostvss_speed,
                        0,
                        (unsigned char *) ram_window.
                        pg10.boostvss_duty, 10);
        } else {
            unsigned char w;
            int tmp1 = 0, tmp2 = 0;
            if ( (ram5.dualfuel_sw2 & 0x20) && (ram5.dualfuel_sw & 0x1)
                && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
                w = 7;
            } else if ((pin_boost_tsw && ((*port_boost_tsw & pin_boost_tsw) == pin_match_boost_tsw)) ||
                (((ram4.boost_feats & 0x1f) == 15) &&
                 (outpc.gear >= ram4.boost_gear_switch))) {
                w = 2;
            } else if ((ram4.boost_feats & 0x1f) == 14) {
                w = 3;
            } else {
                w = 1;
            }

            if (w & 1) {
                tmp1 = intrp_2dctable(outpc.rpm, outpc.tps, 8, 8,
                            &ram_window.pg8.boost_ctl_pwmtarg_rpm_bins[0],
                            &ram_window.pg8.boost_ctl_pwmtarg_tps_bins[0],
                            &ram_window.pg8.boost_ctl_pwm_targets[0][0], 0,
                            8);
            }
            if (w & 2) {
                tmp2 = intrp_2dctable(outpc.rpm, outpc.tps, 8, 8,
                            &ram_window.pg10.boost_ctl_pwmtarg_rpm_bins2[0],
                            &ram_window.pg10.boost_ctl_pwmtarg_tps_bins2[0],
                            &ram_window.pg10.boost_ctl_pwm_targets2[0][0],
                            0, 10);
            }

            if (w == 2) {
                boost_ctl_duty[0] = tmp2;
            } else if (w == 3) { // blended
                unsigned char blend;
                blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[5] & 0x1f), 9, 
                        (int *) ram_window.pg25.blendx[5], 0, 
                        (unsigned char *)ram_window.pg25.blendy[5], 25);
                boost_ctl_duty[0] = (unsigned int)((((long)tmp1 * (100 - blend)) + ((long)tmp2 * blend)) / 100);
            } else if (w == 7) { // flex blended */
                boost_ctl_duty[0] = (unsigned int)((((long)tmp1 * (100 - flexblend)) + ((long)tmp2 * flexblend)) / 100);
            } else {
                boost_ctl_duty[0] = tmp1;
            }

            if (pin_launch && (ram4.boost_feats & 0x40) && (launch_timer < 0x7ffe)) {  // launch delay
                unsigned char boost_pct;
                boost_pct = intrp_1dctable(launch_timer, 6, (int *) ram_window.pg10.boost_timed_time,
                        0, (unsigned char *)ram_window.pg10.boost_timed_pct, 10);
                boost_ctl_duty[channel] = (unsigned char) (((unsigned int) boost_ctl_duty[channel] *
                            (unsigned int) boost_pct) / 100);
            }

            /* Traction */
            if (tc_boost_duty_delta) {
                int tmp_duty;
                tmp_duty = boost_ctl_duty[channel] += tc_boost_duty_delta;
                /* rail to 0-255 */
                if (tmp_duty < 0) {
                    tmp_duty = 0;
                } else if (tmp_duty > 255) {
                    tmp_duty = 255;
                }
                boost_ctl_duty[channel] = (unsigned char)tmp_duty;
            }
        }
    }
}

#define MAX_BOOST 5001

void boost_ctl(void)
{
    int Kp, Ki, Kd, sensitivity;

    if (outpc.status7 & STATUS7_LIMP) {
        outpc.boostduty = ram5.cel_boost_duty;
        outpc.boostduty2 = ram5.cel_boost_duty2;
    } else if (flagbyte3 & flagbyte3_runboost) {
        DISABLE_INTERRUPTS;
        flagbyte3 &= ~flagbyte3_runboost;
        ENABLE_INTERRUPTS;
        if (ram4.boost_ctl_settings & BOOST_CTL_CLOSED_LOOP) {
            if ((ram4.boost_ctl_flags & BOOST_CTL_FLAGS_MASK) == BOOST_CTL_FLAGS_ADVANCED) {
                Kp = ram4.boost_ctl_Kp;
                Ki = ram4.boost_ctl_Ki;
                Kd = ram4.boost_ctl_Kd;
            } else if ((ram4.boost_ctl_flags & BOOST_CTL_FLAGS_MASK) == BOOST_CTL_FLAGS_BASIC) {
                Kp = 100;
                Ki = 100;
                Kd = 100;
            } else {
                Kp = 0;
                Ki = 0;
                Kd = 0;
            }

            if (boost_sensitivity_old[0] != ram4.boost_ctl_sensitivity) {
                boost_PID_enabled[0] = 0;
                boost_sensitivity_old[0] = ram4.boost_ctl_sensitivity;
            }

            sensitivity = MAX_BOOST - ram4.boost_ctl_sensitivity;

            boost_ctl_cl(0, ram4.boost_ctl_lowerlimit, Kp, Ki, Kd,
                         ram4.boost_ctl_closeduty, ram4.boost_ctl_openduty, sensitivity,
                         &outpc.boostduty);
        } else {
            boost_ctl_ol(0, 0, &outpc.boostduty); /* , 0 was ram4.boost_ctl_openduty */
        }

        outpc.boostduty = boost_ctl_duty[0];

        /* Boost control second channel - no table switching support,
         * no boost vs vss, timer. Always uses second tables */
        /* Incomplete implementation at present, worth reviewing for
         * sequential boost or true twin channel */

        if (ram5.boost_ctl_settings2 & BOOST_CTL_ON) {
            if (ram5.boost_ctl_settings2 & BOOST_CTL_CLOSED_LOOP) {
                if ((ram4.boost_ctl_flags & BOOST_CTL_FLAGS2_MASK) == BOOST_CTL_FLAGS_ADVANCED2) {
                    Kp = ram5.boost_ctl_Kp2;
                    Ki = ram5.boost_ctl_Ki2;
                    Kd = ram5.boost_ctl_Kd2;
                } else if ((ram4.boost_ctl_flags & BOOST_CTL_FLAGS2_MASK) == BOOST_CTL_FLAGS_BASIC2) {
                    Kp = 100;
                    Ki = 100;
                    Kd = 100;
                } else {
                    Kp = 0;
                    Ki = 0;
                    Kd = 0;
                }

                if (boost_sensitivity_old[1] != ram4.boost_ctl_sensitivity2) {
                    boost_PID_enabled[1] = 0;
                    boost_sensitivity_old[1] = ram4.boost_ctl_sensitivity2;
                }

                sensitivity = MAX_BOOST - ram4.boost_ctl_sensitivity2;

                boost_ctl_cl(0, ram5.boost_ctl_lowerlimit2, Kp, Ki, Kd, ram5.boost_ctl_closeduty2,
                             ram5.boost_ctl_openduty2, sensitivity, &outpc.boostduty2);
            } else {
                /* OPEN LOOP BOOST
                 * lookup duty based on TPS,RPM
                 */

                boost_ctl_ol(1, 0, &outpc.boostduty2); /* , 0 was ram5.boost_ctl_openduty2 */
            }
            outpc.boostduty2 = boost_ctl_duty[1];
        }
    }

    if ((ram4.boost_ctl_pwm & 0x30) == 0x10) { // hardware PWM
        *boostport = ((unsigned int)outpc.boostduty * 255) / 100; // uses 0-255 scale (vs. genpwm uses 0-100)
        if (ram5.boost_ctl_settings2 & BOOST_CTL_ON) {
            *port_boost2 = ((unsigned int)outpc.boostduty2 * 255) / 100;
        }
    }
    // software pwms are picked up by swpwm code
}

