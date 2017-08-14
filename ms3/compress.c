/*  $Id: compress.c,v 1.23.6.1 2015/01/23 15:44:31 jsmcortina Exp $
 * This source file is derived from public domain code, it was modified
 * for Megasquirt-3 use and remains public domain.
 *
 * This file is a part of Megasquirt-3.
 *
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
 */

/* simplified compression - derived from ncompress42.c */
/* (N)compress42.c - File compression ala IEEE Computer, Mar 1992.
 */
#include "ms3.h"
#include        <stdlib.h>
//#include        <string.h>

#define IBUFSIZ 512             /* input buffer size */
#define OBUFSIZ 2048            /* output buffer size */
#define SIZE_INNER_LOOP         256     /* Size of the inter (fast) compress loop */
/* Defines for third byte of header */
#define MAGIC_1         (uint8_t)'\037' /* First byte of compressed file */
#define MAGIC_2         (uint8_t)'\235' /* Second byte of compressed file */
#define BIT_MASK        0x1f    /* Mask for 'number of compresssion bits' */
/* Masks 0x20 and 0x40 are free.                                */
/* I think 0x20 should mean that there is               */
/* a fourth header byte (for expansion).        */
#define BLOCK_MODE      0x80    /* Block compresssion if table is full and */
/* compression rate is dropping flush tables    */

/* the next two codes should not be changed lightly, as they must not   */
/* lie within the contiguous general code space. */
#define FIRST   257             /* first free entry */
#define CLEAR   256             /* table clear output code */
#define INIT_BITS 9             /* initial number of bits/code */
#define BITS 12                 /* can be 16,15,14,13,12 */
#define HSIZE   5003            /* 80% occupancy */
#define CHECK_GAP 2000 // was 10000, but testing on one file showed 2000 gave 5% smaller result

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef char int8_t;
typedef int int16_t;
typedef long int32_t;

#define MAXCODE(n)      (1L << (n))

/*
#define output(b,o,c,n) {       uint16_t        p = (b) + ((o)>>3); \
    int32_t          i = ((int32_t)(c))<<((o)&0x7); \
    comp_output3(p, i); \
    (o) += (n); \
}
*/

#define SIZEOFOUTBUF OBUFSIZ+2048
/* memory addresses now in ms3_vars.h */

int comp_readsd(unsigned int *inbuf, unsigned char *ibufsw)
{
    if (sd_filesize_bytes >= 1) {       // used as SECTOR counter
        unsigned int ad = 0;
        unsigned char magic;

      crsd_redo:
        ARMCOP = 0x55;
        ARMCOP = 0xAA;
        if (*ibufsw == 0) {
            // If first time here, going to read sector, busy wait
            // then fire off a read of the second sector.
            ad = 0;
            sd_block = 0;
        } else if (*ibufsw == 1) {
            while (sd_int_phase != 0) {
                ARMCOP = 0x55;
                ARMCOP = 0xAA;
            };                  // has last sector read finished?
            ad = 0x200;         // store next read in second buffer
            *inbuf = SDSECT1;         // compress to read from first buffer
            *ibufsw = 2;
            sd_block++;
        } else if (*ibufsw == 2) {
            while (sd_int_phase != 0) {
                ARMCOP = 0x55;
                ARMCOP = 0xAA;
            };                  // has last sector read finished?
            ad = 0;             // store next read in first buffer
            *inbuf = SDSECT2;     // compress to read from second buffer
            *ibufsw = 1;
            sd_block++;
        } else {
            return (0);         // shouldn't happen
        }

        if (outpc.sd_status & 0x02) {
            sd_int_sector = sd_pos;
        } else {
            sd_int_sector = sd_pos << SD_BLOCK_SHIFT;
        }
        sd_int_cmd = 5;         // read block command
        sd_int_phase = 1;       // first stage
        sd_match = SD_OK;
        sd_int_addr = SDSECT1 + ad;      // where to store the results.
        sd_int_cnt = 0;
        sd_int_error = 0;
        SS_ENABLE;
        (void) SPI1SR;
        SPI1DRL = SD_CMD17 | 0x40;      // read block command
        SPI1CR1 |= 0x80;        // enable SPI interrupts, on data received

        // now we can carry on
        sd_pos++;
        if (sd_filesize_bytes) {        // prevent negative
            sd_filesize_bytes--;
        }

        if (*ibufsw == 0) {
            while (sd_int_phase != 0) {
                ARMCOP = 0x55;
                ARMCOP = 0xAA;
            };                  // wait for first read to complete
            __asm__ __volatile__("ldab 0,x\n" "stab %0\n":"=m"(sd_magic)
                                 :"x"(*inbuf + 127)
                );
            *ibufsw = 1;
            goto crsd_redo;
        }

        DISABLE_INTERRUPTS;
        __asm__ __volatile__("ldab 0,x\n" "stab %0\n":"=m"(magic)
                             :"x"(*inbuf + 63)
            );
        if ((sd_block > 7) && (magic != sd_magic)) {
            ENABLE_INTERRUPTS;
            return (0);         // magic number doesn't match, must be beyond EOF
        } else {
            ENABLE_INTERRUPTS;
            return (512);
        }
    } else {
        return (0);
    }
}

/*****************************************************************
 * TAG( main )
 *
 * Algorithm from "A Technique for High Performance Data Compression",
 * Terry A. Welch, IEEE Computer Vol 17, No 6 (June 1984), pp 8-19.
 *
 */

/*
 * compress fdin to fdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the 
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */
void read_compress()
{
    int32_t bytes_in;               /* Total number of byte from input                              */
    int32_t bytes_out;              /* Total number of byte to output                               */
    int16_t hp; // was 32
    uint16_t outbits; // was 32
    int16_t rpos, rlop, rsize;  // likely u16 in final
    uint8_t stcode;             // only 1 bit needed
    int32_t free_ent; // was int32_t
    //      int32_t boff;
    int16_t boff;
    uint8_t n_bits;
    int32_t ratio;
    int32_t checkpoint;
    int32_t extcode;

    uint16_t inbuf /*, outbuf*/;
    uint8_t ibufsw;

    union {
        int32_t code;
        struct {
            uint16_t ent;
            uint8_t ignore;
            uint8_t c;
        } e;
    } fcode;


    GPAGE = 0x0f;   /* global page for RAM. Code here and ASM functions require this. */

    // lower parts of global addresses
//    inbuf = 0x0000;             // Same as RPAGE = 0xf0, local address = 0x1000.   g = 0x0f 0000
    inbuf = SDSECT1;             // Same as                       local RAM 0x0c00    g = 0x13 FC00
//    outbuf = 0xa000;            // Same as RPAGE = 0xfa, local address = 0x1000  g = 0x0f a000
// now defined in ms3_vars.h

    sd_rb_block = 0x0000;
    ibufsw = 0;

    ratio = 0;
    checkpoint = CHECK_GAP;
    extcode = MAXCODE(n_bits = INIT_BITS) + 1;
    stcode = 1;
    free_ent = FIRST;

    bytes_out = 0;
    bytes_in = 0;

    sd_filesize_bytes++;        // need an extra count due to sector pre-reading

    //disable many interrupts to give max CPU time
//    TIE = 0; // all timers off
//    TIMTIE = 0; // TIM timers too
//    PITINTE = 0; // PIT
    //    RTICTL = 0; // 0.128ms

    //add an additional dummy write/read in here
    SS_DISABLE;
    SPI1CR1 &= ~0x80;           // turn off ints so foreground code can work
    SPI1DRL = 0xFF;             // send to receive

    while (!(SPI1SR & 0x80)) {
        ARMCOP = 0x55;
        ARMCOP = 0xAA;
    };
    (void) SPI1DRL;

    //add a second additional dummy write/read in here
    SS_DISABLE;
    SPI1CR1 &= ~0x80;           // turn off ints so foreground code can work
    SPI1DRL = 0xFF;             // send to receive

    while (!(SPI1SR & 0x80)) {
        ARMCOP = 0x55;
        ARMCOP = 0xAA;
    };
    (void) SPI1DRL;

    comp_memnmagic(OUTBUF, SIZEOFOUTBUF);

    boff = outbits = (3 << 3);
    fcode.code = 0;

    comp_clearhash(HSIZE);
    
    sd_timeout = 0;

    while ((rsize = comp_readsd(&inbuf, &ibufsw)) > 0) {
        if (bytes_in == 0) {
            __asm__ __volatile__(
                "clra\n"
                "ldab 0,x\n"
                "std %0\n"
                :"=m"(fcode.e.ent)
                :"x"(inbuf)
                :"d"
            );

            rpos = 1;
        } else {
            rpos = 0;
        }

        rlop = 0;

        do {
            ARMCOP = 0x55;
            ARMCOP = 0xAA;
            if (free_ent >= extcode && fcode.e.ent < FIRST) {
                if (n_bits < BITS) {
                    boff = outbits = (outbits - 1) + ((n_bits << 3) -
                                                      ((outbits - boff -
                                                        1 +
                                                        (n_bits << 3)) %
                                                       (n_bits << 3)));
                    if (++n_bits < BITS)
                        extcode = MAXCODE(n_bits) + 1;
                    else
                        extcode = MAXCODE(n_bits);
                } else {
                    extcode = MAXCODE(16) + OBUFSIZ;
                    stcode = 0;
                }
            }

            if (!stcode && (bytes_in >= checkpoint)
                && (fcode.e.ent < FIRST)) {
                int32_t newratio;

                checkpoint = bytes_in + CHECK_GAP;

                // checking to see if the compression is working, if not then clear the hash table and start again
                // this is the adaptive reset
                if (bytes_in > 0x007fffff) {    /* shift will overflow */
                    newratio = (bytes_out + (outbits >> 3)) >> 8;

                    if (newratio == 0) {  /* Don't divide by zero */
                        newratio = 0x7fffffff;
                    } else {
                        newratio = bytes_in / newratio;
                    }
                } else {
                    newratio = (bytes_in << 8) / (bytes_out + (outbits >> 3));  /* 8 fractional bits */
                }

                if (newratio >= ratio) {
                    ratio = (int32_t) newratio; // rolling compression ratio. Cast not required at the moment
                } else {
                    ratio = 0;  // start again
                    comp_clearhash(HSIZE);      // clear hash table
                    outbits = comp_output(outbits, CLEAR, n_bits);     // put CLEAR code in stream

                    boff = outbits = (outbits - 1) + ((n_bits << 3) -
                                                      ((outbits - boff -
                                                        1 +
                                                        (n_bits << 3)) %
                                                       (n_bits << 3)));

                    extcode = MAXCODE(n_bits = INIT_BITS) + 1;
                    free_ent = FIRST;
                    stcode = 1; // start code
                }
            } // end of checkpoint checking

            if (outbits >= (OBUFSIZ << 3)) {

                //buffer full, so write it out
                if (comp_writesd(OUTBUF, OBUFSIZ) != OBUFSIZ) {
                    return;
                    //                      fprintf(stderr, "\nwrite error on");
                    //                    perror("stdout");
                }
                // reset to start (ish)
                outbits -= (OBUFSIZ << 3);
                boff = -(((OBUFSIZ << 3) - boff) % (n_bits << 3));
                bytes_out += OBUFSIZ;

                // copy back remainder
                comp_cp_remainder(OUTBUF, OBUFSIZ, (outbits >> 3) + 1);

            }

            {
                int32_t i, j; // j needs to be 32bit

                i = rsize - rlop; // maximum size is 512 bytes, only gets smaller later.

                j = (int32_t) (extcode - free_ent);
                if (i > j) {
                    i = j;
                }

                j = (uint16_t)((((SIZEOFOUTBUF - 32) * (uint16_t)8) - outbits) / n_bits);
                if (i > j) {
                    i = j;
                }

                j = (int32_t) (checkpoint - bytes_in);
                if (!stcode && (i > j)) {
                    i = j;
                }

                rlop += i;
                bytes_in += i;
            }

            goto next;
          hfound:
            fcode.e.ent = comp_readcode(hp);

          next:if (rpos >= rlop)
                goto endlop;
          next2:
            //                      fcode.e.c = readinbuf(rpos++);
            {
                unsigned char result_c;

                __asm__ __volatile__(
                    "addx  %1\n"
                    "ldaa 0,x\n"
                    "staa %0\n"
                    "incw %1\n"
                    :"=m"(result_c), "=m"(rpos)
                    :"x"(inbuf)
                    :"d"
                );

                fcode.e.c = result_c;
            }

            {
                int32_t i;

//                hp = (((int32_t) (fcode.e.c)) << (BITS - 8)) ^
//                    (int32_t) (fcode.e.ent);
                /* if BITS <= 15 this only needs int16_t */
                hp = (((int16_t) (fcode.e.c)) << (BITS - 8)) ^ (int16_t) (fcode.e.ent);


                i = comp_readhash(hp);
                if (i == fcode.code) {
                    goto hfound;
                }


                if (i != -1) {
                    int16_t disp;

                    disp = (HSIZE - hp) - 1;    /* secondary hash (after G. Knott) */

                    do {
                        if ((hp -= disp) < 0)
                            hp += HSIZE;

                        i = comp_readhash(hp);

                        if (i == fcode.code) {
                            goto hfound;
                        }
                    }
                    while (i != -1);
                }
            }

            outbits = comp_output(outbits, fcode.e.ent, n_bits);

            {
                int32_t fc;
                fc = fcode.code;

                fcode.e.ent = fcode.e.c;


                if (stcode) {
                    comp_writecode(hp, free_ent++);
                    comp_writehash(hp, fc);
                }
            }

            goto next;

          endlop:if (fcode.e.ent >= FIRST && rpos < rsize)
                goto next2;

            if (rpos > rlop) {
                bytes_in += rpos - rlop;
                rlop = rpos;
            }

        }
        while (rlop < rsize); // end of do loop
    }

    if (rsize < 0) {
        //          fprintf(stderr, "\nread error on");
        //              perror("stdin");
    }

    if (bytes_in > 0)
        outbits = comp_output(outbits, fcode.e.ent, n_bits);

    if (comp_writesd(OUTBUF, (outbits + 7) >> 3) != (outbits + 7) >> 3) {
        asm("nop");
    }

    return;
}
