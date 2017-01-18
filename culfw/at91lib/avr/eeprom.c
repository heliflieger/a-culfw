#include "eeprom.h"

#include <dbgu/dbgu.h>
#include <utility/trace.h>
#include <board.h>

#ifdef HM_CFG
#undef USE_DATAFLASH
#endif

#ifdef USE_DATAFLASH
#include <pio/pio.h>
#include <aic/aic.h>
#include <utility/assert.h>
#include <utility/trace.h>
#include <spi-flash/at45.h>

/// SPI driver instance.
static Spid spid;

/// AT45 driver instance.
static At45 at45;

/// Pins used by the application.
static const Pin pins[]  = {BOARD_AT45_A_SPI_PINS, BOARD_AT45_A_NPCS_PIN};

/// SPI clock frequency, in Hz.
#define SPCK        1000000

#define FLASHPAGE	3

#else

#define NUM_PAGES	64
//(AT91C_IFLASH_NB_OF_PAGES / AT91C_IFLASH_NB_OF_LOCK_BITS)
#define FIRSTPAGE	(AT91C_IFLASH_NB_OF_PAGES - NUM_PAGES -1)
#define EE_BASE		(AT91C_IFLASH+(FIRSTPAGE * AT91C_IFLASH_PAGE_SIZE))

#define FMCN_BITS(mck)      (ROUND((mck) / 100000) << 16)
#define ROUND(n)    ((((n) % 10) >= 5) ? (((n) / 10) + 1) : ((n) / 10))
#define FMCN_FLASH(mck)         ((((mck) / 2000000) * 3) << 16)

typedef struct _EE_MEMORY {
	union {
	uint32_t value[64];
	uint8_t byte[256];
	};

} EE_MEMORY;

typedef struct _uint32u8_t {
	union {
	uint32_t dbword;
	uint8_t byte[4];
	};

} uint32u8_t;

//#define USE_RAM

#if defined(USE_RAM)
uint8_t ee_ram[256];

#endif

#endif



//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------
#ifdef USE_DATAFLASH
//------------------------------------------------------------------------------
/// SPI interrupt handler. Invokes the SPI driver handler to check for pending
/// interrupts.
//------------------------------------------------------------------------------
static void ISR_Spi(void)
{
    SPID_Handler(&spid);
}

//------------------------------------------------------------------------------
/// Retrieves and returns the At45 current status, or 0 if an error
/// happened.
/// \param pAt45  Pointer to a At45 driver instance.
//------------------------------------------------------------------------------
static unsigned char AT45_GetStatus(At45 *pAt45)
{
    unsigned char error;
    unsigned char status;

    // Sanity checks
    ASSERT(pAt45, "-F- AT45_GetStatus: pAt45 is null\n\r");

    // Issue a status register read command
    error = AT45_SendCommand(pAt45, AT45_STATUS_READ, 1, &status, 1, 0, 0, 0);
    ASSERT(!error, "-F- AT45_GetStatus: Failed to issue command.\n\r");

    // Wait for command to terminate
    while (AT45_IsBusy(pAt45));

    return status;
}

//------------------------------------------------------------------------------
/// Waits for the At45 to be ready to accept new commands.
/// \param pAt45  Pointer to a At45 driver instance.
//------------------------------------------------------------------------------
static void AT45_WaitReady(At45 *pAt45)
{
    unsigned char ready = 0;

    // Sanity checks
    ASSERT(pAt45, "-F- AT45_WaitUntilReady: pAt45 is null\n\r");

    // Poll device until it is ready
    while (!ready) {

        ready = AT45_STATUS_READY(AT45_GetStatus(pAt45));
    }
}

//------------------------------------------------------------------------------
/// Reads and returns the JEDEC identifier of a At45.
/// \param pAt45  Pointer to a At45 driver instance.
//------------------------------------------------------------------------------
static unsigned int AT45_GetJedecId(At45 *pAt45)
{
    unsigned char error;
    unsigned int id;

    // Sanity checks
    ASSERT(pAt45, "-F- AT45_GetJedecId: pAt45 is null\n\r");

    // Issue a manufacturer and device ID read command
    error = AT45_SendCommand(pAt45, AT45_ID_READ, 1, (void *) &id, 4, 0, 0, 0);
    ASSERT(!error, "-F- AT45_GetJedecId: Could not issue command.\n\r");

    // Wait for transfer to finish
    while (AT45_IsBusy(pAt45));

    return id;
}

//------------------------------------------------------------------------------
/// Reads data from the At45 inside the provided buffer. Since a continuous
/// read command is used, there is no restriction on the buffer size and read
/// address.
/// \param pAt45  Pointer to a At45 driver instance.
/// \param pBuffer  Data buffer.
/// \param size  Number of bytes to read.
/// \param address  Address at which data shall be read.
//------------------------------------------------------------------------------
void AT45_Read(
    At45 *pAt45,
    unsigned char *pBuffer,
    unsigned int size,
    unsigned int address)
{
    unsigned char error;

    // Sanity checks
    ASSERT(pAt45, "-F- AT45_Read: pAt45 is null\n\r");
    ASSERT(pBuffer, "-F- AT45_Read: pBuffer is null\n\r");

    // Issue a continuous read array command
    error = AT45_SendCommand(pAt45, AT45_CONTINUOUS_READ_LEG, 8, pBuffer, size, address, 0, 0);
    ASSERT(!error, "-F- AT45_Read: Failed to issue command\n\r");

    // Wait for the read command to execute
    while (AT45_IsBusy(pAt45));
}

//------------------------------------------------------------------------------
/// Reads data from the At45 inside the provided buffer. Since a continuous
/// read command is used, there is no restriction on the buffer size and read
/// address.
/// \param pAt45  Pointer to a At45 driver instance.
/// \param pBuffer  Data buffer.
/// \param size  Number of bytes to read.
/// \param address  Address at which data shall be read.
//------------------------------------------------------------------------------
void AT45_Read_Security(
    At45 *pAt45,
    unsigned char *pBuffer,
    unsigned int size,
    unsigned int address)
{
    unsigned char error;

    // Sanity checks
    ASSERT(pAt45, "-F- AT45_Read: pAt45 is null\n\r");
    ASSERT(pBuffer, "-F- AT45_Read: pBuffer is null\n\r");

    // Issue a continuous read array command
    error = AT45_SendCommand(pAt45, 0x77, 4, pBuffer, size, address, 0, 0);
    ASSERT(!error, "-F- AT45_Read: Failed to issue command\n\r");

    // Wait for the read command to execute
    while (AT45_IsBusy(pAt45));
}

//------------------------------------------------------------------------------
/// Writes data on the At45 at the specified address. Only one page of
/// data is written that way; if the address is not at the beginning of the
/// page, the data is written starting from this address and wraps around to
/// the beginning of the page.
/// \param pAt45  Pointer to a At45 driver instance.
/// \param pBuffer  Buffer containing the data to write.
/// \param size  Number of bytes to write.
/// \param address  Destination address on the At45.
//------------------------------------------------------------------------------
void AT45_Write(
    At45 *pAt45,
    unsigned char *pBuffer,
    unsigned int size,
    unsigned int address)
{
    unsigned char error;

    // Sanity checks
    ASSERT(pAt45, "-F- AT45_Write: pAt45 is null.\n\r");
    ASSERT(pBuffer, "-F- AT45_Write: pBuffer is null.\n\r");
    ASSERT(size <= pAt45->pDesc->pageSize, "-F- AT45_Write: Size too big\n\r");

    // Read flash page to buffer 1 command
    error = AT45_SendCommand(pAt45, AT45_PAGE_BUF1_TX, 4, pBuffer, 0, address, 0, 0);
    // Wait until the command is sent
    while (AT45_IsBusy(pAt45));

    // Wait until the At45 becomes ready again
    AT45_WaitReady(pAt45);

    // Issue a page write through buffer 1 command
    error = AT45_SendCommand(pAt45, AT45_PAGE_WRITE_BUF1, 4, pBuffer, size, address, 0, 0);
    ASSERT(!error, "-F- AT45_Write: Could not issue command.\n\r");

    // Wait until the command is sent
    while (AT45_IsBusy(pAt45));

    // Wait until the At45 becomes ready again
    AT45_WaitReady(pAt45);
}

/*
//------------------------------------------------------------------------------
/// Erases a page of data at the given address in the At45.
/// \param pAt45  Pointer to a At45 driver instance.
/// \param address  Address of page to erase.
//------------------------------------------------------------------------------
static void AT45_Erase(At45 *pAt45, unsigned int address)
{
    unsigned char error;

    // Sanity checks
    ASSERT(pAt45, "-F- AT45_Erase: pAt45 is null\n\r");

    // Issue a page erase command.
    error = AT45_SendCommand(pAt45, AT45_PAGE_ERASE, 4, 0, 0, address, 0, 0);
    ASSERT(!error, "-F- AT45_Erase: Could not issue command.\n\r");

    // Wait for end of transfer
    while (AT45_IsBusy(pAt45));

    // Poll until the At45 has completed the erase operation
    AT45_WaitReady(pAt45);
}
*/

#else

__attribute__ ((section (".ramfunc")))
unsigned char EFC_PerformCommand1(
    unsigned char command,
    unsigned short argument)
{
    unsigned int status;
    uint32_t mask;

    mask=AT91C_BASE_AIC->AIC_IMR;
    AT91C_BASE_AIC->AIC_IDCR =0xffffffff;

    // Set FMCN
    switch (command) {

        case AT91C_MC_FCMD_LOCK:
        case AT91C_MC_FCMD_UNLOCK:
        case AT91C_MC_FCMD_SET_SECURITY:
        	AT91C_BASE_MC->MC_FMR = (AT91C_BASE_MC->MC_FMR & ~AT91C_MC_FMCN) | FMCN_BITS(BOARD_MCK) | AT91C_MC_FWS_1FWS;
            break;

        case AT91C_MC_FCMD_START_PROG:
        case AT91C_MC_FCMD_ERASE_ALL:
        	AT91C_BASE_MC->MC_FMR = (AT91C_BASE_MC->MC_FMR & ~AT91C_MC_FMCN) | FMCN_FLASH(BOARD_MCK) | AT91C_MC_FWS_1FWS;
            break;
    }


    AT91C_BASE_MC->MC_FCR = (0x5A << 24) | (argument << 8) | command;
    do {

        status = AT91C_BASE_MC->MC_FSR;
    }
    while ((status & AT91C_MC_FRDY) == 0);

    AT91C_BASE_AIC->AIC_IECR=mask;

    return (status & (AT91C_MC_PROGE | AT91C_MC_LOCKE));
}

void init_page(int16_t oldpage, int16_t newpage) {

	unsigned int i;
	EE_MEMORY *ee_mem = (void*)EE_BASE;


	for (i = 0; i < (AT91C_IFLASH_PAGE_SIZE/4) ;i++) {
		if(oldpage < 0)
			ee_mem[newpage].value[i] = 0xFFFFFFFF;
		else
			ee_mem[newpage].value[i] = ee_mem[oldpage].value[i];
	}

	i=(AT91C_IFLASH_PAGE_SIZE/4)-1;

	//Write magic Byte
	if(oldpage < 0)
		ee_mem[newpage].value[i] = 0xAAFFFFFF;
	else
		ee_mem[newpage].value[i] = (ee_mem[oldpage].value[i] & 0x00ffffff) | 0xAAFFFFFF;

}

void clean_pages(uint16_t active_page) {

	unsigned int i;
	EE_MEMORY *ee_mem = (void*)EE_BASE;
	uint16_t page;

	for(page=0;page<NUM_PAGES;page++) {

		if(page != active_page) {
			for (i = 0; i < (AT91C_IFLASH_PAGE_SIZE/4) ;i++) {
					ee_mem[page].value[i] = 0xFFFFFFFF;
			}
			AT91C_BASE_MC->MC_FMR &= ~AT91C_MC_NEBP;
			EFC_PerformCommand1(AT91C_MC_FCMD_START_PROG, FIRSTPAGE + page);
		}

	}

}

void disable_page(uint16_t page) {

	EE_MEMORY *ee_mem = (void*)EE_BASE;

	//fill Buffer
	for (unsigned int i = 0; i < (AT91C_IFLASH_PAGE_SIZE/4) ;i++) {
		ee_mem[page].value[i]=0xFFFFFFFF;
	}

	//clear last byte of page
	ee_mem[page].value[(AT91C_IFLASH_PAGE_SIZE/4)-1] = 0x00FFFFFF;

	//no erase before write and write page
	AT91C_BASE_MC->MC_FMR |= AT91C_MC_NEBP;
	EFC_PerformCommand1(AT91C_MC_FCMD_START_PROG, FIRSTPAGE + page);

}

int16_t get_active_page(void) {

	EE_MEMORY *ee_mem = (void*)EE_BASE;

	for (unsigned int i = 0; i < NUM_PAGES ;i++) {
		if (ee_mem[i].byte[AT91C_IFLASH_PAGE_SIZE-1] == 0xaa) {
			return i;
		}
	}
	return -1;
}

int16_t get_free_page(void) {

	EE_MEMORY *ee_mem = (void*)EE_BASE;

	for (unsigned int i = 0; i < NUM_PAGES ;i++) {
		if (ee_mem[i].byte[AT91C_IFLASH_PAGE_SIZE-1] == 0xff) {
			return i;
		}
	}
	return -1;
}
#endif

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------


void eeprom_write_byte(uint8_t *p, uint8_t v) {

	uint32_t c = (uint32_t)p;
	TRACE_INFO("EE_W A:%02x V:%02x   ",(uint8_t)c,v);

	if(eeprom_read_byte(p) == v) {
		return;
	}

#ifdef USE_RAM
	ee_ram[c]=v;
	return;
#else
#ifdef USE_DATAFLASH

	c += FLASHPAGE * AT45_PageSize(&at45);

	AT45_Write(&at45, &v,1,c);

	c = FLASHPAGE * AT45_PageSize(&at45);

	AT45_Read(&at45, &v,1,c);
	if(v != 0x01) {
		TRACE_INFO("Flash ERROR ");
	}

#else
	uint32u8_t value;
	int16_t active_page;
	EE_MEMORY *ee_mem = (void*)EE_BASE;
	uint32u8_t verify;

	active_page=get_active_page();

	TRACE_INFO_WP("P%02x ",active_page);

	if(active_page < 0) {
		active_page=flash_init();
	}

	value.dbword = ee_mem[active_page].value[c>>2];
	value.byte[c & 0x03]=v;

	//Check if bits have to be set
	if(v & ~(ee_mem[active_page].byte[c])) {
		TRACE_INFO_WP("!= ");
		int16_t new_page = get_free_page();
		disable_page(active_page);

		if(new_page < 0) {
			//clean all pages except active page
			clean_pages(active_page);
			new_page = get_free_page();
		}

		if(new_page < 0) {
			new_page=0;
			AT91C_BASE_MC->MC_FMR &= ~AT91C_MC_NEBP;
		}

		init_page(active_page,new_page);
		active_page=new_page;

	} else {
		//Fill Buffer
		TRACE_INFO_WP("== ");
		for (unsigned int i = 0; i < (AT91C_IFLASH_PAGE_SIZE/4) ;i++) {
			//temp=ee_mem[active_page].value[i];
			ee_mem[active_page].value[i]=0xffffffff;
		}
	}


	ee_mem[active_page].value[c>>2] = value.dbword;
	TRACE_INFO_WP("P%02x ",active_page);

	TRACE_INFO_WP("W:%02x%02x%02x%02x  ",value.byte[3],value.byte[2],value.byte[1],value.byte[0]);

	//AT91C_BASE_MC->MC_FMR &= ~AT91C_MC_NEBP;
	AT91C_BASE_MC->MC_FMR |= AT91C_MC_NEBP;
	EFC_PerformCommand1(AT91C_MC_FCMD_START_PROG, FIRSTPAGE + active_page);


	verify.dbword = ee_mem[active_page].value[c>>2];
	TRACE_INFO_WP("R:%02x%02x%02x%02x ",verify.byte[3],verify.byte[2],verify.byte[1],verify.byte[0]);

	if(value.dbword != verify.dbword) {
		TRACE_INFO_WP("Error ");
		volatile uint32_t temp;
		//Try again
		for (unsigned int i = 0; i < (AT91C_IFLASH_PAGE_SIZE/4) ;i++) {
			temp=ee_mem[active_page].value[i];
			ee_mem[active_page].value[i]=temp;
		}
		ee_mem[active_page].value[c>>2] = value.dbword;
		AT91C_BASE_MC->MC_FMR &= ~AT91C_MC_NEBP;
		EFC_PerformCommand1(AT91C_MC_FCMD_START_PROG, FIRSTPAGE + active_page);

		verify.dbword = ee_mem[active_page].value[c>>2];

		if(value.dbword != verify.dbword) {
				TRACE_INFO_WP("Error \n\r");
		} else {
			TRACE_INFO_WP("OK \n\r");
		}
	} else {
		TRACE_INFO_WP("\n\r");
	}
	AT91C_BASE_MC->MC_FMR |= AT91C_MC_NEBP;
#endif
#endif
}

uint16_t eeprom_read_word(uint16_t *p) {
	union {
	uint16_t word;
	uint8_t byte[2];
	} value;

	value.byte[0] = eeprom_read_byte((uint8_t *)p);
	value.byte[1] = eeprom_read_byte((uint8_t *)p + 1);
	return value.word;
}

uint8_t eeprom_read_byte(uint8_t *p) {
	uint32_t c = (uint32_t)p;

#ifdef USE_RAM
	return ee_ram[c];
#else
#ifdef USE_DATAFLASH
	uint8_t v;

	c += FLASHPAGE * AT45_PageSize(&at45);

	AT45_Read(&at45, &v,1,c);
	return v;

#else
	EE_MEMORY *ee_mem = (void*)EE_BASE;
	uint8_t value;
	int16_t active_page;


	active_page=get_active_page();
	TRACE_INFO("P%02x ",active_page);

	if(active_page < 0) {
		active_page=flash_init();
	}

	value= ee_mem[active_page].byte[c];
	TRACE_INFO_WP("P%02x ",active_page);

	TRACE_INFO_WP("EE_R A:%x V:%x   \n\r",(uint8_t)c,value);
	return value;
#endif
#endif
}

/*
 * The width of the CRC calculation and result.
 * Modify the typedef for a 16 or 32-bit CRC standard.
 */

typedef uint32_t crc;
#define CRC_POLYNOMIAL 0x8005
#define CRC_WIDTH  (8 * sizeof(crc))
#define CRC_TOPBIT (1 << (CRC_WIDTH - 1))

crc CRCs(unsigned char* message, uint16_t count)
{
    crc  remainder = 0;
	int byte;
	unsigned char bit;

    /*
     * Perform modulo-2 division, a byte at a time.
     */
    for (byte = 0; byte < count; ++byte)
    {
        /*
         * Bring the next byte into the remainder.
         */
        remainder ^= (message[byte] << (CRC_WIDTH - 8));

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & CRC_TOPBIT)
            {
                remainder = (remainder << 1) ^ CRC_POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }

    /*
     * The final remainder is the CRC result.
     */
    return (remainder);

}   /* crcSlow() */


uint32_t flash_serial(void) {
#ifdef USE_DATAFLASH
	unsigned char v[128];

	AT45_Read_Security(&at45, v,128,0);

	return CRCs(v,128);
#else
	return 0x01234567;
#endif
}

int16_t flash_init(void) {
#ifdef USE_RAM
	return 0;
#else
#ifdef USE_DATAFLASH
	const At45Desc *pDesc;

	// Configure pins
	PIO_Configure(pins, PIO_LISTSIZE(pins));

	// SPI and At45 driver initialization
	TRACE_INFO("Initializing the SPI and AT45 drivers\n\r");
	AIC_ConfigureIT(BOARD_AT45_A_SPI_ID, 0, ISR_Spi);
	SPID_Configure(&spid, BOARD_AT45_A_SPI_BASE, BOARD_AT45_A_SPI_ID);
	SPID_ConfigureCS(&spid, BOARD_AT45_A_NPCS, AT45_CSR(BOARD_MCK, SPCK));
	AT45_Configure(&at45, &spid, BOARD_AT45_A_NPCS);
	TRACE_INFO("At45 enabled\n\r");
	AIC_EnableIT(BOARD_AT45_A_SPI_ID);
	TRACE_INFO("SPI interrupt enabled\n\r");

	// Identify the At45 device
	TRACE_INFO("Waiting for a dataflash to be connected ...\n\r");
	pDesc = 0;

	while (!pDesc) {

		pDesc = AT45_FindDevice(&at45, AT45_GetStatus(&at45));
	}
	TRACE_INFO("%s detected\n\r", at45.pDesc->name);

	// Output JEDEC identifier of device
	TRACE_INFO("Device identifier: 0x%08X\n\r", AT45_GetJedecId(&at45));


	unsigned char v[2];
	uint32_t c;

	c = FLASHPAGE * AT45_PageSize(&at45);

	AT45_Read(&at45, v,2,c);
	TRACE_INFO("EE Magic: %u %u Start %u\n\r", v[0], v[1],(unsigned int)c);

	TRACE_INFO("Flash Serial: %04x%04x\n\r",(uint16_t)(flash_serial() >> 16),(uint16_t)(flash_serial() & 0xffff));

	return 0;

#else
	int16_t active_page;

	//no erase before write
	AT91C_BASE_MC->MC_FMR |= AT91C_MC_NEBP;

	//Unlock last page
	EFC_PerformCommand1(AT91C_MC_FCMD_UNLOCK, FIRSTPAGE);

	active_page=get_active_page();

	if (active_page < 0) {

		active_page = 0;
		init_page(-1,active_page);

		//erase before write and write page
		AT91C_BASE_MC->MC_FMR &= ~AT91C_MC_NEBP;
		EFC_PerformCommand1(AT91C_MC_FCMD_START_PROG, FIRSTPAGE + active_page);

		TRACE_INFO("Init Flash P%i",active_page);
	}

	return active_page;
#endif
#endif
}

void dump_flash(void) {


}

