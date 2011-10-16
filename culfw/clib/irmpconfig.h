/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irmpconfig.h
 *
 * Copyright (c) 2009-2011 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irmpconfig.h,v 1.74 2011/10/15 14:19:38 odroegehorn Exp $
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

#include "board.h"

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change F_INTERRUPTS if you change the number of interrupts per second,
 * Normally, F_INTERRUPTS should be in the range from 10000 to 15000, typical is 15000
 * A value above 15000 costs additional program space, absolute maximum value is 20000.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef F_INTERRUPTS
#define F_INTERRUPTS                            15625   // interrupts per second, min: 10000, max: 20000, typ: 15000
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change settings from 1 to 0 if you want to disable one or more decoders.
 * This saves program space.
 *
 * 1 enable  decoder
 * 0 disable decoder
 *
 * The standard decoders are enabled per default.
 * Less common protocols are disabled here, you need to enable them manually.
 *
 * If you want to use FDC or RCCAR simultaneous with RC5 protocol, additional program space is required.
 * If you don't need RC5 when using FDC/RCCAR, you should disable RC5.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

// typical protocols, disable here!             Enable  Remarks                 F_INTERRUPTS            Program Space
#define IRMP_SUPPORT_SIRCS_PROTOCOL             1       // Sony SIRCS           >= 10000                 ~150 bytes
#define IRMP_SUPPORT_NEC_PROTOCOL               1       // NEC + APPLE          >= 10000                 ~300 bytes
#define IRMP_SUPPORT_SAMSUNG_PROTOCOL           1       // Samsung + Samsung32  >= 10000                 ~300 bytes
#define IRMP_SUPPORT_MATSUSHITA_PROTOCOL        1       // Matsushita           >= 10000                  ~50 bytes
#define IRMP_SUPPORT_KASEIKYO_PROTOCOL          1       // Kaseikyo             >= 10000                 ~250 bytes
#define IRMP_SUPPORT_DENON_PROTOCOL             1       // DENON, Sharp         >= 10000                 ~250 bytes

// more protocols, enable here!                 Enable  Remarks                 F_INTERRUPTS            Program Space
#define IRMP_SUPPORT_RC5_PROTOCOL               0       // RC5                  >= 10000                 ~250 bytes
#define IRMP_SUPPORT_RC6_PROTOCOL               0       // RC6 & RC6A           >= 10000                 ~250 bytes
#define IRMP_SUPPORT_JVC_PROTOCOL               0       // JVC                  >= 10000                 ~150 bytes
#define IRMP_SUPPORT_NEC16_PROTOCOL             0       // NEC16                >= 10000                 ~100 bytes
#define IRMP_SUPPORT_NEC42_PROTOCOL             0       // NEC42                >= 10000                 ~300 bytes
#define IRMP_SUPPORT_IR60_PROTOCOL              0       // IR60 (SAB2008)       >= 10000                 ~300 bytes
#define IRMP_SUPPORT_GRUNDIG_PROTOCOL           0       // Grundig              >= 10000                 ~300 bytes
#define IRMP_SUPPORT_SIEMENS_PROTOCOL           0       // Siemens Gigaset      >= 15000                 ~550 bytes
#define IRMP_SUPPORT_NOKIA_PROTOCOL             0       // Nokia                >= 10000                 ~300 bytes

// exotic protocols, enable here!               Enable  Remarks                 F_INTERRUPTS            Program Space
#define IRMP_SUPPORT_KATHREIN_PROTOCOL          0       // Kathrein             >= 10000                 ~200 bytes
#define IRMP_SUPPORT_NUBERT_PROTOCOL            0       // NUBERT               >= 10000                  ~50 bytes
#define IRMP_SUPPORT_BANG_OLUFSEN_PROTOCOL      0       // Bang & Olufsen       >= 10000                 ~200 bytes
#define IRMP_SUPPORT_RECS80_PROTOCOL            0       // RECS80 (SAA3004)     >= 15000                  ~50 bytes
#define IRMP_SUPPORT_RECS80EXT_PROTOCOL         0       // RECS80EXT (SAA3008)  >= 15000                  ~50 bytes
#define IRMP_SUPPORT_THOMSON_PROTOCOL           0       // Thomson              >= 10000                 ~250 bytes
#define IRMP_SUPPORT_NIKON_PROTOCOL             0       // NIKON camera         >= 10000                 ~250 bytes
#define IRMP_SUPPORT_NETBOX_PROTOCOL            0       // Netbox keyboard      >= 10000                 ~400 bytes (PROTOTYPE!)
#define IRMP_SUPPORT_FDC_PROTOCOL               0       // FDC3402 keyboard     >= 10000 (better 15000)  ~150 bytes (~400 in combination with RC5)
#define IRMP_SUPPORT_RCCAR_PROTOCOL             0       // RC Car               >= 10000 (better 15000)  ~150 bytes (~500 in combination with RC5)
#define IRMP_SUPPORT_RUWIDO_PROTOCOL            0       // RUWIDO, T-Home       >= 15000                 ~550 bytes
#define IRMP_SUPPORT_LEGO_PROTOCOL              0       // LEGO Power RC        >= 20000                 ~150 bytes

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
//#ifdef PIC_CCS_COMPILER                                 // PIC CCS Compiler:
//#define IRMP_PIN                                PIN_A2  // use PB4 as IR input on PIC
//#else                                                   // AVR:
#ifndef IRMP_PORT                                                               // IF not defined in board.h
#define IRMP_PORT                               PORTA
#define IRMP_DDR                                DDRA
#define IRMP_PIN                                PINA
#define IRMP_BIT                                2       // use PA2 as IR input on AVR
#endif

#define input(x)                                ((x) & (1 << IRMP_BIT))


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Set IRMP_LOGGING to 1 if want to log data to UART with 9600Bd
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRMP_LOGGING
#define IRMP_LOGGING                            0       // 1: log IR signal (scan), 0: do not (default)
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Set IRMP_PROTOCOL_NAMES to 1 if want to access protocol names (for logging etc), costs ~300 bytes RAM!
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define IRMP_PROTOCOL_NAMES                     0       // 1: access protocol names, 0: do not (default),

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Use Callbacks to indicate input signal
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define IRMP_USE_CALLBACK                       0       // flag: 0 = don't use callbacks, 1 = use callbacks, default is 0

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * DO NOT CHANGE THE FOLLOWING LINES !
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if IRMP_SUPPORT_SIEMENS_PROTOCOL == 1 && F_INTERRUPTS < 15000
#warning F_INTERRUPTS too low, SIEMENS protocol disabled (should be at least 15000)
#undef IRMP_SUPPORT_SIEMENS_PROTOCOL
#define IRMP_SUPPORT_SIEMENS_PROTOCOL           0
#endif

#if IRMP_SUPPORT_RUWIDO_PROTOCOL == 1 && F_INTERRUPTS < 15000
#warning F_INTERRUPTS too low, RUWIDO protocol disabled (should be at least 15000)
#undef IRMP_SUPPORT_RUWIDO_PROTOCOL
#define IRMP_SUPPORT_RUWIDO_PROTOCOL            0
#endif

#if IRMP_SUPPORT_RECS80_PROTOCOL == 1 && F_INTERRUPTS < 15000
#warning F_INTERRUPTS too low, RECS80 protocol disabled (should be at least 15000)
#undef IRMP_SUPPORT_RECS80_PROTOCOL
#define IRMP_SUPPORT_RECS80_PROTOCOL            0
#endif

#if IRMP_SUPPORT_RECS80EXT_PROTOCOL == 1 && F_INTERRUPTS < 15000
#warning F_INTERRUPTS too low, RECS80EXT protocol disabled (should be at least 15000)
#undef IRMP_SUPPORT_RECS80EXT_PROTOCOL
#define IRMP_SUPPORT_RECS80EXT_PROTOCOL         0
#endif

#if IRMP_SUPPORT_LEGO_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, LEGO protocol disabled (should be at least 20000)
#undef IRMP_SUPPORT_LEGO_PROTOCOL
#define IRMP_SUPPORT_LEGO_PROTOCOL              0
#endif

#if IRMP_SUPPORT_JVC_PROTOCOL == 1 && IRMP_SUPPORT_NEC_PROTOCOL == 0
#warning JVC protocol needs also NEC protocol, NEC protocol enabled
#undef IRMP_SUPPORT_NEC_PROTOCOL
#define IRMP_SUPPORT_NEC_PROTOCOL               1
#endif

#if IRMP_SUPPORT_NEC16_PROTOCOL == 1 && IRMP_SUPPORT_NEC_PROTOCOL == 0
#warning NEC16 protocol needs also NEC protocol, NEC protocol enabled
#undef IRMP_SUPPORT_NEC_PROTOCOL
#define IRMP_SUPPORT_NEC_PROTOCOL               1
#endif

#if IRMP_SUPPORT_NEC42_PROTOCOL == 1 && IRMP_SUPPORT_NEC_PROTOCOL == 0
#warning NEC42 protocol needs also NEC protocol, NEC protocol enabled
#undef IRMP_SUPPORT_NEC_PROTOCOL
#define IRMP_SUPPORT_NEC_PROTOCOL               1
#endif

#if F_INTERRUPTS > 20000
#error F_INTERRUPTS too high (should be not greater than 20000)
#endif

#endif /* _WC_IRMPCONFIG_H_ */
