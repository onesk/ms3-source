/* $Id: ms3_inj.c,v 1.255.4.3 2015/04/08 21:08:34 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 *
 * calc_opentime
    Origin: James Murray
    Majority: James Murray
 * smallpw
    Origin: James Murray
    Majority: James Murray
 * setup_staging
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * calc_duty
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * staging_on
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * calc_staged_pw
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * run_EAE_calcs
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * run_xtau_calcs
    Origin: Al Grippo
    Majority: Al Grippo
 * wheel_fill_inj_event_array
    Origin: Kenneth Culver
    Moderate: James Murray
    Majority: Kenneth Culver / James Murray
 * inj_event_array_portpin
    Origin: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * calc_reqfuel
    Origin: Kenneth Culver
    Minor: James Murray
    Majority: Kenneth Culver
 * crank_calcs
    Origin: Al Grippo
    Moderate: James Murray / Kenneth Culver
    Majority: Al Grippo / Kenneth Culver / James Murray
 * warmup_calcs
    Origin: Al Grippo
    Moderate: James Murray / Kenneth Culver
    Majority: Al Grippo / Kenneth Culver / James Murray
 * normal_accel
    Origin: Al Grippo
    Minor: AE %ages. James Murray
    Majority: Al Grippo
 * new_accel_calc_percent
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * new_accel
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * main_fuel_calcs
    Origin: Al Grippo
    Moderate: James Murray / Kenneth Culver
    Majority: Al Grippo / Kenneth Culver / James Murray
 * flex_fuel_calcs
    Origin: Al Grippo
    Moderate: James Murray
    Majority: Al Grippo / James Murray
 * do_overrun
    Origin: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * n2o_launch_additional_fuel
    Origin: James Murray
    Majority: James Murray
 * injpwms
    Trace: Al Grippo
    Majority: James Murray
 * do_final_fuelcalcs
    Origin: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * do_seqpw_calcs
    Origin: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * do_sequential_fuel
    Origin: Kenneth Culver
    Minor: James Murray
    Majority: Kenneth Culver
 * calc_fuel_trims
    Origin: James Murray
    Majority: James Murray
 * calc_divider
    Origin: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * calc_fuel_factor
    Origin: James Murray
    Majority: James Murray
 * lookup_fuel_factor
    Origin: James Murray
    Majority: James Murray
 * hpte
    Origin: James Murray
    Majority: James Murray
 * set_ase
    Origin: Al Grippo (as set_prime_ASE)
    Major: James Murray / Kenneth Culver
    Majority: Al Grippo / James Murray / Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/

#include "ms3.h"

/**************************************************************************
 **
 ** Calculation of Injector opening time.
 **
 ** Uses a base time setting * correction curve from battery voltage
 **
 **
 **************************************************************************/
signed int calc_opentime(unsigned char inj_no)
{
    int tmp1, tmp2, curve_no;

    // pick curve and lookup base opentime
    if (pin_dualfuel && (ram5.dualfuel_sw2 & 0x04) && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)) {
        if ((inj_no < 8) && ((ram5.opentime2_opt[0] & 0x80) == 0)) {
            // shared settings from MS3X inj A
            inj_no = 0;
        }
        curve_no = ram5.opentime2_opt[(int)inj_no]& 0x03;
        tmp2 = ram5.inj2Open[inj_no];
    } else {
        if ((inj_no < 8) && ((ram4.opentime_opt[0] & 0x80) == 0)) {
            // shared settings from MS3X inj A
            inj_no = 0;
        }
        curve_no = ram4.opentime_opt[(int)inj_no]& 0x03;
        tmp2 = ram5.injOpen[inj_no];
    }
    // get correction %age
    tmp1 = intrp_1ditable(outpc.batt, 6, (int *) ram_window.pg10.opentimev, 0,
      (int *) &ram_window.pg10.opentime[curve_no][0], 10);
    if (tmp1 < 0) {
        tmp1 = 0;
    } else if (tmp1 > 5000) {
        tmp1 = 5000;
    }
    // scale
    __asm__ __volatile__ (
    "emuls\n"
    "ldx #1000\n"
    "edivs\n"
    :"=y"(tmp1)
    :"d"(tmp1), "y"(tmp2)
    :"x"
    );
    return tmp1;
}

/**************************************************************************
 **
 ** Calculation of non-linear small pulsewidth
 **
 **************************************************************************/
unsigned int smallpw(unsigned int pw_in, unsigned char inj_no)
{
    int curve_no;
    RPAGE = tables[10].rpg;     // load page into ram window, to check upper value

    // pick curve and lookup base opentime
    if (pin_dualfuel && (ram5.dualfuel_sw2 & 0x08) && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)) {
        curve_no = ram5.smallpw2_opt[(int) inj_no] & 0x03;
    } else {
        curve_no = ram4.smallpw_opt[(int) inj_no] & 0x03;
    }
    if ((ram4.smallpw_opt[0] & 0x80)
        && (pw_in < ram_window.pg10.smallpwpw[5])) {
        return intrp_1ditable(pw_in, 6, (int *) ram_window.pg10.smallpwpw,
                          0,(int *) &ram_window.pg10.smallpw[curve_no][0], 10);
    } else {
        return pw_in;
    }
}

/* staging parameters from user are size of primaries and size of secondaries.
 * we will calculate a percentage (100% means same-size injectors) and store
 * in RAM for the staged pw calc
 */
void setup_staging(void)
{
    if (ram4.staged_extended_opts & STAGED_EXTENDED_STAGEPW1OFF) {
        staging_percent = ((unsigned long) ram4.staged_pri_size * 100) /
                          ram4.staged_sec_size;
    } else {
        staging_percent = ((unsigned long) ram4.staged_pri_size * 100) /
                           (ram4.staged_pri_size + ram4.staged_sec_size);
    }
    flagbyte4 &= ~FLAGBYTE4_STAGING_ON;

    if ((glob_sequential & SEQ_SEMI) && (ram4.hardware & HARDWARE_MS3XFUEL)
        && (num_cyl > 4) && !(ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3)
        && (cycle_deg == 7200)) { // not enough events to do this in 1.0
            conf_err = 109;
    }
    if (glob_sequential & SEQ_FULL) {
        squirts_per_revx2 = 1; /* four-stroke */
    } else if ((glob_sequential & SEQ_SEMI) && ((ram4.EngStroke & 1) == 0)) {
            squirts_per_revx2 = 2; /* semi-seq four stroke */
    } else {
        squirts_per_revx2 = num_cyl / divider;
    }
}

unsigned char calc_duty(unsigned long base_pw)
{
    unsigned long time_per_rev;
    unsigned long dtpred_time;
    unsigned int duty;

    /* add this since it's really part of the duty...
     * Divide by 100 because we were doing our calcs in
     * usec * 100 resolution before.
     */
    base_pw = (base_pw / 100) + calc_opentime(8);
    base_pw *= squirts_per_revx2;
    base_pw >>= 1;

    if ((ram4.Alternate & 1) /* Alternating */
        && ((glob_sequential & SEQ_SEMI) == 0)
        && ((glob_sequential & SEQ_FULL) == 0)) {
        base_pw >>= 1; /* non-seq only */
    }

    /* how long is 1 rev right now */
    DISABLE_INTERRUPTS;
    dtpred_time = dtpred;
    ENABLE_INTERRUPTS;

    time_per_rev = (num_cyl * dtpred_time) >> 1;

    /* rolled over, but we don't want to stage b/c of that */
    if (time_per_rev < dtpred_time) {
        return 0;
    }

    duty = (unsigned int) ((base_pw * 100) / time_per_rev);

    if (duty > 255) {
        duty = 255;
    }
    return (unsigned char) duty;
}

/* determine if staging should be on or not */
unsigned char staging_on(unsigned long base_pw)
{
    unsigned char param;
    static unsigned char staged_first_param_on = 0,
                         staged_second_param_on = 0;
    int param_to_check;

    param = ram4.staged & 0x7;
    /* First parameter */

    if (param == 0x1) {
        param_to_check = (int)outpc.rpm;
    } else if (param == 0x2) {
        param_to_check = outpc.map / 10;
    } else if (param == 0x3) {
        param_to_check = outpc.tps / 10;
    } else {
        /* calc duty and check against setting */
        param_to_check = calc_duty(base_pw);
    }

    if (param_to_check >= ram4.staged_param_1) {
        staged_first_param_on = 1;
    } else if (param_to_check < (ram4.staged_param_1 - ram4.staged_hyst_1)) {
        staged_first_param_on = 0;
    }

    param = ram4.staged & 0x38;

    /* only one param, so return the result */
    if (!param) {
        return (staged_first_param_on);
    }

    if (param == 0x8) {
        param_to_check = (int)outpc.rpm;
    } else if (param == 0x10) {
        param_to_check = outpc.map / 10;
    } else if (param == 0x18) {
        param_to_check = outpc.tps / 10;
     } else {
        param_to_check = calc_duty(base_pw);
    }

    if (param_to_check >= ram4.staged_param_2) {
        staged_second_param_on = 1;
    } else if (param_to_check < (ram4.staged_param_2 - ram4.staged_hyst_2)) {
        staged_second_param_on = 0;
    }

    if (ram4.staged & 0x80) {
        return (staged_first_param_on && staged_second_param_on);
    } else {
        return (staged_first_param_on || staged_second_param_on);
    }

    return 0;
}

/* now we calculate the staged pulse-width based on the staging percent, and
 * conditions for staging
 */
void calc_staged_pw(unsigned long base_pw)
{
    long calculated_pw1, calculated_pw2, calculated_base_pw;
    long per_ignevent_amt_pw1, per_ignevent_amt_pw2, calculated_secondary_enriched; 
    unsigned int staged_num_events_pri;
    unsigned int staged_transition_percent;

    /* percent in 1% units */
    calculated_base_pw = (base_pw * (unsigned long) staging_percent) / 100;


    if ((ram4.staged & 0x7) == 5) {
        /* This mode is a bit more complicated... we look up a value in an loadxRPM table,
         * and use the percent there to figure out what percentage through the staging process
         * the user wants to be at... 0% is no staging, and 100% is fully staged
         */
        staged_transition_percent = intrp_2dctable(outpc.rpm, outpc.fuelload, 8, 8,
                                                   ram_window.pg11.staged_rpms,
                                                   ram_window.pg11.staged_loads,
                                                   (unsigned char *) ram_window.
                                                   pg11.staged_percents, 1, 11);

        /* Hard code 0% and 100% to avoid doing the long math when we can */
        if (staged_transition_percent == 0) {
            pw_staged1 = base_pw;
            pw_staged2 = 0;
            flagbyte4 &= ~FLAGBYTE4_STAGING_ON;
        } else if (staged_transition_percent == 1000) {
            flagbyte4 |= FLAGBYTE4_STAGING_ON;
            if (ram4.staged_extended_opts & STAGED_EXTENDED_STAGEPW1OFF) {
                pw_staged1 = 0;
                pw_staged2 = calculated_base_pw;
            } else {
                pw_staged1 = pw_staged2 = calculated_base_pw;
            }
        } else {
            flagbyte4 |= FLAGBYTE4_STAGING_ON;
            /* pw1 is (base pw - (starting pw - the ending pw) * the commanded percent)
             * so that pw1 is scaled down by the same amt that pw2 is scaled up by given the
             * size difference between the injectors
             */
            if (ram4.staged_extended_opts & STAGED_EXTENDED_STAGEPW1OFF) {
                calculated_pw1 = base_pw - (((long) staged_transition_percent * base_pw) /
                                                    1000L);
            } else {
                calculated_pw1 = base_pw - (((long) staged_transition_percent *
                                 (base_pw - calculated_base_pw)) / 1000L);
            }

            if (calculated_pw1 < 0) {
                calculated_pw1 = 0;
            }

            /* pw2 is (commanded percent * base pw) */

            calculated_pw2 = (calculated_base_pw * staged_transition_percent) / 1000L;

            pw_staged1 = calculated_pw1;
            pw_staged2 = calculated_pw2;
        }
    } else if (staging_on(base_pw)) { // Doing staging
        /* This is coded for secondary flaps on GM system.
           They need to be opened a little before secondaries relay is turned on. */
        flagbyte4 |= FLAGBYTE4_STAGING_ON;
        if (pin_staged_out2) {
            if (!(flagbyte17 & FLAGBYTE17_DONEFLAPS)) {
                unsigned int t;
                t = (unsigned int)lmms - staged_flaptimer;
                t /= 781;
                if (t > ram5.staged_out2_time) {
                    flagbyte17 &= ~FLAGBYTE17_DONEFLAPS;
                } else {
                    if (ram5.staged_out2 & 0x80) { //inv
                        SSEM0SEI;
                        *port_staged_out2 &= ~pin_staged_out2;
                        CSEM0CLI;
                    } else {
                        SSEM0SEI;
                        *port_staged_out2 |= pin_staged_out2;
                        CSEM0CLI;
                    }
                    goto STAGED_FLAPPING;
                }
            }
        }

        if (pin_staged_out1) { // turn on if enabled
            if (ram5.staged_out1 & 0x80) { //inv
                SSEM0SEI;
                *port_staged_out1 &= ~pin_staged_out1;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                *port_staged_out1 |= pin_staged_out1;
                CSEM0CLI;
            }
        }

        if ((ram4.staged & 0x40) && (!(flagbyte4 & flagbyte4_transition_done))) {
            unsigned char primary_delay;
            unsigned int secondary_enrichment;

            if (ram5.staged_out1 & 0x3f) { // relay output cannot have different PW1 and PW2.
                primary_delay = 0;
                secondary_enrichment = 0;
            } else {
                primary_delay = ram4.staged_primary_delay;
                secondary_enrichment = ram4.staged_secondary_enrichment;
            }

            /* did staging just come on? if so, pw_staged2 will be 0... use that to reset
             * the transition count if necessary */
            if (pw_staged2 == 0) {
                DISABLE_INTERRUPTS;
                staged_num_events = 1;
                ENABLE_INTERRUPTS;
            }

            calculated_secondary_enriched = (calculated_base_pw + secondary_enrichment);

            if (ram4.staged_extended_opts & STAGED_EXTENDED_STAGEPW1OFF) {
                per_ignevent_amt_pw1 = base_pw / ram4.staged_transition_events;
            } else {
                per_ignevent_amt_pw1 = (base_pw - calculated_base_pw) / ram4.staged_transition_events;
            }
            per_ignevent_amt_pw2 = calculated_secondary_enriched / ram4.staged_transition_events;

            staged_num_events_pri = staged_num_events - (unsigned int)primary_delay;
            if (staged_num_events_pri > staged_num_events) {
                /* rolled over */
                staged_num_events_pri = 0;
            }

            calculated_pw2 = (staged_num_events * per_ignevent_amt_pw2);
            calculated_pw1 = base_pw -
                             (staged_num_events_pri * per_ignevent_amt_pw1);

            if (calculated_pw1 < 0) {
                calculated_pw1 = 0;
            }

            if (!(ram4.staged_extended_opts & STAGED_EXTENDED_STAGEPW1OFF)) {
                if (calculated_pw1 <= calculated_base_pw) {
                    calculated_pw1 = calculated_base_pw;
                }
            }

            if (staged_num_events_pri >= (unsigned int)ram4.staged_transition_events) {
                flagbyte4 |= flagbyte4_transition_done;
            }

            if (flagbyte4 & flagbyte4_transition_done) {
                if (ram4.staged_extended_opts & STAGED_EXTENDED_STAGEPW1OFF) {
                    calculated_pw2 = calculated_base_pw;
                    calculated_pw1 = 0;
                } else {
                    calculated_pw2 = calculated_pw1 = calculated_base_pw;
                }
            }
        } else {
            if (ram4.staged_extended_opts & STAGED_EXTENDED_STAGEPW1OFF) {
                calculated_pw2 = calculated_base_pw;
                calculated_pw1 = 0;
            } else {
                calculated_pw2 = calculated_pw1 = calculated_base_pw;
            }
        }

        pw_staged1 = calculated_pw1;
        pw_staged2 = calculated_pw2;
STAGED_FLAPPING:;        
    } else {
        /* staging should be off, just set the pulse-width to the base_pw */

        pw_staged1 = base_pw;
        pw_staged2 = 0;
        flagbyte4 &= ~FLAGBYTE4_STAGING_ON;
        flagbyte4 &= ~flagbyte4_transition_done;
        if (pin_staged_out1) { // turn off if enabled
            if (ram5.staged_out1 & 0x80) { //inv
                SSEM0SEI;
                *port_staged_out1 |= pin_staged_out1;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                *port_staged_out1 &= ~pin_staged_out1;
                CSEM0CLI;
            }
        }
        if (ram5.staged_out2 & 0x80) { //inv
            SSEM0SEI;
            *port_staged_out2 |= pin_staged_out2;
            CSEM0CLI;
        } else {
            SSEM0SEI;
            *port_staged_out2 &= ~pin_staged_out2;
            CSEM0CLI;
        }
        staged_flaptimer = (unsigned int)lmms;
    }
}

/**************************************************************************
 **
 **  EAE Transient Enrichment Section:
 **
 BF = Basic Fuel amount
 P = MAP value
 EXC = EGO correction factor
 TCC = Corrections for temperature, barometric pressure, etc...
 DFC = Desired Amount of Fuel
 WF = total amt of fuel adhering to walls
 eae_AWC = Adhere to wall coefficient: proportion of the fuel injected in next pulse which will adhere.
 AWA = actual amount that'll adhere in the next pulse
 SOC = proportion of fuel injected in next pulse which will be sucked off the walls
 SOA = actual amt of fuel injected in next pulse which will be sucked off the walls
 SQF = actual amt of fuel squirted for this pulse
 AFC = time that injector is commanded to be open
 DT = injector opening time
 BAWC = eae_AWC but only with reference to manifold pressure.
 BSOC = same as BAWC but for SOC
 AWW = correction to eae_AWC based on CLT
 SOW = correction to SOC based on CLT
 AWN = correction to eae_AWC based on engine speed
 SON = correction to SOC based on engine speed
 AWF = correction to eae_AWC based on air velocity
 SOF = correction to SOC based on air velocity

 eae_AWC = BAWC*AWW*AWN*AWF
 SOC = BSOC*SOW*SON*SOF

 SOA = SOC * WF
 SQF = (DFC - SOA)/(1-eae_AWC)
 AWA = SQF * eae_AWC

 if (fuelcut)
 WF = WF - SOA
 else
 WF = WF + AWA - SOA

 AFC = SQF + DT

 calculation of WF should be done in ISR, the rest can and will be done
 in main loop.
 **
 **************************************************************************/

void run_EAE_calcs(void)
{
    if (((ram4.EAEOption & 0x07) == 1) && (outpc.rpm != 0)) {
        unsigned long wflocal;
        unsigned long SQF, SOAtmp, AWAtmp;
        unsigned char BAWC, AWN, SOC, BSOC, SON, eae_AWC, AWW, SOW;


        BAWC =
            (unsigned char) intrp_1dctable(outpc.eaeload, NO_FMAPS,
                                           (int *) ram_window.pg8.
                                           EAEAWCKPAbins, 0,
                                           (unsigned char *)
                                           ram_window.pg8.EAEBAWC, 8);
        AWN =
            (unsigned char) intrp_1dctable(outpc.rpm, NO_FRPMS,
                                           (int *) ram_window.pg8.
                                           EAEAWCRPMbins, 0,
                                           (unsigned char *)
                                           ram_window.pg8.EAEAWN, 8);
        AWW =
            (unsigned char) intrp_1dctable(outpc.clt, 12,
                                           (int *) ram_window.pg8.
                                           EAEAWWCLTbins, 0,
                                           (unsigned char *)
                                           ram_window.pg8.EAEAWW, 8);
        /* BAWC in 1% units, so 100 = 100%, but make end result in .1% units */

        eae_AWC = ((unsigned long) BAWC * AWN * AWW) / 10000;
        if (eae_AWC >= 100)
            eae_AWC = 99;           /* avoid div by 0 below */

        BSOC =
            (unsigned char) intrp_1dctable(outpc.eaeload, NO_FMAPS,
                                           (int *) ram_window.pg8.
                                           EAESOCKPAbins, 0,
                                           (unsigned char *)
                                           ram_window.pg8.EAEBSOC, 8);
        SON =
            (unsigned char) intrp_1dctable(outpc.rpm, NO_FRPMS,
                                           (int *) ram_window.pg8.
                                           EAESOCRPMbins, 0,
                                           (unsigned char *)
                                           ram_window.pg8.EAESON, 8);
        SOW =
            (unsigned char) intrp_1dctable(outpc.clt, 12,
                                           (int *) ram_window.pg8.
                                           EAESOWCLTbins, 0,
                                           (unsigned char *)
                                           ram_window.pg8.EAESOW, 8);

        /* units here are .1% */
        SOC = ((unsigned long) BSOC * SON * SOW) / 10000;

        /* table lookups done, do calcs */

        /* calculate actual amt "sucked off" the walls */

        DISABLE_INTERRUPTS;
        wflocal = WF1;
        ENABLE_INTERRUPTS;

        SOAtmp = (SOC * wflocal) / 1000;

        DISABLE_INTERRUPTS;
        SOA1 = SOAtmp;
        ENABLE_INTERRUPTS;

        if (!(outpc.engine & ENGINE_CRANK)) {
            tmp_pw1 *= EAE_multiplier;
        }

        if (SOAtmp > tmp_pw1)
            SOAtmp = tmp_pw1;

        /* Calc actual amt of fuel to be injected in next pulse */
        SQF =
            ((unsigned long) ((unsigned long) (tmp_pw1 - SOAtmp) * 100)) /
            (100 - eae_AWC);

        if (SQF > 3200000) {
            SQF = 3200000; // limit to half PW range
        }

        /* do % calc */

        outpc.EAEfcor1 = ((unsigned long) SQF * 100) / tmp_pw1;

        AWAtmp = ((unsigned long) SQF * eae_AWC) / 100;

        DISABLE_INTERRUPTS;
        AWA1 = AWAtmp;
        ENABLE_INTERRUPTS;
        outpc.wallfuel1 = wflocal;

        if (!(outpc.engine & ENGINE_CRANK)) {
            tmp_pw1 = (SQF / EAE_multiplier);
        }

        /* do this optionally to speed up the code when we don't need it */

        if (do_dualouts) {
            BAWC =
                (unsigned char) intrp_1dctable(outpc.eaeload, NO_FMAPS,
                        (int *) ram_window.pg19.
                        EAEAWCKPAbins2, 0,
                        (unsigned char *)
                        ram_window.pg19.EAEBAWC2, 8);
            AWN =
                (unsigned char) intrp_1dctable(outpc.rpm, NO_FRPMS,
                        (int *) ram_window.pg19.
                        EAEAWCRPMbins2, 0,
                        (unsigned char *)
                        ram_window.pg19.EAEAWN2, 8);
            AWW =
                (unsigned char) intrp_1dctable(outpc.clt, 12,
                        (int *) ram_window.pg19.
                        EAEAWWCLTbins2, 0,
                        (unsigned char *)
                        ram_window.pg19.EAEAWW2, 8);
            /* BAWC in 1% units, so 100 = 100%, but make end result in .1% units */

            eae_AWC = ((unsigned long) BAWC * AWN * AWW) / 10000;
            if (eae_AWC >= 100)
                eae_AWC = 99;           /* avoid div by 0 below */

            BSOC =
                (unsigned char) intrp_1dctable(outpc.eaeload, NO_FMAPS,
                        (int *) ram_window.pg19.
                        EAESOCKPAbins2, 0,
                        (unsigned char *)
                        ram_window.pg19.EAEBSOC2, 8);
            SON =
                (unsigned char) intrp_1dctable(outpc.rpm, NO_FRPMS,
                        (int *) ram_window.pg19.
                        EAESOCRPMbins2, 0,
                        (unsigned char *)
                        ram_window.pg19.EAESON2, 8);
            SOW =
                (unsigned char) intrp_1dctable(outpc.clt, 12,
                        (int *) ram_window.pg19.
                        EAESOWCLTbins2, 0,
                        (unsigned char *)
                        ram_window.pg19.EAESOW2, 8);
            DISABLE_INTERRUPTS;
            wflocal = WF2;
            ENABLE_INTERRUPTS;

            SOAtmp = (SOC * wflocal) / 1000;

            DISABLE_INTERRUPTS;
            SOA2 = SOAtmp;
            ENABLE_INTERRUPTS;

            if (SOAtmp > tmp_pw2)
                SOAtmp = tmp_pw2;

            /* Calc actual amt of fuel to be injected in next pulse */
            SQF =
                ((unsigned long) ((unsigned long) (tmp_pw2 - SOAtmp) *
                                  100)) / (100 - eae_AWC);

            if (SQF > 3200000) {
                SQF = 3200000; // limit to half PW range
            }

            /* do % calc */

            outpc.EAEfcor2 = ((unsigned long) SQF * 100) / tmp_pw2;

            AWAtmp = ((unsigned long) SQF * eae_AWC) / 100;

            DISABLE_INTERRUPTS;
            AWA2 = AWAtmp;
            ENABLE_INTERRUPTS;
            outpc.wallfuel2 = wflocal;

            if (!(outpc.engine & ENGINE_CRANK)) {
                tmp_pw2 = SQF;
            }
        } else {
            DISABLE_INTERRUPTS;
            AWA2 = AWAtmp;
            SOA2 = SOAtmp;
            outpc.EAEfcor2 = outpc.EAEfcor1;
            ENABLE_INTERRUPTS;
            tmp_pw2 = tmp_pw1;
        }

        /* WF calc in interrupt */
    }
}

/**************************************************************************
**
**  X,Tau Transient Enrichment Section:
**    
**     fi = [ mi  -  (Mi / (tau / dltau)) ] / (1 - X)
**
**     Mi+1 = Mi + X * fi - (Mi / (tau / dltau))
**
**   where,
**          fi = total fuel injected
**
**          mi = total fuel going directly into combustion chamber (want this 
**                to be the calculated fuel)
**          
**          Mi = net fuel entering/leaving port wall puddling
**          
**          dltau = time (msx10) between squirts
**
**          X = fraction of fuel injected which goes into port wall puddling
**          tau = puddle fuel dissipation time constant (ms) as function 
**                 of rpm and coolant temp.
**                 
**         XTfcor = % correction to calculated fuel to ensure the calculated
**                 amount gets into the comb chamber.  
**
**************************************************************************/
void run_xtau_calcs(void)
{
    int ix;
    long tmp1, tmp2, tmp3, tmp4, beta, ltmp;
    unsigned long utmp1 = 0, utmp2, ultmp, XTX, Tau, XTm, XTf;
    unsigned long ppw[2];
    char Tau0;

    if ((((ram4.EAEOption & 0x07) == 3) || ((ram4.EAEOption & 0x07) == 4))
        && (flagbyte5 & FLAGBYTE5_RUN_XTAU)) {
        ppw[0] = tmp_pw1 / 100;
        ppw[1] = tmp_pw2 / 100;
        // calculate X(%x10), tau(ms) as function of rpm, accel/decel, clt
        tmp1 = -(int) ram4.MapThreshXTD;
        tmp2 = -(int) ram4.MapThreshXTD2;

        if (outpc.mapdot > tmp1) {      // use accel tables
            XTX =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.XAccTable, 11);
            Tau =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.TauAccTable, 11);
        } else if (outpc.mapdot < tmp2) {       // use decel tables
            XTX =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.XDecTable, 11);
            Tau =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.TauDecTable, 11);
        } else {                // transition from accel -> decel
            utmp1 =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.XAccTable, 11);
            utmp2 =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.XDecTable, 11);
            tmp3 = outpc.mapdot - tmp2;
            tmp4 = tmp1 - tmp2;
            tmp3 = ((long) tmp3 * 100L) / tmp4;

            // save tmp3 = ((outpc.mapdot - tmp2)* 100) / (tmp1 - tmp2)
            tmp4 = utmp1 - utmp2;
            tmp4 = ((long) tmp3 * tmp4) / 100;
            XTX = utmp2 + tmp4;
            utmp1 =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.TauAccTable, 11);
            utmp2 =
                intrp_1ditable(outpc.rpm, NO_XTRPMS,
                               ram_window.pg11.XTrpms, 0,
                               ram_window.pg11.TauDecTable, 11);
            tmp4 = (utmp1 - utmp2);
            tmp4 = ((long) tmp3 * tmp4) / 100;
            Tau = utmp2 + tmp4;
        }

        if ((ram4.EAEOption & 0x07) == 4) {
            // use X-Tau for warmup based on clt temp
            utmp1 = (unsigned short) CW_table(outpc.clt, (int *) ram_window.pg11.XClt, ram_window.pg11.XClt_temps, 11); // %
            XTX = ((long) XTX * utmp1) / 100;
            // use X-Tau for warmup based on clt temp
            utmp1 = (unsigned short) CW_table(outpc.clt, (int *) ram_window.pg11.TauClt, ram_window.pg11.TauClt_temps, 11);     // %
            Tau = ((long) Tau * utmp1) / 100;
        }

        if (Tau < 1) {
            Tau = 1;
            Tau0 = 1;           // continue x-tau calc with tau= 1, but make no correction.
        } else {
            Tau0 = 0;
        }

        for (ix = 0; ix < 2; ix++) {
            XTm = ppw[ix];

            DISABLE_INTERRUPTS;
            ultmp = sum_dltau[ix];
            sum_dltau[ix] = 0;
            ENABLE_INTERRUPTS;

            ltmp = (Tau * 1000) / ultmp;        // tau x 1000 (us) / dltau(us)

            if (ltmp < 1) {     // port wall is totally dry
                ltmp = 0;       // These derived from setting (1-X)fi = mi
                XTM[ix] = 0;
            } else {
                ltmp = XTM[ix] / ltmp;
            }

            if (ltmp > (long)XTm) {
                ltmp = XTm;
            }

            beta = ((long) (XTm - ltmp) * 1000) / (long) (1000 - XTX);
            if (beta > 65000) {
                beta = 65000;
            }
            XTf = beta;
            utmp2 = ((long) XTX * XTf) / 1000;
            XTM[ix] = XTM[ix] + utmp2 - ltmp;
            if ((XTm > 0) && !Tau0) {
                tmp1 = ((long) XTf * 100L) / XTm;
            } else {
                tmp1 = 100;
            }
            if (ix == 0) {
                outpc.EAEfcor1 = tmp1;
            } else {
                outpc.EAEfcor2 = tmp1;
            }
            if (!Tau0) {
                ppw[ix] = XTf;
            }
//            if (ppw[ix] > 32000) {
//                ppw[ix] = 32000;        // DON'T rail at 32 ms
//            }
        }
        tmp_pw1 = ppw[0] * 100;
        tmp_pw2 = ppw[1] * 100;
    }
}

void wheel_fill_inj_event_array(inj_event * inj_events_fill, int inj_ang,
                                ign_time last_tooth_time,
                                unsigned int last_tooth_ang,
                                unsigned char num_events,
                                unsigned char num_outs,
                                unsigned char flags)
{
    char iterate, tth, wfe_err, oldout_staged = 0;
    int tth_ang, tmp_ang, i, k;
    ign_time inj_time;
    unsigned int increment, start, end;

    increment = no_triggers / num_events;

    if ((flags & INJ_FILL_STAGED) &&
       (((num_cyl > 4) && (glob_sequential & SEQ_FULL)) ||
         (ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3))) {
        oldout_staged = 1;
    }

    /* This will be bounded at init time */
    if((flags & INJ_FILL_STAGED) && !oldout_staged) {
        start = num_events;
        end = start << 1;
    } else if (oldout_staged) {
        start = 8;
        if (ram4.staged_extended_opts & STAGED_EXTENDED_SIMULT) {
            increment = no_triggers;
            end = 9;
        } else {
            increment = no_triggers>>1;
            end = 10;
        }
    } else {
        start = 0;
        end = num_events;
    }

/* new tests */
        // handle the negative angle case
        if (inj_ang < 0) {
            inj_ang += cycle_deg;
        }

        // handle the excessive angle case
        if (inj_ang > cycle_deg) {
            inj_ang -= cycle_deg;
        }
/* end new */

    for (i = start, k = 0; i < (int)end; i++, k += increment) {
        int tmp_inj_ang;

        wfe_err = 0;
        iterate = 0;
        tth_ang = trig_angs[k];
        tth = trigger_teeth[k];
        tmp_inj_ang = inj_ang;


//////////////////////
// Is this valid???
        // handle the negative angle case
        if (tth_ang > tmp_inj_ang) {
            tmp_inj_ang += cycle_deg;
        }
//////////////////////

        while (!iterate) {
            if (tth_ang > tmp_inj_ang) {
                iterate = 1;
            } else {
                //how far do we step back in deg
                tth--;
                if (tth < 1) {
                    tth = last_tooth;
                    wfe_err++;
                    if (wfe_err > 2) {
                        iterate = 2;
                    }
                }
                tth_ang += deg_per_tooth[tth - 1];
            }
        }
        if (iterate == 2) {
            DISABLE_INTERRUPTS;
            asm("nop\n");       // something screwed up, place for breakpoint
            ENABLE_INTERRUPTS;
            // can't continue as didn't find a valid tooth
            debug_str("Iterate==2 @ ms3_inj.c:");
            debug_byte2dec(__LINE__ / 100); debug_byte2dec(__LINE__ % 100);
            debug_str("\r");
            return;
        }

        tmp_ang = tth_ang - tmp_inj_ang;

        inj_time.time_32_bits = (tmp_ang * last_tooth_time.time_32_bits) /
            last_tooth_ang;

        wfe_err = 0;

        while ((wfe_err < 2) && (inj_time.time_32_bits < SPK_TIME_MIN)) {
            // too soon after tooth, need to step back
            tth--;
            if (tth < 1) {
                tth = last_tooth;
                wfe_err++;
            }
            tth_ang += deg_per_tooth[tth - 1];
            // recalc
            tmp_ang = tth_ang - tmp_inj_ang;
            inj_time.time_32_bits =
                (tmp_ang * last_tooth_time.time_32_bits) / last_tooth_ang;
        }

        if (wfe_err > 2) {
            DISABLE_INTERRUPTS;
            asm("nop\n");       // something screwed up, place for breakpoint
            ENABLE_INTERRUPTS;
            // can't continue as didn't find a valid tooth
            debug_str("Iterate==2 @ ms3_inj.c:");
            debug_byte2dec(__LINE__ / 100); debug_byte2dec(__LINE__ % 100);
            debug_str("\r");
            return;
        }

        inj_events_fill[i].tooth = tth;
        inj_events_fill[i].time = inj_time.time_32_bits/50;
        if (inj_events_fill[i].time == 0) {
            inj_events_fill[i].time = 1;
        }
    }

    if ((oldout_staged) && (ram4.staged_extended_opts & STAGED_EXTENDED_SIMULT)) {
        inj_events_fill[9].tooth = inj_events_fill[8].tooth;
        inj_events_fill[9].time = inj_events_fill[8].time;
    }
}

void inj_event_array_portpin(unsigned char flags)
{
    char oldout_staged = 0;
    unsigned char num_outs;
    int i, j = 0, k;
    unsigned int increment, start, end, pin;

    for (i = 0 ; i < NUM_TRIGS ; i++) {
        injch[i] = 0; // primary
        sister_inj[i] = 0; // (off by one) 0 means none. 1 means chan 0.
    }

    num_outs = num_inj_outs; /* gets changed */
    increment = no_triggers / num_inj_events;

    if (flags & INJ_FILL_STAGED) {
        if ((((num_cyl > 4) && (glob_sequential & SEQ_FULL)) ||
         (ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3))) {
            oldout_staged = 1;
            injch[8] = 1;
            injch[9] = 1;
        }

        if ((num_cyl > 8) && !oldout_staged) {
            conf_err = 109;
            return;
        }
    } else {
        // always set V3 injectors to defaults on first call
        OCPD |= 0xa;
        next_inj[8].port = next_inj[9].port = &PORTT;
        next_inj[8].pin = 0x2;
        next_inj[9].pin = 0x8;
    }

    if (flags & INJ_FILL_TESTMODE) {
        /* force injector to hardware mapping */
        num_outs = 1;
        oldout_staged = 0;
        start = 0;
        end = 12;
        goto IEAPP1;
    }

    /* This will be bounded at init time */
    if((flags & INJ_FILL_STAGED) && !oldout_staged) {
        start = num_inj; // was num_inj_events
        end = start << 1;
    } else if (oldout_staged) {
        start = 8;
        if (ram4.staged_extended_opts & STAGED_EXTENDED_SIMULT) {
            increment = no_triggers;
            end = 10; // was 9
        } else {
            increment = no_triggers>>1;
            end = 10;
        }
    } else {
        start = 0;
        if ((glob_sequential & SEQ_SEMI) && (num_inj_events > 8)) {
            end = num_inj_events >> 1; // V10, V12, V16 semi-seq must run 2 inj per output at the moment
        } else if ((glob_sequential & 0x03) == 0) {
            /* for non sequential all injectors are individually timed so trim works
                this means that >8 cyl must be alternating - checked elsewhere */
            end = num_inj;
        } else {
            end = num_inj_events;
        }
    }

    // semi-seq MS3X dual fuel cases
    if ((glob_sequential & SEQ_SEMI) && (ram4.hardware & HARDWARE_MS3XFUEL) && (do_dualouts)
        && (((num_inj == 4) && (num_cyl == 4)) || (num_cyl == 6) || (num_cyl == 8)) && !(ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3)) {
        if (flags & INJ_FILL_STAGED) {
            oldout_staged = 1;
            start = 1;
            end = 0; // don't do loop below
            for (i = 0; i < NUM_TRIGS; i++) {
                next_inj[i].port = &PORTA; // FIXME may change in the future
            }
            // 4 cyl uses one wire per injector like normal. 8 events in 720 deg, 4 events in 360
            // special cases for 6 and 8 cyl using MS3X for pri and sec. - wire 2 injectors per output
            // in 720 deg case, two events point to each timer used
            if ((num_inj == 4) && (num_cyl == 4)) {
                if (cycle_deg == 7200) {
                    // hardcode the output to get them right
                    next_inj[0].pin = 0x05; // pri
                    next_inj[1].pin = 0x0a;
                    next_inj[2].pin = 0x05;
                    next_inj[3].pin = 0x0a;

                    next_inj[4].pin = 0x50; // sec
                    next_inj[5].pin = 0xa0;
                    next_inj[6].pin = 0x50;
                    next_inj[7].pin = 0xa0;
                    injch[4] = 1;
                    injch[5] = 1;
                    injch[6] = 1;
                    injch[7] = 1;
                } else {
                    // hardcode the output to get them right
                    next_inj[0].pin = 0x05; // pri
                    next_inj[1].pin = 0x0a;

                    next_inj[2].pin = 0x50; // sec
                    next_inj[3].pin = 0xa0;
                    injch[2] = 1;
                    injch[3] = 1;
                }
            } else if (num_cyl == 6) {
                if (cycle_deg == 7200) {
                    inj_cnt_xref[0] = 0;
                    inj_cnt_xref[1] = 1;
                    inj_cnt_xref[2] = 2;
                    inj_cnt_xref[3] = 0;
                    inj_cnt_xref[4] = 1;
                    inj_cnt_xref[5] = 2;

                    inj_cnt_xref[6] = 3;
                    inj_cnt_xref[7] = 4;
                    inj_cnt_xref[8] = 5;
                    inj_cnt_xref[9] = 3;
                    inj_cnt_xref[10]= 4;
                    inj_cnt_xref[11]= 5;
                }
                // hardcode the output to get them right
                next_inj[0].pin = 0x01; // pri
                next_inj[1].pin = 0x02;
                next_inj[2].pin = 0x04;

                next_inj[3].pin = 0x08; // sec
                next_inj[4].pin = 0x10;
                next_inj[5].pin = 0x20;
                injch[3] = 1;
                injch[4] = 1;
                injch[5] = 1;
                num_outs = 1; // looks like seq
            } else if (num_cyl == 8) {
                if (cycle_deg == 7200) {
                    inj_cnt_xref[0] = 0;
                    inj_cnt_xref[1] = 1;
                    inj_cnt_xref[2] = 2;
                    inj_cnt_xref[3] = 3;
                    inj_cnt_xref[4] = 0;
                    inj_cnt_xref[5] = 1;
                    inj_cnt_xref[6] = 2;
                    inj_cnt_xref[7] = 3;

                    inj_cnt_xref[8] = 4; // sec
                    inj_cnt_xref[9] = 5;
                    inj_cnt_xref[10]= 6;
                    inj_cnt_xref[11]= 7;
                    inj_cnt_xref[12]= 4;
                    inj_cnt_xref[13]= 5;
                    inj_cnt_xref[14]= 6;
                    inj_cnt_xref[15]= 7;
                }
                // hardcode the output to get them right
                next_inj[0].pin = 0x01; // pri
                next_inj[1].pin = 0x02;
                next_inj[2].pin = 0x04;
                next_inj[3].pin = 0x08;

                next_inj[4].pin = 0x10; // sec
                next_inj[5].pin = 0x20;
                next_inj[6].pin = 0x40;
                next_inj[7].pin = 0x80;
                injch[4] = 1;
                injch[5] = 1;
                injch[6] = 1;
                injch[7] = 1;
                num_outs = 1; // looks like seq
            }
        }
    }

IEAPP1:;

    for (i = start, k = 0; i < (int)end; i++, k += increment) {
        if ((ram4.hardware & HARDWARE_MS3XFUEL) && (!oldout_staged)) {
            if (flags & INJ_FILL_STAGED) {
            injch[i] = 1;
            }
            pin = 1;
            if ((num_outs > 1) && (num_inj_events < 9) && (num_cyl < 9)) { // i.e. semi-seq but don't enable outputs beyond injH on MS3
                if (cycle_deg == 7200) {
                        pin <<= (num_inj_events >> 1);
                        pin |= 1;
                    if (i >=  (num_inj_events >> 1)) {
                        j = i - (num_inj_events >> 1);
                        sister_inj[i] = 1 + i - (num_inj_events >> 1);
                    } else {
                        j = i;
                        sister_inj[i] = 1 + i + (num_inj_events >> 1);
                    }
                    pin <<= j;
                } else { // 360 deg case
                        pin <<= num_inj_events;
                        pin |= 1;
                    pin <<= i;
                }
            } else { // seq
                pin <<= i;
            }

            if (i < 8) { // 0-7 only
                next_inj[i].pin = pin;
                next_inj[i].port = pPTA;
            } else if (i == 8) {
                if ((ram4.hardware & 0x20) 
                    && (!((MONVER >= 0x380) && (MONVER <= 0x3ff)))) { // only on MS3
                    next_inj[8].pin = 0x08; // PK3 / H3
                    next_inj[8].port = pPTK;
                } else { // optional or non MS3 uses PT1 for injI
                    next_inj[8].pin = 0x02;
                    next_inj[8].port = pPTT;
                }
            } else if (i == 9) {
                if ((ram4.hardware & 0x20) 
                    && (!((MONVER >= 0x380) && (MONVER <= 0x3ff)))) { // only on MS3
                    next_inj[9].pin = 0x02; // PK1 / H4
                    next_inj[9].port = pPTK;
                } else { // optional or non MS3 uses PT3 for injJ
                    next_inj[9].pin = 0x08;
                    next_inj[9].port = pPTT;
                }
            } else if (i == 10) {
                if ((MONVER >= 0x380) && (MONVER <= 0x3ff)) { // non MS3
                    next_inj[10].pin = 0x10;
                    next_inj[10].port = &PTP;
                } else {
                    next_inj[10].pin = 0x80;
                    next_inj[10].port = &PORTK;
                }
            } else if (i == 11) {
                if ((MONVER >= 0x380) && (MONVER <= 0x3ff)) { // non MS3
                    next_inj[11].pin = 0x20;
                    next_inj[11].port = &PTP;
                } else {
                    next_inj[11].pin = 0x04;
                    next_inj[11].port = pPTM;
                }
            }
        } else {
            next_inj[i].port = pPTT;
            if ((ram4.Alternate && !oldout_staged) ||
               (oldout_staged)
                || (!(ram4.hardware & HARDWARE_MS3XFUEL) && (ram4.staged & 0x7))) { // V3 only + staged
                if (i & 0x01) {
                    /* ODD, use inj2 */
                    next_inj[i].pin = 0x08;
                } else {
                    /* EVEN, use inj1 */
                    next_inj[i].pin = 0x02;
                }
            } else {
                next_inj[i].pin = 0x0a;
            }
        }
    }
}

void calc_reqfuel(unsigned int reqfuel_in)
{
    unsigned char reqfuel_multiplier;
    unsigned long utmp1;

    orig_ReqFuel = reqfuel_in;
    orig_divider = ram4.Divider;
    orig_alternate = ram4.Alternate;

    calc_divider();
    /* In here we set up some critical internal variables that are used in other parts of ms3_inj
     * and ms3_ign_in. These and others are:
     * num_inj_outs is the number of outputs per event
     * num_inj_events is the number of expected events
     * num_inj is the number of actual injectors
     * do_dualouts is set for dual fuel
     */

    if (glob_sequential & 0x3) {
        if (glob_sequential & SEQ_FULL) {
            num_inj_outs = 1;   // 1 out per event;
            num_inj_events = num_cyl;
            num_inj = num_cyl;

            reqfuel_multiplier = num_cyl / divider;

            if ((ram4.Alternate & 0x01) && (reqfuel_multiplier > 1)) {
                reqfuel_multiplier >>= 1;
            }

            ReqFuel = (unsigned long) reqfuel_in * reqfuel_multiplier;

            if (num_cyl < num_inj) {
                utmp1 = num_cyl * (unsigned long)ReqFuel;
                ReqFuel = utmp1 / num_inj;
            } else {
                utmp1 = num_inj * (unsigned long)ReqFuel;
                ReqFuel = utmp1 / num_cyl;
            }

            EAE_multiplier = 1;
        } else if ((glob_sequential & SEQ_SEMI) && (ram4.hardware & HARDWARE_MS3XFUEL)) {
            if ((ram4.EngStroke & 0x01) && (!(flagbyte6 & FLAGBYTE6_DONEINIT))) {
                conf_err = 112; // semi-seq doesn't make sense
            }
            if (cycle_deg == 7200) {
                num_inj_events = num_cyl; // need to output twice in semi-seq
            } else {
                num_inj_events = num_cyl >> 1;
            }

            if ((do_dualouts) && (num_cyl > 4) && !(ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3)) {
                if ((num_cyl == 6) || (num_cyl == 8)) {
                    // 2 injectors per output case - now supported
                    num_inj_outs = 2;
                    num_inj = num_cyl >> 1;
                } else {
                    conf_err = 109; // other combinations not supported
                }
            } else {
                num_inj_outs = 2;
                num_inj = num_cyl;
            }

            reqfuel_multiplier = num_cyl / divider;

            if ((ram4.Alternate & 0x01) && (reqfuel_multiplier > 1)) {
                reqfuel_multiplier >>= 1;
            }

            ReqFuel = (unsigned long) reqfuel_in * reqfuel_multiplier;
            ReqFuel >>= 1; // semi-seq squirts half the fuel every 360deg compared to seq every 720deg

            if (num_cyl < num_inj) {
                utmp1 = num_cyl * (unsigned long)ReqFuel;
                ReqFuel = utmp1 / num_inj;
            } else {
                utmp1 = num_inj * (unsigned long)ReqFuel;
                ReqFuel = utmp1 / num_cyl;
            }

            EAE_multiplier = 1;
        } else { /* Semi-seq on standard v3 injectors */
            OCPD |= 0xA;
            /* For this mode, just use whatever the tuning
             * software sets. It is correct.
             */
            num_inj_events = num_cyl / divider;
            if (cycle_deg == 3600) {
                num_inj_events >>= 1;
            }

            if ((num_inj_events < 2) && (!(flagbyte6 & FLAGBYTE6_DONEINIT))) {
                /* These don't make sense... need 720 degrees of data to do properly, but
                 * only have 360, or need to skip an entire half the altogether
                 */
                conf_err = 102; // only report during init phase
            }

            num_inj_outs = num_inj_events >> 1;
            ReqFuel = reqfuel_in;

            EAE_multiplier = num_inj_outs;

            if (!(ram4.Alternate & 0x1)) {
                EAE_multiplier <<= 1;
            }

            EAE_multiplier /= (num_cyl / (ram4.NoInj & 0x1f));

            if (!EAE_multiplier) {
                EAE_multiplier = 1;
            }

            num_inj = 2;
        }
    } else { /* Non seq */
        num_inj_events = num_cyl / divider;
        num_inj_outs = num_inj_events >> 1;
        ReqFuel = reqfuel_in;

        EAE_multiplier = num_inj_outs;

        if (!(ram4.Alternate & 0x1)) {
            EAE_multiplier <<= 1;
        }

        EAE_multiplier /= (num_cyl / (ram4.NoInj & 0x1f));

        if (!EAE_multiplier) {
            EAE_multiplier = 1;
        }

        if (ram4.hardware & HARDWARE_MS3XFUEL) {
            num_inj_outs = 1; /* fix for Mariob 4sq -> 8inj */
            num_inj = ram4.NoInj & 0x1f;
            if ((ram4.EngStroke & 0x01) && (!(flagbyte6 & FLAGBYTE6_DONEINIT))) {
                if (spkmode != 14) { // not twin trigger
                    conf_err = 112; // non-seq MS3X doesn't make sense with rotary or 2-stroke
                }
            }
        } else {
            num_inj = 2;
        }
    }
}

/**************************************************************************
 **
 ** Cranking Mode
 **
 ** Pulsewidth is directly set by the coolant temperature to a value of
 **  CWU (at temp_table[0]) and CWH (at temp_table[NO_TEMPS -1]).
 **  The value is interpolated at clt.
 **
 **************************************************************************/

char crank_calcs(void)
{
    unsigned int utmp1 = 0, utmp2 = 0;

    if (((outpc.rpm < ram4.crank_rpm)
         && (flagbyte2 & flagbyte2_crank_ok))
        /*|| (!(flagbyte3 & flagbyte3_toothinit)) */
        ) {
        unsigned char w = 0;

        if ((ram4.feature7 & 0x20) && (ram4.sequential & 0x03) && (!(flagbyte17 & FLAGBYTE17_BFC_CRANKSET))) {
            /* normally semi or sequential, but requested Batch Fire Crank */
            flagbyte17 |= FLAGBYTE17_BFC_CRANKSET; // say we've done it
            glob_sequential = 0; // batch
            calc_reqfuel(ram4.ReqFuel);
            inj_event_array_portpin(0);
            if (do_dualouts) {
                inj_event_array_portpin(INJ_FILL_STAGED);
            }
        }

        tcrank_done = 0xFFFF;
        DISABLE_INTERRUPTS;
        running_seconds = 0;
        ENABLE_INTERRUPTS;
        if (!(outpc.engine & ENGINE_CRANK)) {
            lmms_crank = lmms; // time of first declaring cranking
        }
        outpc.engine |= ENGINE_CRANK;   // set cranking bit
        outpc.engine &= ~(ENGINE_ASE | ENGINE_WUE);  // clr starting warmup bit & warmup bit
        if (outpc.tps > ram4.TPSWOT) {
            tmp_pw1 = 0;        // usec  (0 ms for Flood Clear)
            tmp_pw2 = 0;
            return 1;
        }
        // cranking pulse width now uses a %age lookup table calculated from ReqFuel.
        // Hopefully new users can leave this table alone and engine will start
        // Changing injector size is corrected by ReqFuel

        if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw & 0x40)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
            w = 7;
        } else if (ram4.Alternate & 0x04) {
            w = 3;
        } else if (pin_dualfuel && (ram5.dualfuel_sw2 & 0x02) && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)) {
            w = 2;
        } else {
            w = 1;
        }

        if (w & 1) {
            utmp1 = CW_table(outpc.clt, (int *) ram_window.pg8.CrankPctTable,
                         (int *) ram_window.pg8.temp_table_p5, 8);
        }
        if (w & 2) {
            utmp2 = CW_table(outpc.clt, (int *) ram_window.pg21.CrankPctTable2,
                         (int *) ram_window.pg21.temp_table_p21, 21);
        }

        if (w == 2) { // switched
            utmp1 = utmp2;
        } else if (w == 3) {  // blended
            unsigned char blend;
            blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[6] & 0x1f), 9, 
                    (int *) ram_window.pg25.blendx[6], 0, 
                    (unsigned char *)ram_window.pg25.blendy[6], 25);
            utmp1 = (unsigned int)((((unsigned long)utmp1 * (100 - blend)) + ((unsigned long)utmp2 * blend)) / 100);
        } else if (w == 7) { /* flex blended */
            utmp1 = (unsigned int)((((unsigned long)utmp1 * (100 - flexblend)) + ((unsigned long)utmp2 * flexblend)) / 100);
        }
        // else just drop through using crank%1

        if ((glob_sequential & SEQ_SEMI) && (ram4.hardware & HARDWARE_MS3XFUEL)) {
            utmp1 >>= 1;
        } else if ((glob_sequential & SEQ_SEMI) && (!(ram4.hardware & HARDWARE_MS3XFUEL))) {
            utmp1 /= num_inj_events;

            if (ram4.Alternate & 0x01) {
                utmp1 <<= 1;
            }
        } else if (!(glob_sequential & 0x03)) {
            if (ram4.Alternate & 0x02) {
                utmp1 <<= 1;
            }

            utmp1 /= divider;

            if (ram4.Alternate & 0x01) {
                utmp1 >>= 1;
            }
        }

        {
            /* do crank multiply/divide and check for overflow */
            unsigned long ultmp_cr;
            ultmp_cr = (unsigned long)utmp1 * ReqFuel;
            if (ultmp_cr > 6500000) {
                utmp1 = 65000;
            } else {
                utmp1 = (unsigned int)(ultmp_cr / 100);
            }
        }

        if (ram4.feature7 & 0x40) {
            /* crank taper calc */
            unsigned long lmms1, ultmp_cr;
            int p;
            if (ram4.feature7 & FEATURE7_CRTPUNITS) {
                lmms1 = cranktpcnt;
            } else {
                DISABLE_INTERRUPTS;
                lmms1 = lmms;
                ENABLE_INTERRUPTS;
                lmms1 = lmms1 - lmms_crank; // how long since we started cranking
                lmms1 /= 780; // convert to 0.1s
            }
            p = intrp_1ditable((int)lmms1, 6,
                   (int *) ram_window.pg11.cranktaper_time, 0,
                   (unsigned int *) ram_window.pg11.cranktaper_pct, 11);
            ultmp_cr = (unsigned long)utmp1 * p;
            if (ultmp_cr > 65000000) {
                utmp1 = 65000;
            } else {
                utmp1 = (unsigned int)(ultmp_cr / 1000);
            }
        }

        if ((ram5.dualfuel_sw & 0x1) && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_SWITCHING)
            && (ram5.dualfuel_opt & DUALFUEL_OPT_OUT) && pin_dualfuel) {
            // Dual Fuel on, Switching, different outputs
            if ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel) {
                // if active, then kill PW1
                tmp_pw1 = 0;
                tmp_pw2 = (unsigned long) utmp1 *100;
            } else {
                // else kill PW2
                tmp_pw1 = (unsigned long) utmp1 *100;
                tmp_pw2 = 0;
            }
        } else { // normal mode
            tmp_pw1 = (unsigned long) utmp1 *100;
            if (ram4.FlexFuel & 0x01) {
                tmp_pw1 = ((unsigned long)tmp_pw1 * outpc.fuelcor) / 100;
            }
            tmp_pw2 = tmp_pw1;
        }
        return 2;

    } else if (tcrank_done == 0xFFFF) {
        if (synch & SYNC_SYNCED) {
            tcrank_done = outpc.seconds;
            DISABLE_INTERRUPTS;
            tcrank_done_lmms = lmms;
            ENABLE_INTERRUPTS;
        }

    } else if (outpc.seconds > (tcrank_done + 5)) {
        flagbyte2 &= ~flagbyte2_crank_ok;

        if ((ram4.feature7 & 0x20) && (ram4.sequential & 0x03) && (!(flagbyte17 & FLAGBYTE17_BFC_RUNSET))) {
            /* normally semi or sequential, but requested Batch Fire Crank */
            flagbyte17 |= FLAGBYTE17_BFC_RUNSET; // say we've done it
            /* Select semi if set as seq, but only half-synced and four stroke */
            if (((ram4.EngStroke & 0x01) == 0) && ((outpc.status1 & STATUS1_SYNCFULL) == 0)
                && ((ram4.sequential & 0x3) == SEQ_FULL)) {
                glob_sequential = 1; // revert to semi instead
            } else {
                glob_sequential = ram4.sequential; // normal
            }
            calc_reqfuel(ram4.ReqFuel);
            inj_event_array_portpin(0);
            if (do_dualouts) {
                inj_event_array_portpin(INJ_FILL_STAGED);
            }
        }

    } else if (outpc.seconds > (tcrank_done + 2)) {
        if (((spkmode == 2) && ((ram4.spk_conf2 & 0x07) == 0x01)) // basic trigger + bypass
            || ((spkmode == 4) && ((ram4.spk_conf2 & 0x07) == 0x02))) { // toothed wheel + C3I
            // Set HEI bypass to 5v - requires that SpkA and SpkB are same polarity
            if (flagbyte10 & FLAGBYTE10_SPKHILO) { /* Going high */
                if (ram4.hardware & 0x02) {
                    SSEM0SEI;
                    PORTB |= 0x02;
                    CSEM0CLI;
                } else {
                    SSEM0SEI;
                    PTM |= 0x10;
                    CSEM0CLI;
                }
            } else { /* going low */
                if (ram4.hardware & 0x02) {
                    SSEM0SEI;
                    PORTB &= ~0x02;
                    CSEM0CLI;
                } else {
                    SSEM0SEI;
                    PTM &= ~0x10;
                    CSEM0CLI;
                }
            }
        }
    }

    return 0;
}

/**************************************************************************
 **
 ** Warm-up and After-start Enrichment Section
 **
 ** The Warm-up enrichment is a linear interpolated value for the current clt
 ** temperature from warmen_table[NO_TEMPS] vs temp_table[NO_TEMPS]. The
 ** interpolation is done in subroutine warmcor_table.
 **
 ** Also, the after-start enrichment value is calculated and applied here - it
 **  is an added percent value on top of the warmup enrichment, and it is applied
 **  for the number of ignition cycles specified in ase_cycles. This enrichment starts
 **  at a value of ase_value at first, then it linearly interpolates down to zero
 **  after ase_cycles cycles.
 **
 **  If (startw, engine is set) then:
 **   compare if (ase_cycles > 0) then:
 **    interpolate for warmup enrichment
 **   else clear startw bit in engine
 **
 **************************************************************************/

void warmup_calcs(void)
{
    int wrmtmp, wrmtmp1, wrmtmp2;
    unsigned int utmp1;
    unsigned char w;
    
    wrmtmp1 = 0;
    wrmtmp2 = 0;

    if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw & 0x40)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
        w = 3;
    } else if (pin_dualfuel && (ram5.dualfuel_sw & 0x40) && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)) {
        w = 2;
    } else {
        w = 1;
    }

    wrmtmp = outpc.warmcor;
    if (outpc.engine & ENGINE_CRANK) {  // if engine cranking
        outpc.engine &= ~ENGINE_CRANK;  // clear crank bit
        outpc.engine |= ENGINE_ASE | ENGINE_WUE;       // set starting warmup bit (ase) & warmup bit
//            asecount = 0;

        set_ase();
        DISABLE_INTERRUPTS;
        running_seconds = 0;
        ENABLE_INTERRUPTS;
    }
    if (w & 1) {
        wrmtmp1 = intrp_1dctable(outpc.clt, NO_TEMPS, (int *) ram_window.pg10.temp_table,
                                0, (unsigned char *) ram_window.pg10.warmen_table, 8);     // %
    }
    if (w & 2) {
        wrmtmp2 = intrp_1dctable(outpc.clt, NO_TEMPS, (int *) ram_window.pg21.temp_table_p21,
                                0, (unsigned char *) ram_window.pg21.warmen_table2, 21);     // %
    }

    if (w == 1) {
        wrmtmp = wrmtmp1;
    } else if (w == 2) {
        wrmtmp = wrmtmp2;
    } else {
        wrmtmp = (unsigned int)((((unsigned long)wrmtmp1 * (100 - flexblend)) + ((unsigned long)wrmtmp2 * flexblend)) / 100);
    }
    outpc.cold_adv_deg = CW_table(outpc.clt, (int *) ram_window.pg10.cold_adv_table,
                                 (int *) ram_window.pg10.temp_table, 10);   // deg x 10

    if (wrmtmp == 100) {        // done warmup
        outpc.engine &= ~ENGINE_WUE;  // clear start warmup bit (ase) & warmup bit
        SSEM0SEI;
        *pPTMpin5 &= ~0x20;   // clear warmup led
        CSEM0CLI;
    } else {
        SSEM0SEI;
        *pPTMpin5 |= 0x20;    // set warmup led
        CSEM0CLI;
        outpc.engine |= ENGINE_WUE;   // set warmup bit
    }

    if (!(outpc.engine & ENGINE_ASE)) {       // if starting warmup bit clear
        goto END_WRM;
    }

    if ((asecount > ase_cycles) || (ase_value == 0)) {
        outpc.engine &= ~ENGINE_ASE;  // clear start warmup bit
        goto END_WRM;
    }

    if (ase_cycles > 0) {
//        utmp1 = (unsigned long)(ase_value * (ase_cycles - asecount)) / ase_cycles;
        __asm__ __volatile__ (
        "tfr d,x\n"
        "subd %1\n"
        "ldy  %2\n"
        "emul\n"
        "ediv\n"
        : "=y" (utmp1)
        : "m" (asecount), "m" (ase_value), "d" (ase_cycles)
        : "x"
        );

        wrmtmp += utmp1;
    }
  END_WRM:
    outpc.warmcor = wrmtmp;
}

/**************************************************************************
 **
 **  Throttle Position Acceleration Enrichment
 **
 **   Method is the following:
 **
 **
 **   ACCELERATION ENRICHMENT:
 **   If (tpsdot < 0) goto DEACCELERATION_ENRICHMENT
 **   If tpsdot > tpsthresh and TPSAEN bit = 0 then (acceleration enrichment):
 **   {
 **    1) Set acceleration mode
 **    2) Continuously determine rate-of-change of throttle, and perform
 **        interpolation of table values to determine acceleration
 **        enrichment amount to apply.
 **   }
 **   If (TPSACLK > TpsAsync) and TPSAEN is set then:
 **   {
 **    1) Clear TPSAEN bit in engine
 **    2) Set TPSACCEL to 0
 **    3) Go to EGO Delta Step Check Section
 **   }
 **   Enrichment tail-off pulsewidth:
 **
 **   ------------------ tpsaccel
 **   |            |\
 **   |            |   \
 **   |            |      \
 **   |            |         \          ____ TpsAccel2
 **   |            |            |
 **   |            |            |
 **   ---------------------------
 **   <--TpsAsync--><-TpsAsync2->
 **
 **
 **   DEACCELERATION ENRICHMENT:
 **   If (-tpsdot) > tpsthresh then (deceleration fuel cut)
 **   {
 **    If (TPSAEN = 1) then:
 **    {
 **      1) TPSACCEL = 0 (no acceleration)
 **      2) Clear TPSAEN bit in ENGINE
 **      3) Go to EGO Delta Step
 **    }
 **    If (RPM > 1500 then (fuel cut mode):
 **    {
 **      1) Set TPSACCEL value to TPSDQ
 **      2) Set TPSDEN bit in ENGINE
 **      3) Go to EGO Delta Step Check Section
 **    }
 **   }
 **   else
 **   {
 **    If (TPSDEN = 1) then
 **    {
 **     1) Clear TPSDEN bit in ENGINE
 **     2) TPSACCEL = 0
 **     3) Go to EGO Delta Step Check Section
 **    }
 **   }
 **
 **************************************************************************/

void normal_accel(void)
{
    int tpsatmp, tpsatmp1, tpsatmp2, ae_scaler, ae_scaler1, ae_scaler2;
    int tmp1, tmp2, tmp3, tmp4;
    int tpsdot_ltch, mapdot_ltch;

    unsigned char prop, prop1, prop2;
    int tpsthresh, tpsthresh1, tpsthresh2;
    int mapthresh, mapthresh1, mapthresh2;
    unsigned char taecolda, taecolda1, taecolda2;
    unsigned char taecoldm, taecoldm1, taecoldm2;
    unsigned char aetime, aetime1, aetime2;
    int endpw, endpw1, endpw2;
    unsigned char tapertime, tapertime1, tapertime2;
    unsigned char tdepct, tdepct1, tdepct2;
    unsigned char w;
    int tae, tae1, tae2;
    int mae, mae1, mae2;

    tpsatmp = outpc.tpsaccel;
    DISABLE_INTERRUPTS;
    tpsdot_ltch = outpc.tpsdot;
    mapdot_ltch = outpc.mapdot;
    ENABLE_INTERRUPTS;

    /* First set option */
    w = 0;
    if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw2 & 0x40)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
        w = 3;
    } else if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw2 & 0x40)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_SWITCHING)
        && pin_dualfuel && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)
        ) {
        w = 2;
    } else {
        w = 1;
    }

    prop1 = 0;
    ae_scaler1 = 0;
    tpsthresh1 = 0;
    mapthresh1 = 0;
    taecolda1 = 0;
    taecoldm1 = 0;
    aetime1 = 0;
    endpw1 = 0;
    tapertime1 = 0;
    tdepct1 = 0;
    tae1 = 0;
    mae1 = 0;

    prop2 = 0;
    ae_scaler2 = 0;
    tpsthresh2 = 0;
    mapthresh2 = 0;
    taecolda2 = 0;
    taecoldm2 = 0;
    aetime2 = 0;
    endpw2 = 0;
    tapertime2 = 0;
    tdepct2 = 0;
    tae2 = 0;
    mae2 = 0;

    /* Calculate all of the variables required either table */
    if (w & 1) {
        prop1 = ram4.Tps_acc_wght;
        RPAGE = tables[28].rpg; /* HARDCODED to match taeBins etc.*/
        tpsatmp1 = (((short) ram_window.pg28.taeBins[0] * prop) +
                ((short) ram_window.pg28.maeBins[0] *
                 (100 - prop))) / 100;
        // apply AE rpm-based scaler
        if ((outpc.rpm <= ram4.ae_lorpm) || (ram4.feature7 & 0x08)) { // if events then ignore down-scaling
            ae_scaler1 = 100;  
        } else if (outpc.rpm >= ram4.ae_hirpm) {
            ae_scaler1 = 0;
        } else {
            ae_scaler1 = (short) (((long) 100 * (ram4.ae_hirpm - outpc.rpm)) /
                    (ram4.ae_hirpm - ram4.ae_lorpm));
        }
        tpsthresh1 = ram4.TpsThresh;
        mapthresh1 = ram4.MapThresh;
        taecolda1 = ram4.Tpsacold;
        taecoldm1 = ram4.AccMult;
        aetime1 = ram4.TpsAsync;
        endpw1 = ram4.TpsAccel2;
        tapertime1 = ram4.TpsAsync2;
        tdepct1 = ram4.TPSDQ;
        tae1 = (unsigned char) (((unsigned long) intrp_1dctable(tpsdot_ltch, NO_TPS_DOTS, (int *) ram_window.pg28.taeRates, 0, (unsigned char *) ram_window.pg28.taeBins, 28) * ReqFuel) / (unsigned long) 10000);
        mae1 = (unsigned char) (((unsigned long) intrp_1dctable(mapdot_ltch, NO_MAP_DOTS, (int *) ram_window.pg28.maeRates, 0, (unsigned char *) ram_window.pg28.maeBins, 28) * ReqFuel) / (unsigned long) 10000);
    }

    if (w & 2) {
        prop2 = ram5.tpsProportion2;
        RPAGE = tables[28].rpg; /* HARDCODED to match taeBins etc.*/
        tpsatmp2 = (((short) ram_window.pg28.taeBins2[0] * prop2) +
                ((short) ram_window.pg28.maeBins2[0] *
                 (100 - prop2))) / 100;
        // apply AE rpm-based scaler
        if ((outpc.rpm <= ram5.ae_lorpm2) || (ram4.feature7 & 0x08)) { // if events then ignore down-scaling
            ae_scaler2 = 100;  
        } else if (outpc.rpm >= ram5.ae_hirpm2) {
            ae_scaler2 = 0;
        } else {
            ae_scaler2 = (short) (((long) 100 * (ram5.ae_hirpm2 - outpc.rpm)) /
                    (ram5.ae_hirpm2 - ram5.ae_lorpm2));
        }
        tpsthresh2 = ram5.tpsThresh2;
        mapthresh2 = ram5.mapThresh2;
        taecolda2 = ram5.taeColdA2;
        taecoldm2 = ram5.taeColdM2;
        aetime2 = ram5.taeTime2;
        endpw2 = ram5.aeEndPW2;
        tapertime2 = ram5.aeTaperTime2;
        tdepct2 = ram5.tdePct2;
        tae2 = (unsigned char) (((unsigned long) intrp_1dctable(tpsdot_ltch, NO_TPS_DOTS, (int *) ram_window.pg28.taeRates2, 0, (unsigned char *) ram_window.pg28.taeBins2, 28) * ReqFuel) / (unsigned long) 10000);
        mae2 = (unsigned char) (((unsigned long) intrp_1dctable(mapdot_ltch, NO_MAP_DOTS, (int *) ram_window.pg28.maeRates2, 0, (unsigned char *) ram_window.pg28.maeBins2, 28) * ReqFuel) / (unsigned long) 10000);
    }

    /* Switch or blend to one */
    if (w == 3) {
        unsigned int t;
        t = TCNT;
        /* blend */
        prop = ((((long)prop1 * (100 - (int)flexblend)) + ((long)prop2 * (int)flexblend)) / 100L);
        ae_scaler = ((((long)ae_scaler1 * (100 - (int)flexblend)) + ((long)ae_scaler2 * (int)flexblend)) / 100L);
        tpsthresh = ((((long)tpsthresh1 * (100 - (int)flexblend)) + ((long)tpsthresh2 * (int)flexblend)) / 100L);
        mapthresh = ((((long)mapthresh1 * (100 - (int)flexblend)) + ((long)mapthresh2 * (int)flexblend)) / 100L);
        taecolda = ((((long)taecolda1 * (100 - (int)flexblend)) + ((long)taecolda2 * (int)flexblend)) / 100L);
        taecoldm = ((((long)taecoldm1 * (100 - (int)flexblend)) + ((long)taecoldm2 * (int)flexblend)) / 100L);
        aetime = ((((long)aetime1 * (100 - (int)flexblend)) + ((long)aetime2 * (int)flexblend)) / 100L);
        endpw = ((((long)endpw1 * (100 - (int)flexblend)) + ((long)endpw2 * (int)flexblend)) / 100L);
        tapertime = ((((long)tapertime1 * (100 - (int)flexblend)) + ((long)tapertime2 * (int)flexblend)) / 100L);
        tdepct = ((((long)tdepct1 * (100 - (int)flexblend)) + ((long)tdepct2 * (int)flexblend)) / 100L);
        tae = ((((long)tae1 * (100 - (int)flexblend)) + ((long)tae2 * (int)flexblend)) / 100L);
        mae = ((((long)mae1 * (100 - (int)flexblend)) + ((long)mae2 * (int)flexblend)) / 100L);
        outpc.istatus5 = TCNT - t;
    } else if (w == 2) {
        prop = prop2;
        ae_scaler = ae_scaler2;
        tpsthresh = tpsthresh2;
        mapthresh = mapthresh2;
        taecolda = taecolda2;
        taecoldm = taecoldm2;
        aetime = aetime2;
        endpw = endpw2;
        tapertime = tapertime2;
        tdepct = tdepct2;
        tae = tae2;
        mae = mae2;
    } else {
        prop = prop1;
        ae_scaler = ae_scaler1;
        tpsthresh = tpsthresh1;
        mapthresh = mapthresh1;
        taecolda = taecolda1;
        taecoldm = taecoldm1;
        aetime = aetime1;
        endpw = endpw1;
        tapertime = tapertime1;
        tdepct = tdepct1;
        tae = tae1;
        mae = mae1;
    }

    /* Then do the AE algo using those vars */

    if ((tpsdot_ltch < 0) && (prop != 0)) {
        goto TDE;               // decelerating
    }
    if ((mapdot_ltch < 0) && (prop != 100)) {
        goto TDE;
    }
    if (outpc.engine & ENGINE_TPSACC) {
        goto AE_COMP_SHOOT_AMT; // if accel enrich bit set
    }
    if (outpc.engine & ENGINE_MAPACC) {
        goto AE_COMP_SHOOT_AMT;
    }
    if (((prop == 0) || (tpsdot_ltch < tpsthresh))
            && ((prop == 100) || (mapdot_ltch < mapthresh))) {
        goto TAE_CHK_TIME;
    }
    // start out using first element - determine actual next time around

    tpsatmp = (unsigned char) (((unsigned long) tpsatmp * ReqFuel) / (unsigned long) 10000);    // .1 ms units

    tpsaccel_end = tpsatmp;     // latch last tpsaccel before tailoff
    tpsaclk = 0;                // incremented in .1 sec timer

    if ((tpsdot_ltch > tpsthresh) && (prop != 0)) {
        outpc.engine |= ENGINE_TPSACC;   // set tpsaen bit
    }
    if ((mapdot_ltch > mapthresh) && (prop != 100)) {
        outpc.engine |= ENGINE_MAPACC;
    }
    outpc.engine &= ~ENGINE_TPSDEC;      // clear tpsden bit
    outpc.engine &= ~ENGINE_MAPDEC;
    SSEM0SEI;
    *pPTMpin4 |= 0x10;        // set accel led
    CSEM0CLI;
    goto END_TPS;
    /* First, calculate Cold temperature add-on enrichment value from coolant value
       TPSACOLD at min temp & 0 at high temp.

       Then determine cold temperature multiplier value ACCELMULT (in percent).

       Next, Calculate Shoot amount (quantity) for acceleration enrichment from table.
       Find bins (between) for corresponding TPSDOT and MAPDOT, and linear interpolate
       to find enrichment amount from table. This is continuously
       checked every time thru main loop while in acceleration mode,
       and the highest value is latched and used.

       The final acceleration enrichment (in .1 ms units) applied is AE =
       (((Alookup(TPSDOT)*Tps_acc_wght + Alookup(MAPDOT)*(100 - Tps_acc_wght))
       /100) * ACCELMULT/100) + TPSACOLD.
     */

AE_COMP_SHOOT_AMT:
    if (tpsaclk < aetime) { // was <=
        // direct use of table data without calling interpolation routine
        RPAGE = tables[10].rpg; // HARDCODED
        tmp1 = (short) taecolda - (short) ((taecolda *
                    (long) (outpc.clt - ram_window.pg10.temp_table[0])
                    / (ram_window.pg10.temp_table[NO_TEMPS - 1] - ram_window.pg10.temp_table[0])));   // in .1 ms
        tmp2 = (short) taecoldm + (short) (((100 - taecoldm) *
                    (long) (outpc.clt - ram_window.pg10.temp_table[0])
                    / (ram_window.pg10.temp_table[NO_TEMPS - 1] - ram_window.pg10.temp_table[0])));    // in %

        if (prop > 0) {
            tmp3 = tae;
        } else {
            tmp3 = 0;
        }

        if (prop < 100) {
            tmp4 = mae;
        } else {
            tmp4 = 0;
        }
        tmp3 = ((tmp3 * prop) + (tmp4 * (100 - prop))) / 100;

        tmp3 = (tmp3 * tmp2) / 100;

        if (tmp3 > 200) {
            tmp3 = 200;         // rail at 200%
        }

        tmp3 += tmp1;
        if (tmp3 > tpsatmp) {
            tpsatmp = tmp3;     // make > tps/mapen_table entry for lowest tps/mapdot
        }
        // plus latch and hold largest pw
        //        tpsaccel_end = tpsatmp; // latch last tpsaccel before tailoff (original non working line)
        if (tpsatmp > (int)tpsaccel_end) {
            tpsaccel_end = tpsatmp;
        } else {
            tpsatmp = tpsaccel_end;
        }      
    } else {                    // tailoff enrichment pulsewidth
TAILOFF:
        if (tapertime > 0) {
            unsigned int tmp_endpw;
            tmp_endpw = (unsigned int) (((unsigned long) endpw * ReqFuel) / (unsigned long) 10000);
            tpsatmp = tpsaccel_end + ((((int)tmp_endpw - (int)tpsaccel_end)
                        * (long) (tpsaclk - aetime)) / tapertime);
        } else {
            tpsatmp = 0;
        }
    }

TAE_CHK_TIME:
    // check if accel is done
    // if tps decel bit not set, accel bit is
    if ((!(outpc.engine & ENGINE_TPSDEC) && (outpc.engine & ENGINE_TPSACC)) ||
            (!(outpc.engine & ENGINE_MAPDEC) && (outpc.engine & ENGINE_MAPACC))) {
        if (tpsaclk < (aetime + tapertime)) {
            goto END_TPS;
        }
    }
    goto KILL_ACCEL;

TDE:                         // tpsdot < 0
    if ((outpc.engine & ENGINE_TPSACC) || (outpc.engine & ENGINE_MAPACC)) {       // if tps accel bit set
        if ((-tpsdot_ltch) > tpsthresh) {
            goto KILL_ACCEL;
        }
        if ((-mapdot_ltch) > mapthresh) {
            goto KILL_ACCEL;
        }
        /* on the bench this next section kills off AE once TPS stops increasing */
        if (tpsaclk < aetime) {
            tpsaclk = aetime;    // just tail off AE
        }
        goto TAILOFF;
    }
    // decel
    if (((-tpsdot_ltch) > tpsthresh) && (tdepct != 0) &&
            (prop > 0)) {
        if (outpc.rpm < 1500) {
            goto END_TPS;
        }
        outpc.tpsfuelcut = tdepct;  // in %
        outpc.engine |= ENGINE_TPSDEC;   // set tps decel bit
        goto END_TPS;
    }
    if (((-mapdot_ltch) > mapthresh) && (tdepct != 100) &&
            (prop < 100)) {
        if (outpc.rpm < 1500) {
            goto END_TPS;
        }
        outpc.tpsfuelcut = tdepct;
        outpc.engine |= ENGINE_MAPDEC;
        goto END_TPS;
    }
    if (outpc.engine & (ENGINE_TPSACC | ENGINE_TPSDEC | ENGINE_MAPACC | ENGINE_MAPDEC)) {  // if decel or just finished accel
KILL_ACCEL:
        outpc.engine &= ~(ENGINE_TPSACC | ENGINE_TPSDEC | ENGINE_MAPACC | ENGINE_MAPDEC);  // clear tps decel, accel bits
        outpc.tpsfuelcut = 100;
        SSEM0SEI;
        *pPTMpin4 &= ~0x10;   // clear accel led
        CSEM0CLI;
        tpsatmp = 0;
    }
END_TPS:

    outpc.tpsaccel = (tpsatmp * ae_scaler) / 100;
}

int new_accel_calc_percent(int x, int *x_table, int *z_table, int dot_threshold,
                           unsigned char engine_acc_flag,
                           unsigned char engine_dec_flag, unsigned char p, int mult)

{
    int tmpdot, accel_percent = 0, tmp2;

    if (x < 0) {
        tmpdot = -x;
    } else {
        tmpdot = x;
    }

    if (tmpdot >= dot_threshold) {
        accel_percent = intrp_1ditable(x, 8, x_table, 1, 
                z_table, p);
        // direct use of table data without calling interpolation routine
        RPAGE = tables[10].rpg; // HARDCODED
        tmp2 = mult + (int) (((1000 - mult) *
                    (long) (outpc.clt - ram_window.pg10.temp_table[0])
                    / (ram_window.pg10.temp_table[NO_TEMPS - 1] -
                        ram_window.pg10.temp_table[0])));

        accel_percent = ((long)accel_percent * tmp2) / 1000;

        if (x > 0) {
            outpc.engine |= engine_acc_flag;
            outpc.engine &= ~engine_dec_flag;
        } else {
            outpc.engine &= ~engine_acc_flag;
            outpc.engine |= engine_dec_flag;
        }
    } else {
        outpc.engine &= ~engine_acc_flag;
        outpc.engine &= ~engine_dec_flag;
    }

    return accel_percent;
}

void new_accel(long *lsum1, long *lsum2)
{
    /* This is the new accel idea. Basically it is a percentage of the
     * ReqFuel for this iteration only. No timers or
     * anything like that. Just measures TPSdot, looks up the appropriate
     * value, sets the percentage, done.
     */
    int tps_ac1, tps_ac2, map_ac1, map_ac2, tot_ac1, tot_ac2;
    long ltmp;
    unsigned char w;

    w = 0;
    if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw2 & 0x40)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
        w = 3;
    } else if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw2 & 0x40)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_SWITCHING)
        && pin_dualfuel && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)
        ) {
        w = 2;
    } else {
        w = 1;
    }

    tps_ac1 = 0;
    map_ac1 = 0;
    tot_ac1 = 0;
    tps_ac2 = 0;
    map_ac2 = 0;
    tot_ac2 = 0;

    if (w & 1) {
        int ae_scaler;
        if (ram5.accel_blend_percent > 0) {
            int tps_percent;
            tps_percent = new_accel_calc_percent(outpc.tpsdot, ram_window.pg28.accel_tpsdots,
                    ram_window.pg28.accel_tpsdot_amts,
                    ram5.accel_tpsdot_threshold,
                    ENGINE_TPSACC,
                    ENGINE_TPSDEC, 28, ram5.accel_CLT_multiplier);
            tps_ac1 = (ram5.accel_blend_percent * (long)tps_percent) / 1000;
        } else {
            /* 0% TPS AE */
            tps_ac1 = 0;
        }
        if (ram5.accel_blend_percent < 1000) {
            int map_percent;
            map_percent = new_accel_calc_percent(outpc.mapdot, ram_window.pg28.accel_mapdots,
                    ram_window.pg28.accel_mapdot_amts,
                    ram5.accel_mapdot_threshold,
                    ENGINE_MAPACC,
                    ENGINE_MAPDEC, 28, ram5.accel_CLT_multiplier);

            map_ac1 = ((1000 - ram5.accel_blend_percent) * (long)map_percent) / 1000;
        } else {
            map_ac1 = 0;
        }

        if (outpc.rpm <= ram4.ae_lorpm) {
            ae_scaler = 1000;
        } else if (outpc.rpm >= ram4.ae_hirpm) {
            ae_scaler = 0;
        } else {
            ae_scaler = (short) ((1000L * (ram4.ae_hirpm - outpc.rpm)) /
                    (ram4.ae_hirpm - ram4.ae_lorpm));
        }

        tot_ac1 = (int)(((long)(tps_ac1 + map_ac1) * ae_scaler) / 1000L);
    }

    if (w & 2) {
        int ae_scaler;
        if (ram5.accel_blend_percent2 > 0) {
            int tps_percent;
            tps_percent = new_accel_calc_percent(outpc.tpsdot, ram_window.pg28.accel_tpsdots2,
                    ram_window.pg28.accel_tpsdot_amts2,
                    ram5.accel_tpsdot_threshold2,
                    ENGINE_TPSACC,
                    ENGINE_TPSDEC, 28, ram5.accel_CLT_multiplier2);
            tps_ac2 = (ram5.accel_blend_percent2 * (long)tps_percent) / 1000;
        } else {
            /* 0% TPS AE */
            tps_ac2 = 0;
        }
        if (ram5.accel_blend_percent2 < 1000) {
            int map_percent;
            map_percent = new_accel_calc_percent(outpc.mapdot, ram_window.pg28.accel_mapdots2,
                    ram_window.pg28.accel_mapdot_amts2,
                    ram5.accel_mapdot_threshold2,
                    ENGINE_MAPACC,
                    ENGINE_MAPDEC, 28, ram5.accel_CLT_multiplier2);

            map_ac2 = ((1000 - ram5.accel_blend_percent2) * (long)map_percent) / 1000;
        } else {
            map_ac2 = 0;
        }

        if (outpc.rpm <= ram5.ae_lorpm2) {
            ae_scaler = 1000;
        } else if (outpc.rpm >= ram5.ae_hirpm2) {
            ae_scaler = 0;
        } else {
            ae_scaler = (short) ((1000L * (ram5.ae_hirpm2 - outpc.rpm)) /
                    (ram5.ae_hirpm2 - ram5.ae_lorpm2));
        }

        tot_ac2 = (int)(((long)(tps_ac2 + map_ac2) * ae_scaler) / 1000L);
    }

    if (w == 3) {
        outpc.tps_accel = ((((long)tps_ac1 * (100 - (int)flexblend)) + ((long)tps_ac2 * (int)flexblend)) / 100L);
        outpc.map_accel = (int)((((long)map_ac1 * (100 - flexblend)) + ((long)map_ac2 * flexblend)) / 100);
        outpc.total_accel = (int)((((long)tot_ac1 * (100 - flexblend)) + ((long)tot_ac2 * flexblend)) / 100);
    } else if (w == 2) {
        outpc.tps_accel = tps_ac2;
        outpc.map_accel = map_ac2;
        outpc.total_accel = tot_ac2;
    } else {
        outpc.tps_accel = tps_ac1;
        outpc.map_accel = map_ac1;
        outpc.total_accel = tot_ac1;
    }

    ltmp = ((outpc.total_accel * (long)ReqFuel) / 10L);

    if (w & 1) {
        *lsum1 += ltmp;
        /* bound to 0-65535 usec */
        if (*lsum1 > 6553500) {
            *lsum1 = 6553500;
        } else if (*lsum1 < 0) {
            *lsum1 = 0;
        }
    }

    if (w & 2) {
        *lsum2 += ltmp;
        /* bound to 0-65535 usec */
        if (*lsum2 > 6553500) {
            *lsum2 = 6553500;
        } else if (*lsum2 < 0) {
            *lsum2 = 0;
        }
    }

    /* Calculate old variable for user interface consistency.
       The addition to PW in the mainloop isn't run when Accel-pump is enabled. */
    outpc.tpsaccel = (unsigned int) (ltmp / 10000);
}

void main_fuel_calcs(long *lsum1, long *lsum2)
{
    unsigned char uctmp, v = 0, w = 0;
    int ve1 = 1000, ve2 = 1000, ve3 = 1000, ve4 = 1000;

    /**************************************************************************
     **  Look up volumetric efficiency as function of rpm and map (tps
     **   in alpha N mode)
     **************************************************************************/
    uctmp = 0;                  //uctmp is local tmp var
    if ((ram5.dualfuel_sw & 0x03) == 0x03) { // on and fuel
        if (pin_tsf && ((*port_tsf & pin_tsf) == pin_match_tsf)) {       // Hardware table switching
            uctmp = 1;
        }
    } else {
        if (ram4.feature5_0 & TSW_F_ON) {      //  table switching
            if ((ram4.feature5_0 & TSW_F_INPUTS) == 0) {
                if (pin_tsf && ((*port_tsf & pin_tsf) == pin_match_tsf)) {       // Hardware table switching
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_F_INPUTS) == TSW_F_RPM) {
                if (outpc.rpm > ram4.tsf_rpm) {
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_F_INPUTS) == TSW_F_MAP) {
                if (outpc.map > ram4.tsf_kpa) {
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_F_INPUTS) == TSW_F_TPS) {
                if (outpc.tps > ram4.tsf_tps) {
                    uctmp = 1;
                }
            } else if ((ram4.feature5_0 & TSW_F_INPUTS) == TSW_F_ONOFFVVT) {
                if (flagbyte15 & FLAGBYTE15_VVTON) {
                    uctmp = 1;
                }
            }
        }
    }

    if (uctmp) {
        outpc.status1 |= STATUS1_FTBLSW;
    } else {
        outpc.status1 &= ~STATUS1_FTBLSW;
    }

    if ( (ram5.dualfuel_sw & 0x1) && (ram5.dualfuel_sw & 0x02)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
        w = 15;
    } else if ((ram5.dualfuel_sw & 0x1) && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_DUALTABLE)) {  /* dual outputs */
        w = 7;
    } else if ((ram4.feature5_0 & 2) && ((ram4.feature5_0 & 0x0c) == 0) && (ram4.tsw_pin_f & 0x1f) == 14) { /* blend from VE1+2 to VE3+4 */
        w = 3;
    } else if (uctmp == 1) { /* table switched */
        w = 2;
    } else {
        w = 1;
    }

    if (((ram4.FuelAlgorithm & 0xf) == 5) && (!(ram4.feature7 & 0x02))) {
        /* Pure MAF algorithm takes VE1 as 100% */
        v = 0;
    } else {
        /* non MAF or MAF with VE tweak table */
        v = w;
    }

    if (v & 1) {
        ve1 = intrp_2ditable(outpc.rpm, outpc.fuelload, NO_EXFRPMS, NO_EXFMAPS, &ram_window.pg19.frpm_tablev1[0],
                  &ram_window.pg19.fmap_tablev1[0], (unsigned int *) &ram_window.pg12.ve_table1[0][0], 12);
    }

    if (v & 2) {
        ve3 = intrp_2ditable(outpc.rpm, outpc.fuelload, NO_EXFRPMS, NO_EXFMAPS, &ram_window.pg19.frpm_tablev3[0],
                  &ram_window.pg19.fmap_tablev3[0], (unsigned int *) &ram_window.pg18.ve_table3[0][0], 18);
    }

    if (v == 2) {
        ve1 = ve3; /* store switched result to ve1 */
    }
    /* No change for Single table unswitched, Dual table or ve1+2 -> ve3+4 blend (1) */

    /* Secondary load options (VE2 + VE4) */
    if (ram4.FuelAlgorithm & 0xf0) {
        if (((ram4.FuelAlgorithm & 0xf0) == 0x50) && (!(ram4.feature7 & 0x02))) {
            /* Pure MAF algorithm takes VE2 as 100% */
            v = 0;
        } else {
            /* non MAF or MAF with VE tweak table */
            v = w;
        }

        if (w & 1) {
            ve2 = intrp_2ditable(outpc.rpm, outpc.fuelload2, NO_EXFRPMS, NO_EXFMAPS, &ram_window.pg19.frpm_tablev2[0],
                    &ram_window.pg19.fmap_tablev2[0], (unsigned int *) &ram_window.pg12.ve_table2[0][0], 12);
        }
        if (w & 2) {
            ve4 = intrp_2ditable(outpc.rpm, outpc.fuelload2, NO_EXFRPMS, NO_EXFMAPS, &ram_window.pg21.frpm_tablev4[0],
                    &ram_window.pg21.fmap_tablev4[0], (unsigned int *) &ram_window.pg22.ve_table4[0][0], 22);
        }

        if (w == 2) {
            ve2 = ve4; /* store switched result to ve2 */
        }

        if ((ram4.loadopts & 0x3) == 1) {
            /* multiply */
            ve1 = ((long)ve1 * ve2) / 1000;
            if (w == 7) {
                ve3 = ((long)ve3 * ve4) / 1000;
            }
        } else if ((ram4.loadopts & 0x3) == 0) {
            /* add */
            ve1 = ve1 + ve2;
            if (w == 7) {
                ve3 = ve3 + ve4;
            }
        }
    }

    /* Idle advance overrides VE1 and VE3 calcs */
    if (ram4.idle_special_ops & IDLE_SPECIAL_OPS_IDLEVE) {
        if ((!(ram4.idleveadv_to_pid & IDLEVEADV_TO_PID_IDLEVE) && 
            (outpc.tps < ram4.idleve_tps) &&
            (outpc.rpm < ram4.idleve_rpm) &&
            (outpc.fuelload > ram4.idleve_load) &&
            (outpc.clt > ram4.idleve_clt) &&
            (((ram4.idle_special_ops & 0x0c) == 0) ||
             (((ram4.idle_special_ops & 0x0c) == 0x04) && (outpc.vss1 < 20)) || // less that 2.0 ms-1
             (((ram4.idle_special_ops & 0x0c) == 0x08) && (outpc.vss2 < 20)))) ||
             ((ram4.idleveadv_to_pid & IDLEVEADV_TO_PID_IDLEVE) &&
              (outpc.status2 & 0x80))
            ) {
            if (((idle_ve_timer >= ram4.idleve_delay) && 
                (flagbyte4 & flagbyte4_idlevereset)) || 
                ((ram4.idleveadv_to_pid & IDLEVEADV_TO_PID_IDLEVE) &&
                 (outpc.status2 & 0x80))) {
                outpc.status6 |= STATUS6_IDLEVE;
                if (w & 1) {
                    ve1 = intrp_2ditable(outpc.rpm, outpc.fuelload, 4, 4,
                                       &ram_window.pg19.idleve_rpms[0][0],
                                       &ram_window.pg19.idleve_loads[0][0],
                                       (unsigned int *) &ram_window.pg19.idleve_table1[0][0], 19);
                }

                if (w & 2) {
                    ve3 = intrp_2ditable(outpc.rpm, outpc.fuelload, 4, 4,
                                           &ram_window.pg19.idleve_rpms[1][0],
                                           &ram_window.pg19.idleve_loads[1][0],
                                           (unsigned int *) &ram_window.pg19.idleve_table2[0][0], 19);
                }

                if (w == 2) {
                    ve1 = ve3;  /* store switched result to ve1 */
                }

            } else {
                outpc.status6 &= ~STATUS6_IDLEVE;
                if (!(flagbyte4 & flagbyte4_idlevereset)) {
                    DISABLE_INTERRUPTS;
                    idle_ve_timer = 0;
                    ENABLE_INTERRUPTS;
                    flagbyte4 |= flagbyte4_idlevereset;
                }
            }
        } else {
            flagbyte4 &= ~flagbyte4_idlevereset;
            outpc.status6 &= ~STATUS6_IDLEVE;
        }
    }
    /* end of idle advance */

    /* If blending, at this point we've calculated individual VE1,2,3,4 values */
    if (w == 3) {
        /* ve1/ve3 and ve2/ve4 blend (3) into single ve1 and ve2
          this is the up/down blend - incompatible with dual outputs*/
        unsigned char blend;

        blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[2] & 0x1f), 9, 
                (int *) ram_window.pg25.blendx[2], 0, 
                (unsigned char *)ram_window.pg25.blendy[2], 25);
        ve1 = (int)((((long)ve1 * (100 - blend)) + ((long)ve3 * blend)) / 100);
        ve2 = (int)((((long)ve2 * (100 - blend)) + ((long)ve4 * blend)) / 100);
    } else if (w == 15) {
        /* flex blending for VE1+2 -> 3+4 blend */
        ve1 = (int)((((long)ve1 * (100 - flexblend)) + ((long)ve3 * flexblend)) / 100);
        ve2 = (int)((((long)ve2 * (100 - flexblend)) + ((long)ve4 * flexblend)) / 100);
    }

    if (w != 7) { // everything except dual outputs
        ve3 = ve1;
        ve4 = ve2;
    }
    /* Now the VE1,3 are combined to VE1  and VE2,4 are combined to VE2 */
    /* With dual outputs, w == 7 so all individuals are still there */

    outpc.vecurr1 = ve1;

    if (w == 7) { // dual outputs
        outpc.vecurr2 = ve3;
    } else if (((ram4.loadopts & 0x3) == 2) && (ram4.FuelAlgorithm & 0xf0)) { // blended (1) and secondary algo defined
        outpc.vecurr2 = ve2;
    } else { // single output
        outpc.vecurr2 = outpc.vecurr1;
    }

    serial();
    /**************************************************************************
     **
     ** Computation of Fuel Parameters
     ** Note that GAMMAE only includes Warm, Tpsfuelcut, Barocor, and Aircor
     ** and LTT
     ** (EGO no longer included)
     **
     **************************************************************************/
    {
        int inj_divider1, inj_load1,  inj_divider2, inj_load2, gammae1, gammae2;
        long fuel_tmp1, fuel_tmp2, local_lsum1, local_lsum2, local_lsum3, local_lsum4;

        fuel_tmp1 = ((outpc.warmcor * (long) outpc.tpsfuelcut) / 10L);

        //Dual fuel correction tables
        /* now included in GammaE (2012-10-30) */
        if (pin_dualfuel && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)) { // dual fuel switch active
            if (ram5.dualfuel_opt & DUALFUEL_OPT_TEMP) { // dual fuel switch - temperature table
                int adj, temperature, i;
                unsigned char t;
                i = ram5.dualfuel_temp_sens & 0x0f;
                temperature = outpc.sensors[i];

                t = ram5.sensor_trans[i];
                if (((t == 3) || (t == 4)) && (ram5.sensor_temp & 0x01)) {
                    // if using CLT, MAT sensor tranform and in degC need to reverse calc back to degF
                    temperature = ((temperature * 9) / 5) + 320;
                }
                adj = intrp_1ditable(outpc.fuel_temp[1], 10,
                   (int *) ram_window.pg21.dualfuel_temp, 1,
                   (unsigned int *) ram_window.pg21.dualfuel_temp_adj, 21);
                if (adj < -1000) {
                    adj = -1000; // can't be less than -100%
                }
                outpc.fueltemp_cor = adj;
                fuel_tmp1 = (fuel_tmp1 * (1000 + adj)) / 1000L;
            }

            if (ram5.dualfuel_opt & DUALFUEL_OPT_PRESS) { // dual fuel switch - pressure table
                int adj;

                adj = intrp_1ditable(outpc.fuel_press[1], 10,
                   (int *) ram_window.pg21.dualfuel_press, 1,
                   (unsigned int *) ram_window.pg21.dualfuel_press_adj, 21);
                if (adj < -1000) {
                    adj = -1000; // can't be less than -100%
                }
                outpc.fuelpress_cor = adj;
                fuel_tmp1 = (fuel_tmp1 * (1000 + adj)) / 1000L;
            }
        } else { /* Primary fuel corrections */
            if ((ram5.fueltemp1 & 0x1f) && (ram5.fueltemp1 & 0x80)) { // temperature correction
                int adj;

                adj = intrp_1ditable(outpc.fuel_temp[0], 10,
                   (int *) ram_window.pg25.fp_temps, 1,
                   (unsigned int *) ram_window.pg25.fp_temp_adj, 25);
                if (adj < -1000) {
                    adj = -1000; // can't be less than -100%
                }
                outpc.fueltemp_cor = adj;
                fuel_tmp1 = (fuel_tmp1 * (1000 + adj)) / 1000L;
            }

            if ((ram5.fp_opt & 0x30) == 0x10) {
                /* fuel pressure correction due to fixed regulator */
                unsigned int adj;
                if (long_abs((int)fpf_baro - (int)outpc.baro) > 20) {
                    calc_fuel_factor();
                }

                adj = lookup_fuel_factor();
                outpc.fuelpress_cor = adj;
                fuel_tmp1 = (fuel_tmp1 * (unsigned long)adj) / 1000L;
            } else if ((ram5.fp_opt & 0x30) == 0x20) {
                /* custom fuel pressure correction */
                int adj;

                adj = intrp_1ditable(outpc.fuel_press[0], 10,
                   (int *) ram_window.pg25.fp_presss, 1,
                   (unsigned int *) ram_window.pg25.fp_press_adj, 25);
                if (adj < -1000) {
                    adj = -1000; // can't be less than -100%
                }
                outpc.fuelpress_cor = adj;
                fuel_tmp1 = (fuel_tmp1 * (1000 + adj)) / 1000L;
            }
            // else no correction

        }

        /* ----------- end of corrections ---------- */

        fuel_tmp2 = fuel_tmp1;

        /* ALGORITHM 1 */

        if ((ram4.FuelAlgorithm & 0xf) != 5) {
            /* Use baro and aircor for everything except primary MAF mode */
            fuel_tmp1 = (fuel_tmp1 * ((outpc.barocor * (long) outpc.aircor) / 1000L) / 1000L);
        }

        gammae1 = (int) (fuel_tmp1 / 10);

        if (((ram4.FuelAlgorithm & 0xf) == 5) || ((ram4.FuelAlgorithm & 0xf) == 4)) {
            if (ram4.feature7 & 0x01) {
                /* uses old-style +/- MAT correction curve instead of new exposed calibration */
                inj_load1 = (int)( ((long)mafload_no_air * 
                    (1000 + intrp_1ditable(outpc.mat, NO_MAFMATS, (int *)ram_window.pg25.MatVals, 1,
                            (unsigned int *)ram_window.pg25.AirCorDel,25) )) / 1000);
            } else {
                inj_load1 = mafload_no_air;
            }

            inj_divider1 = 1000;
            /* MAF algorithm uses MAF to calculate mafload_no_air (calc is in ms3_misc.c)
              mafload is a 'synthetic' load.
              mafload_no_air excludes the aircor variable to avoid cancelling it out again here.
              VE is taken as 100.0% as MAF ideally gives a perfect mass flow number.
              The rest of the fuel calc follows as normal.
              i.e. PW = RF * mafload_no_air * other_corrections
              cf. Speed density where PW = RF * map * VE(rpm,map) * airden * other_corrections
            */
        } else {
            if (ram4.loadopts & 0x4) { // multiply map
                inj_load1 = outpc.map;
            } else {
                inj_load1 = 1000;          // normalizes to ~1 when divide by baro
            }
            if (ram4.loadopts & LOADOPTS_OLDBARO) {
                inj_divider1 = outpc.baro;
            } else {
                inj_divider1 = 1000;
            }
        }

        /* Above here fuel_tmp and inj_load are parts of the same calculation
           and do not refer to different 'banks' */

        /* now calculate by bank. lsum1 = bank1, lsum2 = bank2 for dual outputs */
        if ((ram4.EgoOption & 0x03) && (ram4.egonum > 1)) {
            // > 2 widebands, no ego here, apply to seq_pw at cylinder level calc
            local_lsum1 = fuel_tmp1 * ((1000 * (long)inj_load1) / inj_divider1) / 100L;
            local_lsum2 = local_lsum1;
        } else {
            // Do it the old way, apply egocor here
            local_lsum1 = fuel_tmp1 * ((outpc.egocor1 * (long)inj_load1) / inj_divider1) / 100L;
            local_lsum2 = fuel_tmp1 * ((outpc.egocor2 * (long)inj_load1) / inj_divider1) / 100L;
        }

        /* Now lsum1 and lsum2 have started to become tmp_pw1 and tmp_pw2 */

        /* ve will be 1000 for pure MAF */
        local_lsum1 = (local_lsum1 * ((ve1 * (long) ReqFuel) / 1000L)) / 100L;      // .01 usec
        local_lsum2 = (local_lsum2 * ((ve3 * (long) ReqFuel) / 1000L)) / 100L;      // .01 usec

        /* ALGORITHM 2 */
        if (((ram4.loadopts & 0x3) == 2) && (ram4.FuelAlgorithm & 0xf0)) { // blended (1) and secondary algo defined
            unsigned char blend;
            /* Note that local_lsum numbers don't match VE table numbers.
                local_lsum1 = VE1, local_lsum2 = VE3, local_lsum3 = VE2, local_lsum4 = VE4 */

            if ((ram4.FuelAlgorithm & 0xf0) != 0x50) {
                /* Use baro and aircor for everything except MAF mode */
                fuel_tmp2 = (fuel_tmp2 * ((outpc.barocor * (long) outpc.aircor) / 1000L) / 1000L);
            }

            gammae2 = (int) (fuel_tmp2 / 10);

            if (((ram4.FuelAlgorithm & 0xf0) == 0x50) || ((ram4.FuelAlgorithm & 0xf0) == 0x40)) {
                if (ram4.feature7 & 0x01) {
                    /* uses old-style +/- MAT correction curve instead of new exposed calibration */
                    inj_load2 = (int)( ((long)mafload_no_air * 
                        (1000 + intrp_1ditable(outpc.mat, NO_MAFMATS, (int *)ram_window.pg25.MatVals, 1,
                                (unsigned int *)ram_window.pg25.AirCorDel,25) )) / 1000);
                } else {
                    inj_load2 = mafload_no_air;
                }

                inj_divider2 = 1000;
            } else {
                if (ram4.loadopts & 0x20) { // multiply map (2nd)
                    inj_load2 = outpc.map;
                } else {
                    inj_load2 = 1000;          // normalizes to ~1 when divide by baro
                }
                if (ram4.loadopts & LOADOPTS_OLDBARO) {
                    inj_divider2 = outpc.baro;
                } else {
                    inj_divider2 = 1000;
                }
            }

            /* Above here fuel_tmp and inj_load are parts of the same calculation
               and do not refer to different 'banks' */

            if ((ram4.EgoOption & 0x03) && (ram4.egonum > 1)) {
                // > 2 widebands, no ego here, apply to seq_pw at cylinder level calc
                local_lsum3 = fuel_tmp2 * ((1000 * (long)inj_load2) / inj_divider2) / 100L;
                local_lsum4 = local_lsum1;
            } else {
                // Do it the old way, apply egocor here
                local_lsum3 = (fuel_tmp2 * ((outpc.egocor1 * (long)inj_load2) / inj_divider2) / 100L);
                local_lsum4 = (fuel_tmp2 * ((outpc.egocor2 * (long)inj_load2) / inj_divider2) / 100L);
            }

            /* Now lsum1 and lsum2 have started to become tmp_pw1 and tmp_pw2 */

            /* vecurr will be 1000 for pure MAF */
            local_lsum3 = (local_lsum3 * ((ve2 * (long) ReqFuel) / 1000L)) / 100L;      // .01 usec
            local_lsum4 = (local_lsum4 * ((ve4 * (long) ReqFuel) / 1000L)) / 100L;      // .01 usec

            /* have now calculated both banks with two algos */
            /* ve1->2 blend and ve3->4 blend (1)
             this is the side/side blend */
            blend = intrp_1dctable(blend_xaxis(ram5.blend_opt[0] & 0x1f), 9, 
                    (int *) ram_window.pg25.blendx[0], 0, 
                    (unsigned char *)ram_window.pg25.blendy[0], 25);
            *lsum1 = (((local_lsum1 * (100 - blend)) + (local_lsum3 * blend)) / 100);
            *lsum2 = (((local_lsum2 * (100 - blend)) + (local_lsum4 * blend)) / 100);
            gammae1 = (int)((((long)gammae1 * (100 - blend)) + ((long)gammae2 * blend)) / 100);
        } else {
            /* normal un-blended output */
            *lsum1 = local_lsum1;
            *lsum2 = local_lsum2;
        }

        //Long term trim
        if (ram5.ltt_opt & 0x01) {
            char ltt;
            long l;

            ltt = intrp_2dcstable(outpc.rpm, outpc.fuelload, 16, 16,
                   &ram_window.trimpage.ltt_rpms[0],
                   &ram_window.trimpage.ltt_loads[0],
                   (char *) &ram_window.trimpage.ltt_table1[0][0], 25);
            outpc.ltt_cor = ltt;
            gammae1 = ((long)gammae1 * (1000UL + ltt)) / 1000UL;
//            *lsum1 = (*lsum1  * (1000UL + ltt)) / 1000UL; // this overflows at 43ms
            l = (*lsum1  * (long)ltt) / 1000L;
            *lsum1 += l;
            l = (*lsum2  * (long)ltt) / 1000L;
            *lsum2 += l;
        }

        outpc.gammae = gammae1;
    }

    if ((ram5.dualfuel_sw & 0x1) && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_SWITCHING)
        && (ram5.dualfuel_opt & DUALFUEL_OPT_OUT) && pin_dualfuel) {
        // Dual Fuel on, Switching, different outputs
        if ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel) {
            outpc.vecurr1 = 0; // if active, then kill VE1 (secondary outputs only)
            *lsum1 = 0;
        } else {
            outpc.vecurr2 = 0; // else kill VE2 (primary outputs only)
            *lsum2 = 0;
        }
    }

    if (ram4.loadopts & 0x8) {  /* factor in afr target */
        int tmp_stoich;
        if (pin_tsw_stoich && ((*port_tsw_stoich & pin_tsw_stoich) == pin_match_tsw_stoich)) {  // Stoich switching
            tmp_stoich = ram4.stoich_alt;
        } else {
            tmp_stoich = ram4.stoich;
        }
        *lsum1 = (*lsum1 * (long) tmp_stoich) / (long) gl_afrtgt1;
        *lsum2 = (*lsum2 * (long) tmp_stoich) / (long) gl_afrtgt2; /* NB. gl_afrtgt2 is always the same as 1 (2012-10-29) */
    }
}


/**************************************************************************
 **
 ** Long term trim
 **
 **************************************************************************/
void long_term_trim_in()
{
    int x, y, xcell = 0, ycell = 0;
    unsigned int lower, higher, d;
    unsigned char fx = 0, fy = 0;

    if (stat_afr0) {
        /* if wideband is reading nonsense, then ignore */
        return;
    }

    if ((outpc.seconds - ltt_samp_l) < ram5.ltt_samp_time) { // time to check ?
        return;
    }

    ltt_samp_l = outpc.seconds;

    RPAGE = tables[25].rpg;

    for (x = 0; x < 16; x++) {
        if (ram_window.trimpage.ltt_rpms[x] > outpc.rpm) {
            break;
        }
    }
    higher = ram_window.trimpage.ltt_rpms[x];
    if (x) {
        lower = ram_window.trimpage.ltt_rpms[x - 1];
    } else {
        lower = 0;
    }

    d = (higher - lower) >> 2; // 25% of gap

    if (outpc.rpm < (lower + d)) {
        xcell = x - 1;
        fx = 1;
    } else if (outpc.rpm > (higher - d)) {
        xcell = x;
        fx = 1;
    }

    for (x = 0; x < 16; x++) {
        if (ram_window.trimpage.ltt_loads[x] > outpc.fuelload) {
            break;
        }
    }
    higher = ram_window.trimpage.ltt_loads[x];
    if (x) {
        lower = ram_window.trimpage.ltt_loads[x - 1];
    } else {
        lower = 0;
    }

    y = (higher - lower) >> 2; // 25% of gap

    if (outpc.fuelload < (int)(lower + y)) {
        ycell = x - 1;
        fy = 1;
    } else if (outpc.fuelload > (int)(higher - y)) {
        ycell = x;
        fy = 1;
    }

//    outpc.sensors[4] = fx;
//    outpc.sensors[5] = fy;

    if ((fx == 0) || (fy== 0)) {
        return; // not in range with a cell, bail
    }

    /* We have landed on a cell. */
    /* For testing, store egoCor here */
    x = outpc.egocor[0] - 1000;
    x = x / (int)ram5.ltt_agg; // larger numbers = softer change
    y = ram_window.trimpage.ltt_table1[ycell][xcell];
    /* might want to reduce X if load/rpm aren't bang-on the cell */
    y += x;
    if ((y < 127) && (y > -127)) { // signed char so +/- 12.7 max
        ram_window.trimpage.ltt_table1[ycell][xcell] = y;
    }
}

void long_term_trim_out(long *lsum1, long *lsum2)
{
    unsigned int delta = 0;
    if (flagbyte15 & FLAGBYTE15_LTT64s) {
        unsigned int ram_ad, fl_ad;
        flagbyte15 &= ~FLAGBYTE15_LTT64s;
        /* every 64s (easy minute) check to see scale of change */
        RPAGE = 0xf0;
        EPAGE = 0x19;
        fl_ad = ltt_fl_ad; // sector in current use
        for (ram_ad = 0x1a00 ; ram_ad < 0x1aff; ram_ad++, fl_ad++) { // omit last byte
            char a, b;
            int t;
            a = *(char*)ram_ad;
            b = *(char*)fl_ad;
            t = a - b;
            if (t < 0) {
                t = -t;
            }
            delta += t;
        }
        if (ram5.ltt_opt & 0x04) {
            outpc.sensors[14] = delta;
        }

        if (delta > ram5.ltt_thresh) {
            if (pin_ltt_led) {
                SSEM0SEI;
                *port_ltt_led |= pin_ltt_led;
                CSEM0CLI;
            }
        }
        ltt_timer++;
    }

    if (ltt_fl_state == 0) {

        /* check for tuning commands (when not burning) */
        if (datax1.ltt_control) {
            if ((datax1.ltt_control & 0xfc) == 0x40) { // read in from flash to RAM
                if (datax1.ltt_control == 0x41) {
                    ltt_fl_ad = 0x800;
                } else if (datax1.ltt_control == 0x42) {
                    ltt_fl_ad = 0x900;
                } else {
                    ltt_fl_ad = 0;
                }
                if (ltt_fl_ad) {
                    RPAGE = tables[25].rpg;
                    EPAGE = 0x19;
                    memcpy((unsigned char *) ram_window.trimpage.ltt_table1, (unsigned char *) ltt_fl_ad, 256);   // trim table
                    /* top RH corner (last byte in sector) is used for sector no. so fake the data there */
                    ram_window.trimpage.ltt_table1[15][15] = ram_window.trimpage.ltt_table1[15][14]; 
                }
                datax1.ltt_control = 0;
            } else if (datax1.ltt_control == 0x51) {
                ltt_fl_state = 1; // fire off a flash write
                datax1.ltt_control = 0; // done
            } else if ((datax1.ltt_control & 0xfc) == 0x60) {
                /* zero it all */
                if (datax1.ltt_control == 0x62) { // 2nd step
                    ltt_fl_ad = 0x900; // wipe second table
                    datax1.ltt_control = 0; // done
                    ltt_fl_state = 1;
                } else { // any other value acts as 1st step
                    RPAGE = tables[25].rpg;
                    memset((unsigned char *) ram_window.trimpage.ltt_table1, 0, 256);   // all zero
                    ltt_fl_ad = 0x800; // wipe first table
                    datax1.ltt_control = 0x62;
                    ltt_fl_state = 1;
                }
            } else {
                datax1.ltt_control = 0; // not recognised, no action.
            }
        }

        /* check for button */
        if ((ram5.ltt_opt & 2) == 0) {
            unsigned char tmp_int;
            /* check for time */
            tmp_int = ram5.ltt_int;
            if (tmp_int < 5) {
                /* enforce 5 mins minimum whatever user tried to do */
                tmp_int = 5;
            }
            if ((delta > ram5.ltt_thresh) && (ltt_timer >= tmp_int)
                && (outpc.tps < 300) && (outpc.rpm < 4000)) {
                /* is it time and user not caning it */
                ltt_fl_state = 1;
                ltt_timer = 0;
            }
        } else if (pin_ltt_but) {
            /* check for button */
            if (ltt_but_state == 0) { // nothing presently pressed
                if ((*port_ltt_but & pin_ltt_but) == pin_match_ltt_but) { // is now pressed
                    ltt_but_state++;
                    ltt_fl_state = 1;
                    ltt_but_debounce = (unsigned int)lmms;
                }
            } else if ((ltt_but_state == 1) && (((unsigned int)lmms - ltt_but_debounce) > 8000)) {
                if ((*port_ltt_but & pin_ltt_but) != pin_match_ltt_but) { // no longer pressed - need debounce timer
                    ltt_but_state = 0;
                }
            }
        }
    } else {
        ltt_burn();
    }
}

#define FREQ_CHANGE 30       /* allowed frequency change per sample (3Hz) */
#define FLEX_ERR_MAX 100      /* summed out of range readings allowed, before being believed */
#define FREQ_TOL 30         /* out of range tolerance (3Hz) */
/**************************************************************************
 **
 ** Calc. of Flex Fuel Sensor %alcohol and PW,spk correction (fuelcor,ffspkdel)
 **
 **************************************************************************/
void flex_fuel_calcs()
{
    unsigned int FSensFreq;

    if ((ram4.FlexFuel & 0x01) && (FSens_Pd > 0)) {
        /* check for FSens_Pd is rather bogus */
        if (flagbyte19 & FLAGBYTE19_FLEX) { // new pulse ?
            flagbyte19 &= ~FLAGBYTE19_FLEX;
            FSensFreq = 200000 / FSens_Pd;    // 0.1Hz, (FSens_Pd in .05 tics)

            FSensFreq = median3pt(&flex_data[0], FSensFreq); /* 3pt median filter on input data */

            if (FSensFreq_last) {
                unsigned char bad = 0;
                int d;

                d = FSensFreq - FSensFreq_last;
                if ((d < -FREQ_CHANGE) || (d > FREQ_CHANGE)) { // too large a frequency change
                    bad = 1;
                } else if (FSensFreq < ((ram4.fuelFreq[0] * 10U) - FREQ_TOL)) {
                    bad = 2;
                } else if (FSensFreq > ((ram4.fuelFreq[1] * 10U) + FREQ_TOL)) {
                    bad = 2;
                }

                if (bad) {
                    flex_err_cnt++;
                    if (flex_err_cnt <= FLEX_ERR_MAX) {
                        /* Use last value instead */
                        FSensFreq = FSensFreq_last;
                        /* If within tolerance, bound frequency */
                        if ((FSensFreq < (ram4.fuelFreq[0] * 10U)) && (FSensFreq > ((ram4.fuelFreq[0] * 10U) - FREQ_TOL))) {
                            FSensFreq = ram4.fuelFreq[0] * 10U;
                        } else if ((FSensFreq > (ram4.fuelFreq[1] * 10U)) && (FSensFreq < ((ram4.fuelFreq[1] * 10U) + FREQ_TOL))) {
                            FSensFreq = ram4.fuelFreq[1] * 10U;
                        }
                    } else {
                    /* Else, beyond our allowed limit, perhaps the value is real? */
                        bad = 0;
                        flex_err_cnt = FLEX_ERR_MAX;
                    }                        
                } else { 
                    /* looks ok */
                    if (flex_err_cnt) {
                        flex_err_cnt--;
                    }
                }

                if (!bad) {
                    /* apply lag factor to frequency*/
                    __asm__ __volatile__(
                        "suby   %2\n"
                        "clra\n"
                        "ldab    #10\n"    // fixed 10% LF
                        "emuls\n"
                        "ldx     #100\n"
                        "edivs\n"
                        "addy    %2\n"
                        :"=y"(FSensFreq)
                        :"y"(FSensFreq), "m"(FSensFreq_last)
                        :"x","d"
                    );

                    if ((FSensFreq >= (ram4.fuelFreq[0] * 10U)) && (FSensFreq <= (ram4.fuelFreq[1] * 10U))) {
                        unsigned int freq_delta, freq_max_min;
                        int tmp_fuel, tmp_spark, base_fuel, base_spark;

                        freq_delta = FSensFreq - (ram4.fuelFreq[0] * 10U);
                        freq_max_min = (ram4.fuelFreq[1] - ram4.fuelFreq[0]) * 10; // *10 to allow for 0.1Hz factor in FSensFreq

                        tmp_fuel = ram4.fuelCorr[0] + ((freq_delta * (ram4.fuelCorr[1] - ram4.fuelCorr[0])) / freq_max_min);      // %
                        tmp_spark = ram4.ffSpkDel[0] + (((long)freq_delta * (ram4.ffSpkDel[1] - ram4.ffSpkDel[0])) / freq_max_min);    // degx10
                        outpc.fuel_pct = ram5.fuel_pct[0] + (((unsigned long) freq_delta * (ram5.fuel_pct[1] - ram5.fuel_pct[0])) / freq_max_min);      // %

                        if (ram5.flex_baseline) {
                            unsigned int base_delta, base_max_min;
                            base_delta = ram5.flex_baseline - ram5.fuel_pct[0];
                            base_max_min = ram5.fuel_pct[1] - ram5.fuel_pct[0];

                            base_fuel = ram4.fuelCorr[0] + ((base_delta * (ram4.fuelCorr[1] - ram4.fuelCorr[0])) / base_max_min);      // %
                            base_spark = ram4.ffSpkDel[0] + ((base_delta * (ram4.ffSpkDel[1] - ram4.ffSpkDel[0])) / base_max_min);    // degx10

                            outpc.fuelcor = (tmp_fuel * 100UL) / base_fuel;
                            outpc.flex_advance = tmp_spark - base_spark;
                        } else {
                            outpc.fuelcor = tmp_fuel;
                            outpc.flex_advance = tmp_spark;
                        }

                        stat_flex &= ~1; // clear current bit
                    } else {                // sensor reading bad - use default
                        if ((outpc.seconds > 3) && (ram5.cel_opt3 & 0x01)) { // give it a chance to start. Only report if CEL is on.
                            stat_flex |= 1;
                        }
                        outpc.fuel_pct = 10; // fault condition. 1% is never going to happen. Safe default for anyone used blended spark tables.
                        outpc.fuelcor = ram5.fuelCorr_default;
                        outpc.flex_advance = ram5.fuelSpkDel_default;
                    }

                    /* Fuel temperature */
                    if ((ram5.fueltemp1 & 0x1f) == 1) {

                        ff_pw = median3pt(&flextemp_data[0], ff_pw); /* 3pt median filter on input data */

                        /* bound limits */
                        if (ff_pw <= ram5.ff_tpw0) {
                            outpc.fuel_temp[0] = ram5.ff_temp0;
                        } else if (ff_pw >= ram5.ff_tpw1) {
                            outpc.fuel_temp[0] = ram5.ff_temp1;
                        } else {
                            /* Without sampling over a number of periods, this is jittery */
                            ff_temp_accum += ff_pw;
                            ff_temp_cnt++;
                            /* calculations are in fuel_sensors() */
                        }
                    }
                }
            }
            FSensFreq_last = FSensFreq;
            if ((ram5.dualfuel_sw & 0x1)
                && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {

                flexblend = intrp_1dctable(outpc.fuel_pct, 9, 
                        (int *) ram_window.pg25.blendx[7], 0, 
                        (unsigned char *)ram_window.pg25.blendy[7], 25);
            } else {
                flexblend = 0;
            }
        }
    } else {                    // no flex fuel
        outpc.fuelcor = 100;    // %
        outpc.flex_advance = 0;           // degx10
        outpc.fuel_pct = 0;
    }
//    outpc.status4 = flexblend;
}

/**************************************************************************
 **
 ** Over Run Fuel Cut
 **
 **************************************************************************/
void do_overrun(void)
{
    unsigned int rpm_droprate = 0, fc_rpm_lim = ram4.fc_rpm_lower;

    flagbyte17 &= ~FLAGBYTE17_OVERRUNFC; // only checked in mainloop code
    /* Figure out what RPM we want to re-enable fuel at if variable fuel re-engagement is on */

    if ((ram4.OvrRunC & OVRRUNC_ON) && ((flagbyte2 & flagbyte2_crank_ok) == 0)) { // also check that not just after cranking

        if (outpc.rpmdot < 0) {
            rpm_droprate = -outpc.rpmdot;

            if (rpm_droprate >= ram4.fuelcut_fuelon_upper_rpmdot) {
                fc_rpm_lim = ram4.fc_rpm;
            } else if (rpm_droprate <= ram4.fuelcut_fuelon_lower_rpmdot) {
                fc_rpm_lim = ram4.fc_rpm_lower;
            } else {
                /* How far between the upper and lower rpmdots are we? */
                rpm_droprate = (((unsigned long)rpm_droprate - ram4.fuelcut_fuelon_lower_rpmdot) * 100UL) /
                         ((unsigned long)ram4.fuelcut_fuelon_upper_rpmdot - ram4.fuelcut_fuelon_lower_rpmdot);
                /* Figure out the corresponding RPM that we should cut fuel back in at based on the above */
                fc_rpm_lim = ram4.fc_rpm + ((((unsigned long)ram4.fc_rpm - ram4.fc_rpm_lower) * rpm_droprate) / 100UL);
            }
        }

        if ((outpc.rpm > ram4.fc_rpm) && (outpc.map < ram4.fc_kpa)
            && (outpc.tps < ram4.fc_tps) && (outpc.clt > ram4.fc_clt)) {
            if (fc_phase == 0) {
                fc_phase = 1; // start
                fc_counter = ram4.fc_delay;
            }
        } else if ((outpc.rpm < fc_rpm_lim) || (outpc.map >= ram4.fc_kpa) ||
                   (outpc.tps >= ram4.fc_tps) || (outpc.clt <= ram4.fc_clt)) {
            if (fc_phase < 3) {
                fc_phase = 0;
            } else if (fc_phase == 3) { // transition back or jump to full fuel ?
                if ((ram4.OvrRunC & (OVRRUNC_RETIGN | OVRRUNC_PROGRET)) && (outpc.tps < ram4.fc_tps) // fuel or spark transition on return and throttle shut
                   && (rpm_droprate < ram4.fuelcut_fuelon_lower_rpmdot) ) { // but revs not plummeting (clutch in)
                    fc_counter = ram5.fc_trans_time_ret;
                } else {
                    fc_counter = 0; // throttle opened, straight back to normal timing
                    fc_retard_time = 0;
                }
                fc_phase = 4;
            }
        } else {
            /* If we're here, we're in hysteresis, 
             * if we were fully on, continue cutting fuel.
             * but if we hadn't reached ON, then disable.
             */
            if (fc_phase < 3) {
                fc_phase = 0;
            }
        }
    } else {
        fc_off_time = 0xff; // counter for return of EGO
        fc_phase = 0; // state machine
    }

    /* handle state machine */

    if (fc_phase == 0) { // off
        fc_retard_time = 0;
        fc_ae = 0;
        skipinj_overrun = 0;

    } else if (fc_phase == 1) { // activation timer
        fc_retard_time = 0;
        if (fc_counter == 0) {
            if ( (ram4.OvrRunC & OVRRUNC_PROGIGN) // fuel or spark transition to fuel-cut
                || ((ram4.OvrRunC & OVRRUNC_PROGCUT) && ((flagbyte4 & FLAGBYTE4_STAGING_ON) == 0)) ) {
                fc_counter = ram5.fc_transition_time;
                fc_phase = 2;
            } else { // no transition
                flagbyte17 |= FLAGBYTE17_OVERRUNFC;
                fc_phase = 3;
                fc_off_time = 0xff;
            }
        }

    } else if (fc_phase == 2) { // transition
        unsigned int t;
        t = ram5.fc_transition_time - fc_counter; // downcounter
        if (ram4.OvrRunC & OVRRUNC_PROGIGN) {
            fc_retard_time = t; // apply timing change on cut
        } else {
            fc_retard_time = 0;
        }

        if (fc_counter == 0) {
            fc_phase = 3;
        } else {
            if ( (ram4.OvrRunC & OVRRUNC_PROGCUT) && ((flagbyte4 & FLAGBYTE4_STAGING_ON) == 0) ) { // progressive fuel cut
                unsigned int step, b;
                step = ram5.fc_transition_time / (num_inj - 1);
                for (b = 0 ; b < num_inj ; b++) {
                    if (t > (step * b)) {
                        skipinj_overrun |= fuelcut_array[num_inj - 1][b];
                    }
                }
            }
        }

    } else if (fc_phase == 3) { // fully on
        // fc_retard is maintained
        flagbyte17 |= FLAGBYTE17_OVERRUNFC;
        fc_off_time = 0xff;
        /* sit at this phase until code above moves us on */

    } else if (fc_phase == 4) { // was on, now off
        fc_ae = ram5.fc_ae_pct;
        if (((ram4.OvrRunC & OVRRUNC_PROGRET) == 0) || (flagbyte4 & FLAGBYTE4_STAGING_ON)) {
            skipinj_overrun = 0; // ensure all cyls are ready for action
        } else if (((ram4.OvrRunC & OVRRUNC_PROGCUT) == 0) // prog fuel cut off, prog fuel return on
                && (outpc.tps < ram4.fc_tps) // TPS still low
                && (rpm_droprate < ram4.fuelcut_fuelon_lower_rpmdot)) {
            int b;
            /* kill off all injectors ready so they get returned gradually */
            for (b = 0 ; b < num_inj ; b++) {
                skipinj_overrun |= fuelcut_array[num_inj - 1][b];
            }
        }
        fc_lmms = (unsigned int)lmms;
        fc_phase = 5;

    } else if (fc_phase == 5) { // turn-off AE event and return transition
        unsigned int lmms_t;
        /* If throttle touched, cancel the transition */
        if ((outpc.tps >= ram4.fc_tps)
            || (rpm_droprate >= ram4.fuelcut_fuelon_lower_rpmdot)) {
            fc_counter = 0;
            fc_retard_time = 0;
            skipinj_overrun = 0; // stop cutting immediately
        }

        lmms_t = (unsigned int)lmms;
        if ((lmms_t - fc_lmms) > ((unsigned int)ram5.fc_ae_time * 78)) {
            fc_ae = 0;
            fc_off_time = 0; // up counter until EGO can run again
        }
        if ((fc_counter == 0) && (fc_ae == 0)) {
            fc_phase = 0;
            fc_retard_time = 0;
            skipinj_overrun = 0; // stop cutting immediately
        } else {
            if (ram4.OvrRunC & OVRRUNC_RETIGN) { // apply timing change on return
                fc_retard_time = fc_counter; // downcounter
            } else {
                fc_retard_time = 0;
            }
        }

        /* Progressive fuel return, possible alternative to all on + AE
           makes no sense to use this with AE */
        if ( (ram4.OvrRunC & OVRRUNC_PROGRET) && ((flagbyte4 & FLAGBYTE4_STAGING_ON) == 0) ) {
            unsigned int step, b;
            step = ram5.fc_trans_time_ret / (num_inj - 1);
            for (b = 0 ; b < num_inj ; b++) {
                if (fc_counter < (step * b)) {
                    skipinj_overrun &= ~fuelcut_array[num_inj - 1][b];
                }
            }
        }
    }
}

/**************************************************************************
 ** Additional fuel from nitrous/launch. Zero if unused.
 **************************************************************************/
void n2o_launch_additional_fuel(void)
{
    long tmp_addfuel;

    tmp_pw1 += (unsigned long) outpc.n2o_addfuel * 100;
    tmp_pw2 += (unsigned long) outpc.n2o_addfuel * 100;

//  DONE_ADD_N2O:

    if (outpc.status3 & STATUS3_LAUNCHON) {
        tmp_addfuel = (long) ram4.launch_addfuel * 100L;
        if (ram4.launch_opt & 0x10) {
            tmp_pw1 += tmp_addfuel;
        }
        if (ram4.launch_opt & 0x20) {
            tmp_pw2 += tmp_addfuel;
        }
    }

    if ( (flagbyte9 & FLAGBYTE9_EGTADD) /* fuel addition requested */
        && (!(((ram4.sequential & 3) == SEQ_FULL) && (ram4.egt_conf & EGT_CONF_PERCYL))) /* not sequential + per-cylinder */
        ) {
        tmp_addfuel = (long) ram4.egt_addfuel * 100L;
        if ((ram4.egt_conf & 0x04) && tmp_pw1) {
            tmp_pw1 += tmp_addfuel;
        }
        if ((ram4.egt_conf & 0x08) && tmp_pw2) {
            tmp_pw2 += tmp_addfuel;
        }
    }

//tc
    tmp_addfuel = (long) tc_addfuel * 100L;
    if (ram5.tc_opt & 0x40) {
        tmp_pw1 += tmp_addfuel;
    }
    if (ram5.tc_opt & 0x80) {
        tmp_pw2 += tmp_addfuel;
    }

}

void injpwms(void)
{
    if (flagbyte1 & flagbyte1_tstmode) { /* in testmode */
        if (ram5.testcoil & 0x80) { /* custom PWM params, otherwise normal calcs */
            InjPWMDty1 = ram5.testInjPWMDty;
            InjPWMPd1 = ram5.testInjPWMPd;
            InjPWMTim1 = ram5.testInjPWMTim;

            InjPWMDty2 = InjPWMDty1;
            InjPWMPd2 = InjPWMPd1;
            InjPWMTim2 = InjPWMTim1;

            goto INJPWM_COM;
        }
    }

    /* Calc injector PWM parameters */
    if (pin_dualfuel && (ram5.dualfuel_sw2 & 0x04) && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)) { // dual fuel switch
        InjPWMDty1 = ram5.Inj2PWMDty1;
        InjPWMPd1 = ram5.Inj2PWMPd1;
        if (ram5.opentime2_opt[8] & 0x10) { // PWM enabled or not
            InjPWMTim1 = ram5.Inj2PWMTim1;
        } else {
            InjPWMTim1 = 255;
        }
        if (ram5.opentime2_opt[9] & 0x20) { // own bank 2 settings
            InjPWMDty2 = ram5.Inj2PWMDty2;
            InjPWMPd2 = ram5.Inj2PWMPd2;
            if (ram5.opentime2_opt[9] & 0x10) { // PWM enabled or not
                InjPWMTim2 = ram5.Inj2PWMTim2;
            } else {
                InjPWMTim2 = 255;
            }
        } else {
            InjPWMDty2 = InjPWMDty1;
            InjPWMPd2 = InjPWMPd1;
            InjPWMTim2 = InjPWMTim1;
        }
    } else { // normal
        InjPWMDty1 = ram4.InjPWMDty;
        InjPWMPd1 = ram4.InjPWMPd;
        if (ram4.opentime_opt[8] & 0x10) { // PWM enabled or not
            InjPWMTim1 = ram4.InjPWMTim;
        } else {
            InjPWMTim1 = 255;
        }
        if (ram4.opentime_opt[9] & 0x20) { // own bank 2 settings
            InjPWMDty2 = ram4.InjPWMDty2;
            InjPWMPd2 = ram4.InjPWMPd2;
            if (ram4.opentime_opt[9] & 0x10) { // PWM enabled or not
                InjPWMTim2 = ram4.InjPWMTim2;
            } else {
                InjPWMTim2 = 255;
            }
        } else { // bank2 = bank1
            InjPWMDty2 = InjPWMDty1;
            InjPWMPd2 = InjPWMPd1;
            InjPWMTim2 = InjPWMTim1;
        }
    }
    
    INJPWM_COM:;

    {
        unsigned int t1;
        t1 = (unsigned int)InjPWMPd1 * InjPWMDty1;
        t1 /= 100; /* <- check this out for lousy assembly output */
        pwmd1 = t1;

        pwmd2 = ((unsigned int)InjPWMPd2 * InjPWMDty2) / 100;
    }
}

void do_final_fuelcalcs(void)
{
    unsigned int uitmp1, uitmp2;
    unsigned long tmp_pw_sf, ultmp;

    run_EAE_calcs();
    run_xtau_calcs();
    injpwms();

/* Anti-lag pw modification */
    if (flagbyte23 & FLAGBYTE23_ALS_ON) {
        tmp_pw1 = (tmp_pw1 * (1000 + outpc.als_addfuel)) / 1000;
        tmp_pw2 = (tmp_pw2 * (1000 + outpc.als_addfuel)) / 1000;
    }
/* end anti-lag */

// limit PW to 65ms (or 65x4 ms)
    if (tmp_pw1 > 6500000) {
        tmp_pw1 = 6500000;
    }

    if (tmp_pw2 > 6500000) {
        tmp_pw2 = 6500000;
    }

    // If only using V3 inj, this might not be ideal, but result is stored to pwcalc1
    // would be better if using pwseq[9]
    if (!(ram4.hardware & HARDWARE_MS3XFUEL)) { 
        if ((ram4.EgoOption & 0x03) && (ram4.egonum > 1)) {
            // EGO correction on and more than one sensor, do calc in loop
            tmp_pw_sf = (tmp_pw1 * outpc.egocor[ram5.egomap[8] & 0x07]) / 1000;
        } else {
            tmp_pw_sf = tmp_pw1;
        }
    } else {
        tmp_pw_sf = tmp_pw1;
    }

    if ((glob_sequential & SEQ_TRIM) && (!(ram4.hardware & HARDWARE_MS3XFUEL))) { // V3 inj trim - should this be within above (if) ?
        ultmp = (tmp_pw_sf * (500UL + fuel_trim[8])) / 500UL;
    } else {
        ultmp = tmp_pw_sf;
    }

    /* rail pre-OT PW to 65ms */
    if (ultmp > 6500000) {
        uitmp2 = 65000;
    } else {
        uitmp2 = ultmp / 100;
    }

    uitmp2 = smallpw(uitmp2, 8);

    rawpw[16] = uitmp2;
    uitmp1 = calc_opentime(8);
    outpc.deadtime1 = uitmp1;

    /* See if we can add any OT */
    if (uitmp1 > ((unsigned int)65000 - uitmp2)) {
        uitmp2 = 65000;
    } else if (uitmp2) {
        uitmp2 += uitmp1;
    }

    pwcalc1 = uitmp2;
    // let ign_in set outpc.pwseq[9]

    // If only using V3 inj, this might not be ideal, but result is stored to pwcalc1
    // would be better if using pwseq[9]
    if (!(ram4.hardware & HARDWARE_MS3XFUEL)) { 
        if ((ram4.EgoOption & 0x03) && (ram4.egonum > 1)) {
            // EGO correction on and more than one sensor, do calc in loop
            tmp_pw_sf = (tmp_pw2 * outpc.egocor[ram5.egomap[9] & 0x07]) / 1000;
        } else {
            tmp_pw_sf = tmp_pw2;
        }
    } else {
        tmp_pw_sf = tmp_pw2;
    }

    if ((glob_sequential & SEQ_TRIM) && (!(ram4.hardware & HARDWARE_MS3XFUEL))) { // V3 inj trim
        ultmp = (tmp_pw_sf * (500UL + fuel_trim[9])) / 500UL;
    } else {
        ultmp = tmp_pw_sf;
    } 

    /* rail pre-OT PW to 65ms */
    if (ultmp > 6500000) {
        uitmp2 = 65000;
    } else {
        uitmp2 = ultmp / 100;
    }

    uitmp2 = smallpw(uitmp2, 9);
    rawpw[17] = uitmp2;

    if (ram4.opentime_opt[9] & 0x20) {
        uitmp1 = calc_opentime(9);
    } else {
        uitmp1 = calc_opentime(8);  // bank1 settings
    }

    /* See if we can add any OT */
    if (uitmp1 > ((unsigned int)65000 - uitmp2)) {
        uitmp2 = 65000;
    } else if (uitmp2) {
        uitmp2 += uitmp1;
    }

    pwcalc2 = uitmp2;
    // let ign_in set outpc.pwseq[10]
}

void do_seqpw_calcs(unsigned long pw, int start, int end)
{
    unsigned long ultmp, tmp_pw_sf = 0;
    int fire, i, ix;
    unsigned int uitmp1, uitmp2;
    unsigned long egt_addfuel;

    if ( (flagbyte9 & FLAGBYTE9_EGTADD) /* EGT fuel addition requested */
        && (((ram4.sequential & 3) == SEQ_FULL) && (ram4.egt_conf & EGT_CONF_PERCYL)) /* is sequential + per-cylinder */
        ) {
        egt_addfuel = (long) ram4.egt_addfuel * 100L;
    } else {
        egt_addfuel = 0;
    }

    if (!((ram4.EgoOption & 0x03) && (ram4.egonum > 1)) || (start >= 8)) {
        /* if nil or single EGO sensor then do calc out here and save doing
         * it every loop iteration
         */
        tmp_pw_sf = pw;
    }

    for (ix = start, i = 0; ix < end; ix++, i++) {
        if (pw) {
            if ((ram4.EgoOption & 0x03) && (ram4.egonum > 1) && (ix < NUM_TRIGS)) {
                // EGO correction on and more than one sensor, do calc in loop
                tmp_pw_sf = (pw * outpc.egocor[ram5.egomap[i] & 0x0f]) / 1000;
            }
            if ((ix < NUM_TRIGS) && (glob_sequential & SEQ_TRIM) && (ram4.hardware & HARDWARE_MS3XFUEL)) {
                fire = ram4.fire[i];
                if ((fire > 0) && (fire <= NUM_TRIGS)) {
                    fire--;

                    /* Previous code was:
                     * ultmp = (tmp_pw_sf * (1000 + (fuel_trim[fire] << 1))) / 1000L;
                     * But that can overflow unsigned long. This one just fits. */
                    ultmp = (tmp_pw_sf * (500UL + fuel_trim[fire])) / 500UL;

                    if (egt_addfuel && (egt_warn & twopow[fire+start])) { /* EGT fuel to add for this cylinder? */
                        ultmp += egt_addfuel;
                    }

                    /* rail pre-DT PW to 65ms */
                    if (ultmp > 6500000) {
                        uitmp2 = 65000;
                    } else {
                        uitmp2 = ultmp / 100;
                    }

                    uitmp2 = smallpw(uitmp2, fire);
                    rawpw[ix] = uitmp2;

                    uitmp1 = calc_opentime(fire);
                    if (fire == 0) {
                        outpc.deadtime1 = uitmp1;
                    }

                    /* See if we can add any DT */
                    if (uitmp1 > ((unsigned int)65000 - uitmp2)) {
                        uitmp2 = 65000;
                    } else if (uitmp2) {
                        uitmp2 += uitmp1;
                    }

                    seq_pw[ix] = uitmp2;
                    outpc.pwseq[fire+start] = uitmp2;
                }
            } else {

                fire = ram4.fire[i];

                if ((glob_sequential & SEQ_TRIM) && (!(ram4.hardware & HARDWARE_MS3XFUEL)) && (ix >= 8)) {
                    ultmp = (tmp_pw_sf * (500UL + fuel_trim[ix])) / 500UL;
                } else {
                    ultmp = tmp_pw_sf;
                }

                /* Try to apply to correct cylinder */
                if (egt_addfuel) {
                    if ((fire > 0) && (fire <= NUM_TRIGS)) {
                        if (egt_warn & twopow[fire - 1]) { /* EGT fuel to add for this cylinder? */
                            ultmp += egt_addfuel;
                        }
                    }
                }

                /* rail pre-OT PW to 65ms */
                if (ultmp > 6500000) {
                    uitmp2 = 65000;
                } else {
                    uitmp2 = ultmp / 100;
                }

                uitmp2 = smallpw(uitmp2, ix);
                rawpw[ix] = uitmp2;

                uitmp1 = calc_opentime(ix);

                /* See if we can add any OT */
                if (uitmp1 > ((unsigned int)65000 - uitmp2)) {
                    uitmp2 = 65000;
                } else if (uitmp2) {
                    uitmp2 += uitmp1;
                }
                seq_pw[ix] = uitmp2;
                /* Generally all PWs the same, so doesn't matter, but in the case of
                    added EGT fuel, it is per-cylinder (like with-trim section above)
                    so apply to correct display cylinder */
                if ((fire > 0) && (fire <= NUM_TRIGS)) {
                    outpc.pwseq[fire - 1] = uitmp2;
                } else {
                    outpc.pwseq[ix] = uitmp2;
                }
            }
        } else {
            seq_pw[ix] = 0;
            outpc.pwseq[ix] = 0;
        }
    }
}

void do_sequential_fuel(void)
{
    inj_event *events_to_fill = NULL;
    unsigned char last_tooth_no;
    ign_time last_tooth_time;
    int last_tooth_ang, fuel_ang, inttmp;
    long dsf_lsum;

    if (outpc.rpm) {
        if (ram4.hardware & HARDWARE_MS3XFUEL) {
            if (do_dualouts) {
                unsigned char start, end;

                if (((num_inj > 4) && (glob_sequential & SEQ_FULL)) ||
                        ((num_inj > 4) && !(glob_sequential & 0x3)) ||
                        (ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3)) {

                    start = 8;
                    end = 10;
                } else {
                    if ((cycle_deg == 3600)
                        && (!(ram4.EngStroke & 0x01)) && (num_cyl < 5)
                        && (glob_sequential & SEQ_SEMI)) {
                        start = num_inj >> 1; // like wasted-spark with injectors
                    } else {
                        start = num_inj;
                    }
                    end = start << 1;
                }
                do_seqpw_calcs(tmp_pw1, 0, start);
                do_seqpw_calcs(tmp_pw2, start, end);
            } else {
                do_seqpw_calcs(tmp_pw1, 0, num_inj);
            }
        } else {
            if (do_dualouts) {
                do_seqpw_calcs(tmp_pw1, 8, 9);
                do_seqpw_calcs(tmp_pw2, 9, 10);
            } else {
                do_seqpw_calcs(tmp_pw1, 8, 10);
            }
        }

        /* also do timing lookup */

        if (glob_sequential & 0x3) {

            dsf_lsum =
                intrp_2ditable(outpc.rpm, outpc.fuelload, 12, 12,
                        &ram_window.pg9.inj_timing_rpm[0],
                        &ram_window.pg9.inj_timing_load[0],
                        (unsigned int *) &ram_window.
                        pg9.inj_timing[0][0], 9);

            dsf_lsum += vvt_inj_timing_adj;
            outpc.inj_timing_pri = dsf_lsum;

            DISABLE_INTERRUPTS;
            last_tooth_time = tooth_diff_rpm;
            last_tooth_no = tooth_no_rpm;
            ENABLE_INTERRUPTS;

            if (inj_events == inj_events_a) {
                events_to_fill = inj_events_b;
            } else {
                events_to_fill = inj_events_a;
            }

            if (last_tooth_no == 0) {
                goto END_SEQUENTIAL;
            }
            /* figure out angle between this tooth and previous one
             * array holds angle _ahead_ of the tooth, so step back a tooth
             */
            last_tooth_no--;
            if ((char) last_tooth_no <= 0) {
                last_tooth_no = last_tooth;
            }
            /* make sure we don't land on a zero one */
            while ((last_tooth_ang = deg_per_tooth[last_tooth_no - 1]) == 0) {
                last_tooth_no--;
                if ((char) last_tooth_no <= 0) { /* limits us to 127 teeth */
                    last_tooth_no = last_tooth;
                }
            }

            if (glob_sequential & SEQ_BEGIN) {
                fuel_ang = (int) dsf_lsum;
            } else {
                unsigned long seql;
                if (glob_sequential & SEQ_MIDDLE) {
                    seql = pwcalc1 / 2; /* FIXME review for smallpw */
                } else {
                    seql = pwcalc1;
                }

                if (outpc.status8 & STATUS8_PW4X) {
                    seql *= 4; // PW really 4x length
                }

                fuel_ang =
                    (int) ((seql * last_tooth_ang) / last_tooth_time.time_32_bits);
                fuel_ang = dsf_lsum + fuel_ang;
            }

            /* bound positive */
            while (fuel_ang > 3600) {
                fuel_ang -= cycle_deg;
            }

            /* bound negative */
            while (fuel_ang < -3600) {
                fuel_ang += cycle_deg;
            }

            /* Add calcs for trim and angle here, for now hardcoded angle */
            wheel_fill_inj_event_array(events_to_fill, fuel_ang,
                    last_tooth_time, last_tooth_ang,
                    num_inj_events, num_inj_outs, 0);

            if (do_dualouts) {

                dsf_lsum = intrp_2ditable(outpc.rpm, outpc.fuelload, 12, 12,
                                       &ram_window.pg21.inj_timing_sec_rpm[0],
                                       &ram_window.pg21.inj_timing_sec_load[0],
                                       (unsigned int *) &ram_window.
                                       pg21.inj_timing_sec[0][0], 21);

                dsf_lsum += vvt_inj_timing_adj;
                outpc.inj_timing_sec = dsf_lsum;

                if (glob_sequential & SEQ_BEGIN) {
                    fuel_ang = (int) dsf_lsum;
                } else {
                    if (glob_sequential & SEQ_MIDDLE) {
                        inttmp = pwcalc2 / 2;
                    } else {
                        inttmp = pwcalc2;
                    }

                    fuel_ang = (int) (((long) inttmp * last_tooth_ang) /
                            (long) last_tooth_time.time_32_bits);
                    fuel_ang = dsf_lsum + fuel_ang;
                }

                while (fuel_ang > 3600) {
                    fuel_ang -= cycle_deg;
                }

                while (fuel_ang < -3600) {
                    fuel_ang += cycle_deg;
                }

                wheel_fill_inj_event_array(events_to_fill, fuel_ang,
                        last_tooth_time, last_tooth_ang,
                        num_inj_events, num_inj_outs, INJ_FILL_STAGED);
            }

            if (events_to_fill == inj_events_a) {
                DISABLE_INTERRUPTS;
                inj_events = inj_events_a;
                ENABLE_INTERRUPTS;
            } else {
                DISABLE_INTERRUPTS;
                inj_events = inj_events_b;
                ENABLE_INTERRUPTS;
            }
        }
    }
END_SEQUENTIAL:;
}

void calc_fuel_trims(void)
{
    char trimtmp;

    // update one trim on each pass
    if (fuel_trim_cnt >= NUM_TRIGS) { // range check first
        fuel_trim_cnt = 0;
    }

    trimtmp = (char) intrp_2dcstable(outpc.rpm, outpc.fuelload, 6, 6,
            &ram_window.pg9.inj_trim_rpm[0],
            &ram_window.pg9.inj_trim_load[0],
            &ram_window.pg9.inj_trim[fuel_trim_cnt][0][0],
            9);
    fuel_trim[fuel_trim_cnt] = trimtmp;
    // the array is per table NOT adjusted for firing order
    fuel_trim_cnt++;
}

/*
 *    Calculates num_cyl and divider. At init and on the fly.
 */
void calc_divider(void)
{
    unsigned char tmp_divider, tmp_num_cyl;

    // initialise num_cyl variable.
/*
    *** this seems to do more harm than good ***
    if (ram4.EngStroke & 1) {
        tmp_num_cyl = (ram4.no_cyl & 0x1f) << 1;     // two stroke so double it
    } else */ {
        tmp_num_cyl = (ram4.no_cyl & 0x1f);
    }

    tmp_divider = ram4.Divider; // same to start with

    // handle odd configurations
    if ((ram4.no_cyl & 0x1f) == 1) {
        if (ram4.ICIgnOption & 8) {
            conf_err = 33; // No such thing as oddfire 1 cyl? 
        }
        if ((ram4.EngStroke & 1) == 0) { // 4 stroke
            if ((ram4.spk_mode3 & 0xc0) == 0x40) { // wasted or wasted-COP
                tmp_num_cyl = 2; // would have been 1 // tach every 360deg
                tmp_divider = ram4.Divider << 1;
            // probably in error to be anything else
            }
        }
    } else if ((ram4.no_cyl & 0x1f) == 2) {
/* Does this make any sense?
        if (((ram4.EngStroke & 0x03) == 1) && ((ram4.ICIgnOption & 8) == 0) && ((ram4.spk_mode3 & 0xc0) == 0x00)) {
            // 2 cyl, 2 stroke, even fire and both cyl spark at the same time
            tmp_num_cyl = 2; // would have been 4 // tach every 360deg
            if ((ram4.Divider & 1) == 0) {
                tmp_divider = ram4.Divider >> 1;
            } else {
                conf_err = 72; // need more squirts, divider can't be halved
            }
        } else */
if ( ((ram4.EngStroke & 1) == 0) && (ram4.ICIgnOption & 8) && ((ram4.spk_mode3 & 0xc0) == 0x40) ) {
            // 2 cyl, 4 stroke, odd fire, wasted spark (twice as often)
            tmp_num_cyl = 4;
        }

    } else if ((ram4.no_cyl & 0x1f) == 3) {
        if (ram4.ICIgnOption & 8) {
            conf_err = 33; // Unsure how to support oddfire 3 cyl 
        }
        if ((ram4.spk_mode3 & 0xc0) == 0x40) { // wasted
            if (spkmode != 4) {
                conf_err = 71;
            } else {
                tmp_num_cyl = 6; /* this causes a problem with fuel */
                tmp_divider = ram4.Divider << 1;
            }
        }
    } else if ((ram4.no_cyl & 0x1f) == 5) {
        if (ram4.ICIgnOption & 8) {
            conf_err = 33; // Unsure how to support oddfire 5 cyl
        }
        if ((ram4.spk_mode3 & 0xc0) == 0x40) { // wasted
            if (spkmode != 4) {
                conf_err = 71;
            } else {
                tmp_num_cyl = 10;
                tmp_divider = ram4.Divider << 1;
            }
        }
    }
    num_cyl = tmp_num_cyl;
    divider = tmp_divider;
}

void calc_fuel_factor(void)
{
    unsigned int rail_pressure_abs, rail_pressure_100, sqrt_rp100;
    rail_pressure_abs = ram5.rail_pressure + outpc.baro; // at 0kPa
    rail_pressure_100 = rail_pressure_abs - 1000; // differential pressure at 100kPa

    sqrt_rp100 = int_sqrt32(10000UL * rail_pressure_100);

    // to maintain precision of result, multiply input value by 10000 to get 100x output
    fp_factor[0] = (1000UL * sqrt_rp100) / int_sqrt32(10000UL * rail_pressure_abs); // 0kPa 
    fp_factor[1] = 1000; // 100kPa = 100% by definition
    fp_factor[2] = (1000UL * sqrt_rp100) / int_sqrt32(10000UL * (rail_pressure_abs - 2000)); // 200kPa 
    fp_factor[3] = (1000UL * sqrt_rp100) / int_sqrt32(10000UL * (rail_pressure_abs - 3000)); // 300kPa 
    fp_factor[4] = (1000UL * sqrt_rp100) / int_sqrt32(10000UL * (rail_pressure_abs - 4000)); // 400kPa 
    fpf_baro = outpc.baro;
}

unsigned int lookup_fuel_factor(void)
{
    /* returns fuel flow correction based on MAP for systems with fixed fuel pressure */
    /* running high boost with a fixed pressure system is likely a poor idea */
    if (outpc.map > 4000) { // use 400kPa figure
        return fp_factor[4];
    } else if (outpc.map > 3000) {
        return twoptlookup(outpc.map, 3000, 4000, fp_factor[3], fp_factor[4]);
    } else if (outpc.map > 2000) {
        return twoptlookup(outpc.map, 2000, 3000, fp_factor[2], fp_factor[3]);
    } else if (outpc.map > 1000) {
        return twoptlookup(outpc.map, 1000, 2000, fp_factor[1], fp_factor[2]);
    } else { // <= 100kPa (sensible range)
        return twoptlookup(outpc.map, 0, 1000, fp_factor[0], fp_factor[1]);
    }
}

void hpte(void)
{
    if ((ram4.loadopts & 0x08) && (ram5.hpte_opt & 0x01)) {
        unsigned int time_delta, timer2, lmt;
        lmt = (unsigned int)lmms;
        time_delta = lmt - hpte_time_last;
        hpte_time_last = lmt;

        if ((outpc.fuelload > ram5.hpte_load) && (outpc.rpm > ram5.hpte_rpm)) {
            flagbyte17 |= FLAGBYTE17_HPTE;
            hpte_timer += time_delta;

        } else if (flagbyte17 & FLAGBYTE17_HPTE) {
            hpte_timer -= time_delta;
            if (hpte_timer < 0) {
                hpte_timer = 0;
                flagbyte17 &= ~FLAGBYTE17_HPTE;
                hpte_afr = 0;
            }
        }

        if (flagbyte17 & FLAGBYTE17_HPTE) {
            timer2 = hpte_timer / 781; // into 0.1s units
            RPAGE = tables[25].rpg;
            if (timer2 > ram_window.pg25.hpte_times[5]) {
                timer2 = ram_window.pg25.hpte_times[5];
                hpte_timer = timer2 * 781L;
            }

            hpte_afr = (unsigned char)intrp_1dctable(timer2, 6,
                           (int *)ram_window.pg25.hpte_times, 0,
                           (unsigned char *)ram_window.pg25.hpte_afrs, 25);
        }

    } else { // not on at all
        flagbyte17 &= ~FLAGBYTE17_HPTE;
        hpte_afr = 0;
    }
}

void set_ase(void)
{
    unsigned int av1, av2, ac1, ac2;
    unsigned char w;

    /* ASE */
    if ( (ram5.dualfuel_sw & 0x80) && (ram5.dualfuel_sw & 0x1)
        && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
        w = 3;
    } else if ((ram5.dualfuel_sw & 0x80) && pin_dualfuel && ((*port_dualfuel & pin_dualfuel) == pin_match_dualfuel)) {
        w = 2;
    } else {
        w = 1;
    }

    av1 = 0;
    av2 = 0;
    ac1 = 0;
    ac2 = 0;
    if (w & 1) {
        av1 = (unsigned short) CW_table(outpc.clt, (int *) ram_window.pg8.CWAWEV, (int *) ram_window.pg8.temp_table_p5, 8);        // %
        ac1 = (unsigned short) CW_table(outpc.clt, (int *) ram_window.pg8.CWAWC, (int *) ram_window.pg8.temp_table_p5, 8);  // cycles
    }
    if (w & 2) {
        av2 = (unsigned short) CW_table(outpc.clt, (int *) ram_window.pg21.CWAWEV2, (int *) ram_window.pg21.temp_table_p21, 21);        // %
        ac2 = (unsigned short) CW_table(outpc.clt, (int *) ram_window.pg21.CWAWC2, (int *) ram_window.pg21.temp_table_p21, 21);  // cycles
    }
    if (w == 3) {
        /* Flex blend */
        ase_value = (unsigned int)((((unsigned long)av1 * (100 - flexblend)) + ((unsigned long)av2 * flexblend)) / 100);
        ase_cycles = (unsigned int)((((unsigned long)ac1 * (100 - flexblend)) + ((unsigned long)ac2 * flexblend)) / 100);
    } else if (w == 2) {
        ase_value = av2;
        ase_cycles = ac2;
    } else {
        ase_value = av1;
        ase_cycles = ac1;
    }
}

void calc_fuelflow(void)
/* calculate fuel flow */
{
    unsigned int ff_lmms, t;
    ff_lmms = (unsigned int)lmms;
    t = ff_lmms - ff_lmms_last;
    if (t >= ram5.fuelcalctime) { // user settable interval
        unsigned long ft, flowsum[2], div;
        unsigned int inj1, inj2;

        ff_lmms_last = ff_lmms;
        DISABLE_INTERRUPTS;
        flowsum[0] = flowsum_accum[0];
        flowsum[1] = flowsum_accum[1];
        flowsum_accum[0] = 0;
        flowsum_accum[1] = 0;
        ENABLE_INTERRUPTS;

        if (ram4.hardware & HARDWARE_MS3XFUEL) {
            inj1 = ram4.staged_pri_size;
            if (glob_sequential & SEQ_SEMI) {
                inj1 *= 2;
            }
            if (ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3) {
                inj2 = (ram4.staged_sec_size * ram4.NoInj) / 2;
            } else {
                inj2 = ram4.staged_sec_size;
                if (glob_sequential & SEQ_SEMI) {
                    inj2 *= 2;
                }
            }
        } else {
            inj1 = (ram4.staged_pri_size * ram4.NoInj) / 2;
            inj2 = (ram4.staged_sec_size * ram4.NoInj) / 2;
        }

        /* correct for sampling interval - calcs assume 15625 tick (2.0s) interval */
        div = (1000UL * t) / 15625; // when t = 15625, div = 1000

        ft = ((flowsum[0] / 200) * inj1) / div;
        ft += ((flowsum[1] / 200) * inj2) / div;

        if (flow_int == 0) {
            flow_int = ft;
        } else {
            /* apply 30% lag factor */
            __asm__ __volatile__
            ("subd %2\n"
             "tfr d,y\n"
             "clra\n"
             "ldab #30\n" // it is a uchar
             "emuls\n"
             "ldx #100\n"
             "edivs\n"
             "addy %2\n"
            :"=y"(flow_int)
            :"d"((unsigned int)ft), "m"(flow_int)
            :"x");
        }
        outpc.fuelflow = flow_int / 10; // internal value is *10 for better smoothing
    }
}

