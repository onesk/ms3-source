/* $Id: ms3_ign_in.c,v 1.297.4.7 2015/03/23 16:46:09 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * ISR_Ign_TimerIn()
    Origin: James Murray
    Majority: James Murray
 * exec_TimerIn()
    Origin: James Murray
    Majority: James Murray
 * ISR_Ign_TimerIn_paged
    Trace: Al Grippo
    Major: Complete re-write. James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 * ISR_Ign_TimerIn_part2
    Origin: Al Grippo
    Major: Complete re-write. James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#include "ms3.h"

void ISR_Ign_TimerIn(void)
{
    if (ISR_Ign_TimerIn_paged()) {    // call the paged function - returns true if common_wheel should exec
        ISR_Ign_TimerIn_part2(); // common_wheel onwards
    }
}

void exec_TimerIn(void)
{
    XGSWTM = 0x1000; // clear int flag
    checkforpit0();     // check for pending interrupt
    checkforsci0();
#ifdef MS3PRO
    checkforsci1();
#endif
    if (synch == 0) {
        outpc.syncreason = 65;
        outpc.istatus5 = xg_debug;
        ign_reset();
        return;
    } else {
        if (spkmode == 47) {
            flagbyte2 |= flagbyte2_twintrignow; // use cam timer in flywheel mode only
        }
        if (ISR_Ign_TimerIn_paged()) {    // XGATE told us to call the paged function
            ISR_Ign_TimerIn_part2(); // common_wheel onwards
        }
    }
}

/* This is now the real interrupt handler in paged flash */

INTERRUPT unsigned char ISR_Ign_TimerIn_paged(void)
{

    unsigned long ltmp1;
    unsigned char edge, edge2, portt_save;

#define SPARK 0x1
#define DWELL 0x2
#define FUEL 0x4
#define ROTARY_SPK 0x8
#define ROTARY_DWL 0x10
#define SEQFUEL 0x20
#define MAPSTART 0x40
#define KNOCKWINDOW 0x80

    if (flagbyte2 & flagbyte2_twintrignow) {    // use TC5/2 data for 2nd trig (Twin trig or CAS360)
        if (ram4.hardware & HARDWARE_CAM) {
            if (flagbyte9 & FLAGBYTE9_CAS360_ON) {
                TC0this = TC4;
            } else {
                TC0this = TC2;
            }
        } else {
            TC0this = TC5;
        }
        flagbyte4 |= flagbyte4_tach2;
    } else {                    // normal use this IC timer
        SSEM0;
        TFLG1 = 0x01;           // clear IC interrupt flag // not wanted in CAS360 mode
        TIE |= 0x01;            // re-enable IC interrupt
        CSEM0;
        TC0this = TC0;
        flagbyte4 &= ~flagbyte4_tach2;
    }

    portt_save = PORTT;         // grab it asap to ensure it doesn't change

    // if we triggered a config error then stop! No running, report false data
    // in test mode we ignore IC ISR as well
    if ((outpc.status1 & STATUS1_CONFERR) || (flagbyte1 & flagbyte1_tstmode)) {
        return(0);
    }

    TC0_32bits = ((unsigned long) swtimer << 16) | TC0this;
    if ((flagbyte1 & flagbyte1_ovfclose) && ((TC0this < (unsigned int)0x1000) ||
                                             ((TC0this < TC0_last)
                                              && (swtimer ==
                                                  swtimer_last)))) {
        TC0_32bits += 0x10000;
    }

    TC0_last = TC0this;
    swtimer_last = swtimer;

    TIMTCNT_this = TIMTCNT;

    TIMTCNT_32bits = ((unsigned long) swtimer_inj << 16) | TIMTCNT_this;
    if ((flagbyte6 & FLAGBYTE6_INJ_OVFCLOSE) && ((TIMTCNT_this < (unsigned int)0x1000) ||
                                                 ((TIMTCNT_this <
                                                   TIMTCNT_last)
                                                  && (swtimer_inj ==
                                                      swtimer_inj_last))))
    {
        TIMTCNT_32bits += 0x10000;
    }

    TIMTCNT_last = TIMTCNT_this;
    swtimer_inj_last = swtimer_inj;

    //Figure out which edge of input signal we are on if rising and falling enabled
    edge = 1;
    edge2 = 1;                  // doesn't get reset if single edge trigger on primary
    /* first figure out which edge we care about right now */
    // used by noise filter, dizzy with trigret and some spark modes
    if (flagbyte5 & FLAGBYTE5_CRK_BOTH) {
        if (ram4.ICIgnOption & 0x01) {
            if (!(portt_save & 0x01)) {
                /* this wasn't rise, don't store current timecounter value */
                edge = 0;
            }
        } else {
            if (portt_save & 0x01) {
                /* This wasn't the falling edge, don't store timecounter value */
                edge = 0;
            }
        }

        // Ignition trigger LED, only works at present if using both edges
        if (flagbyte1 & flagbyte1_igntrig) {
            if (edge) {
                SSEM0;
                PTM |= 0x20;    // D15
                CSEM0;
            } else {
                SSEM0;
                PTM &= ~0x20;
                CSEM0;
            }
        }
    }

// This is designed to allow COP with rising and falling mode triggering on both edges so can sync in 360deg
//    if ((spkmode == 4) && (ram4.spk_config & 0x08) && ((ram4.spk_config & 0x30) == 0x30)) {      // trig wheel & dual & rising+falling
    if (flagbyte9 & FLAGBYTE9_SPK4RISEFALL) {      // trig wheel & dual & rising+falling
        if (ram4.spk_config & 0x01) {
            if (!(portt_save & TFLG_trig2)) {
                edge2 = 0;
            }
        } else {
            if (portt_save & TFLG_trig2) {
                edge2 = 0;
            }
        }
    } else {
        /* cam edge / level detection */
        if (flagbyte10 & FLAGBYTE10_CAMPOL) {
            if (!(portt_save & TFLG_trig2)) {
                edge2 = 0;
            }
        } else {
            if (portt_save & TFLG_trig2) {
                edge2 = 0;
            }
        }
    }

// composite tooth logger - captures everything including noise
// logs are in comms page 0xf2, data actually stored in ram_data
// consists of 341 * 3 byte packets in the 1024 byte space
// 1024th byte (ramdata+0x3ff) now stores the logger format version.
// Hardcoded here AND in the isr_ign version of this code!

    if ((flagbyte0 & FLAGBYTE0_COMPLOG) && (!(flagbyte9 & FLAGBYTE9_CAS360_ON))) {
        do_complog_pri(TC0_32bits);     // re-written in assembler because gcc makes SUCH A MESS of it
        IC_last = TC0_32bits;
    }

    if (pulse_no == 0) {        // 1st input pulse
        pulse_no++;
        if (!(flagbyte9 & FLAGBYTE9_CAS360_ON)) {
            if (ram4.no_skip_pulses > 3) {
                tooth_no = ram4.no_skip_pulses - 1; // start down count of skip pulses
            } else {
                tooth_no = 2;
            }
        }
        dtpred = 0;           // predicted time difference for next pulse
        tooth_time1 = TC0_32bits;
        tooth_diff_this = 0;
        tooth_diff_last = 0;
        tooth_diff_last_1 = 0;
        tooth_diff_last_2 = 0;
        fuel_cntr = 0;
        flagbyte4 &= ~flagbyte4_found_miss;
        flagbyte21 &= ~FLAGBYTE21_FOUNDFIRST;
        NoiseFilter1 = 0;
        t_enable_IC = 0xFFFFFFFF;
        trig2cnt = 0;
        trig3cnt = 0;
        trig4cnt = 0;
        trig5cnt = 0;
        flagbyte1 &= ~flagbyte1_trig2active;
        ls1_sl = 0;
        ls1_ls = 0;
        //sum_dltau[0] = 0xFFFFFFFF;
        //sum_dltau[1] = 0xFFFFFFFF;
        SSEM0;
        dwellsel = 0;
        dwellq[0].sel = 0;
        dwellq[1].sel = 0;
        CSEM0;
        if ((spkmode == 4) && ((ram4.spk_conf2 & 0x07) == 0x02)) { // special case for C3I, do not skip_teeth
            tooth_no = 0;
        } else if ((ram4.spk_mode3 & SPK_MODE3_KICK)
            && ((spkmode == 3) || ((spkmode == 2) && (ram4.adv_offset < 200)))
            ) {
            if (lmms < 4) {
                /* Ignore any bogus pulses immediately after power-on */
                pulse_no = 0;
                return(0);
            }
            /* special case, do not skip_teeth */
            tooth_no = 0;
            outpc.engine |= ENGINE_CRANK;
        } else {
            return(0);
        }
    }

    /* automatic polarity noise filter when CRK_BOTH*/
    if (flagbyte9 & FLAGBYTE9_CRKPOLCHK) {
        if (last_edge < 2) {
            if (last_edge == edge) {
                //impossible!
                return(0);
            }
        }
        last_edge = edge;
    }

    /* Noise filter */
    if (flagbyte1 & flagbyte1_noisefilter) {
        if (!(flagbyte21 & FLAGBYTE21_FOUNDFIRST)) {
            if (edge == 0) {    /* We want to store the edge on the opposite one from the trigger */
                NoiseFilter1 = TC0_32bits;
            } else {
                return(0);
            }

            flagbyte21 |= FLAGBYTE21_FOUNDFIRST;
            return(0);
        } else {
            /* This edge should be the opposite edge that we saw previously...
             * DON'T just use it! Check the polarity
             */
            if (edge == 0) {
                return(0);         // must be noise - same edge as last time
            }

            flagbyte21 &= ~FLAGBYTE21_FOUNDFIRST;

            if (outpc.rpm > 10) {
                if ((TC0_32bits - NoiseFilter1) < NoiseFilterMin) {
                    return(0);
                }
            }
        }
        /* or Polarity check (can't presently have both) */
        /* intentionally read from PORTT instead of portt_save, to allow a slight delay */
    } else if (flagbyte1 & flagbyte1_polarity) {
        if (flagbyte2 & flagbyte2_twintrignow) {        // actually came from second trig
            if (ram4.ICIgnOption & 0x01) {      // rising IC
                if ((PORTT & TFLG_trig2) == 0) {        // but is actually low!
                    return(0);     // bail out
                }
            } else {            // falling edge
                if (PORTT & TFLG_trig2) {       // but is actually high!
                    return(0);     // bail out
                }
            }
        } else {                // normal
            if (ram4.ICIgnOption & 0x01) {      // rising IC
                if ((PORTT & 0x01) == 0) {      // but is actually low!
                    return(0);     // bail out
                }
            } else {            // falling edge
                if (PORTT & 0x01) {     // but is actually high!
                    return(0);     // bail out
                }
            }
        }
    }

    flagbyte2 &= ~flagbyte2_twintrignow;        // clear this
    /* Putting this here will cause code to think there's a stall if there are
     * a LOT of short pulses in a row, resetting ignition.
     */
    ltch_lmms = lmms;           // latch RTI .128 ms clk
    TC_crank = TC0_32bits;    // only store on valid tooth

    //*******************************************************************************

    temp1 = TC0_32bits - tooth_time1;   // tooth gap that just happened

    // False trigger period rejection
    if (false_period_crk_tix) {
        if ((unsigned int) temp1 < false_period_crk_tix) {
            // assume this is a false trigger
            return(0);
        }
    }

    //Trigger return handling
    if ((spkmode == 3) && (!edge)) {
        if ((synch & SYNC_SYNCED) && nextcyl_cnt ) {
            if ((ram4.spk_mode3 & SPK_MODE3_KICK) && (temp1 < 300000) && (coilsel == 0)) {
                /* Kickstart delayed firing */
                /* Lazy cranking uses other clause and clears coil as a precaution */
                TC7 = TC0this + ram5.kickdelay;
                TC1 = TC0this + ram5.kickdelay + (outpc.coil_dur * 100);
                SSEM0;
                TIE |= 0x82;
                TFLG1 = 0x82;
                dwellsel = 1;
                coilsel = 1;
                CSEM0;
            } else {
                // trigger return, cranking, return edge
                SSEM0;
                TIE &= ~0x02;
                TFLG1 = 0x02;
                coilsel = 1;
                CSEM0;
                FIRE_COIL;
            }
        }
        // always bail on the return pulse anyway. Not used for timing (anymore)
        return(0);
    }

    tooth_time1 = TC0_32bits;   // so it only runs for real teeth

    tooth_diff_last_2 = tooth_diff_last_1;      // the one before that
    tooth_diff_last_1 = tooth_diff_last;        // the one before
    tooth_diff_last = tooth_diff_this;  // previous gap

    tooth_diff_this = temp1;    // tooth gap that just happened

    //2nd trig polling removed as now input capture

    //tooth logger
    if ((flagbyte0 & FLAGBYTE0_TTHLOG) && edge) {
        unsigned long log_tooth;
        if (flagbyte5 & FLAGBYTE5_CRK_DOUBLE) {
            log_tooth = tooth_diff_this + tooth_diff_last;
        } else {
            log_tooth = tooth_diff_this;
        }

        if (log_offset < 1022) {

//Here we use RPAGE for the tooth/trigger log FIXME HARDCODED due to gcc bug
        __asm__ __volatile__("ldab  %0\n"
                             "pshb\n"
                             "movb #0xf0, %0\n"
                            :
                            :"m"(RPAGE)
                            :"d"
            );

        __asm__ __volatile__("ldab  %1\n"
                             "andb  #0xf\n"
                             "brclr %3, #1, tlnot_sync\n"   // #0x01 is SYNC_SYNCED -> 0x10 in log top byte
                             "orab  #0x10\n"
                             "tlnot_sync:\n"
                             "brclr %3, #8, tlnot_semi\n"       // #0x08 is SYNC_SEMI -> 0x40 in log top byte
                             "orab  #0x40\n"
                             "tlnot_semi:\n"
                             "brclr %3, #0x10, tlnot_semi2\n"   // #0x10 is SYNC_SEMI2 -> 0x80 in log top byt
                             "orab  #0x80\n"
                             "tlnot_semi2:\n"
                             "brclr %4,#1,tlnot_trg2\n"        // #0x01 is flagbyte1_trig2active -> 0x20 in log top byte
                             "orab  #0x20\n"
                             "tlnot_trg2:\n"
                             "stab  0,Y\n"
                             "ldd  %2\n"
                             "std  1,Y\n"
                            :
                            :"y"(log_offset + TRIGLOGBASE),  // FIXME HARDCODED start of RAM window
                             "m"(*((unsigned char *) &log_tooth + 2)),    // byte 3:**2**:1:0 // offset of one due to stacking 
                             "m"(*((unsigned char *) &log_tooth + 3)),     // bytes 3:2:**1**:**0**
                             "m"(synch), "m"(flagbyte1)
                            :"d"
            );

        __asm__ __volatile__("pulb\n"
                             "stab %0\n"
                            :
                            :"m"(RPAGE)
                            :"d"
            );

        }

        log_offset += 3;

        if (log_offset > 1021) {
            flagbyte0 &= ~FLAGBYTE0_TTHLOG;     // turn off logger
            outpc.status3 |= STATUS3_DONELOG;
        }
    } else if (flagbyte0 & (FLAGBYTE0_MAPLOG | FLAGBYTE0_MAFLOG))  {
        do_maplog_tooth(); // records tooth no.
    }

    // cumulative tooth
    cum_tooth++;
    if (((ram4.log_style2 & 0x18) == 0x08) && (sd_phase == 0x41)) {
        sd_phase++;             // trigger a new log block
    }

    {
        unsigned char RPAGE_bak = RPAGE;
        RPAGE = 0xfb;

        if (ram_window.pg24.vvt_opt1 & 0x03) { /* VVT enabled at all */
            /* VVT sample */
            if (flagbyte11 & (FLAGBYTE11_CAM1 | FLAGBYTE11_CAM2 | FLAGBYTE11_CAM3 | FLAGBYTE11_CAM4 )) {
                
                if (flagbyte11 & FLAGBYTE11_CAM1) {
                    cam_tooth_sample[0] = cam_tooth[0];
                    cam_time_sample[0] = cam_time[0];
                    cam_trig_sample[0] = cam_trig[0];
                    crank_time_sample[0] = tooth_diff_this;
                    flagbyte11 &= ~FLAGBYTE11_CAM1;
                    vvt_calc |= 0x1;
                }
                if (flagbyte11 & FLAGBYTE11_CAM2) {
                    cam_tooth_sample[1] = cam_tooth[1];
                    cam_time_sample[1] = cam_time[1];
                    cam_trig_sample[1] = cam_trig[1];
                    crank_time_sample[1] = tooth_diff_this;
                    flagbyte11 &= ~FLAGBYTE11_CAM2;
                    vvt_calc |= 0x2;
                }
                if (flagbyte11 & FLAGBYTE11_CAM3) {
                    cam_tooth_sample[2] = cam_tooth[2];
                    cam_time_sample[2] = cam_time[2];
                    cam_trig_sample[2] = cam_trig[2];
                    crank_time_sample[2] = tooth_diff_this;
                    flagbyte11 &= ~FLAGBYTE11_CAM3;
                    vvt_calc |= 0x4;
                }
                if (flagbyte11 & FLAGBYTE11_CAM4) {
                    cam_tooth_sample[3] = cam_tooth[3];
                    cam_time_sample[3] = cam_time[3];
                    cam_trig_sample[3] = cam_trig[3];
                    crank_time_sample[3] = tooth_diff_this;
                    flagbyte11 &= ~FLAGBYTE11_CAM4;
                    vvt_calc |= 0x8;
                }

            }

            if (vvt_decoder == 1) { /* BMW V10 decoder */
                flagbyte12 |= FLAGBYTE12_CAM2ARM | FLAGBYTE12_CAM4ARM; /* 2 and 4 always armed */
                /* check valid arming teeth for cams 1 + 3 */
                if ((tooth_no == ram_window.pg24.vvt_cam1tth1) || (tooth_no == ram_window.pg24.vvt_cam1tth2)) {
                    flagbyte12 |= FLAGBYTE12_CAM1ARM;
                }
                if ((tooth_no == ram_window.pg24.vvt_cam3tth1) || (tooth_no == ram_window.pg24.vvt_cam3tth2)) {
                    flagbyte12 |= FLAGBYTE12_CAM3ARM;
                }
            } else {
                flagbyte12 |= FLAGBYTE12_CAM1ARM | FLAGBYTE12_CAM2ARM | FLAGBYTE12_CAM3ARM | FLAGBYTE12_CAM4ARM;
            }
        }

        RPAGE = RPAGE_bak;
    }

    checkforpit0();     // check for pending interrupt
    checkforsci0();
#ifdef MS3PRO
    checkforsci1();
#endif

    if (spkmode == 31) {        // go for it right away - no need to sync
        goto SPKMODEFUEL;
    }

    if (spkmode < 2) {
        goto DO_EDIS;
    }

    // start with sync off, and wait a few pulses before trying to sync
    if ((!(synch & SYNC_SYNCED)) && (!(synch & SYNC_SEMI))
        && (!(synch & SYNC_SEMI2))) {
        // not synced yet, count down to 0 on tooth_no.
        if (tooth_no > 0) {
            tooth_no--;
            trig2cnt = 0;
            trig3cnt = 0;
            trig4cnt = 0;
            trig5cnt = 0;
            flagbyte1 &= ~flagbyte1_trig2active;        // ensure we see 2nd trigger at right time
            return(0);
        } else {
            // if we get this far we are 'ready'
            outpc.engine |= ENGINE_READY;
        }
    }


    /************ mode selection "if" *************/
    // creates faster asm
    if (spkmode == 2) {
        goto SPKMODE2;
    } else if (spkmode == 3) {
        goto SPKMODE3;
    } else if (spkmode == 4) {
        goto SPKMODE4;
    } else if (spkmode == 5) {
        goto SPKMODE5;
    } else if (spkmode == 6) {
        goto SPKMODE6;
    } else if ((spkmode == 7) || (spkmode == 57)) {
        goto SPKMODE7;
    } else if (spkmode == 8) {
        goto SPKMODE8;
    } else if (spkmode == 9) {
        goto SPKMODE9;
    } else if (spkmode == 10) {
        goto SPKMODE10;
    } else if (spkmode == 11) {
        goto SPKMODE11;
    } else if (spkmode == 12) {
        goto SPKMODE12;
    } else if (spkmode == 13) {
        goto SPKMODE13;
    } else if (spkmode == 14) {
        goto SPKMODE14;
    } else if (spkmode == 15) {
        goto SPKMODE15;
    } else if (spkmode == 16) {
        goto SPKMODE16;
    } else if (spkmode == 17) {
        goto SPKMODE17;
    } else if (spkmode == 18) {
        goto SPKMODE18;
    } else if (spkmode == 19) {
        goto SPKMODE19;
    } else if (spkmode == 20) {
        goto SPKMODE20;
    } else if (spkmode == 21) {
        goto SPKMODE21;
    } else if (spkmode == 22) {
        goto SPKMODE22;
    } else if (spkmode == 23) {
        goto SPKMODE23;
    } else if (spkmode == 24) {
        goto SPKMODE24;
    } else if (spkmode == 25) {
        goto SPKMODE25;
    } else if (spkmode == 26) {
        return(0); // logging crank in comp logger
    } else if (spkmode == 27) {
        return(0); // logging cam and crank in comp logger
    } else if (spkmode == 28) {
        goto SPKMODE28;
    } else if (spkmode == 29) {
        goto SPKMODE29;
    } else if (spkmode == 30) {
        goto SPKMODE30;
    } else if (spkmode == 31) {
        goto SPKMODEFUEL;
    } else if ((spkmode >= 32) && (spkmode <= 39)) {
        goto SPKMODE_CAS360;
    } else if (spkmode == 40) {
        goto SPKMODE40;
    } else if (spkmode == 41) {
        goto SPKMODE41;
    } else if (spkmode == 42) {
        goto SPKMODE42;
    } else if (spkmode == 43) {
        goto SPKMODE43;
    } else if (spkmode == 44) {
        goto SPKMODE44;
    } else if (spkmode == 45) {
        goto SPKMODE45;
    } else if (spkmode == 46) {
        goto SPKMODE46;
    } else if (spkmode == 47) {
        goto SPKMODE_CAS360;
    } else if (spkmode >= 48) {
        return(ISR_Ign_TimerIn_paged2(edge, edge2));
//        goto SPKMODE50;
//    } else if (spkmode == 51) {
//        goto SPKMODE51;
    } else {
        goto SPKMODEFUEL;       // really mode 31
    }
    //*******************************************************************************
  DO_EDIS:

    if (cycle_deg == 7200) {
        if (tooth_no == last_tooth) {
            if (flagbyte1 & flagbyte1_trig2active) {
                flagbyte1 &= ~flagbyte1_trig2active;
                outpc.status1 |= STATUS1_SYNCFULL;
            } else {
                outpc.syncreason = 11;  // let the user know there is a potential problem without losing sync
            }
            tooth_no = 0;
        }

        // first sync. If it happens again this could be problematic.
        if (flagbyte1 & flagbyte1_trig2active) {
            flagbyte1 &= ~flagbyte1_trig2active;
            tooth_no = 0;
            outpc.syncreason = 11;  // let the user know there is a potential problem without losing sync
            // will be normal to see this once during startup
        }
    }

    synch |= SYNC_SYNCED;

    goto common_wheel;

    /************ missing tooth wheel & missing and 2nd trigger wheel mode *************/
  SPKMODE4:
    if ((ram4.spk_config & 0xc) == 0x8) {       // non missing dual
        goto SPKMODE4B;
    }
    /* this is the "find missing" step */
    // look for missing tooth (subtract last tooth time from this tooth
    // count taking overflow into account, and then multiply previous
    // tooth time by 1.5 and see

    if (!(synch & SYNC_SYNCED)) {
        if (!(synch & SYNC_SEMI)) { 
            /* Here we will look for missing */
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);

            if (tooth_diff_this > temp1) {
                synch |= SYNC_SEMI;
            }
            outpc.cel_status &= ~0x800; // clear cam fault
            return(0);
        } else {
            /* tooth after what we thought was missing, lets make sure this one is short
             * on the first sync, we miss tooth #1, but that's ok as long as we catch it next time
             * split this into 2 operations so compiler doesn't call subroutine
             */
            temp1 = (tooth_diff_last >> 1); /* 1/2 of last value */
            temp1 = temp1 >> 1;     /* 1/4th of last value */

            temp1 = tooth_diff_last - temp1;        /* 3/4ths of last value */

            synch &= ~SYNC_SEMI;
            if (tooth_diff_this >= temp1) {
                /* this wasn't really the missing tooth... but don't tell the user we lost sync */
                return(0);
            } else {
                if ((ram4.spk_config & 0x0c) == 0x0c) { // dual + missing
                    /* For poll level we'll set the initial tooth here based on the cam level */
                    if ((ram4.spk_config & 0x30) == 0x30) { // poll level
                        if (ram4.spk_config & 0x01) {      // high cam for phase 1
                            if ((PORTT & TFLG_trig2) == 0) {        // low so phase 2
                                tooth_no = mid_last_tooth + ram4.No_Miss_Teeth + 1;
                            } else {
                                tooth_no = 1; // high so phase 1
                            }
                        } else {            // low cam for phase 1
                            if (PORTT & TFLG_trig2) {       // high so phase 2
                                tooth_no = mid_last_tooth + ram4.No_Miss_Teeth + 1;
                            } else {
                                tooth_no = 1; // low so phase 2
                            }
                        }
                        flagbyte1 |= flagbyte1_trig2active; // set cam flag regardless - for odd case
                    } else {
                        tooth_no = 1; // for non poll level case we can't guess the phase, so start at 1.
                    }

                    if (flagbyte22 & FLAGBYTE22_ODDFIRECYL) {
                        /* Wasted COP not permitted - must wait for definitive cam signal (or poll level) */
                        if (flagbyte1 & flagbyte1_trig2active) {
                            synch |= SYNC_SYNCED;
                            outpc.status1 |= STATUS1_SYNCFULL;
                        } else {
                            return(0);
                        }
                    } else {
                        /* For even fire and even nums of cylinders, start wasted */
                        synch |= (SYNC_SYNCED | SYNC_SEMI2);
                        if ((ram4.spk_mode3 & 0xe0) == 0x80) { // COP mode
                            synch |= SYNC_WCOP; // enable WCOP mode
                        }
                        ls1_sl = 0; // vars to record sync process
                        ls1_ls = 0;
                    }
                } else {
                    synch |= SYNC_SYNCED;
                    tooth_no = 1; // single wheel missing
                    outpc.status1 |= STATUS1_SYNCFULL;
                }
                flagbyte1 &= ~flagbyte1_trig2active; // clear cam flag
                if (vvt_decoder != 3) { /* Except Hemi VVT decoder */
                    trig2cnt = 0;
                    trig3cnt = 0;
                    trig4cnt = 0;
                    trig5cnt = 0;
                }
            }
        }

    } else {
        /* This is re-sync and where we switch from wasted to COP */

        if ((tooth_no == last_tooth) || ((tooth_no == mid_last_tooth) && ((ram4.spk_config & 0xc) == 0xc))) {
            /* this means we should have the last tooth here, so check for missing */
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);       /* 1.5 * last tooth */

            if (tooth_diff_this <= temp1) {
                // wasn't a missing tooth when it was supposed to be
                outpc.syncreason = 2;
                ign_reset();
                return(0);
            } else {
                // it was the missing tooth

                if ((synch & SYNC_SEMI2) || (synch & SYNC_WCOP)) {
                    /* Poll level, set cam bit in place of interrupt setbit */
                    if ((ram4.spk_config & 0x30) == 0x30) { /* poll level re-check */
                        if (ram4.spk_config & 0x01) {      // high cam for phase 1
                            if ((PORTT & TFLG_trig2) == 0) {        // low so phase 2
                                flagbyte1 &= ~flagbyte1_trig2active;
                            } else {
                                flagbyte1 |= flagbyte1_trig2active;
                            }
                        } else {            // low cam for phase 1
                            if (PORTT & TFLG_trig2) {       // high so phase 2
                                flagbyte1 &= ~flagbyte1_trig2active;
                            } else {
                                flagbyte1 |= flagbyte1_trig2active;
                            }
                        }
                    }
                }

                if (synch & SYNC_SEMI2) {
                    /* SEMI2 here means that we've synced to the crank, but have
                        yet to confirm cam sync. We are still running wasted COP and
                        are looking to match an on/off/on/off cam pattern. */

                    if (vvt_decoder == 2) { /* BMW S54 decoder */
                        ls1_sl++;
                        ls1_ls <<= 2;
                        if (trig2cnt == 3) {
                            ls1_ls |= 1;
                        } else if (trig2cnt == 4) {
                            ls1_ls |= 2;
                        }
                    } else if (vvt_decoder == 3) { /* Hemi VVT decoder */
                        ls1_sl++;
                        ls1_ls <<= 2;
                        if (trig2cnt == 1) {
                            ls1_ls |= 1;
                        } else if (trig2cnt == 2) {
                            ls1_ls |= 2;
                        }
                    } else if (vvt_decoder == 4) { /* Ford Coyote decoder */
                        /* Currently the same as S54, but might need a phase swap */
                        ls1_sl++;
                        ls1_ls <<= 2;
                        if (trig2cnt == 3) {
                            ls1_ls |= 1;
                        } else if (trig2cnt == 4) {
                            ls1_ls |= 2;
                        }
                    } else if (vvt_decoder == 5) { /* Ford Duratec 4cyl decoder */
                        ; /* Nothing here */
                    } else {
                        if ((ram4.poll_level_tooth == 0) // normal zero
                            || ((ram4.spk_config & 0x0c) != 0x0c) // not dual+missing
                            || ((ram4.spk_config & 0x30) != 0x30) // not poll level
                            ) {
                            /* normal behaviour is to check level right here */
                            ls1_sl++;
                            ls1_ls <<= 2;
                            if (flagbyte1 & flagbyte1_trig2active) {
                                ls1_ls |= 1;
                            } else {
                                ls1_ls |= 2;
                            }
                        }
                    }
                    
                    if ((ls1_sl > 3) && ((ls1_ls & 0xff) == 0x99)) { // i.e. 10011001
                        synch &= ~SYNC_SEMI2;        // found cam, stop WCOP of dwells
                        SSEM0;
                        dwellsel = 0; // have to cancel any dwells in rare case there is one
                        dwellq[0].sel = 0;
                        dwellq[1].sel = 0;
                        CSEM0;
                        tooth_no = 0; // declare we really are at the first tooth
                        outpc.status1 |= STATUS1_SYNCFULL;
                        syncfirst(); // re-calculate next triggers to prevent gap
                        if (mapadc_thresh) {
                            /* special cases to cancel wasted-COP dwells for phase sensing and switch to COP */
                            if ((ram4.no_cyl & 0x1f) == 1) {
                                skipdwell[1] = 1;
                            } else if ((ram4.no_cyl & 0x1f) == 2) { /* untested */
                                skipdwell[1] = 1;
                                skipdwell[2] = 1;
                            }
                        }
                    } else if (((mapadc_thresh == 0) && (ls1_sl > 10)) 
                        || (mapadc_thresh && (ls1_sl > 100))) {
                        /* Failed to receive matching pattern, cam input faulty */
                        /* 10 revs allowance on normal electrical cam sensor
                           100 revs for map threshold sensing in case operator starts at WOT */
                        synch &= ~(SYNC_SEMI2 | SYNC_WCOP);
                        ls1_sl = 99; // lock here
                        if ((ram4.spk_mode3 & 0xe0) == 0x80) { // COP mode
                            synch |= SYNC_WCOP2; // force WCOP mode
                        }
                        flagbyte17 |= FLAGBYTE17_STATCAM; //cam fault
                        outpc.cel_status |= 0x800;
                    }
                } else if (synch & SYNC_WCOP) {
                    /* WCOP was left on for one rev to ensure that any remaining spark events were cleared
                        to prevent the possibility of spark hang-on */
                    synch &= ~SYNC_WCOP;
                }
            }
            
            if ((flagbyte22 & FLAGBYTE22_ODDFIRECYL) /* oddfire/oddcyl and dual+missing */
                && ((ram4.spk_config & 0xc) == 0xc)
                && (flagbyte2 & flagbyte2_crank_ok)) {
                /* Old sync method - keep checking cam during initial startup period */

                if ((ram4.spk_config & 0x30) == 0x30) { /* poll level re-check */
                    /* Note that we are actually checking at tooth#0 instead of tooth#1
                        but unlikely to impact users from real-world engine data */
                    if (ram4.spk_config & 0x01) {      // high cam for phase 1
                        if ((PORTT & TFLG_trig2) == 0) {        // low so phase 2
                            flagbyte1 &= ~flagbyte1_trig2active;
                        } else {
                            flagbyte1 |= flagbyte1_trig2active;
                        }
                    } else {            // low cam for phase 1
                        if (PORTT & TFLG_trig2) {       // high so phase 2
                            flagbyte1 &= ~flagbyte1_trig2active;
                        } else {
                            flagbyte1 |= flagbyte1_trig2active;
                        }
                    }
                }

                if (tooth_no == last_tooth) {
                    if (!(flagbyte1 & flagbyte1_trig2active)) { /* need to have had a cam pulse */
                        outpc.syncreason = 17;
                        ign_reset();
                        return(0);
                    }
                    tooth_no = 0;
                } else if (tooth_no == mid_last_tooth) {
                    if (flagbyte1 & flagbyte1_trig2active) { /* must not have had a cam pulse */
                        outpc.syncreason = 17;
                        ign_reset();
                        return(0);
                    }
                    tooth_no += ram4.No_Miss_Teeth;
                }

            } else { /* no re-sync checking */
                if (tooth_no == last_tooth) {
                    tooth_no = 0;
                } else if (tooth_no == mid_last_tooth) {
                    tooth_no += ram4.No_Miss_Teeth;
                }
            }
            flagbyte1 &= ~flagbyte1_trig2active; // clear cam flag
            trig2cnt = 0;
            trig4cnt = 0;

        } else if (synch & SYNC_SEMI2) {
            if (vvt_decoder == 3) { /* Hemi VVT decoder SEMI */
                /* reset points that we count cam teeth across */
                if ((tooth_no == 5)
                    || (tooth_no == 20)
                    || (tooth_no == 35)
                    || (tooth_no == 50)
                    || (tooth_no == 65)
                    || (tooth_no == 80)
                    || (tooth_no == 95)
                    || (tooth_no == 110)) {
                    trig2cnt = 0;
                }
            } else if (vvt_decoder == 5) { /* Ford Duratec 4cyl */
                /* reset points that we count cam teeth across */
                if ((tooth_no == 10) || (tooth_no == 46)) {
                    trig2cnt = 0;
                } else if ((tooth_no == 30) || (tooth_no == 66)) {
                    ls1_sl++;
                    ls1_ls <<= 2;
                    if (trig2cnt == 1) {
                        ls1_ls |= 1;
                    } else if (trig2cnt == 2) {
                        ls1_ls |= 2;
                    }
                }
            } else if (ram4.poll_level_tooth // special option to move poll tooth
                && ((ram4.spk_config & 0x0c) == 0x0c) // dual+missing
                && ((ram4.spk_config & 0x30) == 0x30) // poll level
                && ((tooth_no == ram4.poll_level_tooth) || (tooth_no == (ram4.poll_level_tooth + ram4.No_Teeth))) ) {
                /* special behaviour is to check level at this specified tooth */
                ls1_sl++;
                ls1_ls <<= 2;
                if (ram4.spk_config & 0x01) {      // high cam for phase 1
                    if ((PORTT & TFLG_trig2) == 0) {        // low so phase 2
                        ls1_ls |= 2;
                    } else {
                        ls1_ls |= 1;
                    }
                } else {            // low cam for phase 1
                    if (PORTT & TFLG_trig2) {       // high so phase 2
                        ls1_ls |= 2;
                    } else {
                        ls1_ls |= 1;
                    }
                }
            }
        } else if (vvt_decoder == 4) {
                /* reset points that we count exhaust cam teeth across */
                if ((tooth_no == 21) || (tooth_no == 57)) {
                    trig3cnt = 0;
                    trig5cnt = 0;
                }
        }
    }
    goto common_wheel;

    /************ 2nd trigger non-missing mode *************/
  SPKMODE4B:
    if ((ram4.spk_config & 0x30) == 0x30)
        goto SPKMODE4C;

/* new method - like used on missing tooth wheel - only check for cam trigger when we are expecting it
   Will also need some method of ensuring that masking/period rejection isn't trigger by incorrect
   noise pulse
*/
    if (!(synch & SYNC_SYNCED)) {
        if (!(flagbyte1 & flagbyte1_trig2active)) {
            return(0);             // wait until we get a second trigger
        }
        /* can only get here if we have received a 2nd trigger */
        if (((ram4.spk_config & 0xc0) == 0) || (ram4.EngStroke & 0x01)) {    // cam speed reset pulse or two stroke/rotary
            outpc.status1 |= STATUS1_SYNCFULL;
        }
        synch |= SYNC_SYNCED;
        tooth_no = 0;
    } else {
        if (tooth_no == no_teeth) {
            /* recheck sync */
            if (flagbyte1 & flagbyte1_trig2active) {
                /* all is well, got the second trigger when expected */
                tooth_no = 0;
            } else {
                /* didn't received the second trigger when expected - sync error */
                outpc.syncreason = 17;
                ign_reset();
            }
        } else {
            /* not time to check sync */
            if (flagbyte1 & flagbyte1_trig2active) {    // had a trigger on cam we shouldn't have done
                t_enable_IC2 = 0xffffffff;        // reset this data
                if (ram4.hardware & HARDWARE_CAM) {        // ensure 2nd trig ISR is on
                    SSEM0;
                    TFLG1 = 0x04;
                    TIE |= 0x04;
                    CSEM0;
                } else {
                    SSEM0;
                    TFLG1 = 0x20;
                    TIE |= 0x20;
                    CSEM0;
                }
                TC_trig2_last = TC_trig2_last2; // restore previous tooth time to ignore false pulse
                outpc.syncreason = 11;  // let the user know there is a potential problem without losing sync
            }
            /* do nothing else, just count the teeth in this mode */
        }
    }
    flagbyte1 &= ~flagbyte1_trig2active;        // clear flag
    goto common_wheel;

    /************ 2nd trigger non-missing rising and falling mode *************/
    /* likely redundant as it was designed for LS1, but now replaced by real LS1 mode */
  SPKMODE4C:;
    if ((!(synch & SYNC_SYNCED)) && (!(flagbyte1 & flagbyte1_trig2active))) {
        return(0);                 // wait until we get a second trigger
    }

    if (flagbyte1 & flagbyte1_trig2active) {
        flagbyte1 &= ~flagbyte1_trig2active;

        //noise check
        if (synch & SYNC_SYNCED) {
            if (((edge2) && (flagbyte1 & flagbyte1_trig2statl))
                || ((!edge2) && (!(flagbyte1 & flagbyte1_trig2statl)))) {
                return(0);         // same state as last time = noise
            }
        } else {
            outpc.status1 |= STATUS1_SYNCFULL;
            synch |= SYNC_SYNCED;
        }

        // store last 2nd trigger status
        if (edge2) {
            flagbyte1 |= flagbyte1_trig2statl;
        } else {
            flagbyte1 &= ~flagbyte1_trig2statl;
        }

        if (synch & SYNC_FIRST) {
            syncfirst();
        } else {
            if ((!edge2 && (tooth_no != (no_teeth >> 1)))
                || (edge2 && (tooth_no != no_teeth))) {
                syncerr++;
                if (outpc.status1 & STATUS1_SYNCOK) {   // if currently synced
                    outpc.synccnt++;
                }
            } else {            // no problem so decrement error counter
                if (syncerr) {
                    syncerr--;
                }
            }
        }

        if (edge2) {
            tooth_no = 0;       // start of sequence
        } else {
            tooth_no = no_teeth >> 1;   // halfway through sequence
        }
    }
    goto common_wheel;

    /************ dizzy mode *************/
  SPKMODE2:
  SPKMODE3:
    if ((!(synch & SYNC_SYNCED)) && (!edge)) { // && !edge ??
        return(0);                 // only sync on correct edge
    }

    if ((spkmode == 2) && ((ram4.spk_conf2 & 0x07) == 0x04)) {
        // sig-PIP TFI
        if (!edge) {
            // record time, but nothing more on uneven vane edge
            return(0);
        }
        if (!(outpc.status1 & STATUS1_SYNCFULL)) {
            if (tooth_diff_this && tooth_diff_last && tooth_diff_last_1) {
                unsigned long t1;
                t1 = tooth_diff_this >> 1;
                t1 += tooth_diff_this; // 1.5 * this
                if ((t1 < tooth_diff_last) && (tooth_diff_last > tooth_diff_last_1)) {
                    tooth_no = 0;
                    outpc.status1 |= STATUS1_SYNCFULL;
                }
            }
        }
        //check we are still synced up
    } else if ((!edge) && (!(tooth_no & 1))) { // out of sync
        outpc.syncreason = 13;
        ign_reset();            // kill the lot
        return(0);
    }
    //oddfire sync
    // concern that heavy accel/decel on slightly odd engines might lose sync
    // sync if  this < last  and  last > last_1  i.e. period coming up is long

    if (ram4.ICIgnOption & 0x8) {
        if (!(synch & SYNC_SYNCED)) {
            if ((!tooth_diff_this) || (!tooth_diff_last) || (!tooth_diff_last_1)) {
                return(0); // not enough data yet
            }
            if (!((tooth_diff_this > tooth_diff_last) && (tooth_diff_last < tooth_diff_last_1))) {
                return(0);             // not found right pattern yet
            }
        } else {
            // re-check for sync, tooth_no here is 1 less than normal as ++ below
            if  ((tooth_no & 1) == 0) {
                if ((tooth_diff_this < tooth_diff_last) && (tooth_diff_last > tooth_diff_last_1)) {
                    outpc.syncreason = 14;
                    ign_reset();        // out of sync
                    return(0);
                }
            }
        }
    }

    synch |= SYNC_SYNCED;       // always synced in dizzy mode
    if (synch & SYNC_FIRST) {
        tooth_no = 0;           // syncfirst now follows common_wheel
    }
    goto common_wheel;

    /************ 420A/Neon mode *************/
  SPKMODE5:
    //initial sync

    if (!edge) {                // only sync or do spark on falling edge
        flagbyte1 &= ~flagbyte1_trig2active;    // clear 2nd trig
        return(0);
    }

    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_last) || (!tooth_diff_last)
            || (!tooth_diff_last_1)
            || (!tooth_diff_last_2)) {
            return(0);             // only sync on falling edge
        }
        // see if we've got sync. Look for a tooth that is a lot longer than previous teeth
        if (tooth_diff_last > (tooth_diff_last_1 << 1)) {
            if (tooth_diff_this > (tooth_diff_last_1 << 1)) {
                tooth_no = 0;   // this tooth is 1
                synch |= SYNC_SYNCED;
            } else if ((tooth_diff_last > (tooth_diff_this << 1))
                       && (tooth_diff_last >
                           ((tooth_diff_last_1 +
                             tooth_diff_last_2) << 1))) {
                tooth_no = 4;   // this tooth is 5
                synch |= SYNC_SYNCED;
            } else {
                flagbyte1 &= ~flagbyte1_trig2active;    // clear 2nd trig, in phase when it should be low
                return(0);
            }
        }
        // if COP or allow-CAM, check cam phase
        if (cycle_deg == 7200) {
            outpc.status1 |= STATUS1_SYNCFULL;
            if (flagbyte1 & flagbyte1_trig2active) {
                tooth_no = tooth_no + 8;
            }
        }
    } else {
        // recheck for sync
        if ((tooth_no == 16) || (tooth_no == 12) || (tooth_no == 8)
            || (tooth_no == 4)) {
            // see if we've got sync. Look for the pre 69deg tooth

            if (tooth_diff_last > (tooth_diff_last_1 << 1)) {
                if (tooth_diff_this > (tooth_diff_last_1 << 1)) {
                    if ((tooth_no != 8) && (tooth_no != 16)) {
                        outpc.syncreason = 23;
                        ign_reset();
                        return(0);
                    }
                    // if COP or allow-CAM, check cam phase
                    if (cycle_deg == 7200) {
                        if ((tooth_no == 8)
                            && (flagbyte1 & flagbyte1_trig2active)) {
                            tooth_no = 8;       // only halfway through cycle
                        } else if ((tooth_no == 16)
                                   && ((flagbyte1 & flagbyte1_trig2active)
                                       == 0)) {
                            tooth_no = 0;       // start of cycle
                        } else {
                            outpc.syncreason = 24;
                            ign_reset();        // cam phase wrong
                        }
                    } else {
                        tooth_no = 0;   // this tooth is 1
                    }
                } else if ((tooth_diff_last > (tooth_diff_this << 1))
                           && (tooth_diff_last >
                               ((tooth_diff_last_1 +
                                 tooth_diff_last_2) << 1))) {
                    if ((tooth_no != 4) && (tooth_no != 12)) {
                        outpc.syncreason = 25;
                        ign_reset();
                        return(0);
                    }
                    // tooth no. ok
                } else {
                    outpc.syncreason = 26;
                    ign_reset();
                    return(0);
                }
            } else {
                // got out of sync
                outpc.syncreason = 27;
                ign_reset();
                return(0);
            }
        }
    }

    flagbyte1 &= ~flagbyte1_trig2active;        // clear 2nd trig

    goto common_wheel;

    /************ 36-1+1 36-2+2 NGC mode *************/
  SPKMODE6:
    /* The designers here use a common crank pattern across a number   *
     * of models, but 4,6,8 use a different cam pattern.               *
     * That cam pattern could be used as a fallback and run the engine *
     * if the crank sensor fails, but it makes it a pain to decode.    *
     */
    if (num_cyl == 4) {
        //initial sync
        if (!(synch & SYNC_SYNCED)) {
            if ((!edge) || (!tooth_diff_this) || (!tooth_diff_last)
                || (!tooth_diff_last_1) || (!tooth_diff_last_2)) {
                return(0);             // only sync on falling edge
            }
            // see if we've got sync. Look for the +1/-1 tooth

            if (!(synch & SYNC_SEMI)) {
                // see if we've got sync. Look for the +1/-1 tooth
                if ((tooth_diff_this + tooth_diff_last) > ((tooth_diff_last_1 + tooth_diff_last_2) << 1)) {
                    if (tooth_diff_last > (tooth_diff_this << 1)) {
                        tooth_no = 0;
                    } else {
                        tooth_no = 16;
                    }
                    if (flagbyte5 & FLAGBYTE5_CAM) {
                        synch |= SYNC_SEMI; // have crank sync, now need cam sync too
                        trig2cnt = 0;
                    } else {
                        synch |= SYNC_SYNCED | SYNC_FIRST;
                    }

                } else {
                    return(0);             // not found right sequence yet
                }
            } else {
                tooth_no++;
                if (trig2cnt && ((tooth_no == 32) || (tooth_no == 64))) {
                    if (edge2) {
                        tooth_no = 0;
                    } else {
                        tooth_no = 32;
                    }
                    synch |= SYNC_SYNCED | SYNC_FIRST;
                    synch &= ~SYNC_SEMI;
                    outpc.status1 |= STATUS1_SYNCFULL;
                } else if (tooth_no == 64) {
                    /* got this far without seeing any cam transitions, must be broken, continue without it */
                    if ((ram4.spk_mode3 & 0xc0) == 0x80) {
                        synch |= SYNC_WCOP2;
                    }
                    synch |= SYNC_SYNCED | SYNC_FIRST;
                    synch &= ~SYNC_SEMI;
                    tooth_no = 0;
                    trig2cnt = 0;
                } else {
                    // have SEMI, but not finished sequence yet.
                    goto spkmode6_semi; // start calculating rpms and populating arrays
                }
            }
        } else {
            //while synced only use falling edge for further calcs
            if (!edge) {
                return(0);
                //ignore the rising edge for timing purposes, only count falling edge as a tooth
            }

            // recheck for sync
            if ((tooth_no == 32) || (tooth_no == 16) || (tooth_no == 64) || (tooth_no == 48)) {     // remove leading 1
                // see if we've got sync. Look for the +1/-1 tooth
                if ((tooth_diff_this + tooth_diff_last) > ((tooth_diff_last_1 + tooth_diff_last_2) << 1)) {
                    // only check crank
            		if (tooth_diff_last > (tooth_diff_this <<1)) {
                        if (tooth_no & 0x1f) { // not 0x20 or 0x40
                            outpc.syncreason = 61;
                            goto spkmode6_loss;
                        } else if ((tooth_no == 64) || ((tooth_no == 32) && ((flagbyte5 & FLAGBYTE5_CAM) == 0)))  {
                            tooth_no = 0;
                        }
                        if (outpc.status1 & STATUS1_SYNCFULL) {
                            if (!trig2cnt) { // no cam transitions since last +1/-1
                                outpc.syncreason = 70;
                                goto spkmode6_loss;
                            } else {
                                trig2cnt = 0;
                            }
                        }    
                    } else {
                        if ((tooth_no & 0xdf) != 0x10) { // not 0x30 or 0x10
                            outpc.syncreason = 62;
                            goto spkmode6_loss;
                        } else if ((outpc.status1 & STATUS1_SYNCFULL) && ((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok))) {
                            /* recheck phase during crank & initial only */
                            if (tooth_no == 32) {
                                if (edge2) { /* wrong phase, fix it */
                                    outpc.syncreason = 91;
                                    ign_reset();
                                    synch |= SYNC_SYNCED | SYNC_FIRST;
                                    synch &= ~SYNC_SEMI;
                                    outpc.status1 |= STATUS1_SYNCFULL;
                                    tooth_no = 0;
                                }
                            } else if (tooth_no == 64) {
                                if (!edge2) {
                                    outpc.syncreason = 91;
                                    ign_reset();
                                    synch |= SYNC_SYNCED | SYNC_FIRST;
                                    synch &= ~SYNC_SEMI;
                                    outpc.status1 |= STATUS1_SYNCFULL;
                                    tooth_no = 32;
                                }
                            }
                        }
                    }
        	    } else {
                    outpc.syncreason = 28;
    spkmode6_loss:;
                    ign_reset();    // got out of sync
                    return(0);
                }
            }
        }
    } else if (num_cyl == 6) {
        // decoder for 6cyl.
        // only need cam signal for COP or sequential. Otherwise the crank signal suffices.
        //initial sync
        if (!(synch & SYNC_SYNCED)) {
            if ((!edge) || (!tooth_diff_this) || (!tooth_diff_last)
                || (!tooth_diff_last_1) || (!tooth_diff_last_2)) {
                return(0);             // only sync on falling edge
            }
            if (!(synch & SYNC_SEMI)) {
                // see if we've got sync. Look for the +1/-1 tooth
                if ((tooth_diff_this + tooth_diff_last) > ((tooth_diff_last_1 + tooth_diff_last_2) << 1)) {
                    if (tooth_diff_last > (tooth_diff_this << 1)) {
                        tooth_no = 0;
                    } else {
                        tooth_no = 16;      // standard cam
                    }
                    if (flagbyte5 & FLAGBYTE5_CAM) {
                        synch |= SYNC_SEMI; // have crank sync, now need cam sync too
                        trig2cnt = 0;
                        return(0);
                    } else {
                        synch |= SYNC_SYNCED | SYNC_FIRST;
                    }

                } else {
                    return(0);             // not found right sequence yet
                }
            } else {
                tooth_no++;
                if (tooth_no == 16) {
                    trig2cnt = 0;
                } else if (tooth_no == 22) {
                    /* count the cam teeth that follow the +1 section */
                    if (trig2cnt == 1) {
                        tooth_no = 54;
                    } else if (trig2cnt == 2) {
                        tooth_no = 22;
                    } else {
                        outpc.syncreason = 66;
                        ign_reset();    // got out of sync, start again
                        return(0);
                    }
                    synch |= SYNC_SYNCED | SYNC_FIRST;
                    synch &= ~SYNC_SEMI;
                    outpc.status1 |= STATUS1_SYNCFULL;
                } else if (tooth_no > 33) {
                    // shouldn't be possible to get here
                    outpc.syncreason = 66;
                    ign_reset();    // got out of sync, start again
                    return(0);
                } else {
                    // have SEMI, but not finished sequence yet.
                    goto spkmode6_semi; // start calculating rpms and populating arrays
                }
            }
        } else {
            //while synced only use falling edge for further calcs
            if (!edge) {
                return(0);
                //ignore the rising edge for timing purposes, only count main edge as a tooth
            }
            // recheck for sync - only check crank
            if ((tooth_no == 32) || (tooth_no == 16) || (tooth_no == 64) || (tooth_no == 48)) {     // remove leading 1
                // see if we've got sync. Look for the +1/-1 tooth
                if ((tooth_diff_this + tooth_diff_last) > ((tooth_diff_last_1 + tooth_diff_last_2) << 1)) {
                    // only check crank
            		if (tooth_diff_last > (tooth_diff_this <<1)) {
                        if (tooth_no & 0x1f) { // not 0x20 or 0x00
                            outpc.syncreason = 61;
                            goto spkmode6_loss6;
                        } else if ((tooth_no == 64) || ((tooth_no == 32) && ((flagbyte5 & FLAGBYTE5_CAM) == 0)))  {
                            tooth_no = 0;
                        }
                    } else {
                        if ((tooth_no & 0xdf) != 0x10) { // not 0x30 or 0x10
                            outpc.syncreason = 62;
                            goto spkmode6_loss6;
                        }
                    }
        	    } else {
                    outpc.syncreason = 28;
    spkmode6_loss6:;
                    ign_reset();    // got out of sync
                    return(0);
                }
            }
        }
    } else if (num_cyl == 8) {
        // decoder for 8cyl.
        // only need cam signal for COP or sequential. Otherwise the crank signal suffices.
        //initial sync
        if (!(synch & SYNC_SYNCED)) {
            if ((!edge) || (!tooth_diff_this) || (!tooth_diff_last)
                || (!tooth_diff_last_1) || (!tooth_diff_last_2)) {
                return(0);             // only sync on falling edge
            }
            
            if (!(synch & SYNC_SEMI)) {
                // see if we've got sync. Look for the +1/-1 tooth
                if ((tooth_diff_this + tooth_diff_last) > ((tooth_diff_last_1 + tooth_diff_last_2) << 1)) {
                    if (tooth_diff_last > (tooth_diff_this << 1)) {
                        tooth_no = 0;
                    } else {
                        tooth_no = 16;      // standard cam
                    }
                    if (flagbyte5 & FLAGBYTE5_CAM) {
                        synch |= SYNC_SEMI; // have crank sync, now need cam sync too
                        trig2cnt = 0;
                        trig2cnt_last = 0;
                        goto spkmode6_semi;
                    } else {
                        synch |= SYNC_SYNCED | SYNC_FIRST;
                    }

                } else {
                    return(0);             // not found right sequence yet
                }
            } else {
                tooth_no++;
                if ((tooth_no == 11) || (tooth_no == 27)) {
                    trig2cnt_last = trig2cnt;
                    trig2cnt = 0;
                } else if ((tooth_no == 16) || (tooth_no == 32)) {
                    // should now have two tooth group counts
                    if ((trig2cnt_last == 1) && (trig2cnt == 2)) {
                        tooth_no = 48;
                        synch |= SYNC_SYNCED | SYNC_FIRST; // is SYNC_FIRST desired?
                        synch &= ~SYNC_SEMI;
                    } else if ((trig2cnt_last == 3) && (trig2cnt == 2)) {
                        tooth_no = 0;
                        synch |= SYNC_SYNCED | SYNC_FIRST;
                        synch &= ~SYNC_SEMI;
                    } else if ((trig2cnt_last == 2) && (trig2cnt == 1)) {
                        tooth_no = 16;
                        synch |= SYNC_SYNCED | SYNC_FIRST;
                        synch &= ~SYNC_SEMI;
                    } else if ((trig2cnt_last == 3) && (trig2cnt == 1)) {
                        tooth_no = 32;
                        synch |= SYNC_SYNCED | SYNC_FIRST;
                        synch &= ~SYNC_SEMI;
                    } else {
                        outpc.syncreason = 66;
                        ign_reset();    // got out of sync, start again
                        return(0);
                    }
                    outpc.status1 |= STATUS1_SYNCFULL;
                } else if (tooth_no > 33) {
                    outpc.syncreason = 66;
                    ign_reset();    // got out of sync, start again
                    return(0);
                } else {
                    // have SEMI, but not finished sequence yet.
                    goto spkmode6_semi; // start calculating rpms and populating arrays
                }
            }
        } else {
            //while synced only use falling edge for further calcs
            if (!edge) {
                return(0);
                //ignore the rising edge for timing purposes, only count main edge as a tooth
            }
            // recheck for sync - only check crank
            if ((tooth_no == 32) || (tooth_no == 16) || (tooth_no == 64) || (tooth_no == 48)) {     // remove leading 1
                // see if we've got sync. Look for the +1/-1 tooth
                if ((tooth_diff_this + tooth_diff_last) > ((tooth_diff_last_1 + tooth_diff_last_2) << 1)) {
                    // only check crank
            		if (tooth_diff_last > (tooth_diff_this <<1)) {
                        if (tooth_no & 0x1f) { // not 0x20 or 0x00
                            outpc.syncreason = 61;
                            goto spkmode6_loss8;
                        } else if ((tooth_no == 64) || ((tooth_no == 32) && ((flagbyte5 & FLAGBYTE5_CAM) == 0)))  {
                            tooth_no = 0;
                        }
                    } else {
                        if ((tooth_no & 0xdf) != 0x10) { // not 0x30 or 0x10
                            outpc.syncreason = 62;
                            goto spkmode6_loss8;
                        }
                    }
        	    } else {
                    outpc.syncreason = 28;
    spkmode6_loss8:;
                    ign_reset();    // got out of sync
                    return(0);
                }
            }
        }
    } else {
        return(0);
    }

spkmode6_semi:;
    // be certain we stop here unless correct edge
    if (!edge) {
        return(0);
        //ignore the rising edge for timing purposes, only count falling edge as a tooth
    }
    //only do this on active edges
    goto common_wheel;

    /************ 36-2-2-2 mode *************/
  SPKMODE7: /* and SPKMODE57 */
    /* when CAM input enabled we check it during initial sync only
       This _could_ result in an incorrect phase if we got noise on
       the cam during initial sync and we won't detect it later.
     */
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            trig2cnt = 0;
            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing double tooth after a couple
        // of shorter ones

        if (!(synch & SYNC_SEMI)) {     // just starting
            if ((tooth_diff_this > (tooth_diff_last << 1))
                && (tooth_diff_this < (tooth_diff_last << 2))
                && (tooth_diff_this > (tooth_diff_last_1 << 1))
                && (tooth_diff_this < (tooth_diff_last_1 << 2))) {
                synch |= SYNC_SEMI;     // started sync sequence
                tooth_no = 0;
            }
            trig2cnt = 0;
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;
            if (tooth_no == 1) {
                if ((tooth_diff_this > (tooth_diff_last_1 << 1))
                    && (tooth_diff_this < (tooth_diff_last_1 << 2))) {
                    // compare against tooth before double missing
                    // found double tooth sequence, set tooth and go
                    if (spkmode == 57) {
                        /* On VVT engines, do not attempt to sync on cam pulse by double-missing
                         as it is indeterminate. */
                        tooth_no = 0;
                        trig2cnt = 0;
                        return(0);
                    }
                    if (num_cyl == 6) {
                        if (flagbyte5 & FLAGBYTE5_CAM) {
                            tooth_no = 0;
                            return (0);
                        } else {
                            tooth_no = 1;
                        }
                    } else { /* 2 rotor and 4-cyl */
                        if (flagbyte5 & FLAGBYTE5_CAM) {
                            outpc.status1 |= STATUS1_SYNCFULL;
                            if (trig2cnt) {
                                tooth_no = 17;
                            } else {
                                tooth_no = 47;
                            }
                        } else {
                            tooth_no = 17;
                        }
                    }
                    goto WHL36_2_2_2_OK;
                }

            } else if ((spkmode == 57) && (num_cyl == 4) && (tooth_no == 4)) {
                trig2cnt = 0;
                /* Second part of full-sync */
                return(0);

            } else if ((spkmode == 57) && (num_cyl == 4) && (tooth_no == 17)) {
                /* Third part of full-sync */
                if ((tooth_diff_this > (tooth_diff_last >> 1))
                   && (tooth_diff_this < (tooth_diff_last << 1))
                    && (tooth_diff_last > (tooth_diff_last_1 << 1))
                   && (tooth_diff_last < (tooth_diff_last_1 << 2))) {
                    outpc.status1 |= STATUS1_SYNCFULL;
                    if (trig2cnt) {
                        tooth_no = 47;
                    } else {
                        tooth_no = 17;
                    }
                    goto WHL36_2_2_2_OK;
                } else {
                    outpc.syncreason = 29;
                    ign_reset();
                }

            } else if ((tooth_diff_this > (tooth_diff_last << 1))
                       && (tooth_diff_this < (tooth_diff_last << 2))) {

                if (tooth_no == 13) { /* 2 rotor and 4-cyl */
                    if (spkmode == 57) {
                        /* First part of full-sync */
                        return(0);
                    } else {
                        if ((flagbyte5 & FLAGBYTE5_CAM) && (trig2cnt == 0)) {
                            outpc.status1 |= STATUS1_SYNCFULL;
                            tooth_no = 30;
                        } else {
                            tooth_no = 0;
                        }
                        goto WHL36_2_2_2_OK;
                    }

                } else if (tooth_no == 16) { /* 2 rotor and 4-cyl */
                    if (spkmode == 57) {
                        return(0);
                    }
                    if ((flagbyte5 & FLAGBYTE5_CAM) && (trig2cnt == 0)) {
                        outpc.status1 |= STATUS1_SYNCFULL;
                        tooth_no = 46;
//                    } else {
//                        tooth_no = 16;
                    }
                    goto WHL36_2_2_2_OK;

                } else if (tooth_no == 10) { /* 6 cyl */
                    if (flagbyte5 & FLAGBYTE5_CAM) {
                        outpc.status1 |= STATUS1_SYNCFULL;
                        if (trig2cnt == 0) {
                            tooth_no = 0;
                        } else {
                            tooth_no = 30;
                        }
                        trig2cnt = 0;
                    } else {
                        tooth_no = 0;
                    }
                    goto WHL36_2_2_2_OK;

                } else if (tooth_no == 19) { /* 6 cyl */
                    trig2cnt = 0;
                    if (flagbyte5 & FLAGBYTE5_CAM) {
                        tooth_no = 0;
                        return(0); /* indeterminate */
                    } else {
                        tooth_no = 20;
                    }
                    goto WHL36_2_2_2_OK;
                } else {
                    tooth_no = 0;
                    return(0);
                }
            } else if (tooth_no >= 30) {
                // PS Deviant track 9
                outpc.syncreason = 29;
                ign_reset();
            }
            return(0);
        }
      WHL36_2_2_2_OK:;

        synch &= ~SYNC_SEMI;
        synch |= SYNC_SYNCED;

    } else {
        // recheck for sync
        if (num_cyl == 6) {
            if ((tooth_no == 20) || (tooth_no == 30) || (tooth_no == 50) || (tooth_no == 60)) {     // (one less)
                /* double check cam pattern at and shortly after start */
                if ((cycle_deg == 7200) && ((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok))) {
                    if ( ((tooth_no == 60) && (trig2cnt != 0))
                        || ((tooth_no == 30) && (trig2cnt != 1))
                        ) {
                        outpc.syncreason = 97;
                        ign_reset();
                        return(0);
                    }
                }

                if (tooth_diff_this <= (tooth_diff_last << 1)) {
                    //fell over
                    if ((tooth_no == 20) || (tooth_no == 50)) {
                        outpc.syncreason = 30;
                    } else {
                        outpc.syncreason = 52;
                    }
                    ign_reset();
                    return(0);
                }
                trig2cnt = 0;
            }
        } else { /* 2 rotor or 4-cyl */
            if ((tooth_no == 16) || (tooth_no == 30) || (tooth_no == 46) || (tooth_no == 60)) {     // (one less)
                /* double check cam pattern at and shortly after start */
                if ((spkmode != 57) && (cycle_deg == 7200)
                    && ((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok))) {
                    if ((((tooth_no == 16) || (tooth_no == 60)) && (trig2cnt != 1))
                        || (((tooth_no == 30) || (tooth_no == 46)) && (trig2cnt != 0))) {
                        outpc.syncreason = 97;
                        ign_reset();
                        return(0);
                    }
                }

                if (tooth_diff_this <= (tooth_diff_last << 1)) {
                    //fell over
                    if ((tooth_no == 16) || (tooth_no == 46)) {
                        outpc.syncreason = 30;
                    } else {
                        outpc.syncreason = 52;
                    }
                    ign_reset();
                    return(0);
                }
                if (spkmode != 57) {
                    trig2cnt = 0;
                }
            } else if (spkmode == 57) {
                /* cam reset points */
                if ((tooth_no == 11) || (tooth_no == 41)) {
                    trig2cnt = 0;
                /* cam check points */
                } else if ((tooth_no == 19) || (tooth_no == 49)) {
                    if (((tooth_no == 19) && trig2cnt)
                        || ((tooth_no == 49) && (trig2cnt == 0))) {
                        outpc.syncreason = 52;
                        ign_reset();
                        return(0);
                    }
                    trig2cnt = 0;
                }
            }
        }
    }

    if (flagbyte5 & FLAGBYTE5_CAM) {
        if (tooth_no == 60) {
            tooth_no = 0;
        }
    } else {
        if (tooth_no == 30) {
            tooth_no = 0;
        }
    }

    goto common_wheel;

    /************ Subaru 6/7 mode *************/
  SPKMODE8:
    //  initial sync - wait for two crank teeth with at least one cam tooth in between
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last) || (trig2cnt == 0)) {
            trig2cnt = 0;
            return(0);             // only sync when there's enough data
        }
        // use cam data for cylinder ID and sequential
        if (cycle_deg == 7200) {
            // only sync on 2 or 3 cam teeth. Don't sync on single one as it has no phase info.
            if (trig2cnt > 2) {
                tooth_no = 0;
                outpc.status1 |= STATUS1_SYNCFULL;
                synch |= SYNC_SYNCED;
            } else if (trig2cnt > 1) {
                tooth_no = 6;
                outpc.status1 |= STATUS1_SYNCFULL;
                synch |= SYNC_SYNCED;
            }
        } else {
            if (trig2cnt > 1) {
                tooth_no = 0;
            } else {
                tooth_no = 3;
            }
            synch |= SYNC_SYNCED;
        }
        trig2cnt = 0;
    } else {
        // recheck for sync only on teeth 1 (6) and 4(3) and second rotation
        if (tooth_no == 6) {    // (last tooth we saw)
// new sync method, use tooth times to check for sync and check cam tooth numbers over two teeth. 
            if ((trig2cnt < 2) /* Must have at least one tooth in this period. */
                || ((!((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)))
                    && ((tooth_diff_this < tooth_diff_last) || (tooth_diff_this < tooth_diff_last_1)))
                ) { /* Only check tooth spacing after cranking. */
                //fell over
                outpc.syncreason = 20;
                ign_reset();
                return(0);
            } else {
                if (cycle_deg == 3600) {
                    tooth_no = 0;
                }
                trig2cnt = 0;
            }
        } else if (tooth_no == 12) {    // (last tooth we saw)
// new sync method, use tooth times to check for sync and check cam tooth numbers over two teeth. 
            if ((trig2cnt < 2) /* Must have at least one tooth in this period. */
                || ((!((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)))
                    && ((tooth_diff_this < tooth_diff_last) || (tooth_diff_this < tooth_diff_last_1)))
                ) { /* Only check tooth spacing after cranking. */
                //fell over
                outpc.syncreason = 20;
                ign_reset();
                return(0);
            } else {
                tooth_no = 0;
                trig2cnt = 0;
            }
        } else if (tooth_no == 3) {
            if ((trig2cnt != 1) /* Must have one tooth only in this period. */
                || ((!((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)))
                    && ((tooth_diff_this < tooth_diff_last) || (tooth_diff_this < tooth_diff_last_1)))
                ) {
                //fell over
                outpc.syncreason = 21;
                ign_reset();
                return(0);
            } else {
                trig2cnt = 0;
            }
        } else if ((tooth_no == 1) || (tooth_no == 4)) {
            trig2cnt = 0;
        }                       // don't reset trig2cnt on tooth 2 or tooth 5
    }

    goto common_wheel;

    /************ 99-05 Miata *************/
  SPKMODE9:
    //  initial sync - wait for two crank teeth with at least one cam tooth in between
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last) || (trig2cnt == 0)) {
            trig2cnt = 0;
            return(0);             // only sync when there's enough data
        }

        if (trig2cnt == 1) {
            tooth_no = 0;
        } else if (trig2cnt == 2) {
            tooth_no = 4;
        } else {
            trig2cnt = 0;
            return(0);             // unexpected number of 2nd triggers (noise?)
        }
        outpc.status1 |= STATUS1_SYNCFULL;
        synch |= SYNC_SYNCED;

    } else {
        // recheck for sync
        if (tooth_no == 4) {    // (last tooth we saw)
            if (trig2cnt < 1) { /* was <2 but allow Jap spec VVT to overlap this tooth*/
                outpc.syncreason = 31;
                ign_reset();
                return(0);
            }
        } else if (tooth_no == 8) {     // (last tooth we saw)
            if (trig2cnt == 0) {
                outpc.syncreason = 32;
                ign_reset();
                return(0);
            } else {
                tooth_no = 0;
            }
        }
    }

    trig2cnt = 0;

    goto common_wheel;

    /************ Mitsubishi 6g72 mode *************/
  SPKMODE10:
    //  initial sync - wait for two crank teeth
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            goto m6g72_exit;
        }
        // to allow events to fill before actual rpm declared
        synch |= SYNC_SEMI2;
        outpc.rpm = 1;

        if (!(synch & SYNC_SEMI)) {
            // using falling edge as active
            // until fully synced use "synthetic" tooth numbers
            if (!edge) {
                tooth_no = 2;   // for mainloop
            } else {
                tooth_no = 1;   // for mainloop
                if ((edge2) && (flagbyte1 & flagbyte1_trig2statl)) {
                    synch |= SYNC_SEMI; // found crank tooth with absent cam tooth
                    tooth_no = 5;       // actual 5 or 11.
                }
            }
            goto m6g72_exit;
        } else {
            tooth_no++;

            if (((tooth_no == 6) && (edge2)) || (tooth_no > 7)) {
                ign_reset();    // incorrect sequence, abort syncing
                goto m6g72_exit;
            }

            if (tooth_no == 7) {
                outpc.status1 |= STATUS1_SYNCFULL;
                if (!edge2) {
                    tooth_no = 0;
                    goto m6g72_sync;
                } else {
                    tooth_no = 6;
                    goto m6g72_sync;
                }
            }
        }
        //goto section only reached for exit
      m6g72_exit:
        flagbyte1 &= ~flagbyte1_trig2active;
        // store last 2nd trigger status
        if (edge2) {
            flagbyte1 |= flagbyte1_trig2statl;
        } else {
            flagbyte1 &= ~flagbyte1_trig2statl;
        }
        return(0);                 // only sync when there's enough data

        //continue here if synced
      m6g72_sync:
        synch |= SYNC_SYNCED;
        synch &= ~SYNC_SEMI;

    } else {
        // recheck for sync
        if (tooth_no == 2) {    // (last tooth we saw)
            // cam must be high now and low on last edge
            if ((edge2) || (!(flagbyte1 & flagbyte1_trig2statl))) {
                outpc.syncreason = 33;
                ign_reset();
                return(0);
            }
        } else if (tooth_no == 4) {     // (last tooth we saw)
            // cam must be low now and low on last edge
            if (!(edge2) || (!(flagbyte1 & flagbyte1_trig2statl))) {
                outpc.syncreason = 34;
                ign_reset();
                return(0);
            }
        }

    }

    // store last 2nd trigger status
    if (edge2) {
        flagbyte1 |= flagbyte1_trig2statl;
    } else {
        flagbyte1 &= ~flagbyte1_trig2statl;
    }

    if (tooth_no == 12) {
        tooth_no = 0;
    }

    goto common_wheel;

    /************ IAW Weber-Marelli *************/
    /* As used on Sierra Cosworth, some Fiats, Lancia */
SPKMODE11:
    if (!(synch & SYNC_SYNCED)) {
	    if (!(synch & SYNC_SEMI)) {
	        if ( (!tooth_diff_this) || (!tooth_diff_last) || (!trig2cnt) ) {
		        trig2cnt = 0;
		        return(0);
	        }
	        synch |= SYNC_SEMI;
	        tooth_no = 0;
	    } else {
            tooth_no++;
	        if (trig2cnt == 0 ) {
    		    return(0); // wait until we see a second cam tooth
	        }
            if (tooth_no == 2) {
                goto SM11_SYNC;
            } else if (tooth_no == 6) {
                tooth_no = 0;
                goto SM11_SYNC;
            } else {
                outpc.syncreason = 35;
		        ign_reset();
        		return(0);
            }
SM11_SYNC:;
            outpc.status1 |= STATUS1_SYNCFULL;
	        synch |= SYNC_SYNCED;
            synch &= ~SYNC_SEMI;
	    }
    } else {
	    // recheck for sync
	    if (tooth_no == 8) {   // (last tooth we saw)
	        if (trig2cnt == 0) {
                outpc.syncreason = 35;
		        ign_reset();
        		return(0);
	        } else {
        		tooth_no = 0;
	        }
	    }
    }

    trig2cnt = 0;

    goto common_wheel;
    /************ CAS 4/1 mode *************/
  SPKMODE12:
    //  initial sync - wait for second trigger
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last) || (!edge)
            || (!(flagbyte1 & flagbyte1_trig2active))) {
            if (edge) {
                flagbyte1 &= ~flagbyte1_trig2active;
            }
            return(0);
        }
        tooth_no = 0;
        outpc.status1 |= STATUS1_SYNCFULL;
        synch |= SYNC_SYNCED;

    } else {
        // recheck for sync
        if (tooth_no == 8) {
            if ((!edge) || (!(flagbyte1 & flagbyte1_trig2active))) {
                outpc.syncreason = 36;
                ign_reset();
                return(0);
            } else {
                tooth_no = 0;   // OK, restart sequence
            }
        }
    }

    if (edge) {
        flagbyte1 &= ~flagbyte1_trig2active;
    }

    goto common_wheel;
    /************ 4G63 (CAS 4/2) mode *************/
    // Actually M1 (NA) Miata 89-97
  SPKMODE13:
    // CAS 4/2 mode tied into "Miata" (and others?) trigger disc
    // expects correct timing, trigger angle can be tweaked, but should be around 10BTDC
    // Falling edge of crank signal is "edge" triggers on both edges
    // 2nd trig ISR not actually used, code here polls the pin on appropriate crank edge

    if (!(synch & SYNC_SYNCED)) {
        // For sync first look for rising edge of crank when cam is high
        if (!(synch & SYNC_SEMI)) {
            if ((!tooth_diff_this) || (!tooth_diff_last)) {
                return(0);
            }
            synch |= SYNC_SEMI2;
            outpc.rpm = 1;      // bogus low rpm
            // this should allow the code to fill advance tables before we declare sync
            if (edge) {
                tooth_no = 2;
                return(0);
            } else {
                tooth_no = 1;
            }
            // check if crank and cam the same without checking edge trigger settings
//            if ((utmp13 == (TFLG_trig2 | 0x01)) || (utmp13 == 0)) {
            if (edge == edge2) {
                synch |= SYNC_SEMI;
            } else {
                return(0);
            }
       } else {
            // have semi synced
            if (!edge) {
                // something went wrong, should be "edge"
                outpc.syncreason = 37;
                ign_reset();
                return(0);
            }

            // crank is now low..
            // if same then this tooth is 4, otherwise 8
            if (edge == edge2) {
                tooth_no = 3;
            } else {
                tooth_no = 7;
            }
            outpc.status1 |= STATUS1_SYNCFULL;
            synch |= SYNC_SYNCED;
            synch &= ~SYNC_SEMI;
            synch &= ~SYNC_SEMI2;
        }
    } else {
        // recheck for sync
        if (tooth_no == 3) {
            //if not an active falling edge or cam and crank are different then fail
            if ((!edge) || (edge != edge2)) {
                outpc.syncreason = 38;
                ign_reset();
                return(0);
            }
        } else if (tooth_no == 7) {
            //if not an active falling edge or cam and crank are same then fail
            if ((!edge) || (edge == edge2)) {
                outpc.syncreason = 39;
                ign_reset();
                return(0);
            }
        } else if (tooth_no == 8) {
            tooth_no = 0;       // restart sequence
        }
    }

    goto common_wheel;
    /************ Twin trigger (bike) mode *************/
  SPKMODE14:
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            tooth_no = 0;
            synch &= ~SYNC_SEMI;
            return(0);             // only sync when there's enough data
        }
        if (!(synch & SYNC_SEMI)) {
            if (flagbyte4 & flagbyte4_tach2) {  // flag indicating we arrived via 2nd tach input
                tooth_no = 2;   // normally 0, but use 2 for this syncing
            } else {
                tooth_no = 1;
            }
            synch |= SYNC_SEMI; // we force ourselves to check for both trigger inputs
            return(0);
        } else {
            // seen one trigger, now check it is the other, else problems
            if (flagbyte4 & flagbyte4_tach2) {
                if (tooth_no == 2) {
                    outpc.syncreason = 40;
                    ign_reset();
                    return(0);
                }
            } else {
                if (tooth_no == 1) {
                    outpc.syncreason = 41;
                    ign_reset();
                    return(0);
                }
            }
            synch &= ~SYNC_SEMI;
            synch |= SYNC_SYNCED;
        }
    } else {
        // re-check phasing is correct (could occur due to miswiring)

        // arranging like this instead of one long "if" generates smaller asm
        if (flagbyte4 & flagbyte4_tach2) {
            if (tooth_no == 2) {
                outpc.syncreason = 40;
                ign_reset();
                return(0);
            }
        } else {
            if (tooth_no == 1) {
                outpc.syncreason = 41;
                ign_reset();
                return(0);
            }
        }
    }

    if (tooth_no == 2) {
        tooth_no = 0;
    }

    goto common_wheel;
    /************ Chrysler 2.2/2.5 *************/
SPKMODE15:
    if (!(synch & SYNC_SYNCED)) {
        if (tooth_diff_this && tooth_diff_last && tooth_diff_last_1) {
            unsigned long t;
            t = tooth_diff_last << 1; // 2x
            if ((tooth_diff_this > t) && (tooth_diff_last_1 > t) // tooth edge after gap
                && (tooth_diff_this > tooth_diff_last_1)) { // correct polarity, so (_last + _last_1) ~= _this 
    	        tooth_no = 1;
    	        synch |= SYNC_SYNCED;
            } else {
                return(0);
            }
	    } else {
            return(0);
        }
    } else {
	    // recheck for sync
	    if (tooth_no == 1) {
            unsigned long t;
            t = tooth_diff_last << 1; // 2x
            if ((tooth_diff_this < t) || (tooth_diff_last_1 < t)) {
                outpc.syncreason = 42;
	        	ign_reset();
	        	return(0);
	        }
	    } else if (tooth_no == 5) {
            tooth_no = 0;
        }
    }

    goto common_wheel;
    /************ Renix 44-2-2 *************/
  SPKMODE16:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
	    if  ((!tooth_diff_this) || (!tooth_diff_last) ) {
            trig2cnt = 0;
	        return(0); // only sync when there's enough data
	    } else if ((cycle_deg == 7200) && (trig2cnt == 0)) {
            return(0); // if W/S, COP or use-cam, need a cam pulse too
        }

	    // when unsynced we wait until we see a missing double tooth
	    if (tooth_diff_this > (tooth_diff_last<<1)) {
	        tooth_no = 0;
	        synch |= SYNC_SYNCED;
            if (cycle_deg == 7200) {
                outpc.status1 |= STATUS1_SYNCFULL;
            }
	    } else {
	        return(0);
	    }

        } else {
	    // recheck for sync
	    if ((tooth_no == 20) || (tooth_no == 40) || (tooth_no == 60) || (tooth_no == 80) || (tooth_no == 100) || (tooth_no == 120)) {
	        if (tooth_diff_this <= (tooth_diff_last<<1)) {
                outpc.syncreason = 43;
		        ign_reset();
		        return(0);
	        } else {
                if (tooth_no == last_tooth) {
                    if ((trig2cnt) || (cycle_deg == 3600)) {
                		tooth_no = 0;
                    } else {
                        // where's my cam
                        outpc.syncreason = 43;
		                ign_reset();
		                return(0);
                    }
                } else {
                    trig2cnt = 0;
                }
	        }
	    }
    }

    goto common_wheel;
    /************ Suzuki swift *************/
  SPKMODE17:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            return(0);             // only sync when there's enough data
        }
        // look for tooth longer than either of last ones
        if ((tooth_diff_this > tooth_diff_last)
            && (tooth_diff_this > tooth_diff_last_1)) {
            tooth_no = 0;
            synch |= SYNC_SYNCED;
        } else {
            return(0);
        }

    } else {
        // recheck for sync
        if (tooth_no == 6) {    // (one less)
            if ((tooth_diff_this < tooth_diff_last)
                || (tooth_diff_this < tooth_diff_last_1)) {
                outpc.syncreason = 44;
                ign_reset();
                return(0);
            } else {
                tooth_no = 0;
            }
        }
    }

    goto common_wheel;

    /************ Suzuki vitara 2.0 *************/
  SPKMODE18:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            return(0);             // only sync when there's enough data
        }
        if (!(synch & SYNC_SEMI)) {
            // look for short tooth after two long ones
            if (((tooth_diff_this + (tooth_diff_this >> 1)) <
                 tooth_diff_last)
                && ((tooth_diff_this + (tooth_diff_this >> 1)) <
                    tooth_diff_last_1)) {
                tooth_no = 0;
                synch |= SYNC_SEMI;
            } else {
                return(0);
            }
        } else {
            // semi synced, wait a few teeth
            tooth_no++;
            if (tooth_no < 2) {
                return(0);
            }
            if ((tooth_diff_this + (tooth_diff_this >> 1)) <
                tooth_diff_last) {
                tooth_no = 2;
                synch |= SYNC_SYNCED;
                synch &= ~SYNC_SEMI;
            } else if (tooth_diff_this >
                       (tooth_diff_last + (tooth_diff_last >> 2))) {
                tooth_no = 8;
                synch |= SYNC_SYNCED;
                synch &= ~SYNC_SEMI;
            } else {
                // failed to sync for some reason
                ign_reset();
                return(0);
            }
        }

    } else {
        // recheck for sync
        if (tooth_no == 11) {   // (one less)
            if ((tooth_diff_this > tooth_diff_last)
                || (tooth_diff_this > tooth_diff_last_1)) {
                outpc.syncreason = 45;
                ign_reset();
                return(0);
            } else {
                tooth_no = 0;
            }
        } else if (tooth_no == 6) {     // (one less)
            if ((tooth_diff_this > tooth_diff_last)
                || (tooth_diff_this > tooth_diff_last_1)) {
                outpc.syncreason = 46;
                ign_reset();
                return(0);
            }
        }
    }

    goto common_wheel;

    /************ Daihatsu 3 cyl *************/
  SPKMODE19:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            return(0);             // only sync when there's enough data
        }
        //look for short tooth gap - this is tooth no.1
        ltmp1 = tooth_diff_this << 1;
        if ((tooth_diff_last > ltmp1) && (tooth_diff_last_1 > ltmp1)) {
            tooth_no = 0;
            outpc.status1 |= STATUS1_SYNCFULL;
            synch |= SYNC_SYNCED;
        } else {
            return(0);
        }

    } else {
        // recheck for sync
        if (tooth_no == 4) {    // (one less)
            ltmp1 = tooth_diff_this << 1;
            if ((tooth_diff_last > ltmp1) && (tooth_diff_last_1 > ltmp1)) {
                tooth_no = 0;
            } else {
                outpc.syncreason = 47;
                ign_reset();
                return(0);
            }
        }
    }

    goto common_wheel;

    /************ Daihatsu 4 cyl *************/
  SPKMODE20:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            return(0);             // only sync when there's enough data
        }
        //look for short tooth gap - this is tooth no.1
        ltmp1 = tooth_diff_this << 1;
        if ((tooth_diff_last > ltmp1) && (tooth_diff_last_1 > ltmp1)) {
            tooth_no = 0;
            outpc.status1 |= STATUS1_SYNCFULL;
            synch |= SYNC_SYNCED;
        } else {
            return(0);
        }

    } else {
        // recheck for sync
        if (tooth_no == 5) {    // (one less)
            ltmp1 = tooth_diff_this << 1;
            if ((tooth_diff_last > ltmp1) && (tooth_diff_last_1 > ltmp1)) {
                tooth_no = 0;
            } else {
                outpc.syncreason = 48;
                ign_reset();
                return(0);
            }
        }
    }

    goto common_wheel;

    /************ Honda VTR1000 *************/
  SPKMODE21:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            return(0);             // only sync when there's enough data
        }
        //look for long tooth gap - this is tooth no.1
        ltmp1 = tooth_diff_this << 1;
        if (tooth_diff_this > (tooth_diff_last << 1)) {
            tooth_no = 0;
            synch |= SYNC_SYNCED;
        } else {
            return(0);
        }

    } else {
        // recheck for sync
        if (tooth_no == 9) {    // (one less)
            if (tooth_diff_this > (tooth_diff_last << 1)) {
                tooth_no = 0;
            } else {
                outpc.syncreason = 49;
                ign_reset();
                return(0);
            }
        }

    }

    goto common_wheel;

    /************ Rover 36-1-1 mode *************/
  SPKMODE22:
//initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing tooth
        temp1 = tooth_diff_last + (tooth_diff_last >> 1);       // 1.5*
        if (tooth_diff_this > temp1) {
            tooth_no = 0;
            synch |= SYNC_SYNCED;
        } else {
            return(0);
        }

    } else {
        // recheck for sync - revised method
        // this now does the calc on every tooth - previously only did it on tooth 17
        temp1 = tooth_diff_last + (tooth_diff_last >> 1);       // 1.5*
        if (tooth_diff_this > temp1) {  // missing tooth
            if (tooth_no != 17) {       // we are expecting tooth no. 17 (one less)
                //fell over
//                ign_reset(); // old method - one shot and you are out
                // be more tolerant
                syncerr++;
                outpc.synccnt++;
                return(0);
            }
            tooth_no = 0;
        }
    }

    goto common_wheel;

    /************ Rover 36-1-1-1-1 mode2 (EU3) *************/
    // used on SPI Mini
  SPKMODE23:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            flagbyte1 &= ~flagbyte1_trig2active;
            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing tooth and then another one
        // and count the teeth in-between

        if (!(synch & SYNC_SEMI)) {     // just starting
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;     // started sync sequence
            }
            flagbyte1 &= ~flagbyte1_trig2active;        // assume that cam tooth must occur between missing tooth segments
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;
            if (!(synch & SYNC_SEMI2)) {        // just starting
                temp1 = tooth_diff_last + (tooth_diff_last >> 1);       // 1.5*
                if (tooth_diff_this > temp1) {
                    if (tooth_no == 2) {
                        tooth_no = 0;   // tooth 1
                        goto WHL_ROV2_OK;
                    } else if (tooth_no == 3) {
                        tooth_no = 17;  // tooth 18
                        goto WHL_ROV2_OK;
                    } else if (tooth_no == 13) {
                        tooth_no = 30;  // tooth 15
                        goto WHL_ROV2_OK;
                    } else if (tooth_no == 14) {
                        tooth_no = 14;  // tooth 31
                        goto WHL_ROV2_OK;
                    } else {
                        tooth_no = 0;   // doesn't make sense, so try again from this missing tooth
                    }
                }
                return(0);
            }
        }
      WHL_ROV2_OK:

        synch &= ~SYNC_SEMI;
        synch &= ~SYNC_SEMI2;
        synch |= SYNC_SYNCED;
        if (cycle_deg == 7200) {      // COP or use-cam - seems weak phase detection?
            if (flagbyte1 & flagbyte1_trig2active) {
                tooth_no += 32;
                flagbyte1 &= ~flagbyte1_trig2active;
            }
        }

    } else {
        // recheck for sync in normal running
        if ((tooth_no == 14) || (tooth_no == 17) || (tooth_no == 30) || (tooth_no == 32) || (tooth_no == 46) || (tooth_no == 49) || (tooth_no == 62) || (tooth_no == 64)) {     // (one less)
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this <= temp1) {
                outpc.syncreason = 22;
                ign_reset();
                return(0);
            } else {
                // sync recheck passed, now if in cam/cop mode, see if we are on right phase
                // intended to allow us to sync on wrong phase and 'fix' it soon afterwards
                // ought to start on wasted cop too, but not implemented yet
                if (cycle_deg == 7200) {      // COP or use-cam
                    if (tooth_no < 33) {
                        if (flagbyte1 & flagbyte1_trig2active) {
                            tooth_no += 32;  // fix phase
                        } else {
                            outpc.status1 |= STATUS1_SYNCFULL;
                        }                            
                    } else {
                        if (flagbyte1 & flagbyte1_trig2active) {
                            outpc.status1 |= STATUS1_SYNCFULL;
                        }
                    }
                }
            }
            if (tooth_no == 32) {
                if (cycle_deg == 3600) {   // !COP or use-cam
                    tooth_no = 0;
                }
            } else if (tooth_no == 64) {
                tooth_no = 0;
            }
            flagbyte1 &= ~flagbyte1_trig2active;
        }
    }
    goto common_wheel;

    /************ Rover 36-1-1-1-1 mode3 - do not know application *************/
  SPKMODE24:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing tooth and then another one
        // and count the teeth in-between

        if (!(synch & SYNC_SEMI)) {     // just starting
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;     // started sync sequence
            }
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;
            if (!(synch & SYNC_SEMI2)) {        // just starting
                temp1 = tooth_diff_last + (tooth_diff_last >> 1);       // 1.5*
                if (tooth_diff_this > temp1) {
                    if (tooth_no == 4) {
                        tooth_no = 0;   // tooth 1
                        goto WHL_ROV3_OK;
                    } else if (tooth_no == 5) {
                        tooth_no = 16;  // tooth 17
                        goto WHL_ROV3_OK;
                    } else if (tooth_no == 11) {
                        tooth_no = 11;  // tooth 12
                        goto WHL_ROV3_OK;
                    } else if (tooth_no == 12) {
                        tooth_no = 28;  // tooth 29
                        goto WHL_ROV3_OK;
                    } else {
                        tooth_no = 0;   // doesn't make sense, so try again from this missing tooth
                    }
                }
                return(0);
            }
        }
      WHL_ROV3_OK:

        synch &= ~SYNC_SEMI;
        synch &= ~SYNC_SEMI2;
        synch |= SYNC_SYNCED;

    } else {
        // recheck for sync
        if ((tooth_no == 11) || (tooth_no == 16) || (tooth_no == 28) || (tooth_no == 32)) {     // (one less)
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this <= temp1) {
                outpc.syncreason = 50;
                ign_reset();
                return(0);
            }
            if (tooth_no == 32) {
                tooth_no = 0;
            }
        }
    }
    goto common_wheel;

    /************ GM 7X native *************/
  SPKMODE25:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            return(0);             // only sync when there's enough data
        }
        /* look for long tooth following short tooth, should be more resistant to crank speedup issues */
        ltmp1 = (tooth_diff_last << 1) + tooth_diff_last; /* comparing 10 deg with 50 and 60deg, so x3 */
        if ((tooth_diff_this > ltmp1) && (tooth_diff_last_1 > ltmp1)) {
            if (cycle_deg == 7200) {
                synch |= (SYNC_SYNCED | SYNC_SEMI2);
                if ((ram4.spk_mode3 & 0xe0) == 0x80) { // COP mode
                    synch |= SYNC_WCOP; // enable WCOP mode
                }
                ls1_sl = 0; // vars to record sync process
                ls1_ls = 0;
            } else {
                tooth_no = 0;
            }
            synch |= SYNC_SYNCED;
        } else {
            return(0);
        }

    } else {
        // recheck for sync
        if ((tooth_no == 6) || (tooth_no == 13)) {    // (one less)
            ltmp1 = tooth_diff_this << 1;
            if (!((tooth_diff_last > ltmp1)
                  && (tooth_diff_last_1 > ltmp1))) {
                outpc.syncreason = 51;
                ign_reset();
                return(0);
            }
        } else if (tooth_no == last_tooth) {
            tooth_no = 0;
        }

        if (cycle_deg == 7200) {
            /* Poll level of cam at normal tooth after the +1 pair */
            if ((tooth_no == 7) || (tooth_no == 0)) {

                if ((synch & SYNC_SEMI2) || (synch & SYNC_WCOP)) {
                    ls1_sl++;
                    ls1_ls <<= 2;
                    /* Poll level, set cam bit in place of interrupt setbit */
                    if (ram4.spk_config & 0x01) {      // high cam for phase 1
                        if ((PORTT & TFLG_trig2) == 0) {        // low so phase 2
                            ls1_ls |= 2;
                        } else {
                            ls1_ls |= 1;
                        }
                    } else {
                        if (PORTT & TFLG_trig2) {
                            ls1_ls |= 2;
                        } else {
                            ls1_ls |= 1;
                        }
                    }
                }

                if (synch & SYNC_SEMI2) {
                    /* SEMI2 here means that we've synced to the crank, but have
                        yet to confirm cam sync. We are still running wasted COP and
                        are looking to match an on/off/on/off cam pattern. */

                    if ((ls1_sl > 3) && ((ls1_ls & 0xff) == 0x99)) { // i.e. 10011001
                        synch &= ~SYNC_SEMI2;        // found cam, stop WCOP of dwells
                        SSEM0;
                        dwellsel = 0; // have to cancel any dwells in rare case there is one
                        dwellq[0].sel = 0;
                        dwellq[1].sel = 0;
                        CSEM0;
                        tooth_no = 7; // declare we really are at the 7th tooth
                        outpc.status1 |= STATUS1_SYNCFULL;
                        syncfirst(); // re-calculate next triggers to prevent gap
                    } else if (ls1_sl > 10) {
                        /* Failed to receive matching pattern, cam input faulty */
                        /* 10 revs allowance on normal electrical cam sensor */
                        synch &= ~(SYNC_SEMI2 | SYNC_WCOP);
                        ls1_sl = 99; // lock here
                        if ((ram4.spk_mode3 & 0xe0) == 0x80) { // COP mode
                            synch |= SYNC_WCOP2; // force WCOP mode
                        }
                        flagbyte17 |= FLAGBYTE17_STATCAM; //cam fault
                        outpc.cel_status |= 0x800;
                    }
                } else if (synch & SYNC_WCOP) {
                    /* WCOP was left on for one rev to ensure that any remaining spark events were cleared
                        to prevent the possibility of spark hang-on */
                    synch &= ~SYNC_WCOP;
                }
            }
        }
    }
    goto common_wheel;

        /************  Nissan QR25DE *************/
        /* This is 36-2-2 with a number of cam notches for id */
        /* find a missing tooth, declare SEMI, count cam teeth then SYNC */
#define MODE28CHKTOOTH 10 // 7 for stim, 10 for engine
  SPKMODE28:
//initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            return(0);             // only sync when there's enough data
        }
        if (!(synch & SYNC_SEMI)) {
            // when unsynced we wait until we see a missing double tooth
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;
                trig2cnt = 0;
            } else {
                return(0);
            }
        } else {
            // we've seen one missing tooth, now look for the second
            tooth_no++;
        	if (num_cyl == 4) { /* old way confirmed to work */
                if (tooth_no == 16) {
                    temp1 = tooth_diff_last << 1;       // *2
                    if (tooth_diff_this > temp1) {
                        // NOTE! Only care about the cam this once
                        //these tooth numbers are made up
                        if (trig2cnt == 1) {
                            tooth_no = 0;
                        } else if (trig2cnt == 3) {
                            tooth_no = 16;
                        } else if (trig2cnt == 4) {
                            tooth_no = 32;
                        } else if (trig2cnt == 2) {
                            tooth_no = 48;
                        } else {
                            //failed to get cam sync correctly
                            goto mode28_semi_fail4;
                        }
                        outpc.status1 |= STATUS1_SYNCFULL;
                        synch |= SYNC_SYNCED;
                        synch &= ~SYNC_SEMI;

                        trig2cnt = 0;
                    } else {
    mode28_semi_fail4:;
                        // went wrong
                        outpc.syncreason = 63;
                        ign_reset();
                        return(0);
                    }
                } else {
                    return(0); // not there yet
                }
            } else { /* 8 cyl different way - untested */
                if (tooth_no == MODE28CHKTOOTH) {
                    trig2cnt = 0;
                } else if (tooth_no == 16) {
                    temp1 = tooth_diff_last << 1;       // *2
                    if (!(tooth_diff_this > temp1)) {
    mode28_semi_fail:;
                        // went wrong
                        outpc.syncreason = 63;
                        ign_reset();
                        return(0);
                    }
                } else if (tooth_no == (16 + MODE28CHKTOOTH)) { // tooth after the second gap
                        if (trig2cnt == 1) {
                            tooth_no = 32 + MODE28CHKTOOTH;
                        } else if (trig2cnt == 3) {
                            tooth_no = 48 + MODE28CHKTOOTH;
                        } else if (trig2cnt == 4) {
                            tooth_no = 0 + MODE28CHKTOOTH;
                        } else if (trig2cnt == 2) {
                            tooth_no = 16 + MODE28CHKTOOTH;
                        } else {
                            //failed to get cam sync correctly
                            goto mode28_semi_fail;
                        }
                        outpc.status1 |= STATUS1_SYNCFULL;
                        synch |= SYNC_SYNCED;
                        synch &= ~SYNC_SEMI;
                } else {
                    return(0); // not there yet
                }
            }
        }
    } else {
        //re-sync on crank only. No need to re-check cam.
        if ((tooth_no == 16) || (tooth_no == 32) || (tooth_no == 48) || (tooth_no == 64)) {
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this > temp1) {
                if (tooth_no == 64) {
                    tooth_no = 0;
                }
            } else {
                outpc.syncreason = 63;
                ign_reset();
                return(0);
            }
        } else if ((tooth_no == 26) || (tooth_no == 42) || (tooth_no == 58) || (tooth_no == 10)) { // VVT reset points
            trig2cnt = 0;
        }
    }
    goto common_wheel;

        /************  Honda RC-51  *************/
  SPKMODE29:
    //  initial sync - wait for two crank teeth with at least one cam tooth in between
    if (!(synch & SYNC_SYNCED)) {
        if (!(synch & SYNC_SEMI)) {
            if ((!tooth_diff_this) || (!tooth_diff_last)
                || (!(flagbyte1 & flagbyte1_trig2active))) {
                flagbyte1 &= ~flagbyte1_trig2active;
                return(0);         // only sync when there's enough data
            }
            // just found the first cam pulse
            synch |= SYNC_SEMI;
            tooth_no = 0;
            flagbyte1 &= ~flagbyte1_trig2active;
            return(0);
        } else {
            tooth_no++;
            if (!(flagbyte1 & flagbyte1_trig2active)) {
                return(0);
            }
            // now we've had two cam pulses
            if (tooth_no == 10) {
                tooth_no = 16;
                goto rc51_ok;
            } else if (tooth_no == 2) {
                tooth_no = 18;
                goto rc51_ok;
            } else if (tooth_no == 12) {
                tooth_no = 6;
                goto rc51_ok;
            } else {
                outpc.syncreason = 53;
                ign_reset();
                return(0);
            }
        }
      rc51_ok:
        outpc.status1 |= STATUS1_SYNCFULL;
        synch &= ~SYNC_SEMI;
        synch |= SYNC_SYNCED;
    } else {
        if ((tooth_no == 6) || (tooth_no == 16) || (tooth_no == 18)) {
            if (!(flagbyte1 & flagbyte1_trig2active)) {
                if (tooth_no == 6) {
                    outpc.syncreason = 54;
                } else if (tooth_no == 16) {
                    outpc.syncreason = 55;
                } else {
                    outpc.syncreason = 56;
                }
                ign_reset();
                return(0);
            }
        } else if (tooth_no == 24) {
            tooth_no = 0;
        }
    }

    flagbyte1 &= ~flagbyte1_trig2active;

    goto common_wheel;
        /************  Fiat 1.8 16V  *************/
  SPKMODE30:
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last) || (trig2cnt == 0)) {      // wait for at least two teeth and a cam transition
            trig2cnt = 0;
            return(0);
        }
        if (trig2cnt == 1) {
            if (edge2) {
                tooth_no = 9;
            } else {
                tooth_no = 3;
            }
        } else {                // must be 2
            if (edge2) {
                tooth_no = 0;
            } else {
                tooth_no = 6;
            }
        }
        outpc.status1 |= STATUS1_SYNCFULL;
        synch |= SYNC_SYNCED;
    } else {
        if (tooth_no == 12) {
            if (trig2cnt == 2) {
                tooth_no = 0;
            } else {
                outpc.syncreason = 57;
                ign_reset();
                return(0);
            }
        } else if ((tooth_no == 3) && (trig2cnt != 1)) {
            outpc.syncreason = 58;
            ign_reset();
            return(0);
        } else if ((tooth_no == 6) && (trig2cnt != 2)) {
            outpc.syncreason = 59;
            ign_reset();
            return(0);
        } else if ((tooth_no == 9) && (trig2cnt != 1)) {
            outpc.syncreason = 60;
            ign_reset();
            return(0);
        }
    }
    trig2cnt = 0;
    goto common_wheel;
    /************ CAS 360 *************/
    /* special wheel
     * cam signal handled by XGATE which causes an interrupt to here every 6 degrees
     * crank signal only used for initial sync and re-sync
     * Also used by flywheel tooth mode - Flywheel tri-tach
     */
SPKMODE_CAS360:
    if (tooth_no >= no_teeth) {
        tooth_no = 0;
    }
    if (synch & SYNC_SYNCED) {
        outpc.status1 |= STATUS1_SYNCFULL;
    }
    goto common_wheel;

    /************ GM LS1 *************/
  SPKMODE40:
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last) ) {      // wait for at least two teeth
            return(0);
        }
        
        if (!(synch & SYNC_SEMI)) {
//            if (edge || (tooth_diff_this > (tooth_diff_last << 1)) || ((tooth_diff_this << 1) < tooth_diff_last)) {
            if (edge || (tooth_diff_this > (tooth_diff_last + (tooth_diff_last >> 1)))
                || ((tooth_diff_this + (tooth_diff_this >> 1)) < tooth_diff_last)) {
                // bail if falling edge or teeth are not similar
                return(0);
            } else {
                // have no found two similar (equal) sized teeth
                synch |= SYNC_SEMI;
                ls1_sl = 0;
                ls1_ls = 0;
                ls1_sl_last = 0;
                ls1_ls_last = 0;
            } 
        } else {
            // semi code
            /* two versions - either 2.0:1 or 1.5:1 discrimination */
            if (edge) { // count on falling
//                if (tooth_diff_this > (tooth_diff_last << 1)) {
                if (tooth_diff_this > ((tooth_diff_last + tooth_diff_last + tooth_diff_last) >> 1)) { // is tdt > tdl*1.5
                    ls1_sl++;
//                } else if ((tooth_diff_this << 1) < tooth_diff_last) {
                } else if (((tooth_diff_this + tooth_diff_this + tooth_diff_this) >> 1) < tooth_diff_last) { // is tdt * 1.5 < tdl
                    ls1_ls++;
                } else {
                    synch &= ~SYNC_SEMI; // invalid pattern, start again
                }
                return(0);
            } else { // check on rising
//                if ((tooth_diff_this > (tooth_diff_last << 1)) 
//                    || ((tooth_diff_this << 1) < tooth_diff_last)) {
                if ((tooth_diff_this > (tooth_diff_last + (tooth_diff_last >> 1)))
                    || ((tooth_diff_this + (tooth_diff_this >> 1)) < tooth_diff_last)) {
                    return(0);
                } else {
                    // equal sized - check for sync
                    if ((ls1_sl_last == 0) && (ls1_ls_last == 0)) {
                        goto SPKMODE40_X;
                    } else if ((ls1_sl == 5) && (ls1_ls == 0) && (ls1_sl_last == 0) && (ls1_ls_last == 5)) {
                        tooth_no = 4;
                        goto SPKMODE40_COM;
                    } else if ((ls1_sl == 3) && (ls1_ls == 0) && (ls1_sl_last == 0) && (ls1_ls_last == 1)) {
                        tooth_no = 8;
                        goto SPKMODE40_COM;
                    } else if ((ls1_sl == 0) && (ls1_ls == 2) && (ls1_sl_last == 3) && (ls1_ls_last == 0)) {
                        tooth_no = 10;
                        goto SPKMODE40_COM;
                    } else if ((ls1_sl == 2) && (ls1_ls == 0) && (ls1_sl_last == 0) && (ls1_ls_last == 2)) {
                        tooth_no = 12;
                        goto SPKMODE40_COM;
                    } else if ((ls1_sl == 0) && (ls1_ls == 3) && (ls1_sl_last == 2) && (ls1_ls_last == 0)) {
                        tooth_no = 15;
                        goto SPKMODE40_COM;
                    } else if ((ls1_sl == 0) && (ls1_ls == 5) && (ls1_sl_last == 1) && (ls1_ls_last == 0)) {
                        tooth_no = 23;
SPKMODE40_COM:;
                        synch |= SYNC_SYNCED;
                        synch &= ~SYNC_SEMI;
                        if (flagbyte5 & FLAGBYTE5_CAM) {
                            outpc.status1 |= STATUS1_SYNCFULL;
                            if (tooth_no != 23) { // this one is just after the cam phase change
                                if (!edge2) {
                                    tooth_no += 24;
                                }
                            } else {
                                if (edge2) {
                                    tooth_no += 24;
                                }
                            }
                        }
                    } else {
SPKMODE40_X:;
                        // invalid tooth count, try again
                        ls1_sl_last = ls1_sl;
                        ls1_ls_last = ls1_ls;
                        ls1_sl = 0;
                        ls1_ls = 0;
                    }
                }
            }
        }
        edge2_last = edge2;

        return(0);
    } else {
        /* Validate CAM during cranking if using it. This should throw out mis-syncs. */
        if ((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)) {
            if (edge && (flagbyte5 & FLAGBYTE5_CAM) && (edge2 != edge2_last)) {
                /* had a cam transition */
                if ((tooth_no != 21) && (tooth_no != 22) && (tooth_no != 45) && (tooth_no != 46)) {
                    outpc.syncreason = 91;
                    ign_reset();
                    return(0);
                }
            }
        }

        if (tooth_no >= last_tooth) {
            tooth_no = 0;
        } else if ((!edge) && ((tooth_no == 4) || (tooth_no == 10) || (tooth_no == 16)
                 || (tooth_no == 28) || (tooth_no == 34) || (tooth_no == 40))) { // check on rising
// re-sync on 23 and 47 removed - was giving a false sync-loss
            if ((tooth_diff_this > (tooth_diff_last << 1)) || ((tooth_diff_this << 1) < tooth_diff_last)) {
                outpc.syncreason = 68;
                ign_reset();
                return(0);
            }
        }
    }

    // only use falling edge for timing
    if (!edge) {
        return(0);
    }
    edge2_last = edge2;

    goto common_wheel;

    /************ YZF1000 *************/
  SPKMODE41:
    if (!(synch & SYNC_SYNCED)) {
        unsigned long tmp_tooth, tmp_tooth2;
        if ((!tooth_diff_this) || (!tooth_diff_last) || (!tooth_diff_last_1) ) {      // wait for at least three teeth
            return(0);
        }
        tmp_tooth = tooth_diff_this + (tooth_diff_this >> 1);
        tmp_tooth2 = tooth_diff_last_1 + (tooth_diff_last_1 >> 1);
        if ((tmp_tooth < tooth_diff_last) && ((tooth_diff_this + tooth_diff_last) > tmp_tooth2)) {
            // found the first regular tooth after the weird one
            synch |= SYNC_SYNCED;
        	outpc.status1 |= STATUS1_SYNCOK;
            tooth_no = 0;
        } else {
            return(0);
        }
    } else {
        // these are invalid tooth no.s but should be ok as we don't reach common_wheel
        if (tooth_no == 7) {
            tooth_no++;
            return(0); // we ignore this tooth for timing, handle tooth 0 specially below
        } else if (tooth_no == 8) {
            unsigned long tmp_tooth, tmp_tooth2;
            tmp_tooth = tooth_diff_this + (tooth_diff_this >> 1);
            tmp_tooth2 = tooth_diff_last_1 + (tooth_diff_last_1 >> 1);
            if ((tmp_tooth < tooth_diff_last) && ((tooth_diff_this + tooth_diff_last) > tmp_tooth2)) {
                // found the first regular tooth after the weird one
                tooth_no = 0;
            } else {
                outpc.syncreason = 69;
                ign_reset();
                return(0);
            }
        }
    }

    goto common_wheel;

    /* ----------------------  24-1-1 Honda Acura -------------------- */
  SPKMODE42:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            flagbyte1 &= ~flagbyte1_trig2active;
            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing tooth and then another one
        // and count the teeth in-between

        if (!(synch & SYNC_SEMI)) {     // just starting
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;     // started sync sequence
            }
            flagbyte1 &= ~flagbyte1_trig2active;        // assume that cam tooth must occur between missing tooth segments
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;
            if (!(synch & SYNC_SEMI2)) {        // just starting
                temp1 = tooth_diff_last + (tooth_diff_last >> 1);       // 1.5*
                if (tooth_diff_this > temp1) {
                    if (tooth_no == 7) {
                        /* at this gap presence or absence of cam in last group gives us full sync */
                        if ((flagbyte5 & FLAGBYTE5_CAM) && (flagbyte1 & flagbyte1_trig2active)) {      // COP or use-cam
                            outpc.status1 |= STATUS1_SYNCFULL;
                            tooth_no = 29;   // tooth 30
                        } else {
                            tooth_no = 7;   // tooth 8
                        }
                        synch &= ~SYNC_SEMI2;
                        goto WHL_HACC_OK;
                    } else if (tooth_no == 15) {
                        tooth_no = 0;  // tooth 1
                        if (flagbyte5 & FLAGBYTE5_CAM) { // COP or use-cam, not fully synced yet
                            synch |= SYNC_SEMI2; // don't know phase yet
                            if ((ram4.spk_mode3 & 0xc0) == 0x80) {
                                synch |= SYNC_WCOP;
                            }
                        }
                        goto WHL_HACC_OK;
                    } else {
                        tooth_no = 0;   // doesn't make sense, so try again from this missing tooth
                    }
                }
                return(0);
            }
        }
      WHL_HACC_OK:

        synch &= ~SYNC_SEMI;
        synch |= SYNC_SYNCED;
    } else {
        // recheck for sync in normal running
        if ((tooth_no == 7) || (tooth_no == 22) || (tooth_no == 29) || (tooth_no == 44)) {     // (one less)
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this <= temp1) {
                outpc.syncreason = 71;
                ign_reset();
                return(0);
            } else {
                // sync recheck passed, now if in cam/cop mode, see if we are on right phase
                if (tooth_no == 7) {
                    if (synch & SYNC_SEMI2) { // pending full sync
                        if (flagbyte1 & flagbyte1_trig2active) {
                            tooth_no = 29;   // tooth 30
                        } else {
                            tooth_no = 7;   // tooth 8
                        }
                        synch &= ~SYNC_SEMI2;
                        synch &= ~SYNC_WCOP;        // found cam, stop WCOP totally
                        if (flagbyte5 & FLAGBYTE5_CAM) {
                            outpc.status1 |= STATUS1_SYNCFULL;
                        }
                        SSEM0;
                        dwellsel = 0; // have to cancel any dwells in rare case there is one
                        dwellq[0].sel = 0;
                        dwellq[1].sel = 0;
                        CSEM0;
                        syncfirst(); // re-calculate next triggers to prevent gap
                    } else if (flagbyte5 & FLAGBYTE5_CAM) {
                        /* in COP mode, supposed to have full sync */
                        if (flagbyte1 & flagbyte1_trig2active) { // incorrect
                            outpc.syncreason = 72;
                            ign_reset();
                            return(0);
                        }
                    }
                } else if (tooth_no == 29) {
                    /* only have 29 in COP mode, supposed to have full sync */
                    if (!(flagbyte1 & flagbyte1_trig2active)) {
                        outpc.syncreason = 73;
                        ign_reset();
                        return(0);
                    }
                }
            }
            if (tooth_no == 22) {
                if ((!(flagbyte5 & FLAGBYTE5_CAM)) || (synch & SYNC_SEMI2)) { // // NOT COP or use-cam or not fully synced
                    tooth_no = 0;
                }
            } else if (tooth_no == 44) {
                tooth_no = 0;
            }
            flagbyte1 &= ~flagbyte1_trig2active;
        }
    }
    goto common_wheel;

        /************  Nissan vq35de *************/
        /* Derived from code donated by Gennardy Gurov */
        /* This is 36-2-2-2 with a number of cam notches for id */
        /* find a missing tooth, declare SEMI, count cam teeth then SYNC */
SPKMODE43:
//initial sync
    if (!(synch & SYNC_SYNCED)) {

        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            return(0);             // only sync when there's enough data
        }

        if (!(synch & SYNC_SEMI)) {
            // when unsynced we wait until we see a missing double tooth
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;
                trig2cnt = 0;
            } else {
                return(0);
            }
        } else {
            tooth_no++;
            // when SEMI we wait until we see a second missing double tooth
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this > temp1) {
                if (synch & SYNC_SEMI2) {
                    if (tooth_no == 10) {
                        if (trig2cnt == 1) {
                            tooth_no = 10; /* can't sync yet */
                            trig2cnt = 0;
                            return(0);
                        }
                    } else if (tooth_no == 20) {
                        if (trig2cnt == 0) {
                            tooth_no = 0;
                            goto SM43SS2;
                        } else if (trig2cnt == 2) {
                            goto SM43SS2;
                        }
                    } else if (tooth_no == 30) {
                        if (trig2cnt == 0) {
                            tooth_no = 40;
                            goto SM43SS2;
                        } else if (trig2cnt == 2) {
                            goto SM43SS2;
                        }
                    }
                    /* effective else clause */
                    outpc.syncreason = 75; /* failed SEMI2 somehow */
                    ign_reset();
                    return(0);
SM43SS2:;
                    outpc.status1 |= STATUS1_SYNCFULL;
                    trig2cnt = 0;
                    synch &= ~(SYNC_SEMI |SYNC_SEMI2);
                    synch |= SYNC_SYNCED;
                    /* fall through ok */
                } else {
                /* now figure out where we are */
                    if (trig2cnt < 3) {
                        tooth_no = trig2cnt * 10;
                        synch |= SYNC_SEMI2;
                        trig2cnt = 0;
                    } else {
                        outpc.syncreason = 74; /* must be noise - too many cam teeth */
                        ign_reset();
                        return(0);
                    }
                }
            } else {
                /* keep counting */
                return(0);
            }
        }
    } else {
        //re-sync on crank only. No need to re-check cam.
        if (
            (tooth_no == 10) ||
            (tooth_no == 20) ||
            (tooth_no == 30) ||
            (tooth_no == 40) ||
            (tooth_no == 50) ||
            (tooth_no == 60)) {
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this > temp1) {
                if (tooth_no == 60) {
                    tooth_no = 0;
                }
            } else {
                outpc.syncreason = 76;
                ign_reset();
                return(0);
            }
        } else if (
            (tooth_no == 7) ||
            (tooth_no == 17) ||
            (tooth_no == 27) ||
            (tooth_no == 37) ||
            (tooth_no == 47) ||
            (tooth_no == 57)) {
            trig2cnt = 0;
            trig3cnt = 0;
        }
    }
    goto common_wheel;

        /************  Jeep 2000 *************/
        /* Present implementation REQUIRES a cam signal.
           Assume cam leading edge precedes group of teeth after no.1
           Could be coded for distributor output without a cam signal */
SPKMODE44:
//initial sync
    if (!(synch & SYNC_SYNCED)) {

        if ((!tooth_diff_this) || (!tooth_diff_last) || (!trig2cnt)) {
            trig2cnt = 0;
            return(0);             // only sync when there's enough data
        }

        // when unsynced we wait until we see a missing double tooth
        temp1 = tooth_diff_last << 1;       // *2
        if (tooth_diff_this > temp1) {
            tooth_no = 4;
            outpc.status1 |= STATUS1_SYNCFULL;
            synch |= SYNC_SYNCED;
            trig2cnt = 0;
        } else {
            return(0);
        }
    } else {
        /* resync */
        if ((tooth_no == 4) || (tooth_no == 8) || (tooth_no == 12) || (tooth_no == 16) || (tooth_no == 20) || (tooth_no == 24)) {
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this < temp1) {
                outpc.syncreason = 77;
                ign_reset();
                return(0);
            }
            if (tooth_no == last_tooth) {
                tooth_no = 0;
            }
        }
    }

    goto common_wheel;
        /************  Jeep 2002 *************/
        /* crank pattern the same as Jeep 2000, but cam is another Chrysler weird one */
SPKMODE45:
    if (!(synch & SYNC_SYNCED)) {

        if ((!tooth_diff_this) || (!tooth_diff_last)) {
            return(0);             // only sync when there's enough data
        }

        if (!(synch & SYNC_SEMI)) {
            // when unsynced we wait until we see the long gap
            temp1 = tooth_diff_last << 1;
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;
                trig2cnt = 0;
            } else {
                return(0);
            }
        } else {
            tooth_no++;
            // when SEMI we wait until we see a second missing double tooth
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this > temp1) {
                if (synch & SYNC_SEMI2) {
                    if (tooth_no == 14) {
                        if (trig2cnt == 2) {
                            tooth_no = 4;
                            goto SM45SS2;
                        } else if (trig2cnt == 3) {
                            tooth_no = 16;
                            goto SM45SS2;
                        }
                    } else if (tooth_no == 24) {
                        if (trig2cnt == 3) {
                            tooth_no = 8;
                            goto SM45SS2;
                        } else if (trig2cnt == 2) {
                            tooth_no = 0;
                            goto SM45SS2;
                        }
                    } else if (tooth_no == 34) {
                        if (trig2cnt == 1) {
                            tooth_no = 12;
                            goto SM45SS2;
                        } else if (trig2cnt == 2) {
                            tooth_no = 20;
                            goto SM45SS2;
                        }
                    }
                    /* effective else clause */
                    outpc.syncreason = 79; /* failed SEMI2 somehow */
                    ign_reset();
                    return(0);
SM45SS2:;
                    outpc.status1 |= STATUS1_SYNCFULL;
                    trig2cnt = 0;
                    synch &= ~(SYNC_SEMI |SYNC_SEMI2);
                    synch |= SYNC_SYNCED;
                    /* fall through ok */
                } else {
                /* now figure out where we are */
                    if (trig2cnt && (trig2cnt < 4)) {
                        tooth_no = trig2cnt * 10;
                        synch |= SYNC_SEMI2;
                        trig2cnt = 0;
                    } else {
                        outpc.syncreason = 78; /* must be noise - too many cam teeth */
                        ign_reset();
                        return(0);
                    }
                }
            } else {
                /* keep counting */
                return(0);
            }
        }
    } else {
        /* resync */
        if ((tooth_no == 4) || (tooth_no == 8) || (tooth_no == 12) || (tooth_no == 16) || (tooth_no == 20) || (tooth_no == 24)) {
            temp1 = tooth_diff_last << 1;       // *2
            if (tooth_diff_this < temp1) {
                outpc.syncreason = 77;
                ign_reset();
                return(0);
            }
            if (tooth_no == last_tooth) {
                tooth_no = 0;
            }
        }
    }
    goto common_wheel;
    /************ Zetec VCT *************/
  SPKMODE46:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {

            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing tooth and then another one
        // and count the teeth in-between

        if (!(synch & SYNC_SEMI)) {     // just starting
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;     // started sync sequence
            }
            trig2cnt = 0;
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);       // 1.5*
            if ((tooth_diff_this > temp1) && (tooth_no == 35)) {
                if (num_cyl == 6) {
                    if (trig2cnt == 1) { // 1 for V6
                        tooth_no = 0;   // tooth 1
                        goto ZVCT_OK;
                    } else if (trig2cnt == 2) { // 2 for V6
                        tooth_no = 35;  // tooth 36
                        goto ZVCT_OK;
                    } else {
                        tooth_no = 0;   // doesn't make sense, so try again from this missing tooth
                        trig2cnt = 0;
                    }
                } else {
                    if (trig2cnt == 2) { // 2 for I4,V8
                        tooth_no = 0;   // tooth 1
                        goto ZVCT_OK;
                    } else if (trig2cnt == 3) { // 3 for I4,V8
                        tooth_no = 35;  // tooth 36
                        goto ZVCT_OK;
                    } else {
                        tooth_no = 0;   // doesn't make sense, so try again from this missing tooth
                        trig2cnt = 0;
                    }
                }
            } else if (tooth_no > 35) {
                /* didn't find the second missing, start again */
                synch &= ~SYNC_SEMI;
            }
            return(0);
        }
      ZVCT_OK:
        outpc.status1 |= STATUS1_SYNCFULL;
        trig2cnt = 0;
        synch &= ~SYNC_SEMI;
        synch &= ~SYNC_SEMI2;
        synch |= SYNC_SYNCED;
    } else {

        // recheck for sync in normal running - _only_ crank
        if ((tooth_no == 35) || (tooth_no == 70)) {     // (one less)
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this <= temp1) {
                outpc.syncreason = 82;
                ign_reset();
                return(0);
            }

            /* cam phase validation during cranking */
            if ((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)) {
                if ( ((tooth_no == 35) && (trig2cnt != 3))
                    || ((tooth_no == 70) && (trig2cnt != 2)) ) {
                    outpc.syncreason = 83;
                    ign_reset();
                    return(0);
                }
            }

            trig2cnt = 0;

            if (tooth_no == 70) {
                tooth_no = 0;
            }
        }
    }
    goto common_wheel;

/* ****** Flywheel tri-tach **********
SPKMODE47:
    this is handled by the CAS360 code
*/

    /************ fuel only mode *************/
  SPKMODEFUEL:
    synch |= SYNC_SYNCED;
//    thistoothevents |= FUEL;    // every tach event is a fuel tooth
    /************ end of the long if for different variants of wheel mode *************/

  common_wheel:
    return(1);
}

unsigned char ISR_Ign_TimerIn_paged2(unsigned char edge, unsigned char edge2)
{
    /* modes 48 onwards only */
    if (spkmode == 48) {
        goto SPKMODE48;
    } else if ((spkmode == 49) || (spkmode == 53)) {
        goto SPKMODE49;
    } else if (spkmode == 50) {
        goto SPKMODE50;
    } else if (spkmode == 51) {
        goto SPKMODE51;
    } else if (spkmode == 52) {
        goto SPKMODE52;
    /* SPKMODE53 is handled in SPKMODE49 */
    } else if (spkmode == 54) {
        goto SPKMODE54;
    } else if (spkmode == 55) {
        goto SPKMODE55;
    } else if (spkmode == 56) {
        goto SPKMODE56;
    /* SPKMODE57 is handled in SPKMODE7 */
    } else {
        return(0);
    }

    /************ 2JZ VVti *************/
  SPKMODE48:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {

            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing tooth and then another one
        // and count the teeth in-between

        if (!(synch & SYNC_SEMI)) {     // just starting
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this > temp1) {
                tooth_no = 0;
                synch |= SYNC_SEMI;     // started sync sequence
            }
            trig2cnt = 0;
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;
            if (num_cyl == 4) {
                if (tooth_no == 34) {
                    if (trig2cnt == 1) {
                        tooth_no = 34;
                        goto TWOJZ_OK;
                    } else if (trig2cnt == 2) {
                        tooth_no = 0;
                        goto TWOJZ_OK;
                    } else {
                        synch &= ~SYNC_SEMI;
                    }
                }
            } else { /* 6, 8 and 12 cyl */
                /* look for cam tooth in certain areas only and determine phase */
                /* examine 8-14 and 21-29 */
                if ((tooth_no == 0) || (tooth_no == 12) || (tooth_no == 24)) {
                    trig2cnt = 0;
                } else if ((tooth_no == 10) && trig2cnt) {
                    goto TWOJZ_OK;
                } else if ((tooth_no == 22) && trig2cnt) {
                    tooth_no = 56;
                    goto TWOJZ_OK;
                } else if ((tooth_no == 34) && trig2cnt) {
                    goto TWOJZ_OK;

                } else if (tooth_no > 35) {
                    /* didn't find the second missing, start again */
                    synch &= ~SYNC_SEMI;
                }
            }
            return(0);
        }
      TWOJZ_OK:
        outpc.status1 |= STATUS1_SYNCFULL;
        trig2cnt = 0;
        synch &= ~SYNC_SEMI;
        synch &= ~SYNC_SEMI2;
        synch |= SYNC_SYNCED;
    } else {
        // recheck for sync in normal running - _only_ crank
        if ((tooth_no == 34) || (tooth_no == 68)) {     // (one less)
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);   // 1.5*
            if (tooth_diff_this <= temp1) {
                outpc.syncreason = 84;
                ign_reset();
                return(0);
            }
            if (tooth_no == 68) {
                tooth_no = 0;
            }
        }

        /* cam phase validation during cranking */
        if ((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)) {
            if (num_cyl == 4) {

                if ((tooth_no == 34) || (tooth_no == 0)) {
                    if ( ((tooth_no == 34) && (trig2cnt != 1))
                        || ((tooth_no == 0) && (trig2cnt != 2)) ) {
                        outpc.syncreason = 85;
                        ign_reset();
                        return(0);
                    } else {
                        trig2cnt = 0;
                    }
                }

            } else { /* 6, 8 and 12 cyl */
                if ((tooth_no == 0) || (tooth_no == 46) || (tooth_no == 24)) {
                    trig2cnt = 0;
                } else if (((tooth_no == 56) && (!trig2cnt))
                    || ((tooth_no == 34) && (!trig2cnt))) {
                    outpc.syncreason = 85;
                    ign_reset();
                    return(0);
                }
            }
        }
    }
    goto common_wheel2;

    /************ Honda TSX / D17 + K24A2 *************/
  SPKMODE49:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {

            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a +1 tooth and then another one
        // and count the cam teeth in-between

        if (!(synch & SYNC_SEMI)) {     // just starting
            temp1 = tooth_diff_this << 1;
            if ((tooth_diff_last > temp1) && (tooth_diff_last_1 > temp1)) { // this tooth is smaller than both (1/2 last) and (1/2 last_1)
                tooth_no = 0;
                synch |= SYNC_SEMI;     // started sync sequence
            }
            trig2cnt = 0;
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;
            temp1 = tooth_diff_this << 1;
            if ((tooth_diff_last > temp1) && (tooth_diff_last_1 > temp1)) { // this tooth is smaller than both (1/2 last) and (1/2 last_1)
                if ((tooth_no == 13) && (trig2cnt == 2)) {
                    tooth_no = 1;
                    goto TSX_OK;
                } else if ((tooth_no == 13) && (trig2cnt == 3)) {
                    tooth_no = 14;
                    goto TSX_OK;
                } else {
                    outpc.syncreason = 86;
                    ign_reset();
                    return(0);
                }
            } else if (tooth_no > 26) {
                /* didn't find the second missing, start again */
                synch &= ~SYNC_SEMI;
            }
            return(0);
        }
      TSX_OK:
        outpc.status1 |= STATUS1_SYNCFULL;
        trig2cnt = 0;
        synch &= ~SYNC_SEMI;
        synch &= ~SYNC_SEMI2;
        synch |= SYNC_SYNCED;
    } else {
        // recheck for sync in normal running - _only_ crank
        if ((tooth_no == 1) || (tooth_no == 14)) {
            temp1 = tooth_diff_this << 1;
            if (! ((tooth_diff_last > temp1) && (tooth_diff_last_1 > temp1))) {
                outpc.syncreason = 87;
                ign_reset();
                return(0);
            }
            /* cam phase validation during cranking */
            if ((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)) {
               if (((tooth_no == 1) && (trig2cnt != 2)) || ((tooth_no == 14) && (trig2cnt != 3))) {
                    outpc.syncreason = 88;
                    ign_reset();
                    return(0);
                }
            }
            trig2cnt = 0;
        }

        if (tooth_no == 26) {
            tooth_no = 0;
        }
    }
    goto common_wheel2;

    /************ Mazda6 2.3 VVT (36-2-2-2) *************/
  SPKMODE50:
    /* when CAM input enabled we check it during initial sync only
       This _could_ result in an incorrect phase if we got noise on
       the cam during initial sync and we won't detect it later.
     */
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            return(0);             // only sync when there's enough data
        }
        // when unsynced we wait until we see a missing double tooth after a couple
        // of shorter ones

        if (!(synch & SYNC_SEMI)) {     // just starting
            if ((tooth_diff_this > (tooth_diff_last << 1))
                && (tooth_diff_this < (tooth_diff_last << 2))
                && (tooth_diff_this > (tooth_diff_last_1 << 1))
                && (tooth_diff_this < (tooth_diff_last_1 << 2))) {
                synch |= SYNC_SEMI;     // started sync sequence
                tooth_no = 0;
            }
            return(0);
        } else {                // started sequence, check possible sync points
            tooth_no++;

            if (tooth_no == 1) {
                if ((tooth_diff_this > (tooth_diff_last_1 << 1))
                    && (tooth_diff_this < (tooth_diff_last_1 << 2))) {
                    // compare against tooth before double missing
                    // found double tooth sequence, set tooth and go
                    tooth_no = 17;
                    goto MODE50_OK;
                }
            } else if ((tooth_diff_this > (tooth_diff_last << 1))
                       && (tooth_diff_this < (tooth_diff_last << 2))) {
                if (tooth_no == 13) {
                    tooth_no = 0;
                    goto MODE50_OK;
                } else if (tooth_no == 16) {
//                    tooth_no = 16;
                    goto MODE50_OK;
                }
            } else if (tooth_no > 30) {
                outpc.syncreason = 89;
                ign_reset();
            }
            return(0);
        }
MODE50_OK:
        if ((ram4.spk_mode3 & 0xc0) == 0x80) {
            synch |= SYNC_WCOP;
        }
        trig2cnt = 0;
        ls1_ls = 0; // use as tooth counter since sync
        synch &= ~SYNC_SEMI;
        synch |= SYNC_SYNCED;

    } else {
        // recheck for sync - and also check for full sync
        if (cycle_deg == 7200) {
            if (!(outpc.status1 &= STATUS1_SYNCFULL)) {
                /* find phase */
                ls1_ls++;
                if ((tooth_no == 3) || (tooth_no == 33)) {
                    if (ls1_ls > 4) {
                        if (trig2cnt == 2) {
                            tooth_no = 33;
                            outpc.status1 |= STATUS1_SYNCFULL;
                        } else if (trig2cnt == 1) {
                            tooth_no = 3;
                            outpc.status1 |= STATUS1_SYNCFULL;
                        }
                    }
                    trig2cnt = 0;
                } else if ((tooth_no == 19) || (tooth_no == 49)) {
                    if (ls1_ls > 4) {
                        if (trig2cnt == 2) {
                            tooth_no = 49;
                            outpc.status1 |= STATUS1_SYNCFULL;
                        } else if (trig2cnt == 1) {
                            tooth_no = 19;
                            outpc.status1 |= STATUS1_SYNCFULL;
                        }
                    }
                    trig2cnt = 0;
                }
            }
        }

        if ((tooth_no == 16) || (tooth_no == 30) || (tooth_no == 46) || (tooth_no == 60)) {     // (one less)
            if (tooth_diff_this <= (tooth_diff_last << 1)) {
                //fell over
//                if ((tooth_no == 16) || (tooth_no == 46)) {
//                    outpc.syncreason = 99;
//                } else {
                    outpc.syncreason = 99;
//                }
                ign_reset();
                return(0);
            }

            if (flagbyte5 & FLAGBYTE5_CAM) {
                if (tooth_no == 60) {
                    tooth_no = 0;
                }
            } else {
                if (tooth_no == 30) {
                    tooth_no = 0;
                }
            }
        }
    }
    goto common_wheel2;
    /************ Viper V10 Gen 2 *************/
  SPKMODE51:
    //initial sync, wait for a cam edge
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last) || (!tooth_diff_last_1) // 3 teeth at least
            || (!(flagbyte1 & flagbyte1_trig2active)) // must have seen a cam transition
            || (!(tooth_diff_this > (tooth_diff_last << 1)))) { // validates tooth sizes
            flagbyte1 &= ~flagbyte1_trig2active;
            return(0);             // only sync when there's enough data
        }
        /* Have some teeth and have seen a cam edge transition */
        if (ram4.ICIgnOption & 0x01) {      // 1 = rising edge
            if ((PORTT & TFLG_trig2) == 0) {
                tooth_no = 10;  // low so phase 2
            } else {
                tooth_no = 0; // high so phase 1
            }
        } else {            // opposite
            if (PORTT & TFLG_trig2) {
                tooth_no = 10; // high so phase 2
            } else {
                tooth_no = 0; // low so phase 1
            }
        }
        synch |= SYNC_SYNCED;
        outpc.status1 |= STATUS1_SYNCFULL;
    } else {
        /* resync code */
        if (tooth_no == 20) {
            if (((((outpc.engine & ENGINE_CRANK) || (flagbyte2 & flagbyte2_crank_ok)))
                    && (!(flagbyte1 & flagbyte1_trig2active)))
                    || (tooth_diff_this < (tooth_diff_last << 1))) {
                // check for cam edge presence each cycle
                // check tooth large/small pattern
                outpc.syncreason = 17;
                ign_reset();
            } else {
                tooth_no = 0;
            }
        }
    }
    flagbyte1 &= ~flagbyte1_trig2active;
    goto common_wheel2;

    /************ Viper V10 Gen 1 *************/
  SPKMODE52:
    //initial sync, wait for a cam edge
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last) || (!tooth_diff_last_1) // 3 teeth at least
            || (!(flagbyte1 & flagbyte1_trig2active)) // must have seen a cam transition
            || (!(tooth_diff_this > (tooth_diff_last << 1)))) { // validates tooth sizes
            flagbyte1 &= ~flagbyte1_trig2active;
            return(0);             // only sync when there's enough data
        }
        if (trig2cnt != 2) {
            trig2cnt = 0;
            return(0);             // only sync when there's enough data
        }
        tooth_no = 16;
        synch |= SYNC_SYNCED;
        outpc.status1 |= STATUS1_SYNCFULL;
    } else {
        /* resync code */
        if ((tooth_no == 10) || (tooth_no == 16) || (tooth_no == 20)) {
            if (!(flagbyte1 & flagbyte1_trig2active)) {
                // check for cam tooth presence on these teeth
                if (tooth_no == 16) { 
                    outpc.syncreason = 94; // this is where the double cam pulse should have been
                } else {
                    outpc.syncreason = 95; // this is where a single cam pulse should have been
                }
                goto SPK52_LOST;
            } else if (tooth_diff_this < (tooth_diff_last << 1)) {
                // check tooth large/small pattern
                outpc.syncreason = 93;
SPK52_LOST:
                ign_reset();
            } else if (tooth_no == 20) {
                tooth_no = 0;
            }
        }
    }
    flagbyte1 &= ~flagbyte1_trig2active;
    goto common_wheel2;

    /************ HD 32-2 with MAP phase sensing instead of cam sensor *************/

  SPKMODE54:
    if (!(synch & SYNC_SYNCED)) {
        if (!(synch & SYNC_SEMI)) { 
            /* Here we will look for missing */
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);

            if (tooth_diff_this > temp1) {
                synch |= SYNC_SEMI;
            }
            return(0);
        } else {
            /* tooth after what we thought was missing, lets make sure this one is short
             * on the first sync, we miss tooth #1, but that's ok as long as we catch it next time
             * split this into 2 operations so compiler doesn't call subroutine
             */
            temp1 = (tooth_diff_last >> 1); /* 1/2 of last value */
            temp1 = temp1 >> 1;     /* 1/4th of last value */

            temp1 = tooth_diff_last - temp1;        /* 3/4ths of last value */

            synch &= ~SYNC_SEMI;
            if (tooth_diff_this >= temp1) {
                /* this wasn't really the missing tooth... but don't tell the user we lost sync */
                return(0);
            } else {
                synch |= SYNC_SYNCED;
                tooth_no = 1; // for non poll level case we can't guess the phase, so start at 1.
                /* Always start wasted */
                synch |= SYNC_SEMI2;
                synch |= SYNC_WCOP; // enable WCOP mode (not for odd numbers of cylinders > 1)
                ls1_sl = 0; // vars to record sync process
                ls1_ls = 0;
                flagbyte1 &= ~flagbyte1_trig2active; // clear cam flag
            }
        }

    } else {
        /* This is re-sync and where we switch from wasted to COP */
        if ((tooth_no == 30) || (tooth_no == 60)) {
            /* this means we should have the last tooth here, so check for missing */
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);       /* 1.5 * last tooth */

            if (tooth_diff_this <= temp1) {
                // wasn't a missing tooth when it was supposed to be
                outpc.syncreason = 2;
                ign_reset();
                return(0);
            } else {
                // it was the missing tooth
                flagbyte1 &= ~flagbyte1_trig2active; // never record the cam signal

                if (synch & SYNC_SEMI2) {
                    /* SEMI2 here means that we've synced to the crank, but have
                        yet to confirm cam sync. We are still running wasted COP and
                        are looking to match an on/off/on/off cam pattern. */
                    unsigned char match;
                    ls1_sl++;
                    ls1_ls <<= 2;

                    /* choose phase determination */
                    /* both of these need confirmation */
                    if ((ram4.hardware & 0xc0) == 0x80) {
                        /* use MAP sensor for sync */
                        if (((mapmin_tooth >= 19) && (mapmin_tooth <= 21)) 
                            || ((mapmin_tooth >= 49) && (mapmin_tooth <= 51))) {
                            ls1_ls |= 1; // one phase
                        } else if (((mapmin_tooth >= 16) && (mapmin_tooth <= 18)) 
                            || ((mapmin_tooth >= 46) && (mapmin_tooth <= 48))) {
                            ls1_ls |= 2; // other phase
                        } else {
                            // don't set any bits
                        }
                    } else {
                        /* use cam sensor */
                        if (PTT & TFLG_trig2) { // poll cam
                            ls1_ls |= 2; // cam
                        } else {
                            ls1_ls |= 1; // no-cam
                        }
                    }

////////////////////////////////
//                    if (ls1_sl < 80) { // for testing, force a delay
//                        ls1_ls &= ~0x03; // clear bits to force failed sync
//                    }
////////////////////////////////        

                    match = 0x09; // 1001 // else 0x06 = 0110

                    if ((ls1_sl > 3) && ((ls1_ls & 0x0f) == match)) {
                        synch &= ~SYNC_SEMI2;        // found cam, stop WCOP of dwells
                        dwellsel = 0; // have to cancel any dwells in rare case there is one
                        dwellq[0].sel = 0;
                        dwellq[1].sel = 0;
                        if (tooth_no == 30) {
                            /* going to be going back in time, check for pre-existing fuel pulses to prevent doubles*/
                            if (next_inj[0].tooth < 30) {
                                skipinj |= 1;
                            }
                            if (next_inj[1].tooth < 30) {
                                skipinj |= 2;
                            }
                        }
                        tooth_no = 0; // declare we really are at the first tooth
                        outpc.status1 |= STATUS1_SYNCFULL;
                        syncfirst(); // re-calculate next triggers to prevent gap
                        /* kill off some dwells */
                        skipdwell[2] = 1;
                        skipdwell[3] = 1;
                    } else if (ls1_sl > 100) {
                        /* Failed to receive matching pattern, "cam" input faulty */
                        /* 100 revs for map threshold sensing in case operator starts at WOT */
                        synch &= ~(SYNC_SEMI2 | SYNC_WCOP);
                        ls1_sl = 99; // lock here
                        synch |= SYNC_WCOP2; // force WCOP mode
                    }
                } else if (synch & SYNC_WCOP) {
                    synch &= ~SYNC_WCOP;        // stop WCOP totally
                    /* WCOP was left on for a cycle to ensure that any remaining spark events were cleared
                        to prevent the possibility of spark hang-on */
                    dwellsel = 0; // have to cancel any dwells in rare case there is one
                    dwellq[0].sel = 0;
                    dwellq[1].sel = 0;
                }
            }
            
            flagbyte1 &= ~flagbyte1_trig2active; // clear cam flag

            if (tooth_no == 60) {
                tooth_no = 0;
            }
        }
    }

    goto common_wheel2;

    /************ Miata 36-2 *************/
SPKMODE55:
    if (!(synch & SYNC_SYNCED)) {
        if (!(synch & SYNC_SEMI)) { 
            /* Here we will look for missing */
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);

            if (tooth_diff_this > temp1) {
                synch |= SYNC_SEMI;
            }
            trig2cnt = 0;
            return(0);
        } else {
            /* tooth after what we thought was missing, lets make sure this one is short
             * on the first sync, we miss tooth #1, but that's ok as long as we catch it next time
             * split this into 2 operations so compiler doesn't call subroutine
             */
            temp1 = (tooth_diff_last >> 1); /* 1/2 of last value */
            temp1 = temp1 >> 1;     /* 1/4th of last value */

            temp1 = tooth_diff_last - temp1;        /* 3/4ths of last value */

            synch &= ~SYNC_SEMI;
            if (tooth_diff_this >= temp1) {
                /* this wasn't really the missing tooth... but don't tell the user we lost sync */
                return(0);
            } else {
                synch |= SYNC_SYNCED;
                tooth_no = 1; // for non poll level case we can't guess the phase, so start at 1.
                /* Always start wasted */
                synch |= SYNC_SEMI2;
                synch |= SYNC_WCOP; // enable WCOP mode (not for odd numbers of cylinders > 1)
                ls1_sl = 0; // vars to record sync process
                ls1_ls = 0;
                flagbyte1 &= ~flagbyte1_trig2active; // clear cam flag
                trig2cnt = 0;
            }
        }

    } else {
        /* This is re-sync and where we switch from wasted to COP */
        if ((tooth_no == 34) || (tooth_no == 68)) {
            /* this means we should have the last tooth here, so check for missing */
            temp1 = tooth_diff_last + (tooth_diff_last >> 1);       /* 1.5 * last tooth */

            if (tooth_diff_this <= temp1) {
                // wasn't a missing tooth when it was supposed to be
                outpc.syncreason = 2;
                ign_reset();
                return(0);
            } else {
                // it was the missing tooth

                if (synch & SYNC_SEMI2) {
                    /* SEMI2 here means that we've synced to the crank, but have
                        yet to confirm cam sync. We are still running wasted COP and
                        are looking to match an on/off/on/off cam pattern. */
                    ls1_sl++;
                    ls1_ls <<= 2;

                    ls1_ls |= (trig2cnt & 0x03); // 1 or 2 cam pulses
                    
                    if ((ls1_sl > 3) && ((ls1_ls & 0x0f) == 0x09)) { // i.e. 1001
                        synch &= ~SYNC_SEMI2;        // found cam, stop WCOP of dwells
                        goto SM55_MT;

                    } else if (ls1_sl > 10) {
                        /* Failed to receive matching pattern, "cam" input faulty */
                        synch &= ~(SYNC_SEMI2 | SYNC_WCOP);
                        ls1_sl = 99; // lock here
                        synch |= SYNC_WCOP2; // force WCOP mode
                    }
                } else if (synch & SYNC_WCOP) {
                    if (flagbyte1 & flagbyte1_trig2active)  {
                        synch &= ~SYNC_WCOP;        // found cam, stop WCOP totally
                        /* WCOP was left on for a cycle to ensure that any remaining spark events were cleared
                            to prevent the possibility of spark hang-on */
SM55_MT:;
                        dwellsel = 0; // have to cancel any dwells in rare case there is one
                        dwellq[0].sel = 0;
                        dwellq[1].sel = 0;
                        // consider pre-existing fuel pulses
                        tooth_no = 0; // declare we really are at the first tooth
                        outpc.status1 |= STATUS1_SYNCFULL;
                        syncfirst(); // re-calculate next triggers to prevent gap
                    }
                }
            }
            
            flagbyte1 &= ~flagbyte1_trig2active; // clear cam flag
            trig2cnt = 0;

            if (tooth_no == 68) {
                tooth_no = 0;
            }
        }
    }

    goto common_wheel2;

    /************ Daihatsu 12+1 (3 cyl) *************/
  SPKMODE56:
    //initial sync
    if (!(synch & SYNC_SYNCED)) {
        if ((!tooth_diff_this) || (!tooth_diff_last)
            || (!tooth_diff_last_1)) {
            return(0);             // only sync when there's enough data
        }
        //look for short tooth gap - this is tooth no.1
        if ( (tooth_diff_this > tooth_diff_last)
            && (tooth_diff_this < tooth_diff_last_1)
            && ((tooth_diff_last << 1) < tooth_diff_last_1)
            ) {
            tooth_no = 0;
            outpc.status1 |= STATUS1_SYNCFULL;
            synch |= SYNC_SYNCED;
        } else {
            return(0);
        }

    } else {
        // recheck for sync
        if (tooth_no == 13) {    // (one less)
            if ( (tooth_diff_this > tooth_diff_last)
                && (tooth_diff_this < tooth_diff_last_1)
                && ((tooth_diff_last << 1) < tooth_diff_last_1)
                ) {
                tooth_no = 0;
            } else {
                outpc.syncreason = 47;
                ign_reset();
                return(0);
            }
        }
    }

//    goto common_wheel;


  common_wheel2:;
    return(1);
}

void ISR_Ign_TimerIn_part2(void)
{
    unsigned int fuel_flag = 0;
    unsigned int sched_both;
    unsigned char next = 0;
    unsigned char thistoothevents = 0;

    checkforpit0();     // check for pending interrupt
    checkforsci0();
#ifdef MS3PRO
    checkforsci1();
#endif

// this is the real common_wheel
    if (synch & SYNC_FIRST) {
        syncfirst();
    }
    if (synch & SYNC_SYNCED) {
        tooth_no++;
        if (!(spkmode == 15)) {
            if (tooth_no > last_tooth) {
                if (!((spkmode <= 3) || (spkmode == 31))) {   // NOT 0,1,2,3,31
                    outpc.syncreason = 10;
                    ign_reset();    // stuff the permissiveness
                } else {
                    tooth_no = 1;
                }
            }
        }
    }

    if ((synch & SYNC_SYNCED) || (synch & SYNC_SEMI)) { 
        if ((spkmode == 5) || (spkmode == 6) || (spkmode == 40) || ((spkmode == 41) && (tooth_no == 1)) || ((spkmode == 2) && ((ram4.spk_conf2 & 0x07) == 0x04)))  {
            // Neon/420A and 36-1+1 join up tooth times. Same for YZF1000 over weird tooth
            dtpred_adder += tooth_diff_this + tooth_diff_last;  // look for more flexible way to do this
            tooth_diff_rpm_last = tooth_diff_rpm;
            tooth_diff_rpm.time_32_bits =
                tooth_diff_this + tooth_diff_last;
            tooth_no_rpm = tooth_no;
        } else if (spkmode == 15) {
            if (tooth_no == 5) { // vane tooth not used for timing
                return;
            } else if (tooth_no == 1) { // tooth after vane must include the vane
                dtpred_adder += tooth_diff_this + tooth_diff_last;
                tooth_diff_rpm_last = tooth_diff_rpm;
                tooth_diff_rpm.time_32_bits = tooth_diff_this+tooth_diff_last;
                tooth_no_rpm = tooth_no;
            } else { // others are normal
                dtpred_adder += tooth_diff_this;
                tooth_no_rpm = tooth_no;
                tooth_diff_rpm_last = tooth_diff_rpm;
                tooth_diff_rpm.time_32_bits = tooth_diff_this;
            }
        } else {
            dtpred_adder += tooth_diff_this;
            tooth_no_rpm = tooth_no;
            tooth_diff_rpm_last = tooth_diff_rpm;
            tooth_diff_rpm.time_32_bits = tooth_diff_this;
        }
        // log to ring buffer for mainloop to extract tooth times
        act_tooth_time[tooth_counter] = tooth_diff_rpm;
        act_tooth_num[tooth_counter] = tooth_no;
        tooth_counter++;
        if (tooth_counter >= WHEEL_NUM_TEETH) {
            tooth_counter = 0;
        }

        if (outpc.rpm == 0) {
            outpc.rpm = 1;      // fake non zero rpm
            if (!(outpc.engine & ENGINE_CRANK)) {
                lmms_crank = lmms; // time of first declaring cranking
            }
            outpc.engine |= ENGINE_CRANK;       // declare cranking
        }

    }
    // if we get repeated additional or lost teeth then something is wrong
    if (syncerr > 3) {
        outpc.syncreason = 12;
        ign_reset();
        return;
    }
    //---

    /* update the dwell and spark time... if possible.
     * Make sure that dwell and spark tooth are both
     * still the same, otherwise don't update.
     */
    if ((next_dwell.tooth == dwell_events[next_dwell.coil].tooth) &&
        (next_spark.tooth == spark_events[next_spark.coil].tooth)) {
        next_dwell.time32 = dwell_events[next_dwell.coil].time32;
        next_spark.time32 = spark_events[next_spark.coil].time32;
    }

    if (((ram4.EngStroke & 0x03) == 0x03)) {
        if ((next_dwl_trl.tooth == dwell_events[next_dwl_trl.coil].tooth)
            && (next_spk_trl.tooth ==
                spark_events[next_spk_trl.coil].tooth)) {
            next_dwl_trl.time32 = dwell_events[next_dwl_trl.coil].time32;
            next_spk_trl.time32 = spark_events[next_spk_trl.coil].time32;
        }
    }

    /* now I need to figure out which coil I want to fire,
     * or if I'm on a "fuel tooth"
     */

    // distributor mode
    // calc the spark and dwell timing right here for best accuracy with the low tooth count
    if ((spkmode & 0xfe) == 2) {  // 2,3
        unsigned long tooth_diff_next;
        unsigned char ks;

        ks = 0;

        if (ram4.adv_offset < 200) { // next cyl specific
            if (coilsel) { // there's a spark pending, fire it
                SSEM0;
                TIE &= ~0x02;
                TFLG1 = 0x02;
                CSEM0;
                FIRE_COIL;
            }

            if (spkmode == 2) { // non trigger-return only
                if (outpc.engine & ENGINE_CRANK) {
                    nextcyl_cnt = 10;
                }
                if (nextcyl_cnt) {
                    nextcyl_cnt--; // decays to zero
                    coilsel = 1; // always fire here at the trigger during cranking and during crank->run cycles
                    FIRE_COIL;
                }
            }

        } else if (spkmode == 3) {
            if (outpc.engine & ENGINE_CRANK) {
                nextcyl_cnt = 4;
            }
            if (nextcyl_cnt) {
                nextcyl_cnt--; // decays to zero
            }
        }

        if ((ram4.ICIgnOption & 0x08) || ((spkmode == 2) && ((ram4.spk_conf2 & 0x07) == 0x04))) {
            // isn't the next period at all (reflects the odd period we just had) but is the best estimate used to
            // calculate the delay until spark
            // For oddfire we use two periods for the scaling calc.
            // by good fortune this is what's needed on sig-PIP TFI too
            tooth_diff_next = tooth_diff_this + tooth_diff_last;
            if (tooth_diff_last_1 && tooth_diff_last_2) { // enough old data to do prediction
                tooth_diff_next = (tooth_diff_next << 1) - tooth_diff_last_1 - tooth_diff_last_2; // 1st deriv prediction over longer period
            }
        } else {
            tooth_diff_next = (tooth_diff_this << 1) - tooth_diff_last; // 1st deriv prediction worked fine in MS1.
        }

        /* Check for kick-start feature */
        if ((ram4.spk_mode3 & SPK_MODE3_KICK)
            && ((outpc.rpm < 100) || (outpc.engine & ENGINE_CRANK) || nextcyl_cnt)
            && ((spkmode == 3) || ((spkmode == 2) && (ram4.adv_offset < 200)))
            ) {
            if ((outpc.rpm < 100) || (outpc.engine & ENGINE_CRANK)) {
                /* In Kick-start we fire AFTER the trigger pulse only
                    so do not use normal scheduling */
                ks = 1;
                thistoothevents &= ~DWELL;
                thistoothevents &= ~SPARK;
                coilsel = 0;
                dwellsel = 0;
                /* On MS3 also necessary to kill the timers here */
                SSEM0;
                TIE &= ~0x82;
                TFLG1 = 0x82;
                CSEM0;
            } /* else, during transition do normal and delayed spark */
            /* SPKMODE2 next-cyl, schedule for soon after */
            if (spkmode == 2) {
                /* Kickstart delayed firing */
                TC7 = TC0this + ram5.kickdelay;
                TC1 = TC0this + ram5.kickdelay + (outpc.coil_dur * 100);
                SSEM0;
                TIE |= 0x82;
                TFLG1 = 0x82;
                dwellsel = 1;
                coilsel = 1;
                CSEM0;
            }
            /* SPKMODE3 handled on trigger-return edge (above) */
        }

        if (ks == 0) {

            if ((spkmode == 3) && ((outpc.rpm < 100) || (outpc.engine & ENGINE_CRANK))) {
                // In trigger return during cranking we'll use the late timer driven spark while
                // expecting that the return will actually do the business
                next_spark.time32 = muldiv(trigret_scaler, tooth_diff_next);
            } else {
                next_spark.time32 = muldiv(dizzy_scaler[tooth_no-1], tooth_diff_next);
            }

            if (next_spark.time32 < 110) { // same number as below
                next_spark.time32 = 111; // prevent it getting too close to trigger
            }

            if (nextcyl_cnt < 8) {
                thistoothevents |= SPARK;
            }

            if (num_spk > 1) { // i.e. 2
                if (tooth_no & 1) {
                    SSEM0;
                    tmpcoilsel = 1;
                    CSEM0;
                } else {
                    SSEM0;
                    tmpcoilsel = 2;
                    CSEM0;
                }
            } else {
                SSEM0;
                tmpcoilsel = 1;
                CSEM0;
            }
          
            if ((ram4.dwellmode & 3) != 2) {
                unsigned long tmp_next_sparktime;
                unsigned char tmp_tooth;
                SSEM0;
                tmpdwellsel = 1; // FIXME oddfire ?
                CSEM0;

                if (ram4.ICIgnOption & 0x08) { // oddfire
                    tmp_tooth = tooth_no;
                    if (tmp_tooth >= no_teeth) { // look at next tooth period
                        tmp_tooth = 0;
                    }
                    // redo spark calc for period coming up (as best we can) using old data
                    tmp_next_sparktime = muldiv(dizzy_scaler[tmp_tooth], tooth_diff_next);
                } else {
                    if ((spkmode == 3) && ((outpc.rpm < 100) || (outpc.engine & ENGINE_CRANK))) {
                        // in trigger return figure out when we think the return-spark ought to happen 
                        tmp_next_sparktime = muldiv(dizzy_scaler[tooth_no-1], tooth_diff_next);
                    } else {
                        tmp_next_sparktime = next_spark.time32;
                    }
                }

                // choose to schedule dwell from this trigger (low speeds)
                if (next_spark.time32 > (dwell_long + 70)) {
                    // have time to schedule dwell from here
                    // this is for a dwell coming after this trigger and before the spark
                    thistoothevents |= DWELL;
                    if ((spkmode == 3) && ((outpc.rpm < 100) || (outpc.engine & ENGINE_CRANK))) {
                        next_dwell.time32 = tmp_next_sparktime - dwell_long;
                    } else { // normal
                        next_dwell.time32 = next_spark.time32 - dwell_long;
                    }
                    dwell_us = 0;
                } else {
                    // this is when the dwell happens before the trigger i.e. after the spark
                    next_dwell.time32 = 0;
                    thistoothevents &= ~DWELL;
                    // always dwell here in case of rapid accel where stepback happened too fast
                    if (!spkcut_thresh) {
                        dwellsel = 1;
                        DWELL_COIL;
                    }
                }

                // and/or to schedule from the back of the previous spark (high speeds)
                // during the transition, both can happen harmlessly
                if ((dwell_long > (tmp_next_sparktime - 250)) || (tmp_next_sparktime < 251)) {
                    unsigned long tt;
                    if (ram4.ICIgnOption & 0x08) { // oddfire, grab the real period (expected period coming up)
                        tooth_diff_next = tooth_diff_last;
                    }

                    tt = tooth_diff_next - dwell_long;
                    if (ram4.adv_offset < 200) { // next-cyl
                        tt = tt + tmp_next_sparktime - next_spark.time32;
                    }

                    if ((unsigned int)(tt >> 16)) { // if (tt > 65535) {
                        // shouldn't happen.. must be extreme advance at low rpms
                        // fire dwell very shortly from now
                        dwell_us = 0;
                        dwellsel_next = 0;
                    } else {
                        dwell_us = (unsigned int)tt;

                        if (num_spk > 1) { // i.e. 2
                            if (tooth_no & 1) {
                                SSEM0;
                                dwellsel_next = 2;
                                CSEM0;
                            } else {
                                SSEM0;
                                dwellsel_next = 1;
                                CSEM0;
                            }
                        } else {
                            SSEM0;
                            dwellsel_next = 1;
                            CSEM0;
                        }
                    }
                } else {
                    dwell_us = 0;
                }
            }
        }

    /************* twin trigger ************ */
    } else if (spkmode == 14) { // twin trigger
        unsigned long tooth_diff_next;
        // here we'll handle the twin trigger almost like two independent 'distributors'
        // so odd or even is irrelevant

        if (tooth_diff_last_2) {
            tooth_diff_next = ((tooth_diff_this + tooth_diff_last) << 1) - (tooth_diff_last_1 + tooth_diff_last_2); // 1st deriv prediction
        } else if (tooth_diff_last) {
            tooth_diff_next = tooth_diff_this + tooth_diff_last;
        } else {
            tooth_diff_next = tooth_diff_this << 1; // would be a problem if oddfire. Condition avoided with enough skip_pulses
        }

        if (tooth_no == 1) {
            if ((coilsel) && (ram4.adv_offset < 200)) {
                // there's a spark pending, fire it
                SSEM0;
                TIE &= ~0x02;
                TFLG1 = 0x02;
                CSEM0;
                FIRE_COIL;
            }

            next_spark.time32 = muldiv(dizzy_scaler[0], tooth_diff_next);

            if (next_spark.time32 < 110) { // same number as below
                next_spark.time32 = 111; // prevent it getting too close to trigger
            }
      
            thistoothevents |= SPARK;
            SSEM0;
            tmpcoilsel = 1;
            CSEM0;
          
            if ((ram4.dwellmode & 3) != 2) {
                unsigned long tmp_next_sparktime;
                SSEM0;
                tmpdwellsel = 1;
                CSEM0;

                tmp_next_sparktime = next_spark.time32;

                // choose to schedule dwell from this trigger (low speeds)
                if (next_spark.time32 > (dwell_long + 70)) {
                    // have time to schedule dwell from here
                    // this is for a dwell coming after this trigger and before the spark
                    thistoothevents |= DWELL;
                    next_dwell.time32 = next_spark.time32 - dwell_long;
                    dwell_us = 0;
                } else {
                    // this is when the dwell happens before the trigger i.e. after the spark
                    next_dwell.time32 = 0;
                    thistoothevents &= ~DWELL;
                    if (!spkcut_thresh) {
                        dwellsel = 1;
                        DWELL_COIL;
                    }
                }

                // and/or to schedule from the back of the previous spark (high speeds)
                // during the transition, both can happen harmlessly
                if ((dwell_long > (tmp_next_sparktime - 250)) || (tmp_next_sparktime < 251)) {
                    unsigned long tt;

                    tt = tooth_diff_next - dwell_long;
                    if (ram4.adv_offset < 200) { // next-cyl
                        tt = tt + tmp_next_sparktime - next_spark.time32;
                    }

                    if ((unsigned int)(tt >> 16)) { // if (tt > 65535)
                        // shouldn't happen.. must be extreme advance at low rpms
                        // fire dwell very shortly from now
                        dwell_us = 0;
                        dwellsel_next = 0;
                    } else {
                        dwell_us = (unsigned int)tt;
                        dwellsel_next = 1;
                    }
                } else {
                    dwell_us = 0;
                }
            }

        } else if (tooth_no == 2) { // tooth_no == 2
            // For the second spark output we'll use the rotary dwell and spark timers
            // to give an independent spark system

            if ((rotaryspksel) && (ram4.adv_offset < 200)) {
                // there's a spark pending, fire it
                SSEM0;
                TIE &= ~0x08;
                TFLG1 = 0x08;
                CSEM0;
                FIRE_COIL_ROTARY;
            }

            next_spk_trl.time32 = muldiv(dizzy_scaler[1], tooth_diff_next);

            if (next_spk_trl.time32 < 110) { // same number as below
                next_spk_trl.time32 = 111; // prevent it getting too close to trigger
            }
      
            thistoothevents |= ROTARY_SPK;
            SSEM0;
            rotaryspksel = 2; /* Rotary stuff is being reused here for twin trigger */
            CSEM0;
         
            if ((ram4.dwellmode & 3) != 2) {
                unsigned long tmp_next_sparktime;
                SSEM0;
                rotarydwlsel = 2; /* Rotary stuff being reused here too */
                CSEM0;

                tmp_next_sparktime = next_spk_trl.time32;

                // choose to schedule dwell from this trigger (low speeds)
                if (next_spk_trl.time32 > (dwell_long + 70)) {
                    // have time to schedule dwell from here
                    // this is for a dwell coming after this trigger and before the spark
                    thistoothevents |= ROTARY_DWL;
                    next_dwl_trl.time32 = next_spk_trl.time32 - dwell_long;
                    dwell_us2 = 0;
                } else {
                    // this is when the dwell happens before the trigger i.e. after the spark
                    next_dwl_trl.time32 = 0;
                    thistoothevents &= ~ROTARY_DWL;
                    if (!spkcut_thresh) {
                        rotarydwlsel = 2; /* Rotary stuff being reused here */
                        DWELL_COIL_ROTARY;
                    }
                }

                // and/or to schedule from the back of the previous spark (high speeds)
                // during the transition, both can happen harmlessly
                if ((dwell_long > (tmp_next_sparktime - 250)) || (tmp_next_sparktime < 251)) {
                    unsigned long tt;

                    tt = tooth_diff_next - dwell_long;
                    if (ram4.adv_offset < 200) { // next-cyl
                        tt = tt+ tmp_next_sparktime - next_spk_trl.time32;
                    }

                    if ((unsigned int)(tt >> 16)) { // if (tt > 65535) 
                        // shouldn't happen.. must be extreme advance at low rpms
                        // fire dwell very shortly from now
                        dwell_us2 = 0;
                    } else {
                        dwell_us2 = (unsigned int)tt;
                    }
                } else {
                    dwell_us2 = 0;

                }

            }
        }

    /**************************/
    } else if (spkmode == 31) { // fuel only
        /* do not set any spark stuff */

    /**************************/

    } else { // regular wheel modes

        if (next_spark.tooth == tooth_no) {
            tmpcoilsel = 0;
            thistoothevents |= SPARK;
            set_coil(&next_spark.coil, &tmpcoilsel);
            if (synch & (SYNC_WCOP | SYNC_WCOP2)) {
                if (flagbyte22 & FLAGBYTE22_ODDFIRECYL) {
                    tmpcoilsel = 0; /* wasted COP not permitted */
                } else if (num_spk == 4) {
                    tmp_coil = next_spark.coil + 2;
                    if (tmp_coil > 3) {
                        tmp_coil -= 4;
                    }
                    set_coil(&tmp_coil, &tmpcoilsel);
                } else if (num_spk == 6) {
                    tmp_coil = next_spark.coil + 3;
                    if (tmp_coil > 5) {
                        tmp_coil -= 6;
                    }
                    set_coil(&tmp_coil, &tmpcoilsel);
                } else if (num_spk == 8) {
                    tmp_coil = next_spark.coil + 4;
                    if (tmp_coil > 7) {
                        tmp_coil -= 8;
                    }
                    set_coil(&tmp_coil, &tmpcoilsel);
                } else if (num_spk == 10) {
                    tmp_coil = next_spark.coil + 5;
                    if (tmp_coil > 9) {
                        tmp_coil -= 10;
                    }
                    set_coil(&tmp_coil, &tmpcoilsel);
                } else if (num_spk == 12) {
                    tmp_coil = next_spark.coil + 6;
                    if (tmp_coil > 11) {
                        tmp_coil -= 12;
                    }
                    set_coil(&tmp_coil, &tmpcoilsel);
                } else if (num_spk == 14) {
                    tmp_coil = next_spark.coil + 7;
                    if (tmp_coil > 13) {
                        tmp_coil -= 14;
                    }
                    set_coil(&tmp_coil, &tmpcoilsel);
                } else if (num_spk == 16) {
                    tmp_coil = next_spark.coil + 8;
                    if (tmp_coil > 15) {
                        tmp_coil -= 16;
                    }
                    set_coil(&tmp_coil, &tmpcoilsel);
                }
            }
        }

        if ((next_dwell.tooth == tooth_no) && ((ram4.dwellmode & 3) != 2)) {
            thistoothevents |= DWELL;
            tmpdwellsel = 0;
            if (!(skipdwell[next_dwell.coil])) {
                /* In the normal case we set the dwell.
                    Only skipped if MAP phase sense and we've figured out phase and moved to COP */
                set_coil(&next_dwell.coil, &tmpdwellsel);
            }
            if (((synch & SYNC_WCOP) && (synch & SYNC_SEMI2)) || (synch & SYNC_WCOP2)) {
                if (flagbyte22 & FLAGBYTE22_ODDFIRECYL) {
                    tmpcoilsel = 0; /* wasted COP not permitted */
                } else if (num_spk == 4) {
                    tmp_coil = next_dwell.coil + 2;
                    if (tmp_coil > 3) {
                        tmp_coil -= 4;
                    }
                    set_coil(&tmp_coil, &tmpdwellsel);
                } else if (num_spk == 6) {
                    tmp_coil = next_dwell.coil + 3;
                    if (tmp_coil > 5) {
                        tmp_coil -= 6;
                    }
                    set_coil(&tmp_coil, &tmpdwellsel);
                } else if (num_spk == 8) {
                    tmp_coil = next_dwell.coil + 4;
                    if (tmp_coil > 7) {
                        tmp_coil -= 8;
                    }
                    set_coil(&tmp_coil, &tmpdwellsel);
                } else if (num_spk == 10) {
                    tmp_coil = next_dwell.coil + 5;
                    if (tmp_coil > 9) {
                        tmp_coil -= 10;
                    }
                    set_coil(&tmp_coil, &tmpdwellsel);
                } else if (num_spk == 12) {
                    tmp_coil = next_dwell.coil + 6;
                    if (tmp_coil > 11) {
                        tmp_coil -= 12;
                    }
                    set_coil(&tmp_coil, &tmpdwellsel);
                } else if (num_spk == 14) {
                    tmp_coil = next_dwell.coil + 7;
                    if (tmp_coil > 13) {
                        tmp_coil -= 14;
                    }
                    set_coil(&tmp_coil, &tmpdwellsel);
                } else if (num_spk == 16) {
                    tmp_coil = next_dwell.coil + 8;
                    if (tmp_coil > 15) {
                        tmp_coil -= 16;
                    }
                    set_coil(&tmp_coil, &tmpdwellsel);
                }
            }
        }

        /* now check for rotary spark/dwell teeth */
        if (((ram4.EngStroke & 0x03) == 0x03)) {
            if (next_spk_trl.tooth == tooth_no) {
                thistoothevents |= ROTARY_SPK;
                tmp_coil = next_spk_trl.coil; // word vs byte
                tmp_sel = 0;
                set_coil(&tmp_coil, &tmp_sel);
                SSEM0;
                rotaryspksel |= tmp_sel;
                CSEM0;
            }

            if ((next_dwl_trl.tooth == tooth_no) && (spkcut_thresh == 0)) {
                thistoothevents |= ROTARY_DWL;
                tmp_coil = next_dwl_trl.coil;
                tmp_sel = 0;
                set_coil(&tmp_coil, &tmp_sel);
                SSEM0;
                rotarydwlsel |= tmp_sel;
                CSEM0;
            }
        }
    }

    if (next_map_start_event.tooth == tooth_no) {
        thistoothevents |= MAPSTART;
    }

    if (pin_knock_out && (next_knock_start_event.tooth == tooth_no)) {
        thistoothevents |= KNOCKWINDOW;
    }

    if (glob_sequential & 0x3) {
        int i;

        fuel_flag = 0;
        for (i = 0; i < num_inj_events; i++) {
            int t;
            t = next_inj[i].tooth;
            if (t == tooth_no) {
                fuel_flag |= twopow[i];
                thistoothevents |= SEQFUEL;
            } else if (t == 0) {
                /* Fix a race that can cause there to never be a valid fuel event */
                SSEM0;
                next_inj[i].time = inj_events[i].time;
                next_inj[i].tooth = inj_events[i].tooth;
                CSEM0;

                if (next_inj[i].tooth == tooth_no) {
                    fuel_flag |= twopow[i];
                    thistoothevents |= SEQFUEL;
                }
            }
        }
 
        if (do_dualouts) {
            int start,end;

            if (((num_cyl <= 4) || ((num_cyl > 4) && (glob_sequential & SEQ_SEMI))) &&
                 !(ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3)) {
                start = num_inj_events;
                end = start << 1;
            } else {
                start = 8;
                end = 10;
            }

            for (i = start; i < end; i++) {
                int t;
                t = next_inj[i].tooth;
                if (t == tooth_no) {
                    fuel_flag |= twopow[i];
                    thistoothevents |= SEQFUEL;
                } else if (t == 0) {
                    /* Fix a race that can cause there to never be a valid fuel event */
                    SSEM0;
                    next_inj[i].time = inj_events[i].time;
                    next_inj[i].tooth = inj_events[i].tooth;
                    CSEM0;

                    if (next_inj[i].tooth == tooth_no) {
                        fuel_flag |= twopow[i];
                        thistoothevents |= SEQFUEL;
                    }
                }
            }
        }
    }

    if (next_fuel == tooth_no) {
        thistoothevents |= FUEL;
        // cumulative cycle no,
        cum_cycle++;
        if (((ram4.log_style2 & 0x18) == 0x10) && (sd_phase == 0x41)) {
            sd_phase++;         // trigger a new log block
        }
    }

    checkforpit0();     // check for pending interrupt

    /************************* set the timers here ********************/
    if (spkmode < 2) {
        unsigned int tc_tmp;
        // EDIS special case

    // EDIS Ignition Output
    // cyl n                                   cyl n+1
    // tdc                                        tdc
    //  |-------- dtpred ---------------------------|
    //  |-delay- __________                         |
    //  |       |          |                        |
    //  |       |---SAW----|                        |
    //  |       |          |                        |
    //  |_______|          |________________________|
    // TC0
    //
    // send SAW pulse after delay, so we don't send
    // SAW while still sparking (send at 64us atdc) <--- this doesn't work because PIP doesn't happen at TDC

        thistoothevents &= ~SPARK;
        thistoothevents &= ~DWELL;

        coilsel = 1;
        dwellsel = 1;

        // set delay (dwell)
        TC7 = (unsigned short)TC0this + 64;
        SSEM0;
        TFLG1 = 0x80;
        TIE |= 0x80;
        CSEM0;

        // set end of SAW (spark)

        // multispark EDIS
        if (spkmode == 1) {
            if (flagbyte4 & flagbyte4_first_edis) { // 1st SAW for multispk is 2048 us
                flagbyte4 &= ~flagbyte4_first_edis;
                tc_tmp = 2048;       //2048us  1024 if ever have 10 cyl
            } else if (outpc.rpm < 1200) {
                tc_tmp = dwell_us + 2048;      //2048us 1024 if ever have 10 cyl
            } else {
                tc_tmp = dwell_us;
            }
        } else {
            tc_tmp = dwell_us;
        }

        tc_tmp = TC7 + tc_tmp;

        TC1 = tc_tmp;
        SSEM0;
        TFLG1 = 0x02;
        TIE |= 0x02;
        CSEM0;

    } else if (spkmode == 31) {
        // Fuel only special case
        thistoothevents &= ~SPARK;
        thistoothevents |= FUEL;
        coilsel = 0;
        dwellsel = 0;
    }

    if (thistoothevents & FUEL) {

// trigger logger
        if (flagbyte0 & FLAGBYTE0_TRGLOG) {

            if (log_offset < 1022) {

//Here we use RPAGE for the tooth/trigger log FIXME HARDCODED due to lack of gcc understanding
            __asm__ __volatile__("ldab  %0\n"
                                 "pshb\n"
                                 "movb #0xf0, %0\n"
                                :
                                :"m"(RPAGE)
                                :"d"
                );

            __asm__ __volatile__("ldab  %1\n"
                                 "andb  #0xf\n"    // top 4 bits not used in trigger logger for consistency
                                 "stab  0,Y\n"
                                 "ldd  %2\n"
                                 "std  1,Y\n"
                                :
                                :"y"(log_offset + TRIGLOGBASE), "m"(*((unsigned char *) &dtpred + 1)),       // byte 3:**2**:1:0
                                 "m"(*((unsigned int *) &dtpred + 1))   // bytes 3:2:**1**:**0**
                                :"d"
                );

            __asm__ __volatile__("pulb\n"
                                 "stab %0\n"
                                :
                                :"m"(RPAGE)
                                :"d"
                );
            }

            log_offset += 3;

            if (log_offset > 1021) {
                flagbyte0 &= ~FLAGBYTE0_TRGLOG; // turn off logger
                outpc.status3 |= STATUS3_DONELOG;
            }
        }


/* experi - record period times */
        if (ram5.u08_debug38 & 2) {
            int x;
            x = ram4.fire[fuel_cntr] - 1;
            if (x < 16) { // range check - up to 8cyl only
                cyl_time[x] = dtpred_adder;
            }
        }

        if (fuel_cntr >= no_triggers - 1) {
            fuel_cntr = 0;
            flagbyte20 |= FLAGBYTE20_CCT;
        } else {
            fuel_cntr++;
        }
        next_fuel = trigger_teeth[(unsigned) fuel_cntr];
        dtpred_last3 = dtpred_last2;
        dtpred_last2 = dtpred_last;
        dtpred_last = dtpred;
        dtpred = dtpred_adder;
        dtpred_adder = 0;
        synch |= SYNC_RPMCALC;
        flagbyte15 |= FLAGBYTE15_MAP_AVG_TRIG;
        flagbyte20 |= FLAGBYTE20_MAF_AVG_TRIG;
    }

    if (synch & SYNC_SEMI) {
        // not ready to actually fire fuel or spark just yet, but wanted to get rpms
        return;
    }

    if (thistoothevents & SPARK) {
        /* used by all modes */
        unsigned int lp = 0;
TDE_S:;
        // XGATE lockup detection - we hope this never happens
        xgate_deadman++;
        if (xgate_deadman > 3) {
            unsigned char save_rpage;
            save_rpage = RPAGE;
            config_xgate();
            RPAGE = save_rpage;
            xgate_deadman = 0;
        }

        if (spk_crk_targ) {
            /* Ensure we always DO fire the coil even if counter is just ahead of trigger
               requires a next-cylinder CAS pattern */
            if (coilsel) {  // ought to be zero, so fire off that late coil
                FIRE_COIL;
            }
            SSEM0;
            if (xgspkq[0].sel) {
                xgspkq[1].sel = tmpcoilsel;
                xgspkq[1].cnt = spk_crk_targ;
            } else {
                xgspkq[0].sel = tmpcoilsel;
                xgspkq[0].cnt = spk_crk_targ;
            }
            CSEM0;
        } else if (next_spark.time32 < 50) {
            SSEM0;
            TIE &= ~0x02;
            TFLG1 = 0x02;
            CSEM0;
            coilsel = tmpcoilsel;
            FIRE_COIL;
        } else if (next_spark.time32 < 250) {
            unsigned int tc_tmp;
            tc_tmp = (unsigned short) (TC0this + next_spark.time16_low);
            TC1 = tc_tmp;
            coilsel = tmpcoilsel;
            SSEM0;
            TFLG1 = 0x02;
            TIE |= 0x02;
            CSEM0;
        } else {
            /* use queue - allows up to 8 seconds */
            unsigned int time_mms, time_us;
//            time_mms = (next_spark.time32 / 128);
            time_mms = muldiv(512, next_spark.time32); // same effect but should be faster
            if (time_mms > 4) {
                time_mms-= 4;
            } else {
                time_mms = 1;
            }
            time_us = TC0this + next_spark.time16_low;
            if (spkq[0].sel == 0) {
                spkq[0].sel = tmpcoilsel;
                spkq[0].time_us = time_us;
                spkq[0].time_mms = time_mms;
            } else {
                spkq[1].sel = tmpcoilsel;
                spkq[1].time_us = time_us;
                spkq[1].time_mms = time_mms;
            }
        }

        if (spkmode != 14) {
            /* next one */
            if (next_spark.coil == (unsigned int)(no_triggers - 1)) {
                next = 0;
            } else {
                next = next_spark.coil + 1;
            }
            next_spark.time = spark_events[next].time;
            next_spark.tooth = spark_events[next].tooth;
            next_spark.coil = spark_events[next].coil;
            next_spark.ftooth = spark_events[next].ftooth;
            next_spark.fs = spark_events[next].fs;

            /* re-check for a second event from same tooth */
            if ((next_spark.tooth == tooth_no) && (lp < 2)) {
                lp++;
                tmpcoilsel = 0;
                set_coil(&next_spark.coil, &tmpcoilsel);
                goto TDE_S;
            }
        }
    }

/* the XGATE tooth counting spark/dwell scheduling is disabled.
   It does work, but in the rare case the engine stops after the coil has been turned on,
   it will hang on until the stall timer kicks in.
   This looks like a pain to solve, in effect the spark needs to be timed after the physical
   turn on. However, that would require all the code below to be duplicated on the XGATE, or
   use up the final XGATE7 interrupt. Thoughts?
*/

    if (thistoothevents & DWELL) {
        /* not used by dizzy or twin-trigger which schedule dwell after spark in XGATE */
        unsigned int lp = 0;
TDE_D:;
        if (dwl_crk_targ) {
            /* Ensure we always DO fire the coil even if counter is just ahead of trigger
               requires a next-cylinder CAS pattern */
            SSEM0;
            if (xgdwellq[0].sel) {
                xgdwellq[1].sel = tmpdwellsel;
                xgdwellq[1].cnt = dwl_crk_targ;
            } else {
                xgdwellq[0].sel = tmpdwellsel;
                xgdwellq[0].cnt = dwl_crk_targ;
            }
            CSEM0;
        } else if (next_dwell.time32 < 50) { /* this case shouldn't happen */
            /* so short fire immediately */
            SSEM0;
            TIE &= ~0x80;
            TFLG1 = 0x80;
            CSEM0;
            dwellsel = tmpdwellsel;
            DWELL_COIL;
        } else if (next_dwell.time32 < 250) {
            /* set timer in here */
            TC7 = (unsigned short) (TC0this + next_dwell.time16_low);
            dwellsel = tmpdwellsel;
            SSEM0;
            TFLG1 = 0x80;
            TIE |= 0x80;
            CSEM0;
        } else {
            /* use queue - allows up to 8 seconds */
            unsigned int time_mms, time_us;
//            time_mms = (next_dwell.time32 / 128);
            time_mms = muldiv(512, next_dwell.time32); // same effect but should be faster
            if (time_mms > 4) {
                time_mms-= 4;
            } else {
                time_mms = 1;
            }
            time_us = TC0this + next_dwell.time16_low;
            if (dwellq[0].sel == 0) {
                dwellq[0].sel = tmpdwellsel;
                dwellq[0].time_us = time_us;
                dwellq[0].time_mms = time_mms;
            } else {
                dwellq[1].sel = tmpdwellsel;
                dwellq[1].time_us = time_us;
                dwellq[1].time_mms = time_mms;
            }
        }

        if (spkmode != 14) {
            /* next one */
            if (next_dwell.coil == (unsigned int)(no_triggers - 1)) {
                next = 0;
            } else {
                next = next_dwell.coil + 1;
            }
            next_dwell.time = dwell_events[next].time;
            next_dwell.tooth = dwell_events[next].tooth;
            next_dwell.coil = dwell_events[next].coil;
        
            /* re-check for a second event from same tooth */
            if ((next_dwell.tooth == tooth_no) && ((ram4.dwellmode & 3) != 2)
                && (lp < 2)) {
                lp++;
                tmpdwellsel = 0;
                set_coil(&next_dwell.coil, &tmpdwellsel);
                goto TDE_D;
            }
        }
    }

    if (thistoothevents & ROTARY_DWL) {
        if (next_dwl_trl.time32 < 110) {
            SSEM0;
            TIE &= ~0x40;
            TFLG1 = 0x40;       // clear rot dwell OC interrupt flag
            CSEM0;
            DWELL_COIL_ROTARY;
        } else if (!next_dwl_trl.time16_high) {
            TC6 = (unsigned short) (TC0this + next_dwl_trl.time16_low);
            SSEM0;
            TFLG1 = 0x40;
            TIE |= 0x40;
            CSEM0;
        } else {
            wheeldec_ovflo |= OVFLO_ROT_DWL;
            dwl_time_ovflo_trl.time_32_bits = TC0_32bits + next_dwl_trl.time32;
        }
        if (spkmode != 14) {
            if (next_dwl_trl.coil == (unsigned int)((num_cyl<<1) - 1)) {
                next = num_cyl;
            } else {
                next = next_dwl_trl.coil + 1;
            }

            next_dwl_trl.time = dwell_events[next].time;
            next_dwl_trl.tooth = dwell_events[next].tooth;
            next_dwl_trl.coil = dwell_events[next].coil;
        }
    }

    if (thistoothevents & ROTARY_SPK) {
        if (next_spk_trl.time32 < 110) {
            SSEM0;
            TIE &= ~0x08;
            TFLG1 = 0x08;
            CSEM0;
            FIRE_COIL_ROTARY;
        } else if (!next_spk_trl.time16_high) {
            TC3 = (unsigned short) (TC0this + next_spk_trl.time16_low);
            SSEM0;
            TFLG1 = 0x08;
            TIE |= 0x08;
            CSEM0;
        } else {
            wheeldec_ovflo |= OVFLO_ROT_SPK;
            spk_time_ovflo_trl.time_32_bits = TC0_32bits + next_spk_trl.time32;
        }

        if (spkmode != 14) {
            if (next_spk_trl.coil == (unsigned int)((num_cyl << 1) - 1)) {
                next = num_cyl;
            } else {
                next = next_spk_trl.coil + 1;
            }

            next_spk_trl.time = spark_events[next].time;
            next_spk_trl.tooth = spark_events[next].tooth;
            next_spk_trl.coil = spark_events[next].coil;
        }
    }

    if (thistoothevents & MAPSTART) {
        if ((next_map_start_event.evnum == 0) && (flagbyte0 & FLAGBYTE0_MAPLOGARM)) {
            flagbyte0 &= ~FLAGBYTE0_MAPLOGARM;
            flagbyte0 |= FLAGBYTE0_MAPLOG;
            do_maplog_rpm(); // starts log by recording rpm
        }
        if ((next_map_start_event.evnum == 0) && (flagbyte0 & FLAGBYTE0_MAFLOGARM)) {
            flagbyte0 &= ~FLAGBYTE0_MAFLOGARM;
            flagbyte0 |= FLAGBYTE0_MAFLOG;
            do_maplog_rpm(); // starts log by recording rpm
        }
        map_start_countdown = next_map_start_event.time;
        map_window_set = next_map_start_event.map_window_set;
        if (next_map_start_event.evnum == no_triggers - 1) {
            next = 0;
        } else {
            next = next_map_start_event.evnum + 1;
        }

        next_map_start_event.tooth = map_start_event[next].tooth;
        next_map_start_event.time = map_start_event[next].time;
        next_map_start_event.map_window_set =
            map_start_event[next].map_window_set;
        next_map_start_event.evnum = map_start_event[next].evnum;
        map_deadman++;
    }

    if (thistoothevents & KNOCKWINDOW) {
        unsigned char x;
        if (knock_state == 0) {
            /* Shouldn't happen, but don't clash with SPI comms */
            knock_start_countdown = next_knock_start_event.time;
            knock_window_set = next_knock_start_event.map_window_set;
            knock_chan_tmp = next_knock_start_event.evnum;
        }
        if (next_knock_start_event.evnum == no_triggers - 1) {
            next = 0;
        } else {
            next = next_knock_start_event.evnum + 1;
        }

        next_knock_start_event.tooth = knock_start_event[next].tooth;
        next_knock_start_event.time = knock_start_event[next].time;
        next_knock_start_event.map_window_set =
            knock_start_event[next].map_window_set;
        next_knock_start_event.evnum = knock_start_event[next].evnum;
        if (ram5.knock_conf & 0x80) {
            x = ram4.fire[next_knock_start_event.evnum] - 1;
        } else {
            x = 0;
        }
        knock_gain = ram5.knock_gain[x] & 0x3f;
        knock_chan = ram5.knock_sens[x] & 0x01;
    }

    /* apply any rev limiting fuel cut */
    if (skipinj_revlim) {
        skipinj |= skipinj_revlim;
    }
    /* injector disabling test mode */
    if (testmode_glob == 4) {
        skipinj |= skipinj_test;
    }
    /* apply any overrun fuel cut */
    if (skipinj_overrun) {
        skipinj |= skipinj_overrun;
    }
    /* apply any shifter fuel cut */
    if (skipinj_shifter) {
        skipinj |= skipinj_shifter;
    }
    /* apply any pit limiter fuel cut */
    if (skipinj_pitlim) {
        skipinj |= skipinj_pitlim;
    }

    if (ram4.hardware & HARDWARE_MS3XFUEL) {
        if (thistoothevents & SEQFUEL) {
            int i;

            for (i = 0; i < NUM_TRIGS; i++) {
                unsigned int bitset, xref;
                bitset = twopow[i];

                if (skipinj & bitset) {
                    skipinj &= ~bitset;
                } else {
                    xref = inj_cnt_xref[i];
                    if (seq_pw[xref] != 0) {

                        if (fuel_flag & bitset) {
                            // ALS fuel cut
                            if (fuel_cuty) {
                                fuel_cuti++;
                                if (fuel_cuti > fuel_cuty) {
                                    fuel_cuti = 1;
                                }
                                if (fuel_cutx >= fuel_cuti) {
                                    goto ALS_FUEL_CUT;
                                }
                            }
                            SSEM0;
                            /* Schedule xref'd timer */
                            inj_cnt[xref] = next_inj[i].time;
                            CSEM0X; // without X gcc tries to use "Z"
                            flowsum_accum[injch[xref]] += rawpw[xref]; // fuel flow
                        }
                    } 
                }
ALS_FUEL_CUT:;
            }
        }
    } else {
        if (thistoothevents & SEQFUEL) {
            int i;

            if (num_inj_events /*>= 2*/) {
                for (i = 0; i < num_inj_events; i++) {
                    unsigned int array_elem, bitset;

                    array_elem = i & 1;

                    if (seq_pw[array_elem+8]) {
                        bitset = twopow[i];

                        if (skipinj & bitset) {
                            skipinj &= ~bitset;
                        } else if (fuel_flag & bitset) {
                            SSEM0;
                            inj_cnt[array_elem+8] = next_inj[i].time;
                            CSEM0;
                            flowsum_accum[injch[array_elem+8]] += rawpw[array_elem+8]; // fuel flow
                        }
                    }
                }
            }
        }
    }

    /* Update event table without clobbering events on higher channels
       that were scheduled for same tooth. */
    if (thistoothevents & SEQFUEL) {
        int i, j;

        for (i = 0; i < num_inj_events; i++) {
            if (next_inj[i].tooth == tooth_no) {
                /* update previous injector event to reduce risk of double scheduling*/
                if (i == 0) {
                    j = num_inj_events - 1;
                } else {
                    j = i - 1;
                }
                SSEM0;
                next_inj[j].time = inj_events[j].time;
                next_inj[j].tooth = inj_events[j].tooth;
                CSEM0;
            }
        }

        if (do_dualouts) {
            int start,end;

            if (((num_cyl <= 4) || ((num_cyl > 4) && (glob_sequential & SEQ_SEMI))) &&
                 !(ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3)) {
                start = num_inj_events;
                end = start << 1;
            } else {
                start = 8;
                end = 10;
            }

            for (i = start; i < end; i++) {
                if (next_inj[i].tooth == tooth_no) {
                    /* update previous injector event to reduce risk of double scheduling*/
                    if (i == start) {
                        j = end - 1;
                    } else {
                        j = i - 1;
                    }
                    SSEM0;
                    next_inj[j].time = inj_events[j].time;
                    next_inj[j].tooth = inj_events[j].tooth;
                    CSEM0;
                }
            }
        }
    }

    

    if (thistoothevents & FUEL) {
        goto START_INJ;
    }
    goto IC_EXIT;

    //*******************************************************************************

  START_INJ:
    // Set up for Injector squirt(s)
    if ((igncount == 0) && ((ram4.feature3 & 2) == 0)) {
        asecount++;
    }
    egocount++;
    cranktpcnt++;

    if (ram4.feature7 & 0x08) {
        tpsaclk++; // AE in ignition events
    }

    staged_num_events++;

    // Turn on fuel pump if not already
    if ((flagbyte6 & FLAGBYTE6_DONEINIT) && (!(outpc.engine & ENGINE_READY))) {
        outpc.engine |= ENGINE_READY;   // set engine running
        fuelpump_prime();
    }

    //do tacho output
    if ((ram4.tacho_opt2 & 0xc0) == 0x80) { /* On and Fixed */
        if (ram4.tacho_opt2 & 0x20) {
            if (flagbyte20 & FLAGBYTE20_TO) {
                flagbyte20 &= ~FLAGBYTE20_TO;
                tacho_targ = lowres_ctr;
            } else {
                flagbyte20 |= FLAGBYTE20_TO;
                tacho_targ = 1; // i.e. turn off asap
                goto NO_TACHSET;
            }
        } else {
            tacho_targ = lowres_ctr >> 1;
        }
        SSEM0;
        *port_tacho |= pin_tacho;
        CSEM0;
    } else if ((ram4.tacho_opt2 & 0xc0) == 0x00) { /* Off */
        /* still want tacho_targ set for inj LED */
        tacho_targ = lowres_ctr >> 1;
    }
    /* In variable mode, tacho and inj LED are handled by SW PWM */
NO_TACHSET:
    lowres_ctr = 0;

    SSEM0;
    *pPTMpin3 |= 0x08; /* Turn on Inj LED ... more like tacho LED now */
    CSEM0;

    if (water_pw) {
        water_pw_cnt = water_pw;
    }

    if ((ram4.EAEOption & 0x07) > 2) {
        sum_dltau[0] += dtpred;
        sum_dltau[1] = sum_dltau[0];
    }

    if ((outpc.engine & ENGINE_CRANK) || (glob_sequential & 0x3))
        goto SCHED_SQUIRT;

    igncount++;
    if (ram4.feature3 & 0x08) {
        if (tooth_no == tooth_init) {   // wait for chosen tooth
            if (!(flagbyte3 & flagbyte3_toothinit)) {
                flagbyte3 |= flagbyte3_toothinit;
                igncount = 0;
                altcount = 1;
            }
        }
    } else {
        flagbyte3 |= flagbyte3_toothinit;
    }

    if (divider == 1) {
        goto SCHED_SQUIRT;
    } else {
        if (igncount < divider) {
            goto IC_EXIT;   // skip Divider tach pulses
        }
    }

SCHED_SQUIRT:
    if ((ram4.EAEOption & 0x07) == 1) {
        WF1 += AWA1;
        if (SOA1 <= WF1) {
            WF1 -= SOA1;
        } else {
            WF1 = 0;
        }
        WF2 += AWA2;
        if (SOA2 <= WF2) {
            WF2 -= SOA2;
        } else {
            WF2 = 0;
        }
    }
    igncount = 0;
    flagbyte5 |= FLAGBYTE5_RUN_XTAU;

    if (outpc.status3 & STATUS3_CUT_FUEL) {
        outpc.pw1 = 0;          // let world know injectors are off
        outpc.pw2 = 0;
        goto IC_EXIT;
    }

    if (glob_sequential & 0x3) {
        outpc.pw1 = pwcalc1;
        outpc.pw2 = pwcalc2;
        goto IC_EXIT;
    }

    if (outpc.engine & ENGINE_CRANK) {  // if engine cranking
        flagbyte3 &= ~flagbyte3_toothinit;
        if ((!(flagbyte3 & flagbyte3_toothinit))
            && (ram4.Alternate & 0x02)) {
            // alternate during cranking option
            goto DO_ALT;
        }
        sched_both = 1;
        goto SCHED1;
    }
    // run mode

    if (!(ram4.Alternate & 0x01)) {     // if no alternate option (i.e. simultaneous)
        sched_both = 1;
        goto SCHED1;
    }

DO_ALT:
    sched_both = 0;
    altcount = 1 - altcount;
    if (altcount)
        goto SCHED2;

SCHED1:
    /* Catch a rare race condition that can cause semi-sequential
     * to pick the wrong tooth
     */
    if ((ram4.feature3 & 0x08) && (tooth_init != tooth_no)) {
        flagbyte3 &= ~flagbyte3_toothinit;
        sched_both = 1;
    }
    // Turn On Inj1

    // Set up to turn Off Inj1 when get to pw us
    outpc.pw1 = pwcalc1;
    igncount = 0;

    if (skipinj & 1) {
        skipinj &= ~1;
    } else {
        if (ram4.hardware & HARDWARE_MS3XFUEL) {
            int i;

            for(i = 0; i < num_inj>>1; i++) {
                if (seq_pw[i]) {
                    inj_cnt[i] = 1;
                    flowsum_accum[injch[i]] += rawpw[i]; // fuel flow
                }
            }

            if ((do_dualouts) &&
               ((num_cyl > 4) || (ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3))) {
                if (seq_pw[8]) {
                    inj_cnt[8] = 1;
                    flowsum_accum[injch[8]] += rawpw[8]; // fuel flow
                }
            } else if ((do_dualouts) && (num_cyl <= 4)) {
                for (i = num_inj; i < (num_inj + (num_inj >> 1)); i++) {
                    if (seq_pw[i]) {
                        inj_cnt[i] = 1;
                        flowsum_accum[injch[i]] += rawpw[i]; // fuel flow
                    }
                }
            }
        } else {
            if (pwcalc1) {
                seq_pw[8] = pwcalc1;
                outpc.pwseq[8] = pwcalc1;
                inj_cnt[8] = 1;
                flowsum_accum[injch[8]] += rawpw[8]; // fuel flow
            } else {
                outpc.pwseq[8] = 0;
            }
        }
    }

    if (!sched_both) {
        goto IC_EXIT;
    }

  SCHED2:
    // Set up to turn Off Inj2 when get to pw us
    outpc.pw2 = pwcalc2;

    if (skipinj & 2) {
        skipinj &= ~2;
    } else {
        if (ram4.hardware & HARDWARE_MS3XFUEL) {
            int i;

            for(i = num_inj>>1; i < num_inj; i++) {
                if (seq_pw[i]) {
                    inj_cnt[i] = 1;
                    flowsum_accum[injch[i]] += rawpw[i]; // fuel flow

                }
            }
            if ((do_dualouts) &&
               ((num_cyl > 4) || (ram4.staged_extended_opts & STAGED_EXTENDED_USE_V3))) {
                if (seq_pw[9]) {
                    inj_cnt[9] = 1;
                }
            } else if ((do_dualouts) && (num_cyl <= 4)) {
                for (i = (num_inj + (num_inj >> 1)); i < num_inj<<1; i++) {
                    if (seq_pw[i]) {
                        inj_cnt[i] = 1;
                        flowsum_accum[injch[i]] += rawpw[i]; // fuel flow
                    }
                }
            }
        } else {
            if (pwcalc2) {
                seq_pw[9] = pwcalc2;
                outpc.pwseq[9] = pwcalc2;
                inj_cnt[9] = 1;
                flowsum_accum[injch[9]] += rawpw[9]; // fuel flow
            } else {
                outpc.pwseq[9] = 0;
            }
        }
    }

IC_EXIT:
    if (false_mask_crk) {
        // Set up to re-enable IC interrupt in Timer ISR after a part of the time
        //  to next IC has elapsed to avoid noise false interrupts
        t_enable_IC = ltch_lmms + false_mask_crk;
        SSEM0;
        TIE &= ~0x01;           // disable interrupt
        TFLG1 = 0x01;           // clear flag
        CSEM0;
    } else {
        t_enable_IC = 0xffffffff;       // keep int enabled
        SSEM0;
        TIE |= 0x01;            // ensure it is still enabled
        CSEM0;
    }

    return;
}
