; crc32.s
;*********************************************************************
; $Id: crc32.s,v 1.11 2013/02/18 01:26:30 jsmcortina Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; code derived from public domain crc32.c
; I release this implementation back to the public domain. James Murray 2009.
;
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *

;g_crc32buf used for taking CRC of globally addressed data


.sect .textfb
.globl crc32buf
.globl crc32byte
.globl g_crc32buf
;*********************************************************************

; unsigned long crc32buf(unsigned long crcin, *buf, unsigned int bufsize)
crc32buf:
; on entry input crc is in X:D
; buffer is on first word on stack
; bufsize is second word on stack

    leas	-8,SP    ;   /-- usage in this function
    movw	0x2808, 0x2,-SP  ; crc32H
    movw	0x280a, 0x2,-SP  ; crc32L
    movw	0x280c, 0x2,-SP  ; bufptr
    movw	0x280e, 0x2,-SP  ; not used
    movw	0x2810, 0x2,-SP  ; buf size
    ldy	    0x17,SP ; off by one due to "call"
    sty	    0x2810

;    /** accumulate crc32 for buffer **/
;    crc32 = inCrc32 ^ 0xFFFFFFFF;
    comb
    coma
    comx
    std	 0x280a
    stx	 0x2808

;    byteBuf = (unsigned char*) buf;
    ldx	    0x15,SP
    stx	    0x280c

;    for (i=0; i < bufLen; i++) {
    clrw    0x2802  ; i
    tbeq	Y, .LM7

.LM5:
;        crc32 = (crc32 >> 8) ^ crcTable[ (crc32 & 0xff) ^ byteBuf[i] ];
    ldd     0x2809      ; crc32>>8
    std     0x10, SP    ; save the 24bits into the 32bit temp space
    clra
    ldab    0x2808
    std     0x0e, SP

    ldd	    0x280c ; buf
    ldx	    0x2802 ; [i]
    leay	D,X
    clra
    ldab    0x280b
    eorb	0x0,Y
    ldy	    #4
    emul
    tfr	    D,Y         ; table pointer of u32

    ldx     0xe,SP
    eorx    crcTable, Y
    stx	    0x2808

    ldx     0x10,SP
    eorx    crcTable+2, Y
    stx	    0x280a

;    /** accumulate crc32 for buffer **/
    ldx	    0x2802
    inx
    stx	    0x2802
    cpx     0x2810
    bcs	    .LM5

.LM7:
    ldd	    0x280a
    ldx	    0x2808
    comb
    coma
    comx

    movw	0x2,SP+,  0x2810
    movw	0x2,SP+,  0x280e
    movw	0x2,SP+,  0x280c
    movw	0x2,SP+,  0x280a
    movw	0x2,SP+,  0x2808
    leas	0x8,SP
    rtc

;*********************************************************************
crc32byte:
; This still needs significant optimisation
; reads from and stores back to sd_crc
; Caution! Ensure only one instance of this is running.
; Intended to be used from isr_sci for SD readback only.
; byte is in B

    pshb

;    /** accumulate crc32 for buffer **/
;    crc32 = inCrc32 ^ 0xFFFFFFFF;
    ldy     sd_crc+2    ; save low byte
    ldx     sd_crc+1
    comx
    stx	 sd_crc+2
    clra
    ldab     sd_crc
    comb
    std     sd_crc

;        crc32 = ((inCrc32 ^ 0xFFFFFFFF) >> 8) ^ crcTable[ (crc32 & 0xff) ^ byteBuf[i] ];
;        crc32 = crc32 ^ 0xffffffff
    tfr     y,d         ; retrieve low byte
    clra
    comb
    eorb	1,sp+       ; inbyte
    ldy	    #4
    emul
    tfr	    D,Y         ; table pointer of u32

    ldx     sd_crc
    eorx    crcTable, Y
    comx
    stx	    sd_crc

    ldx     sd_crc+2
    eorx    crcTable+2, Y
    comx
    stx	    sd_crc+2

    rtc

; unsigned long g_crc32buf(unsigned long crcin, *buf, unsigned int bufsize)
g_crc32buf:
; on entry input crc is in X:D
; buffer is on first word on stack - assumes global address with GPAGE set before calling
; bufsize is second word on stack

; Use stack space instead of softregs for slightly smaller cycle count
    leas	-4,SP    ;   /-- usage in this function
;   new     0-1,sp = crc32H
;   new     2-3,sp = crc32L
;   return page+address 
;           7-8,sp = bufptr
;           9-10,sp = buf size, used as downcounter


;    /** accumulate crc32 for buffer **/
;    crc32 = inCrc32 ^ 0xFFFFFFFF;
    comb
    coma
    comx
    std	 2,sp
    stx	 0,sp

    ldaa    0,sp

.gLM5:
;        crc32 = (crc32 >> 8) ^ crcTable[ (crc32 & 0xff) ^ byteBuf[i] ];
    tab   ; this is 0,sp from end of loop D
    clra
;    ldab    0,sp

    tfr     d,x ; faster than stack -- don't touch X until ... !X!

    ldy	    7,sp ; [i]
    gldab   0x0,Y   ; globalised
    eorb    3,sp
    lsld
    lsld
    tfr	    D,Y         ; table pointer of u32
    leay    crcTable, Y

    ; --- here's our X  !X!
    eorx    2, y+
    tfr     x,d ; store in a mo

    ldx     1,sp  ; do this now instead of load/store/load
    eorx    0, y
    stx	    2,sp
    std     0,sp ; store that X from before

    /** accumulate crc32 for buffer **/
    incw    7,sp
    decw    9,sp
    bne     .gLM5

    ldd	    2,sp
    ldx	    0,sp
    comb
    coma
    comx

    leas	0x4,SP
    rtc


crcTable:
   fqb 0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3 ; 00
   fqb 0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91 ; 08
   fqb 0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7 ; 10
   fqb 0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5 ; 18
   fqb 0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B ; 20
   fqb 0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59 ; 28
   fqb 0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F ; 30
   fqb 0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D ; 38
   fqb 0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433 ; 40
   fqb 0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01 ; 48
   fqb 0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457 ; 50
   fqb 0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65 ; 58
   fqb 0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB ; 60
   fqb 0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9 ; 68
   fqb 0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F ; 70
   fqb 0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD ; 78
   fqb 0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683 ; 80
   fqb 0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1 ; 88
   fqb 0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7 ; 90
   fqb 0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5 ; 98
   fqb 0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B ; a0
   fqb 0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79 ; a8
   fqb 0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F ; b0
   fqb 0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D ; b8
   fqb 0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713 ; c0
   fqb 0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21 ; c8
   fqb 0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777 ; d0
   fqb 0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45 ; d8
   fqb 0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB ; e0
   fqb 0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9 ; e8
   fqb 0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF ; f0
   fqb 0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D ; f8

