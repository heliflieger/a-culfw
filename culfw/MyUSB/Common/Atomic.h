/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

/** \file
 *
 *  This file contains macros for the generation of atomic and non-atomic blocks, where interrupts are explicitly
 *  enabled or disabled during the block's execution to prevent data corruption from simultaneous access.
 *
 *  \note Do not include this file directly, rather include the Common.h header file instead to gain this file's
 *        functionality.
 */

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

	/* Includes: */
		#include <avr/io.h>
		#include <avr/interrupt.h>
		#include <avr/version.h>

	/* Preprocessor Checks: */
		#if !defined(__COMMON_H__)
			#error Do not include this file directly. Include MyUSB/Common/Common.h instead to gain this functionality.
		#endif

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** Creates a statement block whose contents are guaranteed to be atomic (i.e. interrupts disabled).
			 *  The block's exit mode can be specified as either ATOMIC_RESTORESTATE or ATOMIC_FORCEON. Atomic
			 *  blocks made with this macro may be exited via any legal fashion - return, break, goto, etc.
			 *  without any special consideratons.
			 */
			#define ATOMIC_BLOCK(exitmode)     for ( exitmode, __ToDo = __iCliRetVal(); __ToDo ; __ToDo = 0 )

			/** Exit mode for ATOMIC_BLOCK(). When this exit mode is used, the current global interrupt state
			 *  (enabled or disabled) is saved before being disabled, and restored after the atomic block exits.
			 */
			#define ATOMIC_RESTORESTATE        uint8_t sreg_save __attribute__((__cleanup__(__iRestore))) = SREG

			/** Exit mode for ATOMIC_BLOCK(). When this exit mode is used, the global interrupt state is disabled for
			 *  the duration of the block, and enabled (under all conditions) when the block exits.
			 */
			#define ATOMIC_FORCEON             uint8_t sreg_save __attribute__((__cleanup__(__iSeiParam))) = 0
			
			/** Creates a statement block whose contents are NOT guaranteed to be atomic (i.e. interrupts enabled).
			 *  The block's exit mode can be specified as either NONATOMIC_RESTORESTATE or ATOMIC_FORCEOFF. This
			 *  macro is most useful when nested inside a normal ATOMIC_BLOCK, for sections where atomic access is
			 *  not explicitly required. Blocks created with this macro may be exited via any legal fashion - return,
			 *  break, goto, etc. without any special considerations.
			 */
			#define NON_ATOMIC_BLOCK(exitmode) for ( exitmode, __ToDo = __iSeiRetVal(); __ToDo ; __ToDo = 0 )
			
			/** Exit mode for NONATOMIC_BLOCK(). When this exit mode is used, the current global interrupt state
			 *  (enabled or disabled) is saved before being enabled, and restored after the non atomic block exits.
			 */
			#define NONATOMIC_RESTORESTATE     uint8_t sreg_save __attribute__((__cleanup__(__iRestore)))  = SREG

			/** Exit mode for NONATOMIC_BLOCK(). When this exit mode is used, the global interrupt state is enabled for
			 *  the duration of the block, and disabled (under all conditions) when the block exits.
			 */
			#define NONATOMIC_FORCEOFF         uint8_t sreg_save __attribute__((__cleanup__(__iCliParam))) = 0

	/* Private Interface - For use in library only: */
	#if !defined(__DOXYGEN__)
		/* Inline Functions: */
			static inline uint8_t __iSeiRetVal(void)
			{
				sei();
				return 1;
			}
  
			static inline uint8_t __iCliRetVal(void)
			{
				cli();
				return 1;
			}
  
			static inline void __iSeiParam(const uint8_t* __s) 
			{
				sei();
				asm volatile ("" ::: "memory");
				(void)__s;
			}

			static inline void __iCliParam(const uint8_t* __s)
			{
				cli();
				asm volatile ("" ::: "memory");
				(void)__s;
			}

			static inline void __iRestore(const  uint8_t* __s)
			{
				SREG = *__s;
				asm volatile ("" ::: "memory");
			}
	#endif
	
	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif
		
#endif
