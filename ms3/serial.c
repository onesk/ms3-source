/* $Id: serial.c,v 1.92.4.4 2015/05/20 11:20:54 jsmcortina Exp $
 * Copyright 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * Origin: James Murray
 * Majority: James Murray
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
 */

/* mainloop portion of serial processing */

#include "ms3.h"

#define GWR(a) g_write_generic(a, &x, sizeof(a));

#define DB_BUFADDR 0xf900
#define DB_BUFSIZE 0x0300 /* was 0x700 */

void g_write_generic(unsigned long val, unsigned int* ad, unsigned char size)
{
    if (size == 1) {
        g_write8((unsigned char)val, *ad);
        *ad += 1;
    } else if (size == 2) {
        g_write16((unsigned int)val, *ad);
        *ad += 2;
    } else if (size == 4) {
        DISABLE_INTERRUPTS;
        g_write32(val, *ad);
        ENABLE_INTERRUPTS;
        *ad += 4;
    }
    return;
}

void serial()
{
    unsigned int size, ad, x, r_offset, r_size;
    unsigned long crc;
    unsigned char cmd, r_canid, r_table, rp, compat = 0;

    /* Not necessarily the best place to copy, but makes it higher priority. */

    if (flagbyte22 & FLAGBYTE22_CANGPS) {
        volatile signed char tmp_gps_latdeg;
        volatile unsigned char tmp_gps_latmin;
        volatile unsigned int tmp_gps_latmmin;
        volatile unsigned char tmp_gps_londeg, tmp_gps_lonmin;
        volatile unsigned int tmp_gps_lonmmin;
        volatile unsigned char tmp_gps_outstatus;
        volatile signed char tmp_gps_altk;
        volatile signed int tmp_gps_altm;
        volatile unsigned int tmp_gps_speedkm, tmp_gps_course;

        DISABLE_INTERRUPTS;
        flagbyte22 &= ~FLAGBYTE22_CANGPS;
        tmp_gps_latdeg = datax1.gps_latdeg;
        tmp_gps_latmin = datax1.gps_latmin;
        tmp_gps_latmmin = datax1.gps_latmmin;
        tmp_gps_londeg = datax1.gps_londeg;
        tmp_gps_lonmin = datax1.gps_lonmin;
        tmp_gps_lonmmin = datax1.gps_lonmmin;
        tmp_gps_outstatus = datax1.gps_outstatus;
        tmp_gps_altk = datax1.gps_altk;
        tmp_gps_altm = datax1.gps_altm;
        tmp_gps_speedkm = datax1.gps_speedkm;
        tmp_gps_course = datax1.gps_course;
        ENABLE_INTERRUPTS;

        /* See if anything changed */
        if ((outpc.gps_outstatus != tmp_gps_outstatus)
            || (outpc.gps_latdeg != tmp_gps_latdeg)
            || (outpc.gps_latmin != tmp_gps_latmin)
            || (outpc.gps_latmmin != tmp_gps_latmmin)
            || (outpc.gps_londeg != tmp_gps_londeg)
            || (outpc.gps_lonmin != tmp_gps_lonmin)
            || (outpc.gps_lonmmin != tmp_gps_lonmmin)
            || (outpc.gps_altk != tmp_gps_altk)
            || (outpc.gps_altm != tmp_gps_altm)
            || (outpc.gps_speedkm != tmp_gps_speedkm)
            || (outpc.gps_course != tmp_gps_course)
            ) {

            flagbyte22 |= FLAGBYTE22_GPSLOG;
            outpc.gps_latdeg = tmp_gps_latdeg;
            outpc.gps_latmin = tmp_gps_latmin;
            outpc.gps_latmmin = tmp_gps_latmmin;
            outpc.gps_londeg = tmp_gps_londeg;
            outpc.gps_lonmin = tmp_gps_lonmin;
            outpc.gps_lonmmin = tmp_gps_lonmmin;
            outpc.gps_outstatus = tmp_gps_outstatus;
            outpc.gps_altk = tmp_gps_altk;
            outpc.gps_altm = tmp_gps_altm;
            outpc.gps_speedkm = tmp_gps_speedkm;
            outpc.gps_course = tmp_gps_course;
        }
    }

    ck_log_clr();
    chk_crc();
    if ((!(flagbyte17 & FLAGBYTE17_CANSUSP)) && (ram5.can_enable & CAN_ENABLE_ON) /* overall enables */
        && (ram5.can_outpc_gp[0] & 0x80)) { /* master enable */
        can_bcast_outpc_cont(); /* makes it run more than once per mainloop to spread CANbus load */
    }

    GPAGE = 0x13;
    ad = 0xf002;

    if ((txmode == 0) && datax1.newbaud) {
        if ((datax1.newbaud & 0x50) == 0x50) {
            if (datax1.newbaud == 0x51) {
                sci_baud(ram5.baudhigh);
            } else {
                sci_baud(ram4.baud);
            }
        }
        datax1.newbaud = 0;
    }

#if 0
    if ((MONVER >= 0x380) && (MONVER <= 0x38f)) {
        /* Only on MS3pro */
        if ((flagbyte2 & FLAGBYTE2_SCI_CONFIRMED) && (sci_lock_timer > 10)) {
            flagbyte2 &= ~FLAGBYTE2_SCI_CONFIRMED;
            /* had locked but nothing for over 10 seconds, unlock */
            (void)SCI0SR1; // clear any flags
            (void)SCI0DRL;
            SCI0CR1 = 0x00;
            SCI0CR2 = 0x24;             // TIE=0,RIE = 1; TE=0,RE =1

            /* now set SCI1 the same */
            (void)SCI1SR1; // clear any flags
            (void)SCI1DRL;
            SCI1CR1 = 0x00;
            SCI1CR2 = 0x24;             // TIE=0,RIE = 1; TE=0,RE =1
        }
    }
#endif

    if (flagbyte21 & FLAGBYTE21_KILL_SRL0) {
        unsigned int lmms_tmp, timeout;
        if (srlerr0 == 7) {
            timeout = 7812; // 1 second
        } else {
            timeout = 390; // about 50ms seconds of timeout
        }
        lmms_tmp = (unsigned int)lmms;

        if ((lmms_tmp - srl_timeout0) > timeout) {
            unsigned char dummy;
            if (SCI0SR1 & 0x20) { // data available
                dummy = SCI0DRL; // gobble it up and wait again            
                srl_timeout0 = (unsigned int)lmms;
                return;
            }
            dummy = SCI0SR1; // step 1, clear any pending interrupts
            dummy = SCI0DRL; // step 2
            SCI0CR2 |= 0x24;        // rcv, rcvint re-enable

            rcv_timeout0 = 0xFFFFFFFF;
            flagbyte21 &= ~FLAGBYTE21_KILL_SRL0;
            if (srlerr0 == 1) {
                g_write8(0x8c, 0xf002); // parity error
            } else if (srlerr0 == 2) {
                g_write8(0x8d, 0xf002); // framing error
            } else if (srlerr0 == 3) {
                g_write8(0x8e, 0xf002); // noise
            } else if (srlerr0 == 4) {
                g_write8(0x81, 0xf002); // overrun
            } else if (srlerr0 == 5) {
                g_write8(0x8f, 0xf002); // Transmit txmode range
            } else if (srlerr0 == 6) {
                g_write8(0x84, 0xf002); // Out of range
            } else if (srlerr0 == 7) {
                unsigned int z;
                const char errstr[] = "Too many bad requests! Stop doing that!";
                g_write8(0x91, 0xf002); // Too many!

                for (z = 0; z < sizeof(errstr); z++) {
                    g_write8(errstr[z], 0xf003 + z); 
                }
                txgoal = sizeof(errstr);
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            } else {
                g_write8(0x90, 0xf002); // unknown
            }
            txgoal = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            port_sci = &SCI0BDH; // force it to be sent to SCI0 as that's where the error originated
            goto EXIT_SERIAL;
        }
    }

    if (flagbyte21 & FLAGBYTE21_KILL_SRL1) {
        unsigned int lmms_tmp, timeout;
        if (srlerr1 == 7) {
            timeout = 7812; // 1 second
        } else {
            timeout = 390; // about 50ms seconds of timeout
        }
        lmms_tmp = (unsigned int)lmms;

        if ((lmms_tmp - srl_timeout1) > timeout) {
            unsigned char dummy;
            if ((MONVER >= 0x380) && (MONVER <= 0x38f)) {
                if (SCI1SR1 & 0x20) { // data available
                    dummy = SCI1DRL; // gobble it up and wait again            
                    srl_timeout1 = (unsigned int)lmms;
                    return;
                }

                dummy = SCI1SR1; // step 1, clear any pending interrupts
                dummy = SCI1DRL; // step 2
                SCI1CR2 |= 0x24;        // rcv, rcvint re-enable
            }

            rcv_timeout1 = 0xFFFFFFFF;
            flagbyte21 &= ~FLAGBYTE21_KILL_SRL1;
            if (srlerr1 == 1) {
                g_write8(0x8c, 0xf002); // parity error
            } else if (srlerr1 == 2) {
                g_write8(0x8d, 0xf002); // framing error
            } else if (srlerr1 == 3) {
                g_write8(0x8e, 0xf002); // noise
            } else if (srlerr1 == 4) {
                g_write8(0x81, 0xf002); // overrun
            } else if (srlerr1 == 5) {
                g_write8(0x8f, 0xf002); // Transmit txmode range
            } else if (srlerr1 == 6) {
                g_write8(0x84, 0xf002); // Out of range
            } else if (srlerr1 == 7) {
                unsigned int z;
                const char errstr[] = "Too many bad requests! Stop doing that!";
                g_write8(0x91, 0xf002); // Too many!

                for (z = 0; z < sizeof(errstr); z++) {
                    g_write8(errstr[z], 0xf003 + z); 
                }
                txgoal = sizeof(errstr);
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            } else {
                g_write8(0x90, 0xf002); // unknown
            }
            txgoal = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            port_sci = &SCI1BDH; // force it to be sent to SCI1 as that's where the error originated
            goto EXIT_SERIAL;
        }
    }
    
    /* check for under-run or old commands */
    if (rxmode) {
        unsigned long lmms_tmp, rtime;

        DISABLE_INTERRUPTS;
        lmms_tmp = lmms;
        ENABLE_INTERRUPTS;

        if (port_sci == &SCI1BDH) {
            rtime = rcv_timeout1;
        } else {
            rtime = rcv_timeout0;
        }

        if (lmms_tmp > rtime) {
            /* had apparent under-run, check for old commands */
            cmd = g_read8(0xf000);
            if ((rxmode == 1) && (cmd == 'Q')) {
                ad = 0xf000;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'S')) {
                ad = 0xf000;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'C')) {
                ad = 0xf000;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'F')) {
                ad = 0xf000;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'I')) {
                ad = 0xf000;
                size = 1;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'A')) {
                ad = 0xf000;
                size = 1;
                compat = 1; // full A
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 1) && (cmd == 'D')) {
                unsigned int cnt, cnt2, pt1;
                txgoal = 0;
                /* debug mode */
                if (flagbyte15 & FLAGBYTE15_DB_WRAP) {
                    cnt2 = DB_BUFSIZE;
                    pt1 = db_ptr;
                } else {
                    cnt2 = db_ptr;
                    pt1 = 0;
                }
                for (cnt = 0 ; cnt < cnt2 ; cnt++) {
                    unsigned char c;
                    c = g_read8(DB_BUFADDR + pt1);
                    pt1++; /* gcc makes better code with this on its own line */
                    if (pt1 >= DB_BUFSIZE) {
                        pt1 = 0;
                    }
                    /* change 0x0a into 0x0d 0x0a */
                    if (c) {
                        if (c == 0x0a) {
                            g_write8(0x0d, 0xf000 + txgoal);
                            txgoal++;
                        } else if ((c != 0x0a) && (c != 0x0d) && ((c < 32) || (c > 127))) {
                            c = '.';
                        }
                        g_write8(c, 0xf000 + txgoal);
                        txgoal++;
                    }
                }
                rxmode = 0;
                /* send as unwrapped data */
                txcnt = 0;
                txmode = 129;
                *(port_sci + 7) = g_read8(0xf000); // SCIxDRL
                *(port_sci + 3) |= 0x88; // xmit enable & xmit interrupt enable // SCIxCR2
                flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;

            } else if ((rxmode == 2) && (cmd == 'r') && (g_read8(0xf001) == 0x00) && (g_read8(0xf002) == 0x04)) {
                /* Megaview request config data command */
                ad = 0xf000;
                size = 7;
                compat = 1;
                rxmode = 0;
                goto SERIAL_OK;
            } else if ((rxmode == 2) && (cmd == 'a') && (g_read8(0xf001) == 0x00) && (g_read8(0xf002) == 0x06)) {
                /* Megaview read command */
                ad = 0xf000;
                g_write8('A', ad); /* fake A command */
                size = 1;
                compat = 2; // MV read
                rxmode = 0;
                goto SERIAL_OK;
            } else {
                /* had some undefined under-run situation */
                g_write8(0x80, 0xf002);
                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                rxmode = 0;
                goto EXIT_SERIAL;
            }
        }
    }

    if ((flagbyte14 & FLAGBYTE14_SERIAL_BURN) && (burnstat == 0)) {
        /* burn now completed */
        flagbyte14 &= ~FLAGBYTE14_SERIAL_BURN;
        g_write8(0x04, 0xf002); // burn OK code
        txgoal = 0;
        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
        goto EXIT_SERIAL;
    }

    if (flagbyte14 & FLAGBYTE14_SERIAL_OK) {
        flagbyte14 &= ~FLAGBYTE14_SERIAL_OK;
        goto RETURNOK_SERIAL;
    }

    if (flagbyte11 & FLAGBYTE11_CANRX) {
        flagbyte11 &= ~FLAGBYTE11_CANRX;
        g_write8(0x06, 0xf002); // OK code
        txgoal = g_read16(0xf000); // what was stored there previously
        flagbyte3 &= ~flagbyte3_getcandat;
        cp_targ = 0;
        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
        goto EXIT_SERIAL;
    }

    if (flagbyte3 & FLAGBYTE3_REMOTEBURNSEND) {
        /* Got an ACK packet, let TS know */
        flagbyte3 &= ~(flagbyte3_getcandat | FLAGBYTE3_REMOTEBURN | FLAGBYTE3_REMOTEBURNSEND);
        goto RETURNOK_SERIAL;
    }

    if (flagbyte3 & flagbyte3_getcandat) {
        unsigned int ti;
        DISABLE_INTERRUPTS;
        ti = (unsigned int)lmms - cp_time;
        ENABLE_INTERRUPTS;

        if ((flagbyte3 & FLAGBYTE3_REMOTEBURN) && (ti > 469)) { /* 60ms */
            /* If an ack packet was received, the flag would be cleared
                no method to know if other end supports it, so have to say ok now
               This forces TS to wait some time for remote burn to complete */
            flagbyte3 &= ~(flagbyte3_getcandat | FLAGBYTE3_REMOTEBURN);
            goto RETURNOK_SERIAL;
        }

        if (ti > 1000) {
            /* taking too long to get a reply... bail on it */
            g_write8(0x8a, 0xf002); // CAN timeout
            txgoal = 0;
            flagbyte3 &= ~flagbyte3_getcandat;
            cp_targ = 0;
            if (can_getid < 16) { // sanity check
                can_err_cnt[can_getid]++; // increment error counter
            } 
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (flagbyte14 & FLAGBYTE14_CP_ERR) {
            flagbyte3 &= ~flagbyte3_getcandat;
            flagbyte14 &= ~FLAGBYTE14_CP_ERR;
            g_write8(0x8b, 0xf002); // CAN failure
            txgoal = 0;
            cp_targ = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }
        if (flagbyte14 & FLAGBYTE14_SERIAL_PROCESS) {
            goto BUSY_SERIAL;
        }
    }

    if (flagbyte3 & flagbyte3_sndcandat) {
        unsigned int ti;
        DISABLE_INTERRUPTS;
        ti = (unsigned int)lmms - cp_time;
        ENABLE_INTERRUPTS;

        if (ti > 1000) {
            g_write8(0x8a, 0xf002);
            txgoal = 0;
            flagbyte3 &= ~flagbyte3_sndcandat;
            cp_targ = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (flagbyte14 & FLAGBYTE14_CP_ERR) {
            flagbyte3 &= ~flagbyte3_sndcandat;
            flagbyte14 &= ~FLAGBYTE14_CP_ERR;
            g_write8(0x8b, 0xf002); // CAN failure - not sure that TS will be expecting this reply now...
            txgoal = 0;
            cp_targ = 0;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }
    }

    if (!(flagbyte14 & FLAGBYTE14_SERIAL_PROCESS)) {
        if (flagbyte14 & FLAGBYTE14_SERIAL_FWD) {
            /* pass on forwarded CAN data if not doing any other transaction */
            flagbyte14 &= ~FLAGBYTE14_SERIAL_FWD;
            txgoal = canbuf[16];
            g_write8(6, 0xf002); /* CAN data */
            for (x = 0; x < txgoal; x++) {
                g_write8(canbuf[x], 0xf003 + x);
            }
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;

        } else {
            return;
        }
    }

    size = g_read16(0xf000); // size of payload
    /* calc CRC of data */
    crc = g_crc32buf(0, 0xf002, size);    // incrc, buf, size

    if (crc != g_read32(0xf002 + size)) {
        /* CRC did not match - build return error packet */
        g_write8(0x82, 0xf002); // CRC failed code
        txgoal = 0;
        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
        goto EXIT_SERIAL;
    }

    /* Get here when packet looks ok */
SERIAL_OK:;

    /* We got something real, so lock to this serial port only. */
    sci_lock_timer = 0;
    if (!(flagbyte2 & FLAGBYTE2_SCI_CONFIRMED)) {
        flagbyte2 |= FLAGBYTE2_SCI_CONFIRMED;
        if (port_sci == &SCI0BDH) {
            /* Disable SCI1 */
            SCI1CR1 = 0x00;
            SCI1CR2 = 0x00;
        } else {
            /* Disable SCI0 */
            SCI0CR1 = 0x00;
            SCI0CR2 = 0x00;
        }
    }
/*
;**************************************************************************
; **
; ** SCI Communications - within size/CRC32 wrapper
; **
; ** Communications is established when the PC communications program sends
; ** a command character - the particular character sets the mode:
; **
; *  "a"+<canid>+<table id> = send all of the realtime display variables (outpc structure) via txport.
; *  "A" send all realtime vars
; *  "b"+<canid>+<table id> =  burn a ram data block into a flash data block.
; *  "c" = Test communications - echo back U16 Seconds
; ** "D" = Display 'debug buffer'
;    "e" = removed
; *  "F" = return serial version in ASCII e.g. 001
;    "g" = Get selective outpc realtime data. (Returns error if not yet defined.)
;    "h"+<0/1> = Broadcast a CAN 'halt' or 'unhalt' 1 = halt, 0 = unhalt.
; *  "f"+<canid> = return U08 serial version, U08 blocking factor table, U16 blocking factor write
; *  "I" = return CANid in binary
; 'k'+<canid>+<table id>+offset+size = return CRC of data page
;    "M" = return two byte monitor version
; *  "Q" = Send over Embedded Code Revision Number
; *  "r"+<canid>+<table id>+<offset lsb>+<offset msb>+<nobytes> = read and
;     send back value of a data parameter(s) in offset location within table
; *  "S" = Send program title.
;    "T" = removed
;    "t" = removed
;    "w"+<canid>+<table id>+<offset lsb>+<offset msb>+<nobytes>+<newbytes> =
;     receive updated data parameter(s) and write into offset location
;     relative to start of data block
;    "y" = removed
; 
; Generally commands require the 'newserial' wrapper.
; * = supported wrapped or unwrapped for compatability.
;** = supported unwrapped only - intended for use in Miniterminal
;
; **************************************************************************
*/

    cmd = g_read8(ad);
    if (size == 1) {
        /* single character commands */
        if (cmd == 'A') {
            if (!compat) {

                if (conf_err) {
                    /* if there's a configuration error, then we'll send that back instead of the realtime data */
                    g_write8(0x03, ad);    /* config error packet */
                    ad++;
                    for (x = 0 ; x < 256 ; x++) {
                        cmd = g_read8(0xf700 + x);
                        g_write8(cmd, ad++);
                        if (cmd == 0) {
                            break;
                        }
                    }
                    txgoal = x;
                    if (conf_err >= 200) {
                        conf_err = 0;
                    }
                    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                    goto EXIT_SERIAL;
                }

                g_write8(1, ad);    /* realtime data packet */
                ad++;
                // copy outpc to txbuf one by one to allow interrupts
            }

            x = ad;
            if (compat < 2) {
                txgoal = sizeof(outpc);
            } else {
                txgoal = 112;
            }
            // takes ~50us to copy the lot over
            cp2serialbuf(ad, (unsigned int)&outpc, txgoal);

            if (compat && (ram4.Temp_Units & 1)) { /* re-write these two for Megaview if needed */
                x = 0xf000 + 20;
                g_write_generic(((outpc.mat - 320) * 5) / 9, &x, sizeof(outpc.mat));
                g_write_generic(((outpc.clt - 320) * 5) / 9, &x, sizeof(outpc.clt));
            }

            if (compat < 2) {
                /* MS3 different from now */
                if (outpc.syncreason) {
                    outpc.syncreason = 0;       // send the sync loss reason code once then reset it
                }
            } else {
                x = 0xf000 + 72;
                GWR(outpc.EAEfcor1);
                GWR(outpc.egoV1);
                GWR(outpc.egoV2);
                g_write_generic(0, &x, 2); /* amc Updates */
                g_write_generic(0, &x, 2); /* kpaix */
                GWR(outpc.EAEfcor2);
                g_write_generic(0, &x, 2); /* spare1 */
                g_write_generic(0, &x, 2); /* spare2 */
                g_write_generic(0, &x, 2); /* trigfix */
                g_write_generic(0, &x, 2); /* spare4 */
                g_write_generic(0, &x, 2); /* spare5 */
                g_write_generic(0, &x, 2); /* spare6 */
                g_write_generic(0, &x, 2); /* spare7 */
                g_write_generic(0, &x, 2); /* spare8 */
                g_write_generic(0, &x, 2); /* spare9 */
                g_write_generic(0, &x, 2); /* spare10 */
                g_write_generic(0, &x, 2); /* tachcount */
                g_write_generic(0, &x, 2); /* ospare/cksum */
                g_write_generic(0, &x, 2); /* deltaT */
                g_write_generic(0, &x, 2); /* deltaT pt.2 */
            }

            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;

        } else if (cmd == 'C') {
            /* test data packet */
            if (!compat) {
                g_write8(0, ad); /* OK */
                ad++;
            }
            x = ad;
            // copy outpc to txbuf one by one to allow interrupts
            GWR(outpc.seconds);
            txgoal = 2;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;

        } else if (cmd == 'Q') {
            /* serial format */
            if (!compat) {
                g_write8(0, ad); /* OK */
                ad++;
            }
            for (x = 0; x < SIZE_OF_REVNUM; x++) {
                g_write8(RevNum[x], ad + x);
            }
            txgoal = SIZE_OF_REVNUM;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'S') {
            /* string for title bar */
            if (!compat) {
                g_write8(0, ad);
                ad++;
            }
            for (x = 0; x < SIZE_OF_SIGNATURE; x++) {
                g_write8(Signature[x], ad + x);
            }
            txgoal = SIZE_OF_SIGNATURE;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'F') {
            /* format i.e. 001 = newserial */
            if (!compat) {
                g_write8(0, ad);
                ad++;
            }
            g_write8('0', ad++);
            g_write8('0', ad++);
            g_write8(48 + SRLVER, ad);
            txgoal = 3;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'I') {
            /* return my CANid */
            if (!compat) {
                g_write8(0, ad);
                ad++;
            }
            g_write8(CANid, ad);
            txgoal = 1;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'M') {
            /* return my CANid */
            g_write8(0, ad);
            ad++;
            g_write16(*(unsigned int*)0xfefe, ad);
            txgoal = 2;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else if (cmd == 'g') {
            /* Tuning software has defined a dataset, so process it. */
            unsigned int a, dst, off, sz;
            dst = 0xf003;
            RPAGE = tables[25].rpg;
            for (a = 0 ; a < 256 ; a ++) {
                off = ram_window.trimpage.rtvarsel[a];
                if ((off & 0xe000) == 0x2000) {
                    sz = 1;
                } else if ((off & 0xe000) == 0x4000) {
                    sz = 2;
                } else if ((off & 0xe000) == 0x6000) {
                    sz = 4;
                } else {
                    sz = 0;
                }
                if (sz) {
                    off &= 0x3ff; // within dataspace allowed for outpc

                    if (sz == 1) {  // 8 bit
                        g_write8(*((unsigned char *) &outpc + off), dst);
                        dst++;
                    } else if (sz == 2) {   // 16 bit
                        g_write16(*((unsigned int *)&(*((unsigned char *) &outpc + off))), dst);
                        dst += 2;
                    } else if (sz == 4) {   // 32 bit
                        unsigned long *ul_tmp_ad;

                        ul_tmp_ad = (unsigned long *)
                            &(*((unsigned char *) &outpc + off));
                        DISABLE_INTERRUPTS;
                        g_write32(*((unsigned long *) ul_tmp_ad), dst);
                        ENABLE_INTERRUPTS;
                        dst += 4;
                    }
                    if (dst > 0xf3f0) {
                        a = 257; // no more data
                    }
                }
            }
            txgoal = dst - 0xf003;

            if (txgoal == 0) {
                /* didn't actually find anything! */
                unsigned int z;
                const char errstr[] = "Need to define shortform realtime data before trying to read it.";
                g_write8(0x93, 0xf002);

                for (z = 0; z < sizeof(errstr); z++) {
                    g_write8(errstr[z], 0xf003 + z);
                }
                txgoal = sizeof(errstr);
            } else {
                g_write8(0, 0xf002);
            }

            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;

        } /* end of single byte commands */

    } else if ((size == 3) && (cmd == 'b')) {
        /* need a way of avoiding clashes between remotely and locally initiated burn */
        r_canid = g_read8(0xf003) & 0x0f;
        r_table = g_read8(0xf004);

        if ((r_canid >= MAX_CANBOARDS) || (r_table >= NO_TBLES)) {
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {
            unsigned int r;

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            DISABLE_INTERRUPTS;
            r = can_sendburn(r_table, r_canid); // ring0
            ENABLE_INTERRUPTS;
            if (r) {
                g_write8(0x89, 0xf002); // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
            goto EXIT_SERIAL;
        } else {
            /* local burn */
            if ((burnstat) || (flagbyte1 & flagbyte1_tstmode)) {
                /* burn already in progress ! */
                /* no more burning during test mode */
                goto BUSY_SERIAL;
            }

            burn_idx = r_table;
            burnstat = 1;
            flocker = 0xdd;
            flagbyte14 |= FLAGBYTE14_SERIAL_BURN; // burn mode
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
            goto EXIT_SERIAL;
        }

    } else if ((size == 7) && (cmd == 'k')) {
        /* CRC of 1k ram page */
        /* or of 16k memory region */
        r_canid = g_read8(0xf003) & 0x0f;
        r_table = g_read8(0xf004);
        /* ignore the other 4 bytes */

        if ((r_canid >= MAX_CANBOARDS) || (r_table < 0x70 && (r_table >= NO_TBLES))
            || (r_table > 0x7f)) {
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {
            unsigned int r;

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            canrxad = 0xf003;
            canrxgoal = 4;
            g_write16(canrxgoal, 0xf000);
            DISABLE_INTERRUPTS;
            r = can_crc32(r_table, r_canid);
            ENABLE_INTERRUPTS;
            if (r) {
                g_write8(0x89, 0xf002); // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
            goto EXIT_SERIAL;
        } else {
            /* local CRC */
            g_write8(0, ad);
            ad++;
            if (r_table < 0x40) {
                if (tables[r_table].n_bytes == 0) {
                        goto INVALID_OUTOFRANGE;
                } else if (r_table < 4) {
                    unsigned char gp_tmp;
                    /* Has no RAM address, check in flash */
                    gp_tmp = GPAGE;
                    GPAGE = 0x10; // eeprom
                    crc = g_crc32buf(0, tables[r_table].addrFlash, tables[r_table].n_bytes);    // incrc, buf, size
                    GPAGE = gp_tmp;
                } else {
                    /* CRC of RAM page */
                    rp = tables[r_table].rpg;
                    if (rp) {
                        RPAGE = rp;
                    } else {
                        goto INVALID_OUTOFRANGE;
                    }
                    crc = crc32buf(0, tables[r_table].adhi << 8, 0x400);    // incrc, buf, size
                }
            } else {
                /* 'table' is GPAGE value and calc CRC of a l<<8 bytes */
                unsigned int ad2;
                unsigned long ll;
                ad2 = g_read8(0xf005) << 8;
                /* without the double cast, gcc erroneously sign extends the intermediate 16bit result */
                ll = (unsigned long)((unsigned int)(g_read8(0xf006) << 8));
                if (ll == 0) {
                    ll = 0x10000;
                }
                if ((r_table == 0x7f) && (((unsigned long)ad2 + ll) > 0xf000)) {
                    /* avoid the monitor  */
                    goto INVALID_OUTOFRANGE;
                }
                GPAGE = r_table;
                crc = g_crc32buf(0, ad2, (unsigned int)ll);
                GPAGE = 0x13;
            }
            g_write32(crc, ad);
            txgoal = 4;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }

    } else if ((size == 7) && (cmd == 'r')) {
        ad++;
        r_canid = g_read8(ad);
        ad++;
        r_table = g_read8(ad);
        ad++;
        r_offset = g_read16(ad);
        ad += 2;
        r_size = g_read16(ad);

        if ((r_canid >= MAX_CANBOARDS)
            || (r_size > 0x4000) || (r_offset >= 0x4000) /* Prevent wrap-around overflow */
            || ((r_table >= NO_TBLES) && ((r_table < 0xf0) || (r_table > 0xf8)))) {
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            unsigned int by, r = 0;

            canrxad = 0xf003;
            canrxgoal = r_size;
            g_write16(canrxgoal, 0xf000);

            if (r_size <= 8) {
                by = r_size;
            } else {
                /* set up other passthrough read, variables handled in CAN TX ISR */
                cp_id = r_canid;
                cp_table = r_table;
                cp_targ = r_size;
                cp_cnt = 0;
                cp_offset = r_offset;
                by = 8;
            }

            DISABLE_INTERRUPTS;
            r = can_reqdata(r_table, r_canid, r_offset, by);
            ENABLE_INTERRUPTS;
            if (r) {
                g_write8(0x89, 0xf002); // CAN overflow code

                cp_targ = 0;
                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }

            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
            goto EXIT_SERIAL;
        } else {
            unsigned int ram_ad;
            if ((r_table >= 0xf0) && (r_table <= 0xf8)) {
                /* logger pages
                0xf0 = tooth logger
                0xf1 = trigger logger
                0xf2 = composite logger
                0xf3 = sync error composite logger
                0xf4 = map logger
                0xf5 = maf logger
                0xf6 = engine logger
                0xf7 = engine logger + MAP
                0xf8 = engine logger + MAF

                 */

                flagbyte0 &= ~(FLAGBYTE0_TTHLOG | FLAGBYTE0_TRGLOG | FLAGBYTE0_COMPLOG | FLAGBYTE0_MAPLOG | FLAGBYTE0_MAPLOGARM
                                | FLAGBYTE0_MAFLOG | FLAGBYTE0_MAFLOGARM | FLAGBYTE0_ENGLOG);
                /* ensure tooth logger off, ensure trigger logger off, ensure composite tooth logger off */

                outpc.status3 &= ~STATUS3_DONELOG; /* clear 'complete/available' flag */

                RPAGE = TRIGLOGPAGE;
                ram_ad = TRIGLOGBASE;
                if (page != r_table) {
                    /* first read - zap page to prevent garbage */
                    for (x = TRIGLOGBASE; x < (TRIGLOGBASE + 0x400); x++) {
                        *(unsigned char*)x = 0;
                    }
                } else if ((r_table == 0xf4) || (r_table == 0xf5)) {
                    /* process teeth into angles in map logger */
                    for (x = TRIGLOGBASE; x < (TRIGLOGBASE + 0x400); x+=2) {
                        unsigned int t;
                        t = *(unsigned int*)x;
                        if ((t & 0xc000) == 0x8000) {
                            t = t & 0x3fff;
                            if (t == no_teeth) {
                                t = 0;
                            }
                            t = tooth_absang[t];
                            t |= 0x8000;
                            *(unsigned int*)x = t;
                        }
                    }
//                    if (r_table == 0xf5) { // MAF logger - convert ADC to g/sec
//                    }
                }
                page = r_table;
                flagbyte14 |= FLAGBYTE14_SERIAL_TL;

            } else {
                /* local read */
                unsigned char r_offset_h = r_offset >> 8;
                if ((r_table != 0x14) && ((r_table > NO_TBLES)
                    || ((r_table != 7) && ((r_offset + r_size) > tables[r_table].n_bytes))
                    || ((r_table == 7) && (r_offset_h < 0x2) && ((r_offset + r_size) > tables[7].n_bytes))
                    || ((r_table == 7) && (r_offset_h == 0x4) && ((r_offset + r_size) > (CONFERR_OFFSET + CONFERR_SIZE))) // config errors via CAN_COMMANDS
                    || ((r_table == 7) && (r_offset_h == 0x2) && ((r_offset + r_size) > (DATAX1_OFFSET + sizeof(datax1))))
                    )) {
                    /* last two lines to handle outpc vs. datax1 in same 'page' */
                    goto INVALID_OUTOFRANGE;
                }

                if (r_table == 0x14) { /* SDcard file readback */
                    /* not yet documented
                        sd_sync2 does this for new-serial
                        buf[2] = 'r';
                        buf[3] = canid;
                        buf[4] = 0x14; // sd_buf
                        buf[5] = blk_num >> 8; // start
                        buf[6] = blk_num & 0xff; 
                        buf[7] = 0x08; // ignored anyway
                        buf[8] = 0x00;
                    */
                    if (r_offset == sd_rb_block) {
                        /* asked for next block */
                        flagbyte7 |= FLAGBYTE7_SF_GO; // actual send setup is handled in the compress
                        goto EXIT_SERIAL;
                    } else {
                        /* not yet supporting old ones */
                        /* do want to add this to allow retrying without requesting the file all over again */
                        goto INVALID_SERIAL;
                    }
                } else if ((r_table == 0x10) && (flagbyte8 & FLAGBYTE8_MODE10)) { // special handling
                    /* restart timeout */
                    mode10_time = (unsigned int)lmms;
                }

                RPAGE = tables[r_table].rpg;
                if ((r_table == 7) && (r_offset >= CONFERR_OFFSET)) {
                    ram_ad = 0xf700;
                    r_offset -= CONFERR_OFFSET;
                } else if ((r_table == 7) && (r_offset >= DATAX1_OFFSET)) {
                    ram_ad = (unsigned int)&datax1;
                    r_offset -= DATAX1_OFFSET;
                } else {
                    ram_ad = (unsigned int)tables[r_table].addrRam;
                }
            }

            if (ram_ad == 0) {
                /* No valid RAM copy to read back
                    This will happen if user tries to read calibration tables.
                    Could extend code to pull back flash, but not yet. */
                goto INVALID_OUTOFRANGE;
            }

            if (!compat) {
                g_write8(0, 0xf002); // OK
                ad = 0xf003;
            } else {
                ad = 0xf000;
            }

            if (ram_ad == 0xf700) { // special case for config errors
                unsigned char a;
                /* buffer is pre-filled with zeros before writing message in conferr.s */
                for (x = 0; x < r_size; x++) {
                    a = g_read8(ram_ad + r_offset + x);
                    g_write8(a, ad + x);
                }
            } else {
                for (x = 0; x < r_size; x++) {
                    g_write8(*(unsigned char*)(ram_ad + r_offset + x), ad + x);
                }
                if (compat && (r_table == 0x04) && (r_offset == 0x0000) && (r_size == 0x0392)) {
                    /* This must be Megaview reading us, plonk ram4.Temp_Units back where it expects it */
                    g_write8(ram4.Temp_Units, ad + 599);
                }
            }
            txgoal = r_size;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }
        
    } else if ((size > 7) && ((cmd == 'w') || (cmd == 'x'))) {
        r_canid = g_read8(0xf003) & 0x0f;
        r_table = g_read8(0xf004);
        r_offset = g_read16(0xf005);
        r_size = g_read16(0xf007);

        if ((r_canid >= MAX_CANBOARDS)
            || (r_size > 0x4000) || (r_offset >= 0x4000) /* Prevent wrap-around overflow */
            || (r_table >= NO_TBLES)) {
            goto INVALID_OUTOFRANGE;
        }

        if (r_canid != CANid) {
            unsigned int by, r = 0;

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            if (r_size <= 8) {
                by = r_size;
            } else {
                by = 8;
                /* set up a passthrough write, handled in CAN TX ISR */
                /* when all packets in queue, flag will be set to tell PC */
                flagbyte3 |= flagbyte3_sndcandat;
                cp_id = r_canid;
                cp_table = r_table;
                cp_targ = r_size;
                cp_cnt = 8;
                cp_offset = r_offset;
            }

            DISABLE_INTERRUPTS;
            r = can_snddata(r_table, r_canid, r_offset, by, 0xf009);
            ENABLE_INTERRUPTS;
            if (r) {
                g_write8(0x89, 0xf002); // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }

            if (r_size <= 8) {
                /* for a single packet say ok now */
                if (cmd == 'x') {
                    /* no-ACK command */
                    flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
                    goto EXIT_SERIAL;
                } else {
                    goto RETURNOK_SERIAL;
                }
            }
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else {
            unsigned char ch = 0;
            unsigned int ram_ad;
            /* local write */
            if (cmd != 'w') {
                goto INVALID_SERIAL;
            }

            if ((r_table > NO_TBLES)
                || ((r_table != 7) && ((r_offset + r_size) > tables[r_table].n_bytes))
                || ((r_table == 7) && (r_offset < DATAX1_OFFSET) && ((r_offset + r_size) > tables[7].n_bytes))
                || ((r_table == 7) && (r_offset >= DATAX1_OFFSET) && ((r_offset + r_size) > (DATAX1_OFFSET + sizeof(datax1))))
                ) {
                goto INVALID_OUTOFRANGE;
            }

            if (r_table != 6) { 
                if (r_table == 0x10) { // special handling
                    if (r_size != 0x40) {
//                        goto INVALID_OUTOFRANGE;
                        g_write8(0x87, 0xf002); // FIXME 0x87
                        txgoal = 0;
                        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                        goto EXIT_SERIAL;
                    }

                    if (outpc.rpm) {
                        goto BUSY_SERIAL;
                    }

                    if (!(flagbyte8 & FLAGBYTE8_MODE10)) {
                        for (x = 0xf009 ; x < 0xf049 ; x++) {
                            if (g_read8(x)) {
                                g_write8(0x88, 0xf002); // FIXME 0x88
                                txgoal = 0;
                                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                                goto EXIT_SERIAL;
                            }
                        }
                        flagbyte8 |= FLAGBYTE8_MODE10;
                    }
                    mode10_time = (unsigned int)lmms;
                    flagbyte1 |=FLAGBYTE1_EX;
                } else if (r_table == 25) {
                    /* special case handling for MAF lookup curve */
                    if (r_offset < 256) {
                        flagbyte21 |= FLAGBYTE21_POPMAF;
                    }
                }

                rp = tables[r_table].rpg;
                if (rp) {
                    RPAGE = rp;
                }

                if (r_table == 0x11) { /* SDcard operations */
                    /* use r_offset as command no. */

                    if (r_offset == 4) { // sd_stream:
                        /* unlike other newserial, this just spews back raw data */
                        if (g_read8(0xf009)) {
                            flagbyte6 |= FLAGBYTE6_STREAM_CONT;
                            *(port_sci + 3) &= ~0x80; //xmit interrupt disable // SCIxCR2
                            *(port_sci + 3) |= 0x08; // xmit enable // SCIxCR2
                            goto EXIT_SERIAL;

                        } else {
                            /* unlikely to be able to see this command */
                            flagbyte6 &= ~FLAGBYTE6_STREAM_CONT;
                            goto RETURNOK_SERIAL;
                        }
                    /* all following commands require a card to be present */
                    } else if (!(outpc.sd_status & 1)) {
                        goto BUSY_SERIAL;

                    } else if (r_offset == 0) { /* do command */
                        unsigned char cmd2;
                        cmd2 = g_read8(0xf009);
                        if (cmd2 == 0) { // sd_resetgo:
                            sd_phase = 0;
                            flagbyte6 &= ~FLAGBYTE6_SD_MANUAL;
                            flagbyte15 &= ~FLAGBYTE15_SDLOGRUN;
                            flagbyte10 &= ~FLAGBYTE10_SDERASEAUTO;
                            goto RETURNOK_SERIAL;
                        } else if (cmd2 == 1) { // sd_resetwait:
                            sd_phase = 0;
                            flagbyte6 |= FLAGBYTE6_SD_MANUAL;
                            flagbyte15 &= ~FLAGBYTE15_SDLOGRUN;
                            goto RETURNOK_SERIAL;
                        } else if (cmd2 == 2) { // sd_stoplog:
                            if (!(outpc.sd_status & 8)) { // not allowed while NOT logging
                                goto BUSY_SERIAL;
                            }
                            flagbyte6 |= FLAGBYTE6_SD_MANUAL;
                            flagbyte15 &= ~FLAGBYTE15_SDLOGRUN;
                            sd_phase = 0x48;
                            goto RETURNOK_SERIAL;
                        } else if (cmd2 == 3) { // sd_startlog:
                            if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                                goto BUSY_SERIAL;
                            }
                            flagbyte6 |= FLAGBYTE6_SD_MANUAL;
                            flagbyte15 |= FLAGBYTE15_SDLOGRUN;
                            /* FIXME - need to check if we are allowed to do this */
                            sd_phase = 0x30; // update time in dirent first
                            goto RETURNOK_SERIAL;
                        } else if (cmd2 == 4) { // sd_sendstat:
                            if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                                goto BUSY_SERIAL;
                            }
                            // put data into buffer ready for fetch
                            *(unsigned char*)(SDSECT1 + 0) = outpc.sd_status;
                            *(unsigned char*)(SDSECT1 + 0x1) = outpc.sd_error;
                            *(unsigned int*)(SDSECT1 + 0x2) = sd_sectsize;
                            *(unsigned int*)(SDSECT1 + 0x4) = u32MaxBlocks >> 16;
                            *(unsigned int*)(SDSECT1 + 0x6) = (unsigned int)u32MaxBlocks;
                            *(unsigned int*)(SDSECT1 + 0x8) = sd_max_roots;
                            *(unsigned int*)(SDSECT1 + 0xa) = sd_dir_start >> 16;
                            *(unsigned int*)(SDSECT1 + 0xc) = (unsigned int)sd_dir_start;
                            *(unsigned int*)(SDSECT1 + 0xe) = 0;
                            goto RETURNOK_SERIAL;
                        } else if (cmd2 == 5) { // sd_init:
                            flagbyte15 &= ~FLAGBYTE15_SDLOGRUN;
                            sd_phase = 0x00;
                            goto RETURNOK_SERIAL;
                        } else {
                            goto INVALID_SERIAL;
                        }
                    } else if (r_offset == 1) { // sd_readdir:
                        if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                            goto BUSY_SERIAL;
                        }
                        if (r_size != 2) {
                            goto INVALID_SERIAL;
                        }
                        sd_pos = sd_dir_start + g_read16(0xf009);
                        sd_phase = 0x50;
                        sd_uitmp = 0;
                        outpc.sd_status &= ~0x04;
                        goto RETURNOK_SERIAL;

                    } else if (r_offset == 2) { // sd_readsect:
                        if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                            goto BUSY_SERIAL;
                        }
                        if (r_size != 4) {
                            goto INVALID_SERIAL;
                        }
                        sd_pos = g_read32(0xf009);
                        sd_phase = 0x54;
                        outpc.sd_status &= ~0x04;
                        goto RETURNOK_SERIAL;

                    } else if (r_offset == 3) { // sd_writesect:
                        if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                            goto BUSY_SERIAL;
                        }
                        if (r_size != 0x204) {
                            goto INVALID_SERIAL;
                        }
                        sd_pos = g_read32(0xf209);
                        /* copy from serial buffer to SDcard buffer */
                        for (x = 0; x < 0x200; x+=2) {
                            unsigned int ad1, v;
                            v = g_read16(0xf009 + x);
                            ad1 = SDSECT1 + x;
                            *(unsigned int*)ad1 = v;
                        }
                        sd_phase = 0x58;
                        outpc.sd_status &= ~0x04;
                        sd_ledstat = 2;
                        goto RETURNOK_SERIAL;

                    } else if (r_offset == 5) { // sd_readfile:
                        if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                            goto BUSY_SERIAL;
                        }
                        if (r_size != 8) {
                            goto INVALID_SERIAL;
                        }
                        outpc.sd_status &= ~0x04; // not ready
                        sd_pos = g_read32(0xf009);
                        sd_filesize_bytes = g_read32(0xf00d);
                        flagbyte7 |= FLAGBYTE7_SENDFILE; // trigger mainloop sending
                        flagbyte7 &= ~FLAGBYTE7_SF_GO; // wait until asked
                        goto RETURNOK_SERIAL; // FIXME - needs review !

                    } else if (r_offset == 6) { // sd_erasefile:
                        if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                            goto BUSY_SERIAL;
                        }
                        if (r_size != 6) {
                            goto INVALID_SERIAL;
                        }
                        *(unsigned long*)SDSECT2 = g_read32(0xf009);
                        sd_uitmp2 = g_read16(0xf00d);
                        sd_pos = sd_dir_start + sd_uitmp2;
                        sd_phase = 0x88;
                        goto RETURNOK_SERIAL;

                    } else if (r_offset == 7) { // sd_speedtest:
                        if ((outpc.sd_status & 8) || (!(outpc.sd_status & 4))) { // logging || !ready
                            goto BUSY_SERIAL;
                        }
                        if (r_size != 8) {
                            goto INVALID_SERIAL;
                        }
                        sd_pos = g_read32(0xf009);
                        *(unsigned long*)SDSECT1 = g_read32(0xf00d);
                        sd_phase = 0xa0;
                        outpc.sd_status &= ~0x04; // not ready

                        goto RETURNOK_SERIAL;
                    }
                    /* if nothing caught yet, must be invalid */
                    goto INVALID_SERIAL;
                }
                /* back to normal transactions */

                if ((r_table == 7) && (r_offset >= DATAX1_OFFSET)) {
                    ram_ad = (unsigned int)&datax1 + r_offset - DATAX1_OFFSET;
                } else {
                    ram_ad = (unsigned int)tables[r_table].addrRam + r_offset;
                }

                if (ram_ad) {
                   ch = g_read_copy(0xf009, r_size, ram_ad);// src, count, dest
                } else {
                    ch = 0;
                }

                if (r_table <= 3) { // calibration data
                    /* Via serial we expect to receive and only accept the whole table in one go. */
                    /* Contrast CAN where we'll receive 8 bytes at a time and have to buffer somewhere.
                        Serial data is double buffered, this is a little wasteful, but keeps the burning code
                        common between serial and CAN as all data is read from RPAGE=0xf1, addr=0x1000+
                     */
                    if ((r_offset == 0) && (r_size == tables[r_table].n_bytes)) {

                        if ((burnstat) || (flagbyte1 & flagbyte1_tstmode))  {
                            /* burn already in progress ! */
                            /* no more burning during test mode */

                            goto BUSY_SERIAL;
                        }

                        if ((ram5.flashlock & 1) == 0) {
                            g_write8(0x86, 0xf002); // invalid command code
                            txgoal = 1;
                            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                            goto EXIT_SERIAL;
                        }
    
                        burn_idx = r_table;
                        burnstat = 300; // erase and writing handle in dribble_burn
                        flocker = 0xdd;
                        flagbyte14 |= FLAGBYTE14_SERIAL_BURN; // burn mode
                        flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND; // don't send anything yet
                        goto EXIT_SERIAL;

                    }

                } else if ((r_table == 7) && (r_offset >= DATAX1_OFFSET)
                        && ((r_offset - DATAX1_OFFSET) <= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1))
                        && ((r_offset - DATAX1_OFFSET + r_size) >= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1) + 1)) {
                    /* test mode special code */
                    if (datax1.testmodelock == 12345) {
                        unsigned int ret;
                        ret = do_testmode();
                        if (ret == 0) {
                            goto RETURNOK_SERIAL;
                        } else if (ret == 1) {
                            goto BUSY_SERIAL;
                        } else if (ret == 2) {
                            goto INVALID_OUTOFRANGE;
                        } else if (ret >= 3) {
                            /* undefined, return error */
                            goto INVALID_OUTOFRANGE;
                        }
                    } else {
                        flagbyte1 &= ~flagbyte1_tstmode; /* disable test mode */
                        outpc.status3 &= ~STATUS3_TESTMODE;
                    }
                }

                if (ch &&
                    ((r_table == 4) || (r_table == 5) || (r_table == 8)
                    || (r_table == 9) || (r_table == 10) || (r_table == 11)
                    || (r_table == 12) || (r_table == 13) || (r_table == 18)
                    || (r_table == 19) || (r_table == 21) || (r_table == 22)
                    || (r_table == 23) || (r_table == 24)
                    || (r_table == 27) || (r_table == 28) || (r_table == 29)
                    )) { /* only applies to tuning data pages */
                    if (!(flagbyte1 & flagbyte1_tstmode)) {
                        outpc.status1 |= STATUS1_NEEDBURN; /* flag burn needed if anything changed */
                    }
                }
                g_write8(0x00, 0xf002); // ok code
            } else {
                goto INVALID_OUTOFRANGE;
            }

            txgoal = 0;

            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }

    } else if ((size == 2) && (cmd == 'f')) {
        r_canid = g_read8(0xf003) & 0x0f;
        if (r_canid != CANid) {
            unsigned int r = 0;

            if (can_err_cnt[r_canid] > CAN_DEAD_THRESH) {
                goto DEAD_CAN;
            }

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            /* pass on via CAN */
            canrxad = 0xf003;
            canrxgoal = 5;
            g_write16(canrxgoal, 0xf000);

            DISABLE_INTERRUPTS;
            r = can_sndMSG_PROT(r_canid, 5); /* request long form of protocol */
            ENABLE_INTERRUPTS;
            if (r) {
                g_write8(0x89, 0xf002); // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }
            flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        } else {
            /* local
            <size><type><Prot><blockingFactorTable><blockingFactorWrite><CRC> */
            g_write8(0x00, 0xf002);
            g_write8(SRLVER, 0xf003);
            g_write16(SRLDATASIZE, 0xf004);
            g_write16(SRLDATASIZE, 0xf006);
            txgoal = 5;
            flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
            goto EXIT_SERIAL;
        }

    } else if ((size == 2) && (cmd == 'h')) {
            unsigned int r = 0;

            if (flagbyte3 & flagbyte3_getcandat) {
                goto BUSY_SERIAL;
            }

            DISABLE_INTERRUPTS;
            r = can_sndMSG_SPND(g_read8(0xf003)); // ring0
            ENABLE_INTERRUPTS;
            if (r) {
                g_write8(0x89, 0xf002); // CAN overflow code

                txgoal = 0;
                flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
                goto EXIT_SERIAL;
            }
            goto RETURNOK_SERIAL;

    } else if ((size == 14)
        && (g_read8(0xf002) == '!')
        && (g_read8(0xf003) == '!')
        && (g_read8(0xf004) == '!')
        && (g_read8(0xf005) == 'S')
        && (g_read8(0xf006) == 'a')
        && (g_read8(0xf007) == 'f')
        && (g_read8(0xf008) == 'e')
        && (g_read8(0xf009) == 't')
        && (g_read8(0xf00a) == 'y')
        && (g_read8(0xf00b) == 'F')
        && (g_read8(0xf00c) == 'i')
        && (g_read8(0xf00d) == 'r')
        && (g_read8(0xf00e) == 's')
        && (g_read8(0xf00f) == 't')) {
        /* jumperless reboot, shut down and enter monitor through COP timeout */
        *port_iacen |= pin_iacen; // turn off stepper power
        DISABLE_INTERRUPTS;
        // turn off fuel pump. User is instructed to have coils wired via the relay
        fuelpump_off();
        PORTA = 0; // turn off injectors - FIXME - ASSUMES NOTHING ELSE ON THOSE PORTS
        coilsel = 0xffff;
        FIRE_COIL;
        //Insert code here to reset ports to their default values - reading from D-flash

        // Monitor will check for this string on reboot
        *(unsigned char*)0x3ff0 = 'R';
        *(unsigned char*)0x3ff1 = 'u';
        *(unsigned char*)0x3ff2 = 'n';
        *(unsigned char*)0x3ff3 = 'M';
        *(unsigned char*)0x3ff4 = 'o';
        *(unsigned char*)0x3ff5 = 'n';
        *(unsigned char*)0x3ff6 = 'i';
        *(unsigned char*)0x3ff7 = 't';
        *(unsigned char*)0x3ff8 = 'o';
        *(unsigned char*)0x3ff9 = 'r';
        COPCTL = 0x41;
        while (1) { ; };

    }

/* ------------------------------------------------------------------------- */
INVALID_SERIAL:;
/* Should not be reached - must be invalid command - build return error packet */
    if (port_sci == &SCI1BDH) {
        srl_err_cnt1++;
    } else {
        srl_err_cnt0++;
    }
    g_write8(0x83, 0xf002); // invalid command code
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */
DEAD_CAN:;
    {
        unsigned int z;
        const char errstr[] = "CAN device dead, stop sending requests.";
        g_write8(0x92, 0xf002);

        for (z = 0; z < sizeof(errstr); z++) {
            g_write8(errstr[z], 0xf003 + z); 
        }
        txgoal = sizeof(errstr);
        flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
        goto EXIT_SERIAL;
    }

/* ------------------------------------------------------------------------- */
BUSY_SERIAL:;
    g_write8(0x85, 0xf002); // busy
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */
RETURNOK_SERIAL:;
    g_write8(0x00, 0xf002); // ok
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */
INVALID_OUTOFRANGE:;
/* Should not be reached - must be invalid command - build return error packet */
    if (port_sci == &SCI1BDH) {
        srl_err_cnt1++;
    } else {
        srl_err_cnt0++;
    }
    g_write8(0x84, 0xf002); // out of range command code
    txgoal = 0;
    flagbyte14 |= FLAGBYTE14_SERIAL_SEND;
    goto EXIT_SERIAL;

/* ------------------------------------------------------------------------- */

EXIT_SERIAL:;
    flagbyte14 &= ~FLAGBYTE14_SERIAL_PROCESS;

    if (flagbyte14 & FLAGBYTE14_SERIAL_SEND) {
        if (srl_err_cnt0 > 3) {
            srlerr0 = 7;
            rxmode = 0;
            txmode = 0;
            (void)SCI0DRL; // read assumed garbage data to clear flags
            SCI0CR2 &= ~0xAC;   // rcv, xmt disable, interrupt disable
            flagbyte21 |= FLAGBYTE21_KILL_SRL0;
            srl_timeout0 = (unsigned int)lmms;
            return;
        }

        if (srl_err_cnt1 > 3) {
            rxmode = 0;
            txmode = 0;
            (void)SCI1DRL; // read assumed garbage data to clear flags
            SCI1CR2 &= ~0xAC;   // rcv, xmt disable, interrupt disable
            flagbyte21 |= FLAGBYTE21_KILL_SRL1;
            srlerr1 = 7;
            srl_timeout0 = (unsigned int)lmms;
            return;
        }

        if (!compat) {
            txgoal++; /* 1 byte return code */
            g_write16(txgoal, 0xf000);
            g_write32(g_crc32buf(0, 0xf002, txgoal), 0xf002 + txgoal);
            txgoal += 6; /* 2 bytes size, 4 bytes CRC */
        }
        txcnt = 0;
        txmode = 129;
        *(port_sci + 7) = g_read8(0xf000); // SCIxDRL
        *(port_sci + 3) |= 0x88; // xmit enable & xmit interrupt enable // SCIxCR2
        flagbyte14 &= ~FLAGBYTE14_SERIAL_SEND;
    }
    return;
}

/* ------------------------------------------------------------------------- */

void debug_init(void)
{
    db_ptr = 0;
    debug_str("MS3 debug buffer ");
    debug_inthex(MONVER);
    debug_str("\r");
    debug_str("================\r");
}

void debug_str(unsigned char* str)
{
    unsigned char c, m = 0;
    GPAGE = 0x13;
    do {
        c = *str;
        str++;
        if (c) {
            g_write8(c, DB_BUFADDR + db_ptr);
            db_ptr++;
            if (db_ptr >= DB_BUFSIZE) {
                db_ptr = 0;
                flagbyte15 |= FLAGBYTE15_DB_WRAP;
            }
        }
        m++;
    } while ((c != 0) && (m < 80));
}

void debug_byte(unsigned char c)
{
    GPAGE = 0x13;
    g_write8(c, DB_BUFADDR + db_ptr);
    db_ptr++;
    if (db_ptr >= DB_BUFSIZE) {
        db_ptr = 0;
        flagbyte15 |= FLAGBYTE15_DB_WRAP;
    }
}

void debug_bytehex(unsigned char b)
{
    /* logs byte b in hex */
    unsigned char c;

    c = b >> 4;
    if (c < 10) {
        c += 48;
    } else {
        c += 55;
    }
    debug_byte(c);

    c = b & 0x0f;
    if (c < 10) {
        c += 48;
    } else {
        c += 55;
    }
    debug_byte(c);
}

void debug_bytedec(unsigned char b)
{
    /* logs byte b in decimal, 3 digits */
    unsigned char c;

    c = b / 100;
    c += 48;
    debug_byte(c);

    c = (b / 10) % 10;
    c += 48;
    debug_byte(c);

    c = b % 10;
    c += 48;
    debug_byte(c);
}

void debug_byte2dec(unsigned char b)
{
    /* logs byte b in decimal, 2 digits */
    unsigned char c;

    c = (b / 10) % 10;
    c += 48;
    debug_byte(c);

    c = b % 10;
    c += 48;
    debug_byte(c);
}

void debug_inthex(unsigned int b)
{
    debug_bytehex((b >> 8) & 0xff);
    debug_bytehex((b >> 0) & 0xff);
}

void debug_longhex(unsigned long b)
{
    debug_bytehex((b >> 24) & 0xff);
    debug_bytehex((b >> 16) & 0xff);
    debug_bytehex((b >> 8) & 0xff);
    debug_bytehex((b >> 0) & 0xff);
}

void sci_baud(unsigned long baudrate)
{
    // sets baud to user settable baud rate
    // restrict to sensible rates to prevent data corruption causing a problem
    if ((baudrate == 9600) || (baudrate == 19200) || (baudrate == 38400) || (baudrate == 57600)) {
        // SCI baud rate = SCI bus clock / (16 x SBR[12:0])
        // SBR[12:0] = SCI bus clock / (16 x SCI baud rate )
        *(volatile unsigned int*)&SCI0BDH = (unsigned int) (50000000 / (16L * baudrate));
    } else if (baudrate == 230400) {
        SCI0BDH = 0;
        SCI0BDL = 14;
    } else if (baudrate == 300000) { // 312500
        SCI0BDH = 0;
        SCI0BDL = 10;
    } else if (baudrate == 460800) { // doesn't work in JSM testing
        SCI0BDH = 0;
        SCI0BDL = 7;
    } else {
        // sets baud to 115200. Default / fallback
        SCI0BDH = 0;
        SCI0BDL = 27;
    }

    /* set SCI1 the same (for now) */
    SCI1BDH = SCI0BDH;
    SCI1BDL = SCI0BDL;
}

