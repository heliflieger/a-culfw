/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmpconfig.h
 *
 * Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmpconfig.h,v 1.1 2011-05-03 23:51:58 tostmann Exp $
 *
 * ATMEGA88 @ 8 MHz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#ifndef _IRMPCONFIG_H_
#define _IRMPCONFIG_H_

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change F_INTERRUPTS if you change the number of interrupts per second,
 * Normally, F_INTERRUPTS should be in the range from 10000 to 15000.
 * A value above 15000 costs additional program space, absolut maximum value is 20000.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef F_INTERRUPTS
#define F_INTERRUPTS                            15625   // interrupts per second, min: 10000, max: 20000
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change settings from 1 to 0 if you want to disable one or more decoders.
 * This saves program space.
 *
 * 1 enable  decoder
 * 0 disable decoder
 *
 * The standard decoders are enabled per default.
 * Some less common protocols are disabled here, you need to enable them manually.
 *
 * If you want to use FDC or RCCAR simultaneous with RC5 protocol, additional program space is required.
 * If you don't need RC5 when using FDC/RCCAR, you should disable RC5.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

//      Protocol                                Enable  Remarks                 F_INTERRUPTS            Program Space
#define IRMP_SUPPORT_SIRCS_PROTOCOL             1       // Sony SIRCS           >= 10000                 ~100 bytes
#define IRMP_SUPPORT_NEC_PROTOCOL               1       // NEC + APPLE          >= 10000                 ~250 bytes
#define IRMP_SUPPORT_SAMSUNG_PROTOCOL           1       // Samsung + Samsung32  >= 10000                 ~250 bytes
#define IRMP_SUPPORT_MATSUSHITA_PROTOCOL        1       // Matsushita           >= 10000                  ~50 bytes
#define IRMP_SUPPORT_KASEIKYO_PROTOCOL          1       // Kaseikyo             >= 10000                 ~250 bytes
#define IRMP_SUPPORT_DENON_PROTOCOL             1       // DENON                >= 10000                 ~250 bytes
#define IRMP_SUPPORT_JVC_PROTOCOL               1       // JVC                  >= 10000                 ~250 bytes
#define IRMP_SUPPORT_RC5_PROTOCOL               1       // RC5                  >= 10000                 ~250 bytes
#define IRMP_SUPPORT_RC6_PROTOCOL               1       // RC6 & RC6A           >= 10000                 ~200 bytes
#define IRMP_SUPPORT_GRUNDIG_PROTOCOL           1       // Grundig              >= 10000                 ~150 bytes
#define IRMP_SUPPORT_NOKIA_PROTOCOL             1       // Nokia                >= 10000                 ~150 bytes
#define IRMP_SUPPORT_NUBERT_PROTOCOL            1       // NUBERT               >= 10000                  ~50 bytes
#define IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL      0       // Bang & Olufsen       >= 10000                 ~200 bytes
#define IRMP_SUPPORT_NIKON_PROTOCOL             0       // NIKON                >= 10000                 ~250 bytes
#define IRMP_SUPPORT_FDC_PROTOCOL               1       // FDC3402 keyboard     >= 10000 (better 15000)   ~50 bytes (~400 in combination with RC5)
#define IRMP_SUPPORT_RCCAR_PROTOCOL             0       // RC Car               >= 10000 (better 15000)  ~150 bytes (~500 in combination with RC5)
#define IRMP_SUPPORT_SIEMENS_PROTOCOL           0       // Siemens Gigaset      >= 15000                 ~150 bytes
#define IRMP_SUPPORT_RECS80_PROTOCOL            0       // RECS80               >= 20000                  ~50 bytes
#define IRMP_SUPPORT_RECS80EXT_PROTOCOL         0       // RECS80EXT            >= 20000                  ~50 bytes

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef PIC_CCS_COMPILER                                 // PIC CCS Compiler:

#define IRMP_PIN                                PIN_D3  // use PB4 as IR input on PIC

#else                                                   // AVR:

#define IRMP_PORT                               PORTD
#define IRMP_DDR                                DDRD
#define IRMP_PIN                                PIND
#define IRMP_BIT                                3       // use PB6 as IR input on AVR

#define input(x)                                ((x) & (1 << IRMP_BIT))
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Set IRMP_LOGGING to 1 if want to log data to UART with 9600Bd
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_LOGGING
#define IRMP_LOGGING                            0       // 1: log IR signal (scan), 0: do not (default)
#endif

#if IRMP_SUPPORT_SIEMENS_PROTOCOL == 1 && F_INTERRUPTS < 15000
#warning F_INTERRUPTS too low, SIEMENS protocol disabled (should be at least 15000)
#undef IRMP_SUPPORT_SIEMENS_PROTOCOL
#define IRMP_SUPPORT_SIEMENS_PROTOCOL           0
#endif

#if IRMP_SUPPORT_RECS80_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, RECS80 protocol disabled (should be at least 20000)
#undef IRMP_SUPPORT_RECS80_PROTOCOL
#define IRMP_SUPPORT_RECS80_PROTOCOL            0
#endif

#if IRMP_SUPPORT_RECS80EXT_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, RECS80EXT protocol disabled (should be at least 20000)
#undef IRMP_SUPPORT_RECS80EXT_PROTOCOL
#define IRMP_SUPPORT_RECS80EXT_PROTOCOL         0
#endif

#if IRMP_SUPPORT_JVC_PROTOCOL == 1 && IRMP_SUPPORT_NEC_PROTOCOL == 0
#warning JVC protocol needs also NEC protocol, NEC protocol enabled
#undef IRMP_SUPPORT_NEC_PROTOCOL
#define IRMP_SUPPORT_NEC_PROTOCOL               1
#endif

#endif /* _WC_IRMPCONFIG_H_ */
