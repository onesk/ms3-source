/* $Id: ms3_pid.c,v 1.14 2014/09/23 01:23:53 culverk Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * Origin: Kenneth Culver
 * Majority: Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
 */

#include "ms3.h"

void convert_unitless_percent(int min, int max, int targ, int raw_PV,
                              long *PV, long *SP)
{
    *PV = (((long)raw_PV - min) * 10000L) /
         (max - min);
    *SP = (((long)targ - min) * 10000L) /
          (max - min);
}

long generic_pid_routine(int min, int max, int targ, int raw_PV,
                         int set_Kp, int set_Ki, int set_Kd, int looptime,
                         long *PV_last_arg, long *last_error, 
                         unsigned char config)
{
    long Kp, Ki, Kd, PV, SP, error, pid_deriv, tmp1;
    int pid_divider, pid_multiplier;

    if (config & PID_LOOPTIME_RTC) {
        pid_divider = 7812;
        pid_multiplier = 781;
    } else {
        pid_divider = 1000;
        pid_multiplier = 100;
    }

    convert_unitless_percent(min, max, targ, raw_PV, &PV, &SP);
   
    error = SP - PV;

    /* Reset previous PV vals to same as PV to avoid
     * bad behavior on the first time through the loop
     */
    if (config & PID_INIT) {
        PV_last_arg[0] = PV_last_arg[1] = PV;
        *last_error = error;
    }

    pid_deriv = PV - (2 * PV_last_arg[0]) + PV_last_arg[1];

    if (config & PID_TYPE_C) {
        Kp = ((long) ((PV - PV_last_arg[0]) * (long)set_Kp));
    } else {
        Kp = ((long) ((error - *last_error) * (long)set_Kp));
        *last_error = error;
    }
    Ki = ((((long) error * looptime) / (long)pid_divider) * (long)set_Ki);
    Kd = ((long) pid_deriv * (((long) set_Kd * pid_multiplier) / looptime));

    PV_last_arg[1] = PV_last_arg[0];
    PV_last_arg[0] = PV;

    if (config & PID_TYPE_C) {
        tmp1 = Kp - Ki + Kd;
    } else {
        tmp1 = Kp + Ki - Kd;
    }

    return tmp1;
}

/* outmult needs to be the multiplier to get to 2 decimal places of precision */
long generic_ideal_pid_routine(int min, int max, int targ, int raw_PV,
                               int set_Kp, int set_Ki, int set_Kd,
                               long *I_term_sum, long *last_error, int bias,
                               int minoutput, int maxoutput, int outmult,
                               unsigned char config)
{
    long Kp, Ki, Kd, PV, SP, error, pid_deriv;
    long tmp1;

    convert_unitless_percent(min, max, targ, raw_PV, &PV, &SP);
   
    error = SP - PV;

    if (config & PID_INIT) {
	*I_term_sum = 0;
    }

    *I_term_sum += error;

    /* ensure full range of correction regardless of I term */

    tmp1 = ((maxoutput - bias) * (long)outmult * 100) / (long)set_Ki;
   
    if (tmp1 < 0) {
       tmp1 = 0;
    } 

    if (*I_term_sum > tmp1) {
	*I_term_sum = tmp1;
    }

    tmp1 = ((bias - minoutput) * (long)outmult * 100) / (long)set_Ki;

    if (tmp1 < 0) {
        tmp1 = 0;
    }

    if (*I_term_sum < -tmp1) {
	*I_term_sum = -tmp1;
    }

    pid_deriv = error - *last_error;
    *last_error = error;

    Kp = (long)set_Kp * error;
    Ki = (long)set_Ki * *I_term_sum;
    Kd = (long)set_Kd * pid_deriv;

    return Kp + Ki + Kd;
}

void generic_pid()
{
    int i;

    RPAGE = tables[27].rpg;

    for (i = 0; i < 2; i++) {
        /* Figure out if it's time for generic PID to run on this channel */
        if (ram_window.pg27.generic_pid_flags[i] & GENERIC_PID_ON) {
            if (flagbyte22 & (twopow[i])) {
                int load, target, PV;
                long pidout;
                char pidflags = PID_TYPE_C, ctmp;

                DISABLE_INTERRUPTS;
                flagbyte22 &= ~(twopow[i]);
                ENABLE_INTERRUPTS;

                /* Figure out load axis for PID target lookup */
                load = calc_outpc_input(ram_window.pg27.generic_pid_load_sizes[i],
                        ram_window.pg27.generic_pid_load_offsets[i]);

                /* Do PID target lookup */
                target = intrp_2ditable(outpc.rpm, load, 8, 8,
                        ram_window.pg27.generic_pid_axes3d[i][0],
                        ram_window.pg27.generic_pid_axes3d[i][1],
                        &ram_window.pg27.generic_pid_targets[i][0][0],
                        27);

                /* Figure out PV input (similar to load lookup) */
                PV = calc_outpc_input(ram_window.pg27.generic_pid_PV_sizes[i],
                        ram_window.pg27.generic_pid_PV_offsets[i]);

                /* Run PID routine with appropriate flags */
                ctmp = ram_window.pg27.generic_pid_flags[i] & GENERIC_PID_TYPE;
                if (ctmp == GENERIC_PID_TYPE_B) {
                    pidflags = PID_TYPE_B;
                } else if (ctmp == GENERIC_PID_TYPE_C) {
                    pidflags = PID_TYPE_C;
                } else {
                    pidflags = PID_TYPE_C;
                }
                pidout = generic_pid_routine(ram_window.pg27.generic_pid_lower_inputlims[i],
                                             ram_window.pg27.generic_pid_upper_inputlims[i],
                                             target, PV,
                                             ram_window.pg27.generic_pid_P[i],
                                             ram_window.pg27.generic_pid_I[i],
                                             ram_window.pg27.generic_pid_D[i],
                                             ram_window.pg27.generic_pid_control_intervals[i],
                                             generic_PID_PV_last[i], &generic_PID_last_error[i],
                                             pidflags) / 1000;

                /* Bound outputs to upper/lower limits */

                if (ctmp == GENERIC_PID_TYPE_B) {
                    pidout += generic_pidouts_100[i];
                } else {
                    pidout = generic_pidouts_100[i] - pidout;
                }

                if (pidout < (ram_window.pg27.generic_pid_output_lowerlims[i] * 100)) {
                    pidout = ram_window.pg27.generic_pid_output_lowerlims[i] * 100;
                } else if (pidout > (ram_window.pg27.generic_pid_output_upperlims[i] * 100)) {
                    pidout = ram_window.pg27.generic_pid_output_upperlims[i] * 100;
                }

                generic_pidouts_100[i] = pidout;
                /* divide down by 100 to get commanded duty */

                outpc.generic_pid_duty[i] = pidout / 100;
            }
        }
    }
}
