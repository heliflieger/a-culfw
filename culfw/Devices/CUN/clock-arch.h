#ifndef __CLOCK_ARCH_H__
#define __CLOCK_ARCH_H__

#include <avr/io.h>
#include "board.h"

// all done in clib/clock.h
#include "clock.h"

// ticks per second 
#define CLOCK_CONF_SECOND (clock_time_t)125

#endif /* __CLOCK_ARCH_H__ */
