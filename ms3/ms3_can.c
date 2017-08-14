/* $Id: ms3_can.c,v 1.78.4.9 2015/06/26 21:13:54 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * CanInit()
    Origin: Al Grippo.
    Minor: XEPisms by James Murray
    Majority: Al Grippo
 * can_sendburn()
    Origin: Al Grippo.
    Majority: Re-write James Murray
    Majority: James Murray
 * can_crc32()
    Origin: Code by James Murray using MS-CAN framework.
    Majority: James Murray
 * can_reqdata()
    Origin: Al Grippo.
    Majority: Re-write James Murray
    Majority: James Murray
 * can_snddata()
    Origin: Al Grippo.
    Majority: Re-write James Murray
    Majority: James Murray
 * can_sndMSG_PROT()
    Origin: James Murray
    Majority: James Murray
 * can_scan_prot()
    Origin: James Murray
    Majority: James Murray
 * can_sndMSG_SPND()
    Origin: James Murray
    Majority: James Murray
 * can_poll()
    Origin: Jean Belanger (in MS2/Extra)
    Moderate: XEPisms by James Murray
    Majority: Jean Belanger / James Murray
 * send_can11bit()
    Origin: "stevevp" used with permission
    Majority: Complete re-write James Murray
 * send_can29bit()
    Origin: send_can11bit()
    Majority: Re-write to 29bit, James Murray
 * can_broadcast()
    Origin: "stevevp" used with permission
    Moderate: James Murray
    Majority: stevevp / James Murray
 * can_bcast_outpc_cont()
    Origin: James Murray
    Majority: James Murray
 * can_bcast_outpc()
    Origin: James Murray
    Majority: James Murray
 * can_rcv_process
    Origin: James Murray
    Majority: James Murray
* conf_iobox()
    Origin: James Murray
    Majority: James Murray
* can_iobox()
    Origin: James Murray
    Majority: James Murray
* io_pwm_outs()
    Origin: James Murray
    Majority: James Murray
* can_do_tx()
    Origin: James Murray
    Majority: James Murray
* can_build_msg()
    Origin: James Murray
    Majority: James Murray
* can_build_msg_req()
    Origin: James Murray
    Majority: James Murray
* chk_crc
    Origin: James Murray
    Majority: James Murray
* can_build_outmsg
    Origin: Al Grippo
    Majority: Al Grippo / Jean Belanger / James Murray
* can_dashbcast()
    Origin: James Murray
    Majority: James Murray

 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
 */
#include "ms3.h"

#define r27 ram_window.pg27

#define CAN_INC_TXRING \
    can_tx_num++; /* Increment count */\
    can_tx_in++; /* Next ring buffer slot */\
    if (can_tx_in >= NO_CANTXMSG) {\
        can_tx_in = 0;\
    }

void CanInit(void)
{
    unsigned int ix;
    /* Set up CAN communications */
    /* Enable CAN0, set Init mode so can change registers */
    CAN0CTL1 |= 0x80;
    CAN0CTL0 |= 0x01;

    can_status = 0;
    flagbyte3 &= ~(flagbyte3_can_reset | flagbyte3_sndcandat | flagbyte3_getcandat);

    while (!(CAN0CTL1 & 0x01)); // make sure in init mode (possible lockup?)

    /* Set Can enable, use Oscclk (8 MHz),clear rest */
//    CAN0CTL1 = 0x80;
    /* Set timing for .5Mbits/ sec */
// Use same timing as MS2
//    CAN0BTR0 = 0xC0;            /* SJW=4,BR Prescaler= 1(8MHz CAN0 clk) */
/* Next line has invalid bus timing */
//    CAN0BTR1 = 0x1C;  /* Set time quanta: tseg2 =2,tseg1=13 
//                        (16 Tq total including sync seg (=1)) */
//    CAN0BTR1 = 0x3a; /* Time segment1 = 11, Time segment2 = 4. Valid with SJW=4 */
CAN0CTL1 = 0xc0; /* BUSCLK (50MHz) */
CAN0BTR0 = 0xc4; /* SJW=4, prescaler = 5 */
CAN0BTR1 = 0x5c; /* Segment2 = 6, Segment1 = 13 */

    CAN0IDAC = 0x00;            /* 2 32-bit acceptance filters */
    /* CAN message format:
     * Reg Bits: 7 <-------------------- 0
     * IDR0:    |---var_off(11 bits)----|  (Header bits 28 <-- 21)
     * IDR1:    |cont'd 1 1 --msg type--|  (Header bits 20 <-- 15)
     * IDR2:    |---From ID--|--To ID---|  (Header bits 14 <--  7)
     * IDR3:    |--var_blk-|--spare--rtr|  (Header bits  6 <-- 0,rtr)
     */
    /* Set identifier acceptance and mask registers to accept 
       messages only for can_id or device #15 (=> all devices) */
    /* 1st 32-bit filter bank-to mask filtering, set bit=1 */
    CAN0IDMR0 = 0xFF;           // anything ok in IDR0(var offset)
    CAN0IDAR1 = 0x18;           // 0,0,0,SRR=IDE=1
    CAN0IDMR1 = 0xE7;           // anything ok for var_off cont'd, msgtype
    CAN0IDAR2 = CANid;  // rcv msg must be to can_id, but
    CAN0IDMR2 = 0xF0;           // can be from any other device
    CAN0IDMR3 = 0xFF;           // any var_blk, spare, rtr
    /* 2nd 32-bit filter bank */
//    if (ram5.can_rcv_opt & CAN_RCV_OPT_ON) { /* This is standard 11bit CAN reception */
        CAN0IDMR4 = 0xFF;           // any
        CAN0IDMR5 = ~0x08;          // check IDE
        CAN0IDAR5 = 0x00;           // IDE=0 receive 11bit messages
        CAN0IDMR6 = 0xFF;           // any
        CAN0IDMR7 = 0xFF;           // any
//    } else { /* This is Al-CAN 'broadcast' message reception */
//        CAN0IDMR4 = 0xFF;           // anything ok in IDR0(var offset)
//        CAN0IDAR5 = 0x18;           // 0,0,0,SRR=IDE=1
//        CAN0IDMR5 = 0xE7;           // anything ok for var_off cont'd, msgtype
//        CAN0IDAR6 = 0x0F;           // rcv msg can be to everyone (id=15), and
//        CAN0IDMR6 = 0xF0;           // can be from any other device
//        CAN0IDMR7 = 0xFF;           // any var_blk, spare, rtr
//    }
    /* clear init mode */
    CAN0CTL0 &= 0xFE;
    /* wait for synch to bus */
    ix = 0;
    while ((!(CAN0CTL0 & 0x10)) && (ix < 0xfffe)) {
        ix++;
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
    }
    if (ix == 0xfffe) { // normal behaviour ~38
        // CAN broken
        conf_err = 190;
    }

    CAN0TIER = 0; /* Do not enable TX interrupts */
    /* clear RX flag to ready for CAN0 recv interrupt */
    CAN0RFLG = 0xC3;
    CAN0RIER = 0x01; /* set CAN0 rcv full interrupt bit */

//    PTS |= 0x20;                // demo board - enable CAN transceiver
    cp_targ = 0;
    cp_cnt = 0;
    flagbyte14 &= ~FLAGBYTE14_CP_ERR;

    /* It is safe to change RPAGE as Can_Init is only called from _init or _main */
    RPAGE = RPAGE_VARS1; /* required to access v1. */
    for (ix = 0 ; ix < 64; ix++) {
        v1.can_boc_tim[ix] = ix; /* spread timers */
    }
    can_outpc_bcast_ptr = 0;
    can_rcv_in = 0;
    can_rcv_proc = 0;
    /* Clear TX ring buffer */
    for (ix = 0; ix < NO_CANTXMSG; ix++) {
        can_tx_buf[ix].id = 0;
    }
    /* Clear RX ring buffer */
    RPAGE = tables[28].rpg; /* can_rcv config data in page 28 */
    for (ix = 0; ix < NO_CANRCVMSG; ix++) {
        v1.can_rcv_buf[ix].id = 0;
    }
    can_tx_in = 0;
    can_tx_out = 0;
    can_tx_num = 0;
    can_rx_num = 0;
    return;
}

unsigned int can_sendburn(unsigned int table, unsigned int id)
{
    unsigned int ret;
    unsigned char tmp_data[8];

    DISABLE_INTERRUPTS;
    ret = can_build_msg(MSG_BURN, id, table, 0, 0, (unsigned char *)&tmp_data, 0);
    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    flagbyte3 |= FLAGBYTE3_REMOTEBURN;
    ENABLE_INTERRUPTS;

    return ret;
}

unsigned int can_crc32(unsigned int table, unsigned int id)
{
    unsigned int ret;
    unsigned char tmp_data[8];
    /* Must be called with interrupts disabled */
    tmp_data[0] = MSG_CRC;
    tmp_data[1] = 6; /* Send back to table 6 */
    tmp_data[2] = 0; /* Send back to offset 0 at this time */
    tmp_data[3] = 4; /* Always 4 bytes */

    ret = can_build_msg(MSG_XTND, id, table, 0, 4, (unsigned char *)&tmp_data, 0);

    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    return ret;
}

unsigned int can_reqdata(unsigned int table, unsigned int id, unsigned int offset, unsigned char num)
{
    /* Must be called with interrupts disabled */
    /* Create the first read in a series of passthrough requests */
    unsigned int ret;

    ret = can_build_msg_req(id, table, offset, 6, 0, num);

    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    return ret;
}

unsigned int can_snddata(unsigned int table, unsigned int id, unsigned int offset, unsigned char num, unsigned int bufad)
{
    /* Must be called with interrupts disabled */
//    unsigned int ix;
    unsigned int ret;
    unsigned char tmp_data[8];
    /* Send data */
    /* called from serial.c and can.c - both use _global_ bufad - requires GPAGE setting externally */

    /* Double buffer */
    *(unsigned long*)&tmp_data[0] = g_read32(bufad + 0);
    *(unsigned long*)&tmp_data[4] = g_read32(bufad + 4);

    ret = can_build_msg(MSG_CMD, id, table, offset, num, (unsigned char *)&tmp_data, 1); /* passthrough action */

    cp_time = (unsigned int)lmms;
    return ret;
}

unsigned int can_sndMSG_PROT(unsigned int id, unsigned char sz)
{
    unsigned int ret;

    unsigned char tmp_data[8];

    tmp_data[0] = MSG_PROT;
    tmp_data[1] = 6; /* Send back to table 6 */
    tmp_data[2] = 0; /* Send back to offset 0 at this time */
    tmp_data[3] = sz; /* # bytes - either 1 (just protocol no.) or 5 (block sizes as well) */

    DISABLE_INTERRUPTS;
    ret = can_build_msg(MSG_XTND, id, 0, 0, 4, (unsigned char *)&tmp_data, 0);

    flagbyte3 |= flagbyte3_getcandat;
    can_getid = id;
    cp_time = (unsigned int)lmms;
    ENABLE_INTERRUPTS;
    return ret;
}

void can_scan_prot(void)
{
    unsigned int ret, off;
    unsigned char tmp_data[8];

    if (can_scanid == CANid) {
        datax1.can_proto[can_scanid] = SRLVER;
        can_scanid++;
        return;
    }

    off = DATAX1_OFFSET + (unsigned int)&datax1.can_proto[can_scanid] - (unsigned int)&datax1;
    tmp_data[0] = MSG_PROT;
    tmp_data[1] = 7; /* Send back to table 6 */
    tmp_data[2] = off >> 3; /* Send back to offset 0 at this time */
    tmp_data[3] = ((off & 0x07) << 5) | 1; /* # bytes - either 1 (just protocol no.) or 5 (block sizes as well) */

    DISABLE_INTERRUPTS;
    ret = can_build_msg(MSG_XTND, can_scanid, 0, 0, 4, (unsigned char *)&tmp_data, 0);

    cp_time = (unsigned int)lmms;
    ENABLE_INTERRUPTS;

    if (ret == 0) {
        can_scanid++;
    }
}

unsigned int can_sndMSG_SPND(unsigned int onoff)
{
    /* Send a message to CANid=15. This may not be supported on receiving end. */
    unsigned int ret;
    unsigned char tmp_data[8];

    tmp_data[0] = onoff;

    ret = can_build_msg(MSG_SPND, 15, 0, 0, 1, (unsigned char *)&tmp_data, 0);

    return ret;
}

/***************************************************************************
 **
 **  CAN polling from I/O slave device
 **
 **************************************************************************/
/* CAN message set up as follows
 * cx_msg_type = type of message being sent. MSG_REQ is a request for data
 * cx_dest = remote CANid that we are sending message to
 * cx_destvarblk = which variable block (page) to get data from. page 6 = realtime data, outpc
 * cx_destvaroff = offset within that datablock
 * cx_datbuf = a buffer for the data we are sending, none for a request, eight maximum.
 * cx_varbyt = number of bytes sent or requested
 * cx_myvarblk = the variable block to put the reply in (the other end sends a message back with this block no)
 * cx_myvaroff = the variable offset to put the reply in (the other end sends a message back with this block no)
 *
 * With MSG_REQ we send all of that info off and then the other end replies with a MSG_RSP including the variable block and offset
 * to tell us where to deposit the data, kind of like a "push" of the data. We then (blindly) store it where the MSG_RSP asked us to.
 * In the code below we are asking the GPIO board for the raw ADC data and then store it in datax1.gpioadc[] which are then available for
 * use in the code. These can be transformed in the code and then available as outpc.sensors[] as part of the realtime data pack.
 */

void can_poll(void)
{
    unsigned int can_snd_interval;

    if (can_tx_num >= NO_CANTXMSG_POLLMAX) {
        return; /* Queue too full - don't broadcast now */
    }

    // CAN polling code.
    // If enabled it sends a request to a GPIO type board which then sends data back to us.
    // This gets ADC values from a GPIO with user defined CAN_ID= ram4.can_poll_id
    can_snd_interval = (unsigned int)lmms - can_sndclk;

    if (can_snd_interval >= 8) {
        can_sndclk = (unsigned int)lmms; /* If we got busy then CAN polling must take a lower priority */

        can_sndclk_cnt++;
        if (can_sndclk_cnt == 1) {
            can_poll1();
        } else if (can_sndclk_cnt == 2) {
            can_poll2();
        } else if (can_sndclk_cnt == 3) {
            can_poll3();
        } else if (can_sndclk_cnt == 4) {
            can_poll4();
        } else if (can_sndclk_cnt == 5) {
            can_poll5();
        } else if (can_sndclk_cnt == 6) {
            can_poll6();
        } else if (can_sndclk_cnt == 7) {
            can_poll7();
        } else if (can_sndclk_cnt == 8) {
            can_poll8();
        } else if (can_sndclk_cnt == 9) {
            can_poll9();
        } else if (can_sndclk_cnt == 10) {
            can_poll10();
        } else {
            can_sndclk_cnt = 0;
        }
    }
}

void can_poll1(void)
{
    int ix;
    unsigned int off;

    // load ring buffer - send request for data

    if (ram4.enable_poll & 0x06) {
        // copy previous to outpc - circa 10ms delay
        if (ram4.enable_poll & 0x04) { // 32 bit
            unsigned long pwmtmp;
            for (ix = 0 ; ix < 4 ; ix++) {
                unsigned long *ad;
                ad = (unsigned long*)&datax1.pwmin32[ix];
                DISABLE_INTERRUPTS;
                pwmtmp = *ad;
                ENABLE_INTERRUPTS;
                if (pwmtmp & 0xffff0000) {
                    outpc.gpiopwmin[ix] = 0xffff;
                } else {
                    outpc.gpiopwmin[ix] = (unsigned int)pwmtmp;
                }
            }
        } else { // 16bit
            outpc.gpiopwmin[0] = datax1.pwmin16[0];
            outpc.gpiopwmin[1] = datax1.pwmin16[1];
            outpc.gpiopwmin[2] = datax1.pwmin16[2];
            outpc.gpiopwmin[3] = datax1.pwmin16[3];
        }

        // Get remote 16 bit PWM data from Generic board or first 2 x 32 bits
        off = DATAX1_OFFSET + (unsigned short)(&datax1.pwmin16[0]) - (unsigned short)(&datax1); // this is offset of where to store it

        (void)can_build_msg_req(ram4.can_poll_id, ram4.poll_tables[1], ram4.poll_offset[1], 7, off, 8);
    }

    if (ram4.enable_poll & 0x04) {
        // Get second 2 x 32 bit PWM data from Generic board
        off = DATAX1_OFFSET + 8 + (unsigned short)(&datax1.pwmin16[0]) - (unsigned short)(&datax1); // this is offset of where to store it

        (void)can_build_msg_req(ram4.can_poll_id, ram4.poll_tables[1], 8 + ram4.poll_offset[1], 7, off, 8);
    }
}

void can_poll2(void)
{
    unsigned char tmp_data[8];
    unsigned int off;
    // CAN digital port input
    if (ram4.enable_poll & 0x08) {
        // Get remote ports data from Generic board
        off = (unsigned short)(&outpc.canin1_8) - (unsigned short)(&outpc); // this is offset of where to store it

        (void)can_build_msg_req(ram4.can_poll_id_ports, ram4.poll_tables[2], ram4.poll_offset[2], 7, off, 8);
    }

    // CAN digital port outputs
    if (ram4.enable_poll & 0x30) {
        unsigned char num;
        // Send remote ports data to Generic board
        tmp_data[0] = outpc.canout1_8;
        if (ram4.enable_poll & 0x20) {
            tmp_data[1] = outpc.canout9_16;
            num = 2;
        } else {
            num = 1;
        }

        (void)can_build_msg(MSG_CMD, ram4.can_poll_id_ports, ram4.poll_tables[2], ram4.poll_offset[3], num,
            (unsigned char *)tmp_data, 0);
    }
}

void can_poll3(void)
{
     // CAN PWM outputs
    if (ram4.enable_poll & 0x40) {
        // Set remote PWM data on Generic board
        (void)can_build_msg(MSG_CMD, ram4.can_pwmout_id, ram4.can_pwmout_tab, ram4.can_pwmout_offset, 8,
            (unsigned char *)datax1.canpwmout, 0);
    }
}

void can_poll4(void)
{
    int ix;
    unsigned int off;

    //CAN ADCs part 1
    if (ram4.enable_poll & 0x01) {
        for (ix = 0 ; ix < 2 ; ix++) {
            if (ram4.canadc_opt & (1 << ix)) { // are these group of 4 enabled?
                off = DATAX1_OFFSET + (unsigned short)(&datax1.adc[ix << 2]) - (unsigned short)(&datax1); // this is offset of where to store it

                (void)can_build_msg_req(ram4.canadc_id[ix], ram4.canadc_tab[ix], ram4.canadc_off[ix], 7, off, 8);
            }
        }
    }
}

void can_poll5(void)
{
    int ix;
    unsigned int off;

    //CAN ADCs part 2
    if (ram4.enable_poll & 0x01) {
        for (ix = 2 ; ix < 4 ; ix++) {
            if (ram4.canadc_opt & (1 << ix)) { // are these group of 4 enabled?
                off = DATAX1_OFFSET + (unsigned short)(&datax1.adc[ix << 2]) - (unsigned short)(&datax1); // this is offset of where to store it

                (void)can_build_msg_req(ram4.canadc_id[ix], ram4.canadc_tab[ix], ram4.canadc_off[ix], 7, off, 8);
            }
        }
    }
}

void can_poll6(void)
{
    int ix;
    unsigned int off;

    //CAN ADCs part 3
    if (ram4.enable_poll & 0x01) {
        for (ix = 4 ; ix < 6 ; ix++) {
            if (ram4.canadc_opt & (1 << ix)) { // are these group of 4 enabled?
                off = DATAX1_OFFSET + (unsigned short)(&datax1.adc[ix << 2]) - (unsigned short)(&datax1); // this is offset of where to store it

                (void)can_build_msg_req(ram4.canadc_id[ix], ram4.canadc_tab[ix], ram4.canadc_off[ix], 7, off, 8);
            }
        }
    }
}

void can_poll7(void)
{
    int ix;
    unsigned int off;

    // Innovate EGO via CAN
    // currently only supported by Extender
    if (ram4.can_poll2 & CAN_POLL2_EGO) {
        for (ix = 0 ; ix < 2 ; ix++) {
            off = DATAX1_OFFSET + (unsigned short)(&datax1.ego[ix*4]) - (unsigned short)(&datax1);

            (void)can_build_msg_req(ram4.can_ego_id, ram4.can_ego_table, ram4.can_ego_offset + (ix * 8), 7, off, 8);
        }
    }
}

void can_poll8(void)
{
    unsigned char num;
    unsigned int off;

    if (ram4.can_poll2 & CAN_POLL2_VSS) { /* polled or listen */
        /* VSS1 */
        if (((ram4.vss_opt0 & 0x03) == 0x01) && ((ram4.vss_opt & 0x0f) == 0x0e)) {
            if (ram4.vss_can_size & 0x01) { // 16 bit
                num = 2;
                off = DATAX1_OFFSET + (unsigned short)(&datax1.vss1_16) - (unsigned short)(&datax1);
            } else {
                num = 1;
                off = DATAX1_OFFSET + (unsigned short)(&datax1.vss1_8) - (unsigned short)(&datax1);
            }

            (void)can_build_msg_req(ram4.vss1_can_id, ram4.vss1_can_table, ram4.vss1_can_offset, 7, off, num);

            /* VSS3 */
            if ((ram4.vss_opt0 & 0x10) == 0x10) {
                if (ram4.vss_can_size & 0x01) { // 16 bit
                    num = 2;
                    off = DATAX1_OFFSET + (unsigned short)(&datax1.vss3_16) - (unsigned short)(&datax1);
                } else {
                    num = 1;
                    off = DATAX1_OFFSET + (unsigned short)(&datax1.vss3_8) - (unsigned short)(&datax1);
                }

                (void)can_build_msg_req(ram4.vss1_can_id, ram4.vss1_can_table, ram4.vss3_can_offset, 7, off, num);
            }
        }

        /* VSS2 */
        if (((ram4.vss_opt0 & 0x0c) == 0x04) && ((ram4.vss_opt & 0xf0) == 0xe0)) {
            if (ram4.vss_can_size & 0x01) { // 16 bit
                num = 2;
                off = DATAX1_OFFSET + (unsigned short)(&datax1.vss2_16) - (unsigned short)(&datax1);
            } else {
                num = 1;
                off = DATAX1_OFFSET + (unsigned short)(&datax1.vss2_8) - (unsigned short)(&datax1);
            }

            (void)can_build_msg_req(ram4.vss1_can_id, ram4.vss1_can_table, ram4.vss2_can_offset, 7, off, num);

            /* VSS4 */
            if ((ram4.vss_opt0 & 0x40) == 0x40) {
                if (ram4.vss_can_size & 0x01) { // 16 bit
                    num = 2;
                    off = DATAX1_OFFSET + (unsigned short)(&datax1.vss4_16) - (unsigned short)(&datax1);
                } else {
                    num = 1;
                    off = DATAX1_OFFSET + (unsigned short)(&datax1.vss4_8) - (unsigned short)(&datax1);
                }

                (void)can_build_msg_req(ram4.vss1_can_id, ram4.vss1_can_table, ram4.vss4_can_offset, 7, off, num);
            }
        }

        if ((ram4.gear_method & 0x03) == 0x03) {
            off = DATAX1_OFFSET + (unsigned short)(&datax1.gear) - (unsigned short)(&datax1);

            (void)can_build_msg_req(ram4.vss1_can_id, ram4.vss1_can_table, ram4.gear_can_offset, 7, off, 1);
        }
    }
}

void can_poll9(void)
{
    unsigned int off;
    if ((ram4.can_poll2 & CAN_POLL2_GPS_MASK) == CAN_POLL2_GPS_JBPERF) {
        /* First packet of GPS data */
        off = DATAX1_OFFSET + (unsigned short)(&datax1.gps_latdeg) - (unsigned short)(&datax1);
        /* evaluates to 667 2014-01-02 */

        (void)can_build_msg_req(ram5.can_gps_id, ram5.can_gps_table, ram5.can_gps_offset, 7, off, 8);

        /* Second packet */
        off = DATAX1_OFFSET + 8 + (unsigned short)(&datax1.gps_latdeg) - (unsigned short)(&datax1);
        /* evaluates to 667 2014-01-02 */

        (void)can_build_msg_req(ram5.can_gps_id, ram5.can_gps_table, ram5.can_gps_offset + 8, 7, off, 8);
    }
}

void can_poll10(void)
{
    unsigned char tmp_data[8];
    unsigned int off;

    // CAN RTC. Assumes same 8 byte format as extender
/*
    unsigned char rtc_sec, rtc_min, rtc_hour, rtc_day, rtc_date, rtc_month;
    unsigned int rtc_year;
*/
    if ((ram4.opt142 & 0x03) == 2) {
        if (flagbyte9 & FLAGBYTE9_GETRTC) {
            flagbyte9 &= ~FLAGBYTE9_GETRTC;

            // Get time/date

            off = DATAX1_OFFSET+ (unsigned short)(&datax1.rtc_sec) - (unsigned short)(&datax1); // this is offset of where to store it
            /* evaluates to 589 2014-01-02 */

            (void)can_build_msg_req(ram4.can_poll_id_rtc, ram4.poll_tables[0], ram4.poll_offset[0], 7, off, 8);

        } else if (datax1.setrtc_lock == 0x5a) {
            unsigned char tr;
            datax1.setrtc_lock = 0;

            /* HARDCODED: per JB, send to table 0x10, offset 0 and encode in BCD */

            // unsigned char setrtc_sec, setrtc_min, setrtc_hour, setrtc_day, setrtc_date, setrtc_month
            // unsigned int setrtc_year;
            tmp_data[0] = (datax1.setrtc_sec % 10) | ((datax1.setrtc_sec / 10) << 4);
            tmp_data[1] = (datax1.setrtc_min % 10) | ((datax1.setrtc_min / 10) << 4);
            tmp_data[2] = (datax1.setrtc_hour % 10) | ((datax1.setrtc_hour / 10) << 4);
            tmp_data[3] = datax1.setrtc_day;
            tmp_data[4] = (datax1.setrtc_date % 10) | ((datax1.setrtc_date / 10) << 4);
            tmp_data[5] = (datax1.setrtc_month % 10) | ((datax1.setrtc_month / 10) << 4);
            tmp_data[6] = (datax1.setrtc_year % 10) | (((datax1.setrtc_year % 100) / 10) << 4);
            if (ram4.rtc_trim < 0) {
                if (ram4.rtc_trim <= -62) {
                    tr = 63;  // 31 plus sign bit
                } else {
                    tr = 0x20 | ((-ram4.rtc_trim)>>1);  // Set trim to positive value, divide by 2, add sign bit
                }
            } else {
                 tr = ram4.rtc_trim >> 2; // Divide trim by 4
            }
            tmp_data[7] = tr;
            (void)can_build_msg(MSG_CMD, ram4.can_poll_id_rtc, 0x10, 0, 8, (unsigned char *)tmp_data, 0);
        }
    }
}

void send_can11bit(unsigned int id, unsigned char *data, unsigned char bytes) {
    can_tx_msg *this_msg;
    unsigned char maskint_tmp, maskint_tmp2;

    MASK_INTERRUPTS;
    if (can_tx_out < NO_CANTXMSG) {
        this_msg = &can_tx_buf[can_tx_in];
        /* Store 11bit ID into Freescale raw format */
        this_msg->id = (unsigned long)id << 21;

        /* Store data and length */
        *(unsigned int*)&this_msg->data[0] = *(unsigned int*)&data[0];
        *(unsigned int*)&this_msg->data[2] = *(unsigned int*)&data[2];
        *(unsigned int*)&this_msg->data[4] = *(unsigned int*)&data[4];
        *(unsigned int*)&this_msg->data[6] = *(unsigned int*)&data[6];
        this_msg->sz = bytes;
        CAN_INC_TXRING;
    } else {
        /* fail silently */
    } 
    RESTORE_INTERRUPTS;
}

void send_can29bit(unsigned long id, unsigned char *data, unsigned char bytes) {
    /* Sends 29bit CAN message header and chosen data, bypassing Al-CAN
        Beware of sending messages that correspond to Al-CAN messages
        as there is no error trapping.
    */
    unsigned long a, b;
    unsigned int l;
    can_tx_msg *this_msg;
    unsigned char maskint_tmp, maskint_tmp2;

    MASK_INTERRUPTS;
    if (can_tx_out < NO_CANTXMSG) {
        l = bytes; /* To workaround ICE. */
        this_msg = &can_tx_buf[can_tx_in];

        /* Store 29bit ID into Freescale raw format */
        a = (id << 1) & 0x0007ffff;
        b = (id << 3) & 0xffe00000;
        this_msg->id = a | b | 0x00180000;

        /* Store data and length */

        *(unsigned int*)&this_msg->data[0] = *(unsigned int*)&data[0];
        *(unsigned int*)&this_msg->data[2] = *(unsigned int*)&data[2];
        *(unsigned int*)&this_msg->data[4] = *(unsigned int*)&data[4];
        *(unsigned int*)&this_msg->data[6] = *(unsigned int*)&data[6];
        this_msg->sz = l;

        CAN_INC_TXRING;
    } else {
        /* fail silently */
    }
    RESTORE_INTERRUPTS;
}

/* The following function is derived from code written by 'stevep' and donated to MS2/Extra / MS3 */
void can_broadcast(void)
{
    /* Broadcast selected CAN messages using STD CAN*/

    // Send Message Variables
    unsigned int id, val;
    unsigned char data[8];

    if (can_tx_num >= NO_CANTXMSG_POLLMAX) {
        return; /* Queue too full - don't broadcast now */
    }
  
    DISABLE_INTERRUPTS;
    val = (unsigned int)lmms;
    ENABLE_INTERRUPTS;
    if ((val - can_bcast_last) > ram5.can_bcast_int) {
        can_bcast_last = val; // rollover safe
    } else {
        return;
    }
   
    if (ram5.can_bcast1 & 0x02) {
        id = Engine_RPM280; //0x280 - Motorsteuergerat 640 - engine control unit RPM is 4:1 ratio = outpc.rpm*4
        val = outpc.rpm * 4;
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = (unsigned char)(val & 0xff); //byte 3 = RPM, L
        data[3] = (unsigned char)(val >> 8);   //byte 4 = RPM, H
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 0x00;
        data[7] = 0x00;
        send_can11bit(id, data, 8);
    }

    if (ram5.can_bcast1 & 0x04) {
        /* id definition says 1:1 ?? */
        id = Engine_RPM280; //0x280 - Motorsteuergerat 640 - engine control unit RPM is 1:1 ratio = outpc.rpm*1
        val = outpc.rpm * 1;
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = (unsigned char)(val & 0xff); //byte 3 = RPM, L
        data[3] = (unsigned char)(val >> 8);   //byte 4 = RPM, H
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 0x00;
        data[7] = 0x00;
        send_can11bit(id, data, 8);
    }

    if (ram5.can_bcast1 & 0x08) {
        long t1, t2;
        id = Engine_Temp289;//     0x289 // 996 - Motorsteuerger�t 649 - Engine Temp /y=46.14+1.67*x where x is degrees C
        // quadratic to match guage position curve y = -.0336*x*x + 7.7986*x + -225.2571
        val = (((outpc.clt - 320) * 5) / 9)/10;
        if (val > 74) {
            t1 = (-336L * val) * val;
            t2 = 77986L * val;
            val = (unsigned int)((t1 + t2 -2252571) / 10000);
        }
        else {
            val = (5447 + (154 * val)) / 100;
        }
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = (unsigned char)val;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 0x00;
        data[7] = 0x00;
        send_can11bit(id, data, 8);
    }

    if (ram5.can_bcast1 & 0x10) { // from Peter Florance
	    id = Engine_RPM316; //0x316 BMW E46 - engine control unit RPM is 6.42:1 ratio = outpc.rpm*6.42
        val = (unsigned int)((outpc.rpm * 642UL) / 100);        
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = (unsigned char)(val & 0xff); //byte 3 = RPM, L
        data[3] = (unsigned char)(val >> 8);   //byte 4 = RPM, H
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 0x00;
        data[7] = 0x00;
        send_can11bit(id, data, 8);
    }

    if (ram5.can_bcast1 & 0x20) { // from Peter Florance
        id = Engine_Temp329;//     0x329 
//        val = (((outpc.clt - 320) * 5) / 9)/10;
//	    val = (int)(((val + 48)*4)/3);
        val = (1088 + (outpc.clt << 1)) / 27;        
        data[0] = 0x00;
        data[1] = (unsigned char)val;
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 0x00;
        data[7] = 0x00;
        send_can11bit(id, data, 8);
    }

    if ((ram5.can_bcast1 & 0x40)
        && ((ram5.can_bcast2 & 0x01) || (flagbyte21 & FLAGBYTE21_CAN1ST))) { // and 1st message sent (if enabled)
        /* Alfa/Fiat/Lancia dash coolant, RPM. Submitted by 'acab' */
        id = 0x561;
        int valclt, valrpm;

        val = (outpc.clt - 320) / 18;
        valclt = val + 40;
        valrpm = outpc.rpm/32;
/*
0x561 8 00 00 00 00 xx xx 00 00
xx= fuel consumption 0-132 liter/hour
1bit =  0,0022 liter/hour
*/        
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = 0x00;
        data[3] = (unsigned char)valclt;   // Coolant temp
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = (unsigned char)valrpm;   // RPM 
        data[7] = 0x00;
        send_can11bit(id, data, 8);
    }

/* additional submissions by 'acab'
    0x361 | 8 | 00 xx 00 00 00 00 00 00
    xx= torque 0-99.96 % / 1bit = 0.392 %

    0x361 | 8 | 00 00 xx xx 00 00 00 00
    xx xx = rpm 0-10240 rpm / 1bit = 1 rpm

    0x361 | 8 | 00 00 00 00 00 00 00 xx
    xx= gas pedal position 0-99,96 % / 1bit = 0,392 % (may be same as throttle position)

    0x361 | 8 | 00 00 00 00 xx 00 00 00
    xx= throttle position 0- 99,96 % / 1bit = 0,392 %

    Initialisation packet, send only one time after ignition:
    0x041 | 7 | 06 00 00 00 00 00 00
*/
    if ((ram5.can_bcast1 & 0x80)
        && ((ram5.can_bcast2 & 0x01) || (flagbyte21 & FLAGBYTE21_CAN1ST))) { // and 1st message sent (if enabled)
        id = 0x361;
        /* Alfa/Fiat/Lancia torque, RPM, TPS. Submitted by 'acab' */
        int tps, torque;

        if (outpc.tps < 0) {
            tps = 0;
        } else if (outpc.tps > 1000) {
            tps = 255;
        } else {
            tps = (outpc.tps *255UL) / 1000;
        }

        torque = calc_duty(global_base_pw1); // assume duty is a reasonable measure of torque

        data[0] = 0x00;
        data[1] = (unsigned char)torque;
        data[2] = outpc.rpm >> 8;
        data[3] = outpc.rpm & 0xff;
        data[4] = (unsigned char)tps;
        data[5] = 0x00;
        data[6] = 0x00;
        data[7] = (unsigned char)tps;
        send_can11bit(id, data, 8);
    }


    if ((ram5.can_bcast2 & 0x01) && (outpc.seconds) && (!(flagbyte21 & FLAGBYTE21_CAN1ST))) {
        flagbyte21 |= FLAGBYTE21_CAN1ST;
        id = 0x041;

        data[0] = 0x06;
        data[1] = 0x00;
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 0x00;
        send_can11bit(id, data, 7);
    }

    if (ram5.can_bcast2 & 0x02) {
        /* holset HE351VE VGT turbo and a wastegate
        29bit CAN message 0x0cffc600
        8 bytes
        D0 = 0 - 0xff for how far open the vanes should be. 0xff is all the way open.
        D1 = 0x02
        D2 = 0x01
        D3 = 0xff
        D4 = 0xff
        D5 = 0xff
        D6 = 0xff
        D7 = 0xff
        ----------------------
        */
        data[0] = (outpc.boostduty * 255U) / 100U;
        data[1] = 0x02;
        data[2] = 0x01;
        data[3] = 0xff;
        data[4] = 0xff;
        data[5] = 0xff;
        data[6] = 0xff;
        data[7] = 0xff;
        send_can29bit(0x0cffc600, data, 8);
    }

    if (ram5.can_bcast2 & 0x80) {
        RPAGE = tables[28].rpg;
        id = ram_window.pg28.can_bcast_user_id;
        data[0] = ram_window.pg28.can_bcast_user_d[0];
        data[1] = ram_window.pg28.can_bcast_user_d[1];
        data[2] = ram_window.pg28.can_bcast_user_d[2];
        data[3] = ram_window.pg28.can_bcast_user_d[3];
        data[4] = ram_window.pg28.can_bcast_user_d[4];
        data[5] = ram_window.pg28.can_bcast_user_d[5];
        data[6] = ram_window.pg28.can_bcast_user_d[6];
        data[7] = ram_window.pg28.can_bcast_user_d[7];
        send_can11bit(id, data, 8);
    }
}

void can_bcast_outpc_cont()
{
    /* This function gets called multiple times per mainloop pass from serial() */
    unsigned int b, t, t1;
    unsigned char op;

    RPAGE = RPAGE_VARS1; /* required to access v1. */

#define CAN_NUM_PER_PASS 16
    t = (unsigned int)lmms; /* using a local var here produces more efficient code (weird) */

    for (b = 0; b < CAN_NUM_PER_PASS ; b++) { /* Handle 16 in each pass to avoid bloating looptime */
        /* Make sure we do not put too many messages into the queue */
        if (can_tx_num >= NO_CANTXMSG_POLLMAX) {
            b = CAN_NUM_PER_PASS; /* bail on loop */
        } else {
            op = ram5.can_outpc_gp[can_outpc_bcast_ptr] & 0x07;

            if (op) {
/*     can_outpc_gp00 = bits, U08, 488, [0:2], "Off", "1Hz", "2Hz", "5Hz", "10Hz", "20Hz", "50Hz", "100Hz" */
                t1 = can_outpc_int[op];

                if ((t - v1.can_boc_tim[can_outpc_bcast_ptr]) > t1) {
                    v1.can_boc_tim[can_outpc_bcast_ptr] = t;

                    can_bcast_outpc(can_outpc_bcast_ptr); /* send the message */
                }
            }
            can_outpc_bcast_ptr = (can_outpc_bcast_ptr + 1) & 0x3f; /* range of 0-63 */
        }
    }
}

void can_bcast_outpc(unsigned int gp)
{
    /* This function gets called to grab a pre-defined group of data and put
        it into the CAN message queue.
        Note that originally this data is not aligned with outpc, it is
        re-arranged for more logic 8 byte boundaries.
        We do not want to break predefined CAN templates developed for 3rd party
        dashes etc  !!! Leave this data format here as it is !!!*/
    unsigned int id;
    unsigned char data[8];
    unsigned char dataLen;

    id = ram5.can_outpc_msg + gp;
    dataLen = 0x08;

    /* This isn't a particularly elegant way of achieving this, but should
       achieve goal of not changing even if outpc is re-arranged.
       Alternative more elegant solution could be use a const array of bytes
       to grab, but maintenance could be tricky and there could be some hoops
       to jump through to put it into FAR space. */

    if (gp == 0) {
        *(unsigned int*)&data[0] = outpc.seconds;
        *(unsigned int*)&data[2] = outpc.pw1;
        *(unsigned int*)&data[4] = outpc.pw2;
        *(unsigned int*)&data[6] = outpc.rpm;

    } else if (gp == 1) {
        *(int*)&data[0] = outpc.adv_deg;
        data[2] = outpc.squirt;
        data[3] = outpc.engine;
        data[4] = outpc.afrtgt1;
        data[5] = outpc.afrtgt2;
        data[6] = outpc.wbo2_en1;
        data[7] = outpc.wbo2_en2;

    } else if (gp == 2) {
        *(int*)&data[0] = outpc.baro;
        *(int*)&data[2] = outpc.map;
        *(int*)&data[4] = outpc.mat;
        *(int*)&data[6] = outpc.clt;

    } else if (gp == 3) {
        *(int*)&data[0] = outpc.tps;
        *(int*)&data[2] = outpc.batt;
        *(int*)&data[4] = outpc.ego1;
        *(int*)&data[6] = outpc.ego2;

    } else if (gp == 4) {
        *(int*)&data[0] = outpc.knock;
        *(int*)&data[2] = outpc.egocor1;
        *(int*)&data[4] = outpc.egocor2;
        *(int*)&data[6] = outpc.aircor;

    } else if (gp == 5) {
        *(int*)&data[0] = outpc.warmcor;
        *(int*)&data[2] = outpc.tpsaccel;
        *(int*)&data[4] = outpc.tpsfuelcut;
        *(int*)&data[6] = outpc.barocor;

    } else if (gp == 6) {
        *(int*)&data[0] = outpc.gammae;
        *(int*)&data[2] = outpc.vecurr1;
        *(int*)&data[4] = outpc.vecurr2;
        *(int*)&data[6] = outpc.iacstep;

    } else if (gp == 7) {
        *(int*)&data[0] = outpc.cold_adv_deg;
        *(int*)&data[2] = outpc.tpsdot;
        *(int*)&data[4] = outpc.mapdot;
        *(int*)&data[6] = outpc.rpmdot;

    } else if (gp == 8) {
        *(int*)&data[0] = outpc.mafload;
        *(int*)&data[2] = outpc.fuelload;
        *(int*)&data[4] = outpc.fuelcor;
        *(int*)&data[6] = outpc.maf;

    } else if (gp == 9) {
        *(int*)&data[0] = outpc.egoV1;
        *(int*)&data[2] = outpc.egoV2;
        *(unsigned int*)&data[4] = outpc.coil_dur;
        *(unsigned int*)&data[6] = outpc.dwell_trl;

    } else if (gp == 10) {
        data[0] = outpc.status1;
        data[1] = outpc.status2;
        data[2] = outpc.status3;
        data[3] = outpc.status4;
        *(int*)&data[4] = outpc.istatus5;
        data[6] = outpc.status6;
        data[7] = outpc.status7;

    } else if (gp == 11) {
        *(int*)&data[0] = outpc.fuelload2;
        *(int*)&data[2] = outpc.ignload;
        *(int*)&data[4] = outpc.ignload2;
        *(int*)&data[6] = outpc.airtemp;

    } else if (gp == 12) {
        DISABLE_INTERRUPTS;
        *(long*)&data[0] = outpc.wallfuel1;
        *(long*)&data[2] = outpc.wallfuel2;
        ENABLE_INTERRUPTS;

    } else if (gp == 13) {
        *(int*)&data[0] = outpc.sensors[0];
        *(int*)&data[2] = outpc.sensors[1];
        *(int*)&data[4] = outpc.sensors[2];
        *(int*)&data[6] = outpc.sensors[3];

    } else if (gp == 14) {
        *(int*)&data[0] = outpc.sensors[4];
        *(int*)&data[2] = outpc.sensors[5];
        *(int*)&data[4] = outpc.sensors[6];
        *(int*)&data[6] = outpc.sensors[7];

    } else if (gp == 15) {
        *(int*)&data[0] = outpc.sensors[8];
        *(int*)&data[2] = outpc.sensors[9];
        *(int*)&data[4] = outpc.sensors[10];
        *(int*)&data[6] = outpc.sensors[11];

    } else if (gp == 16) {
        *(int*)&data[0] = outpc.sensors[12];
        *(int*)&data[2] = outpc.sensors[13];
        *(int*)&data[4] = outpc.sensors[14];
        *(int*)&data[6] = outpc.sensors[15];

    } else if (gp == 17) {
        *(int*)&data[0] = outpc.boost_targ_1;
        *(int*)&data[2] = outpc.boost_targ_2;
        data[4] = outpc.boostduty;
        data[5] = outpc.boostduty2;
        *(unsigned int*)&data[6] = outpc.maf_volts;

    } else if (gp == 18) {
        *(int*)&data[0] = outpc.pwseq[0];
        *(int*)&data[2] = outpc.pwseq[1];
        *(int*)&data[4] = outpc.pwseq[2];
        *(int*)&data[6] = outpc.pwseq[3];

    } else if (gp == 19) {
        *(int*)&data[0] = outpc.pwseq[4];
        *(int*)&data[2] = outpc.pwseq[5];
        *(int*)&data[4] = outpc.pwseq[6];
        *(int*)&data[6] = outpc.pwseq[7];

    } else if (gp == 20) {
        *(int*)&data[0] = outpc.pwseq[8];
        *(int*)&data[2] = outpc.pwseq[9];
        *(int*)&data[4] = outpc.pwseq[10];
        *(int*)&data[6] = outpc.pwseq[11];

    } else if (gp == 21) {
        *(int*)&data[0] = outpc.pwseq[12];
        *(int*)&data[2] = outpc.pwseq[13];
        *(int*)&data[4] = outpc.pwseq[14];
        *(int*)&data[6] = outpc.pwseq[15];

    } else if (gp == 22) {
        *(int*)&data[0] = outpc.egt[0];
        *(int*)&data[2] = outpc.egt[1];
        *(int*)&data[4] = outpc.egt[2];
        *(int*)&data[6] = outpc.egt[3];

    } else if (gp == 23) {
        *(int*)&data[0] = outpc.egt[4];
        *(int*)&data[2] = outpc.egt[5];
        *(int*)&data[4] = outpc.egt[6];
        *(int*)&data[6] = outpc.egt[7];

    } else if (gp == 24) {
        *(int*)&data[0] = outpc.egt[8];
        *(int*)&data[2] = outpc.egt[9];
        *(int*)&data[4] = outpc.egt[10];
        *(int*)&data[6] = outpc.egt[11];

    } else if (gp == 25) {
        *(int*)&data[0] = outpc.egt[12];
        *(int*)&data[2] = outpc.egt[13];
        *(int*)&data[4] = outpc.egt[14];
        *(int*)&data[6] = outpc.egt[15];

    } else if (gp == 26) {
        data[0] = outpc.nitrous1_duty;
        data[1] = outpc.nitrous2_duty;
        *(unsigned int*)&data[2] = outpc.nitrous_timer_out;
        *(int*)&data[4] = outpc.n2o_addfuel;
        *(int*)&data[6] = outpc.n2o_retard;

    } else if (gp == 27) {
        *(int*)&data[0] = outpc.gpiopwmin[0];
        *(int*)&data[2] = outpc.gpiopwmin[1];
        *(int*)&data[4] = outpc.gpiopwmin[2];
        *(int*)&data[6] = outpc.gpiopwmin[3];

    } else if (gp == 28) {
        *(unsigned int*)&data[0] = outpc.cl_idle_targ_rpm;
        *(int*)&data[2] = outpc.tpsadc;
        *(int*)&data[4] = outpc.eaeload;
        *(int*)&data[6] = outpc.afrload;

    } else if (gp == 29) {
        *(unsigned int*)&data[0] = outpc.EAEfcor1;
        *(unsigned int*)&data[2] = outpc.EAEfcor2;
        *(int*)&data[4] = outpc.vss1dot;
        *(int*)&data[6] = outpc.vss2dot;

    } else if (gp == 30) {
        *(int*)&data[0] = outpc.accelx;
        *(int*)&data[2] = outpc.accely;
        *(int*)&data[4] = outpc.accelz;
        data[6] = outpc.stream_level;
        data[7] = outpc.water_duty;

    } else if (gp == 31) {
        data[0] = outpc.afr[0];
        data[1] = outpc.afr[1];
        data[2] = outpc.afr[2];
        data[3] = outpc.afr[3];
        data[4] = outpc.afr[4];
        data[5] = outpc.afr[5];
        data[6] = outpc.afr[6];
        data[7] = outpc.afr[7];

    } else if (gp == 32) {
        data[0] = outpc.afr[8];
        data[1] = outpc.afr[9];
        data[2] = outpc.afr[10];
        data[3] = outpc.afr[11];
        data[4] = outpc.afr[12];
        data[5] = outpc.afr[13];
        data[6] = outpc.afr[14];
        data[7] = outpc.afr[15];

    } else if (gp == 33) {
        data[0] = outpc.duty_pwm[0];
        data[1] = outpc.duty_pwm[1];
        data[2] = outpc.duty_pwm[2];
        data[3] = outpc.duty_pwm[3];
        data[4] = outpc.duty_pwm[4];
        data[5] = outpc.duty_pwm[5];
        data[6] = outpc.gear;
        data[7] = outpc.status8;

    } else if (gp == 34) {
        *(int*)&data[0] = outpc.egov[0];
        *(int*)&data[2] = outpc.egov[1];
        *(int*)&data[4] = outpc.egov[2];
        *(int*)&data[6] = outpc.egov[3];

    } else if (gp == 35) {
        *(int*)&data[0] = outpc.egov[4];
        *(int*)&data[2] = outpc.egov[5];
        *(int*)&data[4] = outpc.egov[6];
        *(int*)&data[6] = outpc.egov[7];

    } else if (gp == 36) {
        *(int*)&data[0] = outpc.egov[8];
        *(int*)&data[2] = outpc.egov[9];
        *(int*)&data[4] = outpc.egov[10];
        *(int*)&data[6] = outpc.egov[11];

    } else if (gp == 37) {
        *(int*)&data[0] = outpc.egov[12];
        *(int*)&data[2] = outpc.egov[13];
        *(int*)&data[4] = outpc.egov[14];
        *(int*)&data[6] = outpc.egov[15];

    } else if (gp == 38) {
        *(int*)&data[0] = outpc.egocor[0];
        *(int*)&data[2] = outpc.egocor[1];
        *(int*)&data[4] = outpc.egocor[2];
        *(int*)&data[6] = outpc.egocor[3];

    } else if (gp == 39) {
        *(int*)&data[0] = outpc.egocor[4];
        *(int*)&data[2] = outpc.egocor[5];
        *(int*)&data[4] = outpc.egocor[6];
        *(int*)&data[6] = outpc.egocor[7];

    } else if (gp == 40) {
        *(int*)&data[0] = outpc.egocor[8];
        *(int*)&data[2] = outpc.egocor[9];
        *(int*)&data[4] = outpc.egocor[10];
        *(int*)&data[6] = outpc.egocor[11];

    } else if (gp == 41) {
        *(int*)&data[0] = outpc.egocor[12];
        *(int*)&data[2] = outpc.egocor[13];
        *(int*)&data[0] = outpc.egocor[14];
        *(int*)&data[2] = outpc.egocor[15];

    } else if (gp == 42) {
        *(unsigned int*)&data[0] = outpc.vss1;
        *(unsigned int*)&data[2] = outpc.vss2;
        *(unsigned int*)&data[4] = outpc.vss3;
        *(unsigned int*)&data[6] = outpc.vss4;

    } else if (gp == 43) {
        data[0] = outpc.synccnt;
        data[1] = outpc.syncreason;
        *(unsigned int*)&data[2] = outpc.sd_filenum;
        data[4] = outpc.sd_error;
        data[5] = outpc.sd_phase;
        data[6] = outpc.sd_status;
        data[7] = outpc.timing_err;

    } else if (gp == 44) {
        *(int*)&data[0] = outpc.vvt_ang[0];
        *(int*)&data[2] = outpc.vvt_ang[1];
        *(int*)&data[4] = outpc.vvt_ang[2];
        *(int*)&data[6] = outpc.vvt_ang[3];

    } else if (gp == 45) {
        *(int*)&data[0] = outpc.vvt_target[0];
        *(int*)&data[2] = outpc.vvt_target[1];
        *(int*)&data[4] = outpc.vvt_target[2];
        *(int*)&data[6] = outpc.vvt_target[3];

    } else if (gp == 46) {
        data[0] = outpc.vvt_duty[0];
        data[1] = outpc.vvt_duty[1];
        data[2] = outpc.vvt_duty[2];
        data[3] = outpc.vvt_duty[3];
        *(int*)&data[4] = outpc.inj_timing_pri;
        *(int*)&data[6] = outpc.inj_timing_sec;

    } else if (gp == 47) {
        *(int*)&data[0] = outpc.fuel_pct;
        *(int*)&data[2] = outpc.tps_accel;
        *(unsigned int*)&data[4] = outpc.ss1;
        *(unsigned int*)&data[6] = outpc.ss2;

    } else if (gp == 48) {
        data[0] = outpc.knock_cyl[0];
        data[1] = outpc.knock_cyl[1];
        data[2] = outpc.knock_cyl[2];
        data[3] = outpc.knock_cyl[3];
        data[4] = outpc.knock_cyl[4];
        data[5] = outpc.knock_cyl[5];
        data[6] = outpc.knock_cyl[6];
        data[7] = outpc.knock_cyl[7];

    } else if (gp == 49) {
        data[0] = outpc.knock_cyl[8];
        data[1] = outpc.knock_cyl[9];
        data[2] = outpc.knock_cyl[10];
        data[3] = outpc.knock_cyl[11];
        data[4] = outpc.knock_cyl[12];
        data[5] = outpc.knock_cyl[13];
        data[6] = outpc.knock_cyl[14];
        data[7] = outpc.knock_cyl[15];

    } else if (gp == 50) {
        *(int*)&data[0] = outpc.map_accel;
        *(int*)&data[2] = outpc.total_accel;
        *(unsigned int*)&data[4] = outpc.launch_timer;
        *(int*)&data[6] = outpc.launch_retard;

    } else if (gp == 51) {
        data[0] = outpc.porta;
        data[1] = outpc.portb;
        data[2] = outpc.porteh;
        data[3] = outpc.portk;
        data[4] = outpc.portmj;
        data[5] = outpc.portp;
        data[6] = outpc.portt;
        data[7] = outpc.cel_errorcode;

    } else if (gp == 52) {
        data[0] = outpc.canin1_8;
        data[1] = outpc.canout1_8;
        data[2] = outpc.canout9_16;
        data[3] = outpc.knk_rtd;
        *(int*)&data[4] = outpc.fuelflow;
        *(int*)&data[6] = outpc.fuelcons;

    } else if (gp == 53) {
        *(int*)&data[0] = outpc.fuel_press[0];
        *(int*)&data[2] = outpc.fuel_press[1];
        *(int*)&data[4] = outpc.fuel_temp[0];
        *(int*)&data[6] = outpc.fuel_temp[1];

    } else if (gp == 54) {
        *(int*)&data[0] = outpc.batt_curr;
        *(unsigned int*)&data[2] = outpc.cel_status;
        data[4] = outpc.fp_duty;
        data[5] = outpc.alt_duty;
        data[6] = outpc.load_duty;
        data[7] = outpc.alt_targv;

    } else if (gp == 55) {
        *(unsigned int*)&data[0] = outpc.looptime;
        *(int*)&data[2] = outpc.fueltemp_cor;
        *(int*)&data[4] = outpc.fuelpress_cor;
        data[6] = outpc.ltt_cor;
        data[7] = outpc.sp1;

    } else if (gp == 56) {
        *(int*)&data[0] = outpc.tc_retard;
        *(int*)&data[2] = outpc.cel_retard;
        *(int*)&data[4] = outpc.fc_retard;
        *(int*)&data[6] = outpc.als_addfuel;

    } else if (gp == 57) {
        *(int*)&data[0] = outpc.base_advance;
        *(int*)&data[2] = outpc.idle_cor_advance;
        *(int*)&data[4] = outpc.mat_retard;
        *(int*)&data[6] = outpc.flex_advance;

    } else if (gp == 58) {
        *(int*)&data[0] = outpc.adv1;
        *(int*)&data[2] = outpc.adv2;
        *(int*)&data[4] = outpc.adv3;
        *(int*)&data[6] = outpc.adv4;

    } else if (gp == 59) {
        *(int*)&data[0] = outpc.revlim_retard;
        *(int*)&data[2] = outpc.als_timing;
        *(int*)&data[4] = outpc.ext_advance;
        *(int*)&data[6] = outpc.deadtime1;

    } else if (gp == 60) {
        *(int*)&data[0] = outpc.launch_timing;
        *(int*)&data[2] = outpc.step3_timing;
        *(int*)&data[4] = outpc.vsslaunch_retard;
        *(unsigned int*)&data[6] = outpc.cel_status2;

    } else if (gp == 61) {
        *(signed char*)&data[0] = outpc.gps_latdeg;
        data[1] = outpc.gps_latmin;
        *(unsigned int*)&data[2] = outpc.gps_latmmin;
        data[4] = outpc.gps_londeg;
        data[5] = outpc.gps_lonmin;
        *(unsigned int*)&data[6] = outpc.gps_lonmmin;

    } else if (gp == 62) {
        data[0] = outpc.gps_outstatus;
        *(signed char*)&data[1] = outpc.gps_altk;
        *(unsigned int*)&data[2] = outpc.gps_altm;
        *(unsigned int*)&data[4] = outpc.gps_speedkm;
        *(unsigned int*)&data[6] = outpc.gps_course;

    } else if (gp == 63) {
        data[0] = outpc.generic_pid_duty[0];
        data[1] = outpc.generic_pid_duty[1];
        data[2] = 0; /* unassigned */
        data[3] = 0;
        data[4] = 0;
        data[5] = 0;
        data[6] = 0;
        data[7] = 0;

    } else {
        /* Should be an impossible case */
        dataLen = 0;
    }
    if (dataLen) {
        send_can11bit(id, data, dataLen);
    }

}

void can_rcv_process(void)
{
    unsigned int lp, id;
    unsigned char m;
    unsigned long longid;
    can_rcv_msg *this_msg;

    /* Sanity check first, this should never happen but prevents an infinite loop */
    if (can_rcv_in > NO_CANRCVMSG) {
        can_rcv_in = 0;
        return;
    }
    RPAGE = tables[28].rpg; /* can_rcv config data in page 28 */

    lp = 0; /* Loop counter to save getting stuck if CAN goes crazy */
    while ((can_rcv_in != can_rcv_proc) && (lp < 20)) {
        int a;
        can_rcv_proc++;
        if (can_rcv_proc >= NO_CANRCVMSG) {
            can_rcv_proc = 0;
        }

        this_msg = &v1.can_rcv_buf[can_rcv_proc];

        longid = this_msg->id;
        this_msg->id = 0; /* Ensure slot is marked as empty */

        if (longid & 0x80000000) { /* 29bit CAN */

            DISABLE_INTERRUPTS;
            if (can_rx_num) {
                can_rx_num--; /* Declare that we processed a message */
            }
            ENABLE_INTERRUPTS;

            /* Here we process the 29bit Megasquirt-CAN messages.
               the header is presented without the Freescale bits.*/

            /* Ignore, should have been handled in ISR */


        } else if (longid) { /* 11bit CAN (and non zero) */
            id = (unsigned int)longid;

            DISABLE_INTERRUPTS;
            if (can_rx_num) {
                can_rx_num--; /* Declare that we processed a message */
            }
            ENABLE_INTERRUPTS;

            m = 0;
            if (ram5.can_rcv_opt & CAN_RCV_OPT_ON) {
                for (a = 0 ; a < NO_CANRCVMSG; a++) {
                    /* compare against available bcast opts */
                    if (id == ram_window.pg28.can_rcv_id[a]) {
                        m = 1;
                        unsigned char s, t, o, *v;
                        signed long sval;
                        unsigned long uval;

                        s = 0;
                        t = ram_window.pg28.can_rcv_size[a] & 0x0f;
                        /* can_rcv_size1 = bits, U08, 800, [0:3], "1U", "1S", "B2U", "B2S", "B4U", "B4S", "L2U", "L2S", "L4U", "L4S"... */
                        v = &this_msg->data[ram_window.pg28.can_rcv_off[a]];
                        if (t == 0) { /* U08 */
                            uval = (long)(unsigned char)v[0];
                        } else if (t == 1) { /* S08 */
                            sval = (long)(signed char)v[0];
                            s = 1;
                        } else if (t == 2) { /* U16 big */
                            uval = (long)*(unsigned int*)v;
                        } else if (t == 3) { /* S16 big */
                            sval = (long)*(signed int*)v;
                            s = 1;
                        } else if (t == 4) { /* U32 big */
                            uval = (long)*(unsigned long*)v;
                        } else if (t == 5) { /* S32 big */
                            sval = (long)*(signed long*)v;
                            s = 1;
                        } else if (t == 6) { /* U16 little */
                            unsigned char *w;
                            w = (unsigned char*)&uval;
                            w[0] = 0;
                            w[1] = 0;
                            w[2] = v[1];
                            w[3] = v[0];
                        } else if (t == 7) { /* S16 little */
                            unsigned char *w;
                            int tmp_int; 
                            w = (unsigned char*)&tmp_int;
                            w[0] = v[1];
                            w[1] = v[0];
                            sval = (long)tmp_int;
                            s = 1;
                        } else if (t == 8) { /* U32 little */
                            unsigned char *w;
                            w = (unsigned char*)&uval;
                            w[0] = v[3];
                            w[1] = v[2];
                            w[2] = v[1];
                            w[3] = v[0];
                        } else if (t == 9) { /* S32 little */
                            unsigned char *w;
                            w = (unsigned char*)&sval;
                            w[0] = v[3];
                            w[1] = v[2];
                            w[2] = v[1];
                            w[3] = v[0];
                            s = 1;
                        } else {
                            sval = -1;
                            uval = 32767;
                        }

                        /* can easily overflow here */
                        if (s == 1) {
                            sval *= ram_window.pg28.can_rcv_mult[a];
                            sval /= ram_window.pg28.can_rcv_div[a];
                            sval += ram_window.pg28.can_rcv_add[a];
                            uval = (unsigned int)sval;
                        } else {
                            uval *= ram_window.pg28.can_rcv_mult[a];
                            uval /= ram_window.pg28.can_rcv_div[a];
                            uval += ram_window.pg28.can_rcv_add[a];
                            sval = (int)uval;
                        }

                        /* Now store it */
        /* can_rcv_var1 = bits,    U08,    784, [0:5], "Off", "CAN VSS1", "CAN VSS2", "CAN VSS3", "CAN VSS4", "INVALID", "INVALID", "INVALID", "CAN ADC01", "CAN ADC02", "CAN ADC03", "CAN ADC04", "CAN ADC05", "CAN ADC06", "CAN ADC07", "CAN ADC08", "CAN ADC09", "CAN ADC10", "CAN ADC11", "CAN ADC12", "CAN ADC13", "CAN ADC14", "CAN ADC15", "CAN ADC16", "CAN ADC17", "CAN ADC18", "CAN ADC19", "CAN ADC20", "CAN ADC21", "CAN ADC22", "CAN ADC23", "CAN ADC24" */
                        o = ram_window.pg28.can_rcv_var[a] & 0x01f;
                        if (o == 1) {
                            datax1.vss1_16 = uval;
                        } else if (o == 2) {
                            datax1.vss2_16 = uval;
                        } else if (o == 3) {
                            datax1.vss3_16 = uval;
                        } else if (o == 4) {
                            datax1.vss4_16 = uval;
                        } else if (o == 7) {
                            outpc.istatus5 = sval;
                        } else if ((o >= 8) && (o <= 31)) {
                            datax1.adc[o - 8] = sval;
                        } else {
                            /* nothing */
                        }
                    }
                }
            }
            /* IO-box uses v1. in RPAGE_VARS1. Valid page checked at config time */
            if (m == 0) { /* Didn't match above, check for IO-box */
                for (a = 0 ; a < 4; a++) {
                    if (ram_window.pg28.iobox_opta[a] & 0x1) { /* Is it enabled at all */ 
                        if (id == (v1.io_id[a] + 8)) {
                            if (this_msg->data[0] == 1) {
                                v1.io_pwmclk[a] = *(unsigned int*)&this_msg->data[4];
                                v1.io_tachclk[a] = *(unsigned int*)&this_msg->data[6];
                                v1.io_phase[a] = 2;
                            } else {
                                v1.io_phase[a] = 255; /* Not supported */
                            }
                        } else if (id == (v1.io_id[a] + 9)) {
                            unsigned int *base_adc;
                            if (ram_window.pg28.iobox_opta[a] & 0x2) { /* Advanced */
                                base_adc = (unsigned int*)&datax1.adc[((ram_window.pg28.iobox_opta[a] & 0xc0) >> 6) * 8];
                                /* Gives CANADC1, 9, 17, 23 */
                            } else {
                                base_adc = (unsigned int*)&datax1.adc[a * 8];
                            }
                            base_adc[0] = *(unsigned int*)&this_msg->data[0];
                            base_adc[1] = *(unsigned int*)&this_msg->data[2];
                            base_adc[2] = *(unsigned int*)&this_msg->data[4];
                            base_adc[3] = *(unsigned int*)&this_msg->data[6];
                        } else if (id == (v1.io_id[a] + 10)) {
                            unsigned int *base_adc;
                            unsigned char *canin_ptr, bitshift, bitmask, opt;

                            if (ram_window.pg28.iobox_opta[a] & 0x2) { /* Advanced */
                                base_adc = (unsigned int*)&datax1.adc[((ram_window.pg28.iobox_opta[a] & 0xc0) >> 6) * 8];
                                /* Gives CANADC1, 9, 17, 23 */
                                opt = (ram_window.pg28.iobox_opta[a] & 0x0c) >> 2;
                            } else {
                                base_adc = (unsigned int*)&datax1.adc[a * 8];
                                opt = a;
                            }
                            if (opt == 0) {
                                canin_ptr = (unsigned char*)&outpc.canin1_8;
                                bitshift = 0;
                                bitmask = 0xd8;
                            } else if (opt == 1) {
                                canin_ptr = (unsigned char*)&outpc.canin1_8;
                                bitshift = 3;
                                bitmask = 0xc7;
                            } else if (opt == 2) {
                                canin_ptr = (unsigned char*)&outpc.canin1_8;
                                bitshift = 6;
                                bitmask = 0x3f;
                            } else {
                                canin_ptr = (unsigned char*)&dummyReg;
                                bitshift = 0;
                                bitmask = 0;
                            }

                            *canin_ptr = (*canin_ptr & bitmask) | ((this_msg->data[0] & 0x07) << bitshift);
                            base_adc[4] = *(unsigned int*)&this_msg->data[2];
                            base_adc[5] = *(unsigned int*)&this_msg->data[4];
                            base_adc[6] = *(unsigned int*)&this_msg->data[6];
                        } else if (id == (v1.io_id[a] + 11)) {
                            /* Store time and teeth into local variables */
                            if (v1.io_tachconf[a] & 1) {
                                unsigned long t;
                                t = *(unsigned long*)&this_msg->data[0];
                                t = t * v1.io_tachclk[a]; /* Possibility of overflow */
                                t = t / 5000; /* Convert back to 50us ticks */
                                /* Special case for IObox#1,#2 running two tach ins each */
                                if ((a == 1) && (v1.io_tachconf[0] == 3)) {
                                    vss3_time_sum = t;
                                    vss3_teeth = *(unsigned int*)&this_msg->data[4];
                                    flagbyte19 |= FLAGBYTE19_SAMPLE_VSS3;
                                    vss3_stall = 0;
                                } else {
                                    vss1_time_sum = t;
                                    vss1_teeth = *(unsigned int*)&this_msg->data[4];
                                    flagbyte8 |= FLAGBYTE8_SAMPLE_VSS1;
                                    vss1_stall = 0;
                                }
                            }
                        } else if (id == (v1.io_id[a] + 12)) {
                            if (v1.io_tachconf[a] & 2) {
                                unsigned long t;
                                t = *(unsigned long*)&this_msg->data[0];
                                t = t * v1.io_tachclk[a]; /* Possibility of overflow */
                                t = t / 5000; /* Convert back to 50us ticks */
                                /* Special case for IObox#1,#2 running two tach ins each */
                                if ((a == 1) && (v1.io_tachconf[0] == 3)) {
                                    vss4_time_sum = t;
                                    vss4_teeth = *(unsigned int*)&this_msg->data[4];
                                    flagbyte19 |= FLAGBYTE19_SAMPLE_VSS4;
                                    vss4_stall = 0;
                                } else {
                                    vss2_time_sum = t;
                                    vss2_teeth = *(unsigned int*)&this_msg->data[4];
                                    flagbyte8 |= FLAGBYTE8_SAMPLE_VSS2;
                                    vss2_stall = 0;
                                }
                            }

                        } else if (id == (v1.io_id[a] + 13)) {
                            if (v1.io_tachconf[a] & 4) {
                                unsigned long t;
                                t = *(unsigned long*)&this_msg->data[0];
                                t = t * v1.io_tachclk[a]; /* Possibility of overflow */
                                t = t / 5000; /* Convert back to 50us ticks */
                                vss3_time_sum = t;
                                vss3_teeth = *(unsigned int*)&this_msg->data[4];
                                flagbyte19 |= FLAGBYTE19_SAMPLE_VSS3;
                                vss3_stall = 0;
                            }

                        } else if (id == (v1.io_id[a] + 14)) {
                            if (v1.io_tachconf[a] & 8) {
                                unsigned long t;
                                t = *(unsigned long*)&this_msg->data[0];
                                t = t * v1.io_tachclk[a]; /* Possibility of overflow */
                                t = t / 5000; /* Convert back to 50us ticks */
                                vss4_time_sum = t;
                                vss4_teeth = *(unsigned int*)&this_msg->data[4];
                                flagbyte19 |= FLAGBYTE19_SAMPLE_VSS4;
                                vss4_stall = 0;
                            }
                        }
                    }
                }
            }
        }
    }
}

#define DEFAULT_IOBOX_ID 512
void conf_iobox(void)
{
    int a, b, base;
    unsigned char opta;

    if (tables[28].rpg != RPAGE_VARS1) {
        conf_err = 183;
        return;
    }

    RPAGE = RPAGE_VARS1;

    for (a = 0; a < 4; a++) {
        v1.io_conf[a] = 0;
        v1.io_tachconf[a] = 0;
        v1.io_sndclk[a] = 0;
        opta = ram_window.pg28.iobox_opta[a];
        if (opta & 0x1) { /* Enabled */
            v1.io_phase[a] = 0;

            if (opta & 0x2) { /* Advanced */
                v1.io_id[a] = ram_window.pg28.iobox_id[a];
                base = (opta & 0x30) >> 1;
            } else {
                v1.io_id[a] = DEFAULT_IOBOX_ID + (a * 16);
                base = a * 8;
            }
            /* Mapping to PWM IO channels */
            if (base < 9) {
                for (b = 0; b < 7; b++) {
                    if (v1.io_stat[base + b] & 1) {
                        v1.io_conf[a] |= twopow[b];
                    }
                }
            }

            /* Configure VSS data */
            if (ram_window.pg28.iobox_optb[a] & 0x40) {
                if (((ram4.vss_opt0 & 0x03) == 0x01) && ((ram4.vss_opt & 0x0f) == 0x0f)) {
                    /* Allow VSS1,2 on one IO-box and VSS3,4 on the next */
                    if (flagbyte24 & FLAGBYTE24_IOVSS1) {
                        flagbyte24 |= FLAGBYTE24_IOVSS3;
                    } else {
                        flagbyte24 |= FLAGBYTE24_IOVSS1;
                    }
                    v1.io_tachconf[a] |= 0x01;
                }
                if (((ram4.vss_opt0 & 0x03) == 0x01) && ((ram4.vss_opt & 0xf0) == 0xf0)) {
                    if (flagbyte24 & FLAGBYTE24_IOVSS2) {
                        flagbyte24 |= FLAGBYTE24_IOVSS4;
                    } else {
                        flagbyte24 |= FLAGBYTE24_IOVSS2;
                    }
                    v1.io_tachconf[a] |= 0x02;
                }
            }
            if (ram_window.pg28.iobox_optb[a] & 0x80) {
                if ((ram4.vss_opt0 & 0x10) && ((ram4.vss_opt & 0x0f) == 0x0f)) {
                    flagbyte24 |= FLAGBYTE24_IOVSS3;
                    v1.io_tachconf[a] |= 0x04;
                }
                if ((ram4.vss_opt0 & 0x10) && ((ram4.vss_opt & 0xf0) == 0xf0)) {
                    flagbyte24 |= FLAGBYTE24_IOVSS4;
                    v1.io_tachconf[a] |= 0x08;
                }
            }
        } else {
            v1.io_phase[a] = 255; /* Force disabled */
        }
    }
}

void can_iobox(void)
{
    unsigned int can_snd_interval, t, a, base;
    unsigned char *canout_ptr, tmp_data[8], opta, optb;

    RPAGE = RPAGE_VARS1; /* required to access v1. */

    for (a = 0; a < 4; a++) {
        opta = ram_window.pg28.iobox_opta[a];
        optb = ram_window.pg28.iobox_optb[a];

        if ((opta & 0x1) && (v1.io_phase[a] != 255)) { /* Enabled */
    
            can_snd_interval = (unsigned int)lmms - v1.io_sndclk[a];

            if ((v1.io_phase[a] == 0) && (can_snd_interval > 156)) {
                /* Send AYT message */
                send_can11bit(v1.io_id[a], tmp_data, 0);
                v1.io_sndclk[a] = (unsigned int)lmms;
                v1.io_phase[a] = 1;
            } else if ((v1.io_phase[a] == 2) || (v1.io_refresh[a] > 100)) {
                v1.io_refresh[a] = 0;
                /* Send this every few seconds, in case remote loses power. */

                /* Send AYT if we haven't had a reply yet */
                if (v1.io_phase[a] == 1) {
                    /* Send AYT message */
                    send_can11bit(v1.io_id[a], tmp_data, 0);
                }

                /* Send config message */
                 tmp_data[0] = v1.io_conf[a] & 0xff; /* configure outputs as PWM */
                 tmp_data[1] = 0;
                 tmp_data[2] = v1.io_tachconf[a];
                 tmp_data[3] = 0;

                if (opta & 0x2) { /* Advanced */
                    /* ADC / input broadcast rate */
                    if ((optb & 0x0c) == 0x00) {
                        tmp_data[4] = 100; /* 10Hz */
                    } else if ((optb & 0x0c) == 0x04) {
                        tmp_data[4] = 50; /* 20Hz */
                    } else if ((optb & 0x0c) == 0x08) {
                        tmp_data[4] = 20; /* 50Hz */
                    } else {
                        tmp_data[4] = 10; /* 100Hz */
                    }

                    /* Tachin sample/broadcast rate */
                    if ((optb & 0x03) == 0x00) {
                        tmp_data[5] = 20; /* 50Hz */
                    } else {
                        tmp_data[5] = 10; /* 100Hz */
                    }
                } else {
                    tmp_data[4] = 20; /* ADC broadcast interval in ms */
                    tmp_data[5] = 20; /* Tach broadcast interval in ms */
                }
                tmp_data[6] = 0;
                tmp_data[7] = 0;
                send_can11bit(v1.io_id[a] + 1, tmp_data, 8);
                if (v1.io_phase[a] == 2) { /* only move on if we have received a reply */
                    v1.io_phase[a] = 3;
                }
            }

            if ((v1.io_phase[a] == 1) || (v1.io_phase[a] == 3)) { /* Always send this data even if remote didn't reply */
                if (opta & 0x2) { /* Advanced */

                    t = optb & 0x30; /* 10, 20, 50, 100Hz */
                    if (t == 0) {
                        t = 781;
                    } else if (t == 0x10) {
                        t = 390;
                    } else if (t == 0x20) {
                        t = 156;
                    } else {
                        t = 78;
                    }
                } else {
                    t = 156;
                }

                if (can_snd_interval >= t) {
                    v1.io_sndclk[a] = (unsigned int)lmms;
                    v1.io_refresh[a]++;
                    if (opta & 0x2) { /* Advanced */
                        base = (opta & 0x30) >> 4;
                    } else {
                        base = a;
                    }
                    if (base == 0) {
                        canout_ptr = (unsigned char*)&outpc.canout1_8;
                    } else if (base == 1) {
                        canout_ptr = (unsigned char*)&outpc.canout9_16;
                    } else {
                        canout_ptr = (unsigned char*)&dummyReg;
                    }

                    base = (opta & 0x30) >> 1;

                    /* Need to create a method to determine which PWM channels get sent to which box */
                    if ((v1.io_conf[a] & 0x03) && (base <= 8)) { /* PWM channels 1 or 2 */
                        *(unsigned int*)&tmp_data[0] = v1.io_max_on[base]; /* Adjust this to send next in group */
                        *(unsigned int*)&tmp_data[2] = v1.io_max_off[base];
                        *(unsigned int*)&tmp_data[4] = v1.io_max_on[base + 1];
                        *(unsigned int*)&tmp_data[6] = v1.io_max_off[base + 1];
                        send_can11bit(v1.io_id[a] +2, tmp_data, 8);
                    }
                    if ((v1.io_conf[a] & 0x0c) && (base <= 8)) { /* PWM channels 3 or 4 */
                        *(unsigned int*)&tmp_data[0] = v1.io_max_on[base + 2]; /* Adjust this to send next in group */
                        *(unsigned int*)&tmp_data[2] = v1.io_max_off[base + 2];
                        *(unsigned int*)&tmp_data[4] = v1.io_max_on[base + 3];
                        *(unsigned int*)&tmp_data[6] = v1.io_max_off[base + 3];
                        send_can11bit(v1.io_id[a] +3, tmp_data, 8);
                    }
                    if ((v1.io_conf[a] & 0x30) && (base <= 8)) { /* PWM channels 5 or 6 */
                        *(unsigned int*)&tmp_data[0] = v1.io_max_on[base + 4]; /* Adjust this to send next in group */
                        *(unsigned int*)&tmp_data[2] = v1.io_max_off[base + 4];
                        *(unsigned int*)&tmp_data[4] = v1.io_max_on[base + 5];
                        *(unsigned int*)&tmp_data[6] = v1.io_max_off[base + 5];
                        send_can11bit(v1.io_id[a] +4, tmp_data, 8);
                    }
                    if ((v1.io_conf[a] & 0xc0) || 1) { /* PWM channels 7 or bitfields - always sent */
                        if (base <= 8) {
                            *(unsigned int*)&tmp_data[0] = v1.io_max_on[base + 6]; /* Adjust this to send next in group */
                            *(unsigned int*)&tmp_data[2] = v1.io_max_off[base + 6];
                        }
                        tmp_data[4] = *canout_ptr;
                        send_can11bit(v1.io_id[a] +5, tmp_data, 5);
                    }
                }
            }
        }
    }
}

void io_pwm_outs()
{
    /**************************************************************************
     ** Remote software PWM outputs - calculate on/off times 
     ** same concept as local software PWMs
     **************************************************************************/
    int i;
    unsigned char duty;
    unsigned int max, trig, trig2, freq;
    RPAGE = RPAGE_VARS1;

    /* Calculate PWM clock rate */
    for (i = 0; i < NUM_IO_PWMS ; i++) {
        if (v1.io_stat[i] & 1) {
            /* figure out PWM parameters for isr_rtc.s */

            if (v1.io_stat[i] & 0x20) { // variable frequency, fixed duty
                /* use the duty variable as the frequency */
                freq = *v1.io_duty[i];

                trig = (100000000UL / v1.io_pwmclk[0]) / freq;

                trig2 = trig >> 1;
                if (trig & 1) {
                    trig = trig2 + 1; /* This gains us an extra bit of duty precision */
                } else {
                    trig = trig2;
                }
                v1.io_max_on[i] = trig;
                v1.io_max_off[i] = trig;
                
            } else { // fixed frequency, variable duty
                freq = v1.io_freq[i]; /* contains Hz x10 */
                duty = *v1.io_duty[i];
                if ((!(v1.io_stat[i] & 0x80)) && (duty > 100)) { //0-100
                    duty = 100;
                }

                max = (1000000000UL / v1.io_pwmclk[0]) / freq;

                if (v1.io_stat[i] & 0x80) { //0-255
                    trig = (unsigned int) (((unsigned long)max * duty) / 255);
                } else {
                    trig = (unsigned int) (((unsigned long)max * duty) / 100);
                }

                if (v1.io_stat[i] & 0x40) { // negative polarity
                    v1.io_max_off[i] = trig; // off time
                    v1.io_max_on[i] = max - trig; // on time
                } else { // positive polarity
                    v1.io_max_on[i] = trig; // on time
                    v1.io_max_off[i] = max - trig; // off time
                }
            }
        }
    }
}

void can_do_tx()
{
    /* Take a message from our queue and put it into the CPU TX buffer if space */
    /* Interrupts MUST be masked by calling code */
    unsigned long id;
    unsigned char action;
    unsigned char *d;
    can_tx_msg *this_msg;

    this_msg = &can_tx_buf[can_tx_out]; /* grab a pointer */
    id = this_msg->id;

    if (can_tx_num) {
        can_tx_num--; /* Declare that we sent a packet */
    } else {
        /* fail silently */
    }
    can_tx_out++; /* Ring buffer pointer */
    if (can_tx_out >= NO_CANTXMSG) {
        can_tx_out = 0;
    }

    if (id) {
        CAN0TBSEL = CAN0TFLG; /* Select the free TX buffer */
        /* Tx buffer holds raw Freescale format ID registers */
        *(unsigned long*)&CAN0_TB0_IDR0 = id;
        this_msg->id = 0; /* Mark as zero to say we've handled it */
        CAN0_TB0_DLR = this_msg->sz;
        action = this_msg->action;

        d = (unsigned char*)&CAN0_TB0_DSR0;
        *(unsigned int*)&d[0] = *(unsigned int*)&this_msg->data[0];
        *(unsigned int*)&d[2] = *(unsigned int*)&this_msg->data[2];
        *(unsigned int*)&d[4] = *(unsigned int*)&this_msg->data[4];
        *(unsigned int*)&d[6] = *(unsigned int*)&this_msg->data[6];

        CAN0_TB0_TBPR = 0x02; /* Arbitrary message priority */

        can_status &= CLR_XMT_ERR;
        can_txtime = (unsigned int)lmms;

        CAN0TFLG = CAN0TBSEL; /* Send it off */

        if (action == 1) { /* check for any passthrough write messages */
            if ((flagbyte3 & flagbyte3_sndcandat) && cp_targ) {
                int num;
                unsigned int r;
                unsigned char gp_tmp = GPAGE;

                num = cp_targ - cp_cnt;
                if (num > 8) {
                    num = 8;
                } else {
                    /* final block, prevent sending any more*/
                    flagbyte14 |= FLAGBYTE14_SERIAL_OK; // serial code to return ack code that all bytes were stuck into pipe
                    flagbyte3 &= ~flagbyte3_sndcandat;
                    cp_targ = 0;
                }
                
                GPAGE = 0x13;
                r = can_snddata(cp_table, cp_id, cp_offset + cp_cnt, num, 0xf009 + cp_cnt); // ring0
                GPAGE = gp_tmp;
                if (r) {
                    /* flag up the error so serial.c can act*/
                    flagbyte14 |= FLAGBYTE14_CP_ERR;
                }
                cp_cnt += num;
            }
        } else if (action >= 10) { /* OUTMSG send */
            can_build_outmsg(action - 10);
        }
    }
}

unsigned int can_build_msg(unsigned char msg, unsigned char to, unsigned char tab, unsigned int off,
                             unsigned char by, unsigned char *dat, unsigned char action) {
    /* Build a 29bit CAN message in processor native format */
    unsigned long id;
    can_tx_msg *this_msg;
    unsigned int ret;
    unsigned char maskint_tmp, maskint_tmp2;

#define CALC_ID_ASM

    MASK_INTERRUPTS;
    if (can_tx_out < NO_CANTXMSG) {
#ifdef CALC_ID_ASM
        unsigned int idh, idl;
        __asm__ __volatile__ (
            "ldd %1\n"
            "ldy #32\n"
            "emul\n"
            "orab   #0x18\n"
            "orab   %2\n"
            :"=d"(idh)
            :"m"(off), "m"(msg)
            :"y","x"
        );

        __asm__ __volatile__ (
            "ldaa %1\n"
            "lsla\n"
            "lsla\n"
            "lsla\n"
            "lsla\n"
            "oraa   %2\n"
            "ldab   %3\n"
            "lslb\n"
            "lslb\n"
            "lslb\n"
            "lslb\n"
            "bcc    no_bit4\n"
            "orab   #0x08\n"
            "no_bit4:"
            :"=d"(idl)
            :"m"(ram4.mycan_id), "m"(to), "m"(tab)
            :"y","x"
        );

        id = ((unsigned long)idh << 16) | idl;
#else
        /* This C implementation looks cleaner but takes 11us instead of 1us */
        id = 0;
        id |= (unsigned long)off << 21;
        id |= (unsigned long)msg << 16;
        id |= (unsigned long)ram4.mycan_id << 12;
        id |= (unsigned long)to << 8;
        id |= (tab & 0x0f) << 4;
        id |= (tab & 0x10) >> 1;
        id |= 0x180000; /* SRR and IDE bits */
#endif
        this_msg = &can_tx_buf[can_tx_in]; /* grab a pointer */
        this_msg->id = id;
        this_msg->sz = by;
        this_msg->action = action;
        *(unsigned int*)&this_msg->data[0] = *(unsigned int*)&dat[0];
        *(unsigned int*)&this_msg->data[2] = *(unsigned int*)&dat[2];
        *(unsigned int*)&this_msg->data[4] = *(unsigned int*)&dat[4];
        *(unsigned int*)&this_msg->data[6] = *(unsigned int*)&dat[6];

        CAN_INC_TXRING;
        ret = 0;
    } else {
        ret = 1;
    }
    RESTORE_INTERRUPTS;
    return ret;
}

unsigned int can_build_msg_req(unsigned char to, unsigned char tab, unsigned int off,
                     unsigned char rem_table,
                     unsigned int rem_off, unsigned char rem_by) {
    /* Build a MSQ_REQ message */
    /* Must be called with interrupts off */
    unsigned char tmp_data[8];
    unsigned int ret;

    if (tab < 32) {
        tmp_data[0] = rem_table; /* Send back to table x */
        tmp_data[1] = rem_off >> 3; /* offset */
        tmp_data[2] = ((rem_off & 0x07) << 5) | rem_by; /* offset, # bytes */
        ret = can_build_msg(MSG_REQ, to, tab, off, 3, (unsigned char *)&tmp_data, 0);
    } else {
        tmp_data[0] = MSG_REQX;
        tmp_data[1] = rem_table; /* Send back to table x */
        tmp_data[2] = rem_off >> 3; /* offset */
        tmp_data[3] = ((rem_off & 0x07) << 5) | rem_by; /* offset, # bytes */
        tmp_data[4] = tab; /* Large table number */
        ret = can_build_msg(MSG_XTND, to, tab, off, 5, (unsigned char *)&tmp_data, 0);
    }
    return ret;
}

void chk_crc(void)
{
    unsigned long crc = 0;
    unsigned char save_rpage, tmp_data[8];
    /* if required, calc crc of ram page */
    /* local CRC handled in serial.c now */
    if (flagbyte9 & FLAGBYTE9_CRC_CAN) {
        flagbyte9 &= ~FLAGBYTE9_CRC_CAN;
// only check ram copy, irrespective of page number

        save_rpage = RPAGE;
        RPAGE = tables[tble_idx].rpg;
        crc = crc32buf(0, tables[tble_idx].adhi << 8, 0x400);    // incrc, buf, size
        RPAGE = save_rpage;
        *(unsigned long*)&tmp_data[0] = crc;

        can_build_msg(MSG_RSP, canbuf[5], canbuf[4], 0, 4, (unsigned char*)tmp_data, 0);
    }
}

void can_build_outmsg(unsigned char msg_no)
{
    unsigned char rp, var_byt;
    unsigned int p, kx, lx, cv, cur_size;
    outmsg *this_outmsg;
    outmsg_stat *this_outmsgstat;
    unsigned char tmp_data[8];

    rp = RPAGE;
    RPAGE = tables[27].rpg;

    this_outmsg = &r27.outmsgs[msg_no];
    this_outmsgstat = &cur_outmsg_stat[msg_no];
    cv = this_outmsgstat->count;

    var_byt = 0;
    for (kx = cv; kx < OUTMSG_SIZE; kx++) {
        cur_size = this_outmsg->size[kx];
        if ((cur_size != 0) && ((var_byt + cur_size) <= 8)) {
            p = (unsigned int)&outpc + this_outmsg->offset[cv];
            for (lx = 0; lx < cur_size; lx++) {
                tmp_data[var_byt] = *(unsigned char *)(p + lx);
                var_byt++;
            }
            cv++;
        } else {
            break;
        }
    }

    this_outmsgstat->count = cv;

    if (var_byt) {
        can_build_msg(OUTMSG_RSP, this_outmsgstat->id, this_outmsgstat->tab, this_outmsgstat->off, var_byt, (unsigned char*)tmp_data, 10 + msg_no);
        this_outmsgstat->off += var_byt;
    }
    RPAGE = rp;
}

#define DEFAULT_DASHBCAST_ID 1512
void can_dashbcast(void)
{
    unsigned int can_snd_interval, t, id;
    unsigned char tmp_data[8], opta;

    RPAGE = RPAGE_VARS1; /* required to access v1. */

    opta = ram_window.pg28.dashbcast_opta;
    if (opta & 0x1) { /* Enabled */

        can_snd_interval = (unsigned int)lmms - v1.dashbcast_sndclk;

        if (opta & 0x2) { /* Advanced */
            t = opta & 0x30; /* 10, 20, 50, 100Hz */
            if (t == 0) {
                t = 781;
            } else if (t == 0x10) {
                t = 390;
            } else if (t == 0x20) {
                t = 156;
            } else {
                t = 78;
            }
            id = ram_window.pg28.dashbcast_id;
        } else {
            t = 156;
            id = DEFAULT_DASHBCAST_ID;
        }

        if (can_snd_interval >= t) {
            v1.dashbcast_sndclk = (unsigned int)lmms;

            /* packet 0 */
            *(unsigned int*)&tmp_data[0] = outpc.map;
            *(unsigned int*)&tmp_data[2] = outpc.rpm;
            *(unsigned int*)&tmp_data[4] = outpc.clt;
            *(unsigned int*)&tmp_data[6] = outpc.tps;
            send_can11bit(id + 0, tmp_data, 8);

            /* packet 1 */
            *(unsigned int*)&tmp_data[0] = outpc.pw1;
            *(unsigned int*)&tmp_data[2] = outpc.pw2;
            *(unsigned int*)&tmp_data[4] = outpc.mat;
            *(unsigned int*)&tmp_data[6] = outpc.adv_deg;
            send_can11bit(id + 1, tmp_data, 8);

            /* packet 2 */
            tmp_data[0] = outpc.afrtgt1;
            tmp_data[1] = outpc.afr[0];
            *(unsigned int*)&tmp_data[2] = outpc.egocor[0];
            *(unsigned int*)&tmp_data[4] = outpc.egt[0];
            *(unsigned int*)&tmp_data[6] = outpc.pwseq[0];
            send_can11bit(id + 2, tmp_data, 8);

            /* packet 3 */
            *(unsigned int*)&tmp_data[0] = outpc.batt;
            *(unsigned int*)&tmp_data[2] = outpc.sensors[0];
            *(unsigned int*)&tmp_data[4] = outpc.sensors[1];
            *(unsigned int*)&tmp_data[6] = outpc.knk_rtd;
            send_can11bit(id + 3, tmp_data, 8);

            /* packet 4 */
            *(unsigned int*)&tmp_data[0] = outpc.vss1;
            *(unsigned int*)&tmp_data[2] = outpc.tc_retard;
            *(unsigned int*)&tmp_data[4] = outpc.launch_timing;
            *(unsigned int*)&tmp_data[6] = 0; /* unused */
            send_can11bit(id + 4, tmp_data, 8);
        }
    }
}
