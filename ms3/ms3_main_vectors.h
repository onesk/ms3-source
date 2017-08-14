/* $Id: ms3_main_vectors.h,v 1.15 2012/08/20 15:04:01 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 *
    Origin: Al Grippo
    Major: MS3 vectors. James Murray / Kenneth Culver
    Majority: James Murray / Kenneth Culver
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#ifndef _MS3_MAIN_VECTORS
#define _MS3_MAIN_VECTORS
const tIsrFunc _vect[] VECT_ATTR = {    /* Pseudo vector Interrupt table. */
    UnimplementedISR,           /* FF10 vector 119 Spurious */
    UnimplementedISR,           /* FF12 vector 118 SYS */
    UnimplementedISR,           /* FF14 vector 117 MPU acc err */
    xgswe_handler,              /* FF16 vector 116 XGATE err */
    UnimplementedISR,           /* FF18 vector 115 */
    UnimplementedISR,           /* FF1A vector 114 */
    UnimplementedISR,           /* FF1C vector 113 */
    UnimplementedISR,           /* FF1E vector 112 */
    UnimplementedISR,           /* FF20 vector 111 */
    UnimplementedISR,           /* FF22 vector 110 */
    UnimplementedISR,           /* FF24 vector 109 */
    UnimplementedISR,           /* FF26 vector 108 */
    UnimplementedISR,           /* FF28 vector 107 */
    UnimplementedISR,           /* FF2A vector 106 */
    UnimplementedISR,           /* FF2C vector 105 */
    UnimplementedISR,           /* FF2E vector 104 */
    UnimplementedISR,           /* FF30 vector 103 */
    UnimplementedISR,           /* FF32 vector 102 */
    UnimplementedISR,           /* FF34 vector 101 */
    UnimplementedISR,           /* FF36 vector 100 */
    UnimplementedISR,           /* FF38 vector 99 */
    UnimplementedISR,           /* FF3A vector 98 */
    UnimplementedISR,           /* FF3C vector 97 ATD1 compare */
    UnimplementedISR,           /* FF3E vector 96 ATD0 compare */
    UnimplementedISR,           /* FF40 vector 95 TIM PAC in edge */
    UnimplementedISR,           /* FF42 vector 94 TIM PAC ovf */
    ISR_TIMTimerOverflow,       /* FF44 vector 93 TIM overflow */
    UnimplementedISR,           /* FF46 vector 92 TIM7 */
    UnimplementedISR,           /* FF48 vector 91 TIM6 */
    UnimplementedISR,           /* FF4A vector 90 TIM5 */
    UnimplementedISR,           /* FF4C vector 89 TIM4 */
    UnimplementedISR,           /* FF4E vector 88 TIM3 */
    UnimplementedISR,           /* FF50 vector 87 TIM2 */
    UnimplementedISR,           /* FF52 vector 86 TIM1 */
    UnimplementedISR,           /* FF54 vector 85 TIM0 */
    UnimplementedISR,           /* FF56 vector 84 SCI7 */
    UnimplementedISR,           /* FF58 vector 83 PIT7 */
    UnimplementedISR,           /* FF5A vector 82 PIT6 */
    UnimplementedISR,           /* FF5C vector 81 PIT5 */
    UnimplementedISR,           /* FF5E vector 80 PIT4 */
    UnimplementedISR,           /* FF60 vector 79 reserved */
    UnimplementedISR,           /* FF62 vector 78 reserved */
    UnimplementedISR,           /* FF64 vector 77 XGATE7 */
    xg_cam_logger,              /* FF66 vector 76 XGATE6 handled on S12X, cam logger from XGATE*/
    xg_crank_logger,            /* FF68 vector 75 XGATE5 handled on S12X, crank logger from XGATE*/
    exec_TimerIn,               /* FF6A vector 74 XGATE4 handled on S12X*/
    UnimplementedISR,           /* FF6C vector 73 XGATE3 handled on XGATE */
    UnimplementedISR,           /* FF6E vector 72 XGATE2 handled on XGATE */
    UnimplementedISR,           /* FF70 vector 71 XGATE1 handled on XGATE */
    UnimplementedISR,           /* FF72 vector 70 XGATE0 handled on XGATE */
    UnimplementedISR,           /* FF74 vector 69 PIT3 */
    UnimplementedISR,           /* FF76 vector 68 PIT2 handled on XGATE */
    ISR_pit1,                   /* FF78 vector 67 PIT1 */
    ISR_pit0,                   /* FF7A vector 66 PIT0 */
    UnimplementedISR,           /* FF7C vector 65 High temp */
    UnimplementedISR,           /* FF7E vector 64 API */

    UnimplementedISR,           /* FF80 vector 63 LVI */
    UnimplementedISR,           /* FF82 vector 62 IIC1 */
    UnimplementedISR,           /* FF84 vector 61 SCI5 */
    UnimplementedISR,           /* FF86 vector 60 SCI4 */
    UnimplementedISR,           /* FF88 vector 59 SCI3 */
    UnimplementedISR,           /* FF8A vector 58 SCI2 */
    UnimplementedISR,           /* FF8C vector 57 PWM emerg */
    UnimplementedISR,           /* FF8E vector 56 Port P */
    UnimplementedISR,           /* FF90 vector 55 CAN4 tx */
    UnimplementedISR,           /* FF92 vector 54 CAN4 rx */
    UnimplementedISR,           /* FF94 vector 53 CAN4 err */
    UnimplementedISR,           /* FF96 vector 52 CAN4 wake */
    UnimplementedISR,           /* FF98 vector 51 CAN3 tx */
    UnimplementedISR,           /* FF9A vector 50 CAN3 rx */
    UnimplementedISR,           /* FF9C vector 49 CAN3 err */
    UnimplementedISR,           /* FF9E vector 48 CAN3 wake */
    UnimplementedISR,           /* FFA0 vector 47 CAN2 tx */
    UnimplementedISR,           /* FFA2 vector 46 CAN2 rx */
    UnimplementedISR,           /* FFA4 vector 45 CAN2 err */
    UnimplementedISR,           /* FFA6 vector 44 CAN2 wake */
    UnimplementedISR,           /* FFE8 vector 43 CAN1 tx */
    UnimplementedISR,           /* FFAA vector 42 CAN1 rx */
    UnimplementedISR,           /* FFAC vector 41 CAN1 err */
    UnimplementedISR,           /* FFAE vector 40 CAN1 wake */
    CanTxIsr,                   /* FFB0 vector 39 CAN0 tx */
    CanRxIsr,                   /* FFB2 vector 38 CAN0 rx */
    CanRxIsr,                   /* FFB4 vector 37 CAN0 err */
    UnimplementedISR,           /* FFB6 vector 36 CAN0 wake */
    UnimplementedISR,           /* FFB8 vector 35 Flash */
    UnimplementedISR,           /* FFBA vector 34 Flash fault detect */
    UnimplementedISR,           /* FFBC vector 33 SPI2 */
    ISR_SPI,                    /* FFBE vector 32 SPI1 */
    UnimplementedISR,           /* FFC0 vector 31 IIC0 bus */
    UnimplementedISR,           /* FCC2 vector 30 SCI6 */
    UnimplementedISR,           /* FFC4 vector 29 CRG self clock mode */
    UnimplementedISR,           /* FFC6 vector 28 CRG PLL lock */
    UnimplementedISR,           /* FFC8 vector 27 PAC B overflow */
    UnimplementedISR,           /* FFCA vector 26 Modulus down u/flow */
    UnimplementedISR,           /* FFCC vector 25 Port H */
    UnimplementedISR,           /* FFCE vector 24 Port J */
    UnimplementedISR,           /* FFD0 vector 23 ATD1 */
    UnimplementedISR,           /* FFD2 vector 22 ATD0 */
    ISR_SCI1,                   /* FFD4 vector 21 SCI1 */
    ISR_SCI0,                   /* FFD6 vector 20 SCI0 */
    UnimplementedISR,           /* FFD8 vector 19 SPI0 */
    UnimplementedISR,           /* FFDA vector 18 PAC A input edge */
    UnimplementedISR,           /* FFDC vector 17 PA overflow */
    ISR_TimerOverflow,          /* FFDE vector 16 ECT overflow */
    UnimplementedISR,           /* FFE0 vector 15 ECT7 dwell on XGATE */
    ISR_TC6,                    /* FFE2 vector 14 ECT6 rotary dwell on XGATE or additional cam*/
    ISR_TC5,                    /* FFE4 vector 13 ECT5 JS10 cam */
    ISR_TC4,                    /* FFE6 vector 12 ECT4 additional cam*/
    UnimplementedISR,           /* FFE8 vector 11 ECT3 rotary spark on XGATE */
    ISR_TC2,                    /* FFEA vector 10 ECT2 MS3X cam */
    UnimplementedISR,           /* FFEC vector 09 ECT1 spark on XGATE */
    ISR_Ign_TimerIn,            /* FFEE vector 08 ECT0 main crank tach in*/
    ISR_Timer_Clock,            /* FFF0 vector 07 RTI */
    UnimplementedISR,           /* FFF2 vector 06 IRQ */
    UnimplementedISR,           /* FFF4 vector 05 XIRQ */
    UnimplementedISR,           /* FFF6 vector 04 SWI */
    UnimplementedISR,           /* FFF8 vector 03 Unimplemented instruction */
    UnimplementedISR,           /* FFFA vector 02 COP watchdog */
    UnimplementedISR,           /* FFFC vector 01 Clock monitor reset */
    _start                      /* FFFE vector 00 Reset vector */
};
#endif
