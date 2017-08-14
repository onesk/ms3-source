;gcc port 2007 James Murray
; $Id: hcs12xedef.inc,v 1.8 2013/01/23 20:32:26 jsmcortina Exp $

/* No copyright asserted */

;Converted from hcs12xedef.h based on 1.07 datasheet
;Until someone can find a simple way to share definitions between asm and C we have to
;define everything twice..

.equ PORTA,    0x0000
.equ pPORTA,   0x0000
.equ PORTB,    0x0001
.equ DDRA,     0x0002
.equ DDRB,     0x0003
.equ PORTC,    0x0004
.equ PORTD,    0x0005
.equ DDRC,     0x0006
.equ DDRD,     0x0007
.equ PORTE,    0x0008
.equ DDRE,     0x0009

.equ MMCCTL0,  0x000A
.equ MODE,     0x000B

.equ PUCR,     0x000C
.equ RDRIV,    0x000D

.equ EBICTL0,  0x000E
.equ EBICTL1,  0x000F

.equ GPAGE,    0x0010
.equ DIRECT,   0x0011
;reserved 0012
.equ MMCCTL1,  0x0013
;reserved 0014
.equ PPAGE,    0x0015
.equ RPAGE,    0x0016
.equ EPAGE,    0x0017

;reserved 0018
;reserved 0019
.equ PARTIDH,  0x001A
.equ PARTIDL,  0x001B

.equ ECLKCTL,  0x001C
;reserved 001D

.equ IRQCR,	 0x001E
;reserved 001F

; Debug module */
.equ DBGC1,    0x0020
.equ DBGSR,    0x0021
.equ DBGTCR,   0x0022
.equ DBGC2,    0x0023
.equ DBGTBH,   0x0024
.equ DBGTBL,   0x0025
.equ DBGCNT,   0x0026
.equ DBGSCRX,  0x0027
.equ DBGMFR,   0x0028
.equ DBGXCTL,  0x0029
.equ DBGXAH,   0x002A
.equ DBGXAM,   0x002B
.equ DBGXDH,   0x002C
.equ DBGXDL,   0x002D
.equ DBGXDHM,  0x002E
.equ DBGXDLM,  0x002F

;reserved 0030
;reserved 0031

.equ PORTK,    0x0032
.equ DDRK,     0x0033

; CRG/PLL definitions */
.equ SYNR,     0x0034
.equ REFDV,    0x0035
.equ POSTDIV,  0x0036
.equ CRGFLG,   0x0037
.equ CRGINT,   0x0038
.equ CLKSEL,   0x0039
.equ PLLCTL,   0x003A
.equ RTICTL,   0x003B
.equ COPCTL,   0x003C
;reserved 003D
;reserved 003E
.equ ARMCOP,   0x003F

; ECT Timer definitions */
.equ TIOS,     0x0040
.equ CFORC,    0x0041
.equ OC7M,     0x0042
.equ OC7D,     0x0043
.equ TCNT,     0x0044
;TCNTL
.equ TSCR1,    0x0046
.equ TTOV,     0x0047
.equ TCTL1,    0x0048
.equ TCTL2,    0x0049
.equ TCTL3,    0x004A
.equ TCTL4,    0x004B
.equ TIE,      0x004C
.equ TSCR2,    0x004D
.equ TFLG1,    0x004E
.equ TFLG2,    0x004F
.equ TC0,      0x0050
.equ TC1,      0x0052
.equ TC2,      0x0054
.equ TC3,      0x0056
.equ TC4,      0x0058
.equ TC5,      0x005A
.equ TC6,      0x005C
.equ TC7,      0x005E
.equ PACTL,    0x0060
.equ PAFLG,    0x0061
.equ MCCTL,    0x0066
.equ MCFLG,    0x0067
.equ ICPAR,    0x0068
.equ DLYCT,    0x0069
.equ ICOVW,    0x006A
.equ ICSYS,    0x006B
.equ OCPD,     0x006C
.equ TIMTST,   0x006D
.equ PTPSR,    0x006E
.equ PTMCPSR,  0x006F
.equ PBCTL,    0x0070
.equ PBFLG,    0x0071
.equ PA3H,     0x0072
.equ PA2H,     0x0073
.equ PA1H,     0x0074
.equ PA0H,     0x0075
.equ MCCNT,    0x0076
.equ TCOH,     0x0078
.equ TC1H,     0x007A
.equ TC2H,     0x007C
.equ TC3H,     0x007E

; ATD1 (ADC) definitions */
.equ ATD1CTL0,   0x0080
.equ ATD1CTL1,   0x0081
.equ ATD1CTL2,   0x0082
.equ ATD1CTL3,   0x0083
.equ ATD1CTL4,   0x0084
.equ ATD1CTL5,   0x0085
.equ ATD1STAT0,  0x0086
;reserved 0087
.equ ATD1CMPE,   0x0088
.equ ATD1STAT,   0x008A
.equ ATD1DIEN,   0x008C
.equ ATD1CMPHT,  0x008E
.equ ATD1DR0,    0x0090
.equ ATD1DR1,    0x0092
.equ ATD1DR2,    0x0094
.equ ATD1DR3,    0x0096
.equ ATD1DR4,    0x0098
.equ ATD1DR5,    0x009A
.equ ATD1DR6,    0x009C
.equ ATD1DR7,    0x009E
.equ ATD1DR8,    0x00A0
.equ ATD1DR9,    0x00A2
.equ ATD1DR10,   0x00A4
.equ ATD1DR11,   0x00A6
.equ ATD1DR12,   0x00A8
.equ ATD1DR13,   0x00AA
.equ ATD1DR14,   0x00AC
.equ ATD1DR15,   0x00AE

; IIC1
.equ IBAD,       0x00B0
.equ IBFD,       0x00B1
.equ IBCR,       0x00B2
.equ IBSR,       0x00B3
.equ IBDR,       0x00B4
;reserved 00B5
;reserved 00B6
;reserved 00B7

; SCI2 definitions */
.equ SCI2BDH,    0x00B8
.equ SCI2BDL,    0x00B9
.equ SCI2CR1,    0x00BA
.equ SCI2CR2,    0x00BB
.equ SCI2SR1,    0x00BC
.equ SCI2DRL,    0x00BF

; SCI3 definitions */
.equ SCI3BDH,    0x00C0
.equ SCI3BDL,    0x00C1
.equ SCI3CR1,    0x00C2
.equ SCI3CR2,    0x00C3
.equ SCI3SR1,    0x00C4
.equ SCI3DRL,    0x00C7

; SCI0 definitions */
.equ SCI0BDH,    0x00C8
.equ SCI0BDL,    0x00C9
.equ SCI0CR1,    0x00CA
.equ SCI0CR2,    0x00CB
.equ SCI0SR1,    0x00CC
.equ SCI0DRL,    0x00CF

; SCI1 definitions */
.equ SCI1BDH,    0x00D0
.equ SCI1BDL,    0x00D1
.equ SCI1CR1,    0x00D2
.equ SCI1CR2,    0x00D3
.equ SCI1SR1,    0x00D4
.equ SCI1DRL,    0x00D7

; SPI1  definitions */
.equ SPI0CR1,    0x00D8
.equ SPI0CR2,    0x00D9
.equ SPI0BR,     0x00DA
.equ SPI0SR,     0x00DB
.equ SPI0DR,     0x00DD

; IIC1
.equ IBAD1,      0x00E0
.equ IBFD1,      0x00E1
.equ IBCR1,      0x00E2
.equ IBSR1,      0x00E3
.equ IBDR1,      0x00E4
;reserved 00E5
;reserved 00E6
;reserved 00E7

;reserved 00E8-EF

; SPI1  definitions */
.equ SPI1CR1,    0x00F0
.equ SPI1CR2,    0x00F1
.equ SPI1BR,     0x00F2
.equ SPI1SR,     0x00F3
.equ SPI1DR,     0x00F5
.equ SPI1DRH,    0x00F4
.equ SPI1DRL,    0x00F5

; SPI2  definitions */
.equ SPI2CR1,    0x00F8
.equ SPI2CR2,    0x00F9
.equ SPI2BR,     0x00FA
.equ SPI2SR,     0x00FB
.equ SPI2DR,     0x00FD

; Flash regs */
.equ FCLKDIV,    0x0100
.equ FSEC,       0x0101
.equ FCCOBIX,    0x0102
.equ FECCRIX,    0x0103
.equ FCNFG,      0x0104
.equ FERCNFG,    0x0105
.equ FSTAT,      0x0106
.equ CCIF,       0b10000000
.equ ACCERR,     0b00100000
.equ PVIOL,      0b00010000
.equ FERSTAT,    0x0107
.equ FPROT,      0x0108
.equ EPROT,      0x0109
.equ FCCOBHI,    0x010A
.equ FCCOBLO,    0x010B
.equ ETAGHI,     0x010C
.equ ETAGLO,     0x010D
.equ FECCRHI,    0x010E
.equ FECCRLO,    0x010F
.equ FOPT,       0x0110
;reserved 0111
;reserved 0112
;reserved 0113

; MPU */
.equ MPUFLG,     0x0114
.equ MPUASTAT0,  0x0115
.equ MPUASTAT1,  0x0116
.equ MPUASTAT2,  0x0117
;reserved 0118
.equ MPUSEL,    0x0119
.equ MPUDESC0,  0x011A
.equ MPUDESC1,  0x011B
.equ MPUDESC2,  0x011C
.equ MPUDESC3,  0x011D
.equ MPUDESC4,  0x011E
.equ MPUDESC5,  0x011F

; S12X INT */
;reserved 0120
.equ IVBR,     0x0121
;reserved 0122
;reserved 0123
;reserved 0124
;reserved 0125
.equ INT_XGPRIO,  0x0126
.equ INT_CFADDR,  0x0127
.equ INT_CFDATA0, 0x0128
.equ INT_CFDATA1, 0x0129
.equ INT_CFDATA2, 0x012A
.equ INT_CFDATA3, 0x012B
.equ INT_CFDATA4, 0x012C
.equ INT_CFDATA5, 0x012D
.equ INT_CFDATA6, 0x012E
.equ INT_CFDATA7, 0x012F

; SCI4 definitions */
.equ SCI4BDH,    0x0130
.equ SCI4BDL,    0x0131
.equ SCI4CR1,    0x0132
.equ SCI4CR2,    0x0133
.equ SCI4SR1,    0x0134
.equ SCI4DRL,    0x0137

; SCI5 definitions */
.equ SCI5BDH,    0x0138
.equ SCI5BDL,    0x0139
.equ SCI5CR1,    0x013A
.equ SCI5CR2,    0x013B
.equ SCI5SR1,    0x013C
.equ SCI5DRL,    0x01CF

 ; CAN0 section */
  .equ CAN0_BASE, 0x0140

  .equ CAN0CTL0,		CAN0_BASE+0x0
  .equ CAN0CTL1,		CAN0_BASE+0x1
  .equ CAN0BTR0,		CAN0_BASE+0x2
  .equ CAN0BTR1,		CAN0_BASE+0x3
  .equ CAN0RFLG,		CAN0_BASE+0x4
  .equ CAN0RIER,		CAN0_BASE+0x5
  .equ CAN0TFLG,		CAN0_BASE+0x6
  .equ CAN0TIER,		CAN0_BASE+0x7
  .equ CAN0TARQ,		CAN0_BASE+0x8
  .equ CAN0TAAK,		CAN0_BASE+0x9
  .equ CAN0TBSEL,		CAN0_BASE+0xA
  .equ CAN0IDAC,		CAN0_BASE+0xB
  .equ CAN0RXERR,		CAN0_BASE+0xE
  .equ CAN0TXERR,		CAN0_BASE+0xF

  .equ CAN0IDAR0,		CAN0_BASE+0x10
  .equ CAN0IDAR1,		CAN0_BASE+0x11
  .equ CAN0IDAR2,		CAN0_BASE+0x12
  .equ CAN0IDAR3,		CAN0_BASE+0x13
  .equ CAN0IDAR4,		CAN0_BASE+0x18
  .equ CAN0IDAR5,		CAN0_BASE+0x19
  .equ CAN0IDAR6,		CAN0_BASE+0x1A
  .equ CAN0IDAR7,		CAN0_BASE+0x1B

  .equ CAN0IDMR0,		CAN0_BASE+0x14
  .equ CAN0IDMR1,		CAN0_BASE+0x15
  .equ CAN0IDMR2,		CAN0_BASE+0x16
  .equ CAN0IDMR3,		CAN0_BASE+0x17
  .equ CAN0IDMR4,		CAN0_BASE+0x1C
  .equ CAN0IDMR5,		CAN0_BASE+0x1D
  .equ CAN0IDMR6,		CAN0_BASE+0x1E
  .equ CAN0IDMR7,		CAN0_BASE+0x1F

  .equ CAN0_RB_IDR0,	CAN0_BASE+0x20
  .equ CAN0_RB_IDR1,	CAN0_BASE+0x21
  .equ CAN0_RB_IDR2,	CAN0_BASE+0x22
  .equ CAN0_RB_IDR3,	CAN0_BASE+0x23

  .equ CAN0_RB_DSR0,	CAN0_BASE+0x24
  .equ CAN0_RB_DSR1,	CAN0_BASE+0x25
  .equ CAN0_RB_DSR2,	CAN0_BASE+0x26
  .equ CAN0_RB_DSR3,	CAN0_BASE+0x27
  .equ CAN0_RB_DSR4,	CAN0_BASE+0x28
  .equ CAN0_RB_DSR5,	CAN0_BASE+0x29
  .equ CAN0_RB_DSR6,	CAN0_BASE+0x2A
  .equ CAN0_RB_DSR7,	CAN0_BASE+0x2B

  .equ CAN0_RB_DLR,	    CAN0_BASE+0x2C

  .equ CAN0_RB_TBPR,	CAN0_BASE+0x2D

  .equ CAN0_TB0_IDR0,	CAN0_BASE+0x30
  .equ CAN0_TB0_IDR1,	CAN0_BASE+0x31
  .equ CAN0_TB0_IDR2,	CAN0_BASE+0x32
  .equ CAN0_TB0_IDR3,	CAN0_BASE+0x33

  .equ CAN0_TB0_DSR0,	CAN0_BASE+0x34
  .equ CAN0_TB0_DSR1,	CAN0_BASE+0x35
  .equ CAN0_TB0_DSR2,	CAN0_BASE+0x36
  .equ CAN0_TB0_DSR3,	CAN0_BASE+0x37
  .equ CAN0_TB0_DSR4,	CAN0_BASE+0x38
  .equ CAN0_TB0_DSR5,	CAN0_BASE+0x39
  .equ CAN0_TB0_DSR6,	CAN0_BASE+0x3A
  .equ CAN0_TB0_DSR7,	CAN0_BASE+0x3B

  .equ CAN0_TB0_DLR,	CAN0_BASE+0x3C

  .equ CAN0_TB0_TBPR,	CAN0_BASE+0x3D

 ; CAN1 section */
  .equ CAN1_BASE, 0x0180

  .equ CAN1CTL0,		CAN1_BASE+0x0
  .equ CAN1CTL1,		CAN1_BASE+0x1
  .equ CAN1BTR0,		CAN1_BASE+0x2
  .equ CAN1BTR1,		CAN1_BASE+0x3
  .equ CAN1RFLG,		CAN1_BASE+0x4
  .equ CAN1RIER,		CAN1_BASE+0x5
  .equ CAN1TFLG,		CAN1_BASE+0x6
  .equ CAN1TIER,		CAN1_BASE+0x7
  .equ CAN1TARQ,		CAN1_BASE+0x8
  .equ CAN1TAAK,		CAN1_BASE+0x9
  .equ CAN1TBSEL,		CAN1_BASE+0xA
  .equ CAN1IDAC,		CAN1_BASE+0xB
  .equ CAN1RXERR,		CAN1_BASE+0xE
  .equ CAN1TXERR,		CAN1_BASE+0xF

  .equ CAN1IDAR0,		CAN1_BASE+0x10
  .equ CAN1IDAR1,		CAN1_BASE+0x11
  .equ CAN1IDAR2,		CAN1_BASE+0x12
  .equ CAN1IDAR3,		CAN1_BASE+0x13
  .equ CAN1IDAR4,		CAN1_BASE+0x18
  .equ CAN1IDAR5,		CAN1_BASE+0x19
  .equ CAN1IDAR6,		CAN1_BASE+0x1A
  .equ CAN1IDAR7,		CAN1_BASE+0x1B

  .equ CAN1IDMR0,		CAN1_BASE+0x14
  .equ CAN1IDMR1,		CAN1_BASE+0x15
  .equ CAN1IDMR2,		CAN1_BASE+0x16
  .equ CAN1IDMR3,		CAN1_BASE+0x17
  .equ CAN1IDMR4,		CAN1_BASE+0x1C
  .equ CAN1IDMR5,		CAN1_BASE+0x1D
  .equ CAN1IDMR6,		CAN1_BASE+0x1E
  .equ CAN1IDMR7,		CAN1_BASE+0x1F

  .equ CAN1_RB_IDR0,	CAN1_BASE+0x20
  .equ CAN1_RB_IDR1,	CAN1_BASE+0x21
  .equ CAN1_RB_IDR2,	CAN1_BASE+0x22
  .equ CAN1_RB_IDR3,	CAN1_BASE+0x23

  .equ CAN1_RB_DSR0,	CAN1_BASE+0x24
  .equ CAN1_RB_DSR1,	CAN1_BASE+0x25
  .equ CAN1_RB_DSR2,	CAN1_BASE+0x26
  .equ CAN1_RB_DSR3,	CAN1_BASE+0x27
  .equ CAN1_RB_DSR4,	CAN1_BASE+0x28
  .equ CAN1_RB_DSR5,	CAN1_BASE+0x29
  .equ CAN1_RB_DSR6,	CAN1_BASE+0x2A
  .equ CAN1_RB_DSR7,	CAN1_BASE+0x2B

  .equ CAN1_RB_DLR,	CAN1_BASE+0x2C

  .equ CAN1_RB_TBPR,	CAN1_BASE+0x2D

  .equ CAN1_TB0_IDR0,	CAN1_BASE+0x30
  .equ CAN1_TB0_IDR1,	CAN1_BASE+0x31
  .equ CAN1_TB0_IDR2,	CAN1_BASE+0x32
  .equ CAN1_TB0_IDR3,	CAN1_BASE+0x33

  .equ CAN1_TB0_DSR0,	CAN1_BASE+0x34
  .equ CAN1_TB0_DSR1,	CAN1_BASE+0x35
  .equ CAN1_TB0_DSR2,	CAN1_BASE+0x36
  .equ CAN1_TB0_DSR3,	CAN1_BASE+0x37
  .equ CAN1_TB0_DSR4,	CAN1_BASE+0x38
  .equ CAN1_TB0_DSR5,	CAN1_BASE+0x39
  .equ CAN1_TB0_DSR6,	CAN1_BASE+0x3A
  .equ CAN1_TB0_DSR7,	CAN1_BASE+0x3B

  .equ CAN1_TB0_DLR,	CAN1_BASE+0x3C

  .equ CAN1_TB0_TBPR,	CAN1_BASE+0x3D

; Also CAN2 + CAN3 0x1C0-0x23F

.equ PORTT,    0x0240
.equ PTT,      0x0240
.equ pPTT,     0x0240
.equ PTIT,     0x0241
.equ DDRT,     0x0242
.equ RDRT,     0x0243
.equ PERT,     0x0244
.equ PPST,     0x0245
;reserved 0246
;reserved 0247

.equ PTS,      0x0248
.equ PTIS,     0x0249
.equ DDRS,     0x024A
.equ RDRS,     0x024B
.equ PERS,     0x024C
.equ PPSS,     0x024D
.equ WOMS,     0x024E
;reserved 024F

.equ PORTM,    0x0250
.equ PTM,      0x0250
.equ pPTM,     0x0250
.equ DDRM,     0x0252
.equ RDRM,     0x0253
.equ PERM,     0x0254
.equ PPSM,     0x0255
.equ WOMM,     0x0256
.equ MODRR,    0x0257

.equ PTP,      0x0258
.equ PTIP,     0x0259
.equ DDRP,     0x025A
.equ RDRP,     0x025B
.equ PERP,     0x025C
.equ PPSP,     0x025D
.equ PIEP,     0x025E
.equ PIFP,     0x025F

.equ PTH,      0x0260
.equ PTIH,     0x0261
.equ DDRH,     0x0262
.equ RDRH,     0x0263
.equ PERH,     0x0264
.equ PPSH,     0x0265
.equ PIEH,     0x0266
.equ PIFH,     0x0267

.equ PTJ,      0x0268
.equ PTIJ,     0x0269
.equ DDRJ,     0x026A
.equ RDRJ,     0x026B
.equ PERJ,     0x026C
.equ PPSJ,     0x026D
.equ PIEJ,     0x026E
.equ PIFJ,     0x026F

;offical names
.equ PT0AD0,   0x0270
.equ PT1AD0,   0x0271
.equ DDR0AD0,  0x0272
.equ DDR1AD0,  0x0273
.equ RDR0AD0,  0x0274
.equ RDR1AD0,  0x0275
.equ PER0AD0,  0x0276
.equ PER1AD0,  0x0277

.equ PT0AD1,   0x0278
.equ PT1AD1,   0x0279
.equ DDR0AD1,  0x027A
.equ DDR1AD1,  0x027B
.equ RDR0AD1,  0x027C
.equ RDR1AD1,  0x027D
.equ PER0AD1,  0x027E
.equ PER1AD1,  0x027F

;more logical
.equ PTAD0H,   0x0270
.equ PTAD0L,   0x0271
.equ DDRAD0H,  0x0272
.equ DDRAD0L,  0x0273
.equ RDRAD0H,  0x0274
.equ RDRAD0L,  0x0275
.equ PERAD0H,  0x0276
.equ PERAD0L,  0x0277

.equ PTAD1H,   0x0278
.equ PTAD1L,   0x0279
.equ DDRAD1H,  0x027A
.equ DDRAD1L,  0x027B
.equ RDRAD1H,  0x027C
.equ RDRAD1L,  0x027D
.equ PERAD1H,  0x027E
.equ PERAD1L,  0x027F

;0x280-2BF CAN4

; ATD1 (ADC) definitions */
.equ ATD0CTL0,   0x02C0
.equ ATD0CTL1,   0x02C1
.equ ATD0CTL2,   0x02C2
.equ ATD0CTL3,   0x02C3
.equ ATD0CTL4,   0x02C4
.equ ATD0CTL5,   0x02C5
.equ ATD0STAT0,  0x02C6
;reserved 02C7
.equ ATD0CMPE,   0x02C8
.equ ATD0STAT,   0x02CA
.equ ATD0DIEN,   0x02CC
.equ ATD0CMPHT,  0x02CE
.equ ATD0DR0,    0x02D0
.equ ATD0DR1,    0x02D2
.equ ATD0DR2,    0x02D4
.equ ATD0DR3,    0x02D6
.equ ATD0DR4,    0x02D8
.equ ATD0DR5,    0x02DA
.equ ATD0DR6,    0x02DC
.equ ATD0DR7,    0x02DE
.equ ATD0DR8,    0x02E0
.equ ATD0DR9,    0x02E2
.equ ATD0DR10,   0x02E4
.equ ATD0DR11,   0x02E6
.equ ATD0DR12,   0x02E8
.equ ATD0DR13,   0x02EA
.equ ATD0DR14,   0x02EC
.equ ATD0DR15,   0x02EE

; voltage reg */
.equ VREGHTVL,   0x02F0
.equ VREGCTRL,   0x02F1
.equ VREGAPICL,  0x02F2
.equ VREGAPITR,  0x02F3
.equ VREGAPIR,  0x02F4
;reserved 02F6
;reserved 02F7
;reserved 02F8-2FF

; PWM definitions */
.equ PWME,        0x0300
.equ PWMPOL,      0x0301
.equ PWMCLK,      0x0302
.equ PWMPRCLK,    0x0303
.equ PWMCAE,      0x0304
.equ PWMCTL,      0x0305
;reserved 0306
.equ PWMPRSC,     0x0307
.equ PWMPRSCLA,   0x0308
.equ PWMPRSCLB,   0x0309
.equ PWMPRSCNTA,  0x030A
.equ PWMPRSCNTB,  0x030B
.equ PWMCNT0,     0x030C
.equ PWMCNT1,     0x030D
.equ PWMCNT2,     0x030E
.equ PWMCNT3,     0x030F
.equ PWMCNT4,     0x0310
.equ PWMCNT5,     0x0311
.equ PWMCNT6,     0x0312
.equ PWMCNT7,     0x0313
.equ PWMPER0,     0x0314
.equ PWMPER1,     0x0315
.equ PWMPER2,     0x0316
.equ PWMPER3,     0x0317
.equ PWMPER4,     0x0318
.equ PWMPER5,     0x0319
.equ PWMPER6,     0x031A
.equ PWMPER7,     0x031B
.equ PWMDTY0,     0x031C
.equ PWMDTY1,     0x031D
.equ PWMDTY2,     0x031E
.equ PWMDTY3,     0x031F
.equ PWMDTY4,     0x0320
.equ PWMDTY5,     0x0321
.equ PWMDTY6,     0x0322
.equ PWMDTY7,     0x0323
.equ PWMSDN,      0x0324
;reserved 0325-32F

; 0x330-337 SCI6
; 0x338-33F SCI7

; PIT timer */
.equ PITCFLMT,    0x0340
.equ PITFLT,      0x0341
.equ PITCE,       0x0342
.equ PITMUX,      0x0343
.equ PITINTE,     0x0344
.equ PITTF,       0x0345
.equ PITMTLD0,    0x0346
.equ PITMTLD1,    0x0347
.equ PITLD0,     0x0348
.equ PITCNT0,    0x034A
.equ PITLD1,     0x034C
.equ PITCNT1,    0x034E
.equ PITLD2,     0x0350
.equ PITCNT2,    0x0352
.equ PITLD3,     0x0354
.equ PITCNT3,    0x0356
.equ PITLD4,     0x0358
.equ PITCNT4,    0x035A
.equ PITLD5,     0x035C
.equ PITCNT5,    0x035E
.equ PITLD6,     0x0360
.equ PITCNT6,    0x0362
.equ PITLD7,     0x0364
.equ PITCNT7,    0x0366

; PIM map 6
.equ PTR,      0x0368
.equ PTIR,     0x0369
.equ DDRR,     0x036A
.equ RDRR,     0x036B
.equ PERR,     0x036C
.equ PPSR,     0x036D
;reserved 036E
.equ PTRRR,    0x036F

.equ PTL,      0x0370
.equ PTIL,     0x0371
.equ DDRL,     0x0372
.equ RDRL,     0x0373
.equ PERL,     0x0374
.equ PPSL,     0x0375
.equ WOML,     0x0376
.equ PTLRR,    0x0377

.equ PTF,      0x0378
.equ PTIF,     0x0379
.equ DDRF,     0x037A
.equ RDRF,     0x037B
.equ PERF,     0x037C
.equ PPSF,     0x037D
;reserved 037E
.equ PTFRR,    0x037F

; XGATE */
.equ XGMCTL,   0x0380
.equ XGCHID,    0x0382
.equ XGCHPL,    0x0383
;reserved 0384
.equ XGISPSEL, 0x0385
.equ XGVBR,    0x0386
.equ XGIF78,   0x0388
.equ XGIF77,   0x0389
.equ XGIF6F,   0x038A
.equ XGIF67,   0x038B
.equ XGIF5F,   0x038C
.equ XGIF57,   0x038D
.equ XGIF4F,   0x038E
.equ XGIF47,   0x038F
.equ XGIF3F,   0x0390
.equ XGIF37,   0x0391
.equ XGIF2F,   0x0392
.equ XGIF27,   0x0393
.equ XGIF1F,   0x0394
.equ XGIF17,   0x0395
.equ XGIF0F,   0x0396
.equ XGIF07,   0x0397
.equ XGSWTM,   0x0398
.equ XGSWT,    0x0399
.equ XGSEMM,   0x039A
.equ XGSEM,    0x039B
;reserved 039C
.equ XGCCR,    0x039D
.equ XGPC,    0x039E
;reserved 03A0
;reserved 03A1
.equ XGR0,    0x03A2
.equ XGR1,    0x03A4
.equ XGR2,    0x03A6
.equ XGR3,    0x03A8
.equ XGR4,    0x03AA
.equ XGR5,    0x03AC
.equ XGR6,    0x03AE
.equ XGR7,    0x03B0
;reserved 03B0-3CF

; TIM Timer definitions */
.equ TIMTIOS,     0x03D0
.equ TIMCFORC,    0x03D1
.equ TIMOC7M,     0x03D2
.equ TIMOC7D,     0x03D3
.equ TIMTCNT,     0x03D4
;TCNTL
.equ TIMTSCR1,    0x03D6
.equ TIMTTOV,     0x03D7
.equ TIMTCTL1,    0x03D8
.equ TIMTCTL2,    0x03D9
.equ TIMTCTL3,    0x03DA
.equ TIMTCTL4,    0x03DB
.equ TIMTIE,      0x03DC
.equ TIMTSCR2,    0x03DD
.equ TIMTFLG1,    0x03DE
.equ TIMTFLG2,    0x03DF
.equ TIMTC0,      0x03E0
.equ TIMTC1,      0x03E2
.equ TIMTC2,      0x03E4
.equ TIMTC3,      0x03E6
.equ TIMTC4,      0x03E8
.equ TIMTC5,      0x03EA
.equ TIMTC6,      0x03EC
.equ TIMTC7,      0x03EE
.equ TIMPACTL,    0x03F0
.equ TIMPAFLG,    0x03F1
.equ TIMPACNT,    0x03F2
;reserved 03F4-3FB
.equ TIMOCPD,     0x03FC
;reserved 03FD
.equ TIMPTPSR,    0x03FE
;reserved 03FF
;reserved 0400-7FF
