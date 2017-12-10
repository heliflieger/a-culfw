#include "eeprom.h"

#include <utility/trace.h>
#include <board.h>
#include "stm32f1xx_hal.h"

//#define NUM_PAGES	64
//(AT91C_IFLASH_NB_OF_PAGES / AT91C_IFLASH_NB_OF_LOCK_BITS)
//#define FIRSTPAGE	(AT91C_IFLASH_NB_OF_PAGES - NUM_PAGES -1)
//#define EE_BASE		(AT91C_IFLASH+(FIRSTPAGE * AT91C_IFLASH_PAGE_SIZE))

//#define FMCN_BITS(mck)      (ROUND((mck) / 100000) << 16)
//#define ROUND(n)    ((((n) % 10) >= 5) ? (((n) / 10) + 1) : ((n) / 10))
//#define FMCN_FLASH(mck)         ((((mck) / 2000000) * 3) << 16)

#define EE_START        ADDR_FLASH_PAGE_124
#define EE_PAGES        4
#define WORDS_PER_PAGE  (FLASH_PAGE_SIZE/2)

static uint8_t startpage;
static uint16_t nbytes;

typedef struct _EE_MEMORY {
	uint8_t address;
	uint8_t data;
} EE_MEMORY;

typedef struct _uint16u8_t {
	union {
	uint16_t word;
	EE_MEMORY byte;
	};
} uint16u8_t;

//#define USE_RAM

#if defined(USE_RAM)
uint8_t ee_ram[256];
#endif

static void erase_pages(uint32_t address, uint8_t count) {
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PAGEError = 0;

  TRACE_DEBUG("EE erase page %x : %x\n\r",address,count);

  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = address;
  EraseInitStruct.NbPages     = count;

  HAL_FLASH_Unlock();
  HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
  HAL_FLASH_Lock();

  startpage = 0;
}

static void reset_flash(void) {
  erase_pages(EE_START, EE_PAGES);

  TRACE_DEBUG("EE reset flash\n\r");

  HAL_FLASH_Unlock();
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, EE_START, 0xFEFF);
  HAL_FLASH_Lock();
}

static uint8_t get_startpage() {
  uint16_t *address = (uint16_t*)EE_START;
  uint8_t x;

  for( x=0; x < EE_PAGES; x++) {
    if (address[WORDS_PER_PAGE * x] == 0xFEFF) {
      return x;
    }
  }

  reset_flash();

  return 0;
}

static uint16_t get_flashdata(uint16_t pos) {
  uint16_t index;
  uint16_t *address = (uint16_t*)EE_START;
  uint32_t offset = startpage * WORDS_PER_PAGE;

  index = (pos + offset + 1) % (WORDS_PER_PAGE * EE_PAGES);

  return address[index];
}

static uint8_t seek_flashdata(uint8_t address) {
  uint16_t x = nbytes;
  uint16u8_t value;

  do {
    value.word = get_flashdata(x);
    if (value.byte.address == address) {
      return value.byte.data;
    }
  } while (x--);

  return 0xFF;
}

static void reorg_flashdata(void) {
  uint8_t newstartpage = (startpage + EE_PAGES - 1) % EE_PAGES;;
  uint16_t *address = (uint16_t*)EE_START;
  uint32_t offset = newstartpage * WORDS_PER_PAGE;
  uint16_t newnbytes = 0;
  uint16u8_t value;

  TRACE_DEBUG("EE reorg flash, start old:%x new:%x\n\r",startpage,newstartpage);

  if (address[offset] != 0xFFFF) {
    reset_flash();
    return;
  }

  HAL_FLASH_Unlock();

  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t)&address[offset++ + newnbytes], 0xFEFF);
  for(uint8_t x = 0; x < 0xff; x++) {
    value.byte.data = seek_flashdata(x);
    if(value.byte.data != 0xff) {
      value.byte.address  = x;
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t)&address[offset + newnbytes++], value.word);
    }
  }

  if(newstartpage) {
    erase_pages(EE_START, newstartpage);
  }
  if((newstartpage + 1) < EE_PAGES) {
    erase_pages(EE_START + ((newstartpage + 1) * FLASH_PAGE_SIZE) ,EE_PAGES - newstartpage - 1);
  }

  HAL_FLASH_Lock();

  startpage = newstartpage;
  nbytes = newnbytes;

}

static void push_flashdata(uint16_t data) {
  uint16_t index;
  uint16_t *address = (uint16_t*)EE_START;
  uint32_t offset = startpage * WORDS_PER_PAGE;

  index = (nbytes + offset + 1) % (WORDS_PER_PAGE * EE_PAGES);

  HAL_FLASH_Unlock();
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t)&address[index], data);
  HAL_FLASH_Lock();

  nbytes++;

  if(nbytes > WORDS_PER_PAGE * (EE_PAGES-1) - 2) {
    reorg_flashdata();
  }
}

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------


void eeprom_write_byte(uint8_t *p, uint8_t v) {

	uint32_t c = (uint32_t)p;
	//TRACE_DEBUG("EE_W A:%02x V:%02x   ",(uint8_t)c,v);

	if(eeprom_read_byte(p) == v) {
		return;
	}

  if(c >= 0xff) {
    return;
  }

#ifdef USE_RAM
	ee_ram[c]=v;
	return;
#else
	uint16u8_t value;

	value.byte.address = c;
  value.byte.data = v;

  push_flashdata(value.word);

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

  if(c >= 0xff) {
    return 0xff;
  }

#ifdef USE_RAM
	return ee_ram[c];
#else
	uint8_t value;

	value = seek_flashdata(c);
	return value;


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

#define STM32_UUID ((uint32_t *)UID_BASE)

uint32_t flash_serial(void) {

  uint32_t v[3];

  v[0] = STM32_UUID[0];
  v[1] = STM32_UUID[1];
  v[2] = STM32_UUID[2];


  return CRCs((unsigned char*)v,9);

}

int16_t flash_init(void) {
  startpage = get_startpage();

	for(uint16_t x = 0; x < ((WORDS_PER_PAGE * EE_PAGES) -1); x++) {
	  if(get_flashdata(x) == 0xFFFF) {
	    nbytes = x;
	    break;
	  }
	}

	return 0;
}

/*
void test_flash() {
  uint8_t value;


  get_flashdata(0);

  for(uint8_t x = 0; x<0xff;x++) {
    eeprom_write_byte((uint8_t*)x, 0xAA - x);
  }
  for(uint8_t x = 0; x<0xFF;x++) {
    value = eeprom_read_byte((uint8_t*)x);
    uint8_t temp = 0xAA - x;
    if(value != temp) {
      TRACE_DEBUG("EE fail1 at %x v:%x t:%x\n\r",x,value,temp);
      value = eeprom_read_byte((uint8_t*)x);
      return;
    }
  }

  for(uint8_t x = 0; x<0xff;x++) {
    eeprom_write_byte((uint8_t*)x, 0x55 - x);
  }
  for(uint8_t x = 0; x<0xff;x++) {
    value = eeprom_read_byte((uint8_t*)x);
    uint8_t temp = 0x55 - x;
    if(value != temp) {
      TRACE_DEBUG("EE fail2 at %x v:%x t:%x\n\r",x,value,temp);
      value = eeprom_read_byte((uint8_t*)x);
      return;
    }
  }
  TRACE_DEBUG("EE test OK\n\r");
}
*/

void dump_flash(void) {


}

