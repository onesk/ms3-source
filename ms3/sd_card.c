/* $Id: sd_card.c,v 1.120.6.1 2015/01/23 15:44:31 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * asc2num()
    Origin: James Murray
    Majority: James Murray
 * write_dirent()
    Origin: James Murray
    Majority: James Murray
 * do_sdcard()
    Origin: James Murray
    Minor: Some code from Freescale examples
    Majority: James Murray
 * SPI_baudlow
    Origin: Freescale (trivial)
    Majority: James Murray
 * SPI_baudhigh
    Origin: Freescale (trivial)
    Majority: James Murray
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
// SD card code

#include "SD.h"
#include "ms3.h"

#define LOG_SIZE 64
#define MIN_LOG_INT 15 // 2ms
#define BLOCKDATA 0
#define BLOCKGPS 1

/************************************************************************************
 *  Takes two characters in little-endian format and returns one uchar
 */
unsigned char asc2num(unsigned int asc)
{
    unsigned char top_nib, btm_nib, result = 0;

    top_nib = *(((unsigned char *) &asc) + 0);
    btm_nib = *(((unsigned char *) &asc) + 1);

    if ((top_nib >= 48) && (top_nib <= 57)) {   //0-9
        result = top_nib - 48;
    } else {
        top_nib &= ~0x20;       // uppercase
        if ((top_nib >= 0x41) && (top_nib <= 0x46)) {
            result = top_nib - 55;
        }                       // else leave it at zero
    }

    result = result << 4;

    if ((btm_nib >= 48) && (btm_nib <= 57)) {   //0-9
        result |= btm_nib - 48;
    } else {
        btm_nib &= ~0x20;       // uppercase
        if ((btm_nib >= 0x41) && (btm_nib <= 0x46)) {
            result |= btm_nib - 55;
        }                       // else leave it at zero
    }

    return result;
}

void write_dirent(unsigned int ad, unsigned char hidden)
{
    unsigned char a, b;
    // empty slot. Use it.

    *(unsigned char *) (ad + 0x00) = 'L';
    *(unsigned char *) (ad + 0x01) = 'O';
    *(unsigned char *) (ad + 0x02) = 'G';
    a = sd_filenum >> 8;
    a = a >> 4;         // top nibble
    if (a > 10) {
        b = a + 55;
    } else {
        b = a + 48;
    }
    *(unsigned char *) (ad + 0x03) = b;
    a = sd_filenum >> 8;
    a = a & 0x0f;       // btm nibble
    if (a > 10) {
        b = a + 55;
    } else {
        b = a + 48;
    }
    *(unsigned char *) (ad + 0x04) = b;
    a = (sd_filenum & 0xff) >> 4;       // top nibble
    if (a > 9) {
        b = a + 55;
    } else {
        b = a + 48;
    }
    *(unsigned char *) (ad + 0x05) = b;
    a = sd_filenum & 0x0f;      // btm nibble
    if (a > 9) {
        b = a + 55;
    } else {
        b = a + 48;
    }
    *(unsigned char *) (ad + 0x06) = b;
    *(unsigned char *) (ad + 0x07) = ' ';
    *(unsigned char *) (ad + 0x08) = 'M';
    *(unsigned char *) (ad + 0x09) = 'S';
    *(unsigned char *) (ad + 0x0a) = '3';

    *(unsigned char *) (ad + 0x0b) = 0x20;      // archive

    *(unsigned char *) (ad + 0x0c) = 0x0;
    *(unsigned char *) (ad + 0x0d) = sd_seq % 200; // "fine resolution time"

    // file creation time/date
    if ((datax1.rtc_year > 2010) && (datax1.rtc_sec < 60)) {
        // looks like a real time/data
        unsigned int tmp_packed;
        unsigned char tmp_sec, tmp_min, tmp_hour, tmp_date, tmp_month;
        unsigned int tmp_year;
        // ensure cohenerncy
        DISABLE_INTERRUPTS;
        tmp_sec = datax1.rtc_sec;
        tmp_min = datax1.rtc_min;
        tmp_hour = datax1.rtc_hour;
        tmp_date = datax1.rtc_date;
        tmp_month = datax1.rtc_month;
        tmp_year = datax1.rtc_year;
        ENABLE_INTERRUPTS;

        // bit pack the time FAT format
        tmp_packed = tmp_hour & 0x1f;
        tmp_packed <<= 6;
        tmp_packed |= tmp_min & 0x03f;
        tmp_packed <<= 5;
        tmp_packed |= ((tmp_sec & 0x03f)>>1);

        *(unsigned char *) (ad + 0x0e) = tmp_packed & 0xff;     // real time
        *(unsigned char *) (ad + 0x0f) = (tmp_packed >> 8);     // real time

        // bit pack the date FAT format
        tmp_packed = (tmp_year - 1980) & 0x7f;
        tmp_packed <<= 4;
        tmp_packed |= tmp_month & 0x0f;
        tmp_packed <<= 5;
        tmp_packed |= tmp_date & 0x01f;

        *(unsigned char *) (ad + 0x10) = tmp_packed & 0xff;     // real date
        *(unsigned char *) (ad + 0x11) = (tmp_packed >> 8);     // real date

    } else {
        *(unsigned char *) (ad + 0x0e) = (((sd_seq % 60) & 0x07) << 5);     // time = 00:00 + file sequence
        *(unsigned char *) (ad + 0x0f) = (((sd_seq % 60) & 0xf8) >> 4);

        *(unsigned char *) (ad + 0x10) = 0x21;      // date
        *(unsigned char *) (ad + 0x11) = 0x3a;
    }

    *(unsigned char *) (ad + 0x12) = *(unsigned char *) (ad + 0x10);      // last access date
    *(unsigned char *) (ad + 0x13) = *(unsigned char *) (ad + 0x11);

    *(unsigned char *) (ad + 0x14) = *(((unsigned char *) &sd_thisfile_clust) + 1);     // cluster
    *(unsigned char *) (ad + 0x15) = *(((unsigned char *) &sd_thisfile_clust) + 0);     // high word.

    *(unsigned char *) (ad + 0x16) = *(unsigned char *) (ad + 0x0e);    // last mod time
    *(unsigned char *) (ad + 0x17) = *(unsigned char *) (ad + 0x0f);

    *(unsigned char *) (ad + 0x18) = *(unsigned char *) (ad + 0x10);      // last mod date
    *(unsigned char *) (ad + 0x19) = *(unsigned char *) (ad + 0x11);

    *(unsigned char *) (ad + 0x1a) = *(((unsigned char *) &sd_thisfile_clust) + 3);     // cluster
    *(unsigned char *) (ad + 0x1b) = *(((unsigned char *) &sd_thisfile_clust) + 2);     // low word

    *(unsigned char *) (ad + 0x1c) = *(((unsigned char *) &sd_filesize_bytes) + 3);     // size
    *(unsigned char *) (ad + 0x1d) =
        *(((unsigned char *) &sd_filesize_bytes) + 2);
    *(unsigned char *) (ad + 0x1e) =
        *(((unsigned char *) &sd_filesize_bytes) + 1);
    *(unsigned char *) (ad + 0x1f) =
        *(((unsigned char *) &sd_filesize_bytes) + 0);

    if (hidden) {
        *(unsigned char *) (ad + 0x0b) |= 0x02;      // mark as hidden because file will be empty shortly
    }

    // Now write this sector back to the card
// FIXME need to add voltage protection here
    if (outpc.sd_status & 0x02) {
        sd_int_sector = sd_pos;
    } else {
        sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
    }
    sd_int_cmd = 4;     // write block command
    sd_int_phase = 1;   // first stage
    sd_match = SD_OK;
    sd_match_mask = 0xff;
    sd_int_addr = SDSECT1;       // where to read from
    sd_int_cnt = 0;
    sd_int_error = 0;
    SS_ENABLE;
    (void) SPI1SR;
    SPI1DRL = SD_CMD24 | 0x40;
    SPI1CR1 |= 0x80;    // enable SPI interrupts, on data received
}

/***********************************************************************************
  outpc.sd_error. Name indicates attempted action that failed
0 = no error
1 = sending clocks
2 = idle
3 = init
4 = set blocksize
5 = request CSD
6 = reading MBR
7 = MBR end marker not found (MBR missing or read corruption)
8 = no partition 1 defined
9 = reading partition boot sector
10 = reading directory
11 = (reading start of file) no longer used.
12 = reading FAT
13 = reading FAT continuation sector
14 = writing FAT
15 = reading directory
16 = writing directory
17 = writing log sector
18 = unsupported non-FAT16
19 = -
20 = SDHC detection
21 = VCA check pattern failed
22 = VCA voltage rejected
23 = OCR1
24 = OCR2
25 = OCR3
26 = OCR4
27 = -
28 = Processing CSD
29 = write failed
30 = directory full
31 = can't find space in FAT
32 = looks like VBR but not valid

outpc.sd_status mask
01 : Card present
02 : 0 = SD ; 1 = SDHC
04 : Ready
08 : Logging in process
10 : Error
20 : Ver 2.0 card
40 : FAT32
80 : VBR
*/
/************************************************************************************/
void do_sdcard(void)
{
    /* As of 2015-01-21, using non-paged space for SDcard buffers */

    if (!(ram4.log_style & 0xc0)) {     // SD disabled
        goto SD_end_here;
    }

    //SD card insert/de-insert handling
    if (sd_phase == 0x00) {        // presently inactive
        if (!(PTH & 0x20)) {    // card now inserted
            sd_phase = 1;
            DISABLE_INTERRUPTS;
            sd_lmms_last = lmms;
            ENABLE_INTERRUPTS;
            SPI_baudlow();      // set low rate for initial negotiation
            sd_subblock = 0;    // used as init retry counter
            sd_int_error = 0;   // error flag within interrupt
            outpc.sd_error = 0; // external error flag
            sd_int_phase = 0;   // phase
            outpc.sd_status = 1;        // card present
            sd_ledstat = 0x04;
            sd_ledclk = (unsigned int) lmms;    // only use low word
            sd_block = 0;
            sd_log_addr = SDSECT1;
            sd_int_cmd = 0;
            sd_int_phase = 0;
            sd_lmms_last2 = 0;
            sd_seq = 0;
            sd_retry = 0;
            sd_saved_dir = 0;
            sd_saved_offset = 0;
            sd_filenum = 0xffff;
        }
        goto SD_end_here;
    } else {                    // presently active
        if (PTH & 0x20) {       // card no longer present
            // run routine to disable
            sd_phase = 0;
            outpc.sd_status = 0;
            sd_ledstat = 0;
            goto SD_end_here;
        }
    }

    /* handle LED */
    if ((sd_ledstat & 0x07) == 0) {     // off
        sd_ledstat &= ~0x80;

    } else if ((sd_ledstat & 0x07) == 1) {      // full on - ready
        sd_ledstat |= 0x80;

    } else if ((sd_ledstat & 0x07) == 2) {      // alternating ~0.5s - logging
        if (((unsigned int) lmms - sd_ledclk) > 1953) {
            sd_ledstat ^= 0x80;
            sd_ledclk = (unsigned int) lmms;    // only use low word
        }

    } else if ((sd_ledstat & 0x07) == 3) {      // error flash codes (part 1)
        if (sd_ledstat & 0x80) {
            if (((unsigned int) lmms - sd_ledclk) > 780) {
                sd_ledclk = (unsigned int) lmms;        // only use low word
                sd_ledstat &= ~0x80;
                if (outpc.sd_error > 1) {
                    sd_block = outpc.sd_error - 1;
                    sd_ledstat = 5;
                }
            }
        } else {
            if (((unsigned int) lmms - sd_ledclk) > 15000) {
                sd_ledclk = (unsigned int) lmms;        // only use low word
                sd_ledstat |= 0x80;
            }
        }

    } else if ((sd_ledstat & 0x07) == 4) {      // alternating ~0.1s total period - init
        if (((unsigned int) lmms - sd_ledclk) > 390) {
            sd_ledstat ^= 0x80;
            sd_ledclk = (unsigned int) lmms;    // only use low word
        }

    } else if ((sd_ledstat & 0x07) == 5) { // error flash codes
        if (sd_ledstat & 0x80) {
            if (((unsigned int) lmms - sd_ledclk) > 780) {
                sd_ledclk = (unsigned int) lmms;        // only use low word
                sd_ledstat &= ~0x80;
                sd_block--;
                if (sd_block == 0) {
                    sd_ledstat = 3;
                }
            }
        } else {
            if (((unsigned int) lmms - sd_ledclk) > 3000) {
                sd_ledclk = (unsigned int) lmms;        // only use low word
                sd_ledstat |= 0x80;
            }
        }
    }        

    if (sd_ledstat & 0x80) {
        SSEM0SEI;
        *port_sdled |= pin_sdled;
        CSEM0CLI;
    } else {
        SSEM0SEI;
        *port_sdled &= ~pin_sdled;
        CSEM0CLI;
    }

    /* Handle pulse output */
    if (pin_sdpulse) {
        if ((flagbyte15 & FLAGBYTE15_SDLOGRUN) && ((flagbyte23 & FLAGBYTE23_SDPULSE_ACT) == 0)) {
            SSEM0SEI;
            *port_sdpulse |= pin_sdpulse; /* Enable */
            CSEM0CLI;
            flagbyte23 |= FLAGBYTE23_SDPULSE_ACT;
            flagbyte23 &= ~FLAGBYTE23_SDPULSE_DONE;
            sd_pulse_time = (unsigned int)lmms;
        } else if ((flagbyte23 & FLAGBYTE23_SDPULSE_ACT) && ((flagbyte23 & FLAGBYTE23_SDPULSE_DONE) == 0)) {
            if (((unsigned int)lmms - sd_pulse_time) > 790) { /* Hardcoded 100ms */
                SSEM0SEI;
                *port_sdpulse &= ~pin_sdpulse; /* Disable */
                CSEM0CLI;
                flagbyte23 |= FLAGBYTE23_SDPULSE_DONE;
            }
        } else if ((flagbyte15 & FLAGBYTE15_SDLOGRUN) == 0) {
            flagbyte23 &= ~FLAGBYTE23_SDPULSE_ACT; /* Allow a repeat */
        }
    }

    outpc.sd_phase = sd_phase;

    // Huge if structure that contains the SD card state machine.
    // Move the most common cases to the start and then goto the end to speed up.

    if (sd_phase == 0x41) {     // time to log?
        unsigned long ul_tmp;
        DISABLE_INTERRUPTS;
        ul_tmp = lmms;
        ENABLE_INTERRUPTS;
        if ((ram4.log_style & LOG_STYLE_BLOCKMASK) == LOG_STYLE_BLOCKSTREAM) {  // stream mode with 64 byte
            if (flagbyte6 & FLAGBYTE6_SD_GO) {
                flagbyte6 &= ~FLAGBYTE6_SD_GO;
                if (sd_subblock >= 2) {
                    sd_phase = 0x47;    // dummy phase to prevent any more blocks over-writing stream data
                } else {
                    sd_phase++;
                    sd_lmms_last = ul_tmp;
                }
            }
        } else if ((!(ram4.log_style2 & 0x18)) && ((ul_tmp - sd_lmms_last) >= sd_lmms_int)) {    // in timed mode
            sd_lmms_last = ul_tmp;
            sd_phase++;
        } else if ((ram4.log_style & LOG_STYLE_GPS) && (flagbyte22 & FLAGBYTE22_GPSLOG)) {
            flagbyte22 &= ~FLAGBYTE22_GPSLOG; /* ack it */
            sd_phase = 0x45;
        }

        if ((ram4.log_style & 0xc0) == 0x80) {    // button
            // check for the buttons
            if (flagbyte6 & FLAGBYTE6_SD_DEBOUNCE) {    // is button released yet
                if ((*port_sdbut & pin_sdbut) != pin_match_sdbut) {
                    flagbyte6 &= ~FLAGBYTE6_SD_DEBOUNCE;
                    DISABLE_INTERRUPTS;
                    sd_lmms_last2 = lmms;       // using this to debounce next press
                    ENABLE_INTERRUPTS;
                }

            } else {
                // has button been pressed to end the log?
                if ((*port_sdbut & pin_sdbut) == pin_match_sdbut) {
                    unsigned long ul_tmpb;
                    DISABLE_INTERRUPTS;
                    ul_tmpb = lmms;
                    ENABLE_INTERRUPTS;
                    if ((ul_tmpb - sd_lmms_last2) > 7812) {        // ignore button until 1s after last action
                        flagbyte15 &= ~FLAGBYTE15_SDLOGRUN; // don't do continous logs
                        sd_phase = 0x48;        // setup to write buffer and return to start
                    }
                }
            }
        }
        goto SD_end_here;

    } else if (sd_phase == 0x42) {      // grab a block of data
        //read from outpc and write to log ram buffer
        unsigned char *dest_addr, *st_ad;
        unsigned int x;
        int y;

        dest_addr =
            (unsigned char *) (sd_log_addr + (LOG_SIZE * sd_subblock));
        st_ad = dest_addr;

        *((unsigned long *) dest_addr) = sd_block;
        if (sd_protocol > 1) {
          *((unsigned char *) dest_addr) = BLOCKDATA; /* Regular data block */  
        }
        dest_addr += 4;

        DISABLE_INTERRUPTS;
        *((unsigned long *) &(*dest_addr)) = lmms;      // store lmms at start of log
        ENABLE_INTERRUPTS;
        dest_addr += 4;

        // no method here to prevent overflowing our buffer - perform check in init to save time here
        for (x = 0; x < 47; x++) {
            unsigned char sz;
            unsigned int off;

            sz = ram4.log_size[(int) x];
            off = ram4.log_offset[(int) x];

            if (off & 0x400) {  // internal vars
                off &= 0x3ff;
                /* internal vars at present
                  1024  0 tooth_no U08
                  1025  1 synch U08
                  1026  2 cum_cycle U16
                  1027  3 cum_tooth U16
                  1028  4 coilsel U16
                  1029  5 dwellsel U16
                  1030  6 PTM U08
                  1031  7 PTJ U08
                  1032  8 PTT U08
                  1033  9 PORTA U08
                  1034 10 PORTB U08
                  1035 11 next_dwell.tooth
                  1036 12 next_spark.tooth
                  1037 13 next_inj[0].tooth
                  1038 14 next_inj[1].tooth
                  1039 15 next_inj[2].tooth
                  1040 16 next_inj[3].tooth
                  1041 17 next_inj[4].tooth
                  1042 18 next_inj[5].tooth
                  1043 19 next_inj[6].tooth
                  1044 20 next_inj[7].tooth
                  1046 22  xxxxx
                  1047 23 wheeldec_ovflo
                  1048 24 next_dwell.time32
                  1049 25 next_spark.time32
                  1050 26 TIE
                  1051 27 TFLG1
                  1052 28 TIMTIE
                  1053 29 TIMTFLG1
                  1054 30 next_inj[0].time32
                  1055 31 next_inj[1].time32
                  1056 32 next_inj[2].time32
                  1057 33 next_inj[3].time32
                  1058 34 next_inj[4].time32
                  1059 35 next_inj[5].time32
                  1060 36 next_inj[6].time32
                  1061 37 next_inj[7].time32
                  1062 38 XGSEM (low byte)
                  1063 39 pwcalc1
                  1064 40 pwcalc2
                  1065 41 next_map_start_event.tooth
                  1066 42 XGCHID
                  1067 43 XGSWT
                  1068 44 xgate_deadman
                  1069 45 XGMCTL
                  1070 46 xgpc_copy
                  1071 47 xgswe_count
                  1072 48 next_fuel
                  1073 49 inj_cnt[0]
                  1074 50 inj_cnt[1]
                  1075 51 inj_cnt[2]
                  1076 52 inj_cnt[3]
                  1077 53 inj_cnt[4]
                  1078 54 inj_cnt[5]
                  1079 55 inj_cnt[6]
                  1080 56 inj_cnt[7]
                 */

                if (off == 0) {
                    *dest_addr = tooth_no;
                } else if (off == 1) {
                    *dest_addr = synch;
                } else if (off == 2) {
                    *((unsigned int *) &(*dest_addr)) = cum_cycle;
                } else if (off == 3) {
                    *((unsigned int *) &(*dest_addr)) = cum_tooth;
                } else if (off == 4) {
                    *((unsigned int *) &(*dest_addr)) = coilsel;
                } else if (off == 5) {
                    *((unsigned int *) &(*dest_addr)) = dwellsel;
                } else if (off == 6) {
                    *dest_addr = PTM;
                } else if (off == 7) {
                    *dest_addr = PTJ;
                } else if (off == 8) {
                    *dest_addr = PTT;
                } else if (off == 9) {
                    *dest_addr = PORTA;
                } else if (off == 10) {
                    *dest_addr = PORTB;
                } else if (off == 11) {
                    *dest_addr = next_dwell.tooth;
                } else if (off == 12) {
                    *dest_addr = next_spark.tooth;
                } else if (off == 13) {
                    *dest_addr = next_inj[0].tooth;
                } else if (off == 14) {
                    *dest_addr = next_inj[1].tooth;
                } else if (off == 15) {
                    *dest_addr = next_inj[2].tooth;
                } else if (off == 16) {
                    *dest_addr = next_inj[3].tooth;
                } else if (off == 17) {
                    *dest_addr = next_inj[4].tooth;
                } else if (off == 18) {
                    *dest_addr = next_inj[5].tooth;
                } else if (off == 19) {
                    *dest_addr = next_inj[6].tooth;
                } else if (off == 20) {
                    *dest_addr = next_inj[7].tooth;
                } else if (off == 21) {
                    *dest_addr = 0;


                } else if (off == 23) {
                    *dest_addr = wheeldec_ovflo;
                } else if (off == 24) {
                    DISABLE_INTERRUPTS;
                    *((unsigned long *) &(*dest_addr)) = next_dwell.time32;
                    ENABLE_INTERRUPTS;
                } else if (off == 25) {
                    DISABLE_INTERRUPTS;
                    *((unsigned long *) &(*dest_addr)) = next_spark.time32;
                    ENABLE_INTERRUPTS;
                } else if (off == 26) {
                    *dest_addr = TIE;
                } else if (off == 27) {
                    *dest_addr = TFLG1;
                } else if (off == 28) {
                    *dest_addr = TIMTIE;
                } else if (off == 29) {
                    *dest_addr = TIMTFLG1;
                } else if (off == 30) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[0].time;
                } else if (off == 31) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[1].time;
                } else if (off == 32) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[2].time;
                } else if (off == 33) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[3].time;
                } else if (off == 34) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[4].time;
                } else if (off == 35) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[5].time;
                } else if (off == 36) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[6].time;
                } else if (off == 37) {
                    *((unsigned int *) &(*dest_addr)) = next_inj[7].time;
                } else if (off == 38) {
                    *dest_addr = XGSEM;
                } else if (off == 39) {
                    *((unsigned int *) &(*dest_addr)) = pwcalc1;
                } else if (off == 40) {
                    *((unsigned int *) &(*dest_addr)) = pwcalc2;
                } else if (off == 41) {
                    *dest_addr = next_map_start_event.tooth;
                } else if (off == 42) {
                    *dest_addr = XGCHID;
                } else if (off == 43) {
                    *dest_addr = XGSWT;
                } else if (off == 44) {
                    *dest_addr = xgate_deadman;
                } else if (off == 45) {
                    *dest_addr = (unsigned char)XGMCTL;
                } else if (off == 46) {
                    *((unsigned int *) &(*dest_addr)) = xgpc_copy;
                } else if (off == 47) {
                    *dest_addr = xgswe_count;
                } else if (off == 48) {
                    *dest_addr = next_fuel;
                } else if (off == 49) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[0];
                } else if (off == 50) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[1];
                } else if (off == 51) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[2];
                } else if (off == 52) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[3];
                } else if (off == 53) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[4];
                } else if (off == 54) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[5];
                } else if (off == 55) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[6];
                } else if (off == 56) {
                    *((unsigned int *) &(*dest_addr)) = inj_cnt[7];
                }
// have to honour size even if wrong somehow
                if (sz == 1) {  // 8 bit
                    dest_addr++;
                } else if (sz == 2) {   // 16 bit
                    dest_addr += 2;
                } else if (sz == 4) {   // 32 bit
                    dest_addr += 4;
                }
            } else {            // outpc
                off &= 0x3ff;
                if (sz == 1) {  // 8 bit
                    *dest_addr = *((unsigned char *) &outpc + off);
                    dest_addr++;
                } else if (sz == 2) {   // 16 bit
                    *((unsigned int *) &(*dest_addr)) = *((unsigned int *)&(*((unsigned char *) &outpc + off)));
                    dest_addr += 2;
                } else if (sz == 4) {   // 32 bit
                    unsigned long *ul_tmp_ad;

                    ul_tmp_ad = (unsigned long *)
                        &(*((unsigned char *) &outpc + off));
                    DISABLE_INTERRUPTS;
                    *((unsigned long *) &(*dest_addr)) = *((unsigned long *) ul_tmp_ad);        // nice quick ASM
                    ENABLE_INTERRUPTS;
                    dest_addr += 4;
                }
            }
        }
        y = st_ad + 64 - dest_addr;
        if (y > 0) {
            for (x = 0; (int)x < y; x++) {
                *dest_addr = 0;
                dest_addr++;
            }
        }
        *(unsigned char *) (st_ad + 0x3f) = sd_magic;   // write magic number into every datablock
        sd_block++;
        sd_subblock++;

/* now done in phase 41
        DISABLE_INTERRUPTS;     // reset timer
        sd_lmms_last = lmms;
        ENABLE_INTERRUPTS;
*/
        if (((ram4.log_style & LOG_STYLE_BLOCKMASK) == LOG_STYLE_BLOCKSTREAM) && (sd_subblock >= 2)) {
            sd_phase = 0x47;    // dummy phase to prevent any more blocks over-writing stream data
        } else if (sd_subblock >= sd_blsect) {
            sd_phase++;         // go to next phase
        } else {
            sd_phase = 0x41;    // go back to wait phase
        }
        goto SD_end_here;

    } else if (sd_phase == 0x43) {
        if (sd_int_phase == 0) {        // write block phase
            if (sd_int_error) {
                outpc.sd_error = 17;
                sd_phase = 0xfe;
                goto SD_end_here;
            }
            // check voltage first
            if (ATD0DR4 > MIN_VOLTS) {  // > 6.5V to avoid writing during power fail which might corrupt card
                // setup for a interrupt driven block write
                sd_int_addr = sd_log_addr;
                sd_int_cnt = 0; // counts up to blocksize
                if (outpc.sd_status & 0x02) {
                    sd_int_sector = sd_pos;
                } else {
                    sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
                }
                sd_int_cmd = 4;
                sd_int_phase = 1;       // first stage
                sd_int_error = 0;
                SS_ENABLE;
                (void) SPI1SR;
                (void) SPI1DRL; // ensure nothing pending
                SPI1DRL = SD_CMD24 | 0x40;      // send the command (first byte in here)
                SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
                sd_pos++;       // be ready for next sector

                // interrupt now writing buffer to SD card. Swap buffers and continue logging.
                sd_subblock = 0;        // start at beginning of sector
                if (sd_log_addr == SDSECT1) {
                    sd_log_addr = SDSECT2;       // this is also hardcoded and reflects 512byte sector - which should be safe
                } else {
                    sd_log_addr = SDSECT1;
                }

                flagbyte6 &= ~FLAGBYTE6_STREAM_HOLD;    // take off the hold

                if ((sd_block >= sd_maxlogblocks) || (!(outpc.sd_status & 0x08))) {     // file full or not logging anymore
                    sd_phase = 0x44;
                } else {
                    sd_phase = 0x41;
                }
            }
        }
        goto SD_end_here;

    } else if (sd_phase == 0x44) {
        if (sd_int_phase == 0) {        // wait for last write to finish, then start over
            // Either create a new empty log for button mode, or reset and do it again for insertion mode

            if (sd_block < sd_maxlogblocks) {
                // need to change file size in directory and re-do cluster chain - fun...
                // Ignore for now.
            }

            outpc.sd_status &= ~0x0c;   // not logging any more and no longer ready

            sd_pos = sd_dir_start;      // set up to read from the beginning of directory
            sd_uitmp2 = 0;
            sd_block = 0;       // file count
            sd_subblock = 0;    // file sub count
            sd_filenum = 0xffff;
            sd_ledstat = 0x04;  // init phase again

            sd_phase = 0x16;    // this means read the root directory for the start and find/create a file
        }
        goto SD_end_here;

    } else if (sd_phase == 0x45) {
        /* interleave a GPS packet */
        /* code mostly copied from phase 0x42 */
        //read from outpc and write to log ram buffer
        unsigned char *dest_addr, *st_ad;
        unsigned int x;
        int y;

        dest_addr =
            (unsigned char *) (sd_log_addr + (LOG_SIZE * sd_subblock));
        st_ad = dest_addr;

        *((unsigned long *) dest_addr) = sd_block;
        *((unsigned char *) dest_addr) = BLOCKGPS; /* GPS data */  
        dest_addr += 4;

        DISABLE_INTERRUPTS;
        *((unsigned long *) &(*dest_addr)) = lmms;      // store lmms at start of log
        ENABLE_INTERRUPTS;
        dest_addr += 4;

        *((signed char *) dest_addr) = outpc.gps_latdeg;
        dest_addr += 1;

        *((unsigned char *) dest_addr) = outpc.gps_latmin;
        dest_addr += 1;

        *((unsigned int *) dest_addr) = outpc.gps_latmmin;
        dest_addr += 2;

        *((unsigned char *) dest_addr) = outpc.gps_londeg;
        dest_addr += 1;

        *((unsigned char *) dest_addr) = outpc.gps_lonmin;
        dest_addr += 1;

        *((unsigned int *) dest_addr) = outpc.gps_lonmmin;
        dest_addr += 2;

        *((unsigned char *) dest_addr) = outpc.gps_outstatus;
        dest_addr += 1;

        *((signed char *) dest_addr) = outpc.gps_altk;
        dest_addr += 1;

        *((signed int *) dest_addr) = outpc.gps_altm;
        dest_addr += 2;

        *((unsigned int *) dest_addr) = outpc.gps_speedkm;
        dest_addr += 2;

        *((unsigned int *) dest_addr) = outpc.gps_course;
        dest_addr += 2;

        y = st_ad + 64 - dest_addr;
        if (y > 0) {
            for (x = 0; (int)x < y; x++) {
                *dest_addr = 0;
                dest_addr++;
            }
        }
        *(unsigned char *) (st_ad + 0x3f) = sd_magic;   // write magic number into every datablock
        sd_block++;
        sd_subblock++;

        if (((ram4.log_style & LOG_STYLE_BLOCKMASK) == LOG_STYLE_BLOCKSTREAM) && (sd_subblock >= 2)) {
            sd_phase = 0x47;    // dummy phase to prevent any more blocks over-writing stream data
        } else if (sd_subblock >= sd_blsect) {
            sd_phase = 0x43;     // go to next phase
        } else {
            sd_phase = 0x41;    // go back to wait phase
        }
        goto SD_end_here;

    } else if (sd_phase == 0x47) {
// dummy phase 0x47
        goto SD_end_here;

    } else if (sd_phase == 0x48) {
        // Quit logging.
        // fill remainder of buffer with zeros and write it.
        unsigned char *dest_addr;
        unsigned int x;
        dest_addr =
            (unsigned char *) (sd_log_addr + (LOG_SIZE * sd_subblock));
        while (sd_subblock < sd_blsect) {
            for (x = 0; x < LOG_SIZE; x++) {
                *dest_addr = 0;
                dest_addr++;
            }
            sd_subblock++;
        }
        outpc.sd_status &= ~0x08;       // not logging any more
        sd_phase = 0x43;        // write buffer and return to start
        goto SD_end_here;

        /* phase 1 */

    } else if (sd_phase == 0x01) {      // init stage 1
        unsigned long ul_tmp;
        DISABLE_INTERRUPTS;
        ul_tmp = lmms;
        ENABLE_INTERRUPTS;
        if (PTH & 0x20) {       // card no longer present
            sd_phase = 0;
        } else if ((ul_tmp - sd_lmms_last) > 3906) {    // card has been inserted for at least 500ms to prevent bounce
            sd_log_addr = SDSECT1;       // start with first block
            sd_ledstat = 0x04;  // fast flashing
            sd_phase++;
        }

    } else if (sd_phase == 0x02) {
        SPI_baudlow();      // set low rate for initial negotiation
        sd_int_cmd = 0;         // init, send clocks
        sd_int_phase = 1;
        sd_int_error = 0;
        SS_ENABLE;  // doesn't work if this is missing
        (void) SPI1SR;
        SPI1DRL = 0xff;         // write first byte
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        DISABLE_INTERRUPTS;
        sd_lmms_last = lmms;
        ENABLE_INTERRUPTS;
        sd_phase++;

    } else if (sd_phase == 0x03) {
        if ((ram5.u08_debug38 & 0x01) == 0) {
            // int handler turns off ints and lets us poll
            if ((sd_int_phase == 0) && (SPI1SR & 0x80)) {
                (void) SPI1SR;
                (void) SPI1DRL;                    
                if (sd_int_error) {
                    outpc.sd_error = 1;
                    sd_phase = 0xfe;
                } else {
                    /* IDLE Command */
                    sd_int_sector = 0;
                    sd_int_cmd = 10;        // send command
                    sd_int_phase = 1;       // first stage
                    sd_match = SD_IDLE;     // expecting idle result code
                    sd_match_mask = 0x01;
                    sd_int_error = 0;
                    sd_rx = 0x95;   // Checksum matters here
                    SS_ENABLE;
                    (void) SPI1SR;
                    (void) SPI1DRL; // ensure nothing hanging around
                    SPI1DRL = SD_CMD0 | 0x40;       // send the Idle command to reset the card
                    SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
                    sd_phase++;
                }
            } else {                // check for timeout
                unsigned long ul_tmp;
                DISABLE_INTERRUPTS;
                ul_tmp = lmms;
                ENABLE_INTERRUPTS;
                if ((ul_tmp - sd_lmms_last) > 800) {        // abnormal delay
                    if (sd_subblock < 4) {
                        sd_subblock++;      // retry counter
                        sd_phase = 1;
                    }
                }
            }
        } else {
            /* test code */
            SPI1CR1 &= ~0x80;
            sd_retry++;
            if (sd_retry > 20) {
                SS_ENABLE;
                sd_retry = 0;
                (void)SPI1SR;
                (void)SPI1DRL;
                SPI1DRL = 0x5a;
                SPI_baudhigh();
                SS_DISABLE;
            }
        }
    } else if ((sd_phase == 0x04) && (sd_int_phase == 0)) {
        // int handler handles all writes and reads. No need to poll.
        if (sd_int_error) {
            SS_DISABLE;
            if (sd_subblock < 6) {
                sd_subblock++;  // retry counter
                DISABLE_INTERRUPTS;
                sd_lmms_last = lmms;
                ENABLE_INTERRUPTS;
                sd_int_error = 0;
//                sd_phase = 0x02;
                sd_phase = 0x80;
                sd_match = 0x02;
            } else {
                outpc.sd_error = 2;
                sd_phase = 0xfe;
            }
        } else {
            SS_DISABLE;
            SPI1CR1 &= ~0x80;   // turn off ints so foreground code can work
            SPI1DRL = 0xFF;     // send to receive
            sd_phase++;
        }

    } else if (sd_phase == 0x05) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            sd_int_cnt = 0;
            sd_phase++;
        }

    } else if (sd_phase == 0x06) {
// for SD + SDHC need to use different init phase. Jump to step 0x60
        sd_nofats = 0; // used as error counter in SD/SDHC code
        sd_phase = 0x60;

// returns back here to step 0x08
    } else if (sd_phase == 0x08) {
        sd_int_sector = SD_BLOCK_SIZE;
        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD16 | 0x40;      // Set block length
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x09) && (sd_int_phase == 0)) {
        if (sd_int_error) {
            sd_phase = 0x02;
            goto SD_end_here;
        } else {
            SS_DISABLE;
            SPI1CR1 &= ~0x80;   // turn off ints so foreground code can work
            (void) SPI1SR;
            SPI1DRL = 0xff;     // dummy write
            sd_phase++;
        }

    } else if (sd_phase == 0x0a) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read

            SPI_baudhigh();     // might want to send a bunch of clocks after this?

            (void) SPI1SR;
            SPI1DRL = 0xff;     // dummy write

            sd_subblock = 0;
            sd_phase++;
        }

    } else if (sd_phase == 0x0b) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            sd_phase++;
        }

    } else if (sd_phase == 0x0c) {
        sd_int_sector = 0;
        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD9 | 0x40;       // Request CSD
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if (sd_phase == 0x0d) {
        // int handler handles all writes and reads. No need to poll.
        if (sd_int_phase == 0) {
            if (sd_int_error) {
                sd_phase = 2;   // start over again
            } else {
                SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work
                (void) SPI1SR;
                SPI1DRL = 0xff; // dummy write
                sd_int_cnt = 0;
                sd_phase++;
            }
        }

    } else if (sd_phase == 0x0e) {
        if (SPI1SR & 0x80) {
            sd_rx = SPI1DRL;
            if (sd_rx == 0xfe) {        // wait for 0xFE start token
                sd_int_cnt = 0;
                SPI1DRL = 0xff; // dummy write
                sd_phase++;
            } else {
                if (sd_int_cnt < 200) {
                    sd_int_cnt++;
                    SPI1DRL = 0xff;     // dummy write
                } else {        // retry
                    if (sd_subblock < 4) {
                        sd_subblock++;  // retry counter
                        SS_DISABLE;
                        sd_phase = 0x0c;        // try fetch CSD again
                    } else {
                        sd_phase = 0xfe;
                        outpc.sd_error = 5;
                    }
                }
            }
        }

    } else if (sd_phase == 0x0f) {
        if (SPI1SR & 0x80) {
            vSD_CSD[sd_int_cnt++] = SPI1DRL;
            SPI1DRL = 0xff;     // dummy write
            if (sd_int_cnt == 16) {
                SPI1DRL = 0xff; // dummy write
                sd_phase++;
            }
        }

    } else if (sd_phase == 0x10) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;
            SPI1DRL = 0xff;
            sd_phase++;
        }

    } else if (sd_phase == 0x11) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;
            SS_DISABLE;
            SPI1DRL = 0xff;
            sd_phase++;
        }

    } else if (sd_phase == 0x12) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;
            sd_phase++;
        }

    } else if (sd_phase == 0x13) {
        unsigned int u16Temp;
        // process CSD

        // Block Size
        sd_sectsize = 1 << (vSD_CSD[5] & 0x0F); // READ_BL_LEN

        if (outpc.sd_status & 0x02) {   // SD v2.0
            unsigned char v;
            v = vSD_CSD[0] & 0xc0;
            if (v != 0x40) {
                outpc.sd_error = 28;
                sd_phase = 0xfe;
                goto SD_end_here;
            }
            u32MaxBlocks =
                ((unsigned long) (vSD_CSD[7] & 0x3f) << 16) | (vSD_CSD[8]
                                                               << 8) |
                vSD_CSD[9];
            u32MaxBlocks = u32MaxBlocks * 1024;     // number of sectors (of 512 bytes each)
            asm("nop");

        } else {                // SD v1.0
            unsigned char v;
            v = vSD_CSD[0] & 0xc0;
            if (v != 0x00) {
                outpc.sd_error = 28;
                sd_phase = 0xfe;
                goto SD_end_here;
            }
            // Max Blocks
            u16Temp = (vSD_CSD[10] & 0x80) >> 7;        // C_SIZE_MULT
            u16Temp += (vSD_CSD[9] & 0x03) << 1;
            u32MaxBlocks = vSD_CSD[6] & 0x03;
            u32MaxBlocks = u32MaxBlocks << 8;
            u32MaxBlocks += vSD_CSD[7];
            u32MaxBlocks = u32MaxBlocks << 2;
            u32MaxBlocks += (vSD_CSD[8] & 0xC0) >> 6;
            u32MaxBlocks++;
            u32MaxBlocks = u32MaxBlocks << (u16Temp + 2);
        }

        // Patch for SD Cards of 2 GB
        if (sd_sectsize > 512) {
            sd_sectsize = 512;
            u32MaxBlocks = u32MaxBlocks << 1;
        }

        if (outpc.sd_status & 0x02) {
            sd_int_sector = 0;   // read MBR at block 0
        } else {
            sd_int_sector = 0 << SD_BLOCK_SHIFT; // read MBR at block 0
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;      // read block command
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x14) && (sd_int_phase == 0)) {
        unsigned char part_type;

        if (sd_int_error) {
            outpc.sd_error = 6;
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        // check for 55AA
        if (*((unsigned int *) (SDSECT1 + 0x1fe)) != 0x55AA) {     //byte 0x1fe
            outpc.sd_error = 7;
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        // Handle partition 1 only.
        // data is little endian so have to reverse it
        *(unsigned char *) &sd_part_start = *((unsigned char *) (SDSECT1 + 0x1c9));
        *((unsigned char *) &sd_part_start + 1) = *((unsigned char *) (SDSECT1 + 0x1c8));
        *((unsigned char *) &sd_part_start + 2) = *((unsigned char *) (SDSECT1 + 0x1c7));
        *((unsigned char *) &sd_part_start + 3) = *((unsigned char *) (SDSECT1 + 0x1c6));


        if (sd_part_start == 0x00000000) {
            /* weak detection of VBR vs. MBR */
            if (((*((unsigned char *) (SDSECT1 + 0x26)) == 0x29)
                && (*((unsigned char *) (SDSECT1 + 0x36)) == 'F')
                && (*((unsigned char *) (SDSECT1 + 0x37)) == 'A')
                && (*((unsigned char *) (SDSECT1 + 0x38)) == 'T')
                && (*((unsigned char *) (SDSECT1 + 0x39)) == '1')
                && (*((unsigned char *) (SDSECT1 + 0x3a)) == '6'))

                || (((*((unsigned char *) (SDSECT1 + 0x42)) == 0x29))
                && (*((unsigned char *) (SDSECT1 + 0x52)) == 'F')
                && (*((unsigned char *) (SDSECT1 + 0x53)) == 'A')
                && (*((unsigned char *) (SDSECT1 + 0x54)) == 'T')
                && (*((unsigned char *) (SDSECT1 + 0x55)) == '3')
                && (*((unsigned char *) (SDSECT1 + 0x56)) == '2'))) { // found FAT16/32 sigs, assume VBR
                    sd_part_start = 0;
                    outpc.sd_status |= 0x80; // VBR
                    sd_phase++; // go forwards without reading new data.
                    goto SD_end_here;
            } else {
                // looks like MBR, but no partition?
                outpc.sd_error = 8;
                sd_phase = 0xfe;
                goto SD_end_here;
            }
        }

        part_type = *((unsigned char *) (SDSECT1 + 0x1c2));

        if (!((part_type == 0x0b) || (part_type == 0x0c)
              || (part_type == 0x04) || (part_type == 0x06)
              || (part_type == 0x0e))) {
            // Note that if you reformat a partition the partition table is NOT updated to show what file system is on there, so this
            // byte is an unreliable source to determine FAT16 vs FAT32 etc.

            if ((*((unsigned char *) (SDSECT1 + 0x1be)) & 0x7f) == 0) {
                // partition entry looks ok, but unsupported type - error out
                outpc.sd_error = 18;
                sd_phase = 0xfe;
                outpc.status4 = part_type;
                goto SD_end_here;
            }
        }

        if ((*((unsigned char *) (SDSECT1 + 0x1be)) & 0x7f) || (*((unsigned char *) (SDSECT1 + 0x1ce)) & 0x7f)
            || (*((unsigned char *) (SDSECT1 + 0x1de)) & 0x7f) || (*((unsigned char *) (SDSECT1 + 0x1ee)) & 0x7f)) {
            // partition table not valid
            // How are we supposed to be sure this is a VBR???
            if ((*((unsigned char *) (SDSECT1 + 0x1d2)) == 'e') && (*((unsigned char *) (SDSECT1 + 0x1d3)) == 'r')
                && (*((unsigned char *) (SDSECT1 + 0x1d4)) == 'r') && (*((unsigned char *) (SDSECT1 + 0x1d5)) == 'o')
                && (*((unsigned char *) (SDSECT1 + 0x1d6)) == 'r') && (*((unsigned char *) (SDSECT1 + 0x1d7)) == 0xff)) {
                // Reverse engineered from the message that XP writes, looks like a VBR
                // Don't need to read partition boot sector, because there isn't one, the BPB in this
                // sector ought to contain what would have gathered in phase 0x15
                if ((*((unsigned char *) (SDSECT1 + 0x26)) == 0x29) || ((*((unsigned char *) (SDSECT1 + 0x42)) == 0x29))) { // expected FAT16/32 sigs
                    sd_part_start = 0;
                    outpc.sd_status |= 0x80; // VBR
                    sd_phase++; // go forwards without reading new data.
                    goto SD_end_here;
                } else {
                    outpc.sd_error = 32;
                    sd_phase = 0xfe;
                    goto SD_end_here;
                }

            } else {
                outpc.sd_error = 18;
                sd_phase = 0xfe;
                outpc.status4 = part_type;
                goto SD_end_here;
            }
        }

        SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work

        // read the boot sector of the partition
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_part_start;
        } else {
            sd_int_sector = sd_part_start << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x15) && (sd_int_phase == 0)) {
        unsigned int u16Temp;
        if (sd_int_error) {
            outpc.sd_error = 9;
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        // Figure out what type of partition is on here

        sd_sect2clust = *((unsigned char *) (SDSECT1 + 0xd));

        sd_fat_start =
            sd_part_start + (*((unsigned char *) (SDSECT1 + 0xe)) +
                             (*((unsigned char *) (SDSECT1 + 0xf)) << 8));
        // offset 0x0e is little-endian word num of sectors reserved for boot sector before FAT starts

        // Max roots - only for FAT16
        *(unsigned char *) &sd_max_roots = *((unsigned char *) (SDSECT1 + 0x12));
        *((unsigned char *) &sd_max_roots + 1) = *((unsigned char *) (SDSECT1 + 0x11));

        // sectors per FAT
        *((unsigned char *) &sd_fatsize + 3) = *((unsigned char *) (SDSECT1 + 0x16));
        *((unsigned char *) &sd_fatsize + 2) = *((unsigned char *) (SDSECT1 + 0x17));
        *((unsigned char *) &sd_fatsize + 1) = 0;       // clear top word
        *((unsigned char *) &sd_fatsize + 0) = 0;       // clear top word

        sd_nofats = *((unsigned char *) (SDSECT1 + 0x10));

        if ((sd_max_roots == 0) || (sd_fatsize == 0)) {
            if (*((unsigned char *) (SDSECT1 + 0x42)) == 0x29) {
                unsigned long ult;
                outpc.sd_status |= 0x40;        // FAT32 detected
                *(unsigned char *) &sd_fatsize = *((unsigned char *) (SDSECT1 + 0x27));
                *((unsigned char *) &sd_fatsize + 1) = *((unsigned char *) (SDSECT1 + 0x26));
                *((unsigned char *) &sd_fatsize + 2) = *((unsigned char *) (SDSECT1 + 0x25));
                *((unsigned char *) &sd_fatsize + 3) = *((unsigned char *) (SDSECT1 + 0x24));

                // cluster start of root directory
                *(unsigned char *) &ult = *((unsigned char *) (SDSECT1 + 0x2f));
                *((unsigned char *) &ult + 1) = *((unsigned char *) (SDSECT1 + 0x2e));
                *((unsigned char *) &ult + 2) = *((unsigned char *) (SDSECT1 + 0x2d));
                *((unsigned char *) &ult + 3) = *((unsigned char *) (SDSECT1 + 0x2c));

                sd_file_start = sd_fat_start + (sd_fatsize * sd_nofats);        // start of data area beyond fats
                sd_dir_start = sd_file_start + ((ult - 2) * sd_sect2clust);     // 
                // temporary
                sd_max_roots = sd_sect2clust * (sd_sectsize / 32);        // limit until we handle fragmentation of root dir

            } else {
                outpc.sd_error = 18;
                sd_phase = 0xfe;
                goto SD_end_here;
            }
        } else {
            outpc.sd_status &= ~0x40;   // Must be FAT16
            u16Temp = sd_fatsize * sd_nofats;
            sd_dir_start = sd_fat_start + u16Temp;
            sd_file_start = sd_dir_start + ((sd_max_roots * 32) / sd_sectsize);
        }

        if (*((unsigned int *) (SDSECT1 + 0x13)) == 0) {
            *(unsigned char *) &sd_tot_sect = *((unsigned char *) (SDSECT1 + 0x23));
            *((unsigned char *) &sd_tot_sect + 1) = *((unsigned char *) (SDSECT1 + 0x22));
            *((unsigned char *) &sd_tot_sect + 2) = *((unsigned char *) (SDSECT1 + 0x21));
            *((unsigned char *) &sd_tot_sect + 3) = *((unsigned char *) (SDSECT1 + 0x20));
        } else {
            *(unsigned char *) &sd_tot_sect = *((unsigned char *) (SDSECT1 + 0x14));
            *((unsigned char *) &sd_tot_sect + 1) = *((unsigned char *) (SDSECT1 + 0x13));
        }

        sd_uitmp2 = 0;          // file count
        sd_subblock = 0;        // file sub count
        sd_filenum = 0xffff;
        sd_filenum_low = 0xffff;

        if (flagbyte6 & FLAGBYTE6_SD_MANUAL) {
            // in manual mode we skip all of this
            outpc.sd_status |= 0x04; // say we are ready
            sd_phase = 0x80;        // do dummy read
            sd_match = 0x2c;        // next phase
        } else {
            sd_pos = sd_dir_start;  // set up to read from the beginning of directory
            sd_phase = 0x80;        // do dummy read
            sd_match = 0x16;        // next phase
        }

    } else if (sd_phase == 0x16) {
        // read the root dir

        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x17) && (sd_int_phase == 0)) {
        unsigned char file2sect, i, j;
        unsigned int ad;
        if (sd_int_error) {
            outpc.sd_error = 10;
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        file2sect = sd_sectsize / 32;   // directory entry is 32 bytes
        // scan directory for LOGxxxx.MS3
        i = 0;
        j = 0;

        while (!j) {
            ad = SDSECT1 + (i * 32);
            if (*(unsigned char *) ad == 0) {
                // end of directory
                j = 3;
            } else if ((((*(unsigned char *) (ad + 0x0b)) & ~0x22) == 0) && // ignore archive and hidden
                       ((*(unsigned char *) (ad + 0x00)) == 'L') &&
                       ((*(unsigned char *) (ad + 0x01)) == 'O') &&
                       ((*(unsigned char *) (ad + 0x02)) == 'G') &&
                       ((*(unsigned char *) (ad + 0x07)) == ' ') &&
                       ((*(unsigned char *) (ad + 0x08)) == 'M') &&
                       ((*(unsigned char *) (ad + 0x09)) == 'S') &&
                       ((*(unsigned char *) (ad + 0x0a)) == '3')) {
                unsigned int t1;
                unsigned char t2, t3;
                // found a matching file, see if it is the new highest
                t1 = *((unsigned int *) &(*(unsigned char *) (ad + 0x03)));     // grab first two digits
                t2 = asc2num(t1);       // convert ascii hex to uchar

                t1 = *((unsigned int *) &(*(unsigned char *) (ad + 0x05)));     // grab second two digits
                t3 = asc2num(t1);       // convert ascii hex to uchar

                t1 = (t2 << 8) | t3;

                /* record lowest file number for case of loop erasing */
                if (t1 < sd_filenum_low) {
                    sd_filenum_low = t1;
                }

                if ((t1 > sd_filenum) || (sd_filenum == 0xffff)) {
                    *(unsigned char *) &sd_filesize_bytes =
                        *((unsigned char *) (ad + 0x1f));
                    *((unsigned char *) &sd_filesize_bytes + 1) =
                        *((unsigned char *) (ad + 0x1e));
                    *((unsigned char *) &sd_filesize_bytes + 2) =
                        *((unsigned char *) (ad + 0x1d));
                    *((unsigned char *) &sd_filesize_bytes + 3) =
                        *((unsigned char *) (ad + 0x1c));

                    if (sd_filesize_bytes != 0) {       // non-zero file
                        sd_filenum = t1;        // found new (natural) high  filename number
                        sd_subblock = sd_uitmp2;        // highest file sub count, so we can jump to directory entry in future

                        if (outpc.sd_status & 0x40) {
                            sd_thisfile_clust = (unsigned long)
                                (((unsigned int) (*(unsigned char *) (ad + 0x15)) << 8) 
                                  | (*(unsigned char *) (ad + 0x14))) << 16;
                        } else {
                            sd_thisfile_clust = 0;
                        }
                        sd_thisfile_clust |= ((unsigned int) (*(unsigned char *) (ad + 0x1b)) << 8)
                             | (*(unsigned char *) (ad + 0x1a));
                        sd_saved_dir = sd_pos; // save address of this directory sector
                        sd_saved_offset = ad; // offset within directory sector
                    }
                }
            }

            i++;
            sd_uitmp2++;
            if (i > file2sect) {
                j = 2;
            }
        }

        if (j == 2) {
            // reached end of this sector
            if (sd_uitmp2 > sd_max_roots) {     // end of the directory.. review results
                j = 3;
            } else {
                // more sectors to come, new sector, go fetch it
                sd_pos++;
                sd_phase = 0x16;
            }
        }

        if (j == 3) {
            if (sd_filenum == 0xffff) {
                // didn't find any valid log files, need to create one
                sd_phase = 0x80;        // do a dummy read and continue
                sd_match = 0x20;        // go create a file
            } else {
                // did find a log file, need to check if it is used or not
                sd_thisfile_start = ((sd_thisfile_clust - 2) * (unsigned long) sd_sect2clust);  // without the cast the top word from EMUL is discarded
                sd_thisfile_start += sd_file_start;
                sd_phase = 0x80;        // do a dummy read and continue
                sd_match = 0x1a;        // go read start of file
            }
        }

    } else if ((sd_phase == 0x1a) && (sd_int_phase == 0)) {     // also check not in an ISR
        // read the start of a file
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_thisfile_start;
        } else {
            sd_int_sector = sd_thisfile_start << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x1b) && (sd_int_phase == 0)) {
        if (sd_int_error) {
            // were not able to read that file - invalid cluster no.? Try to create instead.
            sd_phase = 0x20;
        } else {
        // first sector of file now at SDSECT1
            if ((*((unsigned long *) SDSECT1) == 0x00000000)
                && (*((unsigned long *) (SDSECT1 + 4)) == 0x00000000)) {
                // we got a pre-made empty file - hurrah!
                sd_maxlogblocks = sd_filesize_bytes / LOG_SIZE;
                outpc.sd_filenum = (sd_filenum & 0x000f) + (((sd_filenum & 0x00f0) >> 4) * 10) + (((sd_filenum & 0x0f00) >> 8) * 100) + (((sd_filenum & 0xf000) >> 12) * 1000); // convert BCD to decimal
                if (ram4.log_style & 0x80) {
                    sd_phase = 0x2b;    /* go wait for button or trigger*/
                } else {
                    sd_phase = 0x30;    /* in insertion mode, so write over this existing file, but change date/time was 0x40 */
//                    outpc.sd_status |= 0x04;        // ready
                }
            } else {
                // file no good, must create one
                sd_phase = 0x20;
            }
        }

    } else if (sd_phase == 0x20) {      // create new file
        unsigned long ult1;
        flagbyte9 |= FLAGBYTE9_GETRTC; // flag we want to update the RTC
        // Determine how many clusters we want
        sd_maxlogblocks = ram4.log_length * 468750;     // minutes -> 0.128ms
        if ((ram4.log_style & LOG_STYLE_BLOCKMASK) == LOG_STYLE_BLOCKSTREAM) {  // 64 byte + stream
            sd_maxlogblocks = sd_maxlogblocks / 192;    // interval is ~192 timer ticks in stream mode
        } else {
            if (ram4.log_int < MIN_LOG_INT) {
                sd_lmms_int = MIN_LOG_INT;
            } else {
                sd_lmms_int = ram4.log_int;
            }

            sd_maxlogblocks = sd_maxlogblocks / sd_lmms_int;   // max number of sample blocks
        }

        sd_filesize_bytes = sd_maxlogblocks * LOG_SIZE; // number of bytes total

        if ((ram4.log_style & LOG_STYLE_BLOCKMASK) == LOG_STYLE_BLOCKSTREAM) {  // 64 byte + stream
            sd_filesize_bytes = sd_filesize_bytes << 2; // double the size as a quarter of each sector is stream data
        }

        sd_file_numclust = sd_filesize_bytes / sd_sectsize;     // no. sectors
        sd_file_numclust = sd_file_numclust / sd_sect2clust;    // no. clusters in this file

        // likely to have suffered rounding, so work backwards and check.
        ult1 =
            sd_file_numclust * (unsigned long) (sd_sect2clust *
                                                sd_sectsize);

        if (ult1 < (sd_maxlogblocks * LOG_SIZE)) {
            sd_file_numclust++; // round up so it fits
            // Q: is it better to use exact size user requested or roundup to whole cluster?
        }

        sd_filesize_bytes = sd_file_numclust * (unsigned long) (sd_sect2clust * sd_sectsize);   // make certain it matches
        sd_file_numclust++;     // bodge

        // now want to scan the FAT looking for free consecutive clusters
        sd_uitmp2 = 0;          // temp counts up to sd_fat_size
        sd_pos = sd_fat_start;
        sd_thisfile_clust = 0;
        sd_clust_cnt = 2;       // cluster we'll be checking first
        sd_ultmp = 0;           // used to count no. free clusters found 
        sd_phase = 0x80;        // do dummy read
        sd_match = 0x21;        // next phase

    } else if (sd_phase == 0x21) {
        // read some of the FAT
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x22) && (sd_int_phase == 0)) {
        unsigned char j = 0;
        unsigned int x;
        if (sd_int_error) {
            outpc.sd_error = 12;
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        if (sd_clust_cnt == 2) {
            if (outpc.sd_status & 0x40) {
                x = 8;          // ignore very first two FAT long entries
            } else {
                x = 4;          // ignore very first two FAT word entries
            }
        } else {
            x = 0;
        }
        while (!j) {
            unsigned long y;
            if (outpc.sd_status & 0x40) {
                y = *((unsigned long *)
                      &(*(unsigned char *) (SDSECT1 + x)));
            } else {
                y = *((unsigned int *) &(*(unsigned char *) (SDSECT1 + x)));
            }
            if (y == 0) {
                if (sd_thisfile_clust == 0) {
                    sd_thisfile_clust = sd_clust_cnt;
                }
                sd_ultmp++;
            } else {
                sd_thisfile_clust = 0;  // failed to find enough space set back to zero
                sd_ultmp = 0;   // back to zero
            }

            if (sd_ultmp >= sd_file_numclust) { // enough consecutive free clusters?
                j = 1;
            } else {
                /* Don't do these steps if we've found enough clusters. */
                sd_clust_cnt++;
                if (outpc.sd_status & 0x40) {
                    x += 4;         // next long
                } else {
                    x += 2;         // next word
                }
                if (x >= sd_sectsize) {
                    // time to read next sector of FAT
                    j = 2;
                }
            }
        }

        if (j == 1) {
            // Found enough free space starting at cluster no. sd_thisfile_clust
            // next step is to allocate all of the clusters in a chain and then
            // write the directory entry for the file.
            // NB no frags.
            unsigned long ult1, ult2;

            sd_uitmp = 0;       // FAT counter

            sd_ultmp = 0;
            // calc start sector in FAT the start cluster is stored in
            if (outpc.sd_status & 0x40) {
                ult1 = sd_thisfile_clust << 2;  // cluster count as bytes // FAT32
            } else {
                ult1 = sd_thisfile_clust << 1;  // cluster count as bytes // FAT16
            }
            sd_subblock = ult1 % sd_sectsize;   // byte offset within sector
            ult2 = ult1 / sd_sectsize;  // start sector within FAT
            ult2 += sd_fat_start;

            sd_clust_cnt = 1;   // start at 1 to address off by one error
            sd_ultmp = sd_thisfile_clust;       // used as counter below

            if (ult2 == sd_pos) {
                // already have correct sector loaded
                sd_int_phase = 0;
                sd_int_error = 0;       // to be sure
                sd_phase = 0x80;        // do dummy read
                sd_match = 0x23;        // next phase
            } else {
                // need to do a sector fetch
                sd_pos = ult2;
                sd_phase = 0x80;        // do dummy read
                sd_match = 0x23;        // next phase
            }

        } else if (j == 2) {
            unsigned long sdfs;
            if (outpc.sd_status & 0x40) {
                sdfs = sd_fatsize >> 2;
            } else {
                sdfs = sd_fatsize >> 1;
            }
            if (sd_clust_cnt >= (sdfs * sd_sectsize)) {
                // reached end of FAT without finding enough contiguous free space
                outpc.sd_error = 31;
                sd_phase = 0xfe; // try it again though, in case of card quirk
            } else {
                // at end of this sector, read in another one
                sd_pos++;
                sd_phase = 0x80;        // do dummy read
                sd_match = 0x21;        // next phase
            }
        }

    } else if (sd_phase == 0x23) {
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x24) && (sd_int_phase == 0)) {
        unsigned char j = 0;
        // now we have the starting sector of FAT we want to write loaded at SDSECT1
        // sd_subblock contains the starting offset to write at
        // sd_ultmp is counter of clusters allocated

        if (sd_int_error) {
            outpc.sd_error = 13;
            sd_phase = 0xfe;
            goto SD_end_here;
        }


        if (outpc.sd_status & 0x40) {
            // FAT32
            while (!j) {
                unsigned char ct;

                sd_ultmp++;     // next cluster no.

                //wrong endiannism
                ct = sd_ultmp & 0xff;   // low byte
                *(unsigned char *) (SDSECT1 + sd_subblock) = ct;
                ct = (sd_ultmp & 0xff00) >> 8;  // high byte
                *(unsigned char *) (SDSECT1 + 1 + sd_subblock) = ct;
                ct = (sd_ultmp & 0xff0000) >> 16;       // high byte
                *(unsigned char *) (SDSECT1 + 2 + sd_subblock) = ct;
                ct = (sd_ultmp & 0xff000000) >> 16;     // high byte
                *(unsigned char *) (SDSECT1 + 3 + sd_subblock) = ct;


                sd_subblock += 4;
                sd_clust_cnt++;
                if (sd_clust_cnt >= sd_file_numclust) {
                    // go back and put 0xffffff0f
                    *(unsigned long *) &(*(unsigned char *) (SDSECT1 - 4 + sd_subblock)) = 0xffffff0f;       // end of file marker
                    j = 1;
                } else if (sd_subblock >= sd_sectsize) {
                    j = 2;
                }
            }
        } else {
            // FAT16
            while (!j) {
                unsigned char ct;
                sd_ultmp++;     // next cluster no.

                //wrong endiannism
                ct = sd_ultmp & 0xff;   // low byte
                *(unsigned char *) (SDSECT1 + sd_subblock) = ct;
                ct = ((sd_ultmp & 0xff00) >> 8);        // high byte
                *(unsigned char *) (SDSECT1 + 1 + sd_subblock) = ct;
                sd_subblock += 2;
                sd_clust_cnt++;
                if (sd_clust_cnt >= sd_file_numclust) {
                    // go back and put 0xffff
                    *(unsigned int *) &(*(unsigned char *) (SDSECT1 - 2 + sd_subblock)) = 0xffff;    // end of file marker
                    j = 1;
                } else if (sd_subblock >= sd_sectsize) {
                    j = 2;
                }
            }
        }

        // need to actually write the sector back to the flash device and preserve the phase

        if (j == 2) {
            sd_subblock = 0;    // start at the beginning of next sector and flag for phase 0x25
        }
        if ((j == 1) || (j == 2)) {
// FIXME need to add voltage protection here
            if (outpc.sd_status & 0x02) {
                sd_int_sector = sd_pos;
            } else {
                sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
            }
            sd_int_cmd = 4;     // write block command
            sd_int_phase = 1;   // first stage
            sd_match = SD_OK;
            sd_match_mask = 0xff;
            sd_int_addr = SDSECT1;       // where to read from
            sd_int_cnt = 0;
            sd_int_error = 0;
            SS_ENABLE;
            (void) SPI1SR;
            SPI1DRL = SD_CMD24 | 0x40;
            SPI1CR1 |= 0x80;    // enable SPI interrupts, on data received
            DISABLE_INTERRUPTS;
            sd_lmms_last = lmms;
            ENABLE_INTERRUPTS;
            sd_phase++;
        }

    } else if (sd_phase == 0x25) {
        unsigned long ul_tmp;
        DISABLE_INTERRUPTS;
        ul_tmp = lmms;
        ENABLE_INTERRUPTS;
        if ((ul_tmp - sd_lmms_last) > 3906) {   // more than 500ms
            outpc.sd_error = 29;        // write failed - bail
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        if (sd_int_phase == 0) {
            if (sd_int_error) {
                outpc.sd_error = 14;
                sd_phase = 0xfe;
                goto SD_end_here;
            }
            if (sd_subblock == 0) {
                // were in j == 2 above. Read another sector of FAT and keep updating chain
                sd_pos++;
                sd_phase = 0x23;
            } else {            // was j==1
                // have done writing our cluster.. file time
                sd_pos = sd_dir_start;  // set up to read from the beginning of directory
                sd_uitmp2 = 0;  // file count

                sd_uitmp++;
                if (sd_uitmp < sd_nofats) {
                    // have now written the first FAT, must also write the second (assuming there are just two?!
                    unsigned long ult1, ult2;

                    sd_ultmp = 0;
                    // calc start sector in FAT the start cluster is stored in
                    if (outpc.sd_status & 0x40) {
                        ult1 = sd_thisfile_clust << 2;  // cluster count as bytes // FAT32
                    } else {
                        ult1 = sd_thisfile_clust << 1;  // cluster count as bytes // FAT16
                    }
                    sd_subblock = ult1 % sd_sectsize;   // byte offset within sector
                    ult2 = ult1 / sd_sectsize;  // start sector within FAT
                    ult2 += sd_fat_start + (sd_fatsize * sd_uitmp);

                    sd_clust_cnt = 0;
                    sd_ultmp = sd_thisfile_clust;       // used as counter below

                    sd_pos = ult2;
                    sd_phase = 0x23;    // sector fetch then write

                } else {
                    sd_pos = sd_dir_start;      // set up to read from the beginning of directory
                    sd_uitmp2 = 0;      // file count
                    sd_subblock = 0;    // file sub count
                    sd_phase = 0x80;    // do dummy read
                    sd_match = 0x26;    // next phase
                }
            }
        }
    } else if ((sd_phase == 0x26) && (sd_int_phase == 0)) {

        if ((ram4.opt142 & 0x03) && (outpc.seconds < 3) 
            && ((datax1.rtc_year < 2010) || (datax1.rtc_year > 2200) || (datax1.rtc_sec >= 60)) ) {
            /* Realtime clock enabled but not yet valid, wait a bit. */
            /* Unless we wait for RTC, can sometimes get default dates on files as they were created too early */
            /* This could possibly cause the FAT to be allocated but directory not written if user powers off while we wait for RTC */
            goto SD_end_here;
        }

        // read the root dir
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x27) && (sd_int_phase == 0)) {
        unsigned char file2sect, i, j;
        unsigned int ad;

        if (sd_int_error) {
            outpc.sd_error = 15;
            sd_phase = 0xfe;
            goto SD_end_here;
        }

        file2sect = sd_sectsize / 32;   // directory entry is 32 bytes
        // scan directory for empty slot
        i = 0;
        j = 0;
        while (!j) {
            ad = SDSECT1 + (i * 32);
            if ((*(unsigned char *) ad == 0)
                || (*(unsigned char *) ad == 0xe5)) {
                // found an empty slot to use
                j = 3;
            }
            i++;
            sd_uitmp2++;
            if (i > file2sect) {
                j = 2;
            }
        }

        if (j == 2) {
            // reached end of this sector
            if (sd_uitmp2 > sd_max_roots) {     // end of the directory.. 
                /* This isn't wholly true. On small cards formatted with FAT32
                   there can be a single sector allocated to the directory initially
                   MS3 code doesn't presently support more than one sector in this
                    case as it gets tricky in a hurry, so instead erase oldest file on the card.
                 */
                j = 4;

                /* If we've been round once already then we've failed to erase or some other error
                    error out instead of getting stuck in a loop. */
                if ((sd_filenum_low != 0xffff) && (!(flagbyte10 & FLAGBYTE10_SDERASEAUTO))) {
                    unsigned char a, b;

                    /* attempt to erase the oldest file */
                    flagbyte10 |= FLAGBYTE10_SDERASEAUTO;
                    /* numeric portion of filename to erase (in ASCII) */
                    a = sd_filenum_low >> 8;
                    a = a >> 4;         // top nibble
                    if (a > 10) {
                        b = a + 55;
                    } else {
                        b = a + 48;
                    }
                    *(unsigned char*)SDSECT2 = b;
                    a = sd_filenum_low >> 8;
                    a = a & 0x0f;       // btm nibble
                    if (a > 10) {
                        b = a + 55;
                    } else {
                        b = a + 48;
                    }
                    *(unsigned char*)(SDSECT2 + 1) = b;
                    a = (sd_filenum_low & 0xff) >> 4;       // top nibble
                    if (a > 9) {
                        b = a + 55;
                    } else {
                        b = a + 48;
                    }
                    *(unsigned char*)(SDSECT2 + 2) = b;
                    a = sd_filenum_low & 0x0f;      // btm nibble
                    if (a > 9) {
                        b = a + 55;
                    } else {
                        b = a + 48;
                    }
                    *(unsigned char*)(SDSECT2 + 3) = b;
                    sd_pos = sd_dir_start; // start of dir
                    sd_uitmp2 = 0; // first sector of dir
                    sd_phase = 0x88;
                } else {
                    outpc.sd_error = 30;
                    sd_phase = 0xfe;        // failed to store file somehow, can't retry
                }
            } else {
                // more sectors to come, new sector, go fetch it
                sd_pos++;
                sd_phase = 0x80;        // do dummy read
                sd_match = 0x26;        // next phase
                flagbyte10 &= ~FLAGBYTE10_SDERASEAUTO; // Passed this time
            }
        }

        if (j == 3) {
            /* second argument determines whether hidden attribute is written */
            sd_saved_dir = sd_pos;
            sd_saved_offset = ad;
            /* increment sd_filenum */
            if (sd_filenum == 0xffff) {
                sd_filenum = 0;
            } else {
                // use next filenumber in BCD encoding
                __asm__ __volatile__("ldaa %0+1\n"
                                     "adda #1\n"
                                     "daa\n"
                                     "staa %0+1\n"
                                     "ldaa %0\n"
                                     "adca #0\n"
                                     "daa\n"
                                     "staa %0\n"
                                     :"=m"(sd_filenum)
                                     :
                                     :"a");
            }
            write_dirent(ad, (!(((ram4.log_style & 0xc0) == 0x40) && (!(flagbyte6 & FLAGBYTE6_SD_MANUAL)))) );
            sd_phase++;
        }

    } else if ((sd_phase == 0x28) && (sd_int_phase == 0)) {
        // Finally made it. Should have now created a new cluster chain to the FAT
        // and written a directory entry
        if (sd_int_error) {
            outpc.sd_error = 16;
            sd_phase = 0xfe;
            goto SD_end_here;
        }

        sd_thisfile_start = ((sd_thisfile_clust - 2) * (unsigned long) sd_sect2clust);  // without the cast the top word from EMUL is discarded
        sd_thisfile_start += sd_file_start;

        // If using insertion mode just carry on. With button mode need to nullify start of file.
        sd_phase++;

    } else if (sd_phase == 0x29) {
        outpc.sd_filenum = (sd_filenum & 0x000f) + (((sd_filenum & 0x00f0) >> 4) * 10) + (((sd_filenum & 0x0f00) >> 8) * 100) + (((sd_filenum & 0xf000) >> 12) * 1000); // convert BCD to decimal
        if (((ram4.log_style & 0xc0) == 0x40) && (!(flagbyte6 & FLAGBYTE6_SD_MANUAL))) {        // insertion mode, but not manual
            // Can drop a log
            flagbyte15 |= FLAGBYTE15_SDLOGRUN; // continuous
            sd_phase = 0x40;    // go for it now
        } else {
            outpc.sd_status |= 0x04;        // ready
            sd_ledstat = 0x01;  // solid LED
            sd_phase++;         // wait for button
        }

    } else if ((sd_phase == 0x2a) && (ATD0DR4 > MIN_VOLTS)) {
        unsigned int ad;
        for (ad = SDSECT1; ad < SDSECT2; ad++) {  // clear buffer
            *((unsigned int *) ad) = 0;
        }
        // write this empty sector to the start of the file

        // setup for a interrupt driven block write
        sd_int_addr = SDSECT1;
        sd_int_cnt = 0;         // counts up to blocksize
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_thisfile_start;
        } else {
            sd_int_sector = sd_thisfile_start << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 4;
        sd_int_phase = 1;       // first stage
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        (void) SPI1DRL;         // ensure nothing pending
        SPI1DRL = SD_CMD24 | 0x40;      // send the command (first byte in here)
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
//        if (flagbyte6 & FLAGBYTE6_SD_MANUAL) {
//            sd_phase = 0x2c;
//        }
        sd_phase++;

    } else if ((sd_phase == 0x2b) && (sd_int_phase == 0)) {
        if ((!(flagbyte6 & FLAGBYTE6_SD_MANUAL)) || (flagbyte15 & FLAGBYTE15_SDLOGRUN)) { /* wait here in manual mode and not logging */
            sd_phase++;
        } else {
            sd_ledstat = 0x01;      // solid LED
            outpc.sd_status |= 0x04;        // ready
        }

    } else if (sd_phase == 0x2c) {
        // get here in button mode or under TS control

        if (flagbyte15 & FLAGBYTE15_SDLOGRUN) {
            sd_phase = 0x30; // keep logging
        } else {
            sd_ledstat = 0x01;      // solid LED
            outpc.sd_status |= 0x04;        // ready
            if ((ram4.log_style & 0xc0) == 0xc0) {
                // trigger mode
                if ((!(flagbyte6 & FLAGBYTE6_SD_MANUAL)) &&
                    (  (((ram5.log_style4 & 0x03) == 1) && (outpc.tps > ram5.log_trig_tps))
                    || (((ram5.log_style4 & 0x03) == 0) && (outpc.rpm > ram5.log_trig_rpm))
                    || (((ram5.log_style4 & 0x03) == 2) && (outpc.map > ram5.log_trig_map)) )
                    ) {
                    flagbyte15 |= FLAGBYTE15_SDLOGRUN; // continuous logging
                    sd_phase = 0x30;    // go for it now, rewrite dirent first
                }                    
            } else {
                // check for the buttons
                if (pin_sdbut && ((*port_sdbut & pin_sdbut) == pin_match_sdbut)) {
                    flagbyte6 |= FLAGBYTE6_SD_DEBOUNCE;
                    DISABLE_INTERRUPTS;
                    sd_lmms_last2 = lmms;
                    ENABLE_INTERRUPTS;
                    sd_phase++;
                } else {
                    flagbyte6 &= ~FLAGBYTE6_SD_DEBOUNCE;
                }
            }
        }

    } else if (sd_phase == 0x2d) {
        unsigned long ul_tmp;
        DISABLE_INTERRUPTS;
        ul_tmp = lmms;
        ENABLE_INTERRUPTS;
        if ((ul_tmp - sd_lmms_last2) > 2343) {  //300ms
//            outpc.sd_status |= 0x04;    // ready
            flagbyte15 |= FLAGBYTE15_SDLOGRUN; // continuous logging
            sd_phase = 0x30;    // go for it now, rewrite dirent first
        } else {
            // check for the buttons
            // if button off and didn't make the 500ms time, then go back to waiting
            if ((*port_sdbut & pin_sdbut) != pin_match_sdbut) {
                flagbyte6 &= ~FLAGBYTE6_SD_DEBOUNCE;
                sd_phase = 0x2c;
            }
        }

//    } else if (sd_phase == 0x2e) {
        // under control of tuning software, waiting for go command
        // now go to 0x2c instead

    /* these are used to rewrite a directory entry with un-hidden + date/time */
    } else if ((sd_phase == 0x30) && (sd_int_phase == 0)) {
        if (sd_thisfile_clust == 0xffffffff) {
            /* this should not happen, but must re-start if it does to prevent trashing data on card */
            sd_phase = 1;
            goto SD_end_here;
        }
        if (sd_filenum == 0xffff) {
            sd_pos = sd_dir_start;  // set up to read from the beginning of directory
            sd_phase = 0x16;
            goto SD_end_here;
        }

/* debug */
        outpc.sd_filenum = (sd_filenum & 0x000f) + (((sd_filenum & 0x00f0) >> 4) * 10) + (((sd_filenum & 0x0f00) >> 8) * 100) + (((sd_filenum & 0xf000) >> 12) * 1000); // convert BCD to decimal
/* end debug */

        // read the relevant director sector
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_saved_dir;
        } else {
            sd_int_sector = sd_saved_dir << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x31) && (sd_int_phase == 0)) {
        unsigned long tmp_sdpos;
        if (sd_int_error) {
            outpc.sd_error = 10;
            sd_phase = 0xfe;
            goto SD_end_here;
        }

        /* This should be non-zero, but check */
        if (sd_saved_dir) {
            tmp_sdpos = sd_pos;
            sd_pos = sd_saved_dir; // rewrite directory sector
            write_dirent(sd_saved_offset, 0);
            sd_pos = tmp_sdpos; // restore it
        }
        sd_phase++;

    } else if ((sd_phase == 0x32) && (sd_int_phase == 0)) {
        if (sd_int_error) {
            outpc.sd_error = 10;
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        sd_phase = 0x40;

    /* --------- */

    } else if (sd_phase == 0x40) {
        unsigned int x;

        if (((ram4.log_style & LOG_STYLE_BLOCKMASK) != 0)
            && ((ram4.log_style & LOG_STYLE_BLOCKMASK) != LOG_STYLE_BLOCKSTREAM)) {
            sd_phase = 0;
            // non-supported size at the moment
            goto SD_end_here;
        }

        sd_ledstat = 0x02;      // fast even flashing
        outpc.sd_status |= 0x08;        // we are logging
        outpc.sd_status &= ~0x04;        // therefore !ready


        outpc.sd_filenum = (sd_filenum & 0x000f) + (((sd_filenum & 0x00f0) >> 4) * 10) + (((sd_filenum & 0x0f00) >> 8) * 100) + (((sd_filenum & 0xf000) >> 12) * 1000); // convert BCD to decimal

        sd_seq++;

        // block 0

        *(unsigned long *) sd_log_addr = 0L;    // block 0

        for (x = 0; x < 20; x++) {      // fill in data format string
            *((unsigned char *) sd_log_addr + 0x04 + x) =
                *((unsigned char *) &RevNum + x);
        }

        if ((ram4.log_style & LOG_STYLE_GPS) 
            || ((ram4.log_style & LOG_STYLE_BLOCKSIZEMASK) == LOG_STYLE_BLOCK128)
            ) {
            /* Interleaved GPS data or 128 byte blocks require protocol 2 */
            sd_protocol = 2;
        } else {
            sd_protocol = 1;
        }

        *(unsigned char *) (sd_log_addr + 0x18) = ram4.log_style & LOG_STYLE_BLOCKMASK;        // log block size code
        *(unsigned char *) (sd_log_addr + 0x19) = sd_protocol;    // format revision
        if ((datax1.rtc_year > 2010) && (datax1.rtc_sec < 60)) {
            // looks like a real time/data
            unsigned long tmp_packed;
            unsigned char tmp_sec, tmp_min, tmp_hour, tmp_date, tmp_month;
            unsigned int tmp_year;
            // ensure cohenerncy
            DISABLE_INTERRUPTS;
            tmp_sec = datax1.rtc_sec;
            tmp_min = datax1.rtc_min;
            tmp_hour = datax1.rtc_hour;
            tmp_date = datax1.rtc_date;
            tmp_month = datax1.rtc_month;
            tmp_year = datax1.rtc_year;
            ENABLE_INTERRUPTS;

            // bit pack the date - same as FAT
            tmp_packed = (tmp_year - 1980) & 0x7f;
            tmp_packed <<= 4;
            tmp_packed |= tmp_month & 0x0f;
            tmp_packed <<= 5;
            tmp_packed |= tmp_date & 0x1f;
            tmp_packed <<= 5;
            tmp_packed |= tmp_hour & 0x1f;
            tmp_packed <<= 6;
            tmp_packed |= tmp_min & 0x03f;
            tmp_packed <<= 5;
            tmp_packed |= (tmp_sec >> 1) & 0x01f;

            *(unsigned long *) &(*(unsigned char *) (sd_log_addr + 0x1a)) = tmp_packed;     // real date/time
        } else {
            *(unsigned long *) &(*(unsigned char *) (sd_log_addr + 0x1a)) = 0L;     // null date/time
        }
        for (x = 0; x < 17; x++) {
            unsigned char sz;
            unsigned int off;
            sz = ram4.log_size[(int) x];
            if (sz == 0) {
                off = 0xffff;
            } else {
                off = ram4.log_offset[(int) x];
            }
            *((unsigned int *)
              &(*(unsigned char *) (sd_log_addr + 0x1e + (x << 1)))) = off;
        }

        // block 1
        *((unsigned long *) &(*(unsigned char *) (sd_log_addr + 0x40))) = 1L;   // block 1
        for (x = 17; x < 47; x++) {
            unsigned char sz;
            unsigned int off;
            sz = ram4.log_size[(int) x];
            if (sz == 0) {
                off = 0xffff;
            } else {
                off = ram4.log_offset[(int) x];
            }
            *((unsigned int *) &(*(unsigned char *) (sd_log_addr + 0x22 + (x << 1)))) = off;    //(0x1044 + (x-18)  
        }
        // create magic number
        // ideally we ought to read first sector from file to ensure we don't by chance pick the same number
        sd_magic = (unsigned char) TCNT;
        if ((sd_magic == 0) || (sd_magic == 0xff)) {
            sd_magic = 0x01; // less random but 0x00 and 0xff might occur in blank space on card
        }
        *(unsigned char *) (sd_log_addr + 0x7f) = sd_magic;

        sd_pos = sd_thisfile_start;

        sd_blsect = sd_sectsize / LOG_SIZE;
        sd_subblock = 2;        // next block to write to
        sd_block = 2;

        if (ram4.log_int < MIN_LOG_INT) {
            sd_lmms_int = MIN_LOG_INT;
        } else {
            sd_lmms_int = ram4.log_int;
        }
        DISABLE_INTERRUPTS;
        sd_lmms_last = lmms;
        ENABLE_INTERRUPTS;
        sd_stream_ad = SDSECT1 + 0x80;  // more space for stream
        sd_phase++;

        // main logger activity moved to the top for speedup


    } else if (sd_phase == 0x50) {      // read directory
        sd_ledstat = (sd_ledstat & 0xf8) | 4; // fast flashing
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;      // read block command
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x51) && (sd_int_phase == 0)) {
        unsigned int x, ad;
        unsigned long tmp_long;
        *(unsigned int *) SDSECT2 = sd_uitmp;    // sequence number
        sd_uitmp++;
        sd_pos++;
        ad = SDSECT1;
        for (x = 0; x < 16; x++) {      // FIXME HARDCODING no. directory entries per sector
            //bytes 0x00-0x0a as per FAT
            if ((((*(unsigned char *) (ad + 0x0b)) & 0x5e) == 0)   // valid file attributes
                && ((*(unsigned char *) (ad + 0x00)) != 0xe5)
                && ((*(unsigned char*)(ad + 0x00)) == 'L')
                && ((*(unsigned char*)(ad + 0x01)) == 'O')
                && ((*(unsigned char*)(ad + 0x02)) == 'G')
                && ((*(unsigned char*)(ad + 0x07)) == ' ')
                && ((*(unsigned char*)(ad + 0x08)) == 'M')
                && ((*(unsigned char*)(ad + 0x09)) == 'S')
                && ((*(unsigned char*)(ad + 0x0a)) == '3')
                ) {   // is a log file, not hidden or deleted file
                *((unsigned char *) (ad + 0x0b)) = 1;
            } else {
                *((unsigned char *) (ad + 0x0b)) = 0;   // something else
            }

            if (outpc.sd_status & 0x40) {
                tmp_long = (unsigned long) (((unsigned int) (*(unsigned char *) (ad + 0x15)) << 8)
                            | (*(unsigned char *) (ad + 0x14))) << 16;
            } else {
                tmp_long = 0;
            }
            tmp_long |= ((unsigned int) (*(unsigned char *) (ad + 0x1b)) << 8)
                        | (*(unsigned char *) (ad + 0x1a));

            tmp_long = ((tmp_long - 2) * (unsigned long) sd_sect2clust);        // without the cast the top word from EMUL is discarded
            tmp_long += sd_file_start;

            *((unsigned char *) (ad + 0x0c)) = 0;
            // bytes 0x0d-0x11 as per FAT - date/time of creation
            // bytes 0x12-0x15 now start sector number
            *((unsigned long *) &*((unsigned char *) (ad + 0x12))) = tmp_long;  // (big endian)  
            // bytes 0x16-0x1b zero, were other access times
            *((unsigned char *) (ad + 0x16)) = 0;
            *((unsigned char *) (ad + 0x17)) = 0;
            *((unsigned char *) (ad + 0x18)) = 0;
            *((unsigned char *) (ad + 0x19)) = 0;
            *((unsigned char *) (ad + 0x1a)) = 0;
            *((unsigned char *) (ad + 0x1b)) = 0;
            // bytes 0x1c-0x1f as per FAT - filesize in bytes (little endian)
            ad += 0x20;
        }
        sd_ledstat = 1;
        sd_phase = 0x2c;        // wait again

    } else if (sd_phase == 0x54) {      // read sector
        sd_ledstat = (sd_ledstat & 0xf8) | 4; // fast flashing
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;      // read block command
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x55) && (sd_int_phase == 0)) {
        *(unsigned long *) SDSECT2 = sd_pos;     // sequence number
        sd_pos++;
        sd_ledstat = 1;
        sd_phase = 0x2c;        // wait again

    } else if ((sd_phase == 0x58) && (ATD0DR4 > MIN_VOLTS)) {   // write sector and > 6.5V
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 4;         // write block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1;   // where to read from
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD24 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x59) && (sd_int_phase == 0)) {
        sd_ledstat = 1;
        sd_phase = 0x2c;        // wait again

// ******** SD/SDHC init and ver 2.0 or non-ver 2.0 detection

    } else if (sd_phase == 0x60) {      // SD/SDHC initialisation

        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;       // first stage
        sd_match = 1;           // R1 response code for SDHC
        sd_match_mask = 0x01;
        sd_int_error = 0;
        sd_int_sector = 0x000001aa;      // ~3.3V and 'aa' pattern.
        sd_rx = 0x87;           // Checksum matters here
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD8 | 0x40;       // Required for SD spec 2.0
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x61) && (sd_int_phase == 0)) {
//        if ((sd_int_error == 8) && (sd_rx == 5)) {
        if (sd_rx == 5) {
            // Card says this is invalid command
            outpc.sd_status &= ~0x20;   //non ver 2.0 card
            sd_phase = 0x65;
        } else if (sd_int_error) {
            outpc.sd_error = 20;
            sd_phase = 0xfe;
            goto SD_end_here;
        } else {
            outpc.sd_status |= 0x20;    //ver 2.0 card

            SPI1CR1 &= ~0x80;   // turn off ints so foreground code can work
            SPI1DRL = 0xff;     // dummy write
            sd_int_cnt = 0;
            sd_phase++;
        }

    } else if (sd_phase == 0x62) {
        if (SPI1SR & 0x80) {
            *((unsigned char *) (SDSECT1 + sd_int_cnt)) = SPI1DRL;
            sd_int_cnt++;
            SPI1DRL = 0xff;     // dummy write
            if (sd_int_cnt == 4) {
                SS_DISABLE;
                SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work
                (void) SPI1SR;
                SPI1DRL = 0xFF; // send to receive
                sd_phase++;
            }
        }

    } else if (sd_phase == 0x63) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            sd_phase++;
        }

    } else if (sd_phase == 0x64) {
        // check the VCA
        if (*((unsigned char *) (SDSECT1 + 3)) != 0xaa) {    // check pattern doesn't match
            // retry command
            sd_nofats++;
            if (sd_nofats > 10) {
                outpc.sd_error = 21;
                sd_phase = 0xfe;
                goto SD_end_here;
            } else {
                sd_phase = 0x60;    // retry
            }
        } else if (*((unsigned char *) (SDSECT1 + 2)) != 1) {        // voltage rejected
            outpc.sd_error = 22;
            sd_phase = 0xfe;
            goto SD_end_here;
        } else {
            sd_phase++;
        }

    } else if (sd_phase == 0x65) {
        // SD and SDHC continue here
        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_error = 0;
        sd_int_sector = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD55 | 0x40;      // ACMD follows
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x66) && (sd_int_phase == 0)) {
        SS_DISABLE;
        SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work
        (void) SPI1SR;
        SPI1DRL = 0xFF;         // send to receive
        sd_phase++;

    } else if (sd_phase == 0x67) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            sd_phase++;
        }

    } else if (sd_phase == 0x68) {
        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_error = 0;
        if (outpc.sd_status & 0x20) {
            sd_int_sector = 0x40000000;  // Host has High Capacity bit
        } else {
            sd_int_sector = 0x0;
        }
        (void) SPI1SR;
        SS_ENABLE;
        SPI1DRL = SD_CMD41 | 0x40;      // ACMD41
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x69) && (sd_int_phase == 0)) {
        SS_DISABLE;
        SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work
        (void) SPI1SR;
        SPI1DRL = 0xFF;         // send to receive

        if ((sd_rx == 5) && ((outpc.sd_status & 0x20) == 0)) {    // illegal command and not SDHC
            sd_phase = 0x79; // try MMC init
            goto SD_end_here;
        }

        if (sd_int_error) {
//            if (sd_rx == 1) {   // busy
//                sd_phase = 0x63;
//            } else if (sd_rx == 5) {    // illegal command
//                sd_phase = 0x63;
//            } else {
// Changed for 0.26 to handle with a pause and a loop counter to prevent endless looping
            sd_nofats++;
            if (sd_nofats > 20) {
                outpc.sd_error = 23;
                sd_phase = 0xfe;
                goto SD_end_here;
            } else {
                sd_phase = 0x84;    // pause, retry
                sd_match = 0x63;    // have another go
            }
            
        } else {
            sd_phase++;
        }

    } else if (sd_phase == 0x6a) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            sd_phase++;
        }

    } else if (sd_phase == 0x6b) {      // OCR reg
        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;       // R1 response code for SDHC
        sd_match_mask = 0xff;
        sd_int_error = 0;
        sd_int_sector = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD58 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;


    } else if ((sd_phase == 0x6c) && (sd_int_phase == 0)) {
        SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work
        (void) SPI1SR;
        SPI1DRL = 0xff;         // dummy write
        if (sd_int_error) {
            sd_nofats++;
            if (sd_nofats > 10) {
                outpc.sd_error = 24;
                sd_phase = 0xfe;
                goto SD_end_here;
            } else {
                sd_phase = 0x84;    // pause, retry
                sd_match = 0x60;    // have another go
            }
        } else {
            sd_int_cnt = 0;
            sd_phase++;
        }

    } else if ((sd_phase == 0x6d) && (SPI1SR & 0x80)) {
        (void) SPI1SR;
        *((unsigned char *) (SDSECT1 + sd_int_cnt)) = SPI1DRL;
        sd_int_cnt++;
        if (sd_int_cnt < 4) {
            SPI1DRL = 0xff;     // dummy write
        } else if (sd_int_cnt == 4) {
            SS_DISABLE;
            SPI1CR1 &= ~0x80;   // turn off ints so foreground code can work
            SPI1DRL = 0xFF;     // send to receive
            sd_phase++;
        }
    } else if (sd_phase == 0x6e) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            sd_phase++;
        }

    } else if (sd_phase == 0x6f) {
        if ((*((unsigned int *) (SDSECT1)) == 0xffff)
            && (*((unsigned int *) (SDSECT1 + 2)) == 0xffff)) {
            // didn't work, BS data
            SS_DISABLE;
            sd_nofats++;
            if (sd_nofats > 10) {
                outpc.sd_error = 25;
                sd_phase = 0xfe;
                goto SD_end_here;
            } else {
                sd_phase = 0x84;    // pause, retry
                sd_match = 0x6b;    // have another go
            }
            goto SD_end_here;
        }

        if ((*((unsigned char *) (SDSECT1)) & 0x80) == 0) {
            // card still busy in init, repeat command
            SS_DISABLE;
            sd_nofats++;
            if (sd_nofats > 10) {
                outpc.sd_error = 26;
                sd_phase = 0xfe;
                goto SD_end_here;
            } else {
                sd_phase = 0x84;    // pause, retry
                sd_match = 0x6b;    // have another go
            }
        } else {
            // card no longer busy
            if (*((unsigned char *) (SDSECT1)) & 0x40) {
                outpc.sd_status |= 0x02;        // SDHC
            } else {
                outpc.sd_status &= ~0x2;        // SD non-HC
            }
            // OK, we've done init, so can get on with reading from the device

            sd_int_phase = 0;
            sd_int_error = 0;
            SS_DISABLE;
            sd_phase = 0x08;    // do set block length
        }

// ******** non ver 2.0 init
    } else if (sd_phase == 0x79) {
        SS_DISABLE;
        //send a clock with it disabled to "clear the air"
        SPI1DRL = 0xff;
        SPI1CR1 &= ~0x80;       // disable SPI interrupts
        sd_phase++;

    } else if ((sd_phase == 0x7a) && (SPI1SR & 0x80)) {
        (void) SPI1DRL;         // the dummy read
        /* IDLE Command */
        sd_int_sector = 0;
        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;       // first stage
        sd_match = SD_IDLE;     // expecting idle result code
        sd_match_mask = 0x01;
        sd_int_error = 0;
        sd_rx = 0x95;           // Checksum matters here
        SS_ENABLE;
        (void) SPI1SR;
        (void) SPI1DRL;         // ensure nothing hanging around
        SPI1DRL = SD_CMD0 | 0x40;       // send the Idle command to reset the card
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x7b) && (sd_int_phase == 0)) {
        // int handler handles all writes and reads. No need to poll.
        SS_DISABLE;
        SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work
        SPI1DRL = 0xFF;         // send to receive
        sd_phase++;

    } else if ((sd_phase == 0x7c) && (SPI1SR & 0x80)) {
        (void) SPI1DRL;         // the dummy read
        sd_subblock = 0;        // retry counter
        sd_phase++;

    } else if (sd_phase == 0x7d) {
        sd_int_cmd = 10;        // send command
        sd_int_phase = 1;       // first stage
        if (outpc.sd_status & 0x20) {
            sd_int_sector = 0x40000000;
        } else {
            sd_int_sector = 0;
        }
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD1 | 0x40;       // Initialise the card
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x7e) && (sd_int_phase == 0)) {
        // int handler handles all writes and reads. No need to poll.
        if (sd_int_error) {
            SS_DISABLE;
            sd_subblock++;
            if (sd_subblock < 100) {
                sd_phase = 0x7b;        // try again FIXME need timeout. Typically takes two attempts.
//                sd_phase = 0x02;        // REDO from start
            } else {
                outpc.sd_error = 3;
                sd_phase = 0xfe;
            }
        } else {
            SS_DISABLE;
            SPI1CR1 &= ~0x80;   // turn off ints so foreground code can work
            SPI1DRL = 0xFF;     // send to receive
            sd_phase++;
        }

    } else if (sd_phase == 0x7f) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            // non ver 2.0 card init done, back to main code
            SS_DISABLE;
            sd_phase = 0x08;
        }
// ******** end non ver 2.0 init


// ******** dummy send/receive then go back to phase stored in sd_match

    } else if (sd_phase == 0x80) {
        SS_DISABLE;
        SPI1CR1 &= ~0x80;       // turn off ints so foreground code can work
        (void) SPI1SR;
        SPI1DRL = 0xFF;         // send to receive
        sd_phase++;

    } else if (sd_phase == 0x81) {
        if (SPI1SR & 0x80) {
            (void) SPI1DRL;     // the dummy read
            sd_phase = sd_match;        // back to the phase
        }

// ******** pause 100ms and return - used for soft errors and retries

    } else if (sd_phase == 0x84) {
        DISABLE_INTERRUPTS;
        sd_lmms_last = lmms;
        ENABLE_INTERRUPTS;
        sd_phase++;

    } else if (sd_phase == 0x85) {
        unsigned long ul_tmp;
        DISABLE_INTERRUPTS;
        ul_tmp = lmms;
        ENABLE_INTERRUPTS;
        if ((ul_tmp - sd_lmms_last) > 781) {    // 100ms
            sd_phase = 0x80; // do a dummy read as well
        }

// ******** delete/erase file

    } else if (sd_phase == 0x88) {
        sd_ledstat = 4;
        outpc.sd_status &= ~0x04; // not ready
        sd_phase = 0x80; // do dummy read
        sd_match = 0x89; // next phase
        /* we'll use sd_ultmp as clust during erase phases */

    } else if (sd_phase == 0x89) {
        // read the dir
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;    // read block command
        sd_int_phase = 1;   // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1; // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;    // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x8a) && (sd_int_phase == 0)) {
        unsigned char file2sect, i, j;
        if (sd_int_error) {
            outpc.sd_error = 10;
            sd_phase = 0xfe;
            goto SD_end_here;
        }
        file2sect = sd_sectsize / 32; // directory entry is 32 bytes
        // scan directory for LOGxxxx.MS3
        i = 0;
        j = 0;

        while (!j) {
            unsigned int ad;
            ad = SDSECT1 + (i * 32);
            if (*(unsigned char*)ad == 0) {
                // end of directory
                j = 3;
            } else if ( (((*(unsigned char*)(ad + 0x0b)) & ~0x22) == 0) && // ignore achive + hidden
                ((*(unsigned char*)(ad + 0x00)) == 'L') &&
                ((*(unsigned char*)(ad + 0x01)) == 'O') &&
                ((*(unsigned char*)(ad + 0x02)) == 'G') &&

                ((*(unsigned char*)(ad + 0x03)) == *(unsigned char*)(SDSECT2 + 0)) &&
                ((*(unsigned char*)(ad + 0x04)) == *(unsigned char*)(SDSECT2 + 1)) &&
                ((*(unsigned char*)(ad + 0x05)) == *(unsigned char*)(SDSECT2 + 2)) &&
                ((*(unsigned char*)(ad + 0x06)) == *(unsigned char*)(SDSECT2 + 3)) &&

                ((*(unsigned char*)(ad + 0x07)) == ' ') &&
                ((*(unsigned char*)(ad + 0x08)) == 'M') &&
                ((*(unsigned char*)(ad + 0x09)) == 'S') &&
                ((*(unsigned char*)(ad + 0x0a)) == '3') ) {

                // found the file, erase it
                *(unsigned char*)(ad + 0x00) = 0xe5;

                // find size + cluster no.
                *(unsigned char*)&sd_filesize_bytes =       *((unsigned char*)(ad + 0x1f));
                *((unsigned char*)&sd_filesize_bytes + 1) = *((unsigned char*)(ad + 0x1e));
                *((unsigned char*)&sd_filesize_bytes + 2) = *((unsigned char*)(ad + 0x1d));
                *((unsigned char*)&sd_filesize_bytes + 3) = *((unsigned char*)(ad + 0x1c));

                if (outpc.sd_status & 0x40) {
                    sd_ultmp = (unsigned long)(((unsigned int)(*(unsigned char*)(ad + 0x15)) << 8) | (*(unsigned char*)(ad + 0x14))) << 16;
                } else {
                    sd_ultmp = 0;
                }
                sd_ultmp |= ((unsigned int)(*(unsigned char*)(ad + 0x1b)) << 8) | (*(unsigned char*)(ad + 0x1a));
                j = 4;
            }

            i++;
            sd_uitmp2++;
            if (i > file2sect) {
                j = 2;
            }
        }

        if (j == 2) {
            // reached end of this sector
            if (sd_uitmp2 > sd_max_roots) { // end of the directory - can't have found it. Fail silently.
                sd_ledstat = 1;
                outpc.sd_status |= 0x04;
                sd_phase = 0x80; // do a dummy read and continue

                if (flagbyte10 & FLAGBYTE10_SDERASEAUTO) {
                    sd_match = 0x98;
                } else {
                    sd_match = 0x2c; // go back to wait for tuning software phase
                }
            } else {
                // more sectors to come, new sector, go fetch it
                sd_pos++;
                sd_phase = 0x89;
            }
        } else if (j == 4) {
            // found the file. Write the modified sector

//            sd_phase = 0x80; // do a dummy read and continue
//            sd_match = 0x8b; // go read start of file
            sd_phase++;
        } else {
            // something else occurred such as end of directory - done
            sd_ledstat = 1;
            outpc.sd_status |= 0x04;
            sd_phase = 0x80; // do a dummy read and continue
            if (flagbyte10 & FLAGBYTE10_SDERASEAUTO) {
                sd_match = 0x98;
            } else {
                sd_match = 0x2c; // go back to wait for tuning software phase
            }
        }

    } else if ((sd_phase == 0x8b) && (ATD0DR4 > MIN_VOLTS)) { // write sector and > 6.5V
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 4;     // write block command
        sd_int_phase = 1;   // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1; // where to read from
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD24 | 0x40;
        SPI1CR1 |= 0x80;    // enable SPI interrupts, on data received
        sd_phase++;
        if (outpc.sd_status & 0x40) { // FAT32
            sd_uitmp2 = sd_ultmp >> 7; // * 4) / 512);
        } else { // FAT16
            sd_uitmp2 = sd_ultmp >> 8; // * 2) / 512);
        }
        sd_pos = sd_fat_start + sd_uitmp2;

    } else if ((sd_phase == 0x8c) && (sd_int_phase == 0)) {
        // now start fetching the FAT to find the FAT chain
        // figure out FAT sector to grab
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;    // read block command
        sd_int_phase = 1;   // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1; // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;
        SPI1CR1 |= 0x80;    // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x8d) && (sd_int_phase == 0)) {
        unsigned int tmp_fat_pos;
        unsigned char k;

        if (outpc.sd_status & 0x40) { // FAT32
            unsigned int ad;
//            tmp_fat_pos = (sd_ultmp * 4) % 512;
            tmp_fat_pos = (sd_ultmp << 2) & 0x1ff;
            ad = SDSECT1 + tmp_fat_pos;
            k = 0;
            while (!k) {
                // grab the next cluster number
                *(unsigned char*)&sd_ultmp = *(unsigned char*)(ad+3);
                *((unsigned char*)&sd_ultmp + 1) = *(unsigned char*)(ad+2);
                *((unsigned char*)&sd_ultmp + 2) = *(unsigned char*)(ad+1);
                *((unsigned char*)&sd_ultmp + 3) = *(unsigned char*)(ad);

                // free up this cluster
                *(unsigned int*)ad = 0;
                *(unsigned int*)(ad+2) = 0;

                if (((sd_ultmp & 0xfffffff8) == 0xfffffff8) || (sd_ultmp == 0)) {
                    sd_ultmp = 0xffffffff; // end marker
                    k = 1;
                } else {

                    ad += 4;
                    if (ad > (SDSECT1 + 0x1ff)) {
                        k = 2;
                    }
                }
            }
        } else { // FAT16
            unsigned int ad;
//            tmp_fat_pos = (sd_ultmp * 2) % 512;
            tmp_fat_pos = (sd_ultmp << 1) & 0x1ff;
            ad = SDSECT1 + tmp_fat_pos;
            k = 0;
            while (!k) {
                // grab the next cluster number
                *(unsigned char*)&sd_ultmp = 0;
                *((unsigned char*)&sd_ultmp + 1) = 0;
                *((unsigned char*)&sd_ultmp + 2) = *(unsigned char*)(ad+1);
                *((unsigned char*)&sd_ultmp + 3) = *(unsigned char*)(ad);

                // free up this cluster
                *(unsigned int*)ad = 0;

                if ((sd_ultmp & 0xfff8) == 0xfff8) {
                    sd_ultmp = 0xffffffff;  // end marker
                    k = 1;
                } else {
                    ad += 2;
                    if (ad > (SDSECT1 + 0x1ff)) {
                        k = 2;
                    }
                }
            }
        }

        sd_uitmp++;
         // write this sector back to device
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 4;     // write block command
        sd_int_phase = 1;   // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT1; // where to read from
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD24 | 0x40;
        SPI1CR1 |= 0x80;    // enable SPI interrupts, on data received
        sd_phase++;

    } else if ((sd_phase == 0x8e) && (sd_int_phase == 0)) {
        if (sd_nofats > 1) {
             // write this to backup FAT (only one though)
            if (outpc.sd_status & 0x02) {
                sd_int_sector = (sd_pos + sd_fatsize);
            } else {
                sd_int_sector = (sd_pos + sd_fatsize) << SD_BLOCK_SHIFT;
            }
            sd_int_cmd = 4;     // write block command
            sd_int_phase = 1;   // first stage
            sd_match = SD_OK;
            sd_match_mask = 0xff;
            sd_int_addr = SDSECT1; // where to read from
            sd_int_cnt = 0;
            sd_int_error = 0;
            SS_ENABLE;
            (void) SPI1SR;
            SPI1DRL = SD_CMD24 | 0x40;
            SPI1CR1 |= 0x80;    // enable SPI interrupts, on data received
        }
        sd_phase++;

    } else if ((sd_phase == 0x8f) && (sd_int_phase == 0)) {
        if (sd_int_error) {
            outpc.sd_error = 32;
            sd_phase = 0xfe;
        } else {
            if (sd_ultmp == 0xffffffff) {
                outpc.sd_status |= 0x04; // ok
                sd_ledstat = 0;
                if (flagbyte10 & FLAGBYTE10_SDERASEAUTO) {
                    sd_phase = 0x98;
                } else {
                    sd_phase = 0x2c;
                }
            } else {
                sd_pos++;
                sd_phase = 0x8c;
            }
        }

// ******** autonomous erase
    } else if (sd_phase == 0x98) {
        /* have erased oldest file (hopefully!) now go and try to create again*/
        sd_filenum = 0xffff;
        sd_filenum_low = 0xffff;
        /* now go back and read the root directory and have another go */
        sd_pos = sd_dir_start;  // set up to read from the beginning of directory
        sd_uitmp2 = 0; // first sector of dir
        sd_phase = 0x80;        // do dummy read
        sd_match = 0x16;        // next phase

// ******** speed test
    } else if (sd_phase == 0xa0) {
        if (sd_pos == 0) {
            *(unsigned char*)(SDSECT1 + 0xc) = 2; // say there was an error
            // bail for now unless sector number supplied
            sd_phase = 0x2c;
        } else if (*(unsigned long*)SDSECT1 == 0) {
            // requested to test zero sectors ?!
            *(unsigned char*)(SDSECT1 + 0xc) = 2; // say there was an error
            sd_phase = 0x2c;
        } else {
            *(unsigned char*)(SDSECT1 + 0xc) = 0; // status = running
            // given a sector number, jump some steps
            sd_phase = 0xa4;
        }

// space here for phases to allocate some test sectors

    } else if (sd_phase == 0xa4) {
        unsigned int ix;
        // fill data buffer
        for (ix = SDSECT2 ; ix < (SDSECT2 + 0x200) ; ix++) {
            *(unsigned char*)ix = (unsigned char)ix;
        }
        *(unsigned long*)(SDSECT1 + 4) = 0; // total
        *(unsigned int*)(SDSECT1 + 8) = 0xffff; // min
        *(unsigned int*)(SDSECT1 + 0xa) = 0; // max
        sd_phase++;

    } else if (sd_phase == 0xa5) {
        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 4;         // write block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_match_mask = 0xff;
        sd_int_addr = SDSECT2;   // where to read from
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD24 | 0x40;
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received
        DISABLE_INTERRUPTS;
        sd_lmms_last = lmms;
        ENABLE_INTERRUPTS;
        sd_phase++;

    } else if ((sd_phase == 0xa6) && (sd_int_phase == 0)) {
        if (sd_int_error) {
            *(unsigned char*)(SDSECT1 + 0xc) = 0x01; // completed
            sd_phase = 0x2c;
        } else {        
        unsigned int tmp_time;
            tmp_time = (unsigned int)lmms - (unsigned int)sd_lmms_last;

            *(unsigned long*)(SDSECT1 + 4) += tmp_time;
            
            if (tmp_time < *(unsigned int*)(SDSECT1 + 0x8)) {
                *(unsigned int*)(SDSECT1 + 8) = tmp_time;
            }

            if (tmp_time > *(unsigned int*)(SDSECT1 + 0xa)) {
                *(unsigned int*)(SDSECT1 + 0xa) = tmp_time;
            }


            *(unsigned long*)SDSECT1 = *(unsigned long*)SDSECT1 - 1;

            if (*(unsigned long*)SDSECT1) {
                sd_pos++;
                sd_phase = 0xa5; // do another
            } else {
                *(unsigned char*)(SDSECT1 + 0xc) = 0x01; // completed
                sd_phase = 0x2c;
            }
        }

// ********
    } else if (sd_phase == 0xfe) {      // error
        outpc.sd_status |= 0x10;        // error status
        outpc.sd_status &= ~0x0c;       // not ready, not logging
        if (sd_retry > 10) {
            sd_ledstat = 0x03;      // error flash code
            sd_phase++;
        } else {
            sd_retry++;
            // try again from the start
            sd_phase = 1;
            DISABLE_INTERRUPTS;
            sd_lmms_last = lmms;
            ENABLE_INTERRUPTS;
            SPI_baudlow();      // set low rate for initial negotiation
            sd_subblock = 0;    // used as init retry counter
            sd_int_error = 0;   // error flag within interrupt
            outpc.sd_error = 0; // external error flag
            sd_int_phase = 0;   // phase
            outpc.sd_status = 1;        // card present
            sd_ledstat = 0x04;
            sd_ledclk = (unsigned int) lmms;    // only use low word
            sd_block = 0;
            sd_log_addr = SDSECT1;
            sd_int_cmd = 0;
            sd_int_phase = 0;
            sd_lmms_last2 = 0;
            sd_seq = 0;
        }
    }

  SD_end_here:;
}

void SPI_baudlow(void)
{
// SPI1 set up as master, CPOL=0, CPHA=0, SS = bit bash
    SPI1CR2 = 0x00;

    if ((ram4.log_style_led & 0xc0) == 0xc0) {
        SPI1BR = 0x34;              // 390kHz - used for init - won't work on older hardware
    } else {
        SPI1BR = 0x16;              // 195kHz (98) - used for init
    }
    SPI1CR1 = 0x50;
    sd_baud_norm = SPI1BR;
    sd_baud_write = SPI1BR;
}

void SPI_baudhigh(void)
{
    /* baud rates in datasheet (..) are for 25MHz bus-clock, but we run at 50MHz */

    unsigned char sp = ram4.log_style_led & 0xc0;

    if ((MONVER >= 0x380) && (MONVER <= 0x3ff)) {
        sp = 0xc0; // Force to full speed
    }

    /* For reference from p 769 of XEP datasheet
        0x64 = 222kHz
        0x54 = 260kHz
        0x44 = 312kHz
        0x34 = 390kHz
        0x05 = 781kHz
        0x62 = 893kHz
        0x52 = 1042kHz
        0x04 = 1563kHz
        0x51 = 2083kHz
        0x41 = 2500kHz
        0x03 = 3125kHz
    */

    if (sp == 0xc0) {
        sd_baud_norm = 0x34; // 390kHz (195) - only works on MS3 card with updated resistor values
    } else if (sp == 0x80) {
        sd_baud_norm = 0x44; // 312kHz (156) - "faster" works on JSM MS3 card, original standard speed
    } else if (sp == 0x40) {
        sd_baud_norm = 0x64; // 222kHz (111) - "slower"
    } else {
        sd_baud_norm = 0x54; // 260kHz (130) - the new "normal"
    }
    sd_baud_write = sd_baud_norm;

#define SPI_STDFAST 0x03; /* 3125 kHz */
#define SPI_STDWR 0x05; /* 781 kHz, compromise speed */
    /* check for hardware update (continuous improvement)
     With updated BOM, tests showed 4166kHz working.
     Set new rate to 3125kHz to allow for production variation. */
    if ((MONVER >= 0x380) && (MONVER <= 0x38f)) {
        if (MONVER >= 0x384) {
            sd_baud_norm = SPI_STDFAST;
            sd_baud_write = SPI_STDWR;
        }
    } else if ((MONVER >= 0x390) && (MONVER <= 0x39f)) {
        if (MONVER >= 0x394) {
            sd_baud_norm = SPI_STDFAST;
            sd_baud_write = SPI_STDWR;
        }
    } else if ((MONVER >= 0x300) && (MONVER <= 0x30f)) {
        if (MONVER >= 0x309) {
            sd_baud_norm = SPI_STDFAST;
            sd_baud_write = SPI_STDWR;
        }
    }

    SPI1BR = sd_baud_norm; /* Use highest speed now */
    /* Write operation varies baud rate to optimise CPU cycles */
}
