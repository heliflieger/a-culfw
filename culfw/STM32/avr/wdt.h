

#ifndef __WDT_H_
#define __WDT_H_ 1

#include <stdint.h>

#define WDTO_15MS 19
#define WDTO_2S   2500

void wdt_reset(void);
void wdt_enable(uint16_t ms);
void wdt_disable();

#endif /* __PGMSPACE_H_ */
