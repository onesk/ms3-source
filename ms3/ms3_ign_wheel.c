/* $Id: ms3_ign_wheel.c,v 1.152.4.9 2015/05/19 20:40:10 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * ign_wheel_init()
    Origin: Kenneth Culver
    Major: James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/

#include "ms3.h"

void ign_wheel_init(void)
{
    unsigned int j, tmp_deg_per_tooth;
    void *dummy;
    int tmp_offset;
    unsigned char phase;

    /* figure out any necessary parameters for the wheel decoder init */
    wheeldec_ovflo = 0;         // do this in global init as well

    dummy = memset((void *) dwell_events_a, 0, sizeof(dwell_events_a));
    dummy = memset((void *) spark_events_a, 0, sizeof(spark_events_a));
    dummy = memset((void *) dwell_events_b, 0, sizeof(dwell_events_b));
    dummy = memset((void *) spark_events_b, 0, sizeof(spark_events_b));
    dummy = memset((void *) inj_events_a, 0, sizeof(inj_events_a));
    dummy = memset((void *) inj_events_b, 0, sizeof(inj_events_b));
    dummy = memset((void *) map_start_event, 0, sizeof(map_start_event));

    dwell_events = dwell_events_a;
    spark_events = spark_events_a;
    inj_events = inj_events_a;

    phase = 0;
    if (spkmode == 4) {
        tmp_offset = 0;
    } else {
        tmp_offset = ram4.adv_offset;
        if (tmp_offset > 3000) {
            phase = 1;
            tmp_offset -= 3600;
        }
        if (tmp_offset < -200) {
            tmp_offset = -200;
        } else if (tmp_offset > 200) {
            tmp_offset = 200;
        }
        /* else leave unchanged */
    }

    calc_divider(); // and num_cyl

    cycle_deg = 3600;

    Rpm_Coeff = 120000000 / num_cyl;
    if (ram4.EngStroke & 0x01) { /* 2 stroke or rotary */
        Rpm_Coeff >>= 1; // halved for 2 stroke. OK for 3,4 cyl unsure about 1,2 cyl
    }

    // initially set bits to zero. Means single edge on crank only 
    flagbyte5 &=
        ~(FLAGBYTE5_CRK_DOUBLE | FLAGBYTE5_CRK_BOTH | FLAGBYTE5_CAM |
          FLAGBYTE5_CAM_DOUBLE | FLAGBYTE5_CAM_BOTH);

    /* ----------------------  EDIS MODEs ------------------------ */
    if (spkmode < 2) {
            trig_ang = 100; // 10BTDC - needs confirmation
        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)) { // use cam
            no_triggers = num_cyl;
            flagbyte5 |= FLAGBYTE5_CAM;
            cycle_deg = 7200;
        } else {
            no_triggers = num_cyl >> 1;
        }

        no_teeth = no_triggers;
        last_tooth = no_triggers;

        tmp_deg_per_tooth = (((unsigned int) 7200 / (char) num_cyl));
        for (j = 1; j <= no_triggers; j++) {
            deg_per_tooth[j - 1] = tmp_deg_per_tooth;
        }

        for (j = 0; j < no_triggers; j++) {
            trig_angs[j] = 100;
            trigger_teeth[j] = j + 1;
        }

        smallest_tooth_crk = tmp_deg_per_tooth;
        smallest_tooth_cam = 0;

    /* ----------------------  420A/NEON MODE ------------------------ */
    } else if (spkmode == 5) {
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE;

        deg_per_tooth[0] = 200;
        deg_per_tooth[1] = 200;
        deg_per_tooth[2] = 200;
        deg_per_tooth[3] = 1200;
        deg_per_tooth[4] = 200;
        deg_per_tooth[5] = 200;
        deg_per_tooth[6] = 200;
        deg_per_tooth[7] = 1200;

        smallest_tooth_crk = 60;        // now 6.0 deg, was 20.0 deg
        smallest_tooth_cam = 150;
        trig_angs[0] = -1110 + tmp_offset;      // 69 BTDC (on next event) = 111ATDC
        trig_angs[1] = -1110 + tmp_offset;      // 69 BTDC (on next event) = 111ATDC

        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)) {      //if COP mode or SEQ then double up pattern
            flagbyte5 |= FLAGBYTE5_CAM;
            deg_per_tooth[8] = 200;
            deg_per_tooth[9] = 200;
            deg_per_tooth[10] = 200;
            deg_per_tooth[11] = 1200;
            deg_per_tooth[12] = 200;
            deg_per_tooth[13] = 200;
            deg_per_tooth[14] = 200;
            deg_per_tooth[15] = 1200;

            trigger_teeth[0] = 13;
            trigger_teeth[1] = 1;
            trigger_teeth[2] = 5;
            trigger_teeth[3] = 9;
            trig_angs[2] = -1110 + tmp_offset;  // 69 BTDC (on next event) = 111ATDC
            trig_angs[3] = -1110 + tmp_offset;  // 69 BTDC (on next event) = 111ATDC
            no_triggers = 4;
            no_teeth = 16;
            cycle_deg = 7200;
        } else {
            trigger_teeth[0] = 5;
            trigger_teeth[1] = 1;
            no_triggers = 2;
            no_teeth = 8;
        }
        last_tooth = no_teeth;

        if (num_cyl != 4) {
            conf_err = 17;
        }

        /* ----------------------  36-2+2 36-1+1 NGC ------------------------ */

    } else if (spkmode == 6) {
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE;

        deg_per_tooth[0] = 100;
        deg_per_tooth[1] = 100;
        deg_per_tooth[2] = 100;
        deg_per_tooth[3] = 100;
        deg_per_tooth[4] = 100;
        deg_per_tooth[5] = 100;
        deg_per_tooth[6] = 100;
        deg_per_tooth[7] = 100;
        deg_per_tooth[8] = 100;
        deg_per_tooth[9] = 100;
        deg_per_tooth[10] = 100;
        deg_per_tooth[11] = 100;
        deg_per_tooth[12] = 100;
        deg_per_tooth[13] = 100;
        deg_per_tooth[14] = 100;
        deg_per_tooth[15] = 300;

        deg_per_tooth[16] = 100;
        deg_per_tooth[17] = 100;
        deg_per_tooth[18] = 100;
        deg_per_tooth[19] = 100;
        deg_per_tooth[20] = 100;
        deg_per_tooth[21] = 100;
        deg_per_tooth[22] = 100;
        deg_per_tooth[23] = 100;
        deg_per_tooth[24] = 100;
        deg_per_tooth[25] = 100;
        deg_per_tooth[26] = 100;
        deg_per_tooth[27] = 100;
        deg_per_tooth[28] = 100;
        deg_per_tooth[29] = 100;
        deg_per_tooth[30] = 100;
        deg_per_tooth[31] = 300;

        smallest_tooth_crk = 100;
        smallest_tooth_cam = 360;
        if (num_cyl == 4) {
            no_triggers = 2;
            trigger_teeth[0] = 17;
            trigger_teeth[1] = 1;
            trig_angs[0] = -300 + tmp_offset;   // 30 ATDC (on next event)
            trig_angs[1] = -300 + tmp_offset;   // 30 ATDC (on next event)
        } else if (num_cyl == 6) {
            // these triggers should give rpm but are untested
            no_triggers = 3;
            trigger_teeth[0] = 1;
            trigger_teeth[1] = 13;
            trigger_teeth[2] = 23;
            trig_angs[0] = -300 + tmp_offset;   // 30 ATDC (on next event)
            trig_angs[1] = -300 + tmp_offset;   // 30 ATDC (on next event)
            trig_angs[2] = -300 + tmp_offset;   // 30 ATDC (on next event)
        } else if (num_cyl == 8) {
            // these triggers should give rpm but are untested
            no_triggers = 4;
            trigger_teeth[2] = 17;
            trigger_teeth[3] = 26;
            trigger_teeth[0] = 1;
            trigger_teeth[1] = 10;
            trig_angs[0] = -300 + tmp_offset;   // 30 ATDC (on next event)
            trig_angs[1] = -300 + tmp_offset;   // 30 ATDC (on next event)
            trig_angs[2] = -300 + tmp_offset;   // 30 ATDC (on next event)
            trig_angs[3] = -300 + tmp_offset;   // 30 ATDC (on next event)
        } else {
            conf_err = 1;
        }

        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)){      //if COP mode or use cam then double up pattern
            cycle_deg = 7200;
            flagbyte5 |= FLAGBYTE5_CAM;
            deg_per_tooth[32] = 100;
            deg_per_tooth[33] = 100;
            deg_per_tooth[34] = 100;
            deg_per_tooth[35] = 100;
            deg_per_tooth[36] = 100;
            deg_per_tooth[37] = 100;
            deg_per_tooth[38] = 100;
            deg_per_tooth[39] = 100;
            deg_per_tooth[40] = 100;
            deg_per_tooth[41] = 100;
            deg_per_tooth[42] = 100;
            deg_per_tooth[43] = 100;
            deg_per_tooth[44] = 100;
            deg_per_tooth[45] = 100;
            deg_per_tooth[46] = 100;
            deg_per_tooth[47] = 300;

            deg_per_tooth[48] = 100;
            deg_per_tooth[49] = 100;
            deg_per_tooth[50] = 100;
            deg_per_tooth[51] = 100;
            deg_per_tooth[52] = 100;
            deg_per_tooth[53] = 100;
            deg_per_tooth[54] = 100;
            deg_per_tooth[55] = 100;
            deg_per_tooth[56] = 100;
            deg_per_tooth[57] = 100;
            deg_per_tooth[58] = 100;
            deg_per_tooth[59] = 100;
            deg_per_tooth[60] = 100;
            deg_per_tooth[61] = 100;
            deg_per_tooth[62] = 100;
            deg_per_tooth[63] = 300;
            no_teeth = 64;

            if (num_cyl == 4) {
                no_triggers = 4;
                trigger_teeth[0] = 49;
                trigger_teeth[1] = 1;
                trigger_teeth[2] = 17;
                trigger_teeth[3] = 33;

                trig_angs[2] = -300 + tmp_offset;
                trig_angs[3] = -300 + tmp_offset;
            } else if (num_cyl == 6) {
                // these triggers should give rpm but are untested
                no_triggers = 6;
                trigger_teeth[0] = 1;
                trigger_teeth[1] = 13;
                trigger_teeth[2] = 23;
                trigger_teeth[3] = 33;
                trigger_teeth[4] = 45;
                trigger_teeth[5] = 55;
                trig_angs[3] = -300 + tmp_offset;
                trig_angs[4] = -300 + tmp_offset;
                trig_angs[5] = -300 + tmp_offset;
            } else if (num_cyl == 8) {
                // these triggers should give rpm but are untested
                no_triggers = 8;
                trigger_teeth[6] = 17;
                trigger_teeth[7] = 26;
                trigger_teeth[0] = 33;
                trigger_teeth[1] = 42;
                trigger_teeth[2] = 49;
                trigger_teeth[3] = 58;
                trigger_teeth[4] = 1;
                trigger_teeth[5] = 10;
                trig_angs[4] = -300 + tmp_offset;
                trig_angs[5] = -300 + tmp_offset;
                trig_angs[6] = -300 + tmp_offset;
                trig_angs[7] = -300 + tmp_offset;
            }
        } else {
            no_teeth = 32;
        }
        last_tooth = no_teeth;

        /* ----------------------  36-2-2-2 ------------------------ */

    } else if ((spkmode == 7) || (spkmode == 50) || (spkmode == 57)) {
        for (j = 0; j < 60; j++) {
            deg_per_tooth[j] = 100;
        }
        if ( (((ram4.EngStroke & 0x03) == 0x03) && (num_cyl == 2))
            || (((ram4.EngStroke & 0x03) != 0x03) && (num_cyl == 4)) ) {
            deg_per_tooth[15] = 300;
            deg_per_tooth[16] = 300;
            deg_per_tooth[29] = 300;

            smallest_tooth_crk = 100;

            trigger_teeth[0] = 6;
            trigger_teeth[1] = 20;

            trig_angs[0] = -150 + tmp_offset;       // 15 ATDC
            trig_angs[1] = -150 + tmp_offset;       // 15 ATDC

            /* 4-cyl + COP or seq */
            if (((ram4.EngStroke & 0x03) != 0x03)
                && (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL))) {
                //if COP mode or use cam then double up pattern. Not allowed in rotary mode - RX8 doesn't have a cam sensor.
                cycle_deg = 7200;
                flagbyte5 |= FLAGBYTE5_CAM;
                no_teeth = 60;
                no_triggers = 4;
                smallest_tooth_cam = 180;
                deg_per_tooth[45] = 300;
                deg_per_tooth[46] = 300;
                deg_per_tooth[59] = 300;
                trigger_teeth[0] = 6;
                trigger_teeth[1] = 20;
                trigger_teeth[2] = 36;
                trigger_teeth[3] = 50;
                trig_angs[2] = -150 + tmp_offset;   // 15 ATDC
                trig_angs[3] = -150 + tmp_offset;   // 15 ATDC
            } else {
                no_teeth = 30;
                no_triggers = 2;
                smallest_tooth_cam = 0;
            }
            last_tooth = no_teeth;
        } else if (((ram4.EngStroke & 0x03) != 0x03) && (num_cyl == 6)) {
            deg_per_tooth[0] = 300;
            deg_per_tooth[19] = 300;
            deg_per_tooth[29] = 300;

            smallest_tooth_crk = 100;

            trigger_teeth[0] = 26;
            trigger_teeth[1] = 4;
            trigger_teeth[2] = 16;

            trig_angs[0] = -150 + tmp_offset;       // 15 ATDC
            trig_angs[1] = -150 + tmp_offset;       // 15 ATDC
            trig_angs[2] = -150 + tmp_offset;       // 15 ATDC

            /* 6-cyl + COP or seq */
            if ( ((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL) ) {
                cycle_deg = 7200;
                flagbyte5 |= FLAGBYTE5_CAM;
                no_teeth = 60;
                no_triggers = 6;
                smallest_tooth_cam = 180;
                deg_per_tooth[30] = 300;
                deg_per_tooth[49] = 300;
                deg_per_tooth[59] = 300;

                trigger_teeth[0] = 26;
                trigger_teeth[1] = 34;
                trigger_teeth[2] = 46;
                trigger_teeth[3] = 56;
                trigger_teeth[4] = 4;
                trigger_teeth[5] = 16;
                trig_angs[3] = -150 + tmp_offset;   // 15 ATDC
                trig_angs[4] = -150 + tmp_offset;   // 15 ATDC
                trig_angs[5] = -150 + tmp_offset;   // 15 ATDC
            } else {
                no_teeth = 30;
                no_triggers = 3;
                smallest_tooth_cam = 0;
            }
            last_tooth = no_teeth;

        } else {
            /* only 2 rotor or 4-cyl or 6-cyl supported */
            if ((ram4.EngStroke & 0x03) == 0x03) {
                conf_err = 1;
            } else {
                conf_err = 17;
            }
        }

        /* ----------------------  Subaru 6/7 ------------------------ */

    } else if (spkmode == 8) {
        flagbyte5 |= FLAGBYTE5_CAM;
        deg_per_tooth[0] = 320;
        deg_per_tooth[1] = 550;
        deg_per_tooth[2] = 930;
        deg_per_tooth[3] = 320;
        deg_per_tooth[4] = 550;
        deg_per_tooth[5] = 930;

        smallest_tooth_crk = 320;
        smallest_tooth_cam = 50;        // (unsure)
        trigger_teeth[0] = 1;
        trigger_teeth[1] = 4;
        trig_angs[0] = -830 + tmp_offset;       // 83 ATDC
        trig_angs[1] = -830 + tmp_offset;       // 83 ATDC
        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)) {      //if COP mode or use cam then double up pattern
            cycle_deg = 7200;
            deg_per_tooth[6] = 320;
            deg_per_tooth[7] = 550;
            deg_per_tooth[8] = 930;
            deg_per_tooth[9] = 320;
            deg_per_tooth[10] = 550;
            deg_per_tooth[11] = 930;

            trigger_teeth[2] = 7;
            trigger_teeth[3] = 10;
            trig_angs[2] = -830 + tmp_offset;   // 83 ATDC
            trig_angs[3] = -830 + tmp_offset;   // 83 ATDC
            no_teeth = 12;      // for cop
            no_triggers = 4;
        } else {
            no_teeth = 6;       // for w/s
            no_triggers = 2;
        }
        last_tooth = no_teeth;
        if (num_cyl != 4) {
            conf_err = 17;
        }

        /* ----------------------  99-00 Miata  ------------------------ */
        // 4 on crank 1, 2 on cam
    } else if (spkmode == 9) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
        no_teeth = 8;
        last_tooth = no_teeth;
        no_triggers = 4;
        deg_per_tooth[0] = 700;
        deg_per_tooth[1] = 1100;
        deg_per_tooth[2] = 700;
        deg_per_tooth[3] = 1100;
        deg_per_tooth[4] = 700;
        deg_per_tooth[5] = 1100;
        deg_per_tooth[6] = 700;
        deg_per_tooth[7] = 1100;

        smallest_tooth_crk = 700;
        smallest_tooth_cam = 50;        // (unsure)

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 3;
        trigger_teeth[2] = 5;
        trigger_teeth[3] = 7;
        // no idea on this - need data
        trig_angs[0] = -1100 + tmp_offset;      // 110 ATDC
        trig_angs[1] = -1100 + tmp_offset;      // 110 ATDC
        trig_angs[2] = -1100 + tmp_offset;      // 110 ATDC
        trig_angs[3] = -1100 + tmp_offset;      // 110 ATDC
        if (num_cyl != 4) {
            conf_err = 17;      // needs 4 cyl
        }

        /* ----------------------  Mitsubishi 6G72 ------------------------ */

    } else if (spkmode == 10) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE | FLAGBYTE5_CAM;
        no_teeth = 12;
        last_tooth = no_teeth;
        no_triggers = 6;
        deg_per_tooth[0] = 700;
        deg_per_tooth[1] = 500;
        deg_per_tooth[2] = 700;
        deg_per_tooth[3] = 500;
        deg_per_tooth[4] = 700;
        deg_per_tooth[5] = 500;
        deg_per_tooth[6] = 700;
        deg_per_tooth[7] = 500;
        deg_per_tooth[8] = 700;
        deg_per_tooth[9] = 500;
        deg_per_tooth[10] = 700;
        deg_per_tooth[11] = 500;

        smallest_tooth_crk = 500;
        smallest_tooth_cam = 400;

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 3;
        trigger_teeth[2] = 5;
        trigger_teeth[3] = 7;
        trigger_teeth[4] = 9;
        trigger_teeth[5] = 11;
        trig_angs[0] = -450 + tmp_offset;       // 45 ATDC
        trig_angs[1] = -450 + tmp_offset;       // 45 ATDC
        trig_angs[2] = -450 + tmp_offset;       // 45 ATDC
        trig_angs[3] = -450 + tmp_offset;       // 45 ATDC
        trig_angs[4] = -450 + tmp_offset;       // 45 ATDC
        trig_angs[5] = -450 + tmp_offset;       // 45 ATDC
        if (num_cyl != 6) {
            conf_err = 19;
        }

        /* ----------------------  IAW Weber-Marelli ------------------------ */
    } else if (spkmode == 11) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
        no_teeth = 8;
        last_tooth = no_teeth;
        no_triggers = 4;
        deg_per_tooth[0] = 900;
        deg_per_tooth[1] = 900;
        deg_per_tooth[2] = 900;
        deg_per_tooth[3] = 900;
        deg_per_tooth[4] = 900;
        deg_per_tooth[5] = 900;
        deg_per_tooth[6] = 900;
        deg_per_tooth[7] = 900;

        smallest_tooth_crk = 900;
        smallest_tooth_cam = 1800;

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 3;
        trigger_teeth[2] = 5;
        trigger_teeth[3] = 7;

        trig_angs[0] = -950 + tmp_offset;       // 95 ATDC - per forum feedback
        trig_angs[1] = -950 + tmp_offset;
        trig_angs[2] = -950 + tmp_offset;
        trig_angs[3] = -950 + tmp_offset;
        if (num_cyl != 4) {
            conf_err = 24;
        }

        /* ----------------------  CAS 4/1 ------------------------ */
        // This slightly generic mode used for mitsi, Miata type wheels
        // Allows flexibility for user generated similar wheels.
    } else if (spkmode == 12) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE | FLAGBYTE5_CAM;
        no_teeth = 8;
        tmp_deg_per_tooth = (((unsigned int) 7200 / (char) num_cyl));   // degx10 between main teeth
        if (tmp_deg_per_tooth != 1800) {
            conf_err = 1;
        }

        for (j = 1; j <= no_teeth; j++) {
            unsigned int jtmp1;
            if (j & 1) {        // these might be transposed
                jtmp1 = ram4.trigret_ang;       // main trigger -> return
            } else {
                jtmp1 = tmp_deg_per_tooth - ram4.trigret_ang;   // return -> next main trigger
            }
            deg_per_tooth[j - 1] = jtmp1;
        }

        if (tmp_deg_per_tooth < ram4.trigret_ang) {
            smallest_tooth_crk = tmp_deg_per_tooth;
        } else {
            smallest_tooth_crk = ram4.trigret_ang;
        }
        smallest_tooth_cam = 7200;

        // user enters angle of start of slot e.g. 10deg. Convert to ATDC
        trig_angs[0] = ram4.adv_offset - 1800;
        trig_angs[1] = ram4.adv_offset - 1800;
        trig_angs[2] = ram4.adv_offset - 1800;
        trig_angs[3] = ram4.adv_offset - 1800;

        goto com_cas;           // common section a little below
        /* ----------------------  4G63 ------------------------ */
    } else if (spkmode == 13) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE | FLAGBYTE5_CAM;
        no_teeth = 8;

        deg_per_tooth[0] = 700;
        deg_per_tooth[1] = 1100;
        deg_per_tooth[2] = 700;
        deg_per_tooth[3] = 1100;
        deg_per_tooth[4] = 700;
        deg_per_tooth[5] = 1100;
        deg_per_tooth[6] = 700;
        deg_per_tooth[7] = 1100;

        smallest_tooth_crk = 700;
        smallest_tooth_cam = 3000;

        // user enters angle of start of slot e.g. 10deg. Convert to ATDC
        trig_angs[0] = -1030 + tmp_offset;      // 103 ATDC (per md95)
        trig_angs[1] = -1030 + tmp_offset;
        trig_angs[2] = -1030 + tmp_offset;
        trig_angs[3] = -1030 + tmp_offset;
        if (num_cyl != 4) {
            conf_err = 18;
        }

      com_cas:
        no_triggers = 4;
        last_tooth = no_teeth;
        if (phase == 1) { /* Allow phase to be swapped by entering ~360 deg offset */
            trigger_teeth[0] = 5;
            trigger_teeth[1] = 7;
            trigger_teeth[2] = 1;
            trigger_teeth[3] = 3;
        } else {
            trigger_teeth[0] = 1;
            trigger_teeth[1] = 3;
            trigger_teeth[2] = 5;
            trigger_teeth[3] = 7;
        }

        /* ----------------------  twin trigger  ------------------------ */
        // two independant crank triggers (e.g. 2 or 4 cyl bike)
        // we'll feed them into one control routine though
    } else if (spkmode == 14) {
        flagbyte5 |= FLAGBYTE5_CAM;     // strange one this. Although cam IC is used, shares crank ISR
        no_teeth = 2;
        last_tooth = no_teeth;
        no_triggers = 2;

        trigger_teeth[0] = 2;
        trigger_teeth[1] = 1;

        if ((ram4.spk_config & 2) && ((ram4.no_cyl & 0x1f) == 2)) { /* 2 cyl at cam speed */
            cycle_deg = 7200;
        } else { // crank speed
            cycle_deg = 3600;
            if ((ram4.no_cyl & 0x1f) == 2) {
                if (glob_sequential & (SEQ_FULL | SEQ_SEMI)) {
                    conf_err = 150;
                }
                if ((ram4.EngStroke & 0x01) == 0) { // 2 cyl 4-stroke
                    divider = divider << 1; // otherwise double correct fuel
                }

            } else if (!((num_cyl == 4) && ((ram4.spk_mode3 & 0xc0) == 0x40))) { // if 4 cyl but not w/s
                conf_err = 29;
            }
        }

        if (ram4.ICIgnOption & 0x8) {   // oddfire
            deg_per_tooth[0] = ram4.OddFireang;
            deg_per_tooth[1] = cycle_deg - ram4.OddFireang;
            trig_angs[0] = ram4.adv_offset - ram4.OddFireang;
            trig_angs[1] = ram4.adv_offset - (cycle_deg - ram4.OddFireang);
            if (ram4.OddFireang < (cycle_deg - ram4.OddFireang)) {
                smallest_tooth_crk = ram4.OddFireang;
            } else {
                smallest_tooth_crk = cycle_deg - ram4.OddFireang;
            }
        } else {
            smallest_tooth_crk = cycle_deg >> 1;
            deg_per_tooth[0] = smallest_tooth_crk;
            deg_per_tooth[1] = smallest_tooth_crk;
            trig_angs[0] = ram4.adv_offset - smallest_tooth_crk;
            trig_angs[1] = trig_angs[0];
        }

        smallest_tooth_cam = smallest_tooth_crk;

        /* ----------------------  Chrysler 2.2/2.5 ------------------------ */
    } else if (spkmode == 15) {
        /* revised October 2013, uses single edged crank only (ignores second sensor.)
            Gap in vane not used for timing */
        cycle_deg = 7200;
        no_teeth = 4;
        last_tooth = no_teeth;
        no_triggers = 4;
        deg_per_tooth[0] =  1800;
        deg_per_tooth[1] =  1800;
        deg_per_tooth[2] =  1800;
        deg_per_tooth[3] =  1800;

        smallest_tooth_crk = 206;
        smallest_tooth_cam = 206; // irrelevant

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 2;
        trigger_teeth[2] = 3;
        trigger_teeth[3] = 4;

        /* these need review */
        trig_angs[0] = 110 + tmp_offset; // 11 BTDC
        trig_angs[1] = 110 + tmp_offset;
        trig_angs[2] = 110 + tmp_offset;
        trig_angs[3] = 110 + tmp_offset;
        if (num_cyl != 4) {
            conf_err = 17;
        }

        /* ----------------------  Renix 44-2-2 ------------------------ */

    } else if (spkmode == 16) {
        int t;

        smallest_tooth_cam = 0;
        no_teeth = 20;  // for single coil
        no_triggers = 1;

        if (num_cyl == 4) {
            for (t = 0 ; t < 19; t++) {
                deg_per_tooth[t] = 82; // bizarre 8.18 degrees per tooth??!
            }

            deg_per_tooth[19] =  246; // double missing tooth = time * 3
            smallest_tooth_crk = 82;
            trigger_teeth[0] = 14;
            trig_angs[0] = -164 + tmp_offset; // 16.4 ATDC

            if ((ram4.spk_mode3 & 0xc0) || (glob_sequential & (SEQ_SEMI | SEQ_FULL))) {      //if W/S or COP mode or use cam then double up pattern
                cycle_deg = 7200;
                flagbyte5 |= FLAGBYTE5_CAM;

                for (t = 20 ; t < 79; t++) {
                    deg_per_tooth[t] = 82;
                }

                deg_per_tooth[39] =  246;
                deg_per_tooth[59] =  246;
                deg_per_tooth[79] =  246;

                no_teeth = 80;  // for w/s or COP
                no_triggers = 4;
                trigger_teeth[1] = 34;
                trigger_teeth[2] = 54;
                trigger_teeth[3] = 74;
                trig_angs[1] = -164 + tmp_offset; // 16.4 ATDC
                trig_angs[2] = -164 + tmp_offset; // 16.4 ATDC
                trig_angs[3] = -164 + tmp_offset; // 16.4 ATDC
            }
        } else if (num_cyl == 6) {
            // V6 angles likely incorrect
            for (t = 0 ; t < 19; t++) {
                deg_per_tooth[t] = 54 + (t & 1); // bizarre 5.45 degrees per tooth??!
            }
            deg_per_tooth[19] =  165; // double missing tooth = time * 3
            smallest_tooth_crk = 54;
            trigger_teeth[0] = 14;
            trig_angs[0] = -164 + tmp_offset; // 16.4 ATDC

            if ((ram4.spk_mode3 & 0xc0) || (glob_sequential & (SEQ_SEMI | SEQ_FULL))) {      //if W/S or COP mode or use cam then double up pattern
                cycle_deg = 7200;
                flagbyte5 |= FLAGBYTE5_CAM;


                for (t = 20 ; t < 119; t++) {
                    deg_per_tooth[t] = 54 + (t & 1); // bizarre 5.45 degrees per tooth??!
                }

                deg_per_tooth[39] =  246;
                deg_per_tooth[59] =  246;
                deg_per_tooth[79] =  246;
                deg_per_tooth[99] =  246;
                deg_per_tooth[119]=  246;

                no_teeth = 120;  // for w/s or COP
                no_triggers = 6;
                trigger_teeth[1] = 34;
                trigger_teeth[2] = 54;
                trigger_teeth[3] = 74;
                trigger_teeth[4] = 94;
                trigger_teeth[5] = 114;
                trig_angs[1] = -164 + tmp_offset; // 16.4 ATDC
                trig_angs[2] = -164 + tmp_offset; // 16.4 ATDC
                trig_angs[3] = -164 + tmp_offset; // 16.4 ATDC
                trig_angs[4] = -164 + tmp_offset; // 16.4 ATDC
                trig_angs[5] = -164 + tmp_offset; // 16.4 ATDC
            }
        } else {
            conf_err = 1;
        }

        last_tooth = no_teeth;

        /* ----------------------  Suzuki swift ------------------------ */
    } else if (spkmode == 17) {
        no_teeth = 6;
        last_tooth = no_teeth;
        no_triggers = 2;
        deg_per_tooth[0] = 300;
        deg_per_tooth[1] = 550;
        deg_per_tooth[2] = 950;
        deg_per_tooth[3] = 300;
        deg_per_tooth[4] = 550;
        deg_per_tooth[5] = 950;

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 4;
        trig_angs[0] = -890 + tmp_offset;       // 89 ATDC
        trig_angs[1] = -890 + tmp_offset;       // 89 ATDC
        if (num_cyl != 4) {
            conf_err = 23;
        }

        /* ----------------------  Suzuki Vitara 2.0 ------------------------ */
    } else if (spkmode == 18) {
        cycle_deg = 7200;
        no_teeth = 11;
        last_tooth = no_teeth;
        no_triggers = 4;
        deg_per_tooth[0] = 700;
        deg_per_tooth[1] = 300;
        deg_per_tooth[2] = 800;
        deg_per_tooth[3] = 700;
        deg_per_tooth[4] = 800;
        deg_per_tooth[5] = 300;
        deg_per_tooth[6] = 700;
        deg_per_tooth[7] = 1100;
        deg_per_tooth[8] = 700;
        deg_per_tooth[9] = 800;
        deg_per_tooth[10] = 300;

        trigger_teeth[0] = 4;
        trigger_teeth[1] = 7;
        trigger_teeth[2] = 9;
        trigger_teeth[3] = 1;
        trig_angs[0] = -1100 + tmp_offset;      // 90 ATDC
        trig_angs[1] = -1100 + tmp_offset;      // 90 ATDC
        trig_angs[2] = -1100 + tmp_offset;      // 90 ATDC
        trig_angs[3] = -1100 + tmp_offset;      // 90 ATDC
        if (num_cyl != 4) {
            conf_err = 22;
        }

        /* ----------------------  Daihatsu 3cyl ------------------------ */
    } else if (spkmode == 19) {
        cycle_deg = 7200;
        if (ram_window.pg24.vvt_opt1 & 0x03) {
            flagbyte5 |= FLAGBYTE5_CAM; /* With VVT */
        }
        no_teeth = 4;
        last_tooth = no_teeth;
        no_triggers = 3;
        deg_per_tooth[0] = 2100;
        deg_per_tooth[1] = 2400;
        deg_per_tooth[2] = 2400;
        deg_per_tooth[3] = 300;

        trigger_teeth[0] = 3;
        trigger_teeth[1] = 4;
        trigger_teeth[2] = 2;
        trig_angs[0] = -2400 + tmp_offset;      // 240 ATDC
        trig_angs[1] = -2400 + tmp_offset;      // 240 ATDC
        trig_angs[2] = -2400 + tmp_offset;      // 240 ATDC
        if (num_cyl != 3) {
            conf_err = 20;
        }

        /* ----------------------  Daihatsu 4cyl ------------------------ */
    } else if (spkmode == 20) {
        cycle_deg = 7200;
        if (ram_window.pg24.vvt_opt1 & 0x03) {
            flagbyte5 |= FLAGBYTE5_CAM; /* With VVT */
        }
        no_teeth = 5;
        last_tooth = no_teeth;
        no_triggers = 4;
        deg_per_tooth[0] = 1500;
        deg_per_tooth[1] = 1800;
        deg_per_tooth[2] = 1800;
        deg_per_tooth[3] = 1800;
        deg_per_tooth[4] = 300;

        trigger_teeth[0] = 3;
        trigger_teeth[1] = 4;
        trigger_teeth[2] = 5;
        trigger_teeth[3] = 2;
        trig_angs[0] = -1800 + tmp_offset;      // 180 ATDC
        trig_angs[1] = -1800 + tmp_offset;      // 180 ATDC
        trig_angs[2] = -1800 + tmp_offset;      // 180 ATDC
        trig_angs[3] = -1800 + tmp_offset;      // 180 ATDC
        if (num_cyl != 4) {
            conf_err = 21;
        }

        /* ----------------------  Honda VTR1000 V-twin ------------------------ */
    } else if (spkmode == 21) {
        // runs in wasted spark at the moment using only crank sensor
        // user must set "odd fire" to get stable rpms
        no_teeth = 9;
        last_tooth = no_teeth;
        no_triggers = 2;
        deg_per_tooth[0] = 300;
        deg_per_tooth[1] = 300;
        deg_per_tooth[2] = 300;
        deg_per_tooth[3] = 300;
        deg_per_tooth[4] = 300;
        deg_per_tooth[5] = 300;
        deg_per_tooth[6] = 300;
        deg_per_tooth[7] = 300;
        deg_per_tooth[8] = 1200;        // 3 missing teeth = 4 x 30deg

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 7;
        trig_angs[0] = -1200 + tmp_offset;      // front cyl TDC is tooth no. 9 
        trig_angs[1] = -600 + tmp_offset;       // rear  cyl TDC is tooth no. 6

        if ( /*(num_cyl !=2 ) || */ (!(ram4.ICIgnOption & 0x08))
            || ((ram4.spk_mode3 & 0xc0) != 0x40)) {
            conf_err = 34;      // error if not 2 cyl OR not oddfire OR not wasted spark
        }

        /* ----------------------  Rover 36-1-1 ------------------------ */

    } else if (spkmode == 22) {
        no_teeth = 17;          // for single coil
        last_tooth = no_teeth;
        no_triggers = 1;
        deg_per_tooth[0] = 100;
        deg_per_tooth[1] = 100;
        deg_per_tooth[2] = 100;
        deg_per_tooth[3] = 100;
        deg_per_tooth[4] = 100;
        deg_per_tooth[5] = 100;
        deg_per_tooth[6] = 100;
        deg_per_tooth[7] = 100;
        deg_per_tooth[8] = 100;
        deg_per_tooth[9] = 100;
        deg_per_tooth[10] = 100;
        deg_per_tooth[11] = 100;
        deg_per_tooth[12] = 100;
        deg_per_tooth[13] = 100;
        deg_per_tooth[14] = 100;
        deg_per_tooth[15] = 100;
        deg_per_tooth[16] = 200;

        trigger_teeth[0] = 1;
        trig_angs[0] = -300 + tmp_offset;       // to be determined 
        if ((num_cyl != 4) || ((ram4.spk_mode3 & 0xc0) == 0x40)) {
            conf_err = 35;
        }

        /* ----------------------  Rover 36-1-1-1-1 type '2' (EU3) -------------------- */
        // this one has 2 and 3 teeth between the gaps
    } else if (spkmode == 23) {
        deg_per_tooth[0] = 100;
        deg_per_tooth[1] = 100;
        deg_per_tooth[2] = 100;
        deg_per_tooth[3] = 100;
        deg_per_tooth[4] = 100;
        deg_per_tooth[5] = 100;
        deg_per_tooth[6] = 100;
        deg_per_tooth[7] = 100;
        deg_per_tooth[8] = 100;
        deg_per_tooth[9] = 100;
        deg_per_tooth[10] = 100;
        deg_per_tooth[11] = 100;
        deg_per_tooth[12] = 100;
        deg_per_tooth[13] = 200;
        deg_per_tooth[14] = 100;
        deg_per_tooth[15] = 100;
        deg_per_tooth[16] = 200;
        deg_per_tooth[17] = 100;
        deg_per_tooth[18] = 100;
        deg_per_tooth[19] = 100;
        deg_per_tooth[20] = 100;
        deg_per_tooth[21] = 100;
        deg_per_tooth[22] = 100;
        deg_per_tooth[23] = 100;
        deg_per_tooth[24] = 100;
        deg_per_tooth[25] = 100;
        deg_per_tooth[26] = 100;
        deg_per_tooth[27] = 100;
        deg_per_tooth[28] = 100;
        deg_per_tooth[29] = 200;
        deg_per_tooth[30] = 100;
        deg_per_tooth[31] = 200;

        smallest_tooth_crk = 100;
        smallest_tooth_cam = 0;

        trigger_teeth[0] = 28;  // changed from 29 per JB
        trig_angs[0] = -100 + tmp_offset;       // to be confirmed
        trig_angs[1] = -100 + tmp_offset;       // to be confirmed 

        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)){      // COP or use-cam
            cycle_deg = 7200;
            flagbyte5 |= FLAGBYTE5_CAM;
            deg_per_tooth[32] = 100;
            deg_per_tooth[33] = 100;
            deg_per_tooth[34] = 100;
            deg_per_tooth[35] = 100;
            deg_per_tooth[36] = 100;
            deg_per_tooth[37] = 100;
            deg_per_tooth[38] = 100;
            deg_per_tooth[39] = 100;
            deg_per_tooth[40] = 100;
            deg_per_tooth[41] = 100;
            deg_per_tooth[42] = 100;
            deg_per_tooth[43] = 100;
            deg_per_tooth[44] = 100;
            deg_per_tooth[45] = 200;
            deg_per_tooth[46] = 100;
            deg_per_tooth[47] = 100;
            deg_per_tooth[48] = 200;
            deg_per_tooth[49] = 100;
            deg_per_tooth[50] = 100;
            deg_per_tooth[51] = 100;
            deg_per_tooth[52] = 100;
            deg_per_tooth[53] = 100;
            deg_per_tooth[54] = 100;
            deg_per_tooth[55] = 100;
            deg_per_tooth[56] = 100;
            deg_per_tooth[57] = 100;
            deg_per_tooth[58] = 100;
            deg_per_tooth[59] = 100;
            deg_per_tooth[60] = 100;
            deg_per_tooth[61] = 200;
            deg_per_tooth[62] = 100;
            deg_per_tooth[63] = 200;

            trigger_teeth[1] = 44;
            trigger_teeth[2] = 60;
            trigger_teeth[3] = 12;
            no_teeth = 64;      // using cam signal
            no_triggers = 4;
        } else {
            no_teeth = 32;      // for single coil or wasted spark
            no_triggers = 2;
            trigger_teeth[1] = 12;
        }
        last_tooth = no_teeth;
        if (num_cyl != 4) {
            conf_err = 17;
        }

        /* ----------------------  Rover 36-1-1-1-1 type '3' (??) -------------------- */
        // this one has 4 and 5 teeth between the gaps
    } else if (spkmode == 24) {
        no_teeth = 32;          // for single coil or wasted spark
        last_tooth = no_teeth;
        no_triggers = 2;
        deg_per_tooth[0] = 100;
        deg_per_tooth[1] = 100;
        deg_per_tooth[2] = 100;
        deg_per_tooth[3] = 100;
        deg_per_tooth[4] = 100;
        deg_per_tooth[5] = 100;
        deg_per_tooth[6] = 100;
        deg_per_tooth[7] = 100;
        deg_per_tooth[8] = 100;
        deg_per_tooth[9] = 100;
        deg_per_tooth[10] = 200;
        deg_per_tooth[11] = 100;
        deg_per_tooth[12] = 100;
        deg_per_tooth[13] = 100;
        deg_per_tooth[14] = 100;
        deg_per_tooth[15] = 200;
        deg_per_tooth[16] = 100;
        deg_per_tooth[17] = 100;
        deg_per_tooth[18] = 100;
        deg_per_tooth[19] = 100;
        deg_per_tooth[20] = 100;
        deg_per_tooth[21] = 100;
        deg_per_tooth[22] = 100;
        deg_per_tooth[23] = 100;
        deg_per_tooth[24] = 100;
        deg_per_tooth[25] = 100;
        deg_per_tooth[26] = 100;
        deg_per_tooth[27] = 200;
        deg_per_tooth[28] = 100;
        deg_per_tooth[29] = 100;
        deg_per_tooth[30] = 100;
        deg_per_tooth[31] = 200;

        trigger_teeth[0] = 6;
        trigger_teeth[1] = 23;
        trig_angs[0] = -100 + tmp_offset;       // to be determined 
        trig_angs[1] = -100 + tmp_offset;       // to be determined 
        if (num_cyl != 4) {
            conf_err = 17;
        }

        /* ----------------------  GM 7X reluctor natively -------------------- */
    } else if (spkmode == 25) {
        /* Changed 2014-08-12 to use single edge on cam, may run into polarity dependency issue */
        deg_per_tooth[0] = 600;
        deg_per_tooth[1] = 600;
        deg_per_tooth[2] = 600;
        deg_per_tooth[3] = 600;
        deg_per_tooth[4] = 600;
        deg_per_tooth[5] = 100;
        deg_per_tooth[6] = 500;

        smallest_tooth_crk = 100;

        if (num_cyl == 4) {
            no_triggers = 2;
            trigger_teeth[0] = 6;
            trigger_teeth[1] = 3;
            trig_angs[0] = -450 + tmp_offset;
            trig_angs[1] = -450 + tmp_offset;
        } else if (num_cyl == 6) {
            no_triggers = 3;
            trigger_teeth[0] = 5;
            trigger_teeth[1] = 1;
            trigger_teeth[2] = 3;
            trig_angs[0] = -450 + tmp_offset;
            trig_angs[1] = -450 + tmp_offset;
            trig_angs[2] = -450 + tmp_offset;
        } else {
            conf_err = 1;
        }

        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)) {      //if COP mode or use cam then double up pattern
            flagbyte5 |= FLAGBYTE5_CAM;
            cycle_deg = 7200;
            no_teeth = 14;           // COP / seq
            deg_per_tooth[7] = 600;
            deg_per_tooth[8] = 600;
            deg_per_tooth[9] = 600;
            deg_per_tooth[10] = 600;
            deg_per_tooth[11] = 600;
            deg_per_tooth[12] = 100;
            deg_per_tooth[13] = 500;
            smallest_tooth_cam = 20;

            if (num_cyl == 4) {
                no_triggers = 4;
                trigger_teeth[0] = 13;
                trigger_teeth[2] = 6;
                trigger_teeth[3] = 10;
                trig_angs[2] = -450 + tmp_offset;
                trig_angs[3] = -450 + tmp_offset;
            } else if (num_cyl == 6) {
                no_triggers = 6;
                trigger_teeth[0] = 12;
                trigger_teeth[3] = 5;
                trigger_teeth[4] = 8;
                trigger_teeth[5] = 10;
                trig_angs[3] = -450 + tmp_offset;
                trig_angs[4] = -450 + tmp_offset;
                trig_angs[5] = -450 + tmp_offset;
            } else {
                conf_err = 1;
            }

        } else {
            no_teeth = 7;           // for single coil or wasted spark
            smallest_tooth_cam = 0;
        }
        last_tooth = no_teeth;


    } else if (spkmode == 26) {
        // this is crank logging only
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE;

    } else if (spkmode == 27) {
        // this is crank and cam logging only
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE | FLAGBYTE5_CAM | FLAGBYTE5_CAM_DOUBLE;

        /* ----------------------  Nissan QR25DE + VK56 -------------------- */
    } else if (spkmode == 28) {
        cycle_deg = 7200;
    	// QR25DE originally from FSG_PB_Patrick
        flagbyte5 |= FLAGBYTE5_CAM; //using cam notches for identification 
        no_teeth = 64;
        last_tooth = no_teeth;
        deg_per_tooth[0] =   100;
        deg_per_tooth[1] =   100;
        deg_per_tooth[2] =   100;
        deg_per_tooth[3] =   100;
        deg_per_tooth[4] =   100;
        deg_per_tooth[5] =   100;
        deg_per_tooth[6] =   100;
        deg_per_tooth[7] =   100;
        deg_per_tooth[8] =   100;
        deg_per_tooth[9] =   100;
        deg_per_tooth[10] =   100;
        deg_per_tooth[11] =   100;
        deg_per_tooth[12] =   100;
        deg_per_tooth[13] =   100;
        deg_per_tooth[14] =   100;
        deg_per_tooth[15] =   300;
        //gap here
        deg_per_tooth[16] =   100;
        deg_per_tooth[17] =   100;
        deg_per_tooth[18] =   100;
        deg_per_tooth[19] =   100;
        deg_per_tooth[20] =   100;
        deg_per_tooth[21] =   100;
        deg_per_tooth[22] =   100;
        deg_per_tooth[23] =   100;
        deg_per_tooth[24] =   100;
        deg_per_tooth[25] =   100;
        deg_per_tooth[26] =   100;
        deg_per_tooth[27] =   100;
        deg_per_tooth[28] =   100;
        deg_per_tooth[29] =   100;
        deg_per_tooth[30] =   100;
        deg_per_tooth[31] =   300;
        //gap here
        deg_per_tooth[32] =   100;
        deg_per_tooth[33] =   100;
        deg_per_tooth[34] =   100;
        deg_per_tooth[35] =   100;
        deg_per_tooth[36] =   100;
        deg_per_tooth[37] =   100;
        deg_per_tooth[38] =   100;
        deg_per_tooth[39] =   100;
        deg_per_tooth[40] =   100;
        deg_per_tooth[41] =   100;
        deg_per_tooth[42] =   100;
        deg_per_tooth[43] =   100;
        deg_per_tooth[44] =   100;
        deg_per_tooth[45] =   100;
        deg_per_tooth[46] =   100;
        deg_per_tooth[47] =   300;
        //gap here
        deg_per_tooth[48] =   100;
        deg_per_tooth[49] =   100;
        deg_per_tooth[50] =   100;
        deg_per_tooth[51] =   100;
        deg_per_tooth[52] =   100;
        deg_per_tooth[53] =   100;
        deg_per_tooth[54] =   100;
        deg_per_tooth[55] =   100;
        deg_per_tooth[56] =   100;
        deg_per_tooth[57] =   100;
        deg_per_tooth[58] =   100;
        deg_per_tooth[59] =   100;
        deg_per_tooth[60] =   100;
        deg_per_tooth[61] =   100;
        deg_per_tooth[62] =   100;
        deg_per_tooth[63] =   300;
        //final gap here
        smallest_tooth_crk = 100;
        smallest_tooth_cam = 250;

    	if (num_cyl == 4) { // QR25DE
            no_triggers = 4;
            //unsure on the correct order
            //From gurov, A&B were transposed in W/S and +17deg offset required.
            // changed phase for RC2
            trigger_teeth[0] = 55;
            trigger_teeth[1] = 7;
            trigger_teeth[2] = 23;
            trigger_teeth[3] = 39;

            trig_angs[0] = -100 + tmp_offset;
            trig_angs[1] = -100 + tmp_offset; 
            trig_angs[2] = -100 + tmp_offset;
            trig_angs[3] = -100 + tmp_offset; 
    	} else if (num_cyl == 8) { // VK56
            no_triggers = 8;
            // from service manual info
            trigger_teeth[0] = 55;
            trigger_teeth[1] = 64;
            trigger_teeth[2] = 7;
            trigger_teeth[3] = 16;
            trigger_teeth[4] = 23;
            trigger_teeth[5] = 32;
            trigger_teeth[6] = 39;
            trigger_teeth[7] = 48;

            trig_angs[0] = -100 + tmp_offset;
            trig_angs[1] = -100 + tmp_offset; 
            trig_angs[2] = -100 + tmp_offset;
            trig_angs[3] = -100 + tmp_offset; 
            trig_angs[4] = -100 + tmp_offset;
            trig_angs[5] = -100 + tmp_offset; 
            trig_angs[6] = -100 + tmp_offset;
            trig_angs[7] = -100 + tmp_offset; 

    	} else { // invalid
           conf_err = 1;
    	}

        /* ----------------------  Honda RC-51 / FSC600 / RC46 --------------------*/
    } else if (spkmode == 29) {
        unsigned int i;
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
        no_teeth = 24; 
        last_tooth = no_teeth;
        for (i = 0 ; i < 24 ; i++) {
            deg_per_tooth[i] = 300;
        }

        smallest_tooth_crk = 300;
        smallest_tooth_cam = 600;

        if (num_cyl == 2) {
            no_triggers = 2;
            if (ram4.ICIgnOption & 0x8) {
                trigger_teeth[0] = 2; // Oddfire means RC51 
            } else {
                trigger_teeth[0] = 23; // Evenfire means FSC600 
            }
            trigger_teeth[1] = 11;
            trig_angs[0] = -350 + tmp_offset;
            trig_angs[1] = -350 + tmp_offset; 
        } else if (num_cyl == 4) { // RC46
            no_triggers = 4;
            if (!(ram4.ICIgnOption & 0x8)) {
                conf_err = 1;
            }
            trigger_teeth[0] = 11;
            trigger_teeth[1] = 17;
            trigger_teeth[2] = 2;
            trigger_teeth[3] = 8;
            trig_angs[0] = -595 + tmp_offset;
            trig_angs[1] = -595 + tmp_offset;
            trig_angs[2] = -595 + tmp_offset;
            trig_angs[3] = -595 + tmp_offset;
        } else {
            conf_err = 1;
        }

        /* ---------------------- Fiat 1.8 16V  -------------------- */
    } else if (spkmode == 30) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM | FLAGBYTE5_CAM_DOUBLE;
        no_teeth = 12;
        last_tooth = no_teeth;

        deg_per_tooth[0] = 320;
        deg_per_tooth[1] = 550;
        deg_per_tooth[2] = 930;
        deg_per_tooth[3] = 320;
        deg_per_tooth[4] = 550;
        deg_per_tooth[5] = 930;
        deg_per_tooth[6] = 320;
        deg_per_tooth[7] = 550;
        deg_per_tooth[8] = 930;
        deg_per_tooth[9] = 320;
        deg_per_tooth[10] = 550;
        deg_per_tooth[11] = 930;

        smallest_tooth_crk = 320;
        smallest_tooth_cam = 200;

        no_triggers = 4;
        trigger_teeth[0] = 1;
        trigger_teeth[1] = 4;
        trigger_teeth[2] = 7;
        trigger_teeth[3] = 10;
        trig_angs[0] = -875 + tmp_offset;
        trig_angs[1] = -875 + tmp_offset;
        trig_angs[2] = -875 + tmp_offset;
        trig_angs[3] = -875 + tmp_offset;

        /* ----------------------  360 tooth CASes ------------------------ */
    } else if ((spkmode >= 32) && (spkmode <=39)) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM | FLAGBYTE5_CRK_DOUBLE;
        flagbyte9 |= FLAGBYTE9_CAS360_ON;
        pin_xgcam = 0;
        smallest_tooth_crk = 0;
        smallest_tooth_cam = 0;
        xg_teeth = 360; // irrelevant
        fly_div = 3; // not used

        if (spkmode == 32) { //optispark
            if (num_cyl != 8) {
                conf_err = 1;
            } else {
                no_teeth = 8;
                last_tooth = no_teeth;
                no_triggers = no_teeth;
                deg_per_tooth[0] = 900;
                deg_per_tooth[1] = 900;
                deg_per_tooth[2] = 900;
                deg_per_tooth[3] = 900;
                deg_per_tooth[4] = 900;
                deg_per_tooth[5] = 900;
                deg_per_tooth[6] = 900;
                deg_per_tooth[7] = 900;

                trigger_teeth[0] = 8;
                trigger_teeth[1] = 1;
                trigger_teeth[2] = 2;
                trigger_teeth[3] = 3;
                trigger_teeth[4] = 4;
                trigger_teeth[5] = 5;
                trigger_teeth[6] = 6;
                trigger_teeth[7] = 7;

                trig_angs[0] = 40 + tmp_offset; // 4.0 BTDC
                trig_angs[1] = 40 + tmp_offset;
                trig_angs[2] = 40 + tmp_offset;
                trig_angs[3] = 40 + tmp_offset;
                trig_angs[4] = 40 + tmp_offset;
                trig_angs[5] = 40 + tmp_offset;
                trig_angs[6] = 40 + tmp_offset;
                trig_angs[7] = 40 + tmp_offset;
            }
        } else if (spkmode == 33) { // SR20 16,12,8,4 leading
            if (num_cyl != 4) {
                conf_err = 1;
            } else {
                no_teeth = 4;
                last_tooth = no_teeth;
                no_triggers = no_teeth;
                deg_per_tooth[0] = 1800;
                deg_per_tooth[1] = 1800;
                deg_per_tooth[2] = 1800;
                deg_per_tooth[3] = 1800;

                trigger_teeth[0] = 4;
                trigger_teeth[1] = 1;
                trigger_teeth[2] = 2;
                trigger_teeth[3] = 3;

                trig_angs[0] = -780 + tmp_offset; // 0 BTDC
                trig_angs[1] = -780 + tmp_offset;
                trig_angs[2] = -780 + tmp_offset;
                trig_angs[3] = -780 + tmp_offset;

            }
        } else if (spkmode == 34) { // RB25 24,20,16,12,8,4 leading
            if (num_cyl != 6) {
                conf_err = 1;
            } else {
                no_teeth = 6;
                last_tooth = no_teeth;
                no_triggers = no_teeth;
                deg_per_tooth[0] = 1200;
                deg_per_tooth[1] = 1200;
                deg_per_tooth[2] = 1200;
                deg_per_tooth[3] = 1200;
                deg_per_tooth[4] = 1200;
                deg_per_tooth[5] = 1200;

                trigger_teeth[0] = 6;
                trigger_teeth[1] = 1;
                trigger_teeth[2] = 2;
                trigger_teeth[3] = 3;
                trigger_teeth[4] = 4;
                trigger_teeth[5] = 5;

                trig_angs[0] = -170 + tmp_offset; // -17.0 BTDC
                trig_angs[1] = -170 + tmp_offset;
                trig_angs[2] = -170 + tmp_offset;
                trig_angs[3] = -170 + tmp_offset;
                trig_angs[4] = -170 + tmp_offset;
                trig_angs[5] = -170 + tmp_offset;
            }
        } else {
            conf_err = 1; // not yet handled
        }

        /* ---------------------- GM LS1  -------------------- */
    } else if (spkmode == 40) {
        unsigned int i;
        // wheel only uses the even falling edge for timing purposes.
        // uneven rising edge used for sync.
        smallest_tooth_crk = 30;

        trigger_teeth[0] = 23;
        trigger_teeth[1] = 5;
        trigger_teeth[2] = 11;
        trigger_teeth[3] = 17;
        trig_angs[0] = -150 + tmp_offset;
        trig_angs[1] = -150 + tmp_offset;
        trig_angs[2] = -150 + tmp_offset;
        trig_angs[3] = -150 + tmp_offset;
        flagbyte5 |= FLAGBYTE5_CRK_DOUBLE;

        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)) {      // COP or use-cam
            cycle_deg = 7200;
            flagbyte5 |= FLAGBYTE5_CAM | FLAGBYTE5_CAM_DOUBLE;
            no_teeth = 48;
            smallest_tooth_cam = 3600;
            no_triggers = 8;
            trigger_teeth[4] = 23;
            trigger_teeth[5] = 29;
            trigger_teeth[6] = 35;
            trigger_teeth[7] = 41;
            trigger_teeth[0] = 47;
            trig_angs[4] = -150 + tmp_offset;
            trig_angs[5] = -150 + tmp_offset;
            trig_angs[6] = -150 + tmp_offset;
            trig_angs[7] = -150 + tmp_offset;
        } else {
            cycle_deg = 3600;
            no_teeth = 24;
            smallest_tooth_cam = 0;
            no_triggers = 4;
        }
        for (i = 0 ; i < no_teeth ; i++) {
            deg_per_tooth[i] = 150;
        }
        last_tooth = no_teeth;
        if ((ram4.ICIgnOption & 0x8) || (num_cyl != 8)) {
            conf_err = 79; // even V8 required
        } 

        /* ---------------------- YZF1000  -------------------- */
    } else if (spkmode == 41) {
        // Has 7 real drill hole 'teeth' and a milled out recess
        // Due to the reverse polarity of the milled section we'll use that
        // for sync but not timing purposes.
        smallest_tooth_crk = 450;
        smallest_tooth_cam = 0;
        no_triggers = 2;

        no_teeth = 7;
        last_tooth = no_teeth;
        deg_per_tooth[0] =   450;
        deg_per_tooth[1] =   450;
        deg_per_tooth[2] =   450;
        deg_per_tooth[3] =   450;
        deg_per_tooth[4] =   450;
        deg_per_tooth[5] =   450;
        deg_per_tooth[6] =   900;

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 5;

        // no idea
        trig_angs[0] = -450 + tmp_offset;
        trig_angs[1] = -450 + tmp_offset;
        if ((ram4.ICIgnOption & 0x8) || (num_cyl != 4)) {
            conf_err = 80; // even 4cyl required
        }

        /* ----------------------  24-1-1 Honda Acura -------------------- */

    } else if (spkmode == 42) {
        deg_per_tooth[0] = 150;
        deg_per_tooth[1] = 150;
        deg_per_tooth[2] = 150;
        deg_per_tooth[3] = 150;
        deg_per_tooth[4] = 150;
        deg_per_tooth[5] = 150;
        deg_per_tooth[6] = 300; // missing tooth
        deg_per_tooth[7] = 150;
        deg_per_tooth[8] = 150;
        deg_per_tooth[9] = 150;
        deg_per_tooth[10] = 150;
        deg_per_tooth[11] = 150;
        deg_per_tooth[12] = 150;
        deg_per_tooth[13] = 150;
        deg_per_tooth[14] = 150;
        deg_per_tooth[15] = 150;
        deg_per_tooth[16] = 150;
        deg_per_tooth[17] = 150;
        deg_per_tooth[18] = 150;
        deg_per_tooth[19] = 150;
        deg_per_tooth[20] = 150;
        deg_per_tooth[21] = 300;

        smallest_tooth_crk = 150;

        if (((ram4.spk_mode3 & 0xc0) == 0x80) || (glob_sequential & SEQ_FULL)) {
            //if COP mode or use cam then double up pattern.
            cycle_deg = 7200;
            flagbyte5 |= FLAGBYTE5_CAM;
            no_teeth = 44;
            no_triggers = 6;
            smallest_tooth_cam = 180;
            deg_per_tooth[22] = 150;
            deg_per_tooth[23] = 150;
            deg_per_tooth[24] = 150;
            deg_per_tooth[25] = 150;
            deg_per_tooth[26] = 150;
            deg_per_tooth[27] = 150;
            deg_per_tooth[28] = 300; // missing tooth
            deg_per_tooth[29] = 150;
            deg_per_tooth[30] = 150;
            deg_per_tooth[31] = 150;
            deg_per_tooth[32] = 150;
            deg_per_tooth[33] = 150;
            deg_per_tooth[34] = 150;
            deg_per_tooth[35] = 150;
            deg_per_tooth[36] = 150;
            deg_per_tooth[37] = 150;
            deg_per_tooth[38] = 150;
            deg_per_tooth[39] = 150;
            deg_per_tooth[40] = 150;
            deg_per_tooth[41] = 150;
            deg_per_tooth[42] = 150;
            deg_per_tooth[43] = 300;

            trigger_teeth[0] = 32;
            trigger_teeth[1] = 40;
            trigger_teeth[2] = 3;

            trigger_teeth[3] = 10;
            trigger_teeth[4] = 18;
            trigger_teeth[5] = 25;

            trig_angs[0] = -150 + tmp_offset;
            trig_angs[1] = -150 + tmp_offset;
            trig_angs[2] = -150 + tmp_offset;
            trig_angs[3] = -150 + tmp_offset;
            trig_angs[4] = -150 + tmp_offset;
            trig_angs[5] = -150 + tmp_offset;

        } else {
            no_teeth = 22;
            no_triggers = 3;
            smallest_tooth_cam = 0;
            trigger_teeth[0] = 10;
            trigger_teeth[1] = 18;
            trigger_teeth[2] = 3;

            trig_angs[0] = -150 + tmp_offset;
            trig_angs[1] = -150 + tmp_offset;
            trig_angs[2] = -150 + tmp_offset;
        }
        last_tooth = no_teeth;
        if (num_cyl != 6) {
            conf_err = 1;
        }

/* ----------------------  Nissan vq35de -------------------- */
        /* code donated by Gennardy Gurov */
    } else if (spkmode == 43) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM; //using cam notches for identification
       if (num_cyl == 6) { // vq35
            no_teeth = 60;
            last_tooth = no_teeth;
            deg_per_tooth[0] =   100;
            deg_per_tooth[1] =   100;
            deg_per_tooth[2] =   100;
            deg_per_tooth[3] =   100;
            deg_per_tooth[4] =   100;
            deg_per_tooth[5] =   100;
            deg_per_tooth[6] =   100;
            deg_per_tooth[7] =   100;
            deg_per_tooth[8] =   100;
            deg_per_tooth[9] =   300;
            //gap here
            deg_per_tooth[10] =   100;
            deg_per_tooth[11] =   100;
            deg_per_tooth[12] =   100;
            deg_per_tooth[13] =   100;
            deg_per_tooth[14] =   100;
            deg_per_tooth[15] =   100;
            deg_per_tooth[16] =   100;
            deg_per_tooth[17] =   100;
            deg_per_tooth[18] =   100;
            deg_per_tooth[19] =   300;
            //gap here
            deg_per_tooth[20] =   100;
            deg_per_tooth[21] =   100;
            deg_per_tooth[22] =   100;
            deg_per_tooth[23] =   100;
            deg_per_tooth[24] =   100;
            deg_per_tooth[25] =   100;
            deg_per_tooth[26] =   100;
            deg_per_tooth[27] =   100;
            deg_per_tooth[28] =   100;
            deg_per_tooth[29] =   300;
            //gap here
            deg_per_tooth[30] =   100;
            deg_per_tooth[31] =   100;
            deg_per_tooth[32] =   100;
            deg_per_tooth[33] =   100;
            deg_per_tooth[34] =   100;
            deg_per_tooth[35] =   100;
            deg_per_tooth[36] =   100;
            deg_per_tooth[37] =   100;
            deg_per_tooth[38] =   100;
            deg_per_tooth[39] =   300;
            //gap here
            deg_per_tooth[40] =   100;
            deg_per_tooth[41] =   100;
            deg_per_tooth[42] =   100;
            deg_per_tooth[43] =   100;
            deg_per_tooth[44] =   100;
            deg_per_tooth[45] =   100;
            deg_per_tooth[46] =   100;
            deg_per_tooth[47] =   100;
            deg_per_tooth[48] =   100;
            deg_per_tooth[49] =   300;
            //gap here
            deg_per_tooth[50] =   100;
            deg_per_tooth[51] =   100;
            deg_per_tooth[52] =   100;
            deg_per_tooth[53] =   100;
            deg_per_tooth[54] =   100;
            deg_per_tooth[55] =   100;
            deg_per_tooth[56] =   100;
            deg_per_tooth[57] =   100;
            deg_per_tooth[58] =   100;
            deg_per_tooth[59] =   300;
       // final gap
            //final gap here
            smallest_tooth_crk = 100;
            smallest_tooth_cam = 250;

            no_triggers = 6;
            //unsure on the correct order
            trigger_teeth[0] = 60;
            trigger_teeth[1] = 10;
            trigger_teeth[2] = 20;
            trigger_teeth[3] = 30;
            trigger_teeth[4] = 40;
            trigger_teeth[5] = 50;

            trig_angs[0] = -100 + tmp_offset;
            trig_angs[1] = -100 + tmp_offset;
            trig_angs[2] = -100 + tmp_offset;
            trig_angs[3] = -100 + tmp_offset;
            trig_angs[4] = -100 + tmp_offset;
            trig_angs[5] = -100 + tmp_offset;
       } else { //only 6 cyl valid
           conf_err = 1;
       }

/* ----------------------  Jeep 2000 & 2002 -------------------- */
    } else if ((spkmode == 44) || (spkmode == 45)) {
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
       if (num_cyl == 6) {
            no_teeth = 24;
            last_tooth = no_teeth;

            deg_per_tooth[0] =   200;
            deg_per_tooth[1] =   200;
            deg_per_tooth[2] =   200;
            deg_per_tooth[3] =   600;

            deg_per_tooth[4] =   200;
            deg_per_tooth[5] =   200;
            deg_per_tooth[6] =   200;
            deg_per_tooth[7] =   600;

            deg_per_tooth[8] =   200;
            deg_per_tooth[9] =   200;
            deg_per_tooth[10] =  200;
            deg_per_tooth[11] =  600;

            deg_per_tooth[12] =  200;
            deg_per_tooth[13] =  200;
            deg_per_tooth[14] =  200;
            deg_per_tooth[15] =  600;

            deg_per_tooth[16] =  200;
            deg_per_tooth[17] =  200;
            deg_per_tooth[18] =  200;
            deg_per_tooth[19] =  600;

            deg_per_tooth[20] =  200;
            deg_per_tooth[21] =  200;
            deg_per_tooth[22] =  200;
            deg_per_tooth[23] =  600;

            no_triggers = 6;

            trigger_teeth[0] = 1;
            trigger_teeth[1] = 5;
            trigger_teeth[2] = 9;
            trigger_teeth[3] = 13;
            trigger_teeth[4] = 17;
            trigger_teeth[5] = 21;

            trig_angs[0] = -560 + tmp_offset;
            trig_angs[1] = -560 + tmp_offset;
            trig_angs[2] = -560 + tmp_offset;
            trig_angs[3] = -560 + tmp_offset;
            trig_angs[4] = -560 + tmp_offset;
            trig_angs[5] = -560 + tmp_offset;

            smallest_tooth_crk = 200;
            if (spkmode == 44) {
                smallest_tooth_cam = 1800;
            } else {
                smallest_tooth_cam = 200;
            }

        } else if (num_cyl == 4) {
            no_teeth = 16;
            last_tooth = no_teeth;

            deg_per_tooth[0] =   200;
            deg_per_tooth[1] =   200;
            deg_per_tooth[2] =   200;
            deg_per_tooth[3] =  1200;

            deg_per_tooth[4] =   200;
            deg_per_tooth[5] =   200;
            deg_per_tooth[6] =   200;
            deg_per_tooth[7] =  1200;

            deg_per_tooth[8] =   200;
            deg_per_tooth[9] =   200;
            deg_per_tooth[10] =  200;
            deg_per_tooth[11] = 1200;

            deg_per_tooth[12] =  200;
            deg_per_tooth[13] =  200;
            deg_per_tooth[14] =  200;
            deg_per_tooth[15] = 1200;

            no_triggers = 4;

            trigger_teeth[0] = 1;
            trigger_teeth[1] = 5;
            trigger_teeth[2] = 9;
            trigger_teeth[3] = 13;

            trig_angs[0] = -1160 + tmp_offset;
            trig_angs[1] = -1160 + tmp_offset;
            trig_angs[2] = -1160 + tmp_offset;
            trig_angs[3] = -1160 + tmp_offset;

            smallest_tooth_crk = 200;
            if (spkmode == 44) {
                smallest_tooth_cam = 1800;
            } else {
                smallest_tooth_cam = 200;
            }

        } else { // not valid
           conf_err = 1;
        }

        /* ----------------------  Zetec VCT --------------------*/
    } else if (spkmode == 46) {
        /* standard Ford 36-1 on the crank with a 4+1 on the cam */
        unsigned int i;
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
        no_teeth = 70; 
        last_tooth = no_teeth;
        for (i = 0 ; i < 70 ; i++) {
            deg_per_tooth[i] = 100;
        }
        deg_per_tooth[34] = 200;
        deg_per_tooth[69] = 200;

        smallest_tooth_crk = 100;
        smallest_tooth_cam = 900;

        if (num_cyl == 4) {
            no_triggers = 4;
            trigger_teeth[0] = 10;
            trigger_teeth[1] = 28;
            trigger_teeth[2] = 45;
            trigger_teeth[3] = 63;

        } else if (num_cyl == 6) {
            /* these trigger teeth are guesses - awaiting data */
            no_triggers = 6;
            trigger_teeth[0] = 7;
            trigger_teeth[1] = 22;
            trigger_teeth[2] = 34;
            trigger_teeth[3] = 45;
            trigger_teeth[4] = 57;
            trigger_teeth[5] = 69;

            trig_angs[0] = -100 + tmp_offset;
            trig_angs[1] = -100 + tmp_offset;
            trig_angs[2] = -100 + tmp_offset;
            trig_angs[3] = -100 + tmp_offset;
            trig_angs[4] = -100 + tmp_offset;
            trig_angs[5] = -100 + tmp_offset;
        } else if (num_cyl == 8) {
            no_triggers = 8;
            trigger_teeth[0] = 11;
            trigger_teeth[1] = 20;
            trigger_teeth[2] = 29;
            trigger_teeth[3] = 37;
            trigger_teeth[4] = 46;
            trigger_teeth[5] = 55;
            trigger_teeth[6] = 64;
            trigger_teeth[7] = 2;

            trig_angs[0] = -100 + tmp_offset;
            trig_angs[1] = -100 + tmp_offset;
            trig_angs[2] = -100 + tmp_offset;
            trig_angs[3] = -100 + tmp_offset;
            trig_angs[4] = -100 + tmp_offset;
            trig_angs[5] = -100 + tmp_offset;
            trig_angs[6] = -100 + tmp_offset;
            trig_angs[7] = -100 + tmp_offset;
        } else {
           conf_err = 1;
        }

        /* ----------------------  Flywheel tri-tach (Audi etc.) --------------------*/
    } else if (spkmode == 47) {
        cycle_deg = 7200;
        /* the special XGATE code converts the flywheel teeth into a lower number of teeth */
        int x, degpt = 0, teethpt = 0, t = 0, a;
        flagbyte5 |= FLAGBYTE5_CAM;
        flagbyte9 |= FLAGBYTE9_CAS360_ON;
        xg_teeth = ram4.No_Teeth;
        if ((ram4.No_Teeth == 135) && (num_cyl == 5)) {
            no_teeth = 90;
            no_triggers = 5;
            teethpt = 18;
            degpt = 80;
            fly_div = 3;
        } else if ((ram4.No_Teeth == 135) && (num_cyl == 6)) {
            no_teeth = 90;
            no_triggers = 6;
            teethpt = 15;
            degpt = 80;
            fly_div = 3;
        } else if ((ram4.No_Teeth == 136) && (num_cyl == 8)) {
            no_teeth = 136;
            no_triggers = 8;
            teethpt = 17;
            degpt = 53; // actually 5.294 deg/tooth
            fly_div = 2;
        } else if ((ram4.No_Teeth == 132) && (num_cyl == 4)) {
            no_teeth = 88;
            no_triggers = 4;
            teethpt = 22;
            degpt = 82; // actually 8.18
            fly_div = 3;
        } else if ((ram4.No_Teeth == 130) && (num_cyl == 4)) {
            no_teeth = 52;
            no_triggers = 4;
            teethpt = 13;
            degpt = 138; // actually 13.846
            fly_div = 5;
        } else if ((ram4.No_Teeth == 129) && (num_cyl == 6)) {
            no_teeth = 129;
            no_triggers = 6;
            teethpt = 43;
            degpt = 56; // actually 5.58
            fly_div = 2;
        } else {
           conf_err = 1;
        }

        /* Rules for above (for 720 deg cycle)
            teeth_per_cycle = ram4.No_Teeth * 2
            no_triggers = ram4.no_cyl
            no_teeth = teeth_per_cycle / fly_div
            teethpt = teeth_per_cycle / no_triggers
            degpt = 7200 / no_teeth
            fly_div set to give integer no_teeth

            Note that these are all integers and are exposed to ensure the programmer
            has verified them.
        */


        /* sanity check */
        if (no_teeth > MAXNUMTEETH) {
            conf_err = 141;
            return;
        }

        if (ram4.spk_mode3 & 2) {
            pin_xgcam = 0x20; //PT5
        } else {
            pin_xgcam = 0x04; //PT2
        }

        if (degpt) {
            last_tooth = no_teeth;
            for (x = 0 ; x < no_teeth ; x++) {
                deg_per_tooth[x] = degpt;
            }
        /* want to roll back to 10degATDC or more. Aim for 1 pseudo-tooth beyond */
        t = (ram4.Miss_ang + 100 + degpt) / degpt;
        a = (ram4.Miss_ang - (t * degpt)) + tmp_offset;

            for (x = 0 ; x < no_triggers ; x++) {
                trigger_teeth[x] = t;
                trig_angs[x] = a;
                t += teethpt;
            }
        }
        smallest_tooth_crk = 0;
        smallest_tooth_cam = 0;

        /* ----------------------  2JZ VVTi --------------------*/
    } else if (spkmode == 48) {
        /* 4cyl: 36-2 on the crank with a 4-1 on the cam */
        /* 6cyl: 36-2 on the crank with a 3 on the cam at 120deg spacing*/
        /* 8cyl: 36-2 on the crank */
        unsigned int i;
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
        no_teeth = 68; 
        last_tooth = no_teeth;
        for (i = 0 ; i < 68 ; i++) {
            deg_per_tooth[i] = 100;
        }
        deg_per_tooth[33] = 300;
        deg_per_tooth[67] = 300;

        smallest_tooth_crk = 100;
        smallest_tooth_cam = 900;

        if (num_cyl == 4) {
            /* Total guessed for 2zz-ge, unknown, untested */
            no_triggers = 4;
            trigger_teeth[0] = 57;
            trigger_teeth[1] = 5;
            trigger_teeth[2] = 23;
            trigger_teeth[3] = 39;

            trig_angs[0] = -50 + tmp_offset;
            trig_angs[1] = -50 + tmp_offset;
            trig_angs[2] = -50 + tmp_offset;
            trig_angs[3] = -50 + tmp_offset;
        } else if (num_cyl == 6) {
            no_triggers = 6;
            trigger_teeth[0] = 18;
            trigger_teeth[1] = 30;
            trigger_teeth[2] = 40;
            trigger_teeth[3] = 52;
            trigger_teeth[4] = 64;
            trigger_teeth[5] = 6;

            trig_angs[0] = -150 + tmp_offset;
            trig_angs[1] = -150 + tmp_offset;
            trig_angs[2] = -150 + tmp_offset;
            trig_angs[3] = -150 + tmp_offset;
            trig_angs[4] = -150 + tmp_offset;
            trig_angs[5] = -150 + tmp_offset;
        } else if (num_cyl == 8) {
            /* Total guessed for 1UZ, unknown, untested */
            no_triggers = 8;
            trigger_teeth[0] = 19;
            trigger_teeth[1] = 28;
            trigger_teeth[2] = 37;
            trigger_teeth[3] = 46;
            trigger_teeth[4] = 55;
            trigger_teeth[5] = 64;
            trigger_teeth[6] = 1;
            trigger_teeth[7] = 10;

            trig_angs[0] = -50 + tmp_offset;
            trig_angs[1] = -50 + tmp_offset;
            trig_angs[2] = -50 + tmp_offset;
            trig_angs[3] = -50 + tmp_offset;
            trig_angs[4] = -50 + tmp_offset;
            trig_angs[5] = -50 + tmp_offset;
            trig_angs[6] = -50 + tmp_offset;
            trig_angs[7] = -50 + tmp_offset;
        } else if (num_cyl == 12) {
            /* V12 */
            no_triggers = 12;
            trigger_teeth[0] = 19;
            trigger_teeth[1] = 25;
            trigger_teeth[2] = 31;
            trigger_teeth[3] = 35;
            trigger_teeth[4] = 41;
            trigger_teeth[5] = 47;

            trigger_teeth[6] = 53;
            trigger_teeth[7] = 59;
            trigger_teeth[8] = 65;
            trigger_teeth[9] = 1;
            trigger_teeth[10] = 7;
            trigger_teeth[11] = 13;

            trig_angs[0] = -250 + tmp_offset;
            trig_angs[1] = -250 + tmp_offset;
            trig_angs[2] = -250 + tmp_offset;
            trig_angs[3] = -250 + tmp_offset;
            trig_angs[4] = -250 + tmp_offset;
            trig_angs[5] = -250 + tmp_offset;
            trig_angs[6] = -250 + tmp_offset;
            trig_angs[7] = -250 + tmp_offset;
            trig_angs[8] = -250 + tmp_offset;
            trig_angs[9] = -250 + tmp_offset;
            trig_angs[10] = -250 + tmp_offset;
            trig_angs[11] = -250 + tmp_offset;
        } else {
           conf_err = 1;
        }

        /* ----------------------  Honda TSX / D17 --------------------*/
        /* also K24A2 */
    } else if ((spkmode == 49) || (spkmode == 53)) {
        /* 12+1 on the crank and 4+1 on the cam */
        unsigned int i;
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
        no_triggers = 4;
        no_teeth = 26; 
        last_tooth = no_teeth;
        for (i = 0 ; i < 26 ; i++) {
            deg_per_tooth[i] = 300;
        }
        deg_per_tooth[0] = 100;
        deg_per_tooth[1] = 200;
        deg_per_tooth[13] = 100;
        deg_per_tooth[14] = 200;

        smallest_tooth_crk = 100;
        smallest_tooth_cam = 450;

        trigger_teeth[0] = 1;
        trigger_teeth[1] = 8;
        trigger_teeth[2] = 14;
        trigger_teeth[3] = 21;

        if (spkmode == 49) {
            trig_angs[0] = -350 + tmp_offset;
            trig_angs[1] = -350 + tmp_offset;
            trig_angs[2] = -350 + tmp_offset;
            trig_angs[3] = -350 + tmp_offset;
        } else if (spkmode == 53) { // K24A2, angles from 'Barton'
            trig_angs[0] = 50 + tmp_offset;
            trig_angs[1] = 50 + tmp_offset;
            trig_angs[2] = 50 + tmp_offset;
            trig_angs[3] = 50 + tmp_offset;
        }

        if (ram4.no_cyl != 4) {
           conf_err = 1;
        }

        /* ----------------------  36-2-2-2 Mazda6 VVT------------------------ */

//    } else if (spkmode == 50) {

    // see mode 7 36-2-2-2

        /* ----------------- Viper V10 ------------------------ */

    } else if ((spkmode == 51) || (spkmode == 52)) {
        /* Five groups of two slots on crank. Rising/Falling cam. */
        cycle_deg = 7200;
        if (spkmode == 51) {
            flagbyte5 |= FLAGBYTE5_CAM | FLAGBYTE5_CAM_DOUBLE; // Gen 2
            smallest_tooth_cam = 3600;
        } else {
            flagbyte5 |= FLAGBYTE5_CAM; // Gen 1
            smallest_tooth_cam = 180; // Small tooth gap looks ~same as crank gap.
        }
        no_triggers = 10;
        no_teeth = 20; 
        last_tooth = no_teeth;
        deg_per_tooth[0] = 180;
        deg_per_tooth[1] = 540;
        deg_per_tooth[2] = 180;
        deg_per_tooth[3] = 540;
        deg_per_tooth[4] = 180;
        deg_per_tooth[5] = 540;
        deg_per_tooth[6] = 180;
        deg_per_tooth[7] = 540;
        deg_per_tooth[8] = 180;
        deg_per_tooth[9] = 540;
        deg_per_tooth[10] = 180;
        deg_per_tooth[11] = 540;
        deg_per_tooth[12] = 180;
        deg_per_tooth[13] = 540;
        deg_per_tooth[14] = 180;
        deg_per_tooth[15] = 540;
        deg_per_tooth[16] = 180;
        deg_per_tooth[17] = 540;
        deg_per_tooth[18] = 180;
        deg_per_tooth[19] = 540;

        smallest_tooth_crk = 60;

        trigger_teeth[0] = 2;
        trigger_teeth[1] = 3;
        trigger_teeth[2] = 6;
        trigger_teeth[3] = 7;
        trigger_teeth[4] = 10;
        trigger_teeth[5] = 11;
        trigger_teeth[6] = 14;
        trigger_teeth[7] = 15;
        trigger_teeth[8] = 18;
        trigger_teeth[9] = 19;

        trig_angs[0] =  90 + tmp_offset;
        trig_angs[1] =  90 + tmp_offset;
        trig_angs[2] =  90 + tmp_offset;
        trig_angs[3] =  90 + tmp_offset;
        trig_angs[4] =  90 + tmp_offset;
        trig_angs[5] =  90 + tmp_offset;
        trig_angs[6] =  90 + tmp_offset;
        trig_angs[7] =  90 + tmp_offset;
        trig_angs[8] =  90 + tmp_offset;
        trig_angs[9] =  90 + tmp_offset;

        if (ram4.no_cyl != 10) {
           conf_err = 1;
        }
        /* ----------------------  Honda K24A2 ------------------------ */

//    } else if (spkmode == 53) {

    // see mode 49

        /* ---------------------- HD 32-2 with polled cam or MAP sensing  -------------------- */
    } else if (spkmode == 54) {
        int t;
        cycle_deg = 7200;
        no_teeth = 60;
        last_tooth = no_teeth;
        for (t = 0 ; t < 60 ; t++) {
            deg_per_tooth[t] = 112 + (t & 1); // actually 11.25 degrees per tooth
        }
        deg_per_tooth[29] =  338;
        deg_per_tooth[59] =  338;

        smallest_tooth_crk = 112;

        if ((ram4.hardware & 0xc0) == 0x80) {
            /* use MAP sensor for sync */
            no_triggers = 4;
/* A = front */
            trigger_teeth[0] = 11;
            trigger_teeth[1] = 37;
            trigger_teeth[2] = 41;
            trigger_teeth[3] = 7;

            trig_angs[0] = -115 + tmp_offset;
            trig_angs[1] = -115 + tmp_offset; // only fires until phase determined
            trig_angs[2] = -115 + tmp_offset; // only fires until phase determined
            trig_angs[3] = -115 + tmp_offset;
            if ((!(ram4.ICIgnOption & 0x8)) || ((ram4.no_cyl & 0x1f) != 2) || ((ram4.spk_mode3 & 0xe0) != 0x60)) {
                conf_err = 169;
            }
        } else {
            /* use polled CAM */
            /* need to confirm these settings against known working MSQ */
            no_triggers = 2;
/* A = front */
            trigger_teeth[0] = 11;
            trigger_teeth[1] = 37;

            trig_angs[0] = -115 + tmp_offset;
            trig_angs[1] = -115 + tmp_offset;
            if ((!(ram4.ICIgnOption & 0x8)) || ((ram4.no_cyl & 0x1f) != 2)) {
                conf_err = 1;
            }
        }

        /* ----------------------  Miata 36-2 99-05 --------------------*/
    } else if (spkmode == 55) {
        /* 36-2 on the crank with a 2/1 on the cam */
        unsigned int i;
        cycle_deg = 7200;
        flagbyte5 |= FLAGBYTE5_CAM;
        no_teeth = 68;
        last_tooth = no_teeth;
        for (i = 0 ; i < 68 ; i++) {
            deg_per_tooth[i] = 100;
        }
        deg_per_tooth[33] = 300;
        deg_per_tooth[67] = 300;

        smallest_tooth_crk = 100;
        smallest_tooth_cam = 900;

        no_triggers = 4;
        trigger_teeth[0] = 19;
        trigger_teeth[1] = 35;
        trigger_teeth[2] = 53;
        trigger_teeth[3] = 1;

        trig_angs[0] = -300 + tmp_offset;
        trig_angs[1] = -300 + tmp_offset;
        trig_angs[2] = -300 + tmp_offset;
        trig_angs[3] = -300 + tmp_offset;

        if (num_cyl != 4) {
           conf_err = 1;
        }

        /* ----------------------  Daihatsu 12+1 (3 cyl) --------------------*/
    } else if (spkmode == 56) {
        unsigned int i;
        cycle_deg = 7200;
        RPAGE = 0xfb; /* HARDCODED page 24 */
        if (ram_window.pg24.vvt_opt1 & 0x03) {
            flagbyte5 |= FLAGBYTE5_CAM; /* With VVT */
        }
        no_teeth = 13;
        last_tooth = no_teeth;
        for (i = 0 ; i < 13 ; i++) {
            deg_per_tooth[i] = 600;
        }
        deg_per_tooth[11] = 200;
        deg_per_tooth[12] = 400;

        smallest_tooth_crk = 200;
        smallest_tooth_cam = 0;

        no_triggers = 3;

        trigger_teeth[0] = 2;
        trigger_teeth[1] = 6;
        trigger_teeth[2] = 10;

        trig_angs[0] = -550 + tmp_offset;
        trig_angs[1] = -550 + tmp_offset;
        trig_angs[2] = -550 + tmp_offset;

        if (num_cyl != 3) {
           conf_err = 1;
        }

        /* ------------------------------------------ */

    } else {                    // dizzy or wheel
        unsigned char err_count;
        unsigned char trig_tooth, trig_missing, miss_teeth;
        unsigned char base_teeth_per_trig = 1, base_trig_tooth, teeth_per_trig[8], odd_trig_teeth[8];
        signed int odd_trig_ang[8], base_trig_ang, oddf_ang;

        mid_last_tooth = 0;

        /* ----------------------  DIZZY MODE or Fuel only ------------------------ */
        if (((spkmode & 0xfe) == 2) || (spkmode == 31)) {  // dizzy (options 2,3) + fuel only
            teeth_per_trig[0] = 1;
            teeth_per_trig[1] = 1;
            teeth_per_trig[2] = 1;
            teeth_per_trig[3] = 1;
            odd_trig_ang[0] = 0;
            odd_trig_ang[1] = 0;
            odd_trig_ang[2] = 0;
            odd_trig_ang[3] = 0;
            if (num_cyl & 1) {
                no_triggers = num_cyl;  // (for 3, 5 cyl)
            } else if ((spkmode == 2) && ((ram4.spk_conf2 & 0x07) == 0x04)) {
                no_triggers = num_cyl;  // sig-PIP TFI
                cycle_deg = 7200;
                flagbyte5 |= FLAGBYTE5_CRK_DOUBLE;
            } else {
                no_triggers = num_cyl >> 1;     // even no. cylinders
            }

            trig_ang = ram4.adv_offset;
            if ((spkmode == 3) && (trig_ang < 200)) {
                conf_err = 9; // next-cyl trigret not allowed
            }

        /*********** oddfire dizzy + fuel  ********/
            if (ram4.ICIgnOption & 0x8) {
                no_triggers = num_cyl;  // oddfire needs to trigger on each one
                no_teeth = no_triggers;
                last_tooth = no_teeth;
                tmp_deg_per_tooth = (((unsigned int) 14400 / (char) num_cyl));  // degx10 between tooth 1 & 3
                for (j = 1; j <= no_teeth; j++) {
                    unsigned int jtmp1;
                    if ((j & 1) == 0) {        // these might be transposed
                        jtmp1 = ram4.OddFireang;        // short gap
                    } else {
                        jtmp1 = tmp_deg_per_tooth - ram4.OddFireang;    // long gap
                    }
                    deg_per_tooth[j - 1] = jtmp1;
                }

                if (ram4.OddFireang < (tmp_deg_per_tooth - ram4.OddFireang)) {
                    smallest_tooth_crk = ram4.OddFireang;
                } else {
                    smallest_tooth_crk = tmp_deg_per_tooth - ram4.OddFireang;
                }
                smallest_tooth_cam = 0;

        /*********** even dizzy (incl trigger return) ***********/
            } else {
                if ((ram4.no_cyl & 0x1f) == 1) {
                    no_triggers = 2;
                    if (((ram4.EngStroke & 1) == 0) && ((ram4.spk_mode3 & 0xc0) == 0x80)) {
                        tmp_deg_per_tooth = 7200; // 1 cyl 4 stroke COP (unusual)
                    } else {
                        tmp_deg_per_tooth = 3600; // normal 1 cyl arrangement
                        if (ram4.EngStroke & 1) { // 2-stroke
                            no_triggers = 1;
                        }
                    }
                } else if ((ram4.no_cyl & 0x1f) == 2) {
                    no_triggers = 2;
                    tmp_deg_per_tooth = 3600; // normal 2 cyl arrangement in this mode
                } else { // normal calc
                    tmp_deg_per_tooth =
                    (((unsigned int) 7200 / (char) num_cyl));
                }
                no_teeth = no_triggers;
                last_tooth = no_triggers;
                for (j = 1; j <= no_triggers; j++) {
                    deg_per_tooth[j - 1] = tmp_deg_per_tooth;
                }
            }

            smallest_tooth_crk = tmp_deg_per_tooth;
            smallest_tooth_cam = 0;
            if (spkmode == 3) {
                flagbyte5 |= FLAGBYTE5_CRK_DOUBLE;
            }


            /* ----------------------  EVEN WHEEL MODE ------------------------ */
        } else {                // real wheel mode
            if (ram4.No_Teeth < 2) {
                /* That's not going to work */
                conf_err = 1;
                return;
            }

            if ( /*((ram4.spk_mode3 & 0xc0) == 0x80)        // COP selected  breaks 2 stroke COP
                || */((ram4.spk_config & 0x0c) == 0x0c)   //  or 2nd trig+missing
                || (((ram4.spk_config & 0x0c) == 0x04) && (ram4.spk_config & 0x02))     //  or cam speed missing tooth wheel
                || (((ram4.spk_config & 0x0c) == 0x08) && ((ram4.spk_config & 0xc0) == 0x00))  //  or non missing dual wheel with cam speed 2nd trig
                ) {
                no_triggers = num_cyl;  // same triggers as COP i.e. cover 720 crank degrees
                cycle_deg = 7200;
            } else { // triggers cover 360 crank degrees
                if (ram4.EngStroke & 0x01) {
                    no_triggers = num_cyl;     // For 2-stroke or rotary = no. cyl/rotors
                } else {
                    no_triggers = num_cyl >> 1; // for 4-stroke it is half
                }
            }
            // we also need to calc the no. teeth we see - do this a little lower

            if (((ram4.no_cyl & 0x1f) == 2) && (ram4.EngStroke & 1) && ((ram4.spk_mode3 & 0xc0) == 0x80)) { // if 2 cyl 2 stroke COP
                no_triggers = 2; // likely not needed due to above changes
            } else {
                // check COP validity
                if (((ram4.spk_mode3 & 0xc0) == 0x80) && (!(ram4.EngStroke & 1))) {       // if COP and 4-stroke then need 720 deg of info
                    if (((ram4.spk_config & 0xc) == 0x8)
                        && ((ram4.spk_config & 0xc0) == 0x40)) {
                        conf_err = 32;      // dual wheel and COP not allowed with only crank speed 2nd trigger (no phase info)
                    } else if (((ram4.spk_config & 0xc) == 0x4)
                               && ((ram4.spk_config & 2) == 0)) {
                        conf_err = 38;      // single wheel and COP not allowed at crank speed (no phase info)
                    }
                }
            }

            // using cam ?
            if ((ram4.spk_config & 0x08) == 0x08) {     // dual wheel or 2nd trig+missing
                flagbyte5 |= FLAGBYTE5_CAM;
                // rising+falling option removed

                /* Use MAP sensor in lieu of cam sensor ? */
                if ((ram4.hardware & 0xc0) == 0x80) {
                    if ((ram4.no_cyl > 2) || (!((ram4.spk_config & 0x0c) == 0x0c))
                        || (!((ram4.spk_mode3 & 0xe0) == 0x60))
                        || (ram4.mapsample_opt & 0x04)) {
                        /* only valid for 1,2 cyl with dual+missing and WCOP and windowed MAP */
                        conf_err = 152;
                    } else {
                        int mt;
                        flagbyte5 &= ~FLAGBYTE5_CAM; /* not actually using hardware input */
                        /* calculate ADC threshold */
                        mt = ram5.map_phase_thresh - ram4.map0;
                        if (mt < 0) {
                            mt = 0;
                        }
                        mapadc_thresh = (mt * 1023UL) / (ram4.mapmax - ram4.map0);
                    }
                }
            }

            // Do selective division and check remainder for config error

            if (ram4.No_Teeth == 32) {
                // special case
                tmp_deg_per_tooth = 1125; // centi-degrees
                trig_ang = 10 * (ram4.Miss_ang + tmp_offset);
            } else {
            /* Now figure out how many degrees there are per tooth */
            /* only for even wheels, for uneven use special mode*/

        	    if (ram4.spk_config & 0x02) { // cam or crank speed
            		tmp_deg_per_tooth = (unsigned int)(((unsigned int)7200 / (unsigned char)ram4.No_Teeth));
                    if (7200 % ram4.No_Teeth) {
                        conf_err = 1;
                        return;
                    }
        	    } else {
            		tmp_deg_per_tooth = (unsigned int)(((unsigned int)3600 / (unsigned char)ram4.No_Teeth));
                    if (3600 % ram4.No_Teeth) {
                        conf_err = 1;
                        return;
                    }
        	    }
                trig_ang = ram4.Miss_ang + tmp_offset;
            }

            if ((ram4.spk_config & 0xc) == 0x8) {       // dual wheel
                if (ram4.spk_config & 0x02) {   // cam speed
                    if ((ram4.spk_config & 0xc0) == 0x40) {
                        no_teeth = ram4.No_Teeth >> 1;  //  2nd trig every crank rev
                    } else if ((ram4.spk_config & 0xc0) == 0x80) {
                        no_teeth = ram4.No_Teeth / num_cyl;  //  2nd trig every ignition event
                    } else {
                        no_teeth = ram4.No_Teeth;       // 2nd trig every cam rev
                    }
                } else {        // crank speed
                    if ((ram4.spk_config & 0xc0) == 0x40) {
                        no_teeth = ram4.No_Teeth;       //  2nd trig every crank rev
                    } else if ((ram4.spk_config & 0xc0) == 0x80) {
                        no_teeth = ram4.No_Teeth / (num_cyl >> 1);  //  2nd trig every ignition event
                    } else {
                        no_teeth = ram4.No_Teeth << 1;  // 2nd trig every cam rev
                    }
                }
                miss_teeth = 0;
            } else {
                if ((ram4.spk_config & 0xc) == 0xc) {
                    /* missing + extra */
                    no_teeth = ram4.No_Teeth << 1;
                } else {
                    no_teeth = ram4.No_Teeth;
                }
                miss_teeth = ram4.No_Miss_Teeth;
                if (((int)ram4.No_Teeth - (int)ram4.No_Miss_Teeth) < 2) {
                    /* That's not going to work */
                    conf_err = 1;
                    return;
                }
            }

            if ((no_teeth > MAXNUMTEETH) || (ram4.No_Teeth > MAXNUMTEETH)) {
                conf_err = 141;
                return;
            }

            base_teeth_per_trig = no_teeth / no_triggers;
            if ((no_teeth % no_triggers) && (num_cyl != 7)) { /* special case for 7 cyl in a bit */
                conf_err = 1;
            }
            // store degrees per tooth
            if ((ram4.spk_config & 0x0c) == 0x08) {
                last_tooth = no_teeth;  // if 2nd trig only then ignore missing teeth field

                /* Fill in array of tooth sizes for even wheels */
                for (j = 0; j < no_teeth; j++) {
                    deg_per_tooth[j] = tmp_deg_per_tooth;
                }
            } else {
                mid_last_tooth = ram4.No_Teeth - ram4.No_Miss_Teeth;
                last_tooth = no_teeth - ram4.No_Miss_Teeth;

                /* Fill in array of tooth sizes for even wheels */
                for (j = 0; j < mid_last_tooth; j++) {
                    deg_per_tooth[j] = tmp_deg_per_tooth;
                }
                deg_per_tooth[mid_last_tooth - 1] =
                    tmp_deg_per_tooth * (1 + ram4.No_Miss_Teeth);
                deg_per_tooth[mid_last_tooth] = 0;      // tooth doesn't exist

                if (no_teeth != ram4.No_Teeth) {
                    // must be 2nd + missing, go around again
                    for (j = ram4.No_Teeth; j < last_tooth; j++) {
                        deg_per_tooth[j] = tmp_deg_per_tooth;
                    }
                    deg_per_tooth[last_tooth - 1] =
                        tmp_deg_per_tooth * (1 + ram4.No_Miss_Teeth);
                }
            }
        }

        /* ------------------ common to dizzy and wheel ------------------- */

        // Now setup trigger teeth
        /* figure out trigger angle... get as close to -10 as possible.
         * calcs will count back from this tooth to get spark/dwell
         */


        trig_tooth = 1;
        while (trig_ang > -100) {       // 10ATDC
            trig_ang -= deg_per_tooth[trig_tooth - 1];
            trig_tooth++;
            if (trig_tooth > last_tooth) {
                trig_tooth = 1;
            } else if (mid_last_tooth && (trig_tooth == (mid_last_tooth + 1))) {
                trig_tooth += ram4.No_Miss_Teeth;
            }
        }

        if (ram4.ICIgnOption & 0x8) {   // oddfire
            if ((spkmode == 2) || (spkmode == 31)) { // oddfire distributor or fuel only
                teeth_per_trig[0] = 1;
                odd_trig_ang[0] = 0;
                teeth_per_trig[1] = 1;
                odd_trig_ang[1] = ram4.OddFireang;
                teeth_per_trig[2] = 1;
                odd_trig_ang[2] = 0;
                teeth_per_trig[3] = 1;
                odd_trig_ang[3] = ram4.OddFireang;
            } else {
                unsigned short odd_trig_offset;
                unsigned char odd_trig_tooth_offset = 0;

                if (ram4.No_Teeth == 32) {
                    // special case
                    oddf_ang = ram4.OddFireang * 10;
                } else {
                    oddf_ang = ram4.OddFireang;
                }

                odd_trig_tooth_offset = oddf_ang / tmp_deg_per_tooth;
                odd_trig_offset = oddf_ang % tmp_deg_per_tooth;

                if ((ram4.spk_conf2 & 0x18) == 0x10) { // oddfire paired (like vmax)
                    odd_trig_ang[0] = 0;
                    teeth_per_trig[0] = base_teeth_per_trig;
                    odd_trig_ang[1] = 0;
                    teeth_per_trig[1] = odd_trig_tooth_offset;
                    odd_trig_ang[1] = 0;
                    teeth_per_trig[2] = base_teeth_per_trig;
                    odd_trig_ang[2] = odd_trig_offset;
                    teeth_per_trig[3] = (base_teeth_per_trig << 1) - odd_trig_tooth_offset;
                    odd_trig_ang[3] = odd_trig_offset;

                } else if ((ram4.spk_conf2 & 0x18) == 0x00) { // alternating (more normal)
                    teeth_per_trig[0] = odd_trig_tooth_offset;
                    odd_trig_ang[0] = 0;
                    teeth_per_trig[1] = (base_teeth_per_trig << 1) - odd_trig_tooth_offset;
                    odd_trig_ang[1] = odd_trig_offset;
                    teeth_per_trig[2] = teeth_per_trig[0];
                    odd_trig_ang[2] = odd_trig_ang[0];
                    teeth_per_trig[3] = teeth_per_trig[1];
                    odd_trig_ang[3] = odd_trig_ang[1];

                } else { // custom user supplied angles
                    int oddfang_mult;

                    if ((ram5.oddfireangs[0] + ram5.oddfireangs[1]
                        + ram5.oddfireangs[2] + ram5.oddfireangs[3]) != 7200) {
                        conf_err = 149;
                    }

                    if (ram4.No_Teeth == 32) {
                        oddfang_mult = 10;
                    } else {
                        oddfang_mult = 1;
                    }

                    oddf_ang = ram5.oddfireangs[0] * oddfang_mult;
                    odd_trig_tooth_offset = oddf_ang / tmp_deg_per_tooth;
                    odd_trig_offset = oddf_ang % tmp_deg_per_tooth;

                    teeth_per_trig[0] = odd_trig_tooth_offset;
                    odd_trig_ang[1] = odd_trig_offset;

                    oddf_ang = ram5.oddfireangs[1] * oddfang_mult;
                    odd_trig_tooth_offset = oddf_ang / tmp_deg_per_tooth;
                    odd_trig_offset += oddf_ang % tmp_deg_per_tooth;
                    if (odd_trig_offset >= tmp_deg_per_tooth) {
                        odd_trig_tooth_offset++;
                        odd_trig_offset -= tmp_deg_per_tooth;
                    }

                    teeth_per_trig[1] = odd_trig_tooth_offset;
                    odd_trig_ang[2] = odd_trig_offset;

                    oddf_ang = ram5.oddfireangs[2] * oddfang_mult;
                    odd_trig_tooth_offset = oddf_ang / tmp_deg_per_tooth;
                    odd_trig_offset += oddf_ang % tmp_deg_per_tooth;
                    if (odd_trig_offset >= tmp_deg_per_tooth) {
                        odd_trig_tooth_offset++;
                        odd_trig_offset -= tmp_deg_per_tooth;
                    }

                    teeth_per_trig[2] = odd_trig_tooth_offset;
                    odd_trig_ang[3] = odd_trig_offset;

                    oddf_ang = ram5.oddfireangs[3] * oddfang_mult;
                    odd_trig_tooth_offset = oddf_ang / tmp_deg_per_tooth;
                    odd_trig_offset += oddf_ang % tmp_deg_per_tooth;
                    if (odd_trig_offset >= tmp_deg_per_tooth) {
                        odd_trig_tooth_offset++;
                        odd_trig_offset -= tmp_deg_per_tooth;
                    }

                    teeth_per_trig[3] = odd_trig_tooth_offset;
                    odd_trig_ang[0] = 0; // by definition;
                }
            }
        } else { // wholly even
            teeth_per_trig[0] = base_teeth_per_trig;
            teeth_per_trig[1] = base_teeth_per_trig;
            teeth_per_trig[2] = base_teeth_per_trig;
            teeth_per_trig[3] = base_teeth_per_trig;
            odd_trig_ang[0] = 0;
            odd_trig_ang[1] = 0;
            odd_trig_ang[2] = 0;
            odd_trig_ang[3] = 0;
        }

        if (num_cyl == 7) {
            int remainder, base_remainder, z, t = 0;
            /* common */
            teeth_per_trig[4] = base_teeth_per_trig;
            teeth_per_trig[5] = base_teeth_per_trig;
            teeth_per_trig[6] = base_teeth_per_trig;
            teeth_per_trig[7] = 0;
            odd_trig_ang[0] = 0; /* first one 'even' */
            odd_trig_teeth[0] = 0;
            odd_trig_ang[7] = 0;
            if ((ram4.EngStroke & 1) == 0) {
                /* 4 stroke */
                /* angle per event = 102.857 degrees (720.000/7) */
                base_remainder = 1028 % tmp_deg_per_tooth;
                remainder = 0;
                /* second and beyond might be over the next tooth */ 
                for (z = 1; z < 7; z++) {
                    remainder += base_remainder;
                    if (remainder > (int)tmp_deg_per_tooth) {
                        t++;
                        remainder -= tmp_deg_per_tooth;
                    }
                    odd_trig_ang[z] = -remainder;
                    odd_trig_teeth[z] = t;
                    }
            } else {
                /* 2 stroke not supported*/
                conf_err = 1;
            }
        } else {
            // For all other cases, double up pattern due to (j & 7)
            teeth_per_trig[4] = teeth_per_trig[0];
            teeth_per_trig[5] = teeth_per_trig[1];
            teeth_per_trig[6] = teeth_per_trig[2];
            teeth_per_trig[7] = teeth_per_trig[3];
            odd_trig_ang[4] = odd_trig_ang[0];
            odd_trig_ang[5] = odd_trig_ang[1];
            odd_trig_ang[6] = odd_trig_ang[2];
            odd_trig_ang[7] = odd_trig_ang[3];
            odd_trig_teeth[0] = 0;
            odd_trig_teeth[1] = 0;
            odd_trig_teeth[2] = 0;
            odd_trig_teeth[3] = 0;
            odd_trig_teeth[4] = 0;
            odd_trig_teeth[5] = 0;
            odd_trig_teeth[6] = 0;
            odd_trig_teeth[7] = 0;
        }

        /* at this point I have my 1st trigger tooth, and my trigger angle,
         * so I'll go ahead and set the rest of my trigger teeth
         */

        err_count = 0;
        base_trig_tooth = trig_tooth;
        base_trig_ang = trig_ang;

      WHEEL_TOOTH_CALC:
        trig_tooth = base_trig_tooth;
        trig_missing = 0;

        for (j = 0; j < no_triggers; j++) {

            trig_angs[j] = trig_ang + odd_trig_ang[j & 7];
            trigger_teeth[j] = trig_tooth + odd_trig_teeth[j & 7];
        
            // check if we landed on a gap 
            if (mid_last_tooth && (trig_tooth > mid_last_tooth) && (trig_tooth <= ram4.No_Teeth)) {
                trig_missing = ram4.No_Teeth - trig_tooth + 1;
            } else if ((trig_tooth > last_tooth) && (trig_tooth <= no_teeth)) {
                trig_missing = no_teeth - trig_tooth + 1;
            } else if (trig_tooth > no_teeth) {
                trig_tooth -= no_teeth;
            }
#if 0
/* isn't this just a duplicate ?! */
            if ((trig_tooth > last_tooth) && (trig_tooth <= no_teeth)) {
                /* not good, need to adjust all teeth... set a flag
                 * to tell us to do that later
                 */
                trig_missing = no_teeth - trig_tooth + 1;
            } else if (trig_tooth > no_teeth) {
                trig_tooth -= no_teeth;
            }
#endif
            trig_tooth += teeth_per_trig[j & 7];  // for next time around loop
            if (trig_tooth > no_teeth) {
                trig_tooth -= no_teeth;
            }
        }

        if (trig_missing) {
            err_count++;
            if (err_count > ram4.No_Teeth) {
                conf_err = 25; // failed to calc teeth
                return;
            }

            trig_ang = base_trig_ang - (tmp_deg_per_tooth * trig_missing);
            base_trig_tooth += trig_missing;
            if (base_trig_tooth > no_teeth) {
                base_trig_tooth -= no_teeth;
            }
            goto WHEEL_TOOTH_CALC; // try again
        }

        if (ram4.No_Teeth == 32) {
            // special case for 32 teeth
            //now convert back to deci-degrees
            trig_ang /= 10;
            for (j = 0; j < no_triggers; j++) {
                trig_angs[j] /= 10;
            }
            for (j = 0; j < no_teeth; j++) {
                unsigned int tmp_ang;
                tmp_ang = deg_per_tooth[j-1];
                // pattern will be 11.2, 11.3 etc.
                if ((j & 1) & (tmp_ang % 10)) {
                    tmp_ang = (tmp_ang / 10) + 1;
                } else {
                    tmp_ang = tmp_ang / 10;
                }
                deg_per_tooth[j-1] = tmp_ang;
            }
            tmp_deg_per_tooth = 113;
        }

        smallest_tooth_crk = tmp_deg_per_tooth;
        if (ram4.spk_config & 0x08) {   // dual or dual+missing
            if ((ram4.spk_config & 0xc0) == 0x40) {
                smallest_tooth_cam = 3600;      // crank speed
            } else if ((ram4.spk_config & 0xc0) == 0x80) {
                smallest_tooth_cam = 7200 / num_cyl; // every ignition event
            } else {
                smallest_tooth_cam = 7200;      // cam speed
            }
        }
    }                           // end real wheel mode

    // fill in rest of array with zeros
    for (j = no_teeth; j < MAXNUMTEETH; j++) {
        deg_per_tooth[j] = 0;
    }

    if (ram4.feature3 & 0x08) {
        tooth_init = trigger_teeth[ram4.trig_init];
        if ((tooth_init <= 0) || (tooth_init > no_teeth)) {
            tooth_init = trigger_teeth[0];
        }
    }
}

void calc_absangs()
{
/* calculate array of absolute tooth angles*/
    int t1, t2, ang;
    if (spkmode == 4) {
        /* This mode handles tooth numbers differently */
        ang = ram4.Miss_ang;
        t2 = 1; // was 0
    } else {
        // other wheel modes
        ang = trig_angs[0]; // this should always reference TDC compression #1
        t2 = trigger_teeth[0];
    }

    for (t1 = 0 ; t1 < no_teeth; t1++, t2++) {
        if (ang < 0) {
            ang += cycle_deg;
        }
        tooth_absang[t2 - 1] = ang;
        ang -= deg_per_tooth[t2 - 1];
        if (t2 >= no_teeth) {
            t2 = 0;
        }
    }
}
