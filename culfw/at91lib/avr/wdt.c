
#include "wdt.h"
#include <board.h>

#define WATCHDOG_KEY (0xA5 << 24)

void wdt_reset(void) {
	AT91C_BASE_WDTC->WDTC_WDCR = WATCHDOG_KEY | AT91C_WDTC_WDRSTT;
}


void wdt_enable(uint16_t ms) {
	int period = (ms * 256) / 1000;
	 AT91C_BASE_WDTC->WDTC_WDMR =  AT91C_WDTC_WDRSTEN |        // enable reset on timeout
	                                AT91C_WDTC_WDDBGHLT |       // respect debug mode
	                                AT91C_WDTC_WDIDLEHLT |      // respect idle mode
	                                ((period << 16 ) & AT91C_WDTC_WDD) | // delta is as wide as the period, so we can restart anytime
	                                (period & AT91C_WDTC_WDV);  // set the period
}

void wdt_disable() {
  AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
}
