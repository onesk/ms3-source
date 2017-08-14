/* $Id: ms3_can_isr.c,v 1.57.4.3 2015/05/19 20:40:10 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * CanTxIsr()
    Origin: Al Grippo.
    Major: Complete re-write James Murray
    Majority: James Murray
 * CanRxIsr()
    Origin: Al Grippo.
    Moderate: Additions by James Murray / Jean Belanger. Rewrite TX method James Murray.
    Majority: Al Grippo / James Murray / Jean Belanger
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#include "ms3.h"

INTERRUPT void CanTxIsr(void)
{
    CAN0TIER = 0; /* Do not want any interrupts. */
    return;
}

INTERRUPT void CanRxIsr(void)
{
    unsigned char rcv_id,msg_type,var_blk,var_byt,jx,kx;
    unsigned short var_off;
    unsigned char tmp_data[8];
    unsigned char rem_table, rem_bytes;
    unsigned int rem_offset, ad_tmp, dest_addr;
    can_rcv_msg *this_msg;

    /* CAN0 Recv Interrupt */
    if (CAN0RFLG & 0x01) {
        unsigned char save_rpage;

        save_rpage = RPAGE;

        if ((CAN0IDAC & 0x07) == 1) { /* Standard 11bit receiving */
            can_rcv_in++;
            if (can_rcv_in >= NO_CANRCVMSG) {
                can_rcv_in = 0;
            }

            RPAGE = RPAGE_VARS1;
            /* Store actual identifier, without Freescale specific bits */
            this_msg = &v1.can_rcv_buf[can_rcv_in];
            this_msg->id = ((unsigned int)CAN0_RB_IDR0 << 3) | (CAN0_RB_IDR1 >> 5);
            this_msg->sz = CAN0_RB_DLR & 0x0F;
            *(unsigned int*)&this_msg->data[0] = *(unsigned int*)&CAN0_RB_DSR0;
            *(unsigned int*)&this_msg->data[2] = *(unsigned int*)&CAN0_RB_DSR2;
            *(unsigned int*)&this_msg->data[4] = *(unsigned int*)&CAN0_RB_DSR4;
            *(unsigned int*)&this_msg->data[6] = *(unsigned int*)&CAN0_RB_DSR6;

        } else { /* Extended 29 bit */
            var_off = ((unsigned short) CAN0_RB_IDR0 << 3) |
                ((CAN0_RB_IDR1 & 0xE0) >> 5);
            msg_type = (CAN0_RB_IDR1 & 0x07);
            if (msg_type == MSG_XTND) {
                msg_type = CAN0_RB_DSR0;
            }
            rcv_id = CAN0_RB_IDR2 >> 4;     // message from device rcv_id
            var_blk = (CAN0_RB_IDR3 >> 4) | ((CAN0_RB_IDR3 & 0x08) << 1);
            var_byt = (CAN0_RB_DLR & 0x0F);

            dest_addr = 0;
            ad_tmp = 0;

            if (var_off >= 0x4000) {
                /* Impossible! */
                /* in ver 2 protocol will send back error reply */
            } else if ((msg_type == MSG_CMD) || (msg_type == MSG_RSP)) {
                //case MSG_CMD:  // msg for this ecu to set a variable to val in msg
                //case MSG_RSP:  // msg in reply to this ecu's request for a var val
                // value in the data buffer in recvd msg

                if ((msg_type == MSG_RSP) && (flagbyte3 & flagbyte3_getcandat) && (var_blk == 6)) {
//                        can_gd_retry = 0; /* Got a packet, clear retry */
                    if (var_byt && (var_byt <= 8)) {
                        unsigned char gp_tmp = GPAGE;
                        GPAGE = 0x13;
                        for (jx = 0; jx < var_byt; jx++) {
                            g_write8(*(&CAN0_RB_DSR0 + jx), canrxad);
                            canrxad++;
                            if (canrxgoal) {
                                canrxgoal--;
                            }
                        }
                        GPAGE = gp_tmp;

                        if (canrxgoal == 0) {
                            /* not expecting any more data */
                            flagbyte11 |= FLAGBYTE11_CANRX; /* tell mainloop to do something with the data */
                        } else {
                            unsigned char num2;
                            unsigned int r;

                            cp_cnt += var_byt; /* number of bytes received */
                            num2 = canrxgoal; /* number of bytes left */

                            if (num2 > 8) {
                                num2 = 8;
                            }

                            r = can_reqdata(cp_table, cp_id, cp_offset + cp_cnt, num2);

                            if (r) {
                                /* flag up the error so serial.c can act*/
                                flagbyte14 |= FLAGBYTE14_CP_ERR;
                            }
                        }
                    } else {
                        /* bogus message received */
                        /* turn off CAN receive mode */
                        flagbyte3 &= ~flagbyte3_getcandat;
                    }
                } else {
                    unsigned char ck = 0;

                    ad_tmp = dest_addr + var_off;

                    if (var_blk < 32) {

                        dest_addr = (unsigned int)tables[var_blk].addrRam;

                        if (var_blk <= 3) {
                            if (((ram5.flashlock & 1) == 0) || burnstat) {
                                    dest_addr = 0;
                                    /* ideally want to send an error code back to the sender */
                            } else {
                                /* data is temporarily stored to RPAGE 0xf1, used otherwise by SDcard readback */
                                ad_tmp = dest_addr + var_off;
                                if (tables[var_blk].rpg) {
                                    RPAGE = tables[var_blk].rpg;
                                }
                            }

                        } else if (var_blk == 6) {
                            dest_addr = (unsigned int)&canbuf;
                            ad_tmp = dest_addr + var_off;

                        } else if (var_blk == 7) {

                            /* See if we've received the second GPS CAN packet */
                            if ((ram4.can_poll2 & CAN_POLL2_GPS_MASK) == CAN_POLL2_GPS_JBPERF) {
                                if (var_off == (DATAX1_OFFSET + 8 + (unsigned short)(&datax1.gps_latdeg) - (unsigned short)(&datax1))) {
                                    flagbyte22 |= FLAGBYTE22_CANGPS;
                                }
                            }

                            if (var_off >= DATAX1_OFFSET) { // datax1 sharing same block no. as outpc
                                var_off -= DATAX1_OFFSET; // but at addresses DATAX1_OFFSET and beyond
                                if ((var_off + var_byt) > sizeof(datax1)) {
                                    flagbyte3 |= flagbyte3_can_reset;
                                } else {
                                    dest_addr = (unsigned int) &datax1;
                                }
                            } else {
                                if ((var_off + var_byt) > sizeof(outpc)) {
                                    flagbyte3 |= flagbyte3_can_reset;
                                } else {
                                    dest_addr = (unsigned int) &outpc;
                                }
                            }
                            ad_tmp = dest_addr + var_off;

                        } else {
                            ad_tmp = dest_addr + var_off;
                            if (tables[var_blk].rpg) {
                                RPAGE = tables[var_blk].rpg;
                                ck = 1;
                            }
                            if (var_blk == 25) {
                            /* special case handling for MAF lookup curve */
                            if (var_off < 256) {
                                flagbyte21 |= FLAGBYTE21_POPMAF;
                            }
                    }

                        }

                        if ((var_blk != 7) && ((var_off + var_byt) > tables[var_blk].n_bytes)) {
                            flagbyte3 |= flagbyte3_can_reset;
                            dest_addr = 0;
                        }
                    }

                    if (dest_addr) {

                        if ((ad_tmp < 0x1000) || (ad_tmp > 0x3e00) || (var_byt > 8)) {
                            flagbyte3 |= flagbyte3_can_reset;
                        } else {
                            for (jx = 0; jx < var_byt; jx++) {
                                if ((ck)
                                    && (*(unsigned char *)(ad_tmp + jx) != *(&CAN0_RB_DSR0 + jx))) {
                                    outpc.status1 |= STATUS1_NEEDBURN;  // we changed some tuning data
                                }
                                *((unsigned char *) (ad_tmp + jx)) = *(&CAN0_RB_DSR0 + jx);
                            }

                        if (var_blk <= 3) { // calibration data
                            /* At end of each complete block perform a burn automatically. */
                            if (var_off == (tables[var_blk].n_bytes - 8)) {
                                /* Note that this does allow misbehaving tuning software to cause a partial burn. */
                                burn_idx = var_blk;
                                Flash_Init(); // check FDIV written to (should be at reset) likely safe?
                                flocker = 0xdd;
                                burnstat = 300;
                            }
                        } else if ((var_blk == 16) && ((var_off + var_byt) == 64)) {
                            flagbyte1 |= FLAGBYTE1_EX; // page 0x10 check.
                        } else if ((var_blk == 7) && (dest_addr == (unsigned int)&datax1)
                            && (var_off <= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1))
                            && ((var_off + var_byt) >= ((unsigned int)&datax1.testmodelock - (unsigned int)&datax1) + 1)) {
                            /* was the write to datax1 and covering the testmodelock.
                             (To ensure data capture doesn't disturb testmode.) */
                            /* test mode special code */
                            if (datax1.testmodelock == 12345) {
                                unsigned int ret;
                                ret = do_testmode();
                                /* no method of acting upon return value presently */
                            } else {
                                flagbyte1 &= ~flagbyte1_tstmode; /* disable test mode */
                                outpc.status3 &= ~STATUS3_TESTMODE;
                            }
                        }
                        /* Note that SDcard operations aren't supported remotely over CAN. */

                        }
                    }
                }

            } else if ((msg_type == MSG_REQX) || (msg_type == MSG_REQ)) { /* peek message */
                    /* Validate incoming addresses etc, then build return RSP message */
                    if (msg_type == MSG_REQ) {
                        rem_table = CAN0_RB_DSR0 & 0x1f;
                        rem_bytes = CAN0_RB_DSR2 & 0x0f;
                        rem_offset = ((unsigned short)CAN0_RB_DSR1 << 3) | ((CAN0_RB_DSR2 & 0xe0) >> 5);
                    } else {
                        rem_table = CAN0_RB_DSR1;
                        rem_bytes = CAN0_RB_DSR3 & 0x0f;
                        rem_offset = ((unsigned int)CAN0_RB_DSR2 << 3) | ((CAN0_RB_DSR3 & 0xe0) >> 5);
                        var_blk = CAN0_RB_DSR4;
                    }
                // put variable value(s) in xmit ring buffer
                if ((var_blk > 3) && (var_blk < 32)) {
                    if (((var_blk != 7) && ((var_off + rem_bytes) > tables[var_blk].n_bytes))
                        || ((var_blk == 7) && (var_off < DATAX1_OFFSET)  && ((var_off + rem_bytes) > sizeof(outpc)))
                        || ((var_blk == 7) && (var_off >= DATAX1_OFFSET) && (var_off < CONFERR_OFFSET) && ((var_off + rem_bytes) > (sizeof(datax1) + DATAX1_OFFSET)))
                        || ((var_blk == 7) && (var_off >= CONFERR_OFFSET) && ((var_off + rem_bytes) > (CONFERR_OFFSET + CONFERR_SIZE)))
                        ) {
                        flagbyte3 |= flagbyte3_can_reset;
                    } else {
                        if ((var_blk == 7) && (var_off >= CONFERR_OFFSET)) {
                            if (var_off <= (CONFERR_OFFSET + CONFERR_SIZE)) {
                                dest_addr = 0xf700;
                                var_off -= CONFERR_OFFSET;
                            }
                        } else if ((var_blk == 7) && (var_off >= DATAX1_OFFSET)) {
                            dest_addr = (unsigned int)&datax1;
                            var_off -= DATAX1_OFFSET;
                        } else {
                            dest_addr = (unsigned int)tables[var_blk].addrRam;
                        }
                        if (tables[var_blk].rpg) {
                            RPAGE = tables[var_blk].rpg;
                        }
                    }
                } else if ((var_blk >= 0xf0) && (var_blk <= 0xf4)) { // loggers
                    RPAGE = TRIGLOGPAGE;
                    dest_addr = TRIGLOGBASE;
                }
                if (dest_addr == 0xf700) {
                    unsigned char gp;
                    gp = GPAGE;
                    GPAGE = 0x13;
                    ad_tmp = dest_addr + var_off;
                    for (kx = 0; kx < rem_bytes; kx++) {
                        tmp_data[kx] = g_read8(ad_tmp + kx);
                    }
                    GPAGE = gp;
                    ad_tmp = (unsigned int)&tmp_data[0];
                } else if (dest_addr) {
                    ad_tmp = dest_addr + var_off;
                    if ((ad_tmp < 0x1000) || (ad_tmp > 0xeff7) || (rem_bytes > 8)) {
                        flagbyte3 |= flagbyte3_can_reset;
                        ad_tmp = 0;
                    }
                }

                if (ad_tmp) {
                    can_build_msg(MSG_RSP, rcv_id, rem_table, rem_offset, rem_bytes, (unsigned char*)ad_tmp, 0);
                }

                /* MAP/MAF loggers not supported via CAN as they require processing that would impact the engine operation.
                    For serial this conversion is handled in the mainloop, so less troublesome. */
                if ((var_blk >= 0xf0) && (var_blk <= 0xf3)) {
                    if (var_off == 0) {
                        flagbyte0 &= ~(FLAGBYTE0_TTHLOG | FLAGBYTE0_TRGLOG | FLAGBYTE0_COMPLOG /*| FLAGBYTE0_MAPLOG | FLAGBYTE0_MAPLOGARM*/);
                        /* ensure tooth logger off, ensure trigger logger off, ensure composite tooth logger off */
                        outpc.status3 &= ~STATUS3_DONELOG; /* clear 'complete/available' flag */
                    } else if (var_off == 1016) {
                        /* As all CAN handling is in this interrupt, we don't have a way of simply clearing the data
                          on the first read. So all tooth loggers will return one page of junk when accessed via CAN. */
                        flagbyte8 |= FLAGBYTE8_LOG_CLR; /* this actually starts the logging */
                        page = var_blk;
                    }
                }

            } else if (msg_type == OUTMSG_REQ) { // msg to send back current value of an outmsg
                unsigned char msg_no;
                outmsg_stat *this_outmsg_stat;

                msg_no = CAN0_RB_DSR0;
                this_outmsg_stat = &cur_outmsg_stat[msg_no];

                this_outmsg_stat->count = 0; /* Start from beginning */
                this_outmsg_stat->id = rcv_id; /* Where it is going */
                this_outmsg_stat->tab = var_blk;
                this_outmsg_stat->off = var_off;

                can_build_outmsg(msg_no);

    //          case OUTMSG_RSP:  // Not implemented

            } else if (msg_type == MSG_BURN) {
                Flash_Init();       //check FDIV written to (should be at reset)
                flocker = 0xdd;
                burn_idx = var_blk;
                burnstat = 1;       //set flag byte to enable mainloop erase and burn

            } else if (msg_type == MSG_FWD) {  // msg to forward data to the serial port
		        // set up for serial xmit of the recvd CAN bytes (<= 7)
		        // Unsolicited data
		        if ((var_byt > 0) && (var_byt <= 7))  {
                    canbuf[16] = var_byt - 1; // serial.c will use this
			        for (jx = 0;jx < canbuf[16];jx++)  {
				        canbuf[jx] = *(&CAN0_RB_DSR1 + jx);
			        }
                    flagbyte14 |= FLAGBYTE14_SERIAL_FWD; // mainloop should send when safe
		        }

            } else if (msg_type == MSG_CRC) {  // msg to calc CRC32 of page - actually done in misc
                if ((var_blk > 3) && (var_blk < 32)) {
                    tble_idx = var_blk;
                    flagbyte9 |= FLAGBYTE9_CRC_CAN; // set up to do the biz from the mainloop
                    // txbuf seems a fair place to store this data. Hoping nothing else tries to write it.
                    canbuf[4] = CAN0_RB_DSR1; // dest var blk
                    canbuf[5] = rcv_id;
                }

            } else if (msg_type == MSG_PROT) {  // msg to send back current protocols
                rem_table = CAN0_RB_DSR1;
                rem_bytes = CAN0_RB_DSR3 & 0x0f;
                rem_offset = ((unsigned short)CAN0_RB_DSR2 << 3) |
                        ((CAN0_RB_DSR3 & 0xE0) >> 5);
                tmp_data[0] = SRLVER;
                if (rem_bytes == 5) {
                    *(unsigned int*)&tmp_data[1] = SRLDATASIZE;
                    *(unsigned int*)&tmp_data[3] = SRLDATASIZE;
                }
                can_build_msg(MSG_RSP, rcv_id, rem_table, rem_offset, rem_bytes, (unsigned char*)tmp_data, 0);


            } else if (msg_type == MSG_SPND) {
                if (CAN0_RB_DSR1 == 1) {
                    flagbyte17 |= FLAGBYTE17_CANSUSP;
                } else {
                    flagbyte17 &= ~FLAGBYTE17_CANSUSP;
                }

            } else if (msg_type == MSG_BURNACK) {
                flagbyte3 |= FLAGBYTE3_REMOTEBURNSEND;
            }                       // end msg_type switch
            can_status &= CLR_RCV_ERR;
        } /* end else for StdCAN vs. AlCAN */
        RPAGE = save_rpage;
    }
    if (CAN0RFLG & 0x72) {
        /* 0x40 = error counters changed
           0x30 = receiver status bits
           0x02 = overrun
        */
        can_status |= RCV_ERR; /* need to do something with this */
        //can_reset = 1;
    }

    CAN0RFLG = 0xc3; /* Clear all pending flags */

    return;
}
