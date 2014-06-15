#ifndef CC1101_PLLCHECK_H
#define CC1101_PLLCHECK_H

//
// functions to use with TI cc1101 transceiver
// 
// checking for PLL lock, needed as some cc1101 chips sometimes do not settle or keep in PLL lock
//

// to use the functions to your needs just define as desired:
//
// HAS_CC1101_WAIT_STATE					to compile cc1101_wait_state function, suited for general use with cc1101
// HAS_CC1101_PLL_LOCK_CHECK				to compile cc1101_checkPLL function(s)
// HAS_CC1101_PLL_LOCK_CHECK_MSG			to display error messages related to PLL lock problems (PPL0 or PLL1)
// HAS_CC1101_PLL_LOCK_CHECK_MSG_RXTX		to display error messages related to PLL lock problems in RX or TX check (PLL2 or PLL3)
// HAS_CC1101_PLL_LOCK_CHECK_MSG_SW			to have the messages switchable off and on
// HAS_CC1101_PLL_LOCK_CHECK_RX				to compile cc1101_toRX_PLLcheck function(s)
// HAS_CC1101_PLL_LOCK_CHECK_TX				to compile cc1101_toTX_PLLcheck function(s)
// HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT	to compile cc1101_RX_check_PLL_wait_task function(s)
// HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_NOWAIT	to compile cc1101_RX_check_PLL_nowait_task function(s)
// HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_WAIT	to compile cc1101_TX_check_PLL_wait_task function(s)
// HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_NOWAIT	to compile cc1101_TX_check_PLL_nowait_task function(s)

// check of dependencies
#if defined(HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT) || defined(HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_NOWAIT)
# ifndef HAS_CC1101_PLL_LOCK_CHECK
#  define HAS_CC1101_PLL_LOCK_CHECK
# endif
#endif

#if defined(HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_WAIT) || defined(HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_NOWAIT)
# ifndef HAS_CC1101_PLL_LOCK_CHECK
#  define HAS_CC1101_PLL_LOCK_CHECK
# endif
#endif

#if defined(HAS_CC1101_PLL_LOCK_CHECK_RX) || defined(HAS_CC1101_PLL_LOCK_CHECK_TX)
# ifndef HAS_CC1101_PLL_LOCK_CHECK
#  define HAS_CC1101_PLL_LOCK_CHECK
# endif
#endif

#ifdef HAS_CC1101_PLL_LOCK_CHECK
# ifndef HAS_CC1101_WAIT_STATE
#  define HAS_CC1101_WAIT_STATE
# endif
#endif

#if defined(HAS_CC1101_PLL_LOCK_CHECK_MSG_RXTX) && !(defined(HAS_CC1101_PLL_LOCK_CHECK_RX) || defined(HAS_CC1101_PLL_LOCK_CHECK_TX))
# undef HAS_CC1101_PLL_LOCK_CHECK_MSG_RXTX
#endif

#if defined(HAS_CC1101_PLL_LOCK_CHECK_MSG) && !defined(HAS_CC1101_PLL_LOCK_CHECK)
# undef HAS_CC1101_PLL_LOCK_CHECK_MSG
#endif

#if defined(HAS_CC1101_PLL_LOCK_CHECK_MSG_SW) && !(defined(HAS_CC1101_PLL_LOCK_CHECK_MSG) || defined(HAS_CC1101_PLL_LOCK_CHECK_MSG_RXTX))
# undef HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
#endif


#ifdef HAS_CC1101_PLL_LOCK_CHECK_MSG_SW
 extern uint8_t disable_PLLNOLOCK_MSG;				// do not display PLLNOLOCK related errors if > 0. you have to build a device function to switch it off. rf_asksin.c will get/has one with AP1 / AP0 
# define SwitchOffOn_PLLnoLock_messages(a) (disable_PLLNOLOCK_MSG = a)
# define SwitchOff_PLLnoLock_messages() (disable_PLLNOLOCK_MSG = 1)
# define SwitchOn_PLLnoLock_messages() (disable_PLLNOLOCK_MSG = 0)
#endif

#ifdef HAS_CC1101_PLL_LOCK_CHECK
 extern uint8_t cc1101_checkPLL(void);				// check CC1101 PLL lock with recalibration if necessary, returns 0 if PLL is in lock
#endif

#ifdef HAS_CC1101_PLL_LOCK_CHECK_RX
 extern uint8_t cc1101_toRX_PLLcheck(void);			// set CC1101 to RX with recalibration if necessary, returns 0 on success
#endif

#ifdef HAS_CC1101_PLL_LOCK_CHECK_TX
 extern uint8_t cc1101_toTX_PLLcheck(void);			// set CC1101 to TX with recalibration if necessary, returns 0 on success
#endif

#ifdef HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
 extern void cc1101_RX_check_PLL_wait_task(void);	// check for CC1101 PLL Lock and try recalibration, if not. Wait for RX, if recalibrating.
#endif

#ifdef HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_NOWAIT
 extern void cc1101_RX_check_PLL_nowait_task(void);	// check for CC1101 PLL Lock and try recalibration, if not. Don't wait for RX, if recalibrating.
#endif

#ifdef HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_WAIT
 extern void cc1101_TX_check_PLL_wait_task(void);	// check for CC1101 PLL Lock and try recalibration, if not. Wait for TX, if recalibrating.
#endif

#ifdef HAS_CC1101_TX_PLL_LOCK_CHECK_TASK_NOWAIT
 extern void cc1101_TX_check_PLL_nowait_task(void);	// check for CC1101 PLL Lock and try recalibration, if not. Don't wait for TX, if recalibrating.
#endif

#ifdef HAS_CC1101_WAIT_STATE
 extern uint8_t cc1101_wait_state(uint8_t state, uint8_t poll_us);	// wait for a CC1101 MARCSTATE to be reached within 256*poll_us, returns 0 if state is reached in time
#endif

/******************************************************************************/
#endif
