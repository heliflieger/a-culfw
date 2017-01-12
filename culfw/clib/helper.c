/* 
 * Copyright by B.Hempel
 * License: GPL v2
 */

#include "helper.h"

#include <stdint.h>                     // for uint8_t

/**
 * Description in header
 */
uint8_t mirror(uint8_t a)
{
   a = ((a >> 4) & 0x0F0F) | ((a << 4) & 0xF0F0);
   a = ((a >> 2) & 0x3333) | ((a << 2) & 0xCCCC);
   a = ((a >> 1) & 0x5555) | ((a << 1) & 0xAAAA);

   return a;
}


