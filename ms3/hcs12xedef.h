/* $Id: hcs12xedef.h,v 1.14 2012/08/20 15:04:01 jsmcortina Exp $ */
/* redefined for XEP100 in line with 1.07 datasheet
 * Now in numerical instead of functional order */

/* No copyright asserted */

#define PORTA    (*((volatile unsigned char*)(0x0000)))
#define pPTA   ((volatile unsigned char*)(0x0000))
#define PORTB    (*((volatile unsigned char*)(0x0001)))
#define pPTB   ((volatile unsigned char*)(0x0001))
#define DDRA     (*((volatile unsigned char*)(0x0002)))
#define DDRB     (*((volatile unsigned char*)(0x0003)))
#define PORTC    (*((volatile unsigned char*)(0x0004)))
#define PORTD    (*((volatile unsigned char*)(0x0005)))
#define DDRC     (*((volatile unsigned char*)(0x0006)))
#define DDRD     (*((volatile unsigned char*)(0x0007)))
#define PORTE    (*((volatile unsigned char*)(0x0008)))
#define pPTE     ((volatile unsigned char*)(0x0008))
#define DDRE     (*((volatile unsigned char*)(0x0009)))

#define MMCCTL0  (*((volatile unsigned char*)(0x000A)))
#define MODE     (*((volatile unsigned char*)(0x000B)))

#define PUCR     (*((volatile unsigned char*)(0x000C)))
#define RDRIV    (*((volatile unsigned char*)(0x000D)))

#define EBICTL0  (*((volatile unsigned char*)(0x000E)))
#define EBICTL1  (*((volatile unsigned char*)(0x000F)))

#define GPAGE    (*((volatile unsigned char*)(0x0010)))
#define DIRECT   (*((volatile unsigned char*)(0x0011)))
//reserved 0012
#define MMCCTL1  (*((volatile unsigned char*)(0x0013)))
//reserved 0014
#define PPAGE    (*((volatile unsigned char*)(0x0015)))
#define RPAGE    (*((volatile unsigned char*)(0x0016)))
#define EPAGE    (*((volatile unsigned char*)(0x0017)))

//reserved 0018
//reserved 0019
#define PARTIDH  (*((volatile unsigned char*)(0x001A)))
#define PARTIDL  (*((volatile unsigned char*)(0x001B)))
#define ECLKCTL  (*((volatile unsigned char*)(0x001C)))
//reserved 001D
#define IRQCR	 (*((volatile unsigned char*)(0x001E)))
//reserved 001F

/* Debug module */
#define DBGC1    (*((volatile unsigned char*)(0x0020)))
#define DBGSR    (*((volatile unsigned char*)(0x0021)))
#define DBGTCR   (*((volatile unsigned char*)(0x0022)))
#define DBGC2    (*((volatile unsigned char*)(0x0023)))
#define DBGTBH   (*((volatile unsigned char*)(0x0024)))
#define DBGTBL   (*((volatile unsigned char*)(0x0025)))
#define DBGCNT   (*((volatile unsigned char*)(0x0026)))
#define DBGSCRX  (*((volatile unsigned char*)(0x0027)))
#define DBGMFR   (*((volatile unsigned char*)(0x0028)))
#define DBGXCTL  (*((volatile unsigned char*)(0x0029)))
#define DBGXAH   (*((volatile unsigned char*)(0x002A)))
#define DBGXAM   (*((volatile unsigned char*)(0x002B)))
#define DBGXDH   (*((volatile unsigned char*)(0x002C)))
#define DBGXDL   (*((volatile unsigned char*)(0x002D)))
#define DBGXDHM  (*((volatile unsigned char*)(0x002E)))
#define DBGXDLM  (*((volatile unsigned char*)(0x002F)))

//reserved 0030
//reserved 0031

#define PORTK    (*((volatile unsigned char*)(0x0032)))
#define pPTK     ((volatile unsigned char*)(0x0032))
#define DDRK     (*((volatile unsigned char*)(0x0033)))

/* CRG/PLL definitions */
#define SYNR     (*((volatile unsigned char*)(0x0034)))
#define REFDV    (*((volatile unsigned char*)(0x0035)))
#define POSTDIV  (*((volatile unsigned char*)(0x0036)))
#define CRGFLG   (*((volatile unsigned char*)(0x0037)))
#define CRGINT   (*((volatile unsigned char*)(0x0038)))
#define CLKSEL   (*((volatile unsigned char*)(0x0039)))
#define PLLCTL   (*((volatile unsigned char*)(0x003A)))
#define RTICTL   (*((volatile unsigned char*)(0x003B)))
#define COPCTL   (*((volatile unsigned char*)(0x003C)))
//reserved 003D
//reserved 003E
#define ARMCOP   (*((volatile unsigned char*)(0x003F)))

/* ECT Timer definitions */
#define TIOS     (*((volatile unsigned char*)(0x0040)))
#define CFORC    (*((volatile unsigned char*)(0x0041)))
#define OC7M     (*((volatile unsigned char*)(0x0042)))
#define OC7D     (*((volatile unsigned char*)(0x0043)))
#define TCNT     (*((volatile unsigned short*)(0x0044)))
//TCNTL
#define TSCR1    (*((volatile unsigned char*)(0x0046)))
#define TTOV     (*((volatile unsigned char*)(0x0047)))
#define TCTL1    (*((volatile unsigned char*)(0x0048)))
#define TCTL2    (*((volatile unsigned char*)(0x0049)))
#define TCTL3    (*((volatile unsigned char*)(0x004A)))
#define TCTL4    (*((volatile unsigned char*)(0x004B)))
#define TIE      (*((volatile unsigned char*)(0x004C)))
#define TSCR2    (*((volatile unsigned char*)(0x004D)))
#define TFLG1    (*((volatile unsigned char*)(0x004E)))
#define TFLG2    (*((volatile unsigned char*)(0x004F)))
#define TC0      (*((volatile unsigned short*)(0x0050)))
#define TC1      (*((volatile unsigned short*)(0x0052)))
#define TC2      (*((volatile unsigned short*)(0x0054)))
#define TC3      (*((volatile unsigned short*)(0x0056)))
#define TC4      (*((volatile unsigned short*)(0x0058)))
#define TC5      (*((volatile unsigned short*)(0x005A)))
#define TC6      (*((volatile unsigned short*)(0x005C)))
#define TC7      (*((volatile unsigned short*)(0x005E)))
#define PACTL    (*((volatile unsigned char*)(0x0060)))
#define PAFLG    (*((volatile unsigned char*)(0x0061)))
#define MCCTL    (*((volatile unsigned char*)(0x0066)))
#define MCFLG    (*((volatile unsigned char*)(0x0067)))
#define ICPAR    (*((volatile unsigned char*)(0x0068)))
#define DLYCT    (*((volatile unsigned char*)(0x0069)))
#define ICOVW    (*((volatile unsigned char*)(0x006A)))
#define ICSYS    (*((volatile unsigned char*)(0x006B)))
#define OCPD     (*((volatile unsigned char*)(0x006C)))
#define TIMTST   (*((volatile unsigned char*)(0x006D)))
#define PTPSR    (*((volatile unsigned char*)(0x006E)))
#define PTMCPSR  (*((volatile unsigned char*)(0x006F)))
#define PBCTL    (*((volatile unsigned char*)(0x0070)))
#define PBFLG    (*((volatile unsigned char*)(0x0071)))
#define PA3H     (*((volatile unsigned char*)(0x0072)))
#define PA2H     (*((volatile unsigned char*)(0x0073)))
#define PA1H     (*((volatile unsigned char*)(0x0074)))
#define PA0H     (*((volatile unsigned char*)(0x0075)))
#define MCCNT    (*((volatile unsigned short*)(0x0076)))
#define TCOH     (*((volatile unsigned short*)(0x0078)))
#define TC1H     (*((volatile unsigned short*)(0x007A)))
#define TC2H     (*((volatile unsigned short*)(0x007C)))
#define TC3H     (*((volatile unsigned short*)(0x007E)))

/* ATD1 (ADC) definitions */
#define ATD1CTL0   (*((volatile unsigned char*)(0x0080)))
#define ATD1CTL1   (*((volatile unsigned char*)(0x0081)))
#define ATD1CTL2   (*((volatile unsigned char*)(0x0082)))
#define ATD1CTL3   (*((volatile unsigned char*)(0x0083)))
#define ATD1CTL4   (*((volatile unsigned char*)(0x0084)))
#define ATD1CTL5   (*((volatile unsigned char*)(0x0085)))
#define ATD1STAT0  (*((volatile unsigned char*)(0x0086)))
//reserved 0087
#define ATD1CMPE   (*((volatile unsigned short*)(0x0088)))
#define ATD1STAT2  (*((volatile unsigned short*)(0x008A)))
#define ATD1DIEN   (*((volatile unsigned short*)(0x008C)))
#define ATD1DIENH  (*((volatile unsigned char*)(0x008C)))
#define ATD1DIENL  (*((volatile unsigned char*)(0x008D)))
#define ATD1CMPHT  (*((volatile unsigned short*)(0x008E)))
#define ATD1DR0    (*((volatile unsigned short*)(0x0090)))
#define ATD1DR1    (*((volatile unsigned short*)(0x0092)))
#define ATD1DR2    (*((volatile unsigned short*)(0x0094)))
#define ATD1DR3    (*((volatile unsigned short*)(0x0096)))
#define ATD1DR4    (*((volatile unsigned short*)(0x0098)))
#define ATD1DR5    (*((volatile unsigned short*)(0x009A)))
#define ATD1DR6    (*((volatile unsigned short*)(0x009C)))
#define ATD1DR7    (*((volatile unsigned short*)(0x009E)))
#define ATD1DR8    (*((volatile unsigned short*)(0x00A0)))
#define ATD1DR9    (*((volatile unsigned short*)(0x00A2)))
#define ATD1DR10   (*((volatile unsigned short*)(0x00A4)))
#define ATD1DR11   (*((volatile unsigned short*)(0x00A6)))
#define ATD1DR12   (*((volatile unsigned short*)(0x00A8)))
#define ATD1DR13   (*((volatile unsigned short*)(0x00AA)))
#define ATD1DR14   (*((volatile unsigned short*)(0x00AC)))
#define ATD1DR15   (*((volatile unsigned short*)(0x00AE)))

// IIC1
#define IBAD       (*((volatile unsigned char*)(0x00B0)))
#define IBFD       (*((volatile unsigned char*)(0x00B1)))
#define IBCR       (*((volatile unsigned char*)(0x00B2)))
#define IBSR       (*((volatile unsigned char*)(0x00B3)))
#define IBDR       (*((volatile unsigned char*)(0x00B4)))
//reserved 00B5
//reserved 00B6
//reserved 00B7

/* SCI2 definitions */
#define SCI2BDH    (*((volatile unsigned char*)(0x00B8)))
#define SCI2BDL    (*((volatile unsigned char*)(0x00B9)))
#define SCI2CR1    (*((volatile unsigned char*)(0x00BA)))
#define SCI2CR2    (*((volatile unsigned char*)(0x00BB)))
#define SCI2SR1    (*((volatile unsigned char*)(0x00BC)))
#define SCI2DRL    (*((volatile unsigned char*)(0x00BF)))

/* SCI3 definitions */
#define SCI3BDH    (*((volatile unsigned char*)(0x00C0)))
#define SCI3BDL    (*((volatile unsigned char*)(0x00C1)))
#define SCI3CR1    (*((volatile unsigned char*)(0x00C2)))
#define SCI3CR2    (*((volatile unsigned char*)(0x00C3)))
#define SCI3SR1    (*((volatile unsigned char*)(0x00C4)))
#define SCI3DRL    (*((volatile unsigned char*)(0x00C7)))

/* SCI0 definitions */
#define SCI0BDH    (*((volatile unsigned char*)(0x00C8))) // port_sci
#define SCI0BDL    (*((volatile unsigned char*)(0x00C9))) // +1
#define SCI0CR1    (*((volatile unsigned char*)(0x00CA))) // +2
#define SCI0CR2    (*((volatile unsigned char*)(0x00CB))) // +3
#define SCI0SR1    (*((volatile unsigned char*)(0x00CC))) // +4
#define SCI0DRL    (*((volatile unsigned char*)(0x00CF))) // +7

/* SCI1 definitions */
#define SCI1BDH    (*((volatile unsigned char*)(0x00D0)))
#define SCI1BDL    (*((volatile unsigned char*)(0x00D1)))
#define SCI1CR1    (*((volatile unsigned char*)(0x00D2)))
#define SCI1CR2    (*((volatile unsigned char*)(0x00D3)))
#define SCI1SR1    (*((volatile unsigned char*)(0x00D4)))
#define SCI1DRL    (*((volatile unsigned char*)(0x00D7)))

/* SPI0  definitions */
#define SPI0CR1    (*((volatile unsigned char*)(0x00D8)))
#define SPI0CR2    (*((volatile unsigned char*)(0x00D9)))
#define SPI0BR     (*((volatile unsigned char*)(0x00DA)))
#define SPI0SR     (*((volatile unsigned char*)(0x00DB)))
#define SPI0DRH    (*((volatile unsigned char*)(0x00DC)))
#define SPI0DRL    (*((volatile unsigned char*)(0x00DD)))

// IIC1
#define IBAD1      (*((volatile unsigned char*)(0x00E0)))
#define IBFD1      (*((volatile unsigned char*)(0x00E1)))
#define IBCR1      (*((volatile unsigned char*)(0x00E2)))
#define IBSR1      (*((volatile unsigned char*)(0x00E3)))
#define IBDR1      (*((volatile unsigned char*)(0x00E4)))
//reserved 00E5
//reserved 00E6
//reserved 00E7

//reserved 00E8-EF

/* SPI1  definitions */
#define SPI1CR1    (*((volatile unsigned char*)(0x00F0)))
#define SPI1CR2    (*((volatile unsigned char*)(0x00F1)))
#define SPI1BR     (*((volatile unsigned char*)(0x00F2)))
#define SPI1SR     (*((volatile unsigned char*)(0x00F3)))
#define SPI1DRH    (*((volatile unsigned char*)(0x00F4)))
#define SPI1DRL    (*((volatile unsigned char*)(0x00F5)))

/* SPI2  definitions */
#define SPI2CR1    (*((volatile unsigned char*)(0x00F8)))
#define SPI2CR2    (*((volatile unsigned char*)(0x00F9)))
#define SPI2BR     (*((volatile unsigned char*)(0x00FA)))
#define SPI2SR     (*((volatile unsigned char*)(0x00FB)))
#define SPI2DRH    (*((volatile unsigned char*)(0x00FC)))
#define SPI2DRL    (*((volatile unsigned char*)(0x00FD)))

/* Flash regs */
#define FCLKDIV    (*((volatile unsigned char*)(0x0100)))
#define FSEC       (*((volatile unsigned char*)(0x0101)))
#define FCCOBIX    (*((volatile unsigned char*)(0x0102)))
#define FECCRIX    (*((volatile unsigned char*)(0x0103)))
#define FCNFG      (*((volatile unsigned char*)(0x0104)))
#define FERCNFG    (*((volatile unsigned char*)(0x0105)))
#define FSTAT      (*((volatile unsigned char*)(0x0106)))
#define FERSTAT    (*((volatile unsigned char*)(0x0107)))
#define FPROT      (*((volatile unsigned char*)(0x0108)))
#define EPROT      (*((volatile unsigned char*)(0x0109)))
#define FCCOB      (*((volatile unsigned  int*)(0x010A)))
#define FCCOBHI    (*((volatile unsigned char*)(0x010A)))
#define FCCOBLO    (*((volatile unsigned char*)(0x010B)))
#define ETAGHI     (*((volatile unsigned char*)(0x010C)))
#define ETAGLO     (*((volatile unsigned char*)(0x010D)))
#define FECCRHI    (*((volatile unsigned char*)(0x010E)))
#define FECCRLO    (*((volatile unsigned char*)(0x010F)))
#define FOPT       (*((volatile unsigned char*)(0x0110)))
//reserved 0111
//reserved 0112
//reserved 0113

/* MPU */
#define MPUFLG     (*((volatile unsigned char*)(0x0114)))
#define MPUASTAT0  (*((volatile unsigned char*)(0x0115)))
#define MPUASTAT1  (*((volatile unsigned char*)(0x0116)))
#define MPUASTAT2  (*((volatile unsigned char*)(0x0117)))
//reserved 0118
#define MPUSEL    (*((volatile unsigned char*)(0x0119)))
#define MPUDESC0  (*((volatile unsigned char*)(0x011A)))
#define MPUDESC1  (*((volatile unsigned char*)(0x011B)))
#define MPUDESC2  (*((volatile unsigned char*)(0x011C)))
#define MPUDESC3  (*((volatile unsigned char*)(0x011D)))
#define MPUDESC4  (*((volatile unsigned char*)(0x011E)))
#define MPUDESC5  (*((volatile unsigned char*)(0x011F)))

/* S12X INT */
//reserved 0120
#define IVBR     (*((volatile unsigned char*)(0x0121)))
//reserved 0122
//reserved 0123
//reserved 0124
//reserved 0125
#define INT_XGPRIO  (*((volatile unsigned char*)(0x0126)))
#define INT_CFADDR  (*((volatile unsigned char*)(0x0127)))
#define INT_CFDATA0 (*((volatile unsigned char*)(0x0128)))
#define INT_CFDATA1 (*((volatile unsigned char*)(0x0129)))
#define INT_CFDATA2 (*((volatile unsigned char*)(0x012A)))
#define INT_CFDATA3 (*((volatile unsigned char*)(0x012B)))
#define INT_CFDATA4 (*((volatile unsigned char*)(0x012C)))
#define INT_CFDATA5 (*((volatile unsigned char*)(0x012D)))
#define INT_CFDATA6 (*((volatile unsigned char*)(0x012E)))
#define INT_CFDATA7 (*((volatile unsigned char*)(0x012F)))

/* SCI4 definitions */
#define SCI4BDH    (*((volatile unsigned char*)(0x0130)))
#define SCI4BDL    (*((volatile unsigned char*)(0x0131)))
#define SCI4CR1    (*((volatile unsigned char*)(0x0132)))
#define SCI4CR2    (*((volatile unsigned char*)(0x0133)))
#define SCI4SR1    (*((volatile unsigned char*)(0x0134)))
#define SCI4DRL    (*((volatile unsigned char*)(0x0137)))

/* SCI5 definitions */
#define SCI5BDH    (*((volatile unsigned char*)(0x0138)))
#define SCI5BDL    (*((volatile unsigned char*)(0x0139)))
#define SCI5CR1    (*((volatile unsigned char*)(0x013A)))
#define SCI5CR2    (*((volatile unsigned char*)(0x013B)))
#define SCI5SR1    (*((volatile unsigned char*)(0x013C)))
#define SCI5DRL    (*((volatile unsigned char*)(0x01CF)))

/* CAN0 section */
#define CAN0_BASE ((volatile unsigned char*)0x0140)

#define CAN0CTL0		CAN0_BASE[0x0]
#define CAN0CTL1		CAN0_BASE[0x1]
#define CAN0BTR0		CAN0_BASE[0x2]
#define CAN0BTR1		CAN0_BASE[0x3]
#define CAN0RFLG		CAN0_BASE[0x4]
#define CAN0RIER		CAN0_BASE[0x5]
#define CAN0TFLG		CAN0_BASE[0x6]
#define CAN0TIER		CAN0_BASE[0x7]
#define CAN0TARQ		CAN0_BASE[0x8]
#define CAN0TAAK		CAN0_BASE[0x9]
#define CAN0TBSEL		CAN0_BASE[0xA]
#define CAN0IDAC		CAN0_BASE[0xB]
#define CAN0RXERR		CAN0_BASE[0xE]
#define CAN0TXERR		CAN0_BASE[0xF]

#define CAN0IDAR0		CAN0_BASE[0x10]
#define CAN0IDAR1		CAN0_BASE[0x11]
#define CAN0IDAR2		CAN0_BASE[0x12]
#define CAN0IDAR3		CAN0_BASE[0x13]
#define CAN0IDAR4		CAN0_BASE[0x18]
#define CAN0IDAR5		CAN0_BASE[0x19]
#define CAN0IDAR6		CAN0_BASE[0x1A]
#define CAN0IDAR7		CAN0_BASE[0x1B]

#define CAN0IDMR0		CAN0_BASE[0x14]
#define CAN0IDMR1		CAN0_BASE[0x15]
#define CAN0IDMR2		CAN0_BASE[0x16]
#define CAN0IDMR3		CAN0_BASE[0x17]
#define CAN0IDMR4		CAN0_BASE[0x1C]
#define CAN0IDMR5		CAN0_BASE[0x1D]
#define CAN0IDMR6		CAN0_BASE[0x1E]
#define CAN0IDMR7		CAN0_BASE[0x1F]

#define CAN0_RB_IDR0	CAN0_BASE[0x20]
#define CAN0_RB_IDR1	CAN0_BASE[0x21]
#define CAN0_RB_IDR2	CAN0_BASE[0x22]
#define CAN0_RB_IDR3	CAN0_BASE[0x23]

#define CAN0_RB_DSR0	CAN0_BASE[0x24]
#define CAN0_RB_DSR1	CAN0_BASE[0x25]
#define CAN0_RB_DSR2	CAN0_BASE[0x26]
#define CAN0_RB_DSR3	CAN0_BASE[0x27]
#define CAN0_RB_DSR4	CAN0_BASE[0x28]
#define CAN0_RB_DSR5	CAN0_BASE[0x29]
#define CAN0_RB_DSR6	CAN0_BASE[0x2A]
#define CAN0_RB_DSR7	CAN0_BASE[0x2B]

#define CAN0_RB_DLR	CAN0_BASE[0x2C]

#define CAN0_RB_TBPR	CAN0_BASE[0x2D]

#define CAN0_TB0_IDR0	CAN0_BASE[0x30]
#define CAN0_TB0_IDR1	CAN0_BASE[0x31]
#define CAN0_TB0_IDR2	CAN0_BASE[0x32]
#define CAN0_TB0_IDR3	CAN0_BASE[0x33]

#define CAN0_TB0_DSR0	CAN0_BASE[0x34]
#define CAN0_TB0_DSR1	CAN0_BASE[0x35]
#define CAN0_TB0_DSR2	CAN0_BASE[0x36]
#define CAN0_TB0_DSR3	CAN0_BASE[0x37]
#define CAN0_TB0_DSR4	CAN0_BASE[0x38]
#define CAN0_TB0_DSR5	CAN0_BASE[0x39]
#define CAN0_TB0_DSR6	CAN0_BASE[0x3A]
#define CAN0_TB0_DSR7	CAN0_BASE[0x3B]

#define CAN0_TB0_DLR	CAN0_BASE[0x3C]

#define CAN0_TB0_TBPR	CAN0_BASE[0x3D]

 /* CAN1 section */
#define CAN1_BASE ((volatile unsigned char*)0x0180)

#define CAN1CTL0		CAN1_BASE[0x0]
#define CAN1CTL1		CAN1_BASE[0x1]
#define CAN1BTR0		CAN1_BASE[0x2]
#define CAN1BTR1		CAN1_BASE[0x3]
#define CAN1RFLG		CAN1_BASE[0x4]
#define CAN1RIER		CAN1_BASE[0x5]
#define CAN1TFLG		CAN1_BASE[0x6]
#define CAN1TIER		CAN1_BASE[0x7]
#define CAN1TARQ		CAN1_BASE[0x8]
#define CAN1TAAK		CAN1_BASE[0x9]
#define CAN1TBSEL		CAN1_BASE[0xA]
#define CAN1IDAC		CAN1_BASE[0xB]
#define CAN1RXERR		CAN1_BASE[0xE]
#define CAN1TXERR		CAN1_BASE[0xF]

#define CAN1IDAR0		CAN1_BASE[0x10]
#define CAN1IDAR1		CAN1_BASE[0x11]
#define CAN1IDAR2		CAN1_BASE[0x12]
#define CAN1IDAR3		CAN1_BASE[0x13]
#define CAN1IDAR4		CAN1_BASE[0x18]
#define CAN1IDAR5		CAN1_BASE[0x19]
#define CAN1IDAR6		CAN1_BASE[0x1A]
#define CAN1IDAR7		CAN1_BASE[0x1B]

#define CAN1IDMR0		CAN1_BASE[0x14]
#define CAN1IDMR1		CAN1_BASE[0x15]
#define CAN1IDMR2		CAN1_BASE[0x16]
#define CAN1IDMR3		CAN1_BASE[0x17]
#define CAN1IDMR4		CAN1_BASE[0x1C]
#define CAN1IDMR5		CAN1_BASE[0x1D]
#define CAN1IDMR6		CAN1_BASE[0x1E]
#define CAN1IDMR7		CAN1_BASE[0x1F]

#define CAN1_RB_IDR0	CAN1_BASE[0x20]
#define CAN1_RB_IDR1	CAN1_BASE[0x21]
#define CAN1_RB_IDR2	CAN1_BASE[0x22]
#define CAN1_RB_IDR3	CAN1_BASE[0x23]

#define CAN1_RB_DSR0	CAN1_BASE[0x24]
#define CAN1_RB_DSR1	CAN1_BASE[0x25]
#define CAN1_RB_DSR2	CAN1_BASE[0x26]
#define CAN1_RB_DSR3	CAN1_BASE[0x27]
#define CAN1_RB_DSR4	CAN1_BASE[0x28]
#define CAN1_RB_DSR5	CAN1_BASE[0x29]
#define CAN1_RB_DSR6	CAN1_BASE[0x2A]
#define CAN1_RB_DSR7	CAN1_BASE[0x2B]

#define CAN1_RB_DLR	CAN1_BASE[0x2C]

#define CAN1_RB_TBPR	CAN1_BASE[0x2D]

#define CAN1_TB0_IDR0	CAN1_BASE[0x30]
#define CAN1_TB0_IDR1	CAN1_BASE[0x31]
#define CAN1_TB0_IDR2	CAN1_BASE[0x32]
#define CAN1_TB0_IDR3	CAN1_BASE[0x33]

#define CAN1_TB0_DSR0	CAN1_BASE[0x34]
#define CAN1_TB0_DSR1	CAN1_BASE[0x35]
#define CAN1_TB0_DSR2	CAN1_BASE[0x36]
#define CAN1_TB0_DSR3	CAN1_BASE[0x37]
#define CAN1_TB0_DSR4	CAN1_BASE[0x38]
#define CAN1_TB0_DSR5	CAN1_BASE[0x39]
#define CAN1_TB0_DSR6	CAN1_BASE[0x3A]
#define CAN1_TB0_DSR7	CAN1_BASE[0x3B]

#define CAN1_TB0_DLR	CAN1_BASE[0x3C]

#define CAN1_TB0_TBPR	CAN1_BASE[0x3D]

// Also CAN2 + CAN3 0x1C0-0x23F

#define PORTT    (*((volatile unsigned char*)(0x0240)))
#define PTT      (*((volatile unsigned char*)(0x0240)))
#define pPTT     ((volatile unsigned char*)(0x0240))
#define PTIT     (*((volatile unsigned char*)(0x0241)))
#define DDRT     (*((volatile unsigned char*)(0x0242)))
#define RDRT     (*((volatile unsigned char*)(0x0243)))
#define PERT     (*((volatile unsigned char*)(0x0244)))
#define PPST     (*((volatile unsigned char*)(0x0245)))
//reserved 0246
//reserved 0247

#define PTS      (*((volatile unsigned char*)(0x0248)))
#define PTIS     (*((volatile unsigned char*)(0x0249)))
#define DDRS     (*((volatile unsigned char*)(0x024A)))
#define RDRS     (*((volatile unsigned char*)(0x024B)))
#define PERS     (*((volatile unsigned char*)(0x024C)))
#define PPSS     (*((volatile unsigned char*)(0x024D)))
#define WOMS     (*((volatile unsigned char*)(0x024E)))
//reserved 024F

#define PORTM    (*((volatile unsigned char*)(0x0250)))
#define PTM      (*((volatile unsigned char*)(0x0250)))
#define pPTM     ((volatile unsigned char*)(0x0250))
#define DDRM     (*((volatile unsigned char*)(0x0252)))
#define RDRM     (*((volatile unsigned char*)(0x0253)))
#define PERM     (*((volatile unsigned char*)(0x0254)))
#define PPSM     (*((volatile unsigned char*)(0x0255)))
#define WOMM     (*((volatile unsigned char*)(0x0256)))
#define MODRR    (*((volatile unsigned char*)(0x0257)))

#define PTP      (*((volatile unsigned char*)(0x0258)))
#define pPTP     (((volatile unsigned char*)(0x0258)))
#define PTIP     (*((volatile unsigned char*)(0x0259)))
#define DDRP     (*((volatile unsigned char*)(0x025A)))
#define RDRP     (*((volatile unsigned char*)(0x025B)))
#define PERP     (*((volatile unsigned char*)(0x025C)))
#define PPSP     (*((volatile unsigned char*)(0x025D)))
#define PIEP     (*((volatile unsigned char*)(0x025E)))
#define PIFP     (*((volatile unsigned char*)(0x025F)))

#define PTH      (*((volatile unsigned char*)(0x0260)))
#define pPTH     (((volatile unsigned char*)(0x0260)))
#define PTIH     (*((volatile unsigned char*)(0x0261)))
#define DDRH     (*((volatile unsigned char*)(0x0262)))
#define RDRH     (*((volatile unsigned char*)(0x0263)))
#define PERH     (*((volatile unsigned char*)(0x0264)))
#define PPSH     (*((volatile unsigned char*)(0x0265)))
#define PIEH     (*((volatile unsigned char*)(0x0266)))
#define PIFH     (*((volatile unsigned char*)(0x0267)))

#define PTJ      (*((volatile unsigned char*)(0x0268)))
#define pPTJ     (((volatile unsigned char*)(0x0268)))
#define PTIJ     (*((volatile unsigned char*)(0x0269)))
#define DDRJ     (*((volatile unsigned char*)(0x026A)))
#define RDRJ     (*((volatile unsigned char*)(0x026B)))
#define PERJ     (*((volatile unsigned char*)(0x026C)))
#define PPSJ     (*((volatile unsigned char*)(0x026D)))
#define PIEJ     (*((volatile unsigned char*)(0x026E)))
#define PIFJ     (*((volatile unsigned char*)(0x026F)))

//the next names are rather inconsistent, the first digit
//indicates high (0) or low (1) byte, instead of an H/L suffix
#define PT0AD0   (*((volatile unsigned char*)(0x0270)))
#define PT1AD0   (*((volatile unsigned char*)(0x0271)))
#define DDR0AD0  (*((volatile unsigned char*)(0x0272)))
#define DDR1AD0  (*((volatile unsigned char*)(0x0273)))
#define RDR0AD0  (*((volatile unsigned char*)(0x0274)))
#define RDR1AD0  (*((volatile unsigned char*)(0x0275)))
#define PER0AD0  (*((volatile unsigned char*)(0x0276)))
#define PER1AD0  (*((volatile unsigned char*)(0x0277)))

#define PT0AD1   (*((volatile unsigned char*)(0x0278)))
#define PT1AD1   (*((volatile unsigned char*)(0x0279)))
#define DDR0AD1  (*((volatile unsigned char*)(0x027A)))
#define DDR1AD1  (*((volatile unsigned char*)(0x027B)))
#define RDR0AD1  (*((volatile unsigned char*)(0x027C)))
#define RDR1AD1  (*((volatile unsigned char*)(0x027D)))
#define PER0AD1  (*((volatile unsigned char*)(0x027E)))
#define PER1AD1  (*((volatile unsigned char*)(0x027F)))

//my naming scheme
#define PTAD0H   (*((volatile unsigned char*)(0x0270)))
#define pPTAD0H  ((volatile unsigned char*)(0x0270))
#define PTAD0L   (*((volatile unsigned char*)(0x0271)))
#define pPTAD0L  ((volatile unsigned char*)(0x0271))
#define DDRAD0H  (*((volatile unsigned char*)(0x0272)))
#define DDRAD0L  (*((volatile unsigned char*)(0x0273)))
#define RDRAD0H  (*((volatile unsigned char*)(0x0274)))
#define RDRAD0L  (*((volatile unsigned char*)(0x0275)))
#define PERAD0H  (*((volatile unsigned char*)(0x0276)))
#define PERAD0L  (*((volatile unsigned char*)(0x0277)))

#define PTAD1H   (*((volatile unsigned char*)(0x0278)))
#define PTAD1L   (*((volatile unsigned char*)(0x0279)))
#define DDRAD1H  (*((volatile unsigned char*)(0x027A)))
#define DDRAD1L  (*((volatile unsigned char*)(0x027B)))
#define RDRAD1H  (*((volatile unsigned char*)(0x027C)))
#define RDRAD1L  (*((volatile unsigned char*)(0x027D)))
#define PERAD1H  (*((volatile unsigned char*)(0x027E)))
#define PERAD1L  (*((volatile unsigned char*)(0x027F)))

//0x280-2BF CAN4

/* ATD1 (ADC) definitions */
#define ATD0CTL0   (*((volatile unsigned char*)(0x02C0)))
#define ATD0CTL1   (*((volatile unsigned char*)(0x02C1)))
#define ATD0CTL2   (*((volatile unsigned char*)(0x02C2)))
#define ATD0CTL3   (*((volatile unsigned char*)(0x02C3)))
#define ATD0CTL4   (*((volatile unsigned char*)(0x02C4)))
#define ATD0CTL5   (*((volatile unsigned char*)(0x02C5)))
#define ATD0STAT0  (*((volatile unsigned char*)(0x02C6)))
//reserved 02C7
#define ATD0CMPE   (*((volatile unsigned short*)(0x02C8)))
#define ATD0STAT2  (*((volatile unsigned short*)(0x02CA)))
#define ATD0DIEN   (*((volatile unsigned short*)(0x02CC)))
#define ATD0DIENH  (*((volatile unsigned char*)(0x02CC)))
#define ATD0DIENL  (*((volatile unsigned char*)(0x02CD)))
#define ATD0CMPHT  (*((volatile unsigned short*)(0x02CE)))
#define ATD0DR0    (*((volatile unsigned short*)(0x02D0)))
#define ATD0DR1    (*((volatile unsigned short*)(0x02D2)))
#define ATD0DR2    (*((volatile unsigned short*)(0x02D4)))
#define ATD0DR3    (*((volatile unsigned short*)(0x02D6)))
#define ATD0DR4    (*((volatile unsigned short*)(0x02D8)))
#define ATD0DR5    (*((volatile unsigned short*)(0x02DA)))
#define ATD0DR6    (*((volatile unsigned short*)(0x02DC)))
#define ATD0DR7    (*((volatile unsigned short*)(0x02DE)))
#define ATD0DR8    (*((volatile unsigned short*)(0x02E0)))
#define ATD0DR9    (*((volatile unsigned short*)(0x02E2)))
#define ATD0DR10   (*((volatile unsigned short*)(0x02E4)))
#define ATD0DR11   (*((volatile unsigned short*)(0x02E6)))
#define ATD0DR12   (*((volatile unsigned short*)(0x02E8)))
#define ATD0DR13   (*((volatile unsigned short*)(0x02EA)))
#define ATD0DR14   (*((volatile unsigned short*)(0x02EC)))
#define ATD0DR15   (*((volatile unsigned short*)(0x02EE)))

/* voltage reg */
#define VREGHTVL   (*((volatile unsigned char*)(0x02F0)))
#define VREGCTRL   (*((volatile unsigned char*)(0x02F1)))
#define VREGAPICL  (*((volatile unsigned char*)(0x02F2)))
#define VREGAPITR  (*((volatile unsigned char*)(0x02F3)))
#define VREGAPIR  (*((volatile unsigned short*)(0x02F4)))
//reserved 02F6
//reserved 02F7
//reserved 02F8-2FF

/* PWM definitions */
#define PWME        (*((volatile unsigned char*)(0x0300)))
#define PWMPOL      (*((volatile unsigned char*)(0x0301)))
#define PWMCLK      (*((volatile unsigned char*)(0x0302)))
#define PWMPRCLK    (*((volatile unsigned char*)(0x0303)))
#define PWMCAE      (*((volatile unsigned char*)(0x0304)))
#define PWMCTL      (*((volatile unsigned char*)(0x0305)))
//reserved 0306
#define PWMPRSC     (*((volatile unsigned char*)(0x0307)))
#define PWMSCLA   (*((volatile unsigned char*)(0x0308)))
#define PWMSCLB   (*((volatile unsigned char*)(0x0309)))
#define PWMSCNTA  (*((volatile unsigned char*)(0x030A)))
#define PWMSCNTB  (*((volatile unsigned char*)(0x030B)))
#define PWMCNT0     (*((volatile unsigned char*)(0x030C)))
#define PWMCNT1     (*((volatile unsigned char*)(0x030D)))
#define PWMCNT2     (*((volatile unsigned char*)(0x030E)))
#define PWMCNT3     (*((volatile unsigned char*)(0x030F)))
#define PWMCNT4     (*((volatile unsigned char*)(0x0310)))
#define PWMCNT5     (*((volatile unsigned char*)(0x0311)))
#define PWMCNT6     (*((volatile unsigned char*)(0x0312)))
#define PWMCNT7     (*((volatile unsigned char*)(0x0313)))
#define PWMPER0     (*((volatile unsigned char*)(0x0314)))
#define PWMPER1     (*((volatile unsigned char*)(0x0315)))
#define PWMPER2     (*((volatile unsigned char*)(0x0316)))
#define PWMPER3     (*((volatile unsigned char*)(0x0317)))
#define PWMPER4     (*((volatile unsigned char*)(0x0318)))
#define PWMPER5     (*((volatile unsigned char*)(0x0319)))
#define PWMPER6     (*((volatile unsigned char*)(0x031A)))
#define PWMPER7     (*((volatile unsigned char*)(0x031B)))
#define PWMDTY0     (*((volatile unsigned char*)(0x031C)))
#define PWMDTY1     (*((volatile unsigned char*)(0x031D)))
#define PWMDTY2     (*((volatile unsigned char*)(0x031E)))
#define PWMDTY3     (*((volatile unsigned char*)(0x031F)))
#define PWMDTY4     (*((volatile unsigned char*)(0x0320)))
#define PWMDTY5     (*((volatile unsigned char*)(0x0321)))
#define PWMDTY6     (*((volatile unsigned char*)(0x0322)))
#define PWMDTY7     (*((volatile unsigned char*)(0x0323)))
#define PWMSDN      (*((volatile unsigned char*)(0x0324)))
//reserved 0325-32F

// 0x330-337 SCI6
// 0x338-33F SCI7

/* PIT timer */
#define PITCFLMT    (*((volatile unsigned char*)(0x0340)))
#define PITFLT      (*((volatile unsigned char*)(0x0341)))
#define PITCE       (*((volatile unsigned char*)(0x0342)))
#define PITMUX      (*((volatile unsigned char*)(0x0343)))
#define PITINTE     (*((volatile unsigned char*)(0x0344)))
#define PITTF       (*((volatile unsigned char*)(0x0345)))
#define PITMTLD0    (*((volatile unsigned char*)(0x0346)))
#define PITMTLD1    (*((volatile unsigned char*)(0x0347)))
#define PITLD0     (*((volatile unsigned short*)(0x0348)))
#define PITCNT0    (*((volatile unsigned short*)(0x034A)))
#define PITLD1     (*((volatile unsigned short*)(0x034C)))
#define PITCNT1    (*((volatile unsigned short*)(0x034E)))
#define PITLD2     (*((volatile unsigned short*)(0x0350)))
#define PITCNT2    (*((volatile unsigned short*)(0x0352)))
#define PITLD3     (*((volatile unsigned short*)(0x0354)))
#define PITCNT3    (*((volatile unsigned short*)(0x0356)))
#define PITLD4     (*((volatile unsigned short*)(0x0358)))
#define PITCNT4    (*((volatile unsigned short*)(0x035A)))
#define PITLD5     (*((volatile unsigned short*)(0x035C)))
#define PITCNT5    (*((volatile unsigned short*)(0x035E)))
#define PITLD6     (*((volatile unsigned short*)(0x0360)))
#define PITCNT6    (*((volatile unsigned short*)(0x0362)))
#define PITLD7     (*((volatile unsigned short*)(0x0364)))
#define PITCNT7    (*((volatile unsigned short*)(0x0366)))

/* PIM map 6 */
#define PTR      (*((volatile unsigned char*)(0x0368)))
#define PTIR     (*((volatile unsigned char*)(0x0369)))
#define DDRR     (*((volatile unsigned char*)(0x036A)))
#define RDRR     (*((volatile unsigned char*)(0x036B)))
#define PERR     (*((volatile unsigned char*)(0x036C)))
#define PPSR     (*((volatile unsigned char*)(0x036D)))
//reserved 036E
#define PTRRR    (*((volatile unsigned char*)(0x036F)))

#define PTL      (*((volatile unsigned char*)(0x0370)))
#define PTIL     (*((volatile unsigned char*)(0x0371)))
#define DDRL     (*((volatile unsigned char*)(0x0372)))
#define RDRL     (*((volatile unsigned char*)(0x0373)))
#define PERL     (*((volatile unsigned char*)(0x0374)))
#define PPSL     (*((volatile unsigned char*)(0x0375)))
#define WOML     (*((volatile unsigned char*)(0x0376)))
#define PTLRR    (*((volatile unsigned char*)(0x0377)))

#define PTF      (*((volatile unsigned char*)(0x0378)))
#define PTIF     (*((volatile unsigned char*)(0x0379)))
#define DDRF     (*((volatile unsigned char*)(0x037A)))
#define RDRF     (*((volatile unsigned char*)(0x037B)))
#define PERF     (*((volatile unsigned char*)(0x037C)))
#define PPSF     (*((volatile unsigned char*)(0x037D)))
//reserved 037E
#define PTFRR    (*((volatile unsigned char*)(0x037F)))

/* XGATE */
#define XGMCTL   (*((volatile unsigned short*)(0x0380)))
#define XGCHID    (*((volatile unsigned char*)(0x0382)))
#define XGCHPL    (*((volatile unsigned char*)(0x0383)))
//reserved 0384
#define XGISPSEL (*((volatile unsigned char*)(0x0385)))
#define XGVBR    (*((volatile unsigned short*)(0x0386)))
#define XGIF78   (*((volatile unsigned char*)(0x0388)))
#define XGIF77   (*((volatile unsigned char*)(0x0389)))
#define XGIF6F   (*((volatile unsigned char*)(0x038A)))
#define XGIF67   (*((volatile unsigned char*)(0x038B)))
#define XGIF5F   (*((volatile unsigned char*)(0x038C)))
#define XGIF57   (*((volatile unsigned char*)(0x038D)))
#define XGIF4F   (*((volatile unsigned char*)(0x038E)))
#define XGIF47   (*((volatile unsigned char*)(0x038F)))
#define XGIF3F   (*((volatile unsigned char*)(0x0390)))
#define XGIF37   (*((volatile unsigned char*)(0x0391)))
#define XGIF2F   (*((volatile unsigned char*)(0x0392)))
#define XGIF27   (*((volatile unsigned char*)(0x0393)))
#define XGIF1F   (*((volatile unsigned char*)(0x0394)))
#define XGIF17   (*((volatile unsigned char*)(0x0395)))
#define XGIF0F   (*((volatile unsigned char*)(0x0396)))
#define XGIF07   (*((volatile unsigned char*)(0x0397)))
#define XGSWTM   (*((volatile unsigned short*)(0x0398)))        // needs to be written as 16bits
#define XGSWT    (*((volatile unsigned char*)(0x0399)))
#define XGSEMM   (*((volatile unsigned short*)(0x039A)))        // needs to be written as 16bits
#define XGSEM    (*((volatile unsigned char*)(0x039B)))
//reserved 039C
#define XGCCR    (*((volatile unsigned char*)(0x039D)))
#define XGPC    (*((volatile unsigned short*)(0x039E)))
//reserved 03A0
//reserved 03A1
#define XGR0    (*((volatile unsigned short*)(0x03A2)))
#define XGR1    (*((volatile unsigned short*)(0x03A4)))
#define XGR2    (*((volatile unsigned short*)(0x03A6)))
#define XGR3    (*((volatile unsigned short*)(0x03A8)))
#define XGR4    (*((volatile unsigned short*)(0x03AA)))
#define XGR5    (*((volatile unsigned short*)(0x03AC)))
#define XGR6    (*((volatile unsigned short*)(0x03AE)))
#define XGR7    (*((volatile unsigned short*)(0x03B0)))
//reserved 03B0-3CF

/* TIM Timer definitions */
#define TIMTIOS     (*((volatile unsigned char*)(0x03D0)))
#define TIMCFORC    (*((volatile unsigned char*)(0x03D1)))
#define TIMOC7M     (*((volatile unsigned char*)(0x03D2)))
#define TIMOC7D     (*((volatile unsigned char*)(0x03D3)))
#define TIMTCNT     (*((volatile unsigned short*)(0x03D4)))
//TCNTL
#define TIMTSCR1    (*((volatile unsigned char*)(0x03D6)))
#define TIMTTOV     (*((volatile unsigned char*)(0x03D7)))
#define TIMTCTL1    (*((volatile unsigned char*)(0x03D8)))
#define TIMTCTL2    (*((volatile unsigned char*)(0x03D9)))
#define TIMTCTL3    (*((volatile unsigned char*)(0x03DA)))
#define TIMTCTL4    (*((volatile unsigned char*)(0x03DB)))
#define TIMTIE      (*((volatile unsigned char*)(0x03DC)))
#define TIMTSCR2    (*((volatile unsigned char*)(0x03DD)))
#define TIMTFLG1    (*((volatile unsigned char*)(0x03DE)))
#define TIMTFLG2    (*((volatile unsigned char*)(0x03DF)))
#define TIMTC0      (*((volatile unsigned short*)(0x03E0)))
#define TIMTC1      (*((volatile unsigned short*)(0x03E2)))
#define TIMTC2      (*((volatile unsigned short*)(0x03E4)))
#define TIMTC3      (*((volatile unsigned short*)(0x03E6)))
#define TIMTC4      (*((volatile unsigned short*)(0x03E8)))
#define TIMTC5      (*((volatile unsigned short*)(0x03EA)))
#define TIMTC6      (*((volatile unsigned short*)(0x03EC)))
#define TIMTC7      (*((volatile unsigned short*)(0x03EE)))
#define TIMPACTL    (*((volatile unsigned char*)(0x03F0)))
#define TIMPAFLG    (*((volatile unsigned char*)(0x03F1)))
#define TIMPACNT    (*((volatile unsigned short*)(0x03F2)))
//reserved 03F4-3FB
#define TIMOCPD     (*((volatile unsigned char*)(0x03FC)))
//reserved 03FD
#define TIMPTPSR    (*((volatile unsigned char*)(0x03FE)))
//reserved 03FF
//reserved 0400-7FF
