#pragma once

#include <stdint.h>
#include <stm32f103xb.h>


#define ATOMIC_PRIO                      0x10


static inline void __basepriRestoreMem(uint8_t *val)
{
    __set_BASEPRI(*val);
}

static inline uint8_t __basepriSetMemRetVal(uint8_t prio)
{
    __set_BASEPRI_MAX(prio);
    return 1;
}

#define ATOMIC_BLOCK()  for ( uint8_t __basepri_save __attribute__ ((__cleanup__ (__basepriRestoreMem), __unused__)) = __get_BASEPRI(), \
                                     __ToDo = __basepriSetMemRetVal(ATOMIC_PRIO); __ToDo ; __ToDo = 0 )


