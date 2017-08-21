/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
/// \unit
///
/// !Purpose
///
/// Standard output methods for reporting debug information, warnings and
/// errors, which can be easily be turned on/off.
///
/// !Usage
/// -# Initialize the DBGU using TRACE_CONFIGURE() if you intend to eventually
///    disable ALL traces; otherwise use DBGU_Configure().
/// -# Uses the TRACE_DEBUG(), TRACE_INFO(), TRACE_WARNING(), TRACE_ERROR()
///    TRACE_FATAL() macros to output traces throughout the program.
/// -# Each type of trace has a level : Debug 5, Info 4, Warning 3, Error 2
///    and Fatal 1. Disable a group of traces by changing the value of 
///    TRACE_LEVEL during compilation; traces with a level bigger than TRACE_LEVEL 
///    are not generated. To generate no trace, use the reserved value 0.
/// -# Trace disabling can be static or dynamic. If dynamic disabling is selected
///    the trace level can be modified in runtime. If static disabling is selected
///    the disabled traces are not compiled.
///
/// !Trace level description
/// -# TRACE_DEBUG (5): Traces whose only purpose is for debugging the program, 
///    and which do not produce meaningful information otherwise.
/// -# TRACE_INFO (4): Informational trace about the program execution. Should  
///    enable the user to see the execution flow.
/// -# TRACE_WARNING (3): Indicates that a minor error has happened. In most case
///    it can be discarded safely; it may even be expected.
/// -# TRACE_ERROR (2): Indicates an error which may not stop the program execution, 
///    but which indicates there is a problem with the code.
/// -# TRACE_FATAL (1): Indicates a major error which prevents the program from going
///    any further.

//------------------------------------------------------------------------------

#ifndef TRACE_H
#define TRACE_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------


#include <stdio.h>

//------------------------------------------------------------------------------
//         Global Definitions
//------------------------------------------------------------------------------

/// Softpack Version
#define SOFTPACK_VERSION "1.5"

#define TRACE_LEVEL_DEBUG      5
#define TRACE_LEVEL_INFO       4
#define TRACE_LEVEL_WARNING    3
#define TRACE_LEVEL_ERROR      2
#define TRACE_LEVEL_FATAL      1
#define TRACE_LEVEL_NO_TRACE   0

// By default, all traces are output except the debug one.
#if !defined(TRACE_LEVEL)    
#define TRACE_LEVEL TRACE_LEVEL_NO_TRACE
#endif

// By default, trace level is static (not dynamic)
#if !defined(DYN_TRACES)
#define DYN_TRACES 0
#endif

#if defined(NOTRACE)
#error "Error: NOTRACE has to be not defined !"
#endif

#undef NOTRACE
#if (TRACE_LEVEL == TRACE_LEVEL_NO_TRACE)
#define NOTRACE
#endif

extern signed int _printf(const char *pFormat, ...);

//------------------------------------------------------------------------------
//         Global Macros
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Outputs a formatted string using <printf> if the log level is high
/// enough. Can be disabled by defining TRACE_LEVEL=0 during compilation.
/// \param format  Formatted string to output.
/// \param ...  Additional parameters depending on formatted string.
//------------------------------------------------------------------------------
#if defined(NOTRACE)

// Empty macro
#define TRACE_DEBUG(...)      { }
#define TRACE_INFO(...)       { }
#define TRACE_WARNING(...)    { }               
#define TRACE_ERROR(...)      { }
#define TRACE_FATAL(...)      { while(1); }

#define TRACE_DEBUG_WP(...)   { }
#define TRACE_INFO_WP(...)    { }
#define TRACE_WARNING_WP(...) { }
#define TRACE_ERROR_WP(...)   { }
#define TRACE_FATAL_WP(...)   { while(1); }

#elif (DYN_TRACES == 1)

// Trace output depends on traceLevel value
#define TRACE_DEBUG(...)      { if (traceLevel >= TRACE_LEVEL_DEBUG)   {_printf("-D- " __VA_ARGS__); } }
#define TRACE_INFO(...)       { if (traceLevel >= TRACE_LEVEL_INFO)    {_printf("-I- " __VA_ARGS__); } }
#define TRACE_WARNING(...)    { if (traceLevel >= TRACE_LEVEL_WARNING) {_printf("-W- " __VA_ARGS__); } }
#define TRACE_ERROR(...)      { if (traceLevel >= TRACE_LEVEL_ERROR)   {_printf("-E- " __VA_ARGS__); } }
#define TRACE_FATAL(...)      { if (traceLevel >= TRACE_LEVEL_FATAL)   {_printf("-F- " __VA_ARGS__); while(1); } }

#define TRACE_DEBUG_WP(...)   { if (traceLevel >= TRACE_LEVEL_DEBUG)   {_printf(__VA_ARGS__); } }
#define TRACE_INFO_WP(...)    { if (traceLevel >= TRACE_LEVEL_INFO)    {_printf(__VA_ARGS__); } }
#define TRACE_WARNING_WP(...) { if (traceLevel >= TRACE_LEVEL_WARNING) {_printf(__VA_ARGS__); } }
#define TRACE_ERROR_WP(...)   { if (traceLevel >= TRACE_LEVEL_ERROR)   {_printf(__VA_ARGS__); } }
#define TRACE_FATAL_WP(...)   { if (traceLevel >= TRACE_LEVEL_FATAL)   {_printf(__VA_ARGS__); while(1); } }

#else

// Trace compilation depends on TRACE_LEVEL value
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
#define TRACE_DEBUG(...)      {_printf("-D- " __VA_ARGS__); }
#define TRACE_DEBUG_WP(...)   {_printf(__VA_ARGS__); }
#else
#define TRACE_DEBUG(...)      { }
#define TRACE_DEBUG_WP(...)   { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_INFO)
#define TRACE_INFO(...)       {_printf("-I- " __VA_ARGS__); }
#define TRACE_INFO_WP(...)    {_printf(__VA_ARGS__); }
#else
#define TRACE_INFO(...)       { }
#define TRACE_INFO_WP(...)    { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_WARNING)
#define TRACE_WARNING(...)    {_printf("-W- " __VA_ARGS__); }
#define TRACE_WARNING_WP(...) {_printf(__VA_ARGS__); }
#else
#define TRACE_WARNING(...)    { }
#define TRACE_WARNING_WP(...) { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_ERROR)
#define TRACE_ERROR(...)      {_printf("-E- " __VA_ARGS__); }
#define TRACE_ERROR_WP(...)   {_printf(__VA_ARGS__); }
#else
#define TRACE_ERROR(...)      { }
#define TRACE_ERROR_WP(...)   { }
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_FATAL)
#define TRACE_FATAL(...)      {_printf("-F- " __VA_ARGS__); while(1); }
#define TRACE_FATAL_WP(...)   {_printf(__VA_ARGS__); while(1); }
#else
#define TRACE_FATAL(...)      { while(1); }
#define TRACE_FATAL_WP(...)   { while(1); }
#endif

#endif


//------------------------------------------------------------------------------
//         Exported variables
//------------------------------------------------------------------------------
// Depending on DYN_TRACES, traceLevel is a modifable runtime variable
// or a define
#if !defined(NOTRACE) && (DYN_TRACES == 1)
    extern unsigned int traceLevel;
#endif

#endif //#ifndef TRACE_H

