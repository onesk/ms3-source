/* $Id: ms3_ego.c,v 1.55 2014/11/28 22:39:18 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * ego_init()
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * ego_calc
    Origin: Al Grippo
    Major: Rewrite and re-arrange. Kenneth Culver.
    Minor: Extend numer EGO channels. James Murray
    Majority: Kenneth Culver
 * ego_get_targs
    Majority: Kenneth Culver
 * ego_get_targs_gl
    Origin: Al Grippo
    Moderate: Add tableswitching. James Murray
    Moderate: Own function. Kenneth Culver
    Majority: Al Grippo / Kenneth Culver / James Murray
 * ego_closed_loop_simple
    Majority: Kenneth Culver
 * ego_closed_loop_pid_dopid
    Majority: Kenneth Culver
 * do_maxafr()
    Origin: James Murray
    Majority: James Murray
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#include "ms3.h"

void ego_init(void)
{
    ego_last_run = 0;
    /* This is used for a similar function to the valve ranges for other
     * PID loops. It helps scale the PID output down for small engines with
     * large injectors for example
     */
    PID_scale_factor = 100000 / (long) ram4.ReqFuel;
    RPAGE = tables[27].rpg;
    ego_delay_ms = ram_window.pg27.ego_sensor_delay;
}


/* Main EGO routine */
void ego_calc(void)
{
    unsigned char uctmp;
    unsigned long looptime, lmms_ltch;
    int ix, authority;
    unsigned int itmp, ego_delay_ms_ltch;
    long ltmp;

    if (ram4.egoAlgorithm & EGO_ALG_DELAY) {
        RPAGE = tables[27].rpg;
        uctmp = (unsigned char)intrp_2dctable(outpc.rpm, outpc.afrload, NO_FRPMS, NO_FMAPS,
                               ram_window.pg27.ego_delay_rpms,
                               ram_window.pg27.ego_delay_loads,
                               &ram_window.pg27.ego_delay_table[0][0],
                               0, 27);

        itmp = (uctmp * 10U) + (ram_window.pg27.ego_sensor_delay * 2U);

        DISABLE_INTERRUPTS;
        ego_delay_ms_ltch = ego_delay_ms;
        ENABLE_INTERRUPTS;

        if (ego_delay_ms_ltch > 0) {
            return;
        } else {
            DISABLE_INTERRUPTS;
            ego_delay_ms = itmp;
            ENABLE_INTERRUPTS;
        }
    } else {
        if (egocount < ram4.EgoCountCmp) {
            return;
        }
    }

    DISABLE_INTERRUPTS;
    uctmp = running_seconds;
    egocount = 0;
    lmms_ltch = lmms;
    ENABLE_INTERRUPTS;

    looptime = lmms_ltch - ego_last_run;        // .128 ms units
    ego_last_run = lmms_ltch;

    if ((ram4.EgoOption & 0x03) == 0) {
        goto EGO_SETNOCOR;
    }

    if ((outpc.engine & (ENGINE_TPSACC | ENGINE_TPSDEC | ENGINE_MAPACC | ENGINE_MAPDEC) )
        || (outpc.clt < ram4.EgoTemp)
        || (outpc.tps > ram4.TPSOXLimit)
        || (((ram4.egoAlgorithm & 0x04) == 0)
          && ((outpc.afrload > ram4.MAPOXLimit) || (outpc.afrload < ram4.MAPOXMin) || (outpc.rpm < ram4.RPMOXLimit)))
        || (uctmp < ram4.ego_startdelay)
        || ((pwcalc1 == 0) && (pwcalc2 == 0))
        || (ram4.fc_ego_delay && (fc_off_time < ram4.fc_ego_delay))
        || ((ram4.egoAlgorithm & 0x03) == 3)
        || stat_afr0
        || (outpc.status3 & (STATUS3_REVLIMSFT | STATUS3_BIKESHIFT))
        || (outpc.status2 & (STATUS2_SPKCUT | STATUS2_FLATSHIFT | STATUS2_OVERBOOST_ACTIVE))
        || skipinj_pitlim
        || skipinj_revlim
        ) {

        egopstat[0] = 0;
        egopstat[1] = 0;
        egopstat[2] = 0;
        egopstat[3] = 0;
        egopstat[4] = 0;
        egopstat[5] = 0;
        egopstat[6] = 0;
        egopstat[7] = 0;
        egopstat[8] = 0;
        egopstat[9] = 0;
        egopstat[10] = 0;
        egopstat[11] = 0;
        egopstat[12] = 0;
        egopstat[13] = 0;
        egopstat[14] = 0;
        egopstat[15] = 0;
EGO_SETNOCOR:;
        outpc.egocor[0] = 1000;
        outpc.egocor[1] = 1000;
        outpc.egocor[2] = 1000;
        outpc.egocor[3] = 1000;
        outpc.egocor[4] = 1000;
        outpc.egocor[5] = 1000;
        outpc.egocor[6] = 1000;
        outpc.egocor[7] = 1000;
        outpc.egocor[8] = 1000;
        outpc.egocor[9] = 1000;
        outpc.egocor[10] = 1000;
        outpc.egocor[11] = 1000;
        outpc.egocor[12] = 1000;
        outpc.egocor[13] = 1000;
        outpc.egocor[14] = 1000;
        outpc.egocor[15] = 1000;
        goto EGO_END;
    }

    if (ram4.egoAlgorithm & 0x04) {
        authority = intrp_2dctable(outpc.rpm, outpc.afrload, NO_FRPMS, NO_FMAPS,
                ram_window.pg27.ego_auth_rpms, ram_window.pg27.ego_auth_loads,
                &ram_window.pg27.ego_auth_table[0][0], 0, 27);
    } else {
        authority = ram4.EgoLimit;
    }

    /* run closed loop per sensor */
    for (ix = 0; ix < ram4.egonum ; ix++) {
        unsigned int compare_var;

        if (ram4.EgoOption == 1) {
            compare_var = outpc.egov[ix];
        } else {
            compare_var = outpc.afr[ix];
        }

        if ((ram4.egoAlgorithm & 0x03) == 0) {
            int egostep_i = 0;
            /* Simple correction is just a simple up-and-down oscillate around
             * the target correction
             * algorithms 0 = simple, 1 = n/a, 2 = PID, 3 = no correction
             */

            if (ram4.EgoOption == 2) {
                if (compare_var < ram4.ego_lower_bound ||
                    compare_var > ram4.ego_upper_bound) {
                    outpc.egocor[ix] = 1000;
                    continue;
                }
            }

            ego_closed_loop_simple(&egostep_i, outpc.afrtgt1, compare_var);
            outpc.egocor[ix] = outpc.egocor[ix] + egostep_i;

            if (egostep_i < 0) {
                /* check lean limit */
                if (outpc.egocor[ix] < (1000 - authority)) {
                    outpc.egocor[ix] = 1000 - authority;
                }

            } else if (egostep_i > 0) {
                if (outpc.egocor[ix] > (1000 + authority)) {
                    outpc.egocor[ix] = 1000 + authority;
                }
            }

        } else { // 2 = PID
            long egostep_l = 0;

            if (ram4.EgoOption == 2) {
                if (compare_var < ram4.ego_lower_bound ||
                    compare_var > ram4.ego_upper_bound) {
                    outpc.egocor[ix] = 1000;
                    egocor_100[ix] = 100000;
                    egopstat[ix] = 0;
                    continue;
                }
            }

            ego_closed_loop_pid_dopid(&egostep_l, looptime, compare_var, outpc.afrtgt1, ix);

            egocor_100[ix] += egostep_l;

            ltmp = (long)authority * 100;

            if (egocor_100[ix] > (100000 + ltmp)) {
                egocor_100[ix] = 100000 + ltmp;
            } else if (egocor_100[ix] < (100000 - ltmp)) {
                egocor_100[ix] = 100000 - ltmp;
            }

            outpc.egocor[ix] = egocor_100[ix] / 100L;
        }
    }
    EGO_END:;
    /* Maintain historical variables */
    outpc.egocor1 = outpc.egocor[0];
    if ((ram4.EgoOption & 0x03) && (ram4.egonum > 1)) {
        outpc.egocor2 = outpc.egocor[1];
    } else {
        outpc.egocor2 = outpc.egocor1;
    }
}

/* Get targets. */
void ego_get_targs(void)
{
    if (ram4.EgoOption & 1) {  /* narrowband */
        unsigned char t;
        t = intrp_2ditable(outpc.rpm, outpc.afrload, NO_FRPMS, NO_FMAPS,
                ram_window.pg24.narrowband_tgts_rpms, ram_window.pg24.narrowband_tgts_loads,
                &ram_window.pg24.narrowband_tgts[0][0], 24);
        outpc.afrtgt1 = t - hpte_afr; 
        outpc.afrtgt2 = outpc.afrtgt1; 
    } else {                    /* wideband */
        outpc.afrtgt1 = gl_afrtgt1;
        outpc.afrtgt2 = gl_afrtgt2;
    }
}

void ego_get_targs_gl(void)
{
    unsigned char w;
    unsigned int tmp1 = 0, tmp2 =0;

    if ( (ram5.dualfuel_sw & 0x08) && (ram5.dualfuel_sw & 0x1)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
        w = 7;
    } else if (pin_tsw_afr && ((*port_tsw_afr & pin_tsw_afr) == pin_match_tsw_afr)) {      // AFR switching
        w = 2;
    } else if ((ram4.tsw_pin_afr & 0x1f) == 14) {
        w = 3;
    } else {
        w = 1;
    }

    if (w & 1) {
        tmp1 = intrp_2dctable(outpc.rpm, outpc.afrload, NO_FRPMS, NO_FMAPS,
                ram_window.pg10.frpm_tablea[0], ram_window.pg10.fmap_tablea[0],
                &ram_window.pg10.afr_table[0][0][0], 0, 10);
    }
    if (w & 2) {
        tmp2 = intrp_2dctable(outpc.rpm, outpc.afrload, NO_FRPMS, NO_FMAPS,
                ram_window.pg10.frpm_tablea[1], ram_window.pg10.fmap_tablea[1],
                &ram_window.pg10.afr_table[1][0][0], 0, 10);
    }

    if (w == 2) {
        gl_afrtgt1 = tmp2;
    } else if (w == 3) { // blend
        unsigned char blend;
        blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[4] & 0x1f), 9, 
                (int *) ram_window.pg25.blendx[4], 0, 
                (unsigned char *)ram_window.pg25.blendy[4], 25);
        gl_afrtgt1 = (unsigned int)((((unsigned long)tmp1 * (100 - blend)) + ((unsigned long)tmp2 * blend)) / 100);
    } else if (w == 7) { /* flex blended */
        gl_afrtgt1 = (unsigned int)((((unsigned long)tmp1 * (100 - flexblend)) + ((unsigned long)tmp2 * flexblend)) / 100);
    } else {
        gl_afrtgt1 = tmp1;
    }
    gl_afrtgt1 -= hpte_afr; // take off hpte_afr
    gl_afrtgt2 = gl_afrtgt1; // always
}

/* Simple, oscillating EGO control */
void ego_closed_loop_simple(int *egostep, unsigned int tgt, unsigned int compare_var)
{
    if (compare_var > tgt) {
        *egostep = ((int) ram4.EgoStep * 10);
    } else if (compare_var < tgt) {
        *egostep = -((int) ram4.EgoStep * 10);
    } else {
        *egostep = 0;
    }

    if (ram4.EgoOption == 1) {
        *egostep = -*egostep;
    }
}

/* This is the actual PID algorithm. It incorporates looptime and scales using
 * the req_fuel based scale factor
 */
void ego_closed_loop_pid_dopid(long *egostep, unsigned long looptime,
                          unsigned int ego, unsigned int afrtarg, int ix)
{
    int PV, SP, min, max;
    long *PVarray, dummy;
    unsigned char uctmp, flags = PID_TYPE_C | PID_LOOPTIME_RTC;


    /* compiler didn't like when I just used
     * a pointer to do this. It would give
     * an internal compiler error. So
     * use a local temp var instead
     */
    PVarray = egoerrm1[ix];

    /* calc PV, SP, and error. This is done in 
     * unitless percentages based on a range of 9.0 AFR to
     * 20.0 AFR. PV is the actual current afr, SP is the target
     */
    if (ram4.EgoOption == 1) {
        PV = ego;
        SP = afrtarg;
        min = 0;
        max = 1024;
    } else {
        uctmp = (ego < 90) ? 90 : ego;
        PV = uctmp;
        uctmp = (afrtarg < 90) ? 90 : afrtarg;
        SP = uctmp;
        min = 90;
        max = 200;
    }

    if (egopstat[ix] == 0) {
        egopstat[ix] = 1;
        /* Set the whole PV array to PV to avoid large initial jumps in
         * correction that are not based on the actual target
         */
        flags |= PID_INIT;
        egocor_100[ix] = (long)outpc.egocor[ix] * 100;
    }

    *egostep = (long) generic_pid_routine(min, max, SP, PV,
                              ram4.egoKP, ram4.egoKI, ram4.egoKD,
                              looptime, PVarray, &dummy, flags) /
                              (PID_scale_factor);

    if (ram4.EgoOption == 1) {
        *egostep = -*egostep;
    }
}

/* "AFR safety system"
    also does EGT overtemp */
void do_maxafr(void)
{
    unsigned char flag;
    flag = 0;
    if (maxafr_stat == 0) {
        unsigned char maxafr_tmp;
        int maxafr_load;
        int afrtarg;
        int x;

        if (ram4.maxafr_opt1 & 1) { // maxAFR on
            if ((ram4.maxafr_opt1 & 0x06) == 0) {
                maxafr_load = outpc.map;
            } else if ((ram4.maxafr_opt1 & 0x06) == 2) {
                maxafr_load = outpc.tps;
            } else {
                maxafr_load = 65535;        // shouldn't happen
            }

            /* check if above load and rpm set points */
            if ( (outpc.rpm < ram4.maxafr_en_rpm)
                || (maxafr_load < (int)ram4.maxafr_en_load)
                /* check not already doing spark or fuel cut, AFR will be all over the place */
                || (outpc.status2 & STATUS2_SPKCUT)
                || (outpc.status3 & STATUS3_CUT_FUEL) ) {
                maxafr_timer = 0;
                goto MAXAFR_EXIT;
            }

            maxafr_tmp =
                    intrp_2dctable(outpc.rpm, maxafr_load, 6, 6,
                                   ram_window.pg9.maxafr1_rpm,
                                   ram_window.pg9.maxafr1_load,
                                   &ram_window.pg9.maxafr1_afr[0][0], 0, 9);

            // check EGOs
            if ((ram4.N2Oopt & 0x04) && (ram4.N2Oopt & 0x10) &&
               ((outpc.status2 & STATUS2_NITROUS1) || (outpc.status2 & STATUS2_NITROUS2))) {        
                // nitrous on and using AFR2 for safety 
                afrtarg = intrp_2dctable(outpc.rpm, outpc.afrload, NO_FRPMS, NO_FMAPS,
                            ram_window.pg10.frpm_tablea[1], ram_window.pg10.fmap_tablea[1],
                            &ram_window.pg10.afr_table[1][0][0], 0, 10);
            } else {
                afrtarg = gl_afrtgt1;
            }

            for (x = 0; x < ram4.egonum ; x++) {
                if (((int)outpc.afr[x] - (int)afrtarg) > (int)maxafr_tmp) {
                    flag = 1;
                }
            }
        } else {
            goto MAXAFR_EXIT;
        }

        if (flag) {
            outpc.status6 |= STATUS6_AFRWARN;
        } else {
            outpc.status6 &= ~STATUS6_AFRWARN;
            outpc.status6 &= ~STATUS6_AFRSHUT;
        }

        if (flag) {
            if (maxafr_timer == 0) {
                maxafr_timer = 1;       // start counting
            } else if (maxafr_timer > ram4.maxafr_en_time) {    // too weak for too long, start shutdown sequence
                maxafr_stat = 1;        // mainloop code reads the stat and cut appropriately
                maxafr_timer = 1;       // start counting
                if (outpc.status6 & STATUS6_AFRWARN) {
                    outpc.status6 |= STATUS6_AFRSHUT; // got here through AFR
                }
            }
        } else {                // went rich again
            maxafr_timer = 0;   // stop counting
        }

    } else if (maxafr_stat == 1) {
        flag = 1;
        if (maxafr_timer > ram4.maxafr_spkcut_time) {
            /* second stage is to cut fuel totally until below set points */
            maxafr_timer = 0;   // stop counting
            maxafr_stat = 2;
        }

    } else if (maxafr_stat == 2) {
        if ((outpc.tps < ram4.maxafr_ret_tps)
            && (outpc.map < ram4.maxafr_ret_map)
            && (outpc.rpm < ram4.maxafr_ret_rpm) && (!(flagbyte9 & FLAGBYTE9_EGTADD))) {
            maxafr_stat = 0;
            outpc.status6 &= ~STATUS6_AFRWARN;
            outpc.status6 &= ~STATUS6_AFRSHUT;
        } else {
            flag = 1;
        }
    }

MAXAFR_EXIT:;
    if (flag || (flagbyte9 & FLAGBYTE9_EGTADD)) { /* maxAFR or EGT */
        SSEM0SEI;
        *port_maxafr |= pin_maxafr;
        CSEM0CLI;
    } else {
        SSEM0SEI;
        *port_maxafr &= ~pin_maxafr;
        CSEM0CLI;
    }

}
