/* $Id: ms3_misc.c,v 1.500.4.13 2015/06/03 12:26:06 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * get_adc()
    Origin: Al Grippo
    Major:  James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * barocor_eq()
    Origin: Al Grippo
    Majority: Al Grippo
 * CW_table()
    Origin: Al Grippo
    Minor: James Murray
    Majority: Al Grippo
 * set_spr_port()
    Trace: Al Grippo
    Major: Rewrite. James Murray
    Majority: James Murray
 * Flash_Init()
    Trace: Al Grippo
    Major: Rewrite. James Murray
    Majority: James Murray
 * realtime()
    Origin: James Murray
    Majority: James Murray
 * twoptlookup()
    Origin: James Murray
    Majority: James Murray
 * dribble_burn()
    Origin: James Murray
    Majority: James Murray
 * wheel_fill_map_event_array
    Origin: Kenneth Culver
    Moderate: James Murray
    Majority: Kenneth Culver / James Murray
 * speed_sensors()
    Origin: James Murray
    Majority: James Murray
 * nitrous()
    Origin: James Murray
    Majority: James Murray
 * median3pt
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * sample_map_tps()
    Origin: Al Grippo
    Major:  James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * calc_ITB_load()
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * calc_baro_mat_load()
    Origin: Al Grippo
    Major:  James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * water_inj()
    Origin: James Murray
    Majority: James Murray
 * do_launch()
    Origin: James Murray
    Majority: James Murray
 * transbrake()
    Origin: James Murray
    Majority: James Murray
 * do_revlim_overboost_maxafr
    Origin: Kenneth Culver
    Moderate: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * handle_ovflo()
    Origin: Kenneth Culver
    Moderate: MS3. James Murray
    Majority: James Murray / Kenneth Culver
 * handle_spareports()
    Origin: Al Grippo
    Minor: MS3. James Murray
    Majority: Al Grippo
 * do_egt()
    Origin: James Murray
    Majority: James Murray
 * do_sensors()
    Origin: James Murray
    Majority: James Murray
 * gearpos()
    Origin: James Murray
    Majority: James Murray
 * calcvssdot()
    Origin: James Murray
    Majority: James Murray
 * accelerometer()
    Origin: James Murray
    Majority: James Murray
 * ck_log_clr()
    Origin: James Murray
    Majority: James Murray
 * long_abs()
    Origin: James Murray
    Majority: James Murray
 * shifter()
    Origin: James Murray
    Majority: James Murray
 * generic_pwm()
    Origin: James Murray
    Majority: James Murray
 * generic_pwm_out()
    Origin: James Murray
    Majority: James Murray
 * poll_i2c_rtc()
    Origin: James Murray
    Majority: James Murray
 * antilag()
    Origin: James Murray
    Majority: James Murray
 * vvt_ctl_pid_init()
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * vvt_pid()
    Origin: Kenneth Culver
    Majority: Kenneth Culver
 * vvt()
    Origin:  Kenneth Culver / James Murray
    Majority: Kenneth Culver / James Murray
 * tclu()
    Origin: James Murray
    Majority: James Murray
 * traction()
    Origin: James Murray
    Majority: James Murray
 * check_sensors(), check_sensors_reset(), check_sensors_init()
    Origin: James Murray
    Majority: James Murray
 * do_spi2()
    Origin: James Murray
    Majority: James Murray
 * ckstall(), clear_all()
    Origin: James Murray (was in ASM)
    Majority: James Murray
 * fuelpump*()
    Origin: James Murray
    Majority: James Murray
 * fuel_sensors()
    Origin: James Murray
    Majority: James Murray
 * alternator()
    Origin: James Murray
    Majority: James Murray
    Minorty: Kenneth Culver
 * shiftlight()
    Origin: James Murray
    Majority: James Murray
 * oilpress()
    Origin: James Murray
    Majority: James Murray
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#include "ms3.h"

/**************************************************************************
 **
 ** Read main sensor inputs
 **
 **************************************************************************/
void get_adc(char chan1, char chan2)
{
    char chan;
    //long adcval;
    int adcvalv, tmp1, tmp3;
    unsigned int tmpadc;

    for (chan = chan1; chan <= chan2; chan++) {
        //    switch(chan)  {
        // when using switch, gcc puts lookup table around 0x5000 and then linker
        // gets all upset because we are in page 0x3c and jump table isn't. Replace
        // with ifs instead. No functional difference. This is known bug in gcc
        if ((chan == 0) && (flagbyte20 & FLAGBYTE20_USE_MAP)) {
            unsigned int mapadc;
            mapadc = *mapport;

            //removed lag code and first_adc comparison as only called here from init
            __asm__ __volatile__("ldd    %1\n"
                                 "subd   %2\n"
                                 "ldy    %3\n"
                                 "emul\n"
                                 "ldx    #1023\n"
                                 "ediv\n" "addy   %2\n":"=y"(outpc.map)
                                 :"m"(ram4.mapmax), "m"(ram4.map0),
                                 "m"(mapadc)
                                 :"d", "x");

        } else if ((chan == 1) && (!burnstat) && (!ltt_fl_state)) {
            int i;
            unsigned int avg_adc;
            // sliding window
            avg_adc = 0;
            for (i = MAT_N - 1 ; i > 0 ; i--) {
                mat_data[i] = mat_data[i-1];
                avg_adc += mat_data[i];
            }
            mat_data[0] = ATD0DR1;
            avg_adc += mat_data[0];

            if (first_adc) { // fill with same value
                for (i = MAT_N - 1 ; i > 0 ; i--) {
                    mat_data[i] = mat_data[0];
                }
                avg_adc = mat_data[0];
            } else {
                avg_adc /= MAT_N;
            }

            //        adcval = (long)ram4.mat0 +
            //          ((long)ram4.matmult * matfactor_table[ATD0DR1]) / 100; // deg F or C x 10
            GPAGE = 0x10;
            __asm__ __volatile__("aslx\n" "addx   #0x4800\n"    // matfactor_table address
                                 "gldy   0,x\n"
                                 "ldd    %2\n"
                                 "emul\n"
                                 "ldx    #100\n"
                                 "ediv\n" "addy   %3\n":"=y"(tmp1)
                                 :"x"(avg_adc), "m"(ram4.matmult),
                                 "m"(ram4.mat0)
                                 :"d");

            if (first_adc) {
                outpc.mat = tmp1;
            } else {
                //          outpc.mat += (short)((ram4.adcLF * (adcval - outpc.mat)) / 100);
                __asm__ __volatile__(
                                        //                "ldd    %1\n"
                                        "subd   %3\n" "tfr    d,y\n" "clra\n" "ldab    %2\n"    // it is a uchar
                                        "emuls\n"
                                        "ldx     #100\n"
                                        "edivs\n"
                                        "addy    %3\n":"=y"(outpc.mat)
                                        :"d"(tmp1), "m"(ram4.adcLF),
                                        "m"(outpc.mat)
                                        :"x");
            }

            if (stat_mat && (ram5.cel_action1 & 0x02)) { /* MAT sensor is broken */
                outpc.mat = ram5.cel_mat_default;
            }


        } else if ((chan == 2) && (!burnstat) && (!ltt_fl_state)) {
            int i;
            unsigned int avg_adc;
            // sliding window
            avg_adc = 0;
            for (i = CLT_N - 1 ; i > 0 ; i--) {
                clt_data[i] = clt_data[i-1];
                avg_adc += clt_data[i];
            }
            clt_data[0] = ATD0DR2;
            avg_adc += clt_data[0];

            if (first_adc) { // fill with same value
                for (i = CLT_N - 1 ; i > 0 ; i--) {
                    clt_data[i] = clt_data[0];
                }
                avg_adc = clt_data[0];
            } else {
                avg_adc /= CLT_N;
            }
            //        adcval = (long)ram4.clt0 +
            //          ((long)ram4.cltmult * cltfactor_table[ATD0DR2]) / 100; // deg F or C x 10
            GPAGE = 0x10;
            __asm__ __volatile__("aslx\n" "addx   #0x4000\n"    // cltfactor_table address
                                 "gldy   0,x\n"
                                 "ldd    %2\n"
                                 "emul\n"
                                 "ldx    #100\n"
                                 "ediv\n" "addy   %3\n":"=y"(tmp1)
                                 :"x"(avg_adc), "m"(ram4.cltmult),
                                 "m"(ram4.clt0)
                                 :"d");
            if (first_adc)
                outpc.clt = tmp1;
            else {
                //          outpc.clt += (short)((ram4.adcLF * (adcval - outpc.clt)) / 100);
                __asm__ __volatile__("ldd    %1\n" "subd   %3\n" "tfr    d,y\n" "clra\n" "ldab    %2\n" // it is a uchar
                                     "emuls\n"
                                     "ldx     #100\n"
                                     "edivs\n"
                                     "addy    %3\n":"=y"(outpc.clt)
                                     :"m"(tmp1), "m"(ram4.adcLF),
                                     "m"(outpc.clt)
                                     :"d", "x");
            }

            if (stat_clt && (ram5.cel_action1 & 0x04)) { /* CLT sensor is broken */
                if (outpc.seconds > ram5.cel_warmtime) {
                    outpc.clt = ram5.cel_clt_warm;
                } else {
                    /* interpolate a faked warmup */
                    outpc.clt = ram5.cel_clt_cold + (((ram5.cel_clt_warm - ram5.cel_clt_cold) * (long)outpc.seconds) / ram5.cel_warmtime);
                } 
            }
        } else if (chan == 3) {
            // only used by init?
            outpc.tpsadc = ATD0DR3;
            if (first_adc) {
                /* fill ring */
                tps_ring[0] = outpc.tpsadc;
                tps_ring[1] = outpc.tpsadc;
                tps_ring[2] = outpc.tpsadc;
                tps_ring[3] = outpc.tpsadc;
                tps_ring[4] = outpc.tpsadc;
                tps_ring[5] = outpc.tpsadc;
                tps_ring[6] = outpc.tpsadc;
                tps_ring[7] = outpc.tpsadc;

                __asm__ __volatile__
                ("ldd %3\n"
                 "subd %2\n"
                 "tfr  d,y\n"
                 "ldd  %1\n"
                 "subd %2\n"
                 "pshd\n"
                 "ldd #1000\n"
                 "emuls\n"
                 "pulx\n"
                 "edivs\n"
                 :"=y"(outpc.tps)
                 :"m"(ram4.tpsmax), "m"(tps0_auto), "m"(ATD0DR3)
                 :"d", "x");
            }
        } else if (chan == 4) {
            int i;
            unsigned int avg_adc;
            // sliding window
            avg_adc = 0;
            for (i = BATT_N - 1 ; i > 0 ; i--) {
                batt_data[i] = batt_data[i-1];
                avg_adc += batt_data[i];
            }
            batt_data[0] = ATD0DR4;
            avg_adc += batt_data[0];

            if (first_adc) { // fill with same value
                for (i = BATT_N - 1 ; i > 0 ; i--) {
                    batt_data[i] = batt_data[0];
                }
                avg_adc = batt_data[0];
            } else {
                avg_adc /= BATT_N;
            }
            //        adcval = (long)ram4.batt0 +
            //          ((long)(ram4.battmax - ram4.batt0) * ATD0DR4) / 1023; // V x 10
            __asm__ __volatile__("ldd    %1\n"
                                 "subd   %2\n"
                                 "ldy    %3\n"
                                 "emul\n"
                                 "ldx    #1023\n"
                                 "ediv\n"
                                 "addy   %2\n":"=y"(tmp1)
                                 :"m"(ram4.battmax), "m"(ram4.batt0), "m"(avg_adc)
                                 :"d", "x");
            if (first_adc)
                outpc.batt = tmp1;
            else {
                //        outpc.batt += (short)((ram4.adcLF * (adcval - outpc.batt)) / 100);
                __asm__ __volatile__("ldy    %1\n" "suby   %3\n" "clra\n" "ldab    %2\n"        // it is a uchar
                                     "emuls\n"
                                     "ldx     #100\n"
                                     "edivs\n"
                                     "addy    %3\n":"=y"(outpc.batt)
                                     :"m"(tmp1), "m"(ram4.adcLF),
                                     "m"(outpc.batt)
                                     :"x", "d");
            }
        } else if ((chan == 5) && (!burnstat) && (!ltt_fl_state)) {
            int start, egonum, ix;
            // do all EGO sensors in here.

            if (ram4.EgoOption & 0x03) {
                egonum = ram4.egonum;
            } else {
                egonum = 1;
            }

            start = 0;

            for (ix = start; ix < egonum ; ix++) {
                tmpadc = *egoport[ix];

                // check if sensor bad (near limits)
                if ((tmpadc < 3) || (tmpadc > 1020)) {
                    bad_ego_flag |= (0x01 << ix);
                } else {
                    bad_ego_flag &= ~(0x01 << ix);
                }
                
                if ((ram5.egoport[ix] & 0x1f) == 7) {
                    // presently only Innovate 0.5 - 1.523 lambda = 0 to 1023
                    // counts supported convert to AFR
                    unsigned long ul;

                    if (tmpadc > 1023) {
                        tmpadc = 1023;
                    }

                    ul = (tmpadc + 500) * (unsigned long)ram4.stoich;
                    tmp3 = (ul + 500) / 1000;

                    adcvalv = 0; // not used

                } else {

                    GPAGE = 0x10;
                    __asm__ __volatile__
                    ("addx   #0x5000\n"     //egofactor_table address
                     "gldab 0,x\n"
                     "clra\n"
                     "ldy    %2\n"
                     "emul\n"
                     "ldx    #100\n"
                     "ediv\n"
                     "addy   %3\n"
                     :"=y"(tmp3)
                     :"x"(tmpadc), "m"(ram4.egomult), "m"(ram4.ego0)
                     :"d");

                    __asm__ __volatile__
                    ("ldd #250\n"
                     "emul\n"
                     "ldx #1023\n"
                     "ediv\n"
                     :"=y"(adcvalv)
                     :"y"(tmpadc)
                     :"d", "x");
                }

                if ((first_adc) 
                   || (((ram5.egoport[ix] & 0x1f) == 7) && ((ram4.can_poll2 & CAN_POLL2_EGOLAG) == 0))) {
                    /* First time through or CAN EGO on this channel and smoothing off. */
                    outpc.afr[ix] = tmp3;
                    outpc.egov[ix] = tmpadc;
                } else {
                    int tmp2, tmp4;
// lag factor on AFR
                    tmp4 = outpc.afr[ix];
                    //            outpc.ego1 += (short)((ram4.egoLF * (adcval - outpc.ego1)) / 100);
                    __asm__ __volatile__
                    ("ldy    %1\n"
                     "suby   %3\n"
                     "clra\n"
                     "ldab    %2\n"        // it is a uchar
                     "emuls\n"
                     "ldx     #100\n"
                     "edivs\n"
                     "addy    %3\n"
                     :"=y"(outpc.afr[ix])
                     :"m"(tmp3), "m"(ram4.egoLF),"m"(tmp4)
                     :"x", "d");
// lag factor on EGO volts
                    tmp1 = (int) ram4.egoLF;
                    tmp2 = tmpadc - (int)outpc.egov[ix];
                    __asm__ __volatile__
                    ("emuls\n"
                     "ldx #100\n"
                     "edivs\n"
                     :"=y"(adcvalv)
                     :"d"(tmp1), "y"(tmp2)
                     :"x");

                    tmp4 = (int)outpc.egov[ix] + adcvalv; // Vx100
                    outpc.egov[ix] = tmp4;
                }
            }
            // for compatability
            outpc.ego1 = outpc.afr[0];
            outpc.ego2 = outpc.afr[1];
            outpc.egoV1 = outpc.egov[0];
            outpc.egoV2 = outpc.egov[1];
        }                       // end of switch
    }                           // end of for loop

    // if GPIO slave copy these raw ADCs to a convenient place so master can grab them
    if (CANid) {
        outpc.sensors[0] = ATD0DR0;
        outpc.sensors[1] = ATD0DR1;
        outpc.sensors[2] = ATD0DR2;
        outpc.sensors[3] = ATD0DR3;
        outpc.sensors[4] = ATD0DR4;
        outpc.sensors[5] = ATD0DR5;
        outpc.sensors[6] = ATD0DR6;
        outpc.sensors[7] = ATD0DR7;
    }
    // now calculate other stuff that uses optional ADC inputs
    if ((ram4.BaroOption & 0x03) == 2) {

       tmpadc = *baroport; 

        //          adcval = (long)ram4.baro0 +
        //            ((long)(ram4.baromax - ram4.baro0) * tmpadc) / 1023; // kPa x 10
        __asm__ __volatile__("ldd    %1\n"
                             "subd   %2\n"
                             "ldy    %3\n"
                             "emul\n"
                             "ldx    #1023\n"
                             "ediv\n" "addy   %2\n":"=y"(tmp1)
                             :"m"(ram4.baromax), "m"(ram4.baro0),
                             "m"(tmpadc)
                             :"x", "d");

        if (first_adc) {
            outpc.baro = tmp1;
        } else {
            //            outpc.baro += (short)((ram4.adcLF * (adcval - outpc.baro)) / 100);
            __asm__ __volatile__("ldy    %1\n" "suby   %3\n" "clra\n" "ldab    %2\n"    // it is a uchar
                                 "emuls\n"
                                 "ldx     #100\n"
                                 "edivs\n" "addy    %3\n":"=y"(outpc.baro)
                                 :"m"(tmp1), "m"(ram4.adcLF),
                                 "m"(outpc.baro)
                                 :"x", "d");
        }
        if (outpc.baro < ram4.baro_lower) {
            outpc.baro = ram4.baro_lower;
        } else if (outpc.baro > ram4.baro_upper) {
            outpc.baro = ram4.baro_upper;
        }
    }

    return;
}

int barocor_eq(int baro)
{
    // returns baro correction in 0.1% (100 is no correction)
    // baro in kPa x 10
    return ((int)
            (ram4.bcor0 * 10 + (((long) ram4.bcormult * baro) / 100)));
}

int CW_table(int clt, int *table, int *temp_table, unsigned char cwpage)
{
    int ix;
    long interp, interp3;

    RPAGE = tables[cwpage].rpg;
    // returns values for various cold warmup table interpolations
    // bound input arguments
    if (clt > temp_table[NO_TEMPS - 1]) {
        return (table[NO_TEMPS - 1]);
    }
    if (clt < temp_table[0]) {
        return (table[0]);
    }
    for (ix = NO_TEMPS - 2; ix > -1; ix--) {
        if (clt > temp_table[ix]) {
            break;
        }
    }
    if (ix < 0)
        ix = 0;

    interp = temp_table[ix + 1] - temp_table[ix];
    if (interp != 0) {
        interp3 = (clt - temp_table[ix]);
        interp3 = (100 * interp3);
        interp = interp3 / interp;
    }
    return ((int)
            (table[ix] + interp * (table[ix + 1] - table[ix]) / 100));
}

//*****************************************************************************
// Function to set clear output port bit based
// port = software port no.
// lookup actual hardware port in table
// ports 0 = PM, 1 = PJ, 2 = PP, 3 = PT, 4 = PA, 5 = PB, 6 = PK
// val = 0 or 1 for off/on
//*****************************************************************************
void set_spr_port(char port, char val)
{
    unsigned char hw_mask, *hw_addr;

    hw_mask = spr_port_hw[2][(int) port];
    hw_addr = (unsigned char *) spr_port_addr[(int) port];

// wrap XGATE semaphore around here in case a port is shared with XGATE

    if (val) {
        SSEM0SEI;
        *hw_addr |= hw_mask;
        CSEM0CLI;
    } else {
        SSEM0SEI;
        *hw_addr &= ~hw_mask;
        CSEM0CLI;
    }
}

//*****************************************************************************
//* Function Name: Flash_Init
//* Description : Initialize Flash NVM for HCS12 by programming
//* FCLKDIV based on passed oscillator frequency, then
//* uprotect the array, and finally ensure PVIOL and
//* ACCERR are cleared by writing to them.
//*
//*****************************************************************************
void Flash_Init()
{
    if (!(FSTAT & 0x80)) {
        conf_err = 50;          // flash busy error - should not be busy!
    } else {
        /* Next, initialize FCLKDIV register to ensure we can program/erase */
        if ((FCLKDIV & 0x80) == 0) {    // if not already set, then set it.
            FCLKDIV = 7;        // for 8MHz crystal OSCCLK
        }
        //  FPROT = 0xFF; /* Disable all protection (only in special modes)*/
        // commented as it will likely spew out error normally
        FSTAT = 0x30;  // clear ACCERR or FPVIOL if set
    }
    return;
}

int twoptlookup(unsigned int x, unsigned int x0, unsigned int x1,
            int y0, int y1)
{
    long interp, interp3;
    int result;

    // bound input arguments
    if (x >= x1) {
        return (y1);
    }

    if (x <= x0) {
        return (y0);
    }

    interp = (long) x1 - (long) x0;
    interp3 = ((long) x - (long) x0);
    interp3 = (100 * interp3);
    interp = interp3 / interp;

    interp = interp * ((long) y1 - (long) y0) / 100;
    result = (int) ((long) y0 + interp);

    return (result);
}

void dribble_burn()
{                               /* dribble burning */
    if ((burnstat) && (flocker == 0xdd)) {
        if ((burnstat > 1) && ((FSTAT & 0x80) == 0)) {
            goto DB_END;        // previous flash command not completed, so skip
        }

        if ((burnstat >= 1) && (burnstat <= 4)) {
            if (burnstat == 1) {
                Flash_Init();
            }
            //erase all d-flash sectors corresponding to this data page
            // first identify the global address for the first sector
            unsigned int ad150;
            ad150 = (unsigned int) tables[burn_idx].addrFlash;

            if ((tables[burn_idx].n_bytes == 0)
                || (tables[burn_idx].n_bytes > 1024) || (ad150 > 0x8000)) {
                //invalid or unrecognised page sent here (not foolproof detection)
                burnstat = 0;
                flocker = 0;
                goto DB_END;
            }
            ad150 += (burnstat - 1) * 0x100;
            // erase the specified flash block
            DISABLE_INTERRUPTS;
            if ((FSTAT & 0x30) != 0) {
                FSTAT = 0x30;  // clear ACCERR or FPVIOL if set
            }

            FCCOBIX = 0;
            FCCOBHI = 0x12;     // erase D-flash sector command
            FCCOBLO = 0x10;     // global address of all D-flash

            FCCOBIX = 1;
            FCCOB = ad150;      // address within sector

            FSTAT = 0x80;       // initiate flash command (datasheet very confusing)
            ENABLE_INTERRUPTS;

            burnstat++;

        } else if (burnstat >= 300) {
            /* erasefactor for calibration tables */
            flocker = 0xcc;
            /* This busy waits. Might be cleaner to handle a sector at a time,
                but not harmful as calibration tables should be burned with the
                engine stationary. */
            erasefactor();
            burnstat = 5; // carry on as normal on next pass
            flocker = 0xdd;

        } else if (burnstat >= 5) {
            // write data to flash page, a word at a time
            unsigned char rp;
            unsigned int ad150, *ad_loc;

            ad150 = tables[burn_idx].addrFlash;

            if ((tables[burn_idx].n_bytes == 0)
                || (tables[burn_idx].n_bytes > 2048) || (ad150 >= 0x8000)) {
                //invalid or unrecognised page sent here (not foolproof detection)
                burnstat = 0;
                flocker = 0;
                goto DB_END;
            }
            rp = tables[burn_idx].rpg;
            // sanity check in case of corruption
            if (rp < 0xf0) {
                // how did this happen?
                burnstat = 0;
                flocker = 0;
                goto DB_END;
            }
            ad_loc = (unsigned int *) (tables[burn_idx].adhi << 8);
            ad150 += (burnstat - 5) * 8;        // we prog 8 bytes at a time
            ad_loc += (burnstat - 5) * 4;       // only 4 because uint so gets doubled

            if ((FSTAT & 0x30) != 0) {
                FSTAT = 0x30;  // clear ACCERR or FPVIOL if set
            }

            DISABLE_INTERRUPTS;
            RPAGE = rp;

            if (ad150 >= 0x8000) {
                // how did this happen?
                burnstat = 0;
                flocker = 0;
                goto DB_END;
            }

            FCCOBIX = 0;
            FCCOBHI = 0x11;     // prog D-flash
            FCCOBLO = 0x10;     // global addr

            FCCOBIX = 1;
            FCCOB = ad150;      //global addr

            FCCOBIX = 2;
            FCCOB = *ad_loc;
            ad_loc++;           // (words)

            FCCOBIX = 3;
            FCCOB = *ad_loc;
            ad_loc++;

            FCCOBIX = 4;
            FCCOB = *ad_loc;
            ad_loc++;

            FCCOBIX = 5;
            FCCOB = *ad_loc;

            FSTAT = 0x80;       // initiate flash command
            ENABLE_INTERRUPTS;

            if ((((burn_idx >= 4) || (burn_idx == 2)) && (burnstat < 132))
                || (((burn_idx < 2) || (burn_idx == 3)) && (burnstat < 260))) {
                burnstat++;
            } else {
                // finished
                burnstat = 0;
                flocker = 0;
                outpc.status1 &= ~(STATUS1_NEEDBURN + STATUS1_LOSTDATA);        // clear NeedBurn,LostData on burn completion
                if (burn_idx == 3) { // wrote the old MAF curve, update the RAM version.
                    flagbyte21 |= FLAGBYTE21_POPMAF;
                }
            }
        }
    } else if (((burnstat == 0) && (flocker == 0xdd)) || ((burnstat) && (flocker != 0xdd))) {
        // shouldn't happen
        burnstat = 0;
        flocker = 0;
        // might want to kill serial too
    }
  DB_END:;
}


void ltt_burn()
{                               /* dribble burning */
    if (ltt_fl_state) {
        if ((ltt_fl_state > 1) && ((FSTAT & 0x80) == 0)) {
            return;        // previous flash command not completed, so skip
        }

        if ((ltt_fl_state >= 1) && (ltt_fl_state <= 4)) {
            if (ltt_fl_state == 1) {
                Flash_Init();
            }
            // erase the one d-flash sector

            // erase the specified flash block
            DISABLE_INTERRUPTS;
            if ((FSTAT & 0x30) != 0) {
                FSTAT = 0x30;  // clear ACCERR or FPVIOL if set
            }

            FCCOBIX = 0;
            FCCOBHI = 0x12;     // erase D-flash sector command
            FCCOBLO = 0x10;     // global address of all D-flash

            FCCOBIX = 1;
            if (ltt_fl_ad == 0x900) {
                FCCOB = 0x6400; // start of global sector
            } else {
                FCCOB = 0x6500;
            }

            FSTAT = 0x80;       // initiate flash command
            ENABLE_INTERRUPTS;

            ltt_fl_state = 5;

        } else if (ltt_fl_state >= 5) {
            // write data to flash page, a word at a time
            unsigned int ad150, ad_loc;

            if (ltt_fl_ad == 0x900) {
                ad150 = 0x6400; // start of global sector
            } else {
                ad150 = 0x6500;
            }
            ad_loc = (ltt_fl_state - 5) * 8; // we prog 8 bytes at a time
            ad150 += ad_loc;        
            ad_loc += (unsigned int)&ram_window.trimpage.ltt_table1[0][0]; // starts at ram address of table

            if ((FSTAT & 0x30) != 0) {
                FSTAT = 0x30;  // clear ACCERR or FPVIOL if set
            }

            DISABLE_INTERRUPTS;
            RPAGE = tables[25].rpg;

            FCCOBIX = 0;
            FCCOBHI = 0x11;     // prog D-flash
            FCCOBLO = 0x10;     // global addr

            FCCOBIX = 1;
            FCCOB = ad150;      //global addr

            FCCOBIX = 2;
            FCCOB = *(unsigned int*)ad_loc;
            ad_loc += 2; // words

            FCCOBIX = 3;
            FCCOB = *(unsigned int*)ad_loc;
            ad_loc += 2;

            FCCOBIX = 4;
            FCCOB = *(unsigned int*)ad_loc;
            ad_loc += 2;

            FCCOBIX = 5;
            if (ltt_fl_state == 36) { // top RH corner of table
                unsigned char c;
                EPAGE = 0x19;
                if (ltt_fl_ad == 0x800) {
                    c = *(unsigned char*)0x8ff + 1;
                } else {
                    c = *(unsigned char*)0x9ff + 1;
                }
                c &= 0x7f;
                FCCOB = (*(unsigned int*)ad_loc & 0xff00) | c;
            } else {
                FCCOB = *(unsigned int*)ad_loc;
            }

            FSTAT = 0x80;       // initiate flash command
            ENABLE_INTERRUPTS;

            if (ltt_fl_state < 36) {
                ltt_fl_state++;
            } else {
                // finished
                if (ltt_fl_ad == 0x800) {
                    ltt_fl_ad = 0x900;
                } else {
                    ltt_fl_ad = 0x800;
                }
                ltt_fl_state = 0;
                if (pin_ltt_led) {
                    SSEM0SEI;
                    *port_ltt_led &= ~pin_ltt_led; // turn off LED
                    CSEM0CLI;
                }
            }
        }
    }
}


void wheel_fill_map_event_array(map_event * map_events_fill, int map_ang,
                                ign_time last_tooth_time,
                                unsigned int last_tooth_ang, unsigned int duration)
{
    char iterate, tth, wfe_err;
    int tth_ang, tmp_ang, i;
    unsigned int map_time, map_window_time;

    if (map_ang < 0) {
        map_ang += cycle_deg;
    }

    map_window_time =
        (unsigned
         int) (((duration * last_tooth_time.time_32_bits) /
                last_tooth_ang) / 128L);
    if (map_window_time < 1) {
        map_window_time = 1;
    }

    for (i = 0; i < no_triggers; i++) {
        wfe_err = 0;
        iterate = 0;
        tth_ang = trig_angs[i];
        tth = trigger_teeth[i];
        while (!iterate) {
            if (tth_ang > map_ang) {
                iterate = 1;
            } else {
                //how far do we step back in deg
                tth--;
                if (tth < 1) {
                    tth = last_tooth;
                    wfe_err++;
                    if (wfe_err > 2) { /* allow twice around loop, was 1 */
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
            debug_str("2. Failed to find tooth.\r");
            // can't continue as didn't find a valid tooth
            return;
        }

        tmp_ang = tth_ang - map_ang;

        map_time =
            (unsigned
             int) (((tmp_ang * last_tooth_time.time_32_bits) /
                    last_tooth_ang) / 128L);
        wfe_err = 0;

        while ((wfe_err < 2) && (map_time < 1)) {
            // too soon after tooth, need to step back
            tth--;
            if (tth < 1) {
                tth = last_tooth;
                wfe_err++;
            }
            tth_ang += deg_per_tooth[tth - 1];
            // recalc
            tmp_ang = tth_ang - map_ang;
            map_time =
                (unsigned
                 int) (((tmp_ang * last_tooth_time.time_32_bits) /
                        last_tooth_ang) / 128L);
        }

        if (wfe_err > 1) {
            DISABLE_INTERRUPTS;
            asm("nop\n");       // something screwed up, place for breakpoint
            ENABLE_INTERRUPTS;
            debug_str("1. Failed to find tooth.\r");
            // can't continue as didn't find a valid tooth
            return;
        }

        map_events_fill[i].tooth = tth;
        map_events_fill[i].time = map_time;
        map_events_fill[i].map_window_set = map_window_time;
        map_events_fill[i].evnum = i;
    }
    /* special workaround for 2 cyl odd wasted*/
    if (((ram4.no_cyl & 0x1f) == 2) && (ram4.ICIgnOption & 0x8) && ((ram4.spk_mode3 & 0xc0) == 0x40) && (no_triggers == 4)) {
        /* use the later of the two close events only, otherwise get two pairs of events that considerably overlap */
        map_events_fill[1].tooth = map_events_fill[2].tooth;
        map_events_fill[1].time = map_events_fill[2].time;
        map_events_fill[1].map_window_set = map_events_fill[2].map_window_set;

        map_events_fill[2].tooth = map_events_fill[0].tooth;
        map_events_fill[2].time = map_events_fill[0].time;
        map_events_fill[2].map_window_set = map_events_fill[0].map_window_set;

        map_events_fill[3].tooth = map_events_fill[1].tooth;
        map_events_fill[3].time = map_events_fill[1].time;
        map_events_fill[3].map_window_set = map_events_fill[1].map_window_set;
    }
}

void speed_sensors()
{
    unsigned int vss_sample_time, tmp_speed;
    unsigned char dl;
    // VSS code. Derived from JSM Microsquirt trans code

    // Adaptive averaging on VSS
    if (outpc.vss1 > 20) {
        vss_sample_time = 15000 / outpc.vss1;
    } else {
        vss_sample_time = 750;
    }

    /* Use this timing for analogue and CAN based only.
        For digi inputs, the sample flag is set in the capture code. */
    if (((unsigned int)lmms - vss_time) > vss_sample_time) {
        vss_time = (unsigned int)lmms;
        if (((flagbyte24 & FLAGBYTE24_IOVSS1) == 0) &&
            (((ram4.vss_opt0 & 0x03) == 0x02) || (((ram4.vss_opt0 & 0x03) == 0x01) && ((ram4.vss_opt & 0x0f) >= 0x0e)))) {
            flagbyte8 |= FLAGBYTE8_SAMPLE_VSS1;
        }
        if (((flagbyte24 & FLAGBYTE24_IOVSS2) == 0) &&
            (((ram4.vss_opt0 & 0x0c) == 0x08) || (((ram4.vss_opt0 & 0x0c) == 0x04) && ((ram4.vss_opt & 0xf0) >= 0xe0)))) {
            flagbyte8 |= FLAGBYTE8_SAMPLE_VSS2;
        }
        if (((flagbyte24 & FLAGBYTE24_IOVSS3) == 0) &&
            ((ram4.vss_opt0 & 0x10)
            && (((ram4.vss_opt0 & 0x03) == 0x02) || (((ram4.vss_opt0 & 0x03) == 0x01) && ((ram5.vss_opt34 & 0x0f) >= 0x0e))))) {
            flagbyte19 |= FLAGBYTE19_SAMPLE_VSS3;
        }
        if (((flagbyte24 & FLAGBYTE24_IOVSS4) == 0) &&
            ((ram4.vss_opt0 & 0x40)
            && (((ram4.vss_opt0 & 0x0c) == 0x08) || (((ram4.vss_opt0 & 0x0c) == 0x04) && ((ram5.vss_opt34 & 0xf0) >= 0xe0))))) { 
            flagbyte19 |= FLAGBYTE19_SAMPLE_VSS4;
        }
    }

    /*       VSS1       */
    dl = 0;
    if ((ram4.vss_opt0 & 0x03) == 2) {
        if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS1) {
            flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS1;
            tmp_speed =
                (unsigned int) (*(unsigned int *) &(*port_vss1) *
                                (unsigned long) ram4.vss1_an_max /
                                (unsigned long) 1023);
            dl = 1;
        }
    } else if (((ram4.vss_opt0 & 0x03) == 1) && (pin_vss1 || (flagbyte24 & FLAGBYTE24_IOVSS1))) {
        if (vss1_stall < VSS_STALL_TIMEOUT) {
            if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS1) {
                unsigned int tmp_vss_teeth, tmp_vss_time_sum;
                DISABLE_INTERRUPTS;
                tmp_vss_teeth = vss1_teeth;
                tmp_vss_time_sum = vss1_time_sum;
                vss1_teeth = 0;
                vss1_time_sum = 0;
                flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS1;
                ENABLE_INTERRUPTS;

                if (tmp_vss_time_sum > 0) {
                    tmp_speed = (unsigned int) ((vss1_coeff * tmp_vss_teeth) / tmp_vss_time_sum);
                    dl = 1;
                }
            }
        } else {
            vss1_stall = VSS_STALL_TIMEOUT + 1; // keep it from rolling over
            outpc.vss1 = 0;
            vss1_time = 0;
            vss1_st = VSS_SKIP_TEETH;
        }
    } else if (((ram4.vss_opt0 & 0x03) == 1) && ((ram4.vss_opt & 0x0f) == 0x0e)) {
        unsigned int tmp_vss;

        if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS1) {
            flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS1;
            if (ram4.vss_can_size & 0x01) { // 16 bit
                tmp_vss = datax1.vss1_16;
            } else {
                tmp_vss = datax1.vss1_8;
            }

            tmp_speed = (unsigned int)(((unsigned long)tmp_vss * (unsigned long)ram4.vss1_can_scale) / 1000L);
            dl = 1 ;
        }
    } else if (((ram4.vss_opt0 & 0x03) == 1) && ((ram4.vss_opt & 0x0f) == 0x0f)) {
        unsigned long tmppwm;

        if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS1) {
            flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS1;
            if (ram4.enable_poll & 0x04) { // 32 bit
                unsigned long *tmp_ad;
                tmp_ad = (unsigned long*)&datax1.pwmin32[ram4.vss_pwmseq[0]-1];
                DISABLE_INTERRUPTS;
                tmppwm = *tmp_ad;
                ENABLE_INTERRUPTS;
            } else {
                tmppwm = (unsigned long)datax1.pwmin16[ram4.vss_pwmseq[0]-1];
            }

            tmp_speed = (unsigned int) (vss1_coeff / tmppwm);
            dl = 1;
        }
    }

    if (dl) {
        __asm__ __volatile__("ldd    %1\n"
                             "subd   %3\n"
                             "tfr    d,y\n"
                             "clra\n"
                             "ldab    %2\n" // it is a uchar
                             "emuls\n"
                             "ldx     #100\n"
                             "edivs\n"
                             "addy    %3\n"
                            :"=y"(outpc.vss1)
                             :"m"(tmp_speed), "m"(ram4.vss1LF), "m"(outpc.vss1)
                             :"d", "x");
        if ((outpc.vss1 < 20) && (tmp_speed == 0)) {
            outpc.vss1 = 0;
        }

    }

    /*       VSS2       */
    dl = 0;
    if ((ram4.vss_opt0 & 0x0c) == 0x08) {
        if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS2) {
            flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS2;
            tmp_speed =
                (unsigned int) (*(unsigned int *) &(*port_vss2) *
                                (unsigned long) ram4.vss2_an_max /
                                (unsigned long) 1023);
            dl = 1;
        }
    } else if (((ram4.vss_opt0 & 0x0c) == 0x04) && (pin_vss2 || (flagbyte24 & FLAGBYTE24_IOVSS2))) {
        if (vss2_stall < VSS_STALL_TIMEOUT) {
            if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS2) {
                unsigned int tmp_vss_teeth, tmp_vss_time_sum;
                DISABLE_INTERRUPTS;
                tmp_vss_teeth = vss2_teeth;
                tmp_vss_time_sum = vss2_time_sum;
                vss2_teeth = 0;
                vss2_time_sum = 0;
                flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS2;
                ENABLE_INTERRUPTS;

                if (tmp_vss_time_sum > 0) {
                    tmp_speed = (unsigned int) ((vss2_coeff * tmp_vss_teeth) / tmp_vss_time_sum);
                    dl = 1;
                }
            }
        } else {
            vss2_stall = VSS_STALL_TIMEOUT + 1; // keep it from rolling over
            outpc.vss2 = 0;
            vss2_time = 0;
            vss2_st = VSS_SKIP_TEETH;
        }
    } else if (((ram4.vss_opt0 & 0x0c) == 0x04) && ((ram4.vss_opt & 0xf0) == 0xe0)) {
        unsigned int tmp_vss;

        if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS2) {
            flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS2;
            if (ram4.vss_can_size & 0x02) { // 16 bit
                tmp_vss = datax1.vss2_16;
            } else {
                tmp_vss = datax1.vss2_8;
            }

            tmp_speed = (unsigned int)(((unsigned long)tmp_vss * (unsigned long)ram4.vss2_can_scale) / 1000L);
            dl = 1;
        }
    } else if (((ram4.vss_opt0 & 0x0c) == 0x04) && ((ram4.vss_opt & 0xf0) == 0xf0)) {
        unsigned long tmppwm;

        if (flagbyte8 & FLAGBYTE8_SAMPLE_VSS2) {
            flagbyte8 &= ~FLAGBYTE8_SAMPLE_VSS2;
            if (ram4.enable_poll & 0x04) { // 32 bit
                unsigned long *tmp_ad;
                tmp_ad = (unsigned long*)&datax1.pwmin32[ram4.vss_pwmseq[1]-1];
                DISABLE_INTERRUPTS;
                tmppwm = *tmp_ad;
                ENABLE_INTERRUPTS;
            } else {
                tmppwm = (unsigned long)datax1.pwmin16[ram4.vss_pwmseq[1]-1];
            }

            tmp_speed = (unsigned int) (vss2_coeff / tmppwm);
            dl = 1;
        }
    }

    if (dl) {
        __asm__ __volatile__("ldd    %1\n"
                             "subd   %3\n"
                             "tfr    d,y\n"
                             "clra\n"
                             "ldab    %2\n" // it is a uchar
                             "emuls\n"
                             "ldx     #100\n"
                             "edivs\n"
                             "addy    %3\n"
                            :"=y"(outpc.vss2)
                             :"m"(tmp_speed), "m"(ram4.vss2LF), "m"(outpc.vss2)
                             :"d", "x");
        if ((outpc.vss2 < 20) && (tmp_speed == 0)) {
            outpc.vss2 = 0;
        }
    }

    /*       VSS3       */
    if (ram4.vss_opt0 & 0x10) {
        dl = 0;
        if ((ram4.vss_opt0 & 0x03) == 0x02) {
            if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS3) {
                flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS3;
                tmp_speed =
                    (unsigned int) (*(unsigned int *) &(*port_vss3) *
                                    (unsigned long) ram4.vss1_an_max /
                                    (unsigned long) 1023);
                dl = 1;
            }
        } else if (((ram4.vss_opt0 & 0x03) == 0x01) && (pin_vss3 || (flagbyte24 & FLAGBYTE24_IOVSS3))) {
            if (vss3_stall < VSS_STALL_TIMEOUT) {
                if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS3) {
                    unsigned int tmp_vss_teeth, tmp_vss_time_sum;
                    DISABLE_INTERRUPTS;
                    tmp_vss_teeth = vss3_teeth;
                    tmp_vss_time_sum = vss3_time_sum;
                    vss3_teeth = 0;
                    vss3_time_sum = 0;
                    flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS3;
                    ENABLE_INTERRUPTS;

                    if (tmp_vss_time_sum > 0) {
                        tmp_speed = (unsigned int) ((vss1_coeff * tmp_vss_teeth) / tmp_vss_time_sum);
                        dl = 1;
                    }
                }
            } else {
                vss3_stall = VSS_STALL_TIMEOUT + 1; // keep it from rolling over
                outpc.vss3 = 0;
                vss3_time = 0;
                vss3_st = VSS_SKIP_TEETH;
            }
        } else if (((ram4.vss_opt0 & 0x03) == 0x01) && ((ram5.vss_opt34 & 0x0f) == 0x0e)) {
            unsigned int tmp_vss;

            if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS3) {
                flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS3;
                if (ram4.vss_can_size & 0x01) { // 16 bit
                    tmp_vss = datax1.vss3_16;
                } else {
                    tmp_vss = datax1.vss3_8;
                }

                tmp_speed = (unsigned int)(((unsigned long)tmp_vss * (unsigned long)ram4.vss1_can_scale) / 1000L);
                dl = 1 ;
            }
        } else if (((ram4.vss_opt0 & 0x03) == 1) && ((ram5.vss_opt34 & 0x0f) == 0x0f)) {
            unsigned long tmppwm;

            if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS3) {
                flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS3;
                if (ram4.enable_poll & 0x04) { // 32 bit
                    unsigned long *tmp_ad;
                    tmp_ad = (unsigned long*)&datax1.pwmin32[ram5.vss3_pwmseq-1];
                    DISABLE_INTERRUPTS;
                    tmppwm = *tmp_ad;
                    ENABLE_INTERRUPTS;
                } else {
                    tmppwm = (unsigned long)datax1.pwmin16[ram5.vss3_pwmseq-1];
                }

                tmp_speed = (unsigned int) (vss1_coeff / tmppwm);
                dl = 1;
            }
        }

        if (dl) {
            __asm__ __volatile__("ldd    %1\n"
                                 "subd   %3\n"
                                 "tfr    d,y\n"
                                 "clra\n"
                                 "ldab    %2\n" // it is a uchar
                                 "emuls\n"
                                 "ldx     #100\n"
                                 "edivs\n"
                                 "addy    %3\n"
                                :"=y"(outpc.vss3)
                                 :"m"(tmp_speed), "m"(ram4.vss1LF), "m"(outpc.vss3)
                                 :"d", "x");
            if ((outpc.vss3 < 20) && (tmp_speed == 0)) {
                outpc.vss3 = 0;
            }

        }
    }

    /*       VSS4       */
    if (ram4.vss_opt0 & 0x40) {
        dl = 0;
        if ((ram4.vss_opt0 & 0x0c) == 0x08) { // uses VSS2 settings
            if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS4) {
                flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS4;
                tmp_speed =
                    (unsigned int) (*(unsigned int *) &(*port_vss4) *
                                    (unsigned long) ram4.vss2_an_max /
                                    (unsigned long) 1023);
                dl = 1;
            }
        } else if (((ram4.vss_opt0 & 0x0c) == 0x04) && (pin_vss4 || (flagbyte24 & FLAGBYTE24_IOVSS4))) {
            if (vss4_stall < VSS_STALL_TIMEOUT) {
                if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS4) {
                    unsigned int tmp_vss_teeth, tmp_vss_time_sum;
                    DISABLE_INTERRUPTS;
                    tmp_vss_teeth = vss4_teeth;
                    tmp_vss_time_sum = vss4_time_sum;
                    vss4_teeth = 0;
                    vss4_time_sum = 0;
                    flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS4;
                    ENABLE_INTERRUPTS;

                    if (tmp_vss_time_sum > 0) {
                        tmp_speed = (unsigned int) ((vss2_coeff * tmp_vss_teeth) / tmp_vss_time_sum);
                        dl = 1;
                    }
                }
            } else {
                vss4_stall = VSS_STALL_TIMEOUT + 1; // keep it from rolling over
                outpc.vss4 = 0;
                vss4_time = 0;
                vss4_st = VSS_SKIP_TEETH;
            }
        } else if (((ram4.vss_opt0 & 0x0c) == 0x04) && ((ram5.vss_opt34 & 0xf0) == 0xe0)) {
            unsigned int tmp_vss;

            if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS4) {
                flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS4;
                if (ram4.vss_can_size & 0x02) { // 16 bit
                    tmp_vss = datax1.vss4_16;
                } else {
                    tmp_vss = datax1.vss4_8;
                }

                tmp_speed = (unsigned int)(((unsigned long)tmp_vss * (unsigned long)ram4.vss2_can_scale) / 1000L);
                dl = 1;
            }
        } else if (((ram4.vss_opt0 & 0x0c) == 0x04) && ((ram5.vss_opt34 & 0xf0) == 0xf0)) {
            unsigned long tmppwm;

            if (flagbyte19 & FLAGBYTE19_SAMPLE_VSS4) {
                flagbyte19 &= ~FLAGBYTE19_SAMPLE_VSS4;
                if (ram4.enable_poll & 0x04) { // 32 bit
                    tmppwm = datax1.pwmin32[ram5.vss4_pwmseq-1];
                } else {
                    tmppwm = (unsigned long)datax1.pwmin16[ram5.vss4_pwmseq-1];
                }

                tmp_speed = (unsigned int) (vss2_coeff / tmppwm);
                dl = 1;
            }
        }

        if (dl) {
            __asm__ __volatile__("ldd    %1\n"
                                 "subd   %3\n"
                                 "tfr    d,y\n"
                                 "clra\n"
                                 "ldab    %2\n" // it is a uchar
                                 "emuls\n"
                                 "ldx     #100\n"
                                 "edivs\n"
                                 "addy    %3\n"
                                :"=y"(outpc.vss4)
                                 :"m"(tmp_speed), "m"(ram4.vss2LF), "m"(outpc.vss4)
                                 :"d", "x");
            if ((outpc.vss4 < 20) && (tmp_speed == 0)) {
                outpc.vss4 = 0;
            }
        }
    }

    // Shaft Speed
    if (pin_ss1 || (flagbyte18 & (FLAGBYTE18_SS1_PT2 | FLAGBYTE18_SS1_PT4 | FLAGBYTE18_SS1_PT5 | FLAGBYTE18_SS1_PT6))) {
        if (ss1_stall < SS_STALL_TIMEOUT) {
            if (flagbyte8 & FLAGBYTE8_SAMPLE_SS1) {
                unsigned int tmp_vss_teeth, tmp_vss_time_sum;
                DISABLE_INTERRUPTS;
                tmp_vss_teeth = ss1_teeth;
                tmp_vss_time_sum = ss1_time_sum;
                ss1_teeth = 0;
                ss1_time_sum = 0;
                flagbyte8 &= ~FLAGBYTE8_SAMPLE_SS1;
                ENABLE_INTERRUPTS;

                if (tmp_vss_time_sum > 0) {
                    tmp_speed = (unsigned int) ((ss1_coeff * tmp_vss_teeth) / tmp_vss_time_sum);
                    __asm__ __volatile__("ldd    %1\n"
                                         "subd   %3\n"
                                         "tfr    d,y\n"
                                         "clra\n"
                                         "ldab    %2\n" // it is a uchar
                                         "emuls\n"
                                         "ldx     #100\n"
                                         "edivs\n"
                                         "addy    %3\n"
                                        :"=y"(outpc.ss1)
                                         :"m"(tmp_speed), "m"(ram4.ss1LF), "m"(outpc.ss1)
                                         :"d", "x");
                }
            }
        } else {
            ss1_stall = SS_STALL_TIMEOUT + 1;  // keep it from rolling over
            outpc.ss1 = 0;
            ss1_time = 0;
            ss1_st = VSS_SKIP_TEETH;
        }
    }

    if (pin_ss2 || (flagbyte18 & (FLAGBYTE18_SS2_PT2 | FLAGBYTE18_SS2_PT4 | FLAGBYTE18_SS2_PT5 | FLAGBYTE18_SS2_PT6))) {
        if (ss2_stall < SS_STALL_TIMEOUT) {
            if (flagbyte8 & FLAGBYTE8_SAMPLE_SS2) {
                unsigned int tmp_vss_teeth, tmp_vss_time_sum;
                DISABLE_INTERRUPTS;
                tmp_vss_teeth = ss2_teeth;
                tmp_vss_time_sum = ss2_time_sum;
                ss2_teeth = 0;
                ss2_time_sum = 0;
                flagbyte8 &= ~FLAGBYTE8_SAMPLE_SS2;
                ENABLE_INTERRUPTS;

                if (tmp_vss_time_sum > 0) {
                    tmp_speed = (unsigned int) ((ss2_coeff * tmp_vss_teeth) / tmp_vss_time_sum);
                    __asm__ __volatile__("ldd    %1\n"
                                         "subd   %3\n"
                                         "tfr    d,y\n"
                                         "clra\n"
                                         "ldab    %2\n" // it is a uchar
                                         "emuls\n"
                                         "ldx     #100\n"
                                         "edivs\n"
                                         "addy    %3\n"
                                        :"=y"(outpc.ss2)
                                         :"m"(tmp_speed), "m"(ram4.ss2LF), "m"(outpc.ss2)
                                         :"d", "x");
                }
            }
        } else {
            ss2_stall = SS_STALL_TIMEOUT + 1;  // keep it from rolling over
            outpc.ss2 = 0;
            ss2_time = 0;
            ss2_st = VSS_SKIP_TEETH;
        }
    }

    if (pin_vssout) {
        if (outpc.vss1 == 0) {
            vssout_match = 0; // don't flip output
        } else {
            if ((ram4.vssout_opt & 0xc0) == 0x00) {
                /* time */
                vssout_match = ram4.vssout_scale / outpc.vss1;
            } else if ((ram4.vssout_opt & 0xc0) == 0x40) {
                /* pulses per mile
                factor is 16093.44 * 20000 / 2 
                (miles to metresX10, 20000 ticks per second, but need half a period) */
                vssout_match = (160934400UL / ram4.vssout_scale) / outpc.vss1;
            } else if ((ram4.vssout_opt & 0xc0) == 0x80) {
                /* pulses per km
                factor is 10000.00 * 20000 / 2 
                (miles to metresX10, 20000 ticks per second, but need half a period) */
                vssout_match = (100000000UL / ram4.vssout_scale) / outpc.vss1;
            } else {
                vssout_match = 0;
            }
        }
    } else {
        vssout_match = 0;
    }
}

void nitrous()
{
    /**************************************************************************
     **
     ** Nitrous
     **
     **************************************************************************/
    if (ram4.N2Oopt & 0x04) {   // are we using nitrous at all
        unsigned char launchcut;
        if ((ram4.N2Oopt & N2OOPT_5) && (outpc.status2 & STATUS2_LAUNCH)
            && (!(outpc.status2 & STATUS2_FLATSHIFT))) {
            /* Launch+nitrous enabled, in launch, not flat-shift */

            /* Check for MAP limit and use 10kPa hyst */
            if (outpc.map > ram4.N2Olaunchmaxmap) {
                flagbyte23 |= FLAGBYTE23_LAUNCHMAXMAP;
            } else if ((flagbyte23 & FLAGBYTE23_LAUNCHMAXMAP)
                && (outpc.map < (ram4.N2Olaunchmaxmap - 100))) {
                flagbyte23 &= ~FLAGBYTE23_LAUNCHMAXMAP;
            }

            /* Allow or disallow nitrous */
            if (flagbyte23 & FLAGBYTE23_LAUNCHMAXMAP) {
                launchcut = 1; /* Over the MAP limit, kill it */
            } else {
                launchcut = 0; /* Below MAP limit and over-ridden, allow it */
            }
        } else if (outpc.status2 & STATUS2_LAUNCH) {
            launchcut = 1; /* No over-ride, so kill the nitrous */
        } else {
            launchcut = 0; /* Not in launch anyway, so allow it */
        }

        if ((outpc.status3 & STATUS3_3STEP) || launchcut || (flagbyte10 & FLAGBYTE10_TC_N2O)
            || maxafr_stat) { /* if in launch or 3step or TC or AFRsafety cut nitrous */
            outpc.status2 &= ~STATUS2_NITROUS1;
            outpc.status2 &= ~STATUS2_NITROUS2;
            outpc.nitrous1_duty = 0;
            outpc.nitrous2_duty = 0;
            outpc.n2o_addfuel = 0;
            outpc.n2o_retard = 0;
            outpc.nitrous_timer_out = 0;
            goto NITROUS_OUTPUTS;
        }
        // is enable input on
        //check if the chosen pin is low
        if (((*port_n2oin & pin_n2oin) == pin_match_n2oin) && pin_n2oin)
            goto DO_NITROUS;

        // no valid input so turn it off
        outpc.status2 &= ~STATUS2_NITROUS1;
        outpc.status2 &= ~STATUS2_NITROUS2;
        outpc.nitrous1_duty = 0;
        outpc.nitrous2_duty = 0;
        outpc.n2o_addfuel = 0;
        outpc.n2o_retard = 0;
        outpc.nitrous_timer_out = 0;
        goto NITROUS_OUTPUTS;

      DO_NITROUS:
        // selection logic
        if ((outpc.rpm > ram4.N2ORpm) && (outpc.rpm < ram4.N2ORpmMax)
            && (outpc.tps > ram4.N2OTps) && (outpc.clt > ram4.N2OClt)
            && (n2o_act_timer == 0)) {
            if (!(outpc.status2 & STATUS2_NITROUS1)) {
                n2o2_act_timer = ram4.N2O2delay;        // if first change then set delay to stage 2
                nitrous_timer = 0;      // progressive up counter
            }
            outpc.status2 |= STATUS2_NITROUS1;
            if (ram4.N2Oopt2 & 1) {     // progressive
                if ((ram4.N2Oopt & 0xc0) == 0x40) { // time based
                    outpc.n2o_addfuel = intrp_1ditable(nitrous_timer, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_time, 1, (int *) ram_window.pg29.n2o1_pw, 29);
                    outpc.n2o_retard = intrp_1ditable(nitrous_timer, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_time, 0, (int *) ram_window.pg29.n2o1_retard, 29);
                    outpc.nitrous1_duty = intrp_1dctable(nitrous_timer, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_time,  0, ram_window.pg29.n2o1_duty, 29);
                    outpc.nitrous_timer_out = nitrous_timer;
                } else if ((ram4.N2Oopt & 0xc0) == 0x80) { // vss based
                    unsigned int vss;
                    vss = outpc.vss1; /* possibly add option */
                    outpc.n2o_addfuel = intrp_1ditable(vss, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_vss, 1, (int *) ram_window.pg29.n2o1_pw, 29);
                    outpc.n2o_retard = intrp_1ditable(vss, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_vss, 0, (int *) ram_window.pg29.n2o1_retard, 29);
                    outpc.nitrous1_duty = intrp_1dctable(vss, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_vss,  0, ram_window.pg29.n2o1_duty, 29);
                    outpc.nitrous_timer_out = nitrous_timer;
                } else {
                    outpc.n2o_addfuel = intrp_1ditable(outpc.rpm, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_rpm, 1, (int *) ram_window.pg29.n2o1_pw, 29);
                    outpc.n2o_retard = intrp_1ditable(outpc.rpm, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_rpm, 0, (int *) ram_window.pg29.n2o1_retard, 29);
                    outpc.nitrous1_duty = intrp_1dctable(outpc.rpm, NUM_PROGN2O,
                       (int *) ram_window.pg29.n2o1_rpm, 0, ram_window.pg29.n2o1_duty, 29);
                }
            } else {
                outpc.n2o_addfuel =
                    twoptlookup(outpc.rpm, ram4.N2ORpm, ram4.N2ORpmMax,
                                ram4.N2OPWLo, ram4.N2OPWHi);
                outpc.n2o_retard = ram4.N2OAngle;       // retard timing
            }

            // if stage 1 on, then check for stage 2
            if ((ram4.N2Oopt & 0x08)
                && ((n2o2_act_timer == 0)
                    || ((ram4.N2Oopt2 & 1) && (ram4.N2Oopt2 & 2)))
                && ((ram4.N2Oopt2 & 1)
                    || ((outpc.rpm > ram4.N2O2Rpm)
                        && (outpc.rpm < ram4.N2O2RpmMax)))
                && (!(outpc.status2 & STATUS2_LAUNCH))) { /* no stage2 during launch */

                // if stage2 and (time expired OR progressive time-based) and (progressive or within rpm window)  
                outpc.status2 |= STATUS2_NITROUS2;

                if (ram4.N2Oopt2 & 1) { // progressive
                    if ((ram4.N2Oopt & 0xc0) == 0x40) {     // time based
                        outpc.n2o_addfuel += intrp_1ditable(nitrous_timer, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_time, 1, (int *) ram_window.pg29.n2o2_pw, 29);
                        outpc.n2o_retard += intrp_1ditable(nitrous_timer, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_time, 0, (int *) ram_window.pg29.n2o2_retard, 29);
                        outpc.nitrous2_duty = intrp_1dctable(nitrous_timer, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_time, 0, ram_window.pg29.n2o2_duty, 29);
                    } else if ((ram4.N2Oopt & 0xc0) == 0x80) { // vss based
                        unsigned int vss;
                        vss = outpc.vss1; /* possibly add option */
                        outpc.n2o_addfuel = intrp_1ditable(vss, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_vss, 1, (int *) ram_window.pg29.n2o2_pw, 29);
                        outpc.n2o_retard = intrp_1ditable(vss, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_vss, 0, (int *) ram_window.pg29.n2o2_retard, 29);
                        outpc.nitrous1_duty = intrp_1dctable(vss, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_vss,  0, ram_window.pg29.n2o2_duty, 29);
                        outpc.nitrous_timer_out = nitrous_timer;
                    } else {
                        outpc.n2o_addfuel += intrp_1ditable(outpc.rpm, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_rpm, 1, (int *) ram_window.pg29.n2o2_pw, 29);
                        outpc.n2o_retard +=  intrp_1ditable(outpc.rpm, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_rpm, 0, (int *) ram_window.  pg29.n2o2_retard, 29);
                        outpc.nitrous2_duty = intrp_1dctable(outpc.rpm, NUM_PROGN2O,
                           (int *) ram_window.pg29.n2o2_rpm, 0, ram_window.pg29.n2o2_duty, 29);
                    }
                } else {
                    outpc.n2o_addfuel +=
                        twoptlookup(outpc.rpm, ram4.N2O2Rpm,
                                    ram4.N2O2RpmMax, ram4.N2O2PWLo,
                                    ram4.N2O2PWHi);
                    outpc.n2o_retard += ram4.N2O2Angle; // retard timing
                }
            } else {
                outpc.status2 &= ~STATUS2_NITROUS2;     // if stage1 is off then so is stage2
                outpc.nitrous2_duty = 0;
            }

        } else {
            outpc.status2 &= ~STATUS2_NITROUS1;
            outpc.status2 &= ~STATUS2_NITROUS2; // if stage1 is off then so is stage2
            outpc.nitrous1_duty = 0;
            outpc.nitrous2_duty = 0;
            outpc.n2o_addfuel = 0;
            outpc.n2o_retard = 0;
            outpc.nitrous_timer_out = 0;
        }

      NITROUS_OUTPUTS:
        // here we flip the bits for chosen output pins

        if (!(ram4.N2Oopt2 & 1)) {      // not progressive
            // stage 1
            if (outpc.status2 & STATUS2_NITROUS1) {
                SSEM0SEI;
                *port_n2o1n |= pin_n2o1n;
                *port_n2o1f |= pin_n2o1f;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                *port_n2o1n &= ~pin_n2o1n;
                *port_n2o1f &= ~pin_n2o1f;
                CSEM0CLI;
            }

            if (ram4.N2Oopt & 0x08) {
                // stage 2
                if (outpc.status2 & STATUS2_NITROUS2) {
                    SSEM0SEI;
                    *port_n2o2n |= pin_n2o2n;
                    *port_n2o2f |= pin_n2o2f;
                    CSEM0CLI;
                } else {
                    SSEM0SEI;
                    *port_n2o2n &= ~pin_n2o2n;
                    *port_n2o2f &= ~pin_n2o2f;
                    CSEM0CLI;
                }
            }
        }
    }
}

int median3pt(int array[3], int value)
{
    int val0, val1, val2, tmp;
    /* insert new value into master array and local copy */
    val2 = array[2] = array[1];
    val1 = array[1] = array[0];
    val0 = array[0] = value;
    /* sort */
    if (val0 > val1) {
        tmp = val0;
        val0 = val1;
        val1 = tmp;
    }
    if (val1 > val2) {
        tmp = val1;
        val1 = val2;
        val2 = tmp;
    }
    if (val0 > val1) {
        tmp = val0;
        val0 = val1;
        val1 = tmp;
    }

    /* median */
    return val1;
}

void sample_map_tps(char *localflags)
{
    unsigned int utmp1;
    int tmp1;
    int map_local;
    unsigned int tmp_mapsample_time;

    RPAGE = RPAGE_VARS1;

    if ((flagbyte20 & FLAGBYTE20_USE_MAP) && 
         (((flagbyte3 & flagbyte3_samplemap) && !(ram4.mapsample_opt & 0x4)) || 
         ((flagbyte15 & FLAGBYTE15_MAP_AVG_RDY) && (ram4.mapsample_opt & 0x4)))) {

        unsigned long map_sum_local;
        volatile unsigned int map_cnt_local; /* avoid compiler caching */

        *localflags |= LOCALFLAGS_RUNFUEL;

        DISABLE_INTERRUPTS;
        map_sum_local = map_sum;
        map_cnt_local = map_cnt;
        map_local = map_temp;
        flagbyte3 &= ~flagbyte3_samplemap;
        flagbyte15 &= ~FLAGBYTE15_MAP_AVG_RDY;
        ENABLE_INTERRUPTS;

        /* Using average over period or if first time in window.
        (First time into window code can return a zero) */
        if ((ram4.mapsample_opt & 0x4) || (map_local == 0)) {
            map_local = (int)(map_sum_local / (long)map_cnt_local);
        }

        if ((ram4.mapport & 0x60) == 0x20) {
            /* For frequency sensor, convert period to frequency to 0-1023 */
            unsigned int f;

            /* calc freq x 5  (0.2Hz units) */
            if (flagbyte12 & FLAGBYTE12_MAP_FSLOW) {
                f =  1250000UL / map_local; /* in slow mode time is reported in 4us units */
            } else {
                f = 5000000UL / map_local;
            }

            if (f < ram5.map_freq0) {
                map_local = 0;
            } else if (f > ram5.map_freq1) {
                map_local = 1023;
            } else {
                map_local = (int)((1023UL * (f - ram5.map_freq0)) / (ram5.map_freq1 - ram5.map_freq0));
            }
        }

        __asm__ __volatile__("ldd    %1\n"
                             "subd   %2\n"
                             "ldy    %3\n"
                             "emul\n"
                             "ldx    #1023\n"
                             "ediv\n"
                             "addy   %2\n"
                             :"=y"(utmp1)
                             :"m"(ram4.mapmax), "m"(ram4.map0), "m"(map_local)
                             :"x");

        if (stat_map && (ram5.cel_action1 & 0x01) && (ram5.cel_action1 & 0x80)) {
            /* broken MAP and using fallback A-N lookup table */
            utmp1 = intrp_2ditable(outpc.rpm, outpc.tps, 6, 6, &ram_window.pg25.amap_rpm[0],
              &ram_window.pg25.amap_tps[0], &ram_window.pg25.alpha_map_table[0][0], 25); // will alter RPAGE
        }

        /* new MAP value held in tmp1 at this point */

        tmp1 = median3pt(&map_data[0], tmp1);  // <-- wrong var!

        __asm__ __volatile__("ldd    %1\n"
                             "subd   %3\n"
                             "tfr    d,y\n"
                             "clra\n"
                             "ldab    %2\n" // it is a uchar
                             "emuls\n"
                             "ldx     #100\n"
                             "edivs\n"
                             "addy    %3\n"
                             :"=y"(outpc.map)
                             :"m"(utmp1), "m"(ram4.mapLF), "m"(outpc.map)
                             :"x");
    }

    /* Give a continual readout - not just at sample points. */
    if (flagbyte8 & FLAGBYTE8_USE_MAF) {
        if (ram4.MAFOption & 0x20) { // frequency MAFs
            unsigned int f;
            /* calc freq x 4 - note different units for MAF (0.25Hz) */
            if (flagbyte12 & FLAGBYTE12_MAF_FSLOW) {
                f =  1000000UL / *mafport; /* in slow mode time is reported in 4us units */
            } else {
                f = 4000000UL / *mafport;
            }

            if (f < ram5.maf_freq0) {
                outpc.maf_volts = 0;
            } else if (f > ram5.maf_freq1) {
                outpc.maf_volts = 5000;
            } else {
                /* need to calculate fully or rounding gets the better of us */
                outpc.maf_volts = (int)((5000UL * (f - ram5.maf_freq0)) / (ram5.maf_freq1 - ram5.maf_freq0));
            }
        } else { // voltage MAFs
            outpc.maf_volts = (5000L * *mafport) / 1023; // 0.001V
        }
    }

    /* Sample MAF */
    if ((flagbyte8 & FLAGBYTE8_USE_MAF) && (flagbyte20 & FLAGBYTE20_MAF_AVG_RDY)
        && (!burnstat) && (!ltt_fl_state)) { /* but not while eflash in use */
        unsigned long maf_sum_local;
        unsigned int maf_cnt_local, maf_local;

        *localflags |= LOCALFLAGS_RUNFUEL;

        DISABLE_INTERRUPTS;
        maf_sum_local = maf_sum;
        maf_cnt_local = maf_cnt;
        maf_local = maf_temp;
        flagbyte20 &= ~FLAGBYTE20_MAF_AVG_RDY;
        ENABLE_INTERRUPTS;

        if (ram4.MAFOption & 0x20) {
            /* For frequency sensor, convert period to frequency to 0-1023 */
            unsigned int f, mv;

            /* Average over period if data available
               The returned numbers are timer tick values.
               Must convert to frequency, then to flow. */
            if (maf_sum_local) {
                maf_local = (int)(maf_sum_local / (long)maf_cnt_local);
            }

            /* calc freq x 4 - note different units for MAF (0.25Hz) */
            if (flagbyte12 & FLAGBYTE12_MAF_FSLOW) {
                f =  1000000UL / maf_local; /* in slow mode time is reported in 4us units */
            } else {
                f = 4000000UL / maf_local;
            }

            if (f < ram5.maf_freq0) {
                maf_local = 0;
                mv = 0;
                stat_maf |= 1;
            } else if (f > ram5.maf_freq1) {
                maf_local = 1023;
                mv = 5000;
                stat_maf |= 1;
            } else {
                maf_local = (int)((1023UL * (f - ram5.maf_freq0)) / (ram5.maf_freq1 - ram5.maf_freq0));
                /* need to calculate fully or rounding gets the better of us */
                mv = (int)((5000UL * (f - ram5.maf_freq0)) / (ram5.maf_freq1 - ram5.maf_freq0));
                stat_maf &= ~1; // only valid while no other checks are in place
            }
            if (ram4.feature7 & 0x04) { /* old calibration table */
                GPAGE = 0x10;
                __asm__ __volatile__("aslx\n"
                                     "addx   #0x5400\n"    // maffactor_table address
                                     "gldy   0,x\n"
                                     :"=y"(utmp1)
                                     :"x"(maf_local));
            } else {
                utmp1 = intrp_1ditable(mv, 64, (int *)ram_window.pg25.mafv, 0, (unsigned int *)
                    ram_window.pg25.mafflow, 25);
                if (utmp1 > 0x2000) {
                    utmp1 -= 0x2000; // zero offset
                } else {
                    utmp1 = 0;
                }
            }
        } else {
            /* Average over period if data available
               The returned numbers are already flow values. Subtract the zero offset. */
            if (maf_sum_local) {
                if (maf_sum_local < (maf_cnt_local * 0x2000UL)) {
                    utmp1 = 0; // we don't support overall negative flow
                } else {
                    utmp1 = ((maf_sum_local - (maf_cnt_local * 0x2000UL))/ maf_cnt_local);
                }
            } else {
                utmp1 = maf_local;
            }
        }

        utmp1 = median3pt(&maf_data[0], utmp1);

        __asm__ __volatile__("ldd    %1\n"
                "subd   %3\n"
                "tfr    d,y\n"
                "clra\n"
                "ldab    %2\n" // it is a uchar
                "emuls\n"
                "ldx     #100\n"
                "edivs\n"
                "addy    %3\n"
                :"=y"(mafraw)
                :"m"(utmp1), "m"(ram4.mafLF), "m"(mafraw)
                :"x");

        if (ram4.feature7 & 0x04) { /* old correction table */
            /* this is the new code with the unsigned X axis lookup */
            utmp1 = intrp_1dctableu(mafraw, 12, (int *)ram_window.pg11.MAFFlow, 0, (unsigned char *)
                ram_window.pg11.MAFCor, 11);
            outpc.maf = ((unsigned long)mafraw * utmp1) / 100;
        } else {
            outpc.maf = mafraw;
        }

        if (outpc.rpm > 5) { // ensure that 1,2 rpm don't give a bogus huge MAFload value
            unsigned long scaled_maf = outpc.maf * ((ram4.maf_range & 0x03) + 1);
            outpc.mafload = (int)(((unsigned long)(MAFCoef / outpc.aircor) * scaled_maf) / outpc.rpm);
//            if (outpc.mafload < 100) {
//                outpc.mafload = 100;
//            }
            mafload_no_air = (int)(((unsigned long)(MAFCoef / 1000) * scaled_maf) / outpc.rpm);
//            if (mafload_no_air < 100) {
//                mafload_no_air = 100;
//            }
        } else {
            outpc.mafload = 1000;
            mafload_no_air = 1000;
        }
    }

    if (( (!(flagbyte20 & FLAGBYTE20_USE_MAP)) && (!(flagbyte8 & FLAGBYTE8_USE_MAF)) )
        && (flagbyte15 & FLAGBYTE15_MAP_AVG_TRIG)) {
        /* No MAP, no MAF, use trigger from ign_in to run fuel */
        *localflags |= LOCALFLAGS_RUNFUEL;
        DISABLE_INTERRUPTS;
        flagbyte15 &= ~FLAGBYTE15_MAP_AVG_TRIG;
        flagbyte20 &= ~FLAGBYTE20_MAF_AVG_TRIG;
        ENABLE_INTERRUPTS;
    }

    // mapdot
    tmp_mapsample_time = (unsigned int)lmms - mapsample_time;
    if (tmp_mapsample_time > 78) { // minimum 10ms
        DISABLE_INTERRUPTS;
        mapsample_time = (unsigned int)lmms;
        ENABLE_INTERRUPTS;
        if (flagbyte8 & FLAGBYTE8_USE_MAF_ONLY) {
            /* not using real MAP anywhere, use mafload for mapdot */
            map_local = outpc.mafload;
        } else {
            map_local = outpc.map;
        }

        RPAGE = RPAGE_VARS1;

        /* sliding windows */
        /* mapdot calc - sliding window - variable dataset and max sample rate */
        v1.mapdot_data[0][0] = (unsigned int)lmms; /* store actual 16 bit time to first pt in array */
        v1.mapdot_data[0][1] = map_local; /* store map - note this is _after_ the lags*/

        /* minimum 10ms between samples */
        int mapi, samples, a, b;
        long mapdot_sumx, mapdot_sumx2, mapdot_sumy, mapdot_sumxy;
        long toprow, btmrow;
        unsigned long mapdot_x; // was uint

        mapdot_sumx = 0;
        mapdot_sumx2 = 0;
        mapdot_sumy = 0;
        mapdot_sumxy = 0;

        /* decide how many samples to use based on rate from last two points
            miniumum is three, max is MAPDOT_N (20)
        */
        a = v1.mapdot_data[0][1] - v1.mapdot_data[1][1];
        b = v1.mapdot_data[0][0] - v1.mapdot_data[1][0];
        a = (int)((7812L * a) / b);
        a = long_abs(a);

        if (a > 2500) { // 250%/s
            samples = 3;
        } else if (a > 1000) {
            samples = 4;
        } else {
            samples = 5;
        }

/* note that this only uses first and last, so doesn't fully benefit from the over-sampling */
        mapdot_x = 0; // not used
        toprow = (int)v1.mapdot_data[0][1] - (int)v1.mapdot_data[samples - 1][1];
        btmrow = v1.mapdot_data[0][0] - v1.mapdot_data[samples - 1][0];    

        toprow = (toprow * 781) / btmrow;
        if (toprow > 32767) {
            toprow = 32767;
        } else if (toprow < -32767) {
            toprow = -32767;
        }

        toprow = 50L * (toprow - outpc.mapdot); /* now 50% lag */
        if ((toprow > 0) && (toprow < 100)) {
            toprow = 100;
        } else if ((toprow < 0) && (toprow > -100)) {
            toprow = -100;
        }
        outpc.mapdot += (int)(toprow / 100);

        /* shuffle data forwards by one */
        for (mapi = MAPDOT_N - 1; mapi > 0 ; mapi--) {
            v1.mapdot_data[mapi][0] = v1.mapdot_data[mapi - 1][0];
            v1.mapdot_data[mapi][1] = v1.mapdot_data[mapi - 1][1];
        }

        last_map = map_local;
    }

    if (flagbyte20 & FLAGBYTE20_50MS) { // actually every 10ms
        unsigned int tmp_tpssample_time, raw_tps;
        unsigned int raw_tps_accum;
        DISABLE_INTERRUPTS;
        flagbyte20 &= ~FLAGBYTE20_50MS;   // clear flag
        tmp_tpssample_time = (unsigned int)lmms - tpssample_time;
        tpssample_time = (unsigned int)lmms;
        ENABLE_INTERRUPTS;

        /* raw_tps = average of 8 pt ring buffer tps_ring[] */
        DISABLE_INTERRUPTS;
        raw_tps_accum = tps_ring[0];
        raw_tps_accum += tps_ring[1];
        raw_tps_accum += tps_ring[2];
        raw_tps_accum += tps_ring[3];
        raw_tps_accum += tps_ring[4];
        raw_tps_accum += tps_ring[5];
        raw_tps_accum += tps_ring[6];
        raw_tps_accum += tps_ring[7];
        ENABLE_INTERRUPTS;

        raw_tps = raw_tps_accum >> 3;

        // get map, tps
        // map, tps lag(IIR) filters

        outpc.tpsadc = raw_tps; // send back for TPS calib

        if (ram4.tps0 != tps0_orig) {
            // if user sends a new calibration, cancel out any auto-zero
            tps0_auto = tps0_orig = ram4.tps0;
        }

        __asm__ __volatile__("ldd %3\n"
                             "subd %2\n"
                             "tfr  d,y\n"
                             "ldd  %1\n"
                             "subd %2\n"
                             "pshd\n"
                             "ldd #1000\n"
                             "emuls\n" "pulx\n" "edivs\n":"=y"(tmp1)
                             :"m"(ram4.tpsmax), "m"(tps0_auto),
                             "m"(raw_tps)
                             :"d", "x");

        tmp1 = median3pt(&tps_data[0], tmp1);

        __asm__ __volatile__("ldd    %1\n"
                             "subd   %3\n"
                             "tfr    d,y\n"
                             "clra\n"
                             "ldab    %2\n" // it is a uchar
                             "emuls\n"
                             "ldx     #100\n"
                             "edivs\n"
                             "addy    %3\n"
                             :"=y"(outpc.tps)
                             :"m"(tmp1), "m"(ram4.tpsLF), "m"(outpc.tps)
                             :"d", "x");

        if (ram4.feature7 & 0x10) {
            /* use TPSwot curve to ignore TPS > WOT vs. RPM */
            int tps_max;
            tps_max = intrp_1ditable(outpc.rpm, 6,
                     (unsigned int *) ram_window.pg25.tpswot_rpm, 1,
                     (int *) ram_window.pg25.tpswot_tps, 25);
            if (tmp1 > tps_max) {
                tmp1 = tps_max;
            }

            /* scale tmp1 as 0-tps_max */
            tmp1 = (tmp1 * 1000L) / tps_max;

        }

        RPAGE = RPAGE_VARS1;
        /* sliding windows */

        /* tpsdot calc - sliding window - variable dataset and max sample rate */
        v1.tpsdot_data[0][0] = (unsigned int)lmms; /* store actual 16 bit time to first pt in array */
        v1.tpsdot_data[0][1] = tmp1; /* w/o extra bits */

        /* minimum 10ms between samples */
        if ((v1.tpsdot_data[0][0] - v1.tpsdot_data[1][0]) > 78) {
            int tpsi, samples, a, b;
            long toprow, btmrow;

            /* decide how many samples to use based on rate from last two points
                miniumum is three, max is TPSDOT_N (20)
            */
            a = v1.tpsdot_data[0][1] - v1.tpsdot_data[1][1];
            b = v1.tpsdot_data[0][0] - v1.tpsdot_data[1][0];
            a = (int)((7812L * a) / b);
            a = long_abs(a);

            if (a > 2500) { // 250%/s
                samples = 3;
            } else if (a > 1000) {
                samples = 4;
            } else {
                samples = 5;
            }

/* note that this only uses first and last, so doesn't fully benefit from the over-sampling */
            toprow = (int)v1.tpsdot_data[0][1] - (int)v1.tpsdot_data[samples - 1][1];
            btmrow = v1.tpsdot_data[0][0] - v1.tpsdot_data[samples - 1][0];    

            btmrow = btmrow / 10; /* allows top row to be 10x less to reduce overflow. */
            toprow = (toprow * 781) / btmrow;
            if (toprow > 32767) {
                toprow = 32767;
            } else if (toprow < -32767) {
                toprow = -32767;
            }

            toprow = 50L * (toprow - outpc.tpsdot); /* now 50% lag */
            if ((toprow > 0) && (toprow < 100)) {
                toprow = 100;
            } else if ((toprow < 0) && (toprow > -100)) {
                toprow = -100;
            }

            if (stat_tps) {
            /* broken TPS*/
//            outpc.tps = 0;
              outpc.tpsdot = 0; // if wonky then ignore TPSdot to prevent false-AE
            } else {
              outpc.tpsdot += (int)(toprow / 100);
            }

            /* shuffle data forwards by one */
            for (tpsi = TPSDOT_N - 1; tpsi > 0 ; tpsi--) {
                v1.tpsdot_data[tpsi][0] = v1.tpsdot_data[tpsi - 1][0];
                v1.tpsdot_data[tpsi][1] = v1.tpsdot_data[tpsi - 1][1];
            }
        /* end sliding window */
        }
        ego_get_sample();
    }
}

int calc_ITB_load(int percentbaro)
{
    int tmp3, tmp4, tmp5; 

    tmp3 = intrp_1ditable(outpc.rpm, 12,
                         (unsigned int *) ram_window.pg19.ITB_load_rpms, 1,
                         (int *) ram_window.pg19.ITB_load_loadvals, 19);

    if ((outpc.tps <= ram5.ITB_load_idletpsthresh) || 
        (percentbaro < ram5.ITB_load_mappoint)) {
        /* Make MAP fit in to 0-tmp3 % load... so if user selects 60%,
         * 0-90kPa would be 0-60% load, and the throttle position above
         * that point to 100% throttle will be 60% load to 100% load
         */
        tmp3 = (((long) percentbaro * tmp3) / ram5.ITB_load_mappoint);
    } else {
        tmp4 = intrp_1ditable(outpc.rpm, 12,
                             (unsigned int *) ram_window.pg19.ITB_load_rpms, 1,
                             (int *) ram_window.pg19.ITB_load_switchpoints, 19);
        /* Make TPS fit in tmp3 - 100% */
        if (outpc.tps >= tmp4) {
            /* This is the amt of load TPS has to fit into */
            tmp5 = 1000 - tmp3;

            /* This is the actual percent above the
             * switchpoint TPS is at */
            tmp4 = ((long)(outpc.tps - tmp4) * 1000) / (1000 - tmp4);

            /* Now scale that into what's left above the 90 % baro 
             * point
             */
            tmp3 = (((long) tmp4 * tmp5) / 1000) + tmp3;
        }
        /* IF TPS hasn't gone above the setpoint, load should
         * stay at the user-set setpoint */
    }

    /* Make 10 the lowest possible load */

    if (tmp3 < 100) {
        tmp3 = 100;
    }

    if (tmp3 > 1000) {
        tmp3 = 1000;
    }

    return tmp3;
}

void calc_baro_mat_load(void)
{
    int tmp1, tmp2, tmp3 = 0;
    unsigned char uctmp1, uctmp2, uctmp3, uctmp4, uctmp5, uctmp6;

    if ((ram4.BaroOption & 0x03) == 0) {
        outpc.barocor = 1000;
    } else {                    // barometric correction (%)
        outpc.barocor = barocor_eq(outpc.baro) + intrp_1ditable(outpc.baro, NO_BARS,
                        (int *)ram_window.pg8.BaroVals, 1,
                        (unsigned int *)ram_window.pg8.BaroCorDel, 8);
    }

    if ((ram4.feature3 & 0x40) && (outpc.engine & ENGINE_ASE)) {
        outpc.aircor = 1000;
        outpc.airtemp = outpc.mat;  // use raw MAT in this mode
    } else {
        unsigned int flow; // beware of signed ness of function if flow > 32767
        unsigned long flowtmp;

        flowtmp = (unsigned long)outpc.fuelload * outpc.rpm;
        flowtmp /= 1000; // 10 for fuelload. 100 for 0.01 final units
        flowtmp *= outpc.vecurr1;

        flow = (unsigned int) (flowtmp / 1000UL); // normalise VE

        // use MAT/CLT scaling for a temporary mat value
        tmp1 = intrp_1ditable(flow, 6, ram_window.pg8.matclt_flow, 0, ram_window.pg8.matclt_pct, 8);    // returns %clt * 100

        outpc.airtemp = (int) ((((long) outpc.mat * (10000 - tmp1)) + ((long) outpc.clt * tmp1)) / 10000L);  // 'corrected' MAT

        // airdensity correction (%)
        outpc.aircor = intrp_1ditable(outpc.airtemp, NO_MATS, (int *)ram_window.pg8.MatVals, 1,
                        (unsigned int *)ram_window.pg8.AirCorDel,8);
        // ...  = aircor_eq(tmp2) + ... removed
    }

    tmp1 = outpc.map;

    serial();
    uctmp1 = ram4.FuelAlgorithm & 0xF;
    uctmp2 = ram4.FuelAlgorithm & 0xF0;
    uctmp3 = ram4.IgnAlgorithm & 0xF;
    uctmp4 = ram4.IgnAlgorithm & 0xF0;
    uctmp5 = ram4.extra_load_opts & 0xF;
    uctmp6 = ram4.extra_load_opts & 0xF0;
    if ((uctmp1 == 2) || (uctmp2 == 0x20) ||
        (uctmp1 == 6) || (uctmp2 == 0x60) ||
        (uctmp3 == 2) || (uctmp4 == 0x20) ||
        (uctmp3 == 6) || (uctmp4 == 0x60) ||
        (uctmp5 == 2) || (uctmp5 == 6) ||
        (uctmp6 == 0x20) || (uctmp6 == 0x60)) {

        tmp2 = (int) (((long) tmp1 * 1000) / outpc.baro);
    } else {
        tmp2 = 1000;
    }

    if ((uctmp1 == 6) || (uctmp2 == 0x60) ||
        (uctmp3 == 6) || (uctmp4 == 0x60) ||
        (uctmp5 == 6) || (uctmp6 == 0x60)) {
        tmp3 = calc_ITB_load(tmp2);
    }

    /* Fuel Load 1 */
    if (uctmp1 == 1) {
        outpc.fuelload = tmp1;  // kpa x 10, speed density
    } else if (uctmp1 == 2) {
        outpc.fuelload = tmp2; // % baro
    } else if (uctmp1 == 3) {
        outpc.fuelload = outpc.tps; // alpha-n
    } else if (uctmp1 == 4 || uctmp1 == 5) { // maf or mafload
        outpc.fuelload = outpc.mafload;
    } else if (uctmp1 == 6) { // ITB
        outpc.fuelload = tmp3;
    } else {
        /* somehow something is set wrong, just use map */
        outpc.fuelload = outpc.map;
    }

    if (uctmp2 == 0x10) {
        outpc.fuelload2 = tmp1;
    } else if (uctmp2 == 0x20) {
        outpc.fuelload2 = tmp2;
    } else if (uctmp2 == 0x30) {
        outpc.fuelload2 = outpc.tps;
    } else if (uctmp2 == 0x40 || uctmp2 == 0x50) {
        outpc.fuelload2 = outpc.mafload;
    } else if (uctmp2 == 0x60) {
        outpc.fuelload2 = tmp3;
    }

    /* Ign Load 1 */
    if (uctmp3 == 1) {
        outpc.ignload = outpc.map;
    } else if (uctmp3 == 2) {
        outpc.ignload = tmp2;
    } else if (uctmp3 == 3) {
        outpc.ignload = outpc.tps;
    } else if (uctmp3 == 5) {
        outpc.ignload = outpc.mafload;
    } else if (uctmp3 == 6) {
        outpc.ignload = tmp3;
    } else {
        outpc.ignload = outpc.fuelload;
    }

    if (uctmp4 == 0x10) {
        outpc.ignload2 = outpc.map;
    } else if (uctmp4 == 0x20) {
        outpc.ignload2 = tmp2;
    } else if (uctmp4 == 0x30) {
        outpc.ignload2 = outpc.tps;
    } else if (uctmp4 == 0x50) {
        outpc.ignload2 = outpc.mafload;
    } else if (uctmp4 == 0x60) {
        outpc.ignload2 = tmp3;
    }

    /* AFR load */
    if (uctmp5 == 1) {
        outpc.afrload = outpc.map;
    } else if (uctmp5 == 2) {
        outpc.afrload = tmp2;
    } else if (uctmp5 == 3) {
        outpc.afrload = outpc.tps;
    } else if (uctmp5 == 5) {
        outpc.afrload = outpc.mafload;
    } else if (uctmp5 == 6) {
        outpc.afrload = tmp3;
    } else {
        outpc.afrload = outpc.fuelload;
    }

    /* EAEload */
    if (uctmp6 == 0x10) {
        outpc.eaeload = outpc.map;
    } else if (uctmp6 == 0x20) {
        outpc.eaeload = tmp2;
    } else if (uctmp6 == 0x30) {
        outpc.eaeload = outpc.tps;
    } else if (uctmp6 == 0x50) {
        outpc.eaeload = outpc.mafload;
    } else if (uctmp6 == 0x60) {
        outpc.eaeload = tmp3;
    } else {
        outpc.eaeload = outpc.fuelload;
    }
}

void water_inj()
{
    /**************************************************************************
     **
     ** Water injection
     ** Can:
     ** Turn pump on/off at set points
     ** read from level/pressure or whatever switch to shutdown on system failure
     ** use slow-speed (duty based) or high-speed (injector type) valves
     **
     ** Uses various set points and a table
     ** in high-speed mode, valve runs as fast as tacho output
     **************************************************************************/
    if (ram4.water_freq & 0x01) {    // system enabled?
        if ((outpc.tps > ram4.water_tps)
            && (outpc.rpm > ram4.water_rpm)
            && (outpc.map > ram4.water_map)
            && (outpc.mat > ram4.water_mat)) {  // system active?

            if (pin_wipump) {  // using pump output
                SSEM0SEI;
                *port_wipump |= pin_wipump;
                CSEM0CLI;
            }

            if (pin_wiin && ((*port_wiin & pin_wiin) != pin_match_wiin)) {
                // safety input NOT on, e.g. low water level etc. (Might want to change polarity?)
                // i.e. grounded input means safe. If wire falls off the assumes no fluid and activates shutdown.
                if (maxafr_stat == 0) {
                    maxafr_stat = 1;    // mainloop code reads the stat and cut appropriately
                    maxafr_timer = 1;   // start counting
                }
                outpc.status8 |= STATUS8_WINJLOW;
                outpc.water_duty = 0; // Avoid drowning engine during shutdown
            } else {
                outpc.status8 &= ~STATUS8_WINJLOW;
            }

            if ((ram4.water_freq & 0x60) && (maxafr_stat == 0)) {    // using valve output
                outpc.water_duty =
                    intrp_2dctable(outpc.rpm, outpc.map, 8, 4,
                                   &ram_window.pg10.waterinj_rpm[0],
                                   &ram_window.pg10.waterinj_map[0],
                                   &ram_window.pg10.waterinj_duty[0][0], 0,
                                   10);
                if ((ram4.water_freq & 0x60) == 0x20) {      // using high speed output (see isr_pit.s)
                    unsigned long dtpred_local;
                    DISABLE_INTERRUPTS;
                    dtpred_local = dtpred;
                    ENABLE_INTERRUPTS;
                    if (outpc.water_duty > 99) {
                        water_pw = 10 + (dtpred_local / 50);    // make sure full on (bodge)
                    } else {
                        water_pw = 1 + (unsigned int) ((unsigned long) outpc.water_duty * dtpred_local / 5000); // 100 for % and 50 to convert to 50us units
                    }
                } else {    // low speed (set by swpwms in ms3_misc.c / isr_rtc.s)
                    water_pw = 0;
                    water_pw_cnt = 0;
                }
            }


        } else {                // system inactive
            if (pin_wipump) {  // using pump output
                SSEM0SEI;
                *port_wipump &= ~pin_wipump;
                CSEM0CLI;
            }

            outpc.water_duty = 0;
            water_pw = 0;
            water_pw_cnt = 0;

            if ((ram4.water_freq & 0x60) && pin_wivalve) {    // using valve output
                SSEM0SEI;
                *port_wivalve &= ~pin_wivalve;
                CSEM0CLI;
            }
        }
    }
}

void do_launch()
{
    int lt = 0, lr = 0, vlr = 0;
    unsigned int soft_lim = 65535, hard_lim = 65535;
    // 3 step
    {
        int step3 = 0;
        if (pin_3step && ((*port_3step & pin_3step) == pin_match_3step)) {
            // feature on and switch active (low)
            outpc.status3 |= STATUS3_3STEPIN;
            hard_lim = ram4.launch_hrd_lim3;
            soft_lim = ram4.launch_hrd_lim3 - ram4.launch_sft_zone;

            if (outpc.rpm > soft_lim) {
                step3 = ram4.launch_sft_deg3;
                outpc.status3 |= STATUS3_3STEP;
            } else {
                outpc.status3 &= ~STATUS3_3STEP;
            }
        } else {
            outpc.status3 &= ~(STATUS3_3STEPIN | STATUS3_3STEP);
        }
        outpc.step3_timing = step3;
    }

    // do launch in here too

    outpc.status3 &= ~STATUS3_LAUNCHON; // clear bit first
    if (ram4.launch_opt & 0x40) {

        /* pin status for UI only */
        if (pin_launch && ((*port_launch & pin_launch) == pin_match_launch)) {
            if (!(outpc.status2 & STATUS2_LAUNCHIN)) { // gone from no-press to being pressed
                outpc.status2 |= STATUS2_LAUNCHIN;
                if ( (ram4.launch_opt & 0x80) && (outpc.rpm > ram4.flats_arm)
                    && (((ram4.vss_opt0 & 0x03) == 0) /* VSS off */
                    || ((ram4.vss_opt0 & 0x03) && (outpc.vss1 > ram4.flats_minvss))) /* or VSS on and condition satisfied */
                    ) {
                    flagbyte21 |= FLAGBYTE21_FLATS_ARM;
                } else {
                    flagbyte21 &= ~FLAGBYTE21_FLATS_ARM;
                }
            }
        } else {
            outpc.status2 &= ~STATUS2_LAUNCHIN;
        }

        /* check tps */
        if ((outpc.tps >= ram4.launch_tps) && (outpc.status2 & STATUS2_LAUNCHIN)) {
            if (ram4.vss_opt0 & 0x03) { // VSS enabled, check limits
                if (!(flagbyte21 & FLAGBYTE21_FLATS_ARM)) { // not in flat-shift armed mode
                    if (outpc.vss1 <= ram4.launch_maxvss) {
                    // either below launch speed 
                        goto DO_LAUNCH;
                    }
                } else { // flat-shift is armed
                    if (outpc.vss1 >= ram4.flats_minvss) { //above flat-shift speed and rpm
                        goto DO_LAUNCH;
                    }
                }
            } else { // no VSS, go ahead
                goto DO_LAUNCH;
            }
        }

        // nothing active
        if (outpc.status2 & STATUS2_LAUNCH) {   // if we were on, then set nitrous delay timer
            outpc.launch_retard = 0;
            if ((ram4.launch_opt & 0x80) && (outpc.status2 & STATUS2_FLATSHIFT)) {
                n2o_act_timer = ram4.N2Odel_flat;       //flat shift timer
            } else {
                n2o_act_timer = ram4.N2Odel_launch;     //launch timer
                /* Note that this has moved, time is from end of LAUNCH only, not flat-shift */
                launch_timer = 0;        // boost/ retard launch delay timer
            }
            /* Traction perfect run */
            if ((outpc.status2 & STATUS2_FLATSHIFT) == 0) {
                if ( (((ram5.tc_opt & 6) == 0) && (outpc.vss1 <= ram5.tc_minvss)) /* Perfect run VSS */
                    || ((ram5.tc_opt & 6) == 4) ) { /* Perfect run RPM */
                    // only if going slow enough and weren't in flatshift
                    perfect_timer = 1; // set it rolling (strictly off by one)
                }
            }
            /* Line lock staging */
            if (flagbyte23 & FLAGBYTE23_LLSTG) {
                SSEM0SEI;
                *port_llstg_out &= ~pin_llstg_out;
                CSEM0CLI;
                flagbyte23 &= ~FLAGBYTE23_LLSTG;
            }
        }

        outpc.status2 &= ~(STATUS2_LAUNCH | STATUS2_FLATSHIFT); // turn off both
        flagbyte21 &= ~(FLAGBYTE21_FLATS_ARM | FLAGBYTE21_LAUNCHFC); // needs to be atomic

        goto NO_LAUNCH;

        /* ----- flow doesn't pass here ---- */

      DO_LAUNCH:
        launch_timer = 0x7fff;   // boost launch delay timer
        perfect_timer = 0;

        if (flagbyte21 & FLAGBYTE21_FLATS_ARM) {
            outpc.status2 |= STATUS2_FLATSHIFT;
            // use flat shift limits
            hard_lim = ram4.flats_hrd;
            soft_lim = ram4.flats_hrd - ram4.launch_sft_zone;

            if (outpc.rpm > soft_lim) {
                outpc.status3 |= STATUS3_LAUNCHON; // actually in launch mode, not just armed
                outpc.status2 |= STATUS2_LAUNCH; // most code uses this setbit
                lt = ram4.flats_deg;
            }
            if (outpc.rpm > ram4.launch_fcut_rpm) {
                tmp_pw1 = 0;
                tmp_pw2 = 0;
            }
        } else {
            if (ram4.launch_var_on & 0x1f) {       // using variable launch
                unsigned tmp_adc;
                tmp_adc = *port_launch_var;
                if (tmp_adc > 1023) {   // shouldn't ever happen
                    tmp_adc = 0;        // set low rpm limit as safety measure
                }
                hard_lim = ram4.launch_var_low
                    + (unsigned int) (((ram4.launch_var_up - ram4.launch_var_low)
                            * (unsigned long) tmp_adc) / 1024);
            } else {
                hard_lim = ram4.launch_hrd_lim;
            }
            soft_lim = hard_lim - ram4.launch_sft_zone;
            // use launch limits

            if (outpc.rpm > soft_lim) {
                outpc.status3 |= STATUS3_LAUNCHON;
                outpc.status2 |= STATUS2_LAUNCH; // most code uses this setbit
                lt = ram4.launch_sft_deg;
            }
        }
    }
    NO_LAUNCH:;
    /* apply launch retard timer */
    if ( ((outpc.status2 & (STATUS2_LAUNCH | STATUS2_FLATSHIFT)) == 0)
            && (ram4.launch_opt & 0x08) && (launch_timer < 0x7ffe)) {
        lr = intrp_1ditable(launch_timer, NUM_LAUNCHRETS,
        (int *) ram_window.pg11.launch_time, 0,
        (int *) ram_window.pg11.launch_retard, 11);
    }
    outpc.launch_timer = launch_timer;

    /* throttle stops */
    if (pin_tstop && (ram5.tstop_out & 0x1f)) {
        unsigned char st = 0;
        if (launch_timer > (ram5.tstop_delay + ram5.tstop_hold)) {
            st = 0;
        } else if (launch_timer > ram5.tstop_delay) {
            st = 1;
        }
        if (st) {
            *port_tstop |= pin_tstop;
        } else {
            *port_tstop &= ~pin_tstop;
        }
    }

    /* wheel speed based launch */
    if (ram4.launch_opt & (LAUNCH_OPT_VSS1 | LAUNCH_OPT_VSS2)) {
        unsigned int vss;
        if ((ram4.launch_opt & (LAUNCH_OPT_VSS1 | LAUNCH_OPT_VSS2)) == LAUNCH_OPT_VSS2) {
            vss = outpc.vss2;
        } else {
            vss = outpc.vss1;
        }
        if (vss < ram4.launchvss_minvss) { // very slow
            launchvsstimer = 0;
        }

        if (flagbyte20 & FLAGBYTE20_VSSLLO) { // currently disabled, exceeded end of curve
            if (vss < ram4.launchvss_minvss) {
                flagbyte20 &= ~FLAGBYTE20_VSSLLO; // clear the lockout
                outpc.status8 &= ~STATUS8_VSSLLO;
            }
        } else { // currently on
            RPAGE = tables[27].rpg;
            if ((vss > ram_window.pg27.vsslaunch_vss[9])
                || (ram4.launchvsstime && (launchvsstimer > ram4.launchvsstime))) {
                flagbyte20 |= FLAGBYTE20_VSSLLO;
               outpc.status8 |= STATUS8_VSSLLO;

            }
        }

        if ((outpc.tps > ram4.launch_tps) // TPS enough
            && ((outpc.status2 & (STATUS2_LAUNCH | STATUS2_FLATSHIFT)) == 0) // not in switch type LC or FS
            && (!(flagbyte20 & FLAGBYTE20_VSSLLO)) // and not locked out
            && (!((ram4.launch_opt & LAUNCH_OPT_GEAR) && (ram4.gear_method & 0x03) && (outpc.gear > ram4.launchvss_maxgear)))  // gear not too high
            ) {

            outpc.status8 |= STATUS8_VSSLON;

            hard_lim = intrp_1ditable(vss, 10,
                   (int *) ram_window.pg27.vsslaunch_vss, 0,
                   (unsigned int *) ram_window.pg27.vsslaunch_rpm, 27);
            soft_lim = hard_lim - ram4.launch_sft_zone;

            vlr = intrp_1ditable(vss, 10,
                   (int *) ram_window.pg27.vsslaunch_vss, 1,
                   (unsigned int *) ram_window.pg27.vsslaunch_retard, 27);
        } else {
            outpc.status8 &= ~STATUS8_VSSLON;
        }
    }

    /* above code has figured out limits, now check them */
    /* retard is set above */
    if (ram4.launchlimopt & LAUNCHLIMOPT_CUTSPK) {
        unsigned int soft_lim2;

        if (ram4.launchlimopt & LAUNCHLIMOPT_ADV) {
            soft_lim2 = hard_lim - ram4.launchcutzone;
        } else {
            soft_lim2 = soft_lim;
        }
        if (outpc.rpm > soft_lim2) {
            unsigned char sc_tmp;

            if (ram4.launchlimopt & LAUNCHLIMOPT_ADV) {
                if (!(outpc.status2 & STATUS2_FLATSHIFT)) {
                    /* for launch apply the higher RPM timing value */
                    lt = ram4.launchcuttiming;
                }

                if (outpc.status3 & STATUS3_LAUNCHON) {
                    /* over-ride the limiter type - reset to zero on each mainloop pass */
                    spklimiter_type = (ram4.launchlimopt >> 3) & 0x03;
                }
            }

            if ((outpc.rpm >= hard_lim) || (spklimiter_type > 1)) {
                /* Fixed types come on full stream at soft_lim2 */
                sc_tmp = 254; /* Full cut for random limit, but not for X/Y types. */
            } else {
                sc_tmp = (255L * (outpc.rpm - soft_lim2)) / (hard_lim - soft_lim2);
                if (spklimiter_type == 1) {
                    /* Square it, then normalise - for non-linear ramped response */
                    sc_tmp = ((unsigned int)sc_tmp * sc_tmp) / 256;
                }
            }

            if (sc_tmp > spkcut_thresh_tmp) {
                spkcut_thresh_tmp = sc_tmp;
            }
            flagbyte10 |= FLAGBYTE10_SPKCUTTMP;
        }
    }
    if (ram4.launchlimopt & LAUNCHLIMOPT_CUTFUEL)  { // fuel cut with hyst
        if (outpc.rpm > hard_lim) {
            flagbyte21 |= FLAGBYTE21_LAUNCHFC;
        } else if (outpc.rpm < soft_lim) {
            flagbyte21 &= ~FLAGBYTE21_LAUNCHFC;
        } // in between is hyst zone
    }
    /* Set outpc once here, defaults to zero if not set above.
     * Timing changes are actually applied in main */
    outpc.launch_timing = lt;
    outpc.launch_retard = lr;
    outpc.vsslaunch_retard = vlr;
}

void transbrake(void)
{
    /* transbrake control
    */
    if (pin_timed1_in && pin_timed1_out && (ram5.timedout1_in & 0x1f)) {
        if (tb_state == 0) {
            /* not pressed at the moment */
            if ((*port_timed1_in & pin_timed1_in) == pin_match_timed1_in) {
                tb_state = 1;
                *port_timed1_out |= pin_timed1_out;
                tcs_move = 1;
            }

        } else if ((tb_state >= 1) && (tb_state <= 5)) {
            /* was pressed */
            if ((*port_timed1_in & pin_timed1_in) != pin_match_timed1_in) {
                /* button now released */
                if (ram5.timedout1_offdelay) {
                    /* delay before brake release */
                    tb_timer = ram5.timedout1_offdelay;
                    tb_state = 10;
                } else {
                    /* release brake immediately */
                    *port_timed1_out &= ~pin_timed1_out;
                    tb_state = 0;
                }
            }
            /* TCS */
            if (pin_tcs && (ram5.tcs_in & 0x1f)) {
                if (tb_state == 1) {
                    /* TB button held down, check for interrupt button */
                    if (((*port_tcs & pin_tcs) == pin_match_tcs) && (tb_timer == 0)) {
                        tb_timer = ram5.tcs_offtime;
                        *port_timed1_out &= ~pin_timed1_out;
                        tb_state = 2;
                    }
                } else if (tb_state == 2) {
                    /* countdown during move */
                    if (tb_timer == 0) {
                        tb_timer = ram5.tcs_ontime;
                        *port_timed1_out |= pin_timed1_out;
                        tb_state = 3;
                    }
                } else if (tb_state == 3) {
                    /* countdown between moves */
                    if (tb_timer == 0) {
                        if (tcs_move >= ram5.tcs_moves) {
                            tb_state = 4;
                        } else {
                            tcs_move++;
                            tb_timer = ram5.tcs_offtime;
                            *port_timed1_out &= ~pin_timed1_out;
                            tb_state = 2;
                        }
                    }
                } else if (tb_state == 4) {
                    /* wait until button released */
                    if (((*port_tcs & pin_tcs) != pin_match_tcs)) {
                        tb_timer = 250; // minimum 250ms between presses (de-bounce)
                        tb_state = 1;
                    }
                }
            }

        } else if (tb_state == 10) {
            /* countdown to off */
            if (tb_timer == 0) {
                *port_timed1_out &= ~pin_timed1_out;
                tb_state = 0;
            }
        }
    }
}

void do_revlim_overboost_maxafr(void)
{
    unsigned int limit;
    limit = ram4.RevLimNormal2; /* Start with hard limit */

    if (ram4.RevLimOption & 0x8) {
        //check if within bounds of table, otherwise use normal limits
        RPAGE = tables[8].rpg;
        if (outpc.clt < ram_window.pg8.RevLimLookup[7]) {
            if (outpc.tps < ram4.TpsBypassCLTRevlim) {
                limit = intrp_1ditable(outpc.clt, 8,
                                       (int *) ram_window.pg8.RevLimLookup, 0,
                                       (unsigned int *) ram_window.pg8.RevLimRpm1, 8);
            }
        }
    }

    if (outpc.status7 & STATUS7_LIMP) {
        if (limit > ram5.cel_revlim) { /* Use lowest of the two */
            limit = ram5.cel_revlim;
        }
    }

    RevLimRpm2 = limit;
    RevLimRpm1 = limit - ram4.RevLimNormal2_hyst;

    /* random number based spark cut */
    if (ram4.RevLimOption & 0x4) {
        if (outpc.rpm > ram4.RevLimNormal2) {
            spkcut_thresh_tmp = 254;
            flagbyte10 |= FLAGBYTE10_SPKCUTTMP;
        } else if (outpc.rpm > RevLimRpm1) {
            unsigned char sc_tmp;
            if (outpc.rpm >= RevLimRpm2) {
                sc_tmp = 254;
            } else {
                sc_tmp = (255L * (outpc.rpm - RevLimRpm1)) / (RevLimRpm2 - RevLimRpm1);
                if (sc_tmp > 254) {
                    sc_tmp = 254;
                }
            }
            if (sc_tmp > spkcut_thresh_tmp) {
                spkcut_thresh_tmp = sc_tmp;
            }
            flagbyte10 |= FLAGBYTE10_SPKCUTTMP;
        }
        // no else, initialised to zero on each mainloop pass
    }

    if (ram4.OverBoostOption & 0x03) {
        int maxboost;
        unsigned char ob1 = 0;

        if ( (ram5.dualfuel_sw2 & 0x10) && (ram5.dualfuel_sw & 0x1)
            && ((ram5.dualfuel_opt & DUALFUEL_OPT_MODE_MASK) == DUALFUEL_OPT_MODE_FLEXBLEND) ) {
            maxboost = (int)((((long)ram4.OverBoostKpa * (100 - flexblend)) + ((long)ram4.OverBoostKpa2 * flexblend)) / 100);
        } else if (pin_tsw_ob && ((*port_tsw_ob & pin_tsw_ob) == pin_match_tsw_ob)) {
            maxboost = ram4.OverBoostKpa2;
        } else {
            maxboost = ram4.OverBoostKpa;
        }

        if (outpc.map > maxboost) {
            ob1 = 1;
        } else if ((ram4.OverBoostOption & 0x04) && (ram4.boost_ctl_settings & BOOST_CTL_CLOSED_LOOP)
            && ((outpc.map - outpc.boost_targ_1) > ram4.boosttol)) {
            ob1 = 1;
        } else if (outpc.map < (maxboost - ram4.OverBoostHyst)) {
            if ((ram4.OverBoostOption & 0x04) && (ram4.boost_ctl_settings & BOOST_CTL_CLOSED_LOOP)) {
                if (outpc.map < (outpc.boost_targ_1 + ram4.boosttol - ram4.OverBoostHyst)) {
                    ob1 = 2;
                }
            } else {
                ob1 = 2;
            }
        }

//        if ((ram4.boost_ctl_settings & BOOST_CTL_ON) && (ram5.boost_ctl_settings2 & BOOST_CTL_ON)) {
//            ob2 = ; // not checking input2 presently
//        }

        /* set/clear it once */
        if (ob1 == 1) { // 1 or 2 tripped it
            outpc.status2 |= STATUS2_OVERBOOST_ACTIVE;
        } else if (ob1 == 2) { // 1 and 2 both ok
            outpc.status2 &= ~STATUS2_OVERBOOST_ACTIVE;
        } // else leave alone

        if (ram4.OverBoostOption & 0x02) {
            if (outpc.status2 & STATUS2_OVERBOOST_ACTIVE) {
                spkcut_thresh_tmp = 255; // full cut
                flagbyte10 |= FLAGBYTE10_SPKCUTTMP;
            }
        }
    }

    /* Check for fuel cut */
    if (ram4.RevLimOption & 0x20) {
        if ((ram4.RevLimOption & 0x10) && (num_inj > 1)) {
            /* progressive fuel cut */
            if (outpc.rpm > (RevLimRpm2 - ram4.RevLimNormal2_hyst - 100)) {
                unsigned int step, b, base;
                base = RevLimRpm2 - ram4.RevLimNormal2_hyst;
                step = ram4.RevLimNormal2_hyst / (num_inj - 1);
                for (b = 0 ; b < num_inj ; b++) {
                    if (outpc.rpm > (base + (step * b))) {
                        skipinj_revlim |= fuelcut_array[num_inj - 1][b];
                    } else if (outpc.rpm < (base + (step * b) - 100)) {
                        skipinj_revlim &= ~fuelcut_array[num_inj - 1][b];
                    }
                }
            } else {
                skipinj_revlim = 0;
            }
        } else {
            /* hard fuel cut only */
            if (outpc.rpm > RevLimRpm2) {
                // Cut fuel for Over Rev
                flagbyte17 |= FLAGBYTE17_REVLIMFC;
            } else if (outpc.rpm < (RevLimRpm2 - ram4.RevLimNormal2_hyst)) {
                // restore fuel
                flagbyte17 &= ~FLAGBYTE17_REVLIMFC;
            }
        }
    }

    do_maxafr();
    if (maxafr_stat) {
        spkcut_thresh = 255;
        spkcut_thresh_tmp = 255;
        flagbyte10 |= FLAGBYTE10_SPKCUTTMP;
    }

    /* Check what bits are wanting to cut fuel and set/clear the master setbit */
    if ( (flagbyte17 & FLAGBYTE17_REVLIMFC) || (flagbyte17 & FLAGBYTE17_OVERRUNFC)
        || (maxafr_stat == 2)
        || (flagbyte21 & FLAGBYTE21_LAUNCHFC)
        || ((ram4.OverBoostOption & 0x01) && (outpc.status2 & STATUS2_OVERBOOST_ACTIVE))
        || ((outpc.engine & ENGINE_CRANK) && (outpc.tps > ram4.TPSWOT))
        || (flagbyte22 & FLAGBYTE22_SHUTDOWNACTIVE)
        ) {
        outpc.status3 |= STATUS3_CUT_FUEL;
    } else {
        outpc.status3 &= ~STATUS3_CUT_FUEL;
    }
}

void handle_ovflo(void)
{
    // check for long timers
    if (wheeldec_ovflo) {
        unsigned long wotmp;

        while ((TCNT > 0xfff8) || (TCNT < 6));
        // make sure not right at rollover

        wotmp = ((unsigned long) swtimer << 16) | TCNT;

        /* OVFLO_SPK and OVFLO_DWL removed as no longer used */

        if (wheeldec_ovflo & OVFLO_ROT_DWL) {
            if ((dwl_time_ovflo_trl.time_32_bits - wotmp) < 0x10000) {
                TC6 = dwl_time_ovflo_trl.time_16_bits[1];
                SSEM0SEI;
                TFLG1 = 0x40;       // clear ign OC interrupt flag
                TIE |= 0x40;
                CSEM0CLI;
                wheeldec_ovflo &= ~OVFLO_ROT_DWL;   // done overflow
            }
        }

        if (wheeldec_ovflo & OVFLO_ROT_SPK) {
            if ((spk_time_ovflo_trl.time_32_bits - wotmp) < 0x10000) {
                TC3 = spk_time_ovflo_trl.time_16_bits[1];
                SSEM0SEI;
                TFLG1 = 0x08;
                TIE |= 0x08;
                CSEM0CLI;
                wheeldec_ovflo &= ~OVFLO_ROT_SPK;
            }
        }
    }
}

/***************************************************************************
 **
 ** Determine spare port settings
 ** Programmable on/off outputs
 **
 **************************************************************************/
void handle_spareports(void)
{
    int ix;
    char ctmp1 = 0, ctmp2 = 0, cond1 = 0, cond12 = 0, cond2 = 0;
    unsigned char us1, us2;
    long tmp1, tmp2 = 0;

    RPAGE = tables[23].rpg; // need access to the tuning data in paged ram

    for (ix = 0; ix < NPORT; ix++) {
        if (ram_window.pg23.spr_port[ix]) {
            cond1 = ram_window.pg23.condition1[ix];
            cond12 = ram_window.pg23.cond12[ix];
            cond2 = ram_window.pg23.condition2[ix];

            // Evaluate first condition
            if (ram_window.pg23.out_byte1[ix] == 0x01) { // signed char
                tmp1 = *((char *) (&outpc) + ram_window.pg23.out_offset1[ix]);
            } else if (ram_window.pg23.out_byte1[ix] == 0x81) { // unsigned char
                tmp1 = *((unsigned char *) (&outpc) + ram_window.pg23.out_offset1[ix]);
            } else if (ram_window.pg23.out_byte1[ix] == 0x02) { // signed int
                tmp1 = *(int *) ((char *) (&outpc) + ram_window.pg23.out_offset1[ix]);
            } else if (ram_window.pg23.out_byte1[ix] == 0x82) { // unsigned int
                tmp1 = *(unsigned int *) ((char *) (&outpc) + ram_window.pg23.out_offset1[ix]);
            } else {
                tmp1 = 0;
            }
            us1 = ram_window.pg23.out_byte1[ix] & 0x80; // unsigned flag

            if (cond1 == '&') { // bitwise AND
                /* Behaviour changed 2014-10-13 - hyst now used as match criteria */
                tmp1 = ((tmp1 & ram_window.pg23.thresh1[ix]) == ram_window.pg23.hyst1[ix]);
            } else {
                long t;
                if (us1) {
                    t = (long)(unsigned int)ram_window.pg23.thresh1[ix];
                } else {
                    t = (long)(int)ram_window.pg23.thresh1[ix];
                }
                tmp1 = tmp1 - t;
            }

            if (cond1 == '<') {
                tmp1 = -tmp1;   //  convert < condition to same as > condition
            }
            /* assume hyst value doesn't overflow sign bit */
            if (cond1 == '=') {
                if ((tmp1 >= -(long)ram_window.pg23.hyst1[ix]) && (tmp1 <= (long)ram_window.pg23.hyst1[ix]))
                    ctmp1 = 1;  // 1st condition true
            } else if (tmp1 > 0) {
                ctmp1 = 1;      // 1st condition true
            } else {
                ctmp1 = 0;      // 1st condition false
            }
            // Evaluate second condition if there is one
            if (cond12 != ' ') {
                if (ram_window.pg23.out_byte2[ix] == 0x01) { // signed char
                    tmp2 = *((char *) (&outpc) + ram_window.pg23.out_offset2[ix]);
                } else if (ram_window.pg23.out_byte2[ix] == 0x81) { // unsigned char
                    tmp2 = *((unsigned char *) (&outpc) + ram_window.pg23.out_offset2[ix]);
                } else if (ram_window.pg23.out_byte2[ix] == 0x02) { // signed int
                    tmp2 = *(int *) ((char *) (&outpc) + ram_window.pg23.out_offset2[ix]);
                } else if (ram_window.pg23.out_byte2[ix] == 0x82) { // unsigned int
                    tmp2 = *(unsigned int *) ((char *) (&outpc) + ram_window.pg23.out_offset2[ix]);
                } else {
                    tmp2 = 0;
                }
                us2 = ram_window.pg23.out_byte2[ix] & 0x80; // unsigned flag

                if (cond2 == '&') { // bitwise AND
                    /* Behaviour changed 2014-10-13 - hyst now used as match criteria */
                    tmp2 = ((tmp2 & ram_window.pg23.thresh2[ix]) == ram_window.pg23.hyst2[ix]);
                } else {
                    long t;
                    if (us2) {
                        t = (long)(unsigned int)ram_window.pg23.thresh2[ix];
                    } else {
                        t = (long)(int)ram_window.pg23.thresh2[ix];
                    }
                    tmp2 = tmp2 - t;
                }

                if (cond2 == '<') {
                    tmp2 = -tmp2;       //  convert < condition to same as > condition
                }
                if (cond2 == '=') {
                    if ((tmp2 >= -(long)ram_window.pg23.hyst2[ix])
                        && (tmp2 <= (long)ram_window.pg23.hyst2[ix]))
                        ctmp2 = 1;      // 2nd condition true
                } else if (tmp2 > 0) {
                    ctmp2 = 1;  // 2nd condition true
                } else {
                    ctmp2 = 0;  // 2nd condition false
                }
            }
            // Evaluate final condition
            if (((cond12 == '&') && (ctmp1 && ctmp2)) ||
                ((cond12 == '|') && (ctmp1 || ctmp2)) ||
                ((cond12 == ' ') && ctmp1)) {
                char pv = ram_window.pg23.port_val[ix];
                if (lst_pval[ix] != pv) {
                    set_spr_port((char) ix, pv);
                    lst_pval[ix] = pv;
                }
            } else {
                // Evaluate hysteresis conditions
                if ((cond1 == '>')
                    || (cond1 == '<')) {
                    tmp1 = -tmp1 - ram_window.pg23.hyst1[ix];
                }
                if ((cond1 == '=') || (cond1 == '&')) {
                    ctmp1 = 1 - ctmp1;  // 1st hyst. condition opposite of set cond
                } else if (tmp1 > 0) {
                    ctmp1 = 1;  // 1st hysteresis condition true
                } else {
                    ctmp1 = 0;  // 1st hysteresis condition false
                }
                if (cond12 != ' ') {
                    if ((cond2 == '>')
                        || (cond2 == '<'))
                        tmp2 = -tmp2 - ram_window.pg23.hyst2[ix];
                    if ((cond2 == '=') || (cond2 == '&')) {
                        ctmp2 = 1 - ctmp2;      // 2nd hyst. condition opposite of set cond
                    } else if (tmp2 > 0) {
                        ctmp2 = 1;      // 2nd hysteresis condition true
                    } else {
                        ctmp2 = 0;      // 2nd hysteresis condition false
                    }
                }
                // Evaluate final hysteresis condition
                if (((cond12 == '&') && (ctmp1 || ctmp2)) ||
                    ((cond12 == '|') && (ctmp1 && ctmp2)) ||
                    ((cond12 == ' ') && ctmp1)) {
                    char pv = 1 - ram_window.pg23.port_val[ix];
                    if (lst_pval[ix] != pv) {
                        set_spr_port((char) ix, pv);
                        lst_pval[ix] = pv;
                    }
                }
            }                   // end eval of hysteresis conditions
        }                       // end if spr_port
    }
}

/***************************************************************************
 **
 ** EGT
 **
 **************************************************************************/
void do_egt(void)
{
    int ix, numegt;
    unsigned int add;
    unsigned char melt;

    add = 0;
    melt = 0;

    numegt = ram4.egt_num & 0x1f;
    if (numegt > NUM_CHAN) {
        numegt = NUM_CHAN;
    }

    if (numegt == 0) {
        return;
    }

    for (ix = 0; ix < numegt ; ix++) {
        int egt;
        //convert to temp units
        if (ram4.egtport[ix] & 0x1f) {
            egt = ram4.egt_temp0 + (((long)*port_egt[ix] * (long)(ram4.egt_tempmax - ram4.egt_temp0)) / 1023);
        } else {
            egt = 0;
        }
        if ((egt > ram4.egt_tempmax) || (egt < ram4.egt_temp0)) {
            egt = -1000; // nonsense failsafe
        }
        outpc.egt[ix] = egt;
        if (ram4.egt_conf & 0x01) { // are 'actions' enabled
            if (stat_egt[ix] == 0) { // ignore any broken channels
                if (egt > ram4.egt_warn) {
                    add |= twopow[ix]; /* record bitfield of any EGTs over the warning temp */
                }
                if (egt > ram4.egt_max) {
                    melt = 1;
                }
            }
        }
    }

    egt_warn = add; /* store it so that ms3_inj.c can apply fuel per channel if desired */

    if (add) {
        if (!(outpc.status6 & STATUS6_EGTSHUT)) {
            flagbyte9 |= FLAGBYTE9_EGTADD;
        } else {
            flagbyte9 &= ~FLAGBYTE9_EGTADD;
        }
        outpc.status6 |= STATUS6_EGTWARN;
        if ((egt_timer == 0) && (maxafr_stat == 0) && (ram4.egt_conf & 0x02)) {
            egt_timer = 1; // only start this timer if not in shutdown mode
        }
    } else {
        flagbyte9 &= ~FLAGBYTE9_EGTADD;
        outpc.status6 &= ~STATUS6_EGTWARN;
        flagbyte9 &= ~FLAGBYTE9_EGTADD;
        egt_timer = 0; // cancel it
    }

    if (ram4.egt_conf & 0x02) { // shutdown enabled
        if (melt || (egt_timer > ram4.egt_time)) {
            flagbyte9 |= FLAGBYTE9_EGTMELT;
            outpc.status6 |= STATUS6_EGTSHUT;
            flagbyte9 &= ~FLAGBYTE9_EGTADD;
            egt_timer = ram4.egt_time + 1; // hold at this time, go to shut down
            if (maxafr_stat == 0) {
                maxafr_stat = 1;    // mainloop code reads the stat and cut appropriately
                maxafr_timer = 1;   // start counting
            }
        } else {
            flagbyte9 &= ~FLAGBYTE9_EGTMELT;
            outpc.status6 &= ~STATUS6_EGTSHUT;
        }
    }
}


/***************************************************************************
 **
 ** Generic sensors
 **
 **************************************************************************/
void do_sensors()
{
    unsigned char i;
    if (((unsigned int)lmms - sens_time) > 78) { // every 10ms
        sens_time = (unsigned int)lmms;
    } else {
        return;
    }
    for (i = 0; i <= 15 ; i++) {
        unsigned char s;
        s = ram5.sensor_source[i] & 0x1f;
#ifdef MS3PRO
        if (i == 15) {
            s = 1;
        }
#endif
        if (s) {
            int val, last_val;
            unsigned char t;
            // grab raw value
            val = *port_sensor[i];

            // grab last val
            last_val = outpc.sensors[i];

            // do transformation
            t = ram5.sensor_trans[i] & 0x07;
#ifdef MS3PRO
            if (i == 15) {
                /* force this to internal temp sensor */
                val = ATD0DR6;
                t = 7;
            }
#endif
            if (t == 1) {
                long range, lval;
                int min, max;

                min = ram5.sensor_val0[i];
                max = ram5.sensor_max[i];
                range = (long)max - (long)min;
                if (range > 0x7fff) {
                    lval = (long)min + ((range * val) / 1023);
                    if (lval > 0x7fff) {
                        val = 0x7fff;
                    } else if (lval < -0x7fffl) {
                        val = -0x7fff;
                    } else {
                        val = lval;
                    }
                } else {
                    __asm__ __volatile__
                    ("ldd %1\n"
                     "subd %2\n"
                     "ldy %3\n"
                     "emuls\n"
                     "ldx #1023\n"
                     "edivs\n"
                     "addy  %2\n"
                    :"=y"(val)
                    :"m"(max), "m"(min), "m"(val)
                    :"d", "x");
                }
                
            } else if (t == 2) {
                __asm__ __volatile__
                ("ldd    %1\n"
                 "subd   %2\n"
                 "ldy    %3\n"
                 "emul\n"
                 "ldx    #1023\n"
                 "ediv\n"
                 "addy   %2\n"
                 :"=y"(val)
                 :"m"(ram4.mapmax), "m"(ram4.map0), "m"(val)
                 :"d", "x");


            } else if (t == 3) {
                GPAGE = 0x10;
                __asm__ __volatile__
                ("aslx\n"
                 "addx #0x4000\n"    // cltfactor_table address
                 "gldy 0,x\n"
                 "ldd %2\n"
                 "emul\n"
                 "ldx #100\n"
                 "ediv\n"
                 "addy   %3\n"
                :"=y"(val)
                :"x"(val), "m"(ram4.cltmult),"m"(ram4.clt0)
                :"d");
                if (ram5.sensor_temp & 0x01) {
                    val = ((val -320) * 5) / 9;
                }

            } else if (t == 4) {
                GPAGE = 0x10;
                __asm__ __volatile__
                ("aslx\n"
                 "addx #0x4800\n"    // matfactor_table address
                 "gldy 0,x\n"
                 "ldd %2\n"
                 "emul\n"
                 "ldx #100\n"
                 "ediv\n"
                 "addy   %3\n"
                :"=y"(val)
                :"x"(val), "m"(ram4.matmult),"m"(ram4.mat0)
                :"d");
                if (ram5.sensor_temp & 0x01) {
                    val = ((val -320) * 5) / 9;
                }

            } else if (t == 5) {
                GPAGE = 0x10;
                __asm__ __volatile__
                ("addx #0x5000\n"     //egofactor_table address
                 "gldab 0,x\n"
                 "clra\n"
                 "ldy %2\n"
                 "emul\n"
                 "ldx #100\n"
                 "ediv\n"
                 "addy %3\n"
                :"=y"(val)
                :"x"(val), "m"(ram4.egomult), "m"(ram4.ego0)
                :"d");

            } else if (t == 6) {
                GPAGE = 0x10;
                __asm__ __volatile__
                ("aslx\n"
                 "addx #0x5400\n"     //matfactor_table address
                 "gldy 0,x\n"
                :"=y"(val)
                :"x"(val)
                :"d");

            } else if (t == 7) {
                GPAGE = 0x10;
                __asm__ __volatile__
                ("aslx\n"
                 "addx #0x5c00\n"     //gmfactor_table address
                 "gldy 0,x\n"
                :"=y"(val)
                :"x"(val)
                :"d");

#ifdef MS3PRO
                if (i == 15) {
                    /* this adjustment makes the temp sensor very close to the GM calibration over the working range */
                    val -= 110;
                }
#endif
                if (ram5.sensor_temp & 0x01) {
                    val = ((val -320) * 5) / 9;
                }

            } else { //no transformation = raw
                val *= 10; // (display is in 0.1 units)
            }

            // lag factor
            __asm__ __volatile__
            ("ldd %1\n"
             "subd %3\n"
             "tfr d,y\n"
             "clra\n"
             "ldab %2\n" // it is a uchar
             "emuls\n"
             "ldx #100\n"
             "edivs\n"
             "addy %3\n"
            :"=y"(val)
            :"m"(val), "m"(ram5.sensorLF[i]), "m"(last_val)
            :"d", "x");

            // store result
            outpc.sensors[i] = val;
        }
    }
}

void gearpos()
{
/***************************************************************************
 **
 ** Gear detection
 **
 ** Selection options: Off, RPM/VSS, Analogue, CAN VSS
 ** For RPM/VSS the gear ratio and final drive ratio table is used
 ** With analogue, gear position is indicated by input voltage
 ** CAN is a direct gear number captured from a remote board
 **************************************************************************/
    if ((ram4.gear_method & 0x03) == 1) {
        // in this mode calculate gear from RPM/VSS factor
        // output shaft speed = mph / wheel circumference * fdratio (* scaler)
        // gear ratio = rpm / output shaft speed
        // then compare to gear ratio table
        unsigned int expect_rpm[6], x;

        if (outpc.vss1 < 2) {
            outpc.gear = 0;
            return;
        } else if (outpc.vss1 < 5) {
            return;
        }

        /* build array of expected rpms per gear */
        for (x = 0 ; x < ram4.gear_no ; x++) {
            expect_rpm[x] = (unsigned int)((ram4.gear_ratio[x] * (unsigned long)outpc.vss1 * gear_scale) / 10000);
        }

        if (outpc.rpm > expect_rpm[0]) {
            /* first gear or free-revving */
            outpc.gear = 1;
            return;
        }

        if (outpc.rpm < expect_rpm[ram4.gear_no - 1]) {
            /* top gear or coasting in neutral */
            outpc.gear = ram4.gear_no;
            return;
        }

        // now compare against actual rpms
        // Exit as soon as decided on gear or indeterminate
        // note in gear_ratio[] 0 = 1st etc.
        
        // loop to check other gears
        for (x = 0 ; x < ram4.gear_no ; x++) {
            unsigned int low_rpm, high_rpm;
            if (x > 0) {
                high_rpm = expect_rpm[x] + ((expect_rpm[x - 1] - expect_rpm[x]) / 3);
            } else {
                high_rpm = expect_rpm[0];
            }
            if (x < ((unsigned int)ram4.gear_no - 1)) {
                low_rpm = expect_rpm[x] - ((expect_rpm[x] - expect_rpm[x + 1]) / 3);
            } else {
                low_rpm = expect_rpm[ram4.gear_no - 1];
            }

            if ((outpc.rpm > low_rpm) && (outpc.rpm < high_rpm)) {
                outpc.gear = x + 1;
                return;
            }
        }
        /* if in-between, then outpc.gear remains unchanged */
        return;

    } else if (((ram4.gear_method & 0x03) == 2) && port_gearsel) {
        unsigned int gear_v, gear_lowv, gear_highv, x, plus;
        
        // in this mode the 0-5V signal tells us the gear we are in

//       gear_v  = *port_gearsel / 1023 * 500
        __asm__ __volatile__ (
        "ldy #500\n"
        "emul\n"
        "ldx #1023\n"
        "ediv\n"
        : "=y" (gear_v)
        : "d" (*port_gearsel)
        );

        if (ram4.gearv[1] < ram4.gearv[ram4.gear_no]) {
            gear_lowv = 1;
            gear_highv = ram4.gear_no;
            plus = 1;
        } else {
            gear_lowv = ram4.gear_no;
            gear_highv = 1;
            plus = 0xffff;
        }

        // is neutral low or high?
        // note in gearv[] 0 = neutral, 1 = 1st etc.
        if (ram4.gearv[0] > ram4.gearv[gear_highv]) {
            unsigned int v_diff, v_high;
            // high neutral
            v_diff = (ram4.gearv[0] - ram4.gearv[gear_highv]) >> 2;
            v_high = ram4.gearv[0] - v_diff;
            if (gear_v > v_high) {
                outpc.gear = 0;
                return;
            }
        } else {
            unsigned int v_diff, v_low;
            // low neutral
            v_diff = (ram4.gearv[gear_lowv] - ram4.gearv[0]) >> 2;
            v_low = ram4.gearv[0] + v_diff;
            if (gear_v < v_low) {
                outpc.gear = 0;
                return;
            }
        }

        // loop to selection value
        for (x = 1; x <= ram4.gear_no ; x++) {
            int v_diff, v_low, v_high;
            // gap below
            if (x == gear_lowv) { // lowest value
                v_diff = (ram4.gearv[x + plus] - ram4.gearv[x]) >> 2; // 25% // use gap above only
                v_low = ram4.gearv[gear_lowv] - v_diff;
                v_high = ram4.gearv[gear_lowv] + v_diff;
            } else if (x == gear_highv) { // highest value
                v_diff = (ram4.gearv[x] - ram4.gearv[x - plus]) >> 2; // 25% // use gap below only
                v_low = ram4.gearv[gear_highv] - v_diff;
                v_high = ram4.gearv[gear_highv] + v_diff;
            } else {
                v_diff = (ram4.gearv[x] - ram4.gearv[x - plus]) >> 2; // 25% // use gap below
                v_low = ram4.gearv[x] - v_diff;
                v_diff = (ram4.gearv[x + plus] - ram4.gearv[x]) >> 2; // 25% // use gap above
                v_high = ram4.gearv[x] + v_diff;
            }

            if (((int)gear_v > v_low) && ((int)gear_v < v_high)) {
                outpc.gear = x;
                return;
            }
        } // end for
    } else if ((ram4.gear_method & 0x03) == 3) {
        outpc.gear = datax1.gear; // grab the CAN gear defined on the CAN parameters
    }
}

void calcvssdot()
{
    RPAGE = RPAGE_VARS1;

    vss_cnt++;

    if (vss_cnt < ram4.vssdot_int) { // sample every X*10ms
        return;
    } else {
        int vsscalcno;
        vss_cnt = 0;

        for (vsscalcno = 0; vsscalcno < 2 ; vsscalcno++) {
            int vssi;
            long vssdot_sumx, vssdot_sumx2, vssdot_sumy, vssdot_sumxy;
            long toprow, btmrow;
            unsigned long vssdot_x; // was uint

            v1.vssdot_data[0][0][vsscalcno] = (unsigned int)lmms; // store actual 16 bit time to first pt in array
            v1.vssdot_data[0][1][vsscalcno] = *((unsigned int*)&outpc.vss1 +vsscalcno); // store speed

            vssdot_sumx = 0;
            vssdot_sumx2 = 0;
            vssdot_sumy = 0;
            vssdot_sumxy = 0;

            vssi = 0;
            // only use six datapoints for this. Array defined as larger
            while ((vssi < 6) && (vssdot_sumxy < 10000000)) {
                vssdot_x = v1.vssdot_data[0][0][vsscalcno] - v1.vssdot_data[vssi][0][vsscalcno]; // relative time to keep numbers smaller
                vssdot_sumx += vssdot_x;
                vssdot_sumy += v1.vssdot_data[vssi][1][vsscalcno];
                vssdot_sumx2 += ((unsigned long)vssdot_x * vssdot_x); // overflow if vssdot_x > 65535
                vssdot_sumxy += ((unsigned long)vssdot_x * v1.vssdot_data[vssi][1][vsscalcno]);
                vssi++;
            }

            toprow = vssdot_sumxy - (vssdot_sumx * (vssdot_sumy / vssi)); // divide sumy to help overflow
            btmrow = vssdot_sumx2 - (vssdot_sumx * vssdot_sumx / vssi);

            btmrow = btmrow / 10; // allows top row to be 10x less to reduce overflow.
            toprow = (-toprow * 781) / btmrow;
            if (toprow > 32767) {
                toprow = 32767;
            } else if (toprow < -32767) {
                toprow = -32767;
            }

/*
            // very low vssdot
            if ((toprow > -150) && (toprow < 150)) { // these factors would need changing if code ever used
                long tmp_top, tmp_btm;
                // see how it compares to 20 positions
                tmp_btm = vssdot_data[0][0][vsscalcno] - vssdot_data[VSSDOT_N-1][0][vsscalcno];
                tmp_top = (int)vssdot_data[0][1][vsscalcno] - (int)vssdot_data[VSSDOT_N-1][1][vsscalcno];
                tmp_btm = tmp_btm / 10; // allows top row to be 10x less to reduce overflow.
                tmp_top = (tmp_top * 781) / tmp_btm;
                if (tmp_top > 32767) {
                    tmp_top = 32767;
                } else if (tmp_top < -32767) {
                    tmp_top = -32767;
                }
                // use lesser magnitude of two
                if (long_abs(tmp_top) < long_abs(toprow)) {
                    toprow = tmp_top;
                }
            }
*/
            toprow = 30L * (toprow - *((int*)&outpc.vss1dot +vsscalcno)); // relies on vss1dot, vss2dot following each other
            if ((toprow > 0) && (toprow < 100)) {
                toprow = 100;
            } else if ((toprow < 0) && (toprow > -100)) {
                toprow = -100;
            }
            *((int*)&outpc.vss1dot +vsscalcno) += (int)(toprow / 100);

            //shuffle data forwards by one
            for (vssi = VSSDOT_N - 1; vssi > 0 ; vssi--) {
                v1.vssdot_data[vssi][0][vsscalcno] = v1.vssdot_data[vssi - 1][0][vsscalcno];
                v1.vssdot_data[vssi][1][vsscalcno] = v1.vssdot_data[vssi - 1][1][vsscalcno];
            }
        }
    }
}

void accelerometer()
{
    signed int tmp1, tmp2, tmp3;
//need (more!) lag factors
    if (((unsigned int)lmms - accxyz_time) > 78) { // every 10ms
        accxyz_time = (unsigned int)lmms;
    } else {
        return;
    }
    if (ram4.accXport) {
        tmp1 = (ram4.accXcal1 + ram4.accXcal2) >> 1; // mid point
        tmp2 = (ram4.accXcal2 - ram4.accXcal1) >> 1; // 1g range
//        tmp3 = (((int)*accXport - tmp1) * 1000L) / tmp2; // gives g x 1000
        __asm__ __volatile__ (
        "ldy #9810\n" // build in 'g'
        "emuls\n"
        "ldx %2\n"
        "edivs\n"
        :"=y" (tmp3)
        :"d" ((int)*accXport - tmp1), "m" (tmp2)
        :"x"
        );

        //lag factor
        __asm__ __volatile__ (
        "suby   %3\n"
        "clra\n"
        "ldab    %2\n" // it is a uchar
        "emuls\n"
        "ldx     #100\n"
        "edivs\n"
        "addy    %3\n"
        :"=y"(outpc.accelx)
        :"y"(tmp3), "m"(ram4.accxyzLF), "m"(outpc.accelx)
        :"d", "x");

    }
    if (ram4.accYport) {
        tmp1 = (ram4.accYcal1 + ram4.accYcal2) >> 1; // mid point
        tmp2 = (ram4.accYcal2 - ram4.accYcal1) >> 1; // 1g range
//        outpc.accely = (((int)*accYport - tmp1) * 1000L) / tmp2; // gives g x 1000
        __asm__ __volatile__ (
        "ldy #9810\n"
        "emuls\n"
        "ldx %2\n"
        "edivs\n"
        :"=y" (tmp3)
        :"d" ((int)*accYport - tmp1), "m" (tmp2)
        :"x"
        );

        //lag factor
        __asm__ __volatile__ (
        "suby   %3\n"
        "clra\n"
        "ldab    %2\n" // it is a uchar
        "emuls\n"
        "ldx     #100\n"
        "edivs\n"
        "addy    %3\n"
        :"=y"(outpc.accely)
        :"y"(tmp3), "m"(ram4.accxyzLF), "m"(outpc.accely)
        :"d", "x");

    }
    if (ram4.accZport) {
        tmp1 = (ram4.accZcal1 + ram4.accZcal2) >> 1; // mid point
        tmp2 = (ram4.accZcal2 - ram4.accZcal1) >> 1; // range
//        outpc.accelz = (((int)*accZport - tmp1) * 1000L) / tmp2; // gives g x 1000
        __asm__ __volatile__ (
        "ldy #9810\n"
        "emuls\n"
        "ldx %2\n"
        "edivs\n"
        :"=y" (tmp3)
        :"d" ((int)*accZport - tmp1), "m" (tmp2)
        :"x"
        );

        //lag factor
        __asm__ __volatile__ (
        "suby   %3\n"
        "clra\n"
        "ldab    %2\n" // it is a uchar
        "emuls\n"
        "ldx     #100\n"
        "edivs\n"
        "addy    %3\n"
        :"=y"(outpc.accelz)
        :"y"(tmp3), "m"(ram4.accxyzLF), "m"(outpc.accelz)
        :"d", "x");

    }
}

void ck_log_clr(void)
{
/* Check for clearing trigger/tooth logger buffer */
    if (flagbyte8 & FLAGBYTE8_LOG_CLR) {
        if ((page >= 0xf0) && (page <= 0xf4)) { // double check
            unsigned char tmp_rpage;
            tmp_rpage = RPAGE;
            RPAGE = TRIGLOGPAGE;
            __asm__ __volatile__ (
            "ldd   #512\n"
            "tthclr:\n"
            "clrw   2,y+\n"
            "dbne   d, tthclr\n"
            : 
            : "y"(TRIGLOGBASE)
            : "d");
            RPAGE = tmp_rpage;
        }
        log_offset = 0;
        if (page == 0xf0) {
            flagbyte0 |= FLAGBYTE0_TTHLOG;
        } else if (page == 0xf1) {
            flagbyte0 |= FLAGBYTE0_TRGLOG;
        } else if ((page == 0xf2) || (page == 0xf3)) {
            flagbyte0 |= FLAGBYTE0_COMPLOG;
        } else if (page == 0xf4) {
            flagbyte0 |= FLAGBYTE0_MAPLOGARM; // gets started in ISR
        } else if (page == 0xf5) {
            flagbyte0 |= FLAGBYTE0_MAFLOGARM; // gets started in ISR
        } else if ((page == 0xf6) || (page == 0xf7) || (page == 0xf8)) {
            flagbyte0 |= FLAGBYTE0_ENGLOG;
        }
        flagbyte8 &= ~FLAGBYTE8_LOG_CLR;
    }
}

long long_abs(long in)
{
    if (in < 0) {
        return -in;
    } else {
        return in;
    }
}


void shifter()
{
    /**************************************************************************
     ** Bike type ignition cut on shift and optional air-shift output
     **************************************************************************/
    unsigned int tmp_fc;
    tmp_fc = 0;
    if ((ram5.shift_cut & SHIFT_CUT_ON) == 0) {
        outpc.status3 &= ~STATUS3_BIKESHIFT;
    } else {
        // state machine
        if (shift_cut_phase == 0) {
            if ((outpc.rpm > ram5.shift_cut_rpm) && (outpc.tps > ram5.shift_cut_tps) &&   // base conditions AND
                ((pin_shift_cut_in && ((*port_shift_cut_in & pin_shift_cut_in) == pin_shift_cut_match))  // button pressed
                || ((ram5.shift_cut & SHIFT_CUT_AUTO) && (outpc.gear) && (outpc.gear < 6) && (outpc.rpm > ram5.shift_cut_rpmauto[outpc.gear - 1])) ) ) { // OR auto
                shift_cut_phase = 1;
                SSEM0SEI;
                *port_shift_cut_out |= pin_shift_cut_out; // enable solenoid
                CSEM0CLI;
                shift_cut_timer = ram5.shift_cut_delay;
            }
        } else if ((shift_cut_phase == 1) && (shift_cut_timer == 0)) {
            shift_cut_timer = ram5.shift_cut_time;
            if (ram5.shift_cut & SHIFT_CUT_GEAR) {
                shift_cut_timer += ram5.shift_cut_add[outpc.gear - 1];
            }
            shift_cut_phase = 2;
        } else if (shift_cut_phase == 2) {            
            if (shift_cut_timer == 0) {
                shift_cut_timer = ram5.shift_cut_soldelay;
                shift_cut_phase = 3;
            } else {
                flagbyte10 |= FLAGBYTE10_SPKCUTTMP; // have to do this every loop
                spkcut_thresh_tmp = 255; //full cut
                if (ram5.shift_cut & SHIFT_CUT_FUEL) {
                    tmp_fc = 65535;
                }
            }
        } else if ((shift_cut_phase == 3) && (shift_cut_timer == 0)) {
            SSEM0SEI;
            *port_shift_cut_out &= ~pin_shift_cut_out; // disable solenoid
            CSEM0CLI;
            shift_cut_timer = ram5.shift_cut_reshift;
            shift_cut_phase = 4;
        } else if ((shift_cut_phase == 4) && (shift_cut_timer == 0)) {
            //check button not already pressed
            if (pin_shift_cut_in && ((*port_shift_cut_in & pin_shift_cut_in) != pin_shift_cut_match)) {
                shift_cut_phase = 0; // back to the start
            }
        }
        if (shift_cut_phase) {
            outpc.status3 |= STATUS3_BIKESHIFT;
        } else {
            outpc.status3 &= ~STATUS3_BIKESHIFT;
        }
    }
    skipinj_shifter = tmp_fc;
}

int calc_outpc_input(unsigned char sz, unsigned int off)
{
    int load;

    if (sz == 0x01) {  // signed 8 bit
        load = *((char *) &outpc + off);
    } else if (sz == 0x81) { // unsigned 8 bit
        load = *((unsigned char *) &outpc + off);
    } else if (sz == 0x02) {   // signed 16 bit
        load = *((int *)&(*((unsigned char *) &outpc + off)));
    } else if (sz == 0x82) {   // unsigned 16 bit
        unsigned long val;
        val = *((unsigned int *)&(*((unsigned char *) &outpc + off)));
        if (val > 32767) { // too large for signed 16bit
            load = 32767;
        } else {
            load = (int)val;
        }
    } else if (sz == 4) {   // signed 32 bit
        long *ul_tmp_ad, val;

        ul_tmp_ad = (long *)&(*((unsigned char *) &outpc + off));
        DISABLE_INTERRUPTS;
        val = *((long *) ul_tmp_ad);        // nice quick ASM
        ENABLE_INTERRUPTS;
        if (val < -32767) {
            load = -32767;
        } else if (val > 32767) {
            load = 32767;
        } else {
            load = val;
        }
    } else if (sz == 0x84) {   // unsigned 32 bit
        unsigned long *ul_tmp_ad, val;

        ul_tmp_ad = (unsigned long *)&(*((unsigned char *) &outpc + off));
        DISABLE_INTERRUPTS;
        val = *((unsigned long *) ul_tmp_ad);        // nice quick ASM
        ENABLE_INTERRUPTS;
        if (val > 32767) {
            load = 32767;
        } else {
            load = (int)val;
        }
    } else {
        load = 0;
    }

    return(load);
}

void generic_pwm()
{
    /**************************************************************************
     ** Generic PWM open-loop outputs - calculate duty
     **************************************************************************/
    int i;
    for (i = 0; i < 6 ; i++) {
        if (ram5.pwm_opt[i] & 1) { // Might want to add additional logic in here.
            unsigned char duty;
            int load;

            load = calc_outpc_input(ram5.pwm_opt_load_size[i],
                                  ram5.pwm_opt_load_offset[i] & 0x1ff);
 
            if (ram5.pwm_opt_curve & twopow[i]) {   // curve
                duty = intrp_1dctable(load, 12,
                       (int *)&ram_window.pg21.pwm_axes3d[i][0][0], 0,
                       (unsigned char*)&ram_window.pg21.pwm_duties3d[i][0][0], 21);
            } else {                                // table
                duty = intrp_2dctable(outpc.rpm, load, 6, 6,
                       &ram_window.pg21.pwm_axes3d[i][0][0], // rpms
                       &ram_window.pg21.pwm_axes3d[i][1][0], // loadvals
                       &ram_window.pg21.pwm_duties3d[i][0][0], 0, 21);
            }

            if (ram5.pwm_opt[i] & 0x1e) { // variable
                outpc.duty_pwm[i] = duty;
            } else { // on/off
                if (duty > ram5.pwm_onabove[i]) {
                    outpc.duty_pwm[i] = 100;
                } else if (duty < ram5.pwm_offbelow[i]) {
                    outpc.duty_pwm[i] = 0;
                }
            }
        }
    }
}

void generic_pwm_outs()
{
    /**************************************************************************
     ** Generic PWM open-loop outputs - calculate on/off times for RTC
     ** handled here so other features (e.g. ALS) can re-use the outputs
     **************************************************************************/
    int i;
    for (i = 0; i < sw_pwm_num ; i++) {
        if (pin_swpwm[i] == 255) { // magic number for CANPWMs
            unsigned int d;
            d = *sw_pwm_duty[i];

            if (gp_stat[i] & 0x40) { // negative duty
                if (gp_stat[i] & 0x80) {
                    d = 255 - d;
                } else {
                    d = 100 - d;
                }
            }

            if (((gp_stat[i] & 0x80) == 0) && (ram4.enable_poll & ENABLE_POLL_PWMOUT255)) {
                /* native is 0-100, force to 0-255 */
                d = (d * 653) >> 8; // quick *2.55
            }

            *port_swpwm[i] = (unsigned char)d;

        } else if (pin_swpwm[i]) { // non CANPWMs
            /* figure out PWM parameters for isr_rtc.s */
            unsigned char mult, duty;
            unsigned int max, trig;

            if ((gp_stat[i] & 0x30) == 0x20) { // variable frequency, fixed duty
                unsigned int trig2;
                unsigned char f;

                /* use the duty variable as the frequency 1Hz */
                f = *sw_pwm_duty[i];
                if (f) {
                    trig = 7812 / f;
                    trig2 = trig >> 1;
                    trig = trig - trig2; /* maintain period and allow for rounding */
                } else {
                    trig = 3906;
                    trig2 = 3906;
                }
                gp_max_on[i] = trig;
                gp_max_off[i] = trig2;

            } else if ((gp_stat[i] & 0x30) == 0x30) { // variable 16bit frequency, fixed duty
                unsigned int trig2, f;

                /* use the duty variable as the frequency 0.1Hz */
                f = *(unsigned int*)sw_pwm_duty[i];
                if (f) {
                    trig = 78125 / f;
                    trig2 = trig >> 1;
                    trig = trig - trig2; /* maintain period and allow for rounding */
                } else {
                    trig = 0;
                    trig2 = 78;
                }
                gp_max_on[i] = trig;
                gp_max_off[i] = trig2;

            } else { // fixed frequency, variable duty
                if (sw_pwm_freq[i] == 0) { // on/off mode
                    /* work at lowest frequency in interrupt (11Hz) */
                    mult = 15;
                } else { // normal PWM
                    mult = sw_pwm_freq[i];
                }
                duty = *sw_pwm_duty[i];

                if ((!(gp_stat[i] & 0x80)) && (duty > 100)) { //0-100
                    duty = 100;
                }

                if (mult > 8) {
                    mult -= 8;
                    max = mult * 100;
                    trig = (unsigned int)duty * mult;

                } else if (mult == 2) { // 250Hz
                    // gives 250Hz with 3% duty steps (step size refers to 0-100 scale)
                    max = 31;
                    trig = ((unsigned int)duty * 10) / 32; //  divide by 3.2

                } else if (mult == 3) { // 225Hz
                    max = 35;
                    trig = ((unsigned int)duty * 10) / 29; //  divide by 2.9

                } else if (mult == 4) { // 200Hz
                    max = 38;
                    trig = ((unsigned int)duty * 10) / 26; //  divide by 2.6

                } else if (mult == 5) { // 175Hz
                    max = 45;
                    trig = ((unsigned int)duty * 10) / 22; //  divide by 2.2

                } else if (mult == 6) { // 150Hz
                    max = 53;
                    trig = ((unsigned int)duty * 10) / 19; //  divide by 1.9

                } else if (mult == 7) { // 125Hz
                    max = 63;
                    trig = ((unsigned int)duty * 10) / 16; //  divide by 1.6

                } else if (mult == 8) { // 100Hz
                    // gives 250Hz with 3% duty steps
                    max = 77;
                    trig = ((unsigned int)duty * 10) / 13; //  divide by 1.3

                } else { // fallback for INVALID setting, run at 11Hz
                    mult = 7;
                    max = mult * 100;
                    trig = (unsigned int)duty * mult;
                }

                if (gp_stat[i] & 0x80) { //0-255
                    trig = (unsigned int) ((trig * 100UL) / 255);
                }

                if (gp_stat[i] & 0x40) { // negative polarity
                    gp_max_off[i] = trig; // off time
                    gp_max_on[i] = max - trig; // on time
                } else { // positive polarity
                    gp_max_on[i] = trig; // on time
                    gp_max_off[i] = max - trig; // off time
                }
            }

            if ((gp_stat[i] & 1) == 0) {
                gp_clk[i] = 1; // bring it back to earth
                gp_stat[i] |= 1; // enabled
            }
        } else {
            gp_stat[i] &= ~3; // clear bits 0,1
        }
    }
}

void poll_i2c_rtc()
{
/* code to read/write MCP79410 */
    if (i2cstate2 == 0) {
        i2caddr = 0;    // SR
        i2cstate = 1; // read
        i2cstate2++;

    } else if (i2cstate2 == 1) {
        if (i2cstate == 0) {
            if (i2cbyte & 0x80) { // osc is running
                i2cstate2 = 100;
            } else {
                // osc wasn't running, set time as well
                datax1.setrtc_sec = 0;
                datax1.setrtc_min = 0;
                datax1.setrtc_hour = 0;
                datax1.setrtc_month = 0;
                datax1.setrtc_date = 1;
                datax1.setrtc_day = 1;
                datax1.setrtc_year = 2001;
                i2cstate2 = 120;
            }
        }

// -------------- wait phase
    } else if (i2cstate2 == 99) {
        if (flagbyte9 & FLAGBYTE9_GETRTC) {
            flagbyte9 &= ~FLAGBYTE9_GETRTC;
            i2cstate2 = 100;

        } else if (datax1.setrtc_lock == 0x5a) {
            datax1.setrtc_lock = 0;
            i2cstate2 = 120;
        }

// -------------- read phase

    } else if (i2cstate2 == 100) {
        i2caddr = 0;
        i2cstate = 1; // start off a read
        i2cstate2++;

    } else if (i2cstate2 == 101) {
        if (i2cstate == 0) {
            datax1.rtc_sec = (i2cbyte & 0x0f) + (10 * ((i2cbyte >> 4) & 0x07));
            i2caddr = 1;
            i2cstate = 1; // start off a read
            i2cstate2++;
        }

    } else if (i2cstate2 == 102) {
        if (i2cstate == 0) {
            datax1.rtc_min = (i2cbyte & 0x0f) + (10 * (i2cbyte >> 4));
            i2caddr = 2;
            i2cstate = 1; // start off a read
            i2cstate2++;
        }

    } else if (i2cstate2 == 103) {
        if (i2cstate == 0) {
            datax1.rtc_hour = (i2cbyte & 0x0f) + (10 * ((i2cbyte >> 4) & 0x03));
            i2caddr = 4;
            i2cstate = 1; // start off a read
            i2cstate2++;
        }

    } else if (i2cstate2 == 104) {
        if (i2cstate == 0) {
            datax1.rtc_date = (i2cbyte & 0x0f) + (10 * ((i2cbyte >> 4) & 0x03));
            i2caddr = 5;
            i2cstate = 1; // start off a read
            i2cstate2++;
        }

    } else if (i2cstate2 == 105) {
        if (i2cstate == 0) {
            datax1.rtc_month = (i2cbyte & 0x0f) + (10 * ((i2cbyte >> 4) & 0x01));
            i2caddr = 6;
            i2cstate = 1; // start off a read
            i2cstate2++;
        }

    } else if (i2cstate2 == 106) {
        if (i2cstate == 0) {
            datax1.rtc_year = 2000 + (i2cbyte & 0x0f) + (10 * (i2cbyte >> 4)); // not year 2100 safe
            i2caddr = 3;
            i2cstate = 1; // start off a read
            i2cstate2++;
        }

    } else if (i2cstate2 == 107) {
        if (i2cstate == 0) {
            datax1.rtc_day = i2cbyte & 0x07;
//            i2cstate2 = 99;
            i2caddr = 8;
            i2cstate = 1; // start off a read
            i2cstate2++;
        }
    } else if (i2cstate2 == 108) {
        if (i2cstate == 0) {
            i2cstate2 = 99;
        }

// -------------- set time phase

    } else if (i2cstate2 == 120) {
        i2cstate2++;

    } else if (i2cstate2 == 121) {
        if (i2cstate == 0) {
            i2caddr = 0;    // seconds
            i2cbyte = 0x80 | (datax1.setrtc_sec % 10) | ((datax1.setrtc_sec / 10) << 4);
            i2cstate = 30; // write
            i2cstate2++;
        }

    } else if (i2cstate2 == 122) {
        if (i2cstate == 0) {
            i2caddr = 1;
            i2cbyte = (datax1.setrtc_min % 10) | ((datax1.setrtc_min / 10) << 4);
            i2cstate = 30; // write
            i2cstate2++;
        }

    } else if (i2cstate2 == 123) {
        if (i2cstate == 0) {
            i2caddr = 2;
            i2cbyte = ((datax1.setrtc_hour % 10) | ((datax1.setrtc_hour / 10) << 4)) & 0x3f; // ensure 24hr
            i2cstate = 30; // write
            i2cstate2++;
        }

    } else if (i2cstate2 == 124) {
        if (i2cstate == 0) {
            i2caddr = 3;
            i2cbyte = (datax1.setrtc_day & 0x07) | 0x38;
            i2cstate = 30; // write
            i2cstate2++;
        }

    } else if (i2cstate2 == 125) {
        if (i2cstate == 0) {
            i2caddr = 4;
            i2cbyte = (datax1.setrtc_date % 10) | ((datax1.setrtc_date / 10) << 4);
            i2cstate = 30; // write
            i2cstate2++;
        }

    } else if (i2cstate2 == 126) {
        if (i2cstate == 0) {
            i2caddr = 5;
            i2cbyte = (datax1.setrtc_month % 10) | ((datax1.setrtc_month / 10) << 4);
            i2cstate = 30; // write
            i2cstate2++;
        }

    } else if (i2cstate2 == 127) {
        if (i2cstate == 0) {
            i2caddr = 6;
            i2cbyte = (datax1.setrtc_year % 10) | (((datax1.setrtc_year / 10) % 10) << 4);
            i2cstate = 30; // write
            i2cstate2++;
        }
    /* century not stored */


    } else if (i2cstate2 == 128) {
        if (i2cstate == 0) {
            i2caddr = 8;
            if (ram4.rtc_trim < 0) {
                i2cbyte = 128 - ram4.rtc_trim; // 0x80 | abs(rtc_trim)
            } else {
                i2cbyte = ram4.rtc_trim;
            }
            i2cstate = 30; // write
            i2cstate2++;
        }

    } else if (i2cstate2 == 129) {
        if (i2cstate == 0) {
            i2cstate2 = 99; // done
        }
    }
}

void antilag()
{
    unsigned char newstat, newstat2;

    newstat = 0;
    newstat2 = 0;
    RPAGE = 0xfb; // HARDCODING for page 24
    // safe to alter RPAGE in mainloop
    if (ram_window.pg24.als_in_pin & 0x1f) {

        if (((flagbyte23 & FLAGBYTE23_ALS_ON) == 0) && als_timer) { // off, pause timer running
            newstat = 0;

        } else if ((outpc.clt > ram_window.pg24.als_minclt) && (outpc.clt < ram_window.pg24.als_maxclt)
            && (outpc.rpm > ram_window.pg24.als_minrpm) && (outpc.rpm < ram_window.pg24.als_maxrpm)
            && (outpc.mat < ram_window.pg24.als_maxmat)
            && (pin_alsin && ((*port_alsin & pin_alsin) == pin_match_alsin)) ) {
                newstat2 = 1; /* everything except TPS */
                if (outpc.tps < ram_window.pg24.als_maxtps) {
                    newstat = 1;
                }

        } else {
            newstat = 0;
        }

        if (newstat2 && ((flagbyte23 & FLAGBYTE23_ALS_OUT) == 0)) { /* was off, now on */
            flagbyte23 |= FLAGBYTE23_ALS_OUT;
            if (pin_alsout) {
                SSEM0SEI;
                *port_alsout |= pin_alsout;
                CSEM0CLI;
            }
            als_iacstep = IACmotor_pos; // save it

            if (ram_window.pg24.als_opt & 0x04) {
                IACmotor_pos = ram_window.pg24.als_iac_dutysteps;

            }

            if (ram_window.pg24.als_opt & 0x10) {
                als_duty_pwm = ram_window.pg24.als_pwm_duty;
            }

        } else if ((newstat2 == 0) && (flagbyte23 & FLAGBYTE23_ALS_OUT)) { /* was on, now off */
            flagbyte23 &= ~FLAGBYTE23_ALS_OUT;
            IACmotor_pos = als_iacstep; // restore target (RTC code will move if required)
            als_iacstep = 32000;
            if (pin_alsout) {
                SSEM0SEI;
                *port_alsout &= ~pin_alsout;
                CSEM0CLI;
            }
            if (ram_window.pg24.als_opt & 0x10) {
                als_duty_pwm = 0;
            }
        }

        if (newstat) {
            if ((flagbyte23 & FLAGBYTE23_ALS_ON) == 0) { // was off, now on
                flagbyte23 |= FLAGBYTE23_ALS_ON;
                als_timer = ram_window.pg24.als_maxtime;
            } else { // still on
                if (!als_timer) { // reached zero
                    newstat = 0; // turn it off
                    goto ALS_CHKSTAT;
                }
            }

            outpc.als_timing = intrp_2ditable(outpc.rpm, outpc.tps, 6, 6, &ram_window.pg24.als_rpms[0],
                   &ram_window.pg24.als_tpss[0], &ram_window.pg24.als_timing[0][0], 24);
            outpc.als_addfuel = intrp_2ditable(outpc.rpm, outpc.tps, 6, 6, &ram_window.pg24.als_rpms[0],
                   &ram_window.pg24.als_tpss[0], &ram_window.pg24.als_addfuel[0][0], 24);

            /* spark cut handled in rev limit spark cut section, calculate spark cut */
            if (ram_window.pg24.als_opt & 0x02) {
                unsigned int sc_pct;
                unsigned char sc_tmp;
                sc_pct = intrp_2dctable(outpc.rpm, outpc.tps, 6, 6, &ram_window.pg24.als_rpms[0],
                   &ram_window.pg24.als_tpss[0], &ram_window.pg24.als_sparkcut[0][0], 0, 24);
                // rough percentage makes setting easier to comprehend
                sc_tmp = (unsigned char)(((unsigned int)255 * sc_pct) / 100);
                if (sc_tmp > spkcut_thresh_tmp) {
                    spkcut_thresh_tmp = sc_tmp;
                }
                flagbyte10 |= FLAGBYTE10_SPKCUTTMP; // needed on each pass
            }

            /* fuel cut handled in ign_in only, calculate cyclic fuel cut */
            /* only permitted with a sequential variant on MS3X outputs, would be wholly dangerous
               with batch fire outputs */
            if ((ram_window.pg24.als_opt & 0x01) && (ram4.hardware & HARDWARE_MS3XFUEL) && (ram4.sequential & (SEQ_SEMI | SEQ_FULL))) {
                unsigned int fc_pct;
                fc_pct = intrp_2dctable(outpc.rpm, outpc.tps, 6, 6, &ram_window.pg24.als_rpms[0],
                   &ram_window.pg24.als_tpss[0], &ram_window.pg24.als_fuelcut[0][0], 0, 24);
                // rough percentage makes setting easier to comprehend
                fuel_cutx = (unsigned char)(((num_cyl + 1) * fc_pct) / 100);
                fuel_cuty = num_cyl + 1;
                flagbyte10 |= FLAGBYTE10_FUELCUTTMP;
            }
        }
    }

ALS_CHKSTAT:;
    // ensure in all cases that status gets reset if ALS was on
    if ((!newstat) && (flagbyte23 & FLAGBYTE23_ALS_ON)) { // was on, now off
        // clear any settings
        flagbyte23 &= ~FLAGBYTE23_ALS_ON; // ready to restart, (was wait for true de-activation  = 2)
        als_timer = ram_window.pg24.als_pausetime; /* hit timeout, ensure delay */
        fuel_cutx = 0;
        fuel_cuty = 0;
    }

    if ((flagbyte23 & FLAGBYTE23_ALS_ON) == 0) {
        /* Roving idle / fuel cut when ALS inactive */
        /* fuel cut handled in ign_in only, calculate cyclic fuel cut */
        /* only permitted with a sequential variant on MS3X outputs, would be wholly dangerous
           with batch fire outputs */
        if ((ram_window.pg24.als_in_pin & 0x1f) && (ram_window.pg24.als_opt & 0x20)
            && (ram4.hardware & HARDWARE_MS3XFUEL) && (ram4.sequential & (SEQ_SEMI | SEQ_FULL))) {
            unsigned int fc_pct;
            fc_pct = intrp_2dctable(outpc.rpm, outpc.tps, 6, 6, &ram_window.pg24.als_rirpms[0],
               &ram_window.pg24.als_ritpss[0], &ram_window.pg24.als_rifuelcut[0][0], 0, 24);
            // rough percentage makes setting easier to comprehend
            fuel_cutx = (unsigned char)(((num_cyl + 1) * fc_pct) / 100);
            fuel_cuty = num_cyl + 1;
            flagbyte10 |= FLAGBYTE10_FUELCUTTMP;
        }
        outpc.als_timing = 0;
        outpc.als_addfuel = 0;
    }
}

void vvt_ctl_pid_init(void)
{
    int i;

    RPAGE = 0xfb;

    vvt_timer = ram_window.pg24.vvt_ctl_ms;
    if (ram_window.pg24.vvt_opt1 & 0x8) {
        flagbyte16 |= FLAGBYTE16_VVT_TIMED;
    }
    vvt_PID_enabled = 0;

    for (i = 0; i < 4; i++) {
        vvt_ctl_last_pv[i][0] = vvt_ctl_last_pv[i][1] = 0;
        vvt_ctl_last_error[i] = 0;
        vvt_last_run[i] = 0;
        if ((ram_window.pg24.vvt_out[i] & 0x80) && (vvt_decoder != 2)) {
            outpc.vvt_duty[i] = 255;
        } else {
            outpc.vvt_duty[i] = 0;
        }
    }
}

void vvt_pid(unsigned char numvvt)
{
    int i, hold_duty, ang, targ, tmp2;
    long tmp1;
    unsigned long lmms_ltch, looptime;
    unsigned char flags[4] = {PID_TYPE_B, PID_TYPE_B, PID_TYPE_B, PID_TYPE_B};
    unsigned char exhaust_or_intake, Kp, Ki, Kd;

    for (i = 0; i < numvvt; i++) {
        if (vvt_run & twopow[i]) {
            int slew, diff;

            DISABLE_INTERRUPTS;
            vvt_run &= ~(twopow[i]);
            lmms_ltch = lmms;
            ENABLE_INTERRUPTS;
            looptime = lmms_ltch - vvt_last_run[i];
            vvt_last_run[i] = lmms_ltch;

            /* VVT angle slew */
            slew = (int)ram_window.pg24.vvt_slew;
            if (slew == 0) {
                slew = 1;
            }

            diff = vvt_truetarget[i] - outpc.vvt_target[i];
            if (diff > 0) {
                if (diff > slew) {
                    diff = slew;
                }
            } else if (diff < 0) {
                if (diff < -slew) {
                    diff = -slew;
                }
            }
            /* apply change */
            outpc.vvt_target[i] += diff;

            /* cam type */
            if (ram_window.pg24.vvt_opt5 & (0x10 << i)) {
                exhaust_or_intake = 1;
            } else {
                exhaust_or_intake = 0;
            }

            if (exhaust_or_intake) { /* Exhaust */
                if (ram_window.pg24.vvt_opt2 & 0x4) {
                    hold_duty = ram_window.pg24.vvt_hold_duty_exh;
                } else {
                    hold_duty = -1;
                }
                Kp = ram_window.pg24.vvt_ctl_Kp_exh;
                Ki = ram_window.pg24.vvt_ctl_Ki_exh;
                Kd = ram_window.pg24.vvt_ctl_Kd_exh;
                if (ram_window.pg24.vvt_opt1 & 0x80) {
                     /* exhaust works as a retard table (typical)
                        user is shown numbers as retardation
                        code wants advance numbers
                      */
                    targ = ram_window.pg24.vvt_max_ang[i] - outpc.vvt_target[i];
                    ang = ram_window.pg24.vvt_max_ang[i] - outpc.vvt_ang[i];
                } else {
                    targ = outpc.vvt_target[i];
                    ang = outpc.vvt_ang[i];
                }
            } else {
                if (ram_window.pg24.vvt_opt2 & 0x2) {
                    hold_duty = ram_window.pg24.vvt_hold_duty;
                } else {
                    hold_duty = -1;
                }
                Kp = ram_window.pg24.vvt_ctl_Kp;
                Ki = ram_window.pg24.vvt_ctl_Ki;
                Kd = ram_window.pg24.vvt_ctl_Kd;
                targ = outpc.vvt_target[i];
                ang = outpc.vvt_ang[i];
            }

            if (outpc.engine & ENGINE_CRANK) {
                vvt_PID_enabled &= ~twopow[i];
                /* set Duty to 0 to make sure the cam is fully retarded on crank */
                if ((ram_window.pg24.vvt_out[i] & 0x80) && (vvt_decoder != 2)) {
                    outpc.vvt_duty[i] = 255; // have to do this as output polarity is swapped
                } else {
                    outpc.vvt_duty[i] = 0;
                }
            } else {
                if (!(vvt_PID_enabled & twopow[i]) && outpc.rpm) {
                    vvt_PID_enabled |= twopow[i];
                    flags[i] |= PID_INIT;
                    vvt_duty_store[i] = outpc.vvt_duty[i] * 10000L / 255;
                }

                tmp1 = vvt_duty_store[i];

                tmp1 = tmp1 + (generic_pid_routine(0, ram_window.pg24.vvt_max_ang[i] - ram_window.pg24.vvt_min_ang[i],
                                                   targ, ang,
                                                   Kp,
                                                   Ki,
                                                   Kd,
                                                   looptime,
                                                   vvt_ctl_last_pv[i],
                                                   &vvt_ctl_last_error[i],
                                                   flags[i]) / 
                                                   100L);

                if (hold_duty != -1) {
                    if ((outpc.vvt_target[i] / 10) == (outpc.vvt_ang[i] / 10)) {
                        tmp1 = (long)hold_duty * 10000/255L;
                    }
                }


                if (tmp1 < (ram_window.pg24.vvt_minduty1 * 39)) {
                    tmp1 = ram_window.pg24.vvt_minduty1 * 39;
                }

                if (tmp1 > (ram_window.pg24.vvt_maxduty1 * 39)) {
                    tmp1 = (ram_window.pg24.vvt_maxduty1 * 39);
                }

                vvt_duty_store[i] = tmp1;

                tmp2 = (tmp1 * 255) / 10000L; // scale up to 0-255
                if (tmp2 > 255) {
                    tmp2 = 255;
                }
                outpc.vvt_duty[i] = tmp2;
            }
        }
    }
}

void vvt()
{
    int target[2];
    unsigned char numvvt;
    RPAGE = 0xfb; // HARDCODING for page 24
    // safe to alter RPAGE in mainloop
    vvt_inj_timing_adj = 0; // clear now, will get set if needed

    numvvt = ram_window.pg24.vvt_opt1 & 0x03;
    if (numvvt > 2) {
        numvvt = 4;
    }

    if (((outpc.engine & ENGINE_READY) == 0) || (outpc.rpm == 0)
        || (outpc.clt <= ram_window.pg24.vvt_minclt)) {
        int i;
        /* Handle output polarities */
        for (i = 0; i < 4; i++) {
            if ((ram_window.pg24.vvt_out[i] & 0x80) && (vvt_decoder != 2)) {
                outpc.vvt_duty[i] = 255;
            } else {
                outpc.vvt_duty[i] = 0;
            }
        }
        goto VVT_NONRUN;
    }

    if (numvvt) {
        int vvt_load;

        if ((ram_window.pg24.vvt_opt7 & 0x07) == 1) {
            vvt_load = outpc.map;
        } else if ((ram_window.pg24.vvt_opt7 & 0x07) == 2) {
            vvt_load = (int) ((outpc.map * 1000L) / outpc.baro);
        } else if ((ram_window.pg24.vvt_opt7 & 0x07) == 3) {
            vvt_load = outpc.tps;
        } else if ((ram_window.pg24.vvt_opt7 & 0x07) == 4) {
            vvt_load = outpc.mafload;
        } else {
            vvt_load = outpc.fuelload;
        }

        // intake
        target[0] = intrp_2ditable(outpc.rpm, vvt_load, 8, 8,
                               &ram_window.pg24.vvt_timing_rpm[0],
                               &ram_window.pg24.vvt_timing_load[0],
                               (unsigned int *) &ram_window.pg24.vvt_timing[0][0][0], 24);
        // exhaust
        target[1] = intrp_2ditable(outpc.rpm, vvt_load, 8, 8,
                               &ram_window.pg24.vvt_timing_rpm[0],
                               &ram_window.pg24.vvt_timing_load[0],
                               (unsigned int *) &ram_window.pg24.vvt_timing[1][0][0], 24);

        if (ram_window.pg24.vvt_opt2 & 0x01) {
            if (ram_window.pg24.vvt_opt5 & 0x04) { // actual
                if (ram_window.pg24.vvt_opt5 & 0x01) {
                    vvt_inj_timing_adj = outpc.vvt_ang[0] - ram_window.pg24.vvt_min_ang[0];
                } else if (ram_window.pg24.vvt_opt5 & 0x02) {
                    vvt_inj_timing_adj = outpc.vvt_ang[1] - ram_window.pg24.vvt_min_ang[1];
                } else {
                    vvt_inj_timing_adj = 0;
                }
            } else { // commanded
                if (ram_window.pg24.vvt_opt5 & 0x01) {
                    vvt_inj_timing_adj = target[0];
                } else if (ram_window.pg24.vvt_opt5 & 0x02) {
                    vvt_inj_timing_adj = target[1];
                } else {
                    vvt_inj_timing_adj = 0;
                }
            }

            // VVTs use target[0/1] depending on whether intake or exhaust
            /* convert relative to absolute angles */

            vvt_truetarget[0] = target[(ram_window.pg24.vvt_opt5 & 0x10) == 0x10];
            if (numvvt > 1) {
                vvt_truetarget[1] = target[(ram_window.pg24.vvt_opt5 & 0x20) == 0x20];
            } else {
                vvt_truetarget[1] = 0;
                outpc.vvt_target[1] = 0;
            }
            if (numvvt > 2) {
                vvt_truetarget[2] = target[(ram_window.pg24.vvt_opt5 & 0x40) == 0x40];
            } else {
                vvt_truetarget[2] = 0;
                outpc.vvt_target[2] = 0;
            }
            if (numvvt > 3) {
                vvt_truetarget[3] = target[(ram_window.pg24.vvt_opt5 & 0x80) == 0x80];
            } else {
                vvt_truetarget[3] = 0;
                outpc.vvt_target[3] = 0;
            }
            if (outpc.rpm < 3) {
                outpc.vvt_target[0] = vvt_truetarget[0];
                outpc.vvt_target[1] = vvt_truetarget[1];
                outpc.vvt_target[2] = vvt_truetarget[2];
                outpc.vvt_target[3] = vvt_truetarget[3];
            }

            vvt_pid(numvvt);
        } else {
            // on/off only - using PWM E table (presently)
            /* is this the best way to handle on/off ?
             * better way might be to use the same normal VVT table and overcome the interpolation problem ??
             * using the same table is more consistent
             */
            unsigned char duty;
            duty = intrp_2dctable(outpc.rpm, vvt_load, 6, 6,
                   &ram_window.pg21.pwm_axes3d[4][0][0], // rpms
                   &ram_window.pg21.pwm_axes3d[4][1][0], // loadvals
                   &ram_window.pg21.pwm_duties3d[4][0][0], 0, 21);
 
            if (duty > 60) { // fixed threshold
                outpc.vvt_duty[0] = 255;
                flagbyte15 |= FLAGBYTE15_VVTON;
                if (ram_window.pg24.vvt_opt1 & 0x80) {
                    vvt_inj_timing_adj = ram_window.pg24.vvt_onoff_ang;
                } else {
                    vvt_inj_timing_adj = 0;
                }
            } else if (duty < 40) {
                outpc.vvt_duty[0] = 0;
                vvt_inj_timing_adj = 0;
                flagbyte15 &= ~FLAGBYTE15_VVTON;
            }
        }
VVT_NONRUN:;

        // check test modes
        if (ram_window.pg24.vvt_opt1 & 0x70) {
            int ch;
            ch = ((ram_window.pg24.vvt_opt1 & 0x70) >> 4) - 1;
            outpc.vvt_duty[ch] = ram_window.pg24.vvt_test_duty;
        }

        if (vvt_decoder == 2) { //  BMW S54 or S62
            long tmp1, tmp2, i;

            for (i = 0 ; i < numvvt ; i++) {
                /* See if test mode or not */
                if (i == ((ram_window.pg24.vvt_opt1 & 0x70) >> 4) - 1) {
                    tmp1 = (ram_window.pg24.vvt_test_duty * 10000L) / 255; /* Generate same number as PID code */
                } else {
                    tmp1 = vvt_duty_store[i]; /* Use higher resolution stored number */
                }

                /* Fix 'midpoint' in control duty as 50%.
                    Then convert this to open or close duties */
                if (ram_window.pg24.vvt_out[i] & 0x80) {
                    tmp1 = 10000 - tmp1;
                }
                if (tmp1 >= 4980) {
                    tmp2 = (514 * (tmp1 - 4980)) / 10000L;
                    if (tmp2 > 255) {
                        tmp2 = 255;
                    }
                    *port_vvt[i] = tmp2;
                    vvt_softout_duty[i] = 0;
                } else {
                    tmp2 = (514 * (4980 - tmp1)) / 10000L;
                    if (tmp2 > 255) {
                        tmp2 = 255;
                    }
                    *port_vvt[i] = 0;
                    vvt_softout_duty[i] = tmp2;
                }
            }

        } else {
            *port_vvt[0] = outpc.vvt_duty[0];
            *port_vvt[1] = outpc.vvt_duty[1];
            *port_vvt[2] = outpc.vvt_duty[2];
            *port_vvt[3] = outpc.vvt_duty[3];
        }
    }
}

/* Torque convertor lockup
 * written with 700R4 in mind
 */
void tclu()
{
    unsigned char v = 0;
    RPAGE = 0xfb; // HARDCODING for page 24
    // safe to alter RPAGE in mainloop
    if (ram_window.pg24.tclu_outpin & 0x1f) {
            // see if conditions are be met
            if ( (!(((ram_window.pg24.tclu_opt & 0x01) && (outpc.vss1 < ram_window.pg24.tclu_vssmin))
                || ((ram_window.pg24.tclu_opt & 0x02) && (outpc.vss2 < ram_window.pg24.tclu_vssmin))))
                && (!((ram_window.pg24.tclu_opt & 0x04) && (outpc.gear < ram_window.pg24.tclu_gearmin)))
                && (outpc.tps >= ram_window.pg24.tclu_tpsmin)
                && (outpc.tps <= ram_window.pg24.tclu_tpsmax)
                && (outpc.map >= ram_window.pg24.tclu_mapmin)
                && (outpc.map <= ram_window.pg24.tclu_mapmax)
                && ((*port_tcluen & pin_tcluen) == pin_match_tcluen)
                && (!(pin_tclubr && ((*port_tclubr & pin_tclubr) == pin_match_tclubr)))
                ) {
                v = 1;
            }

        if (tclu_state == 0) { // presently off
            if (v == 1) {
                tclu_timer = ram_window.pg24.tclu_delay;
                tclu_state = 1;
            } else {
                SSEM0SEI;
                *port_tcluout &= ~pin_tcluout; // ensure output is off
                CSEM0CLI;
            }
        } else if (tclu_state == 1) { // pending delay
            if (v == 0) {
                tclu_state = 0; // cancel
            } else if (tclu_timer == 0) {
                tclu_state = 2; // turn it on
                SSEM0SEI;
                *port_tcluout |= pin_tcluout;
                CSEM0CLI;
            }
        } else if (tclu_state == 2) { // locked
            v = 1;
            /* recheck conditions with some hyst */
            if (pin_tclubr && ((*port_tclubr & pin_tclubr) == 0)) { // brake switch cancels immediately
                v = 0;
            }
            if (*port_tcluen & pin_tcluen) { // lack of enable input
                v = 0;
            }
            if ( ((ram_window.pg24.tclu_opt & 0x01) && (outpc.vss1 < (ram_window.pg24.tclu_vssmin - 10)))
                || ((ram_window.pg24.tclu_opt & 0x02) && (outpc.vss2 < (ram_window.pg24.tclu_vssmin -10)))) {
                // 1.0 ms-1 hyst
                v = 0;
            }
            if ((ram_window.pg24.tclu_opt & 0x04) && (outpc.gear < ram_window.pg24.tclu_gearmin)) { // wrong gear
                v = 0;
            }
            if ( (outpc.tps < (ram_window.pg24.tclu_tpsmin - 10))
                || (outpc.tps > (ram_window.pg24.tclu_tpsmax + 10)) ) { // TPS with 1.0% hyst
                v = 0;
            }
            if ( (outpc.map < (ram_window.pg24.tclu_mapmin - 10))
                || (outpc.map > (ram_window.pg24.tclu_mapmax + 10)) ) { // MAP with 1.0% hyst
                v = 0;
            }
            if (v == 0) {
                tclu_state = 0;
                SSEM0SEI;
                *port_tcluout &= ~pin_tcluout; // ensure output is off
                CSEM0CLI;
            }
        }
   }
}

void traction()
{
    /**************************************************************************
     ** Traction control
     **************************************************************************/
    // only present methods are 'perfect run' and 'vss1 vs vss2'
    unsigned int slipxtime, tmp_tc_retard;
    unsigned char tmp_tc_spkcut;
    unsigned int tmp_tc_addfuel;
    unsigned long ul;
 
    slipxtime = 0;
    if (((ram5.tc_opt & 1) == 0) || (pin_tcenin && ((*port_tcenin & pin_tcenin) != pin_match_tcenin))) {
        // feature disabled or enable input enabled but inactive
        tc_addfuel = 0;
        tc_nitrous = 100;
        tc_boost = 100;
        sliptimer = 0;
        flagbyte10 &= ~FLAGBYTE10_TC_N2O;
        outpc.tc_retard = 0;
        outpc.tc_slipxtime = 0;
        if (pin_tcled) {
            SSEM0;
            *port_tcled &= ~pin_tcled;
            CSEM0;
        }
        return;
    }
    if (((ram5.tc_opt & 6) == 0) && perfect_timer && (outpc.tps > ram5.tc_mintps) && (outpc.vss1 > ram5.tc_minvss)) { // perfect run method
        unsigned int perfect_vss;
        // see if time within range
        RPAGE = tables[19].rpg;
        if (perfect_timer > ram_window.pg19.tc_perfect_time[9]) {
            perfect_timer = 0; // done
            sliptimer = 0;
        } else {
            perfect_vss = intrp_1ditable(perfect_timer, 10, (int *) ram_window.pg19.tc_perfect_time,
               0, (unsigned int *) ram_window.pg19.tc_perfect_vss, 19);
            if (outpc.vss1 > perfect_vss) {
                slipxtime = ((outpc.vss1 - perfect_vss) * 100UL) / perfect_vss;
                if (sliptimer == 0) {
                    sliptimer = 1;
                }
            } else {
                sliptimer = 0;
            }
        }
    } else if ((ram5.tc_opt & 6) == 2) { // vss1 vs vss2
        // assumes vss1 is driven, vss2 is undriven
        if ((outpc.vss1 > outpc.vss2) && (outpc.vss1 > ram5.tc_minvss) && (outpc.vss2 > ram5.tc_minvss)
            && (outpc.tps > ram5.tc_mintps) && (outpc.map > ram5.tc_minmap)) {
            long lt;
            unsigned char slipth;
            if (ram5.tc_opt & 0x10) {
                slipth = intrp_1dctable(*port_tc_knob, 9, 
                        (int *) ram_window.pg27.tcslipx, 0, 
                        (unsigned char *)ram_window.pg27.tcslipy, 27);
            } else {
                slipth = ram5.tc_slipthresh;
            }

            lt = ((outpc.vss1 - outpc.vss2) * 100L) / outpc.vss2;
            if (lt > slipth) {
                slipxtime = (unsigned int)lt - slipth;
                if (sliptimer == 0) {
                    sliptimer = 1;
                }
            } else {
                sliptimer = 0;
            }
        } else {
            sliptimer = 0;
        }
    } else if (((ram5.tc_opt & 6) == 4) && perfect_timer && (outpc.tps > ram5.tc_mintps)) { // perfect run RPM method
        unsigned int perfect_rpm;
        // see if time within range
        RPAGE = tables[19].rpg;
        if (perfect_timer > ram_window.pg19.tc_perfect_time[9]) {
            perfect_timer = 0; // done
            sliptimer = 0;
        } else {
            perfect_rpm = intrp_1ditable(perfect_timer, 10, (int *) ram_window.pg19.tc_perfect_time,
               0, (unsigned int *) ram_window.pg19.tc_perfect_rpm, 19);
            if (outpc.rpm > perfect_rpm) {
                slipxtime = ((outpc.rpm - perfect_rpm) * 100UL) / perfect_rpm;
                if (sliptimer == 0) {
                    sliptimer = 1;
                }
            } else {
                sliptimer = 0;
            }
        }

    } else if ((ram5.tc_opt & 6) == 6) { // switch input
        // External system flags up traction loss using "enable button", so act on it.
        if ((outpc.tps > ram5.tc_mintps) && (outpc.map > ram5.tc_minmap)) {
            slipxtime = 100;
            if (sliptimer == 0) {
                sliptimer = 1;
            }
        }
    }
    ul = (unsigned long)slipxtime * (sliptimer / 10);
    if (ul > 0x7ffe) {
        slipxtime = 0x7ffe;
    } else {
        slipxtime = (unsigned int)ul;
    }

    if (slipxtime == 0) {
        /* turn everything off */
        tc_addfuel = 0;
        tc_nitrous = 100;
        tc_boost = 100;
        tc_boost_duty_delta = 0;
        tmp_tc_retard = 0;
        flagbyte10 &= ~FLAGBYTE10_TC_N2O;
        outpc.tc_retard = 0;
        if (pin_tcled) {
            SSEM0;
            *port_tcled &= ~pin_tcled;
            CSEM0;
        }
    } else {
        if (pin_tcled) {
            SSEM0;
            *port_tcled |= pin_tcled;
            CSEM0;
        }
        // Now figure out what actions to take
        // for all of these a small number in the table means less action.
        tmp_tc_retard = intrp_1ditable(slipxtime, 4, (int *) ram_window.pg19.tc_react_x,
               0, (unsigned int *) ram_window.pg19.tc_retard, 19);
        tmp_tc_spkcut = intrp_1dctable(slipxtime, 4, (int *) ram_window.pg19.tc_react_x,
               0, (unsigned char *) ram_window.pg19.tc_spkcut, 19);
        tmp_tc_addfuel = (unsigned int)intrp_1dctable(slipxtime, 4, (int *) ram_window.pg19.tc_react_x,
               0, (unsigned char *) ram_window.pg19.tc_addfuel, 19);
        tc_nitrous = 100 - intrp_1dctable(slipxtime, 4, (int *) ram_window.pg19.tc_react_x,
               0, (unsigned char *) ram_window.pg19.tc_nitrous, 19);
        tc_boost = 100 - intrp_1dctable(slipxtime, 4, (int *) ram_window.pg19.tc_react_x,
               0, (unsigned char *) ram_window.pg19.tc_boost, 19);
        tc_boost_duty_delta = intrp_1dctable(slipxtime, 4, (int *) ram_window.pg19.tc_react_x,
               0, (unsigned char *) ram_window.pg19.tc_boost_duty_delta, 19);

        if (tmp_tc_addfuel) {
            /* do multiply/divide and check for overflow */
            unsigned long ultmp_cr;
            ultmp_cr = (unsigned long)tmp_tc_addfuel * ReqFuel;
            if (ultmp_cr > 6500000) {
                tc_addfuel = 65000;
            } else {
                tc_addfuel = (unsigned int)(ultmp_cr / 100);
            }
        } else {
            tc_addfuel = 0;
        }

        if (tmp_tc_spkcut) {
            unsigned char sc_tmp;
            // rough percentage makes setting easier to comprehend
            sc_tmp = (unsigned char)(((unsigned int)255 * tmp_tc_spkcut) / 100);
            if (sc_tmp > spkcut_thresh_tmp) {
                spkcut_thresh_tmp = sc_tmp;
            }
            flagbyte10 |= FLAGBYTE10_SPKCUTTMP; // needed on each pass
        }

/*    if (ram5.tc_opt & 0x20) - progressive not yet supported, so either nitrous is on (100%) or off. */
        if (tc_nitrous < 40) {
            flagbyte10 |= FLAGBYTE10_TC_N2O;
        } else if (tc_nitrous > 60) {
            flagbyte10 &= ~FLAGBYTE10_TC_N2O;
        }
    }

    outpc.tc_retard = tmp_tc_retard;
    outpc.tc_slipxtime = slipxtime;
}

void do_spi2(void)
{
}

void ckstall(void)
{
    unsigned long stalltime, stalltime2;

    if (flagbyte2 & flagbyte1_tstmode) {
        return;
    }

    SSEM0SEI;
    stalltime = lmms - ltch_lmms;
    CSEM0CLI;
    if ((outpc.rpm < 3) || (flagbyte2 & flagbyte2_crank_ok)) {
        /* Longer non-running timeout or not yet out of crank mode
            Saves pump bouncing on and off during starting attempt. */
        if (outpc.seconds > (2 + ((unsigned int)ram4.primedelay / 10))) {
            stalltime2 = 3906; // 0.5s, was 2s
        } else {
            /* right at start include the primedelay too */
            stalltime2 = 15625 + (781 * ram4.primedelay);
        }

    } else {
        /* quick timeout while running */
        stalltime2 = stall_timeout;
    }

    if (stalltime > stalltime2) {
        if (synch & SYNC_SYNCED) {
            ign_reset();
        }
        clear_all();
    }
}

void clear_all(void)
{
    fuelpump_off();
    // idle?
    outpc.engine &= ~1;
    outpc.pw1 = 0;
    outpc.pw2 = 0;
    outpc.pwseq[0] = 0;
    outpc.pwseq[1] = 0;
    outpc.pwseq[2] = 0;
    outpc.pwseq[3] = 0;
    outpc.pwseq[4] = 0;
    outpc.pwseq[5] = 0;
    outpc.pwseq[6] = 0;
    outpc.pwseq[7] = 0;
    outpc.pwseq[8] = 0;
    outpc.pwseq[9] = 0;
    outpc.pwseq[10] = 0;
    outpc.pwseq[11] = 0;
    outpc.pwseq[12] = 0;
    outpc.pwseq[13] = 0;
    outpc.pwseq[14] = 0;
    outpc.pwseq[15] = 0;
    asecount = 0;
    cranktpcnt = 0;
    tcrank_done = 0xffff;
    running_seconds = 0;
    flagbyte2 |= flagbyte2_crank_ok;
    if (!(ram4.EAEOption & 0x01)) {
        outpc.EAEfcor1 = 100;
        outpc.EAEfcor2 = 100;
        outpc.wallfuel1 = 0;
        outpc.wallfuel2 = 0;
        WF1 = 0;
        AWA1 = 0;
        SOA1 = 0;
        WF2 = 0;
        AWA2 = 0;
        SOA2 = 0;
    }
}

int abs_int(int in)
{
    if (in < 0) {
        return -in;
    } else {
        return in;
    }
}

void check_sensors(void)
{
    int x;
    unsigned char stat_egt_all = 0;
    #define CK_SENS_SAMPLES 30
    #define CK_SENS_THRESH 10

    RPAGE = RPAGE_VARS1;

    if (outpc.seconds) {
        /* only check after ECU has been on for a second */
        v1.ck_map_sum += abs_int(*mapport - v1.ck_map_last);
        v1.ck_mat_sum += abs_int(ATD0DR1 - v1.ck_mat_last);
        v1.ck_clt_sum += abs_int(ATD0DR2 - v1.ck_clt_last);
        v1.ck_tps_sum += abs_int(ATD0DR3 - v1.ck_tps_last);
        v1.ck_batt_sum += abs_int(outpc.batt - v1.ck_batt_last);
        v1.ck_afr0_sum += abs_int(outpc.afr[0] - v1.ck_afr0_last);

        if (*mapport < ram5.map_minadc) {
            v1.ck_map_min_cnt++;
        } else if (*mapport > ram5.map_maxadc) {
            v1.ck_map_max_cnt++;
        }

        if (ATD0DR1 < ram5.mat_minadc) {
            v1.ck_mat_min_cnt++;
        } else if (ATD0DR1 > ram5.mat_maxadc) {
            v1.ck_mat_max_cnt++;
        }

        if (ATD0DR2 < ram5.clt_minadc) {
            v1.ck_clt_min_cnt++;
        } else if (ATD0DR2 > ram5.clt_maxadc) {
            v1.ck_clt_max_cnt++;
        }

        if (ATD0DR3 < ram5.tps_minadc) {
            v1.ck_tps_min_cnt++;
        } else if (ATD0DR3 > ram5.tps_maxadc) {
            v1.ck_tps_max_cnt++;
        }

        if ((outpc.batt < ram5.batt_minv) && (running_seconds > 5)) { // expect low battery at and immediately after start so wait 5 seconds
            v1.ck_batt_min_cnt++;
        } else if (outpc.batt > ram5.batt_maxv) {
            v1.ck_batt_max_cnt++;
        }

        if (outpc.afr[0] < ram5.afr_min) {
            v1.ck_afr0_min_cnt++;
        } else if (outpc.afr[0] > ram5.afr_max) {
            v1.ck_afr0_max_cnt++;
        }

        if ((ram5.cel_opt2 & 0x80) && ram4.egt_num) {
            int n;
            n = ram4.egt_num > NUM_CHAN ? NUM_CHAN : ram4.egt_num;
            for (x = 0; x < n ; x++) {
                unsigned long t;
                if ((outpc.egt[x] < ram5.egt_minvalid)
                 && (!((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)))) {
                    /* running minimum will detect dead cylinder too */
                    v1.ck_egt_min_cnt[x]++;
                } else if (outpc.egt[x] > ram5.egt_maxvalid) {
                    v1.ck_egt_max_cnt[x]++;
                }
                t = v1.ck_egt_sum[x] + abs_int(outpc.egt[x] - v1.ck_egt_last[x]);
                if (t > 65535) {
                    t = 65535;
                }
                v1.ck_egt_sum[x] = t;
            }
        }
    }

    /* Always populate these here so sensible data is ready for first pass.*/

    /* use raw data so we can check sensor health even in fallback */
    v1.ck_map_last = *mapport;
    v1.ck_mat_last = ATD0DR1;
    v1.ck_clt_last = ATD0DR2;
    v1.ck_tps_last = ATD0DR3;

    v1.ck_batt_last = outpc.batt;
    v1.ck_afr0_last = outpc.afr[0];
    if ((ram5.cel_opt2 & 0x80) && ram4.egt_num) {
        int n = ram4.egt_num > NUM_CHAN ? NUM_CHAN : ram4.egt_num;
        for (x = 0; x < n ; x++) {
            v1.ck_egt_last[x] = outpc.egt[x];
        }
    }

    v1.ck_cnt++;
    if (v1.ck_cnt >= CK_SENS_SAMPLES) {
        outpc.cel_status = 0; // start with zero and set shortly.
        outpc.cel_status2 = 0; // start with zero and set shortly.

        /* process data */
        v1.ck_cnt = 0;

        /* validation allows 8 samples out of range */
        /* MAP */
        stat_map <<= 1;
        if ((ram5.cel_opt2 & 0x01)
            && ( ((running_seconds > ram5.cel_runtime) && ((v1.ck_map_sum > ram5.map_var_upper) || (v1.ck_map_sum < ram5.map_var_lower)))
            || (v1.ck_map_min_cnt > CK_SENS_THRESH)
            || (v1.ck_map_max_cnt > CK_SENS_THRESH)) ) {
            stat_map |= 1;
        }

        /* MAT */
        stat_mat <<= 1;
        if ((ram5.cel_opt2 & 0x02)
            && ( ((running_seconds > ram5.cel_runtime) && (v1.ck_mat_sum > ram5.mat_var_upper))
            || (v1.ck_mat_min_cnt > CK_SENS_THRESH)
            || (v1.ck_mat_max_cnt > CK_SENS_THRESH)) ) {
            stat_mat |= 1;
        }

        /* CLT */
        stat_clt <<= 1;
        if ((ram5.cel_opt2 & 0x04)
            && ( ((running_seconds > ram5.cel_runtime) && (v1.ck_clt_sum > ram5.clt_var_upper))
            || (v1.ck_clt_min_cnt > CK_SENS_THRESH)
            || (v1.ck_clt_max_cnt > CK_SENS_THRESH)) ) {
            stat_clt |= 1;
        }

        /* TPS */
        stat_tps <<= 1;
        if ((ram5.cel_opt2 & 0x08)
            && ( ((running_seconds > ram5.cel_runtime) && (v1.ck_tps_sum > ram5.tps_var_upper))
            || (v1.ck_tps_min_cnt > CK_SENS_THRESH)
            || (v1.ck_tps_max_cnt > CK_SENS_THRESH)) ) {
            stat_tps |= 1;
        }

        /* Batt */
        stat_batt <<= 1;
        if ((ram5.cel_opt2 & 0x10)
            && ( ((running_seconds > ram5.cel_runtime) && (v1.ck_batt_sum > ram5.batt_var_upper))
            || (v1.ck_batt_min_cnt > CK_SENS_THRESH)
            || (v1.ck_batt_max_cnt > CK_SENS_THRESH)) ) {
            stat_batt |= 1;
        }

        /* AFR0 */
        stat_afr0 <<= 1;
        if ((ram5.cel_opt2 & 0x20)
            && ( ((running_seconds > ram5.cel_runtime) && ((v1.ck_afr0_sum > ram5.afr_var_upper) || (v1.ck_afr0_sum < ram5.afr_var_lower)))
            || (v1.ck_afr0_min_cnt > CK_SENS_THRESH)
            || (v1.ck_afr0_max_cnt > CK_SENS_THRESH)) ) {
            stat_afr0 |= 1;
        }

        /* EGTs */
        stat_egt_all = 0;
        if ((ram5.cel_opt2 & 0x80) && ram4.egt_num) {
            int n = ram4.egt_num > NUM_CHAN ? NUM_CHAN : ram4.egt_num;
            for (x = 0; x < n ; x++) {
                stat_egt[x] <<= 1;
                /* no checks at all during initial period to allow engine to warmup */
                if ((running_seconds > ram5.cel_runtime) && ((v1.ck_egt_sum[x] > ram5.egt_var_upper) || (v1.ck_egt_sum[x] < ram5.egt_var_lower)
                    || (v1.ck_egt_min_cnt[x] > CK_SENS_THRESH)
                    || (v1.ck_egt_max_cnt[x] > CK_SENS_THRESH))) {
                    stat_egt[x] |= 1;
                }
                if (stat_egt[x]) {
                    stat_egt_all = 1;
                    outpc.cel_status |= 0x80;
                }
            }
        }

        stat_sync <<= 1;
        if (outpc.engine & ENGINE_CRANK) {
            if ((outpc.synccnt - v1.ck_sync_last) > 5) { // arbitrary 5 sync loss limit during cranking only
                stat_sync |= 1;
            }
        } else if ((!(outpc.engine & ENGINE_CRANK)) && (flagbyte2 & flagbyte2_crank_ok) && (v1.ck_sync_last == 0)) {
            v1.ck_sync_last = 1 + outpc.synccnt; // during crank to run period, ignore any sync losses and reset last counter
        } else if ((outpc.engine & ENGINE_READY) && ((outpc.synccnt + 1 - v1.ck_sync_last) > ram5.cel_synctol)) { // sync loss during running
            stat_sync |= 1;
        }

        stat_flex <<= 1; // this is checked and set in Flex code
        stat_maf <<= 1; // this is set in MAF code
        stat_knock <<= 1; // this is set in Knock code
        stat_oil <<= 1; // this is set in Oil code
        stat_fp <<= 1; // this is set in Fuel pressure code

        /* Limp mode is an action. Validate which faults trigger limp mode */
        if ((outpc.engine & ENGINE_READY)
            && ((stat_map && (ram5.cel_action1 & 0x01))
             || (stat_mat && (ram5.cel_action1 & 0x02))
            || (stat_clt && (ram5.cel_action1 & 0x04))
            || (stat_tps && (ram5.cel_action1 & 0x08))
            || (stat_batt && (ram5.cel_action1 & 0x10))
            || (stat_afr0 && (ram5.cel_action1 & 0x20))
            || (stat_sync && (ram5.cel_action1 & 0x40))
            || (stat_flex && (ram5.cel_action2 & 0x01))
            || (stat_egt_all && (ram5.cel_action2 & 0x02))
            )) {
            outpc.status7 |= STATUS7_LIMP;
        } else {
            outpc.status7 &= ~STATUS7_LIMP;
        }

        /* monitor */
        if ((ram5.cel_opt & 0x0e) >= 0x02) { /* Is it on? */
            if (ram5.cel_opt & CEL_OPT_ADC) {
                /* cel_opt_stat =  bits, 274, [1:3], "Off", "MAP", "MAT", "CLT", "TPS", "Batt", "EGO", "EGT1" */
                if ((ram5.cel_opt & 0x0e) == 0x02) {
                    outpc.istatus5 = *mapport;
                } else if ((ram5.cel_opt & 0x0e) == 0x04) {
                    outpc.istatus5 = ATD0DR1;
                } else if ((ram5.cel_opt & 0x0e) == 0x06) {
                    outpc.istatus5 = ATD0DR2;
                } else if ((ram5.cel_opt & 0x0e) == 0x08) {
                    outpc.istatus5 = ATD0DR3;
                } else if ((ram5.cel_opt & 0x0e) == 0x0a) {
                    outpc.istatus5 = ATD0DR4;
                } else if ((ram5.cel_opt & 0x0e) == 0x0c) {
                    outpc.istatus5 = *egoport[0];
                } else if ((ram5.cel_opt & 0x0e) == 0x0e) {
                    outpc.istatus5 = *port_egt[0];
                }
            } else { /* fluctuations */
                if ((ram5.cel_opt & 0x0e) == 0x02) {
                    outpc.istatus5 = v1.ck_map_sum;
                } else if ((ram5.cel_opt & 0x0e) == 0x04) {
                    outpc.istatus5 = v1.ck_mat_sum;
                } else if ((ram5.cel_opt & 0x0e) == 0x06) {
                    outpc.istatus5 = v1.ck_clt_sum;
                } else if ((ram5.cel_opt & 0x0e) == 0x08) {
                    outpc.istatus5 = v1.ck_tps_sum;
                } else if ((ram5.cel_opt & 0x0e) == 0x0a) {
                    outpc.istatus5 = v1.ck_batt_sum;
                } else if ((ram5.cel_opt & 0x0e) == 0x0c) {
                    outpc.istatus5 = v1.ck_afr0_sum;
                } else if ((ram5.cel_opt & 0x0e) == 0x0e) {
                    outpc.istatus5 = v1.ck_egt_sum[0];
                }
            }
        }

        /* Let the user know */
        if (stat_map) {
            outpc.cel_status |= 1;
        }
        if (stat_mat) {
            outpc.cel_status |= 2;
        }
        if (stat_clt) {
            outpc.cel_status |= 4;
        }
        if (stat_tps) {
            outpc.cel_status |= 8;
        }
        if (stat_batt) {
            outpc.cel_status |= 0x10;
        }
        if (stat_afr0) {
            outpc.cel_status |= 0x20;
        }
        if (stat_sync) {
            outpc.cel_status |= 0x40;
        }
        /* EGT status set above */
        if (stat_flex) {
            outpc.cel_status |= 0x100;
        }
        if (stat_maf) {
            outpc.cel_status |= 0x200;
        }
        if (stat_knock) {
            outpc.cel_status |= 0x400;
        }
        if (flagbyte17 & FLAGBYTE17_STATCAM) {
            outpc.cel_status |= 0x800;
        }
        if (stat_oil) {
            outpc.cel_status |= 0x1000;
        }
        if (stat_fp) {
            outpc.cel_status |= 0x2000;
        }
        if (outpc.status6 & STATUS6_EGTSHUT) {
            outpc.cel_status |= 0x4000;
        }
        if (outpc.status6 & STATUS6_AFRSHUT) {
            outpc.cel_status |= 0x8000;
        }
        if (outpc.status8 & STATUS8_WINJLOW) {
            outpc.cel_status2 |= 1;
        }
        if (maxafr_stat >= 2) {
            outpc.cel_status2 |= 2; // some kind of AFR/EGT shutdown
        }

        /* reset counters */
        check_sensors_reset();
    }

    /* light handling runs on every pass */
    {
        unsigned char light = 0;
        /* CEL is only a warning */
        if ((outpc.engine & ENGINE_READY)
            && (outpc.cel_status || outpc.cel_status2) ) {
            if (!(outpc.status7 & STATUS7_CEL)) {
                outpc.status7 |= STATUS7_CEL;
                cel_state1 = 0;
            }
            light = 1;
        } else {
            outpc.status7 &= ~STATUS7_CEL;
        }

        if ((ram5.cel_opt & CEL_OPT_WHEN) && (flagbyte2 & flagbyte2_crank_ok)) {
            light = 1; // per user request, light CEL until engine running
        }

#define CEL_LIGHT1 15000 // initial on time
#define CEL_LIGHT2 5000 // initial off time
#define CEL_LIGHT3 900 // digit on time
#define CEL_LIGHT4 2500 // inter digit time
#define CEL_LIGHT5 5000 // inter number time

        if (pin_cel) {
            if ((ram5.cel_opt & CEL_OPT_FLASH) && light) { // flash codes
                if (cel_state1 == 0) {
                    cel_light_timer = (unsigned int)lmms;
                    cel_state1 = 1;

                } else if (cel_state1 == 1) {
                    // initial on period
                    if (((unsigned int)lmms - cel_light_timer) > CEL_LIGHT1) {
                        cel_light_timer = (unsigned int)lmms;
                        cel_state1++;
                    }

                } else if (cel_state1 == 2) {
                    // initial off period
                    light = 0;
                    if (((unsigned int)lmms - cel_light_timer) > CEL_LIGHT2) {
                        cel_state1 = 11;
                        cel_state2 = 0; // bit 0
                    }

                } else if (cel_state1 == 11) {
                    unsigned char er = 0;
                        
                    if (cel_state2 < 16) {
                        if (outpc.cel_status & twopow[cel_state2]) {
                            er = 1;
                        }
                    } else {
                        if (outpc.cel_status2 & twopow[cel_state2 - 16]) {
                            er = 1;
                        }
                    }
                    if (er) {
                        cel_state3 = cel_state2 + 2; // bit0 = 2 flashes
                        outpc.cel_errorcode = cel_state3;
                        cel_light_timer = (unsigned int)lmms;
                        cel_state1++;
                    } else {
                        light = 0;
                    }
                    cel_state2++;
                    if (cel_state2 > 31) {
                        cel_state2 = 0;
                        cel_state1 = 0;
                    }
                } else if (cel_state1 == 12) {
                    // on period
                    if (((unsigned int)lmms - cel_light_timer) > CEL_LIGHT3) {
                        cel_state3--;
                        cel_light_timer = (unsigned int)lmms;
                        cel_state1++;
                    }
                } else if (cel_state1 == 13) {
                    // off period
                    unsigned int w;
                    light = 0;

                    if (cel_state3 == 0) { // final flash
                        w = CEL_LIGHT5; // gap between numbers
                    } else {
                        w = CEL_LIGHT4; // gap between flashes within a number
                    }
                    if (((unsigned int)lmms - cel_light_timer) > w) {
                        cel_light_timer = (unsigned int)lmms;
                        if (cel_state3) {
                            cel_state1 = 12;
                        } else {
                            /* next number */
                            if (cel_state2 == 0) {
                                cel_state1 = 0;
                            } else {
                                cel_state1 = 11;
                            }
                        }
                    }
                } else {
                    cel_state1 = 0;
                }
            } else {
                outpc.cel_errorcode = 0;
            }

            if (light) {
                SSEM0SEI;
                *port_cel |= pin_cel;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                *port_cel &= ~pin_cel;
                CSEM0CLI;
            }
        }
    }
    return;
}

void check_sensors_reset(void)
{
    int x;

    RPAGE = RPAGE_VARS1;

    v1.ck_cnt = 0;
    v1.ck_map_sum = 0;
    v1.ck_mat_sum = 0;
    v1.ck_clt_sum = 0;
    v1.ck_tps_sum = 0;
    v1.ck_afr0_sum = 0;
    v1.ck_batt_sum = 0;
    v1.ck_map_min_cnt = 0;
    v1.ck_mat_min_cnt = 0;
    v1.ck_clt_min_cnt = 0;
    v1.ck_tps_min_cnt = 0;
    v1.ck_afr0_min_cnt = 0;
    v1.ck_batt_min_cnt = 0;
    v1.ck_map_max_cnt = 0;
    v1.ck_mat_max_cnt = 0;
    v1.ck_clt_max_cnt = 0;
    v1.ck_tps_max_cnt = 0;
    v1.ck_afr0_max_cnt = 0;
    v1.ck_batt_max_cnt = 0;
    {
        int n = ram4.egt_num > NUM_CHAN ? NUM_CHAN : ram4.egt_num;
        for (x = 0 ; x < n ; x++) {
            v1.ck_egt_sum[x] = 0;
            v1.ck_egt_min_cnt[x] = 0;
            v1.ck_egt_max_cnt[x] = 0;
        }
    }
}

void check_sensors_init(void)
{
    int x;
    check_sensors_reset();
    stat_map = 0;
    stat_mat = 0;
    stat_clt = 0;
    stat_tps = 0;
    stat_afr0 = 0;
    stat_batt = 0;
    stat_sync = 0;
    stat_flex = 0;
    stat_maf = 0;
    stat_knock = 0;
    {
        int n = ram4.egt_num > NUM_CHAN ? NUM_CHAN : ram4.egt_num;
        for (x = 0 ; x < n ; x++) {
            stat_egt[x] = 0;
        }
    }
    v1.ck_sync_last = outpc.synccnt;
}

/* The following variable is a global local, it should be the only one and as
   a result will be the last variable allocated, so most exposed to the stack. */
unsigned int stack_watch_var;

void stack_watch_init(void)
{
    stack_watch_var = 0;
}

void stack_watch(void)
{
    if (stack_watch_var != 0) {
        conf_err = 153;
    }
}

int blend_xaxis(unsigned char opt)
{
    if (opt == 0) {
        return outpc.tps / 10;
    } else if (opt == 1) {
        return outpc.map / 10;
    } else if (opt == 2) {
        return outpc.rpm;
    } else if (opt == 3) {
        return outpc.mafload / 10;
    } else if (opt == 4) {
        return outpc.fuel_pct / 10;
    } else if (opt == 5) {
        return outpc.vss1 / 10;
    } else if (opt == 6) {
        return outpc.gear;
    } else if (opt == 7) {
        return (int)(((long)outpc.fuelload * outpc.rpm) / 1000L);
    } else if ((opt >= 16) && (opt <= 31)) {
        return outpc.sensors[opt - 16] / 10;
    } else {
        return 0;
    }
}

unsigned int do_testmode()
{
    if (datax1.testmodemode == 0) {
        flagbyte1 &= ~flagbyte1_tstmode; /* disable test mode */
        outpc.status3 &= ~STATUS3_TESTMODE;
        /* Put injector port/pin mappings back to normal (same code as in init) */
        inj_event_array_portpin(0);
        if (do_dualouts) {
            inj_event_array_portpin(INJ_FILL_STAGED);
        }
        testmode_glob = 0;
    } else if (datax1.testmodemode == 1) {
        flagbyte1 |= flagbyte1_tstmode; /* enable test mode */
        outpc.status3 |= STATUS3_TESTMODE;
        inj_event_array_portpin(INJ_FILL_TESTMODE);
        testmode_glob = 0;
        // accept the test mode and return ok
        return 0;
    } else if (flagbyte1 & flagbyte1_tstmode) {
        // only allow test modes to run when enabled
        if (datax1.testmodemode == 2) { // coil testing
            if (outpc.rpm != 0) {
                return 1;
            } else {
                // enable the mode
                testmode_glob = 1;
                return 0;
            }
        } else if (datax1.testmodemode == 3) { // inj testing
            if (outpc.rpm != 0) {
                return 1;
            } else {
                // enable the mode
                testmode_cnt = ram5.testinjcnt;
                testmode_glob = 2;
                return 0;
            }
        // 4 not used
        } else if (datax1.testmodemode == 5) { // FP on
            SSEM0;
            *port_fp |= pin_fp;
            CSEM0;
            outpc.engine |= ENGINE_READY;
        } else if (datax1.testmodemode == 6) { // FP off
            SSEM0;
            *port_fp &= ~pin_fp;
            CSEM0;
            outpc.engine &= ~ENGINE_READY;
        } else if (datax1.testmodemode == 7) { // Cancel inj or spk
            testmode_glob = 0;
            outpc.istatus5 = 0; // testmode injection counter
/* plenty more */
/*"PM3 - Injection LED D14", "PM4 - Accel LED D16", "PM5 - Warmup LED D15", "PJ0 - IAC2", "PJ1 - IAC1", "PJ7 - JS11", "PP2 - Idle", "PP3 - Boost", "PP4 - Nitrous 1", "PP5 - Nitrous 2", "PP6 - VVT", "PP7 - Fidle", "PT1 - V3 Inj 1", "PT3 - V3 Inj 2", "PT5 - JS10", "PK0 - Tacho", "PA0 - Inj A", "PA1 - Inj B", "PA2 - Inj C", "PA3 - Inj D", "PA4 - Inj E", "PA5 - Inj F", "PA6 - Inj G", "PA7 - Inj H", 
*/
        } else if (datax1.testmodemode & 0x80) { // I/O ports
            unsigned int ioport;
            unsigned char ioport_mode;
            ioport = datax1.testmodemode & 0x17c;
            ioport_mode = datax1.testmodemode & 0x03;
            if (ioport == 0x00) { // PP3
                PWME &= ~0x08;
                DDRP |= 0x08;
                port_testio = (unsigned char*)&PTP;
                pin_testio = 0x08;
            } else if (ioport == 0x04) { // PP4
                PWME &= ~0x10;
                DDRP |= 0x10;
                port_testio = (unsigned char*)&PTP;
                pin_testio = 0x10;
            } else if (ioport == 0x08) { // PP5
                PWME &= ~0x20;
                DDRP |= 0x20;
                port_testio = (unsigned char*)&PTP;
                pin_testio = 0x20;
            } else if (ioport == 0x0c) { // PP6
                PWME &= ~0x40;
                DDRP |= 0x40;
                port_testio = (unsigned char*)&PTP;
                pin_testio = 0x40;
            } else if (ioport == 0x10) { // PP7
                PWME &= ~0x80;
                DDRP |= 0x80;
                port_testio = (unsigned char*)&PTP;
                pin_testio = 0x80;
            } else if (ioport == 0x14) { // PK0
                DDRK |= 0x01;
                port_testio = (unsigned char*)&PORTK;
                pin_testio = 0x01;
            } else if (ioport == 0x18) { // PP2
                PWME &= ~0x04;
                DDRP |= 0x04;
                port_testio = (unsigned char*)&PTP;
                pin_testio = 0x04;
            } else if (ioport == 0x1c) { // fuel pump again
                port_testio = port_fp;
                pin_testio = pin_fp;

            } else if (ioport == 0x20) { // PA0
                DDRA |= 0x01;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x01;
            } else if (ioport == 0x24) { // PA1
                DDRA |= 0x02;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x02;
            } else if (ioport == 0x28) { // PA2
                DDRA |= 0x04;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x04;
            } else if (ioport == 0x2c) { // PA3
                DDRA |= 0x08;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x08;
            } else if (ioport == 0x30) { // PA4
                DDRA |= 0x10;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x10;
            } else if (ioport == 0x34) { // PA5
                DDRA |= 0x20;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x20;
            } else if (ioport == 0x38) { // PA6
                DDRA |= 0x40;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x40;
            } else if (ioport == 0x3c) { // PA7
                DDRA |= 0x80;
                port_testio = (unsigned char*)&PORTA;
                pin_testio = 0x80;

            } else if (ioport == 0x40) { // PB0
                DDRB |= 0x01;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x01;
            } else if (ioport == 0x44) { // PB1
                DDRA |= 0x02;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x02;
            } else if (ioport == 0x48) { // PB2
                DDRB |= 0x04;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x04;
            } else if (ioport == 0x4c) { // PB3
                DDRB |= 0x08;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x08;
            } else if (ioport == 0x50) { // PB4
                DDRB |= 0x10;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x10;
            } else if (ioport == 0x54) { // PB5
                DDRB |= 0x20;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x20;
            } else if (ioport == 0x58) { // PB6
                DDRB |= 0x40;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x40;
            } else if (ioport == 0x5c) { // PB7
                DDRB |= 0x80;
                port_testio = (unsigned char*)&PORTB;
                pin_testio = 0x80;

            } else if (ioport == 0x60) { // IAC1
                *port_iacen |= pin_iacen;
                DDRJ |= 0x02;
                port_testio = (unsigned char*)&PTJ;
                pin_testio = 0x02;
            } else if (ioport == 0x64) { // IAC2
                *port_iacen |= pin_iacen;
                DDRJ |= 0x01;
                port_testio = (unsigned char*)&PTJ;
                pin_testio = 0x01;
            } else if (ioport == 0x68) { // Inj1
                DDRT |= 0x02;
                PWME &= ~0x01;
                OCPD |= 0x02;
                PTP |= 0x01;
                port_testio = (unsigned char*)&PTT;
                pin_testio = 0x02;
            } else if (ioport == 0x6c) { // Inj2
                DDRT |= 0x08;
                PWME &= ~0x02;
                OCPD |= 0x08;
                PTP |= 0x02;
                port_testio = (unsigned char*)&PTT;
                pin_testio = 0x08;
            } else if (ioport == 0x70) { // PM2
                DDRM |= 0x04;
                port_testio = (unsigned char*)&PTM;
                pin_testio = 0x04;
            } else if (ioport == 0x74) { // PK1
                DDRK |= 0x02;
                port_testio = (unsigned char*)&PORTK;
                pin_testio = 0x02;
            } else if (ioport == 0x78) { // PK3
                DDRK |= 0x08;
                port_testio = (unsigned char*)&PORTK;
                pin_testio = 0x08;
            } else if (ioport == 0x7c) { // PK7
                DDRK |= 0x80;
                port_testio = (unsigned char*)&PORTK;
                pin_testio = 0x80;

            } else if (ioport == 0x100) { // CANOUT1
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x01;
            } else if (ioport == 0x104) { // CANOUT2
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x02;
            } else if (ioport == 0x108) { // CANOUT3
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x04;
            } else if (ioport == 0x10c) { // CANOUT4
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x08;
            } else if (ioport == 0x110) { // CANOUT5
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x10;
            } else if (ioport == 0x114) { // CANOUT6
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x20;
            } else if (ioport == 0x118) { // CANOUT7
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x40;
            } else if (ioport == 0x11c) { // CANOUT8
                port_testio = (unsigned char*)&outpc.canout1_8;
                pin_testio = 0x80;
            } else if (ioport == 0x120) { // CANOUT9
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x01;
            } else if (ioport == 0x124) { // CANOUT10
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x02;
            } else if (ioport == 0x128) { // CANOUT11
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x04;
            } else if (ioport == 0x12c) { // CANOUT12
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x08;
            } else if (ioport == 0x130) { // CANOUT13
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x10;
            } else if (ioport == 0x134) { // CANOUT14
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x20;
            } else if (ioport == 0x138) { // CANOUT15
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x40;
            } else if (ioport == 0x13c) { // CANOUT16
                port_testio = (unsigned char*)&outpc.canout9_16;
                pin_testio = 0x80;

            } else {
                port_testio = (unsigned char*)&dummyReg;
                pin_testio = 0;
            }

            testmode_glob = 0;
            if (ioport_mode == 0) {
                *port_testio &= ~pin_testio;
            } else if (ioport_mode == 2) {
                testmode_glob = 3;
            } else if (ioport_mode == 3) {
                *port_testio |= pin_testio;
            }

        } else { // invalid mode
            return 2;
        }
    } else {
        /* IAC tests do not require and not possible when in inj/spk test mode */
        if (datax1.testmodemode == 8) { // Cancel IAC test
            iactest_glob = 0;
            if ((outpc.rpm == 0) && ((IdleCtl == 4) || (IdleCtl == 6))) {
                pwmidle_reset = 4;
            }
        } else if (datax1.testmodemode == 9) { // IAC test home
            iactest_glob = 1;
            pwmidle_reset = PWMIDLE_RESET_INIT;
        } else if (datax1.testmodemode == 10) { // IAC test run
            iactest_glob = 3; // nb 2 is used as a holding phase after homing
        } else if (datax1.testmodemode == 0x18) { // IAC test in/out cycle
            iactest_glob = 4; // nb 2 is used as a holding phase after homing

        } else if ((datax1.testmodemode & 0xf8) == 0x10) {
            /* To make this work will need to have a global variable that hold sequential mode.
                Initialise it at boot time in normal operation and tweak it here. */
            if (datax1.testmodemode == 0x10) { // Inj normal
                glob_sequential = ram4.sequential;

            } else if (datax1.testmodemode == 0x11) { // Inj batch
                glob_sequential = (ram4.sequential & 0xfc) | 0x00;

            } else if (datax1.testmodemode == 0x12) { // Inj semi-seq
                glob_sequential = (ram4.sequential & 0xfc) | 0x01;

            } else if (datax1.testmodemode == 0x13) { // Inj sequential
                glob_sequential = (ram4.sequential & 0xfc) | 0x02;

            }

            calc_reqfuel(ram4.ReqFuel); // call now as sets up injector variables

            /* Set up injector port/pin mappings. Also called when test mode is cleared. */
            inj_event_array_portpin(0);
            if (do_dualouts) {
                inj_event_array_portpin(INJ_FILL_STAGED);
            }

        } else if (datax1.testmodemode == 11) { // turn off inj/spk disabling test
            int a;
            testmode_glob = 0;
            outpc.istatus5 = 0;
            outpc.status8 &= ~(STATUS8_INJDIS | STATUS8_SPKDIS);
            for (a = 0 ; a < no_triggers ; a++) {
                skipdwell[a] &= ~0x02; // clear testmode bit
            }
            skipinj_test = 0;

        } else if (datax1.testmodemode == 12) { // enable inj disabling test
            testmode_glob = 4;
            outpc.istatus5 = 0;
            outpc.status8 |= STATUS8_INJDIS;

        } else if (datax1.testmodemode == 13) { // enable spk disabling test
            testmode_glob = 5;
            outpc.istatus5 = 0;
            outpc.status8 |= STATUS8_SPKDIS;

        } else if ((testmode_glob == 4) && ((datax1.testmodemode & 0xf0) == 0x20)) { // injector/spark disabling
            unsigned int bits, out;

            bits = twopow[datax1.testmodemode & 0x0f];
            outpc.istatus5 ^= bits; // UI only

            for (out = 0; out < 16; out++) {
                if ((unsigned int)(ram4.fire[out] - 1) == (datax1.testmodemode & 0x0f)) { // find cylinder
                    break;
                }
            }
            bits = twopow[out];
            skipinj_test ^= bits; // inj skipping

        } else if ((testmode_glob == 5) && ((datax1.testmodemode & 0xf0) == 0x20)) { // injector/spark disabling
            unsigned int bits, out;

            bits = twopow[datax1.testmodemode & 0x0f];
            outpc.istatus5 ^= bits; // UI only

            for (out = 0; out < 16; out++) {
                if ((unsigned int)(ram4.fire[out] - 1) == (datax1.testmodemode & 0x0f)) { // find cylinder
                    break;
                }
            }
            skipdwell[out] ^= 0x02; // flip testmode bit
            /* Users of HD and MAP phase sense modes need to be aware that they'll disturb normal operation */

        } else {
            // tried to run an invalid test when not in test mode?!
            return 2;
        }
    }
    return 0;
}

void fuelpump_run(void)
{
    if (((unsigned int)lmms - fp_lmms) <= ram5.fp_ctl_ms) { // only run periodically
        return;
    } else {
        fp_lmms = (unsigned int)lmms;
    }

    /* only called when running */
    if ((ram5.fp_opt & 0x03) == 0x01) { // open loop table
        outpc.fp_duty = intrp_2dctable(outpc.rpm, outpc.fuelload, 6, 6,
                        &ram_window.pg25.fpd_rpm[0],
                        &ram_window.pg25.fpd_load[0],
                        &ram_window.pg25.fpd_duty[0][0], 0, 25);
    } else if ((ram5.fp_opt & 0x03) == 0x02) { // closed loop
        long tmp1;
        int tmp2;
        unsigned char flags = PID_TYPE_B;
#define MAX_PRESS ram5.rail_pressure + 500   // need this as a setting most likely

        if (!fp_PID_enabled) {
            flags |= PID_INIT;
            fp_PID_enabled = 1;
            fp_ctl_duty_100 = outpc.fp_duty * 100;
        }

// differential pressure for both target and actual
        tmp1 = fp_ctl_duty_100
                + (((long) (generic_pid_routine(0, MAX_PRESS, ram5.rail_pressure, outpc.fuel_press[0],
                ram5.fp_Kp, ram5.fp_Ki, ram5.fp_Kd/10,
                ram5.fp_ctl_ms, fp_ctl_last_pv, &fp_ctl_last_error, flags))
                * (ram5.fp_max_duty - ram5.fp_min_duty)) / 100000); // added an extra 00 to tame sensitivity

        tmp2 = tmp1 / 100;

        if (tmp2 < ram5.fp_min_duty) {
            tmp2 = ram5.fp_min_duty;
            tmp1 = tmp2 * 100;
        } else if (tmp2 > ram5.fp_max_duty) {
            tmp2 = ram5.fp_max_duty;
            tmp1 = tmp2 * 100;
        }

        fp_ctl_duty_100 = tmp1;
        outpc.fp_duty = tmp2;
        /* also want to extend flow correction based on actual reported fuel pressure */

    } else { // on/off
        if (outpc.fp_duty == 0) {
            outpc.fp_duty = 255;
            SSEM0;
            *port_fp |= pin_fp;
            CSEM0;
        }
    }
}

void fuelpump_prime(void)
{
    if (ram5.fp_opt & 0x03) { // open or closed loop
        outpc.fp_duty = ram5.fp_prime_duty;
    } else { // on/off
        outpc.fp_duty = 255;
        SSEM0;
        *port_fp |= pin_fp;
        CSEM0;
    }
    /* actual duty applied to output in RTC */
}

void fuelpump_off(void)
{
    if ((ram5.fp_opt & 0x03) == 0) { // on/off
        SSEM0;
        *port_fp &= ~pin_fp;
        CSEM0;
        outpc.fp_duty = 0;
    } else {
        outpc.fp_duty = ram5.fp_off_duty;
        /* actual duty applied to output in RTC */
    }

}

/**************************************************************************
 **
 ** Read fuel sensor inputs + fuel safety
 **
 **************************************************************************/
void fuel_sensors(void)
{
    unsigned char tmp_opt, valid = 0;

    /* Primary fuel temperature */
    tmp_opt = (ram5.fueltemp1 & 0x1f);
    if (tmp_opt) {
        int temperature = 0;
        if (tmp_opt == 1) {
            if (ff_temp_cnt >= 10) { // accumulated in flex code
                unsigned long t_diff;
                unsigned int max_min, ff_pw_local;

                if (ff_temp_accum > 6553) {
                    ff_pw_local = (ff_temp_accum * 10UL) / ff_temp_cnt;
                } else {
                    ff_pw_local = (ff_temp_accum * 10) / ff_temp_cnt;
                }
                /* in normal operation this gives 10x the value and additional
                    precision in the temperature calculation leading to a less steppy
                    output */
                ff_temp_cnt = 0;
                ff_temp_accum = 0;

                t_diff = ff_pw_local - (ram5.ff_tpw0 * 10U);
                max_min = (ram5.ff_tpw1 - ram5.ff_tpw0) * 10; // correct for the 10x earlier
                temperature = ram5.ff_temp0 + (int)(((ram5.ff_temp1 - ram5.ff_temp0) * t_diff) / max_min);
                valid = 1;
            }
        } else if (tmp_opt > 15) {
            temperature = outpc.sensors[tmp_opt - 16];
            if (ram5.sensor_temp & 0x01) {
                // if using degC need to reverse calc back to degF
                temperature = ((temperature * 9) / 5) + 320;
            }
            valid = 1;
        }

        if (valid) {
            __asm__ __volatile__(
                "suby   %3\n"
                "clra\n"
                "ldab    %2\n"    // it is a uchar
                "emuls\n"
                "ldx     #100\n"
                "edivs\n"
                "addy    %3\n"
                :"=y"(outpc.fuel_temp[0])
                :"y"(temperature), "m"(ram4.adcLF),"m"(outpc.fuel_temp[0])
                :"x"
            );
        }
    } else {
        outpc.fuel_temp[0] = 0;
    }

    /* Primary fuel pressure */
    tmp_opt = ram5.fp_press_in & 0x0f;
    if (tmp_opt) {
        /* custom fuel pressure correction */
        int pressure;
        pressure = outpc.sensors[tmp_opt - 1];
        if ((ram5.fp_press_in & 0xc0) == 0) {
            // That gives gauge pressure. Now convert to differential
            pressure = pressure + outpc.baro - outpc.map;
        } else if ((ram5.fp_press_in & 0xc0) == 0x40) {
        // That gives absolute pressure. Now convert to differential
            pressure = pressure - outpc.map;
        }
        // else Differential
        __asm__ __volatile__(
            "subd   %3\n"
            "tfr    d,y\n"
            "clra\n"
            "ldab    %2\n"    // it is a uchar
            "emuls\n"
            "ldx     #100\n"
            "edivs\n"
            "addy    %3\n"
            :"=y"(outpc.fuel_press[0])
            :"d"(pressure), "m"(ram4.adcLF),"m"(outpc.fuel_press[0])
            :"x"
        );

    } else {
        outpc.fuel_press[0] = 0;
    }

    /* Secondary fuel temperature */
    tmp_opt = ram5.dualfuel_temp_sens & 0x0f;
    if (tmp_opt) {
        int temperature;
        tmp_opt -= 1;
        temperature = outpc.sensors[tmp_opt];

        if (ram5.sensor_temp & 0x01) {
            // if using degC need to reverse calc back to degF
            temperature = ((temperature * 9) / 5) + 320;
        }
        __asm__ __volatile__(
            "subd   %3\n"
            "tfr    d,y\n"
            "clra\n"
            "ldab    %2\n"    // it is a uchar
            "emuls\n"
            "ldx     #100\n"
            "edivs\n"
            "addy    %3\n"
            :"=y"(outpc.fuel_temp[1])
            :"d"(temperature), "m"(ram4.adcLF),"m"(outpc.fuel_temp[1])
            :"x"
        );
    } else {
        outpc.fuel_temp[1] = 0;
    }

    /* Secondary fuel pressure */
    tmp_opt = ram5.dualfuel_press_sens & 0x0f;
    if (tmp_opt) {
        int pressure;
        pressure = outpc.sensors[tmp_opt - 1];
        if ((ram5.dualfuel_press_sens & 0xc0) == 0) {
            // That gives gauge pressure. Now convert to differential
            pressure = pressure + outpc.baro - outpc.map;
        } else if ((ram5.dualfuel_press_sens & 0xc0) == 0x40) {
        // That gives absolute pressure. Now convert to differential
            pressure = pressure - outpc.map;
        }
        // else Differential
        __asm__ __volatile__(
            "subd   %3\n"
            "tfr    d,y\n"
            "clra\n"
            "ldab    %2\n"    // it is a uchar
            "emuls\n"
            "ldx     #100\n"
            "edivs\n"
            "addy    %3\n"
            :"=y"(outpc.fuel_press[1])
            :"d"(pressure), "m"(ram4.adcLF),"m"(outpc.fuel_press[1])
            :"x"
        );
    } else {
        outpc.fuel_press[1] = 0;
    }

    /* fuel pressure safety */
    if ((ram5.fp_press_in & 0x0f) && (ram5.fp_opt & 0x80)) {
        /* check pressure against target and allowed drop */

        if ( (outpc.rpm > ram5.fp_drop_rpm) && (outpc.fuelload > ram5.fp_drop_load)
            && (outpc.fuel_press[0] < (int)ram5.rail_pressure)
            && (((int)ram5.rail_pressure - outpc.fuel_press[0]) > ram5.fp_drop) ) {
            if ((fpdrop_timer == 0) && (maxafr_stat == 0)) {
                fpdrop_timer = 1; // only start this timer if not in shutdown mode
            }
        } else {
            fpdrop_timer = 0; // cancel it
        }

        if (fpdrop_timer > ram5.fp_drop_time) {
            fpdrop_timer = 0; // cancel it, go to shut down
            if (maxafr_stat == 0) {
                maxafr_stat = 1;    // mainloop code reads the stat and cut appropriately
                maxafr_timer = 1;   // start counting
            }
            stat_fp |= 1;
        }
    }
}
#define ALT_HOLDOFF 20 /* number of loops to ignore zero RPM */
#define STARTV 110
void alternator(void)
{
    unsigned char mode;
    mode = ram5.alternator_opt & 0x07;
    if (mode == 0) {
        return;
    }

    if (((unsigned int)lmms - alt_lmms) <= ram5.alternator_ctl_ms) { // only run periodically
        return;
    } else {
        alt_lmms = (unsigned int)lmms;
    }

    /* Process input sensors */
    if (pin_alt_mon && altf_pw) {
        outpc.load_duty = (altf_pw * 100UL) / altf_period;
        altf_pw = 0;
    }

    if (pin_alt_curr && batc_pw) {
        int d;
        d = (batc_pw * 100UL) / batc_period;
        batc_pw = 0;
        /* need conversion from duty to current */
        outpc.batt_curr = 40 * (d - 50); // example data taking 50% duty as no current and +/-200A as full scale
    }


/* alternator state machine
    0 = engine off or cranking. Alternator at minimum output or off
    1 = transitioning to run
    2 = charge mode
    3 = transitioning to economy
    4 = 'economy' mode after initial charge
    0x12/14 = WOT mode when was in charge/economy
    0x22/24 = leaving WOT mode when was in charge/economy
*/

    /* Figure out what state we are in or should be in */
    if (((outpc.engine & ENGINE_READY) && (outpc.engine & ENGINE_CRANK)) || (outpc.rpm < 5)) { // off mode
        if (alt_holdoff == 0) {
            alt_state = 0;
        } else {
            alt_holdoff--; /* delay somewhat to prevent a sync-loss from immediately triggering crank behaviour */
        }
    } else if (alt_state >= 1) {
        alt_holdoff = ALT_HOLDOFF;
    }

    if (alt_state == 0) {
        outpc.alt_targv = 0; // target zero as want alternator to be off
        if (mode == 2) { // lookup frequency for lowest output voltage
            RPAGE = tables[29].rpg;
            outpc.alt_duty = ram_window.pg29.alternator_freqv[0];
        } else if (mode == 3) { // lookup duty for lowest output voltage
            RPAGE = tables[29].rpg;
            outpc.alt_duty = ram_window.pg29.alternator_dutyv[0];
        } else if (mode == 4) {
            outpc.alt_duty = 0; // turn off field
        }

        if ((outpc.engine & ENGINE_READY) && ((outpc.engine & ENGINE_CRANK) == 0) && (outpc.rpm > 5)) {
            unsigned long t;
            DISABLE_INTERRUPTS;
            t = lmms;
            ENABLE_INTERRUPTS;
            t -= tcrank_done_lmms;
            t /= 781; // convert to 0.1s

            if (t > ram5.alternator_startdelay) {
                alt_state = 1;
                DISABLE_INTERRUPTS;
                alt_startramp = lmms;
                ENABLE_INTERRUPTS;
            }
        }
    } else {
        unsigned char chargev;

        /* lookup charge voltage from temp sensor or use fixed value */
        if (ram5.alternator_tempin & 0x07) {
            int temperature;
            unsigned char t, tmp_opt;

            tmp_opt = (ram5.alternator_tempin & 0x07)- 1;
            temperature = outpc.sensors[tmp_opt];

            t = ram5.sensor_trans[tmp_opt];
            if (((t == 3) || (t == 4)) && (ram5.sensor_temp & 0x01)) {
                // if using CLT, MAT sensor tranform and in degC need to reverse calc back to degF
                temperature = ((temperature * 9) / 5) + 320;
            }

            chargev = intrp_1dctable(temperature, 5, 
                (int *) ram_window.pg29.alternator_temp, 0, 
                ram_window.pg29.alternator_targvolts, 29);
        } else {
            chargev = ram5.alternator_targv;
        }

        if (alt_state == 1) { // ramp up
            unsigned long t;
            DISABLE_INTERRUPTS;
            t = lmms;
            ENABLE_INTERRUPTS;
            t -= alt_startramp;
            t /= 781; // convert to 0.1s
            if ((mode == 1) || (t > ram5.alternator_ramptime)) {
                alt_state = 2;
            } else {
                // ramp up. Interpolate between STARTV and Charge voltage
                outpc.alt_targv = STARTV + (((long)t * (chargev - STARTV)) / ram5.alternator_ramptime); 
            }
        } else if (alt_state == 2) { // charge mode
            if ((outpc.seconds - tcrank_done) > ((unsigned int)ram5.alternator_chargetime * 60)) {
                alt_state = 3;
                DISABLE_INTERRUPTS;
                alt_startramp = lmms;
                ENABLE_INTERRUPTS;
            } else if (outpc.tps > ram5.alternator_wot) {
                alt_state += 0x10;
                alt_startwot = outpc.seconds;
                alt_lastvt = outpc.alt_targv;
            } else {
                outpc.alt_targv = chargev;
            }

        } else if (alt_state == 3) { // ramp down
            unsigned long t;
            DISABLE_INTERRUPTS;
            t = lmms;
            ENABLE_INTERRUPTS;
            t -= alt_startramp;
            t /= 781; // convert to 0.1s
            if (t > ram5.alternator_ramptime) {
                alt_state = 4;
            } else if ((outpc.tps > ram5.alternator_wot) && ((flagbyte19 & FLAGBYTE19_ALTWOTLO) == 0)) {
                alt_state += 0x10;
                alt_startwot = outpc.seconds;
                alt_lastvt = outpc.alt_targv;
            } else {
                // ramp up. Interpolate between ChargeV and Run voltage
                outpc.alt_targv = chargev + (((long)t * (ram5.alternator_targvr - chargev)) / ram5.alternator_ramptime); 
            }

        } else if (alt_state == 4) { // economy mode
            if ((outpc.tps > ram5.alternator_wot) && ((flagbyte19 & FLAGBYTE19_ALTWOTLO) == 0)) {
                alt_state += 0x10;
                alt_startwot = outpc.seconds;
                alt_lastvt = outpc.alt_targv;
            } else {
                if (flagbyte17 & FLAGBYTE17_OVERRUNFC) {
                    outpc.alt_targv = ram5.alternator_overrv; // in over-run fuel cut mode (could enable sooner)
                } else {
                    outpc.alt_targv = ram5.alternator_targvr;
                }
            }
        } else if (alt_state > 0x20) { // WOT mode fadeout
            unsigned long t;
            DISABLE_INTERRUPTS;
            t = lmms;
            ENABLE_INTERRUPTS;
            t -= alt_startramp;
            t /= 781; // convert to 0.1s
            if (t > ram5.alternator_ramptime) {
                alt_state -= 0x20; // drop back to state we were in
            } else {
                // ramp . Interpolate between STARTV and Charge voltage
                outpc.alt_targv = ram5.alternator_wotv + (((long)t * (alt_lastvt - ram5.alternator_wotv)) / ram5.alternator_ramptime); 
            }
        } else if (alt_state > 0x10) { // WOT mode
            if (((outpc.seconds - alt_startwot) > ram5.alternator_wottimeout) || (outpc.tps < (ram5.alternator_wot - 50))) {
                alt_state += 0x10;
                DISABLE_INTERRUPTS;
                alt_startramp = lmms;
                ENABLE_INTERRUPTS;
                flagbyte19 |= FLAGBYTE19_ALTWOTLO;
            } else {
                outpc.alt_targv = ram5.alternator_wotv;
            }
        }
    }

    if ((flagbyte19 & FLAGBYTE19_ALTWOTLO) && (outpc.tps < (ram5.alternator_wot - 50))) {
        flagbyte19 &= ~FLAGBYTE19_ALTWOTLO; // clear locked out state
    }

    /* Now have a target voltage, so apply to output */
    if (mode == 1) { // L wire on/off mode - activate L to enable alternator
        if (outpc.alt_targv) {
            if ((ram5.alternator_controlout & 0x40) == 0) {
                SSEM0SEI;
                *port_alt_out |= pin_alt_out;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                *port_alt_out &= ~pin_alt_out;
                CSEM0CLI;
            }
        } else { // off
            if (ram5.alternator_controlout & 0x40) {
                SSEM0SEI;
                *port_alt_out |= pin_alt_out;
                CSEM0CLI;
            } else {
                SSEM0SEI;
                *port_alt_out &= ~pin_alt_out;
                CSEM0CLI;
            }
        }
    } else if (mode == 2) { // frequency voltage lookup
        outpc.alt_duty = intrp_1dctable(outpc.alt_targv, 5, 
                (int *) ram_window.pg29.alternator_fvolts, 0, 
                ram_window.pg29.alternator_freqv, 29); 
    } else if (mode == 3) { // duty voltage lookup
        outpc.alt_duty = intrp_1dctable(outpc.alt_targv, 5, 
                (int *) ram_window.pg29.alternator_dvolts, 0, 
                ram_window.pg29.alternator_dutyv, 29); 
    } else if (mode == 4) { // closed loop field control
        long tmp1;
        int tmp2;
        unsigned char flags = PID_TYPE_B, external_duty;
#define MINALT_VOLTS 0
#define MAXALT_VOLTS 201 

        if (!alt_PID_enabled) {
            flags |= PID_INIT;
            alt_PID_enabled = 1;
            alt_ctl_duty_100 = outpc.alt_duty * 100;
            alternator_sensitivity_old = ram5.alternator_sensitivity;
        }

        if (alternator_sensitivity_old != ram5.alternator_sensitivity) {
            flags |= PID_INIT;
            alternator_sensitivity_old = ram5.alternator_sensitivity;
        }

// may want to use more precise voltage reading instead of 0.1V steps in outpc
        tmp1 = alt_ctl_duty_100
                + (((long) (generic_pid_routine(MINALT_VOLTS, MAXALT_VOLTS -
                ram5.alternator_sensitivity, outpc.alt_targv, outpc.batt,
                ram5.alternator_Kp, ram5.alternator_Ki, ram5.alternator_Kd/10,
                ram5.alternator_ctl_ms, alt_ctl_last_pv, &alt_ctl_last_error, flags))) / 10000);

        tmp2 = tmp1 / 100;

        /* Take the linear output from PID and turn it into a non-linear output
         * for the alternator so that change in volts is linear based on change in
         * PID output
         */

        external_duty = intrp_1dctable(tmp2, 7, ram_window.pg25.alt_dutyin, 0, 
                              ram_window.pg25.alt_dutyout, 25);

        RPAGE = tables[25].rpg;

        if (tmp2 < ram_window.pg25.alt_dutyin[0]) {
            tmp2 = ram_window.pg25.alt_dutyin[0];
            tmp1 = ram_window.pg25.alt_dutyin[0] * 100;
        } else if (tmp2 > ram_window.pg25.alt_dutyin[6]) {
            tmp2 = ram_window.pg25.alt_dutyin[6];
            tmp1 = ram_window.pg25.alt_dutyin[6] * 100;
        }

        alt_ctl_duty_100 = tmp1;
        outpc.alt_duty = external_duty;

    } else if (mode == 5) { /* Miata */
        if (outpc.alt_targv != alt_targv_old) {
            unsigned int base_adc;

            base_adc = ((long)outpc.alt_targv * 1023) / (ram4.battmax - ram4.batt0);
            if (base_adc < 5) { /* When target voltage is zero, could get negative minimum */
                base_adc = 5;
            }
            alt_min_adc = base_adc - 1;
            alt_max_adc = base_adc + 1;
            alt_targv_old = outpc.alt_targv;
        }
    }

    /* lamp output */
    if (pin_alt_lamp) {
        int vd;
        vd = outpc.alt_targv - outpc.batt;
        if (vd < 0) {
            vd = -vd;
        }
        if ( (alt_state == 0) // starting
            || (vd > ram5.alternator_diff) // too large voltage difference
            || ((mode == 4) && (outpc.alt_duty > ram5.alternator_maxload)) // commanded duty too high
            || (mode && pin_alt_mon && (outpc.load_duty > ram5.alternator_maxload)) ) { // monitored field duty too high
            SSEM0SEI;
            *port_alt_lamp |= pin_alt_lamp;
            CSEM0CLI;
        } else {
            SSEM0SEI;
            *port_alt_lamp &= ~pin_alt_lamp;
            CSEM0CLI;
        }
    }
}

void shiftlight(void)
{
    if (pin_shift) {
        unsigned int lim;
        if ((ram5.shiftlight_opt & 0x80) && (ram4.gear_method & 0x03)) {
            int gear;
            gear = outpc.gear;
            if (gear < 1) {
                gear = 1;
            } else if (gear > 6) {
                gear = 6;
            } else if (gear > ram4.gear_no) {
                gear = ram4.gear_no;
            }
            lim = ram5.shiftlight_limit[gear - 1];
        } else {
            lim = ram5.shiftlight_limit[0];
        }
        if (flagbyte19 & FLAGBYTE19_SHIFT) { // already on
            if (outpc.rpm < (lim - 100)) {
                flagbyte19 &= ~FLAGBYTE19_SHIFT;
                SSEM0SEI;
                *port_shift &= ~pin_shift;
                CSEM0CLI;
            }
        } else {
            if (outpc.rpm > lim) {
                flagbyte19 |= FLAGBYTE19_SHIFT;
                SSEM0SEI;
                *port_shift |= pin_shift;
                CSEM0CLI;
            }
        }
    }
}

/* Oil pressure monitoring.
 * Two curves of min/max pressure vs. rpm.
 * unitless to allow user to calibrate as they desire, matches values from sensors[x]
 * can set a lamp or trigger CEL
 */
void oilpress(void)
{
    if (ram5.oilpress_in & 0x0f) {
        int p, p1, p2;
        p = outpc.sensors[(ram5.oilpress_in & 0x0f) - 1];

        p1 = intrp_1ditable(outpc.rpm, 6,
                        (int *) ram_window.pg25.oil_rpm, 0,
                        (int *) ram_window.pg25.oil_press_min, 25);

        p2 = intrp_1ditable(outpc.rpm, 6,
                        (int *) ram_window.pg25.oil_rpm, 0,
                        (int *) ram_window.pg25.oil_press_max, 25);

        if ((p < p1) || (p > p2)) { /* Pressure out of range */
            if (pin_oilpress_out) {
                SSEM0SEI;
                *port_oilpress_out |= pin_oilpress_out; // light this always
                CSEM0CLI;
            }
            if (!((outpc.rpm < 3) || (flagbyte2 & flagbyte2_crank_ok))) {
                stat_oil |= 1; // only flag CEL warning once running
            }
        } else { /* ok */
            if (pin_oilpress_out) {
                SSEM0SEI;
                *port_oilpress_out &= ~pin_oilpress_out;
                CSEM0CLI;
            }
        }
    }
}

void populate_maf(void)
{
    /* Convert the 64 point MAF user curve or the old-style 1024 lookup
       into a 1024 point internal lookup table */
    /* Lookup table will use maf_local 0-1023 to return g/sec (scaled) */
    /* Response on frequency MAFs will depend on mainloop time. */
    int x, v;
    unsigned int utmp1;

    for (x = 0; x < 1024 ; x++) {
        if (ram4.feature7 & 0x04) { /* old calibration table */
            GPAGE = 0x10;
            __asm__ __volatile__("aslx\n"
                                 "addx   #0x5400\n"    // maffactor_table address
                                 "gldy   0,x\n"
                                 :"=y"(utmp1)
                                 :"x"(x));
            if (utmp1 > 0xdfff) {
                utmp1 = 0xdfff;
            }
            utmp1 += 0x2000; // fixed offset for zero

        } else { // 64pt curve
            v = (5000L * x) / 1023; // 0.001V

            utmp1 = intrp_1ditable(v, 64, (int *)ram_window.pg25.mafv, 0, (unsigned int *)
                ram_window.pg25.mafflow, 25);
        }
        RPAGE = 0xf1;
        *(unsigned int*)(0x1000 + (x * 2)) = utmp1;
    }

    flagbyte21 &= ~FLAGBYTE21_POPMAF;
}

void bs_texte0(void)
{
    asm("nop");
}

void linelock_staging(void)
{
    if (ram5.llstg_in & 0x1f) {
        if (llstg_state == 0) {
            if (pin_llstg_in && ((*port_llstg_in & pin_llstg_in) == pin_match_llstg_in)) {
                /* turn on */
                llstg_state = 1;
                llstg_time = (unsigned int)lmms;
                SSEM0SEI;
                *port_llstg_out |= pin_llstg_out;
                CSEM0CLI;
                flagbyte23 |= FLAGBYTE23_LLSTG;
            }
        } else if (llstg_state == 1) {
            if (pin_llstg_in && ((*port_llstg_in & pin_llstg_in) != pin_match_llstg_in)) {
                if (((unsigned int)lmms - llstg_time) > 781) {
                    /* button released and debounce */
                    llstg_state = 2;
                }
            } else {
                llstg_time = (unsigned int)lmms;
            }
        } else if (llstg_state == 2) {
            if (pin_llstg_in && ((*port_llstg_in & pin_llstg_in) == pin_match_llstg_in)) {
                llstg_state = 3;
                llstg_time = (unsigned char)lmms;
                SSEM0SEI;
                *port_llstg_out &= ~pin_llstg_out;
                CSEM0CLI;
                flagbyte23 &= ~FLAGBYTE23_LLSTG;
            }
        } else if (llstg_state == 3) {
            if (pin_llstg_in && ((*port_llstg_in & pin_llstg_in) != pin_match_llstg_in)) {
                if (((unsigned int)lmms - llstg_time) > 781) {
                    /* button released and debounce */
                    llstg_state = 0;
                }
            } else {
                llstg_time = (unsigned int)lmms;
           }
        }

    }
}

#define PLINTERVAL 391 /* 50ms was 10ms */
#define PL_LOWER_DIV 100000L
#define PL_VSS_SOFT 50
#define PL_RPM_SOFT 200
void pitlim(void)
{
    /**************************************************************************
     ** Pit limiter
     ** Semi closed-loop control.
     ** Outputs into launch variables.
     **************************************************************************/
    long l, l2;
    unsigned int pv, pl_lower_max, range;

    RPAGE = tables[28].rpg;

    if (((ram_window.pg28.pitlim_opt & PITLIM_OPT_ON) == 0) || ((pin_plenin != 0) && ((*port_plenin & pin_plenin) != pin_match_plenin))) {
        // disabled or enable input on but inactive
        flagbyte24 &= ~FLAGBYTE24_PITLIMON;
        skipinj_pitlim = 0;
    } else if ((pin_plenin) && ((*port_plenin & pin_plenin) == pin_match_plenin)) {
        if ((ram_window.pg28.pitlim_opt & PITLIM_OPT_MODE) && (ram_window.pg28.pitlim_opt & PITLIM_OPT_VSSRPM)) {
            pv = outpc.rpm; /* Holding && RPM */
            pl_lower_max = PL_RPM_SOFT; /* soft RPM zone */
            range = ram_window.pg28.pitlim_rpm_range;
        } else {
            pv = outpc.vss1; /* Could be selectable VSS1 vs. VSS2*/
            pl_lower_max = PL_VSS_SOFT; /* soft VSS zone */
            range = ram_window.pg28.pitlim_speed_range;
        }
        if ((flagbyte24 & FLAGBYTE24_PITLIMON) == 0) {
            flagbyte24 |= FLAGBYTE24_PITLIMON;
            pltimer = (unsigned int)lmms;
            if (ram_window.pg28.pitlim_opt & PITLIM_OPT_MODE) {
                pl_vss = pv;
            } else {
                pl_vss = ram_window.pg28.pitlim_speed;
            }
            pl_sum = 0;
            pl_lower = 0; /* Start targetting the limit - otherwise might hit limiter immediately */
        } else if (((unsigned int)lmms - pltimer) > PLINTERVAL) {
            l2 = ((long)pv - (long)pl_vss) * ram_window.pg28.pitlim_sensitivity;
            pl_sum += l2;
            if (pl_sum > (PL_LOWER_DIV * pl_lower_max)) {
                pl_sum = PL_LOWER_DIV * pl_lower_max;
            } else if (pl_sum < 0) {
                pl_sum = 0;
            }

            pl_lower = pl_sum / PL_LOWER_DIV; /* Allow for control window to slowly adjust up/down */
            /* sliding scale in [range] window */
            if (pv < (pl_vss - pl_lower)) {
                l = 0;
            } else {
                l = (((long)pv - (long)(pl_vss - pl_lower)) * 255L) / range;
            }
            if (l > 255) {
                l = 255;
            }

            if (l) {
                if (ram_window.pg28.pitlim_opt & PITLIM_OPT_RETARD) {
                    int ret;
                    ret = (l * (long)ram_window.pg28.pitlim_retardmax) / 256;
                    outpc.launch_retard += ret;
                }

                if (ram_window.pg28.pitlim_opt & PITLIM_OPT_SPKCUT) {
                    if (l > spkcut_thresh_tmp) {
                        spkcut_thresh_tmp = l;
                    }
                    flagbyte10 |= FLAGBYTE10_SPKCUTTMP;
                }

                if (ram_window.pg28.pitlim_opt & PITLIM_OPT_FUELCUT) {
                    if (ram_window.pg28.pitlim_opt & PITLIM_OPT_FUELPROG) {
                        if (num_inj > 1) {
                            unsigned int step, b, base;
                            base = pl_vss - pl_lower;
                            step = range / (num_inj - 1);
                            for (b = 0 ; b < num_inj ; b++) {
                                if (pv > (base + (step * b))) {
                                    skipinj_pitlim |= fuelcut_array[num_inj - 1][b];
                                } else if (pv < (base + (step * b) - 100)) {
                                    skipinj_pitlim &= ~fuelcut_array[num_inj - 1][b];
                                }
                            }
                        } else {
                            skipinj_pitlim = 0;
                        }
                    } else { /* Fuel cut, non progressive */
                        if (l > 127) {
                            skipinj_pitlim = 65535;
                        } else {
                            skipinj_pitlim = 0;
                        }
                    }
                }
            } else {
                skipinj_pitlim = 0;
            }
        }
    } else {
        skipinj_pitlim = 0; /* ensure fuel is back on */
    }
    return;
}
