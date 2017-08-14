/* $Id: ms3_idle.c,v 1.117.4.3 2015/03/31 02:13:11 culverk Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * idle_ac_idleup()
    Majority: Kenneth Culver
 * fan_ctl_idleup()
    Majority: Kenneth Culver
 * idle_ctl_init
    Majority: Kenneth Culver
 * idle_test_mode
    Majority: James Murray
 * idle_on_off
    Origin: Al Grippo
    Moderate: MS3 hardware. James Murray
    Majority: Al Grippo / James Murray
 * idle_iac_warmup
    Origin: Al Grippo
    Minor: Fan. Kenneth Culver
    Majority: Al Grippo
 * idle_pwm_warmup
    Origin: Al Grippo
    Minor: Fan. Kenneth Culver
    Majority: Al Grippo
 * idle_closed_loop_throttlepressed
    Majority: Kenneth Culver
 * idle_closed_loop_throttlelifted
    Majority: Kenneth Culver
 * idle_closed_loop_pid
    Majority: Kenneth Culver
 * idle_closed_loop_newtarg
    Majority: Kenneth Culver
 * idle_closed_loop
    Majority: Kenneth Culver
 * idle_ctl
    Majority: Kenneth Culver
 * move_IACmotor
    Origin: Al Grippo
    Minor: MS3 hardware. James Murray. Bug Fixes (from ms2/extra) Kenneth Culver
    Majority: Al Grippo
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/

#include "ms3.h"

void idle_voltage_compensation(void)
{
    idle_voltage_comp = intrp_1ditable(outpc.batt, 6, ram5.idle_voltage_comp_voltage,
                                       1, ram5.idle_voltage_comp_delta, 5);
}

void idle_ac_idleup(void)
{
    if (ram4.ac_idleup_settings & 0x80) {

        if(outpc.rpm < ram4.ac_idleup_min_rpm) {
            SSEM0SEI;
            *port_ac_out &= ~pin_ac_out;
            CSEM0CLI;
            ac_time_since_last_on = 0;
            return;
        }

        /* Check AC idleup input pin */
        if ((*port_ac_in & pin_ac_in) == pin_match_ac_in) {

            /* If AC on, check for TPS/VSS shutoff */
            if (flagbyte16 & FLAGBYTE16_AC_ENABLE) {
                unsigned char ac_disable = 0;
                /* presently on */
                if (outpc.tps > ram4.ac_idleup_tps_offpoint) {
                    flagbyte16 |= FLAGBYTE16_AC_TPSHYST;
                    ac_disable = 1;
                } else if ((flagbyte16 & FLAGBYTE16_AC_TPSHYST) 
                    && (outpc.tps > (ram4.ac_idleup_tps_offpoint - ram4.ac_idleup_tps_hyst))) {
                    ac_disable = 1;
                } else {
                    flagbyte16 &= ~FLAGBYTE16_AC_TPSHYST;
                }

                if (ram4.vss_opt & 0x03) {
                    /* same thing as above for VSS */
                    if (outpc.vss1 >= ram4.ac_idleup_vss_offpoint) {
                        flagbyte16 |= FLAGBYTE16_AC_VSSHYST;
                        ac_disable = 1;
                    } else if ((flagbyte16 & FLAGBYTE16_AC_VSSHYST)
                        && (outpc.vss1 >= (ram4.ac_idleup_vss_offpoint - ram4.ac_idleup_vss_hyst))) {
                            ac_disable = 1;
                    } else {
                        flagbyte16 &= ~FLAGBYTE16_AC_VSSHYST;
                    }
                }

                if (outpc.rpm > ram5.ac_idleup_max_rpm) {
                    ac_disable = 1;
                }

                if (ac_disable) {
                    if (outpc.status7 & STATUS7_ACOUT) {
                        ac_time_since_last_on = 0;
                    }
                    SSEM0SEI;
                    *port_ac_out &= ~pin_ac_out;
                    CSEM0CLI;
                    outpc.status7 &= ~STATUS7_ACOUT;
                    return;
                }
            }

            if ( (outpc.tps < ram4.ac_idleup_tps_offpoint)
                && (!((ram4.vss_opt & 0x0F) && (outpc.vss1 > ram4.ac_idleup_vss_offpoint)))
                && (ac_time_since_last_on > ram4.ac_delay_since_last_on) ) {
                /* button pressed and conditions met, start timer */
                if (last_acbutton_state == 0) {
                    last_acbutton_state = 1;
                    ac_idleup_timer = 0;
                    ac_idleup_adder = ram4.ac_idleup_adder;
                    ac_idleup_mapadder = ram4.ac_idleup_cl_lockout_mapadder;
                    ac_idleup_cl_targetadder = ram4.ac_idleup_cl_targetadder;
                    flagbyte16 &= ~FLAGBYTE16_AC_TPSHYST;
                    flagbyte16 &= ~FLAGBYTE16_AC_VSSHYST;
		    flagbyte25 |= FLAGBYTE25_IDLE_TEMPDISABLE;
                    return;
                }

                if (ac_idleup_timer >= ram4.ac_idleup_delay) {
                    SSEM0SEI;
                    *port_ac_out |= pin_ac_out;
                    CSEM0CLI;
                    outpc.status7 |= STATUS7_ACOUT;
                    flagbyte16 |= FLAGBYTE16_AC_ENABLE;
		    flagbyte25 &= ~FLAGBYTE25_IDLE_TEMPDISABLE;
                }
            }
        } else {
            if (last_acbutton_state == 1) {
                last_acbutton_state = 0;
                ac_idleup_timer = 0;
                ac_idleup_adder = 0;
                ac_idleup_mapadder = 0;
                ac_idleup_cl_targetadder = 0;
                ac_time_since_last_on = 0;
                return;
            }

            if (ac_idleup_timer >= ram4.ac_idleup_delay) {
                SSEM0SEI;
                *port_ac_out &= ~pin_ac_out;
                CSEM0CLI;
                outpc.status7 &= ~STATUS7_ACOUT;
                flagbyte16 &= ~FLAGBYTE16_AC_ENABLE;
            }
        }
    }
}

void fan_ctl_idleup(void)
{
    unsigned char fan_disable = 0;
         
    if (ram4.fanctl_settings & 0x80) {
        if ((ram4.fanctl_opt2 & 0x01) == 0) { 
            if ((outpc.rpm < 3) || (flagbyte2 & flagbyte2_crank_ok)) { // don't run when engine stalled or just after start
                SSEM0SEI;
                *port_fanctl_out &= ~pin_fanctl_out;
                CSEM0CLI;
                outpc.status6 &= ~STATUS6_FAN;
                return;
            }
        } else { // do
            if ((outpc.engine & ENGINE_CRANK) || (outpc.batt < 100)) {
                SSEM0SEI;
                *port_fanctl_out &= ~pin_fanctl_out; // but off when cranking
                CSEM0CLI;
                outpc.status6 &= ~STATUS6_FAN;
                return;
            }
        }

        if (flagbyte16 & FLAGBYTE16_FAN_ENABLE) {
            /* presently on */
            if (outpc.tps > ram4.fan_idleup_tps_offpoint) {
                flagbyte16 |= FLAGBYTE16_FAN_TPSHYST;
                fan_disable = 1;
            } else if ((flagbyte16 & FLAGBYTE16_FAN_TPSHYST) 
                && (outpc.tps > (ram4.fan_idleup_tps_offpoint - ram4.fan_idleup_tps_hyst))) {
                fan_disable = 1;
            } else {
                flagbyte16 &= ~FLAGBYTE16_FAN_TPSHYST;
            }

            if (ram4.vss_opt & 0x03) {
                /* same thing as above for VSS */
                if (outpc.vss1 >= ram4.fan_idleup_vss_offpoint) {
                    flagbyte16 |= FLAGBYTE16_FAN_VSSHYST;
                    fan_disable = 1;
                } else if ((flagbyte16 & FLAGBYTE16_FAN_VSSHYST)
                    && (outpc.vss1 >= (ram4.fan_idleup_vss_offpoint - ram4.fan_idleup_vss_hyst))) {
                    fan_disable = 1;
                } else {
                    flagbyte16 &= ~FLAGBYTE16_FAN_VSSHYST;
                }
            }

            if (fan_disable) {
                SSEM0SEI;
                *port_fanctl_out &= ~pin_fanctl_out;
                CSEM0CLI;
                outpc.status6 &= ~STATUS6_FAN;
            }
        }

        /* Is CLT above threshold? */
        /* For AC button, only turn on fan if the user wants the fan on with AC */
        if ((outpc.clt >= ram4.fanctl_ontemp) || ((last_acbutton_state == 1) && 
                                                   ram4.fan_ctl_settings2 & 0x01)) {
            /* Not disabled, so start timers and such. */
            if (last_fan_state == 0) {
                last_fan_state = 1;
                fan_idleup_timer = 0;
                if ((ram4.fanctl_settings & 0x40) || (ram4.ac_idleup_settings & 0x80)) {
                    fan_idleup_adder = ram4.fanctl_idleup_adder;
                    fan_idleup_cl_targetadder = ram4.fan_idleup_cl_targetadder;
                    flagbyte16 &= ~FLAGBYTE16_FAN_TPSHYST;
                    flagbyte16 &= ~FLAGBYTE16_FAN_VSSHYST;
		    flagbyte25 |= FLAGBYTE25_IDLE_TEMPDISABLE;
                }
                return;
            }

            if ((fan_idleup_timer >= ram4.fanctl_idleup_delay) && (!fan_disable)) {
                SSEM0SEI;
                *port_fanctl_out |= pin_fanctl_out;
                CSEM0CLI;
                outpc.status6 |= STATUS6_FAN;
                flagbyte16 |= FLAGBYTE16_FAN_ENABLE;
		flagbyte25 &= ~FLAGBYTE25_IDLE_TEMPDISABLE;
            }
        }

        if ((outpc.clt <= ram4.fanctl_offtemp) && ((last_acbutton_state == 0) || (!(ram4.fan_ctl_settings2 & 0x01)))) {
            /* cool enough and AC off if linked to fan */
            if (last_fan_state == 1) {
                last_fan_state = 0;
                fan_idleup_timer = 0;
                if (ram4.fanctl_settings & 0x40) {
                    fan_idleup_adder = 0;
                    fan_idleup_cl_targetadder = 0;
                }
                return;
            }

            if (fan_idleup_timer >= ram4.fanctl_idleup_delay) {
                SSEM0SEI;
                *port_fanctl_out &= ~pin_fanctl_out;
                CSEM0CLI;
                outpc.status6 &= ~STATUS6_FAN;
                flagbyte16 &= ~FLAGBYTE16_FAN_ENABLE;
            }
        }
    }
}

void idle_ctl_init(void)
{
    pwmidle_stepsize = 0;
    pwmidle_numsteps = 0;
    pwmidle_targ_stepsize = 0;
    pwmidle_targ_numsteps = 0;
    pwmidle_targ_last = 0;
    PV_last[0] = PV_last[1] = 0;
    ac_idleup_adder = last_acbutton_state = 0;
    fan_idleup_adder = last_fan_state = 0;
    ac_idleup_mapadder = 0;
    ac_idleup_timer = fan_idleup_timer = 0;
    idle_wait_timer = 0;
    pwmidle_shift_timer = ram4.pwmidle_shift_open_time;
    idle_voltage_comp = 0;
    pwmidle_max_rpm_last = ram4.pwmidle_max_rpm;
}

void idle_test_mode(void)
{
    int pos_tmp;
    pos_tmp = ram5.iacpostest;  // adding this temp var halves the code size for this if {} section
    //for PWM valves, scale the result
    if ((IdleCtl == 4) || (IdleCtl == 6)) {   // 4 or 6
        if (pos_tmp > 100) {
            pos_tmp = 100;
        }
        pos_tmp = (pos_tmp * 256) / 100;        // means user enters 0-100% // sensible code with temp var, crap without
        if (pos_tmp > 255) {
            pos_tmp = 255;
        }
    }
    IACmotor_pos = pos_tmp;

    if (iactest_glob == 1) {   // home feature
        iactest_glob = 2; // done it
        if ((IdleCtl == 2) || (IdleCtl == 3) ||
            (IdleCtl == 5) || (IdleCtl == 7) || (IdleCtl == 8)) {
            DISABLE_INTERRUPTS;
            motor_step = -1;
            if (ram4.IdleCtl & 0x20) { // home open
                outpc.iacstep = ram4.iacfullopen - ram5.iachometest;  // set current motor step position (should be negative)
                IACmotor_pos = ram4.iacfullopen; // target position
            } else {
                outpc.iacstep = ram5.iachometest;  // set current motor step position
                IACmotor_pos = 0; // target position
            }
            ENABLE_INTERRUPTS;
            (void)move_IACmotor();
        }
    } else if (iactest_glob == 3) { // run mode
        if ((outpc.iacstep != IACmotor_pos)) {
            // move IAC motor to new step position
            move_IACmotor();
        }
    } else if (iactest_glob == 4) { // cycle in-out mode
        /* only get into test mode when motor not moving */
        /* ignore homing direction setting, run the number of homing steps in each direction */
        if (outpc.iacstep == 0) {
            /* at closed position, go open */
            motor_step = -1;
            IACmotor_pos = ram5.iachometest; // target position
        } else {
            IACmotor_pos = 0;
        }
        (void)move_IACmotor();
    }
}

void idle_on_off(void)
{
    if (outpc.clt < (ram4.FastIdle - ram4.IdleHyst)) {
        SSEM0SEI;
        *port_idleonoff |= pin_idleonoff;
        CSEM0CLI;
    } else if (outpc.clt > ram4.FastIdle) {
        SSEM0SEI;
        *port_idleonoff &= ~pin_idleonoff;
        CSEM0CLI;
    }
}

void idle_iac_warmup(void)
{
    int pos, tmp2, crankpos, runpos;

    crankpos =
        intrp_1ditable(outpc.clt, 4,
                       (int *) ram_window.pg8.pwmidle_crank_clt_temps, 0,
                       (unsigned int *) ram_window.
                       pg8.pwmidle_crank_dutyorsteps, 8);
    runpos =
                CW_table(outpc.clt, (int *) ram_window.pg10.iacstep_table,
                        (int *) ram_window.pg10.temp_table, 10);


    if ((outpc.engine & ENGINE_CRANK) || (outpc.rpm == 0)) {
        pos = crankpos;
        tble_motor_pos = pos;
    } else {
        // after cranking flare back to normal temperature dependent pos
        if ((outpc.seconds >= tcrank_done) &&
            (outpc.seconds <= tcrank_done + ram4.IACcrankxt)) {
            // 0 steps is fully closed (hot idle) and IACStart steps is maximum number of steps to home valve
            pos = runpos;
            if (pos < crankpos) {
                unsigned long t_lmms;
                unsigned int t, u;

                /* calc in deciseconds for smoother taper from crank to run */
                DISABLE_INTERRUPTS;
                t_lmms = lmms;
                ENABLE_INTERRUPTS;
                t_lmms -= tcrank_done_lmms;
                t = t_lmms / 781; // deciseconds since crank->run
                u = ram4.IACcrankxt * 10; // deciseconds of target crank->run time

                if (t < u) {
                    tmp2 = ((long)(crankpos - pos) * (u - t)) / u;
                } else {
                    tmp2 = 0;
                }
                pos += tmp2;
            }
            tble_motor_pos = pos;

            // check if there has been a significant change in clt temp
        } else if ((outpc.clt < (last_iacclt - ram4.IdleHyst)) ||
                (outpc.clt > last_iacclt)) {
            tble_motor_pos = runpos;
        } // else tble_motor_pos unchanged

        pos = tble_motor_pos + datax1.IdleAdj; /* Apply whole of remote adjustment */
        if (pos < 0) {
            pos = 0;
        }

        if (ac_idleup_adder > 0) {
            pos += ac_idleup_adder;
        }
        if (fan_idleup_adder > 0) {
            pos += fan_idleup_adder;
        }
    }

    IACmotor_pos = pos;

    if (outpc.iacstep != IACmotor_pos) {
        // move IAC motor to new step position
        if (IdleCtl != 4) {
            if (move_IACmotor())
                last_iacclt = outpc.clt;
        } else {
            (void)move_IACmotor();
            last_iacclt = outpc.clt;
        }
    }
}

void idle_pwm_warmup(void)
{
    int tmp1, tmp2, pos;

    pos =
        intrp_1ditable(outpc.clt, 4,
                       (int *) ram_window.pg8.pwmidle_crank_clt_temps, 0,
                       (unsigned int *) ram_window.
                       pg8.pwmidle_crank_dutyorsteps, 8);

    //pwmidle warmup only simplified
    if ((outpc.engine & ENGINE_CRANK) || (outpc.rpm == 0)) {
        IACmotor_pos = pos;
    } else {
        // after cranking taper back to normal temperature dependent pos
        // 0% duty is fully closed
        RPAGE = tables[10].rpg; //HARDCODED
        if (outpc.clt < ram_window.pg10.temp_table[0]) {
            tmp2 = ram_window.pg10.pwmidle_table[0];
        } else if (outpc.clt > ram_window.pg10.temp_table[9]) {
            tmp2 = ram_window.pg10.pwmidle_table[9];
        } else {
            tmp2 =
                CW_table(outpc.clt, (int *) ram_window.pg10.pwmidle_table,
                         (int *) ram_window.pg10.temp_table, 10);
        }

        if ((outpc.seconds >= tcrank_done) &&
            (outpc.seconds <= (tcrank_done + ram4.pwmidlecranktaper))) {
            if (tmp2 <= pos) {
                tmp1 = (int) ((long) (pos - tmp2) *
                        (tcrank_done + ram4.pwmidlecranktaper - outpc.seconds) /
                        ram4.pwmidlecranktaper);
                tmp2 += tmp1;
            }
        }

        if (ac_idleup_adder > 0) {
            tmp2 += ac_idleup_adder;
        }
        if (fan_idleup_adder > 0) {
            tmp2 += fan_idleup_adder;
        }
        tmp2 += idle_voltage_comp;
        tmp2 += datax1.IdleAdj;  /* Apply whole of remote adjustment */

        //check range
        if (tmp2 > 255) {
            IACmotor_pos = 255;
        } else if (tmp2 < 0) {
            IACmotor_pos = 0;
        } else {
            IACmotor_pos = tmp2;
        }
        (void)move_IACmotor();
    }
}

#define VALVE_CLOSED 1
#define VALVE_CLOSED_PARTIAL 2

void idle_closed_loop_throttlepressed(unsigned int rpm_thresh)
{
    int pos = IACmotor_pos;

    outpc.status2 &= ~STATUS2_CLIDLE;

    if (outpc.rpm > rpm_thresh) {       /* should we close the valve? */
        if (IACmotor_pos_saved == IACmotor_last) {
            /* IF so, the number of steps is the delay in ms divided by how 
             * often we will run, and the step size is the amount to move total
             * divided by the number of steps
             */
            if (ram4.pwmidle_close_delay) {
                pwmidle_numsteps =
                    ((long) (ram4.pwmidle_close_delay * 1000)) /
                    ram4.pwmidle_ms;
                if (pwmidle_numsteps > 0) {
                    pwmidle_stepsize = (IACmotor_pos - ram4.pwmidle_closed_duty) /
                        pwmidle_numsteps;
                    if (pwmidle_stepsize == 0) {
                        pwmidle_stepsize = 1;
                    }
                } else {
                    pos = ram4.pwmidle_closed_duty;
                    valve_closed = VALVE_CLOSED;
                }

                if (pwmidle_stepsize == 0) {
                    pwmidle_stepsize = 1;
                }
            } else {
                pos = IACmotor_last;
                valve_closed = VALVE_CLOSED;
            }
        }
        if (flagbyte2 & flagbyte2_runidle) {
            if ((pos > ram4.pwmidle_closed_duty)) {
                pos -= pwmidle_stepsize;
                valve_closed = VALVE_CLOSED;
            }
            if (pos <= ram4.pwmidle_closed_duty) {
                pos = ram4.pwmidle_closed_duty;
            }

            DISABLE_INTERRUPTS;
            flagbyte2 &= ~flagbyte2_runidle;
            IACmotor_pos = pos;
            ENABLE_INTERRUPTS;
            IACmotor_pos_saved = pos;
        }
    }

    pwmidle_reset = PWMIDLE_RESET_JUSTLIFTED;

    idle_wait_timer = 0;
    pwmidle_shift_timer = 0;
}

void idle_closed_loop_throttlelifted(unsigned int rpm_thresh, int rpm_targ)
{
    int y_val_lookup;
    char tmp = 0;

    PV_last[0] = PV_last[1] = 0;

    if (ram4.pwmidle_cl_opts & PWMIDLE_CL_USE_INITVALUETABLE) {
        if (ram4.pwmidle_cl_opts & PWMIDLE_CL_INITVAL_CLT) {
            y_val_lookup = outpc.clt;
        } else {
            y_val_lookup = outpc.mat;
        }
        IACmotor_last = intrp_2dctable(rpm_targ, y_val_lookup, 5, 5,
                                       &ram_window.pg19.pwmidle_cl_initialvalue_rpms[0],
                                       &ram_window.pg19.pwmidle_cl_initialvalue_matorclt[0],
                                       &ram_window.pg19.pwmidle_cl_initialvalues[0][0],
                                       0, 19);
    }

    if (((!ram4.pwmidle_close_delay) ||
        (outpc.rpm <= ram4.pwmidle_shift_lower_rpm) ||
        (pwmidle_shift_timer > ram4.pwmidle_shift_open_time)) && 
        (!(pwmidle_reset & PWMIDLE_RESET_DPADDED))) {

        if (valve_closed == VALVE_CLOSED) {
            IACmotor_pos_saved = IACmotor_last + ram4.pwmidle_dp_adder;
            tmp = PWMIDLE_RESET_DPADDED;
        } else {
            IACmotor_pos_saved = IACmotor_last;
        }
    }

    if (IACmotor_pos_saved > (int) ram4.pwmidle_open_duty) {
        IACmotor_pos_saved = ram4.pwmidle_open_duty;
    }

    /* Using VSS for Idle speed control?
     * Wait till <= minimum (default zero) and then go into PID
     */
    if (ram4.IdleCtl & 0x10) {
        if (outpc.vss1 <= ram5.idleminvss) {
            idle_wait_timer = 0;
            pwmidle_reset = PWMIDLE_RESET_CALCNEWTARG;
        }
        goto throttlelifted_end;
    }

    /* check for PID lockout */
    if (flagbyte2 & flagbyte2_runidle) {
        int rpmdot, tmpload;

        DISABLE_INTERRUPTS;
        flagbyte2 &= ~flagbyte2_runidle;
        ENABLE_INTERRUPTS;

        rpmdot = outpc.rpmdot;

        if (rpmdot < 0) {
            rpmdot = -rpmdot;
        }

        if ((ram4.FuelAlgorithm & 0xf) == 3) {
            tmpload = outpc.map;
        } else {
            tmpload = outpc.fuelload;
        }

        if (idle_wait_timer >= ram4.pwmidle_pid_wait_timer) {
            if (rpmdot < ram4.pwmidle_rpmdot_threshold) {
                if (tmpload > ram4.pwmidle_decelload_threshold + ac_idleup_mapadder) {
                    pwmidle_reset = PWMIDLE_RESET_CALCNEWTARG;
                    valve_closed = 0;
                }
            }
        } else {
            /* If the timer is not yet expired... reset the timer to
             * zero if rpmdot or load are outside the acceptable range
             * in order to keep this feature from false triggering
             */
            if ((rpmdot >= ram4.pwmidle_rpmdot_threshold) || 
                (tmpload <= ram4.pwmidle_decelload_threshold)) {
                    idle_wait_timer = 0;
            }
        }
    }
throttlelifted_end:
    pwmidle_reset |= tmp;
}

void idle_closed_loop_pid(int cl_targ_rpm, unsigned int rpm_thresh, char savelast)
{
    long last_error_unused;
    int rpm, pos;
    unsigned char flags = PID_TYPE_C;
    unsigned int Kp, Ki, Kd;

    outpc.status2 |= STATUS2_CLIDLE;      // turn on CL indicator

    /* Convert the rpms to generic percents in % * 10000 units.
     * Avoid going below the min rpm set by the user.
     * That will cause a large duty cycle spike
     */
    rpm = outpc.rpm;

    if ((pwmidle_reset & PWMIDLE_RESET_CALCNEWTARG) ||
        (pwmidle_reset & PWMIDLE_RESET_JUSTCRANKED)) {
        flags |= PID_INIT;
        IACmotor_100 = IACmotor_pos_saved * 100;
    }

    if (ram4.pwmidle_cl_opts & PWMIDLE_CL_DISPLAY_PID) {
        Kp = ram4.pwmidle_Kp;
        Ki = ram4.pwmidle_Ki;
        Kd = ram4.pwmidle_Kd;
    } else {
        Kp = 1000;
        Ki = 1000;
        Kd = 0;
    }

    if (pwmidle_max_rpm_last != ram4.pwmidle_max_rpm) {
        flags |= PID_INIT;
        pwmidle_max_rpm_last = ram4.pwmidle_max_rpm;
    }

    pos = IACmotor_100 - (generic_pid_routine(0, 4001 - ram4.pwmidle_max_rpm,
                                               cl_targ_rpm, rpm,
                                               Kp,
                                               Ki,
                                               Kd,
                                               ram4.pwmidle_ms,
                                               PV_last, &last_error_unused, flags) / 1000);

    if (pos < (ram4.pwmidle_closed_duty * 100)) {
        pos = ram4.pwmidle_closed_duty * 100;
    } else if (pos > (ram4.pwmidle_open_duty * 100)) {
        pos = ram4.pwmidle_open_duty * 100;
    }

    IACmotor_100 = pos;
    IACmotor_pos_saved = IACmotor_100 / 100;

    if ((outpc.rpmdot < ram4.pwmidle_rpmdot_disablepid) || (ram4.IdleCtl & 0x10)) {
        if (savelast && (outpc.rpm < rpm_thresh)) {
            IACmotor_last = IACmotor_pos_saved;
        }
    } else {
        if ((outpc.rpm >= rpm_thresh) &&
            (pwmidle_reset & PWMIDLE_RESET_PID)) {
            pwmidle_reset = PWMIDLE_RESET_JUSTLIFTED;
            outpc.status2 &= ~STATUS2_CLIDLE;
            idle_wait_timer = 0;
            return;
        }
    }

    pwmidle_reset = PWMIDLE_RESET_PID;
}

int idle_closed_loop_newtarg(unsigned int nt_targ_rpm, unsigned char ramptime)
{
    int new_targ, rpm;

    /* This function gradually drops the target from where it is now
     * to the end target
     */
    if ((pwmidle_reset & PWMIDLE_RESET_CALCNEWTARG) ||
        (pwmidle_reset & PWMIDLE_RESET_JUSTCRANKED)) {
        /* first time here, figure out number of steps
         * and the amount per step
         */

        if (outpc.rpm > nt_targ_rpm) {
            rpm = outpc.rpm;
            pwmidle_targ_numsteps = ((unsigned int)ramptime * 1000) /
                ram4.pwmidle_ms;
            if (pwmidle_targ_numsteps > 0) {
                pwmidle_targ_stepsize =
                    (rpm - nt_targ_rpm) / pwmidle_targ_numsteps;
                if (pwmidle_targ_stepsize == 0) {
                    pwmidle_targ_stepsize = 1;
                }
                pwmidle_targ_last = rpm;
                return rpm;
            } else {
                pwmidle_targ_last = 0;
            }
        } else {
            pwmidle_targ_last = 0;
        }
    }

    if (pwmidle_targ_last != 0) {
        new_targ = pwmidle_targ_last - pwmidle_targ_stepsize;
        if (new_targ >= (int)nt_targ_rpm) {
            pwmidle_targ_last = new_targ;
            return new_targ;
        }
        pwmidle_targ_last = 0;
    }
    return nt_targ_rpm;
}

void idle_target_lookup(void)
{
    if ((IdleCtl == 6) || (IdleCtl == 7) || (IdleCtl == 8) ||
        (ram4.idle_special_ops & IDLE_SPECIAL_OPS_CLIDLE_TIMING_ASSIST)) {
        targ_rpm = intrp_1ditable(outpc.clt, 8,
                (int *) ram_window.pg8.pwmidle_clt_temps, 0,
                (unsigned int *) ram_window.pg8.pwmidle_target_rpms,
                8);
        targ_rpm += ac_idleup_cl_targetadder + fan_idleup_cl_targetadder;
        if (IdleCtl < 6) {
            outpc.cl_idle_targ_rpm = targ_rpm; // target for open-loop
        }
    } 
}

void idle_closed_loop(void)
{
    unsigned int rpm_thresh;
    int pos;

    rpm_thresh = targ_rpm + 200;
    if ((outpc.engine & ENGINE_CRANK) || (outpc.rpm == 0)) {
        pos =
            intrp_1ditable(outpc.clt, 4,
                    (int *) ram_window.pg8.pwmidle_crank_clt_temps,
                    0,
                    (unsigned int *) ram_window.
                    pg8.pwmidle_crank_dutyorsteps, 8);
        /* cranking */
        pos += idle_voltage_comp;
        DISABLE_INTERRUPTS;
        IACmotor_pos = pos;
        IACmotor_last = pos;
        IACmotor_pos_saved = pos;
        ENABLE_INTERRUPTS;
        idle_wait_timer = 0;
        valve_closed = 0;
        outpc.status2 &= ~STATUS2_CLIDLE;
        pwmidle_reset = PWMIDLE_RESET_JUSTCRANKED;
    } else {
        if (outpc.tps >= (int) ram4.pwmidle_tps_thresh) {
            idle_closed_loop_throttlepressed(rpm_thresh);
        } else {
            if (pwmidle_reset & PWMIDLE_RESET_JUSTLIFTED) {
                idle_closed_loop_throttlelifted(rpm_thresh, targ_rpm);
            } else {
                /* just after crank */
                unsigned char ramptime;

                if (pwmidle_reset & PWMIDLE_RESET_JUSTCRANKED) {
                    ramptime = ram4.pwmidlecranktaper;
                } else {
                    ramptime = ram4.pwmidle_targ_ramptime;
                }

                if ((flagbyte2 & flagbyte2_runidle) &&
                    (idle_wait_timer >= ram4.pwmidle_pid_wait_timer) && !IAC_moving &&
		    (!(flagbyte25 & FLAGBYTE25_IDLE_TEMPDISABLE))) {
                    unsigned int adj_targ;
                    adj_targ = idle_closed_loop_newtarg(targ_rpm, ramptime);
                    idle_closed_loop_pid(adj_targ, rpm_thresh, adj_targ == targ_rpm);
                    DISABLE_INTERRUPTS;
                    flagbyte2 &= ~flagbyte2_runidle;
                    ENABLE_INTERRUPTS;
                    outpc.cl_idle_targ_rpm = adj_targ; // the actual target we are using including taper
                } 
            }
     
            pos = IACmotor_pos_saved;

            if ((!ram4.pwmidle_close_delay) ||
                    (outpc.rpm <= ram4.pwmidle_shift_lower_rpm) ||
                    (pwmidle_shift_timer > ram4.pwmidle_shift_open_time)) {
                pos += ac_idleup_adder;
                pos += fan_idleup_adder;
                pos += idle_voltage_comp;
            }

            pos += datax1.IdleAdj - idleadj_last; /* Apply delta of remote idle adjustment once */
            idleadj_last = datax1.IdleAdj;

            if (pos < (int)ram4.pwmidle_closed_duty) {
                pos = ram4.pwmidle_closed_duty;
            } else if (pos > (int)ram4.pwmidle_open_duty) {
                pos = ram4.pwmidle_open_duty;
            }

            IACmotor_pos = pos;
        }
    }

    if (IdleCtl >= 6) {
        (void) move_IACmotor();
    }
}

void idle_ctl(void)
{
    if (pwmidle_reset & PWMIDLE_RESET_INIT) {
        if (!IAC_moving) {
            // When stepper idle has stopped homing, clear the flag
            // for CL modes, calc a new target
            pwmidle_reset = PWMIDLE_RESET_CALCNEWTARG;
        } else {
            return; // do nothing else until it homed (steppers only)
        }
    }

    if (IdleCtl == 4 || IdleCtl == 6) {
        idle_voltage_compensation();
    }
    if (flagbyte23 & FLAGBYTE23_ALS_OUT) {
        // ALS alters idle position, move stepper, but skip other idle routines
        (void) move_IACmotor();
    } else if (iactest_glob) {
        if (!IAC_moving) {
            idle_test_mode();
        }
    } else {
        if (IdleCtl == 1) {
            idle_on_off();
        } else if ((IdleCtl == 2) || (IdleCtl == 3) || (IdleCtl == 5)) {
            idle_iac_warmup();
        } else if (IdleCtl == 4) {
            idle_pwm_warmup();
        } else if ((IdleCtl == 6) || (IdleCtl == 7) || (IdleCtl == 8)) {
            idle_closed_loop();
        }
    }

    if ((IdleCtl == 4) || (IdleCtl == 6)) {
        (void) move_IACmotor(); // ensure that duty is set now that code removed from isr_rtc
    }
}

int move_IACmotor(void)
{
//    unsigned char coils;
    short del;

    if (IAC_moving || (outpc.iacstep == IACmotor_pos)) {
        return 0;
    }

    if ((IdleCtl == 4) || (IdleCtl == 6)) {
        if ((iactest_glob == 0) &&
            ((ram4.pwmidle_freq_pin3 & 0x80) == 0) && ((outpc.engine & ENGINE_READY) == 0)) {
            IACmotor_pos = 0; // force it off
        }
        /* PWM idle */
        if (IACmotor_pos > 255) {
            IACmotor_pos = 255;
        }
        *port_idlepwm = (unsigned char)IACmotor_pos;
        *port_idlepwm3 = (unsigned char)IACmotor_pos;
        outpc.iacstep = IACmotor_pos;
        return 0;
    }

    /* The rest is only for stepper motors */
    del = IACmotor_pos - outpc.iacstep;
    if (del < 0) {
        del = -del;
    }
    if (del < ram4.IACminstep) {
        return 0;
    }

    // set up the motor move
    motor_time_ms = ram4.IAC_tinitial_step; // units changed to 1 ms
    // enable current to motor always(set bit= 0)
    iac_dty = 0;
    IAC_moving = 2;

    return 1;
}
