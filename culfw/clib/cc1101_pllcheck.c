#include "board.h"
 
#include <avr/io.h>
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "fncollection.h"

#include "cc1101_pllcheck.h"


#ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
uint8_t disable_PLLNOLOCK_MSG = 0;				// do not display PLLNOLOCK related errors if > 0
#endif


#ifdef HAS_CC1101_PLL_LOCK_CHECK
uint8_t
cc1101_checkPLL(void)  // noansi: returns 0 on success
{
	// check PLL Lock and try to calibrate. Possibly restoring a saved calibration could be faster...
	if (cc1100_readReg( CC1100_FSCAL1 ) != 0x3f) return 0;			// PLL in Lock as described in CC1101 doc and errata

	// try to recover as no PLL Lock as described in CC1101 doc and errata
	ccStrobe( CC1100_SIDLE );

#ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG
# ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
	if (!disable_PLLNOLOCK_MSG) DS_P( PSTR( "PLL0\r\n" ) );			// PLL0 -> PLL Lock problem found. Message gives enough time to switch to IDLE state
	else ccStrobe( CC1100_SNOP );									// wait a moment to give time to switch to IDLE state, this should be enough
# else
	DS_P( PSTR( "PLL0\r\n" ) );										// PLL0 -> PLL Lock problem found. Message gives enough time to switch to IDLE state
# endif
#else
	ccStrobe( CC1100_SNOP );										// wait a moment to give time to switch to IDLE state, this should be enough
#endif
	//cc1101_wait_state(MARCSTATE_IDLE, 1);							// give a litle bit time, normal RX to IDLE should take about 0.1us with ASK, TX to idle about 1us with ASK

	ccStrobe( CC1100_SCAL );										// Try Calibration

	// wait idle state -> calibration finished, if no idle in between is reported here as it may be possible with the state returned by command strobe
	cc1101_wait_state(MARCSTATE_IDLE, 4);

	// check PLL lock again
	if (cc1100_readReg( CC1100_FSCAL1 ) != 0x3f) return 0;			// PLL in Lock as described in CC1101 doc and errata, we recovered

#ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG
# ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
	if (!disable_PLLNOLOCK_MSG) DS_P( PSTR( "PLL1\r\n" ) );			// PLL1 -> Failed to recover from PLL Lock problem
# else
	DS_P( PSTR( "PLL1\r\n" ) );										// PLL1 -> Failed to recover from PLL Lock problem
# endif
#endif

	return 1; // error, no PLL Lock
}
#endif //HAS_CC1101_PLL_LOCK_CHECK


#ifdef HAS_CC1101_WAIT_STATE
uint8_t
cc1101_wait_state(uint8_t state, uint8_t poll_us)
{
	uint8_t l;

	l = 255;
	do
	{
		if (cc1100_readReg( CC1100_MARCSTATE ) == state) return 0;	// ok, state reached
		my_delay_us( poll_us );
	}
	while (l--);

	return 1; // error, state not reached
}
#endif //HAS_CC1101_WAIT_STATE


#ifdef HAS_CC1101_PLL_LOCK_CHECK_RX
uint8_t
cc1101_toRX_PLLcheck(void)
{
	uint8_t n;

	// set RX with check/try PLL is in Lock in RX
	n = 2; // noansi: Try to set RX several times before giving up
	do {
		// enable RX
		while ((ccStrobe( CC1100_SRX ) & CC1100_STATUS_STATE_BM) != 0x10);	// Set RX until Status Byte indicates RX

		if (!cc1101_checkPLL())	return 0; // PLL Lock ok

	} while (--n);

#ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_RXTX
# ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
	if (!disable_PLLNOLOCK_MSG) DS_P( PSTR( "PLL2\r\n" ) );					// PLL2 -> Failed PLL Lock while switching to RX
# else
	DS_P( PSTR( "PLL2\r\n" ) );												// PLL2 -> Failed PLL Lock while switching to RX
# endif
#endif

	return 1;
}
#endif //HAS_CC1101_PLL_LOCK_CHECK_RX


#ifdef HAS_CC1101_PLL_LOCK_CHECK_TX
uint8_t
cc1101_toTX_PLLcheck(void)
{
	uint8_t n;

	// set TX with check/try PLL is in Lock in TX
	n = 2; // noansi: Try to set TX several times before giving up
	do {
		// enable TX
		while ((ccStrobe( CC1100_STX ) & CC1100_STATUS_STATE_BM) != 0x20);	// Set TX until Status Byte indicates TX

		if (!cc1101_checkPLL()) return 0; // PLL Lock ok

	} while (--n);

#ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_RXTX
# ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
	if (!disable_PLLNOLOCK_MSG) DS_P( PSTR( "PLL3\r\n" ) );					// PLL3 -> Failed PLL Lock while switching to TX
# else
	DS_P( PSTR( "PLL3\r\n" ) );												// PLL3 -> Failed PLL Lock while switching to TX
# endif
#endif

	return 1;
}
#endif //HAS_CC1101_PLL_LOCK_CHECK_TX


#ifdef HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
// check if stuck in RX state without PLL Lock and try to recover
void
cc1101_RX_check_PLL_wait_task(void)
{
  if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_RX)
  {
	// try init or recalibration, if stuck in RX State with no PLL Lock as seen in extended read timeout logging
	if (cc1100_readReg( CC1100_FSCAL1 ) == 0x3f)							// no PLL Lock?  as described in CC1101 errata
	{
		cc1101_checkPLL();													// try calibration to recover, takes about 735us
		while ((ccStrobe( CC1100_SRX ) & CC1100_STATUS_STATE_BM) != 0x10);	// Set RX again until Status Byte indicates RX, this will take up to 799us depending on AUTOCAL setting! see cc1101 doc
	}
  }
}
#endif //HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
  

#ifdef HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_NOWAIT
// check if stuck in RX state without PLL Lock and try to recover, don't wait for RX, just strobe
void
cc1101_RX_check_PLL_nowait_task(void)
{
  if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_RX)
  {
	// try init or recalibration, if stuck in RX State with no PLL Lock as seen in extended read timeout logging
	if (cc1100_readReg( CC1100_FSCAL1 ) == 0x3f)							// no PLL Lock?  as described in CC1101 errata
	{
		cc1101_checkPLL();													// try calibration to recover, takes about 735us
		ccStrobe( CC1100_SRX );												// Set RX, other tasks will give the wait time to fully switch to RX in background, up to 799us needed, depending on AUTOCAL setting! see doc
	}
  }
}
#endif //HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_NOWAIT


#ifdef HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_WAIT
// check if stuck in RX state without PLL Lock and try to recover
void
cc1101_TX_check_PLL_wait_task(void)
{
  if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TX)
  {
	// try init or recalibration, if stuck in RX State with no PLL Lock as seen in extended read timeout logging
	if (cc1100_readReg( CC1100_FSCAL1 ) == 0x3f)							// no PLL Lock?  as described in CC1101 errata
	{
		cc1101_checkPLL();													// try calibration to recover, takes about 735us
		while ((ccStrobe( CC1100_STX ) & CC1100_STATUS_STATE_BM) != 0x20);	// Set RX again until Status Byte indicates RX, this will take up to 799us depending on AUTOCAL setting! see cc1101 doc
	}
  }
}
#endif //HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_WAIT
  

#ifdef HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_NOWAIT
// check if stuck in RX state without PLL Lock and try to recover, don't wait for RX, just strobe
void
cc1101_TX_check_PLL_nowait_task(void)
{
  if (cc1100_readReg( CC1100_MARCSTATE ) == MARCSTATE_TX)
  {
	// try init or recalibration, if stuck in RX State with no PLL Lock as seen in extended read timeout logging
	if (cc1100_readReg( CC1100_FSCAL1 ) == 0x3f)							// no PLL Lock?  as described in CC1101 errata
	{
		cc1101_checkPLL();													// try calibration to recover, takes about 735us
		ccStrobe( CC1100_STX );												// Set RX, other tasks will give the wait time to fully switch to RX in background, up to 799us needed depending on AUTOCAL setting! see doc
	}
  }
}
#endif //HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_NOWAIT
