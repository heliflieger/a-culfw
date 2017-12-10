/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * irsndconfig.h
 *
 * Copyright (c) 2010-2011 Frank Meyer - frank(at)fli4l.de
 *
 * $Id: irsndconfig.h,v 1.26 2011/10/15 14:31:38 odroegehorn Exp $
 *
 * ATMEGA88 @ 8 MHz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
 
 #include "board.h"

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change F_INTERRUPTS if you change the number of interrupts per second, F_INTERRUPTS should be in the range from 10000 to 20000, typically 15000
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef F_INTERRUPTS
#define F_INTERRUPTS                            15625   // interrupts per second
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change settings from 1 to 0 if you want to disable one or more encoders.
 * This saves program space.
 * 1 enable  decoder
 * 0 disable decoder
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

// typical protocols, disable here!             Enable  Remarks                 F_INTERRUPTS            Program Space
#define IRSND_SUPPORT_SIRCS_PROTOCOL            1       // Sony SIRCS           >= 10000                 ~150 bytes
#define IRSND_SUPPORT_NEC_PROTOCOL              1       // NEC + APPLE          >= 10000                 ~100 bytes
#define IRSND_SUPPORT_SAMSUNG_PROTOCOL          1       // Samsung + Samsung32  >= 10000                 ~300 bytes
#define IRSND_SUPPORT_MATSUSHITA_PROTOCOL       1       // Matsushita           >= 10000                 ~200 bytes
#define IRSND_SUPPORT_KASEIKYO_PROTOCOL         1       // Kaseikyo             >= 10000                 ~150 bytes
#define IRSND_SUPPORT_DENON_PROTOCOL            1       // DENON, Sharp         >= 10000                 ~200 bytes

// more protocols, enable here!                 Enable  Remarks                 F_INTERRUPTS            Program Space
#define IRSND_SUPPORT_RC5_PROTOCOL              1       // RC5                  >= 10000                 ~150 bytes
#define IRSND_SUPPORT_RC6_PROTOCOL              1       // RC6                  >= 10000                 ~250 bytes
#define IRSND_SUPPORT_RC6A_PROTOCOL             0       // RC6A                 >= 10000                 ~250 bytes
#define IRSND_SUPPORT_JVC_PROTOCOL              0       // JVC                  >= 10000                 ~150 bytes
#define IRSND_SUPPORT_NEC16_PROTOCOL            0       // NEC16                >= 10000                 ~150 bytes
#define IRSND_SUPPORT_NEC42_PROTOCOL            0       // NEC42                >= 10000                 ~150 bytes
#define IRSND_SUPPORT_IR60_PROTOCOL             0       // IR60 (SAB2008)       >= 10000                 DON'T CHANGE, NOT SUPPORTED YET!
#define IRSND_SUPPORT_GRUNDIG_PROTOCOL          1       // Grundig              >= 10000                 ~300 bytes
#define IRSND_SUPPORT_SIEMENS_PROTOCOL          0       // Siemens, Gigaset     >= 15000                 ~150 bytes
#define IRSND_SUPPORT_NOKIA_PROTOCOL            0       // Nokia                >= 10000                 ~400 bytes

// exotic protocols, enable here!               Enable  Remarks                 F_INTERRUPTS            Program Space
#define IRSND_SUPPORT_KATHREIN_PROTOCOL         0       // Kathrein             >= 10000                 DON'T CHANGE, NOT SUPPORTED YET!
#define IRSND_SUPPORT_NUBERT_PROTOCOL           0       // NUBERT               >= 10000                 ~100 bytes
#define IRSND_SUPPORT_BANG_OLUFSEN_PROTOCOL     0       // Bang&Olufsen         >= 10000                 ~250 bytes
#define IRSND_SUPPORT_RECS80_PROTOCOL           0       // RECS80               >= 20000                 ~100 bytes
#define IRSND_SUPPORT_RECS80EXT_PROTOCOL        0       // RECS80EXT            >= 20000                 ~100 bytes
#define IRSND_SUPPORT_THOMSON_PROTOCOL          0       // Thomson              >= 10000                 ~250 bytes
#define IRSND_SUPPORT_NIKON_PROTOCOL            0       // NIKON                >= 10000                 ~150 bytes
#define IRSND_SUPPORT_NETBOX_PROTOCOL           0       // Netbox keyboard      >= 10000                 DON'T CHANGE, NOT SUPPORTED YET!
#define IRSND_SUPPORT_FDC_PROTOCOL              0       // FDC IR keyboard      >= 10000 (better 15000)  ~150 bytes
#define IRSND_SUPPORT_RCCAR_PROTOCOL            0       // RC CAR               >= 10000 (better 15000)  ~150 bytes
#define IRSND_SUPPORT_RUWIDO_PROTOCOL           0       // RUWIDO, T-Home       >= 15000                 DON'T CHANGE, NOT SUPPORTED YET!
#define IRSND_SUPPORT_LEGO_PROTOCOL             0       // LEGO Power RC        >= 20000                 ~150 bytes


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * DO NOT CHANGE:
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define IRSND_OC2                               0       // OC2
#define IRSND_OC2A                              1       // OC2A
#define IRSND_OC2B                              2       // OC2B
#define IRSND_OC0                               3       // OC0
#define IRSND_OC0A                              4       // OC0A
#define IRSND_OC0B                              5       // OC0B

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Change hardware pin here:                    IRSND_OC2  = OC2  on ATmegas         supporting OC2,  e.g. ATmega8
 *                                              IRSND_OC2A = OC2A on ATmegas         supporting OC2A, e.g. ATmega88
 *                                              IRSND_OC2B = OC2B on ATmegas         supporting OC2B, e.g. ATmega88
 *                                              IRSND_OC0  = OC0  on ATmegas         supporting OC0,  e.g. ATmega162
 *                                              IRSND_OC0A = OC0A on ATmegas/ATtinys supporting OC0A, e.g. ATtiny84, ATtiny85
 *                                              IRSND_OC0B = OC0B on ATmegas/ATtinys supporting OC0B, e.g. ATtiny84, ATtiny85
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef IRSND_OCx                                                               //If not defined in board.h
#define IRSND_OCx                               IRSND_OC2A          // use OC2A
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Use Callbacks to indicate output signal or something else
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef IRTX_CALLBACKS
#define IRSND_USE_CALLBACK                      1       // flag: 0 = don't use callbacks, 1 = use callbacks, default is 0
#else
#define IRSND_USE_CALLBACK                      0       // flag: 0 = don't use callbacks, 1 = use callbacks, default is 0
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 *                              D O   N O T   C H A N G E   T H E   F O L L O W I N G   L I N E S   !
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if IRSND_SUPPORT_SIEMENS_PROTOCOL == 1 && F_INTERRUPTS < 15000
#warning F_INTERRUPTS too low, SIEMENS protocol disabled (should be at least 15000)
#undef IRSND_SUPPORT_SIEMENS_PROTOCOL
#define IRSND_SUPPORT_SIEMENS_PROTOCOL          0       // DO NOT CHANGE! F_INTERRUPTS too low!
#endif

#if IRSND_SUPPORT_RECS80_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, RECS80 protocol disabled (should be at least 20000)
#undef IRSND_SUPPORT_RECS80_PROTOCOL
#define IRSND_SUPPORT_RECS80_PROTOCOL           0       // DO NOT CHANGE! F_INTERRUPTS too low!
#endif

#if IRSND_SUPPORT_RECS80EXT_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, RECS80EXT protocol disabled (should be at least 20000)
#undef IRSND_SUPPORT_RECS80EXT_PROTOCOL
#define IRSND_SUPPORT_RECS80EXT_PROTOCOL        0       // DO NOT CHANGE! F_INTERRUPTS too low!
#endif

#if IRSND_SUPPORT_LEGO_PROTOCOL == 1 && F_INTERRUPTS < 20000
#warning F_INTERRUPTS too low, LEGO protocol disabled (should be at least 20000)
#undef IRSND_SUPPORT_LEGO_PROTOCOL
#define IRSND_SUPPORT_LEGO_PROTOCOL             0       // DO NOT CHANGE! F_INTERRUPTS too low!
#endif
