#ifndef _LED_H
#define _LED_H   1

#define HI8(x)  ((uint8_t)((x) >> 8))
#define LO8(x)  ((uint8_t)(x))

#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))

#define LED_PORT		PORTA
#define LED_DDR		        DDRA
#define LED_PIN		        PA3

#define led_init()   LED_DDR  |= _BV(LED_PIN)
#define LED_OFF()    LED_PORT |= _BV(LED_PIN)
#define LED_ON()     LED_PORT &= ~_BV(LED_PIN)
#define LED_TOGGLE() LED_PORT ^= _BV(LED_PIN)

#define CC1100_CS_PORT		PORTA
#define CC1100_CS_DDR		DDRA
#define CC1100_CS_PIN		PA3

#define SPI_PORT		PORTA
#define SPI_DDR			DDRA
#define MISO			PA6
#define MOSI			PA5
#define SCK			PA4


#endif
