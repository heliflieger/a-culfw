/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

/** \file
 *
 *  This file contains macros for enhanced ISRs, by allowing special attributes to be applied to any given ISR.
 *
 *  \note Do not include this file directly, rather include the Common.h header file instead to gain this file's
 *        functionality.
 */

#ifndef __ISRMACRO_H__
#define __ISRMACRO_H__

	/* Preprocessor Checks and Defines: */
		#if defined(ISR)
			#undef ISR
		#endif
   
	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** Gives a default vector name more constant with the current avr-libc naming scheme for vectors. The 
			 *  BADISR_vect psudo-vector executes when an interrupt with no corresponding defined ISR handler is
			 *  fired.
			 */
			#define BADISR_vect __vector_default
			
			/** Inserts a Return From Interrupt ASM instruction (RETI) into the compiled binary at the point of
			 *  insertion. Most useful for ISRs declared with the ISR_NAKED attribute.
			 */
			#define reti() asm volatile ("RETI"::)

			/** Creates an ISR which serves as an alias to another defined ISR vector, compatible with all versions
			 *  of AVR-GCC. The aliased vector will incur a penalty of a JMP instruction when executed compared to
			 *  the non-aliased vector.
			 */
			#define ISR_ALIAS_COMPAT(vector, aliasof)      \
			   void vector (void) ISR_NAKED;               \
			   void vector (void) { asm volatile ("jmp " __replace_and_string(aliasof) ::); }

			/** Creates an ISR handler for the given vector. This macro takes in a valid vector name (see avr-libc
			 *  manual and an optional space seperated list of one or more ISR attributes. */
			#define ISR(vector, ...)                       \
			   void vector (void) ISR_BLOCK __VA_ARGS__;   \
			   void vector (void)

			#if (defined(__GNUC__) && (__GNUC__ > 3)) || defined(__DOXYGEN__)
				/** Indicates that the given ISR is of the non-blocking type, i.e. an ISR inside which interrupts
				 *  are enabled. This makes the given ISR itself interruptable, but should be used with caution to
				 *  prevent possible stack collisions from too many interrupts executing in a nested fashion.
				 */
				#define ISR_NOBLOCK    __attribute__((interrupt, used, externally_visible))
				
				/** Indicates that the given ISR is of the blocking type, i.e. is not itself interruptable. This is
				 *  the safest (and default) mode, as it prevents nested interrupts from crashing the stack but
				 *  it forces other pending interrupts to wait until the ISR completes before being serviced.
				 */
				#define ISR_BLOCK      __attribute__((signal, used, externally_visible))
				
				/** Indicates that the given ISR should have no automatically generated preample or postable, including
				 *  no stack or register saving and restoring. When used, the programmer is responsible for manually
				 *  managing the stack and registers to prevent corruption.
				 */
				#define ISR_NAKED      __attribute__((signal, naked, used, externally_visible))
				
				/** Aliases a given ISR vector to another ISR vector transparently, with no speed or size penalty.
				 *
				 * \warning This attribute is only compatible with GCC 4.X onwards - GCC 3.X or below should use the
				 *          ISR_ALIAS_COMPAT ISR macro instead.
				 */
				#define ISR_ALIASOF(v) __attribute__((alias(__replace_and_string(v)))) // GCC 4.2 and greater only!
			#else
				#define ISR_NOBLOCK   __attribute__((interrupt))
				#define ISR_BLOCK     __attribute__((signal))
				#define ISR_NAKED     __attribute__((signal, naked))
			#endif
			   
	/* Private Interface - For use in library only: */
	#if !defined(__DOXYGEN__)
		/* Macros: */
			#define __replace_and_string(name) #name
	#endif
	
#endif
