#define USB_MAX_POWER           500

#define CC1100_CS_PORT  	PORTE
#define CC1100_CS_DDR		DDRE
#define CC1100_CS_PIN		PE1
#define CC1100_IN_PORT  	PINE
#define CC1100_IN_DDR		DDRE
#define CC1100_IN_PIN		PE5
#define CC1100_OUT_PORT  	PORTE
#define CC1100_OUT_DDR		DDRE
#define CC1100_OUT_PIN		PE0
#define CC1100_INT		INT5
#define CC1100_INTVECT  	INT5_vect
#define CC1100_ISC		ISC50
#define CC1100_EICR             EICRB

#define LED_DDR                 DDRC
#define LED_PORT                PORTC
#define LED_PIN                 PC0
#define LED_INV

#define LCD_BL_DDR              DDRC
#define LCD_BL_PORT             PORTC
#define LCD_BL_PIN              PC6
#define LCD_BL_PWM              OCR3A

#define LCD1_DDR                DDRB
#define LCD1_PORT               PORTB
#define LCD1_CS                 PB4
#define LCD2_DDR                DDRE
#define LCD2_PORT               PORTE
#define LCD2_RST                PE3

// Max8677
#define BAT_MUX                 3
#define BAT_DDR                 DDRA
#define BAT_PORT                PORTA
#define BAT_PIN                 PINA
#define BAT_DONE                PA0
#define BAT_CEN                 PA1
#define BAT_PEN2                PA2
#define BAT_USUS                PA3
#define BAT_CHG                 PA4
#define BAT_DOK                 PA5
#define BAT_UOK                 PA6
#define BAT_FLT                 PA7

#define JOY_DDR1                DDRE
#define JOY_DDR2                DDRB
#define JOY_OUT_PORT1           PORTE
#define JOY_OUT_PORT2           PORTB
#define JOY_IN_PORT1            PINE
#define JOY_IN_PORT2            PINB
#define JOY_PIN1                PB7     // Enter (moved from Port E)
#define JOY_PIN2                PE6     // right
#define JOY_PIN3                PE7     // left
#define JOY_PIN4                PB6     // up
#define JOY_PIN5                PB5     // down
#define JOY_INT1		INT6
#define JOY_INT1VECT    	INT6_vect
#define JOY_INT1ISC		(_BV(ISC60)|_BV(ISC61))
#define JOY_INT2		INT7
#define JOY_INT2VECT     	INT7_vect
#define JOY_INT2ISC		(_BV(ISC70)|_BV(ISC71))
#define JOY_INTREG		EICRB
#define JOY_PCINTMSK		0xe0


#define DF_DDR                  DDRD 
#define DF_PORT                 PORTD 
#define DF_CS                   PD7

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PB0
#define SPI_MISO		PB3
#define SPI_MOSI		PB2
#define SPI_SCLK		PB1

#define RTC_ADDR                0xd0
#define RTC_INT		        INT4
#define RTC_INTVECT    	        INT4_vect
#define RTC_INTISC		(_BV(ISC40)|_BV(ISC41))
#define RTC_INTREG		EICRB
#define RTC_DDR                 DDRE
#define RTC_OUT_PORT            PORTE
#define RTC_PIN                 PE4

