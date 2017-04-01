MCU = atmega328p
F_CPU = 8000000
FORMAT = ihex
TARGET = miniCUL
OBJDIR = .

SRC = $(TARGET).c                                                    \
		../../clib/cc1100.c                                  \
		../../clib/cc1101_pllcheck.c                         \
		../../clib/clock.c                                   \
		../../clib/delay.c                                   \
		../../clib/display.c                                 \
		../../clib/stringfunc.c                              \
		../../clib/fncollection.c                            \
		../../clib/ringbuffer.c                              \
		../../clib/fband.c                                   \
		../../clib/rf_send.c                                 \
		../../clib/rf_receive.c                              \
		../../clib/rf_receive_bucket.c                       \
		../../clib/rf_receive_tcm97001.c                     \
		../../clib/rf_receive_it.c                           \
		../../clib/rf_receive_tx3.c                          \
		../../clib/rf_receive_revolt.c                       \
		../../clib/rf_receive_esa.c                          \
		../../clib/rf_receive_hms.c                          \
		../../clib/rf_native.c 				     \
		../../clib/lacrosse.c 				     \
		../../clib/helper.c                                  \
		../../clib/rf_router.c                               \
		../../clib/rf_asksin.c 				     \
		../../clib/rf_moritz.c 				     \
		../../clib/rf_rwe.c 	   			     \
		../../clib/rf_zwave.c                                \
		../../clib/intertechno.c                             \
		../../clib/somfy_rts.c 				     \
		../../clib/kopp-fc.c 				     \
		../../clib/belfox.c 				     \
		../../clib/fht.c                                     \
		../../clib/fastrf.c                                  \
		../../clib/memory.c                                  \
		../../clib/serial.c                                  \
		../../clib/ttydata.c                                 \
		../../clib/spi.c                                     \
		../../clib/rf_mbus.c                                 \
		../../clib/mbus/3outof6.c                            \
		../../clib/mbus/mbus_packet.c                        \
		../../clib/mbus/crc.c                                \
		../../clib/mbus/manchester.c                         \

CFLAGS  = -D$(TARGET)
CFLAGS += -D$(FREQUENCE)
CFLAGS += -DF_CPU=$(F_CPU)UL
CFLAGS += -Os
CFLAGS += -funsigned-char       #   12 Byte (shorter code)
CFLAGS += -funsigned-bitfields  #    0
CFLAGS += -ffunction-sections   # 2600
CFLAGS += -fpack-struct         #    0
CFLAGS += -fshort-enums         #    0
CFLAGS += -mcall-prologues      #  130
CFLAGS += -finline-limit=20     # 0=>150 10=>12 20=>0 (0: RFR won't work!)
CFLAGS += -Wall
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wa,-adhlns=$(<:%.c=$(OBJDIR)/%.lst)
CFLAGS += -I../../lufa -I../../clib
CFLAGS += -std=gnu99
#CFLAGS += -Wunreachable-code
GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)

EXTMEMOPTS =
LDFLAGS = -Wl,-Map=$(TARGET)$(FREQUENCE).map,--cref
LDFLAGS += -Wl,--relax 
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(EXTMEMOPTS)

AVRDUDE_PROGRAMMER = arduino -b 57600
AVRDUDE_PORT = /dev/ttyUSB0
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex:i
AVRDUDE_FLAGS = -p atmega328p -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AR = avr-ar rcs
NM = avr-nm
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp
AVRDUDE = avrdude

MSG_SIZE = Size:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_CLEANING = Cleaning project:

OBJ = $(SRC:%.c=$(OBJDIR)/%.o)


all: 
	make TARGET=miniCUL FREQUENCE=_433MHZ MCU=atmega328p FLASH_SIZE=32768 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=miniCUL FREQUENCE=_868MHZ MCU=atmega328p FLASH_SIZE=32768 BOOTLOADER_SIZE=2048 mostly_clean build size

build: elf hex eep lss sym

elf: $(TARGET)$(FREQUENCE).elf
hex: $(TARGET)$(FREQUENCE).hex
eep: $(TARGET)$(FREQUENCE).eep
lss: $(TARGET)$(FREQUENCE).lss
sym: $(TARGET)$(FREQUENCE).sym


ELFSIZE = $(SIZE) --format=avr --mcu=$(MCU) $(TARGET)$(FREQUENCE).elf
PROGRAM_SIZE = $(shell $(SIZE) --format=avr --mcu=$(MCU) $(TARGET)$(FREQUENCE).elf | grep Program | tr -s " " | cut -d " " -f 2 ; echo +$(BOOTLOADER_SIZE))
PROGRAM_SIZE_PLUS_BL=$(shell echo $(PROGRAM_SIZE) | bc)

size:
	@if test -f $(TARGET)$(FREQUENCE).elf; then echo $(MSG_SIZE); $(ELFSIZE); \
	2>/dev/null; echo; fi
	@echo "PROGRAM_SIZE_PLUS_BL: $(PROGRAM_SIZE_PLUS_BL), FLASH_SIZE: $(FLASH_SIZE) \n"
	@if test $(PROGRAM_SIZE_PLUS_BL) -gt $(FLASH_SIZE); then echo "FLASH SIZE OVERFLOW ($(PROGRAM_SIZE_PLUS_BL)>$(FLASH_SIZE))"; exit 2; fi;


program: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)

usbprogram:
	@echo 'specify one of: usbprogram_minicul'

avdude_arduino868Mhz: TARGET=miniCUL
avdude_arduino868Mhz: FREQUENCE=_868MHZ
avdude_arduino868Mhz: MCU=atmega328p
avdude_arduino868Mhz: PORT=/dev/ttyUSB0
avdude_arduino868Mhz: BAUD=57600
avdude_arduino868Mhz: do_avdude

avdude_arduino433Mhz: TARGET=miniCUL
avdude_arduino433Mhz: FREQUENCE=_433MHZ
avdude_arduino433Mhz: MCU=atmega328p
avdude_arduino433Mhz: PORT=/dev/ttyUSB0
avdude_arduino433Mhz: BAUD=57600
avdude_arduino433Mhz: do_avdude


do_avdude:
	avrdude -p$(MCU) -cavr109 -P$(PORT) -b$(BAUD) -D -Uflash:w:./$(TARGET)$(FREQUENCE).hex

%.hex: %.elf
	@echo $(MSG_FLASH) $@
	@$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

%.eep: %.elf
	@echo $(MSG_EEPROM) $@
	@-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O $(FORMAT) $< $@ || exit 0

%.lss: %.elf
	@echo $(MSG_EXTENDED_LISTING) $@
	@$(OBJDUMP) -h -z -S $< > $@

%.sym: %.elf
	@echo $(MSG_SYMBOL_TABLE) $@
	@$(NM) -n $< > $@



.SECONDARY : $(TARGET)$(FREQUENCE).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo $(MSG_LINKING) $@
	@$(CC) $(ALL_CFLAGS) $^ --output $@ $(LDFLAGS)


$(OBJDIR)/%.o : %.c
	@echo $(MSG_COMPILING) $<
	@$(CC) -c $(ALL_CFLAGS) $< -o $@ 

mostly_clean:
	@echo $(MSG_CLEANING)
	@$(REMOVE) $(TARGET)$(FREQUENCE).hex
	@$(REMOVE) $(TARGET)$(FREQUENCE).eep
	@$(REMOVE) $(TARGET)$(FREQUENCE).cof
	@$(REMOVE) $(TARGET)$(FREQUENCE).elf
	@$(REMOVE) $(TARGET)$(FREQUENCE).map
	@$(REMOVE) $(TARGET)$(FREQUENCE).sym
	@$(REMOVE) $(TARGET)$(FREQUENCE).lss
	@$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.o)
	@$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.lst)
	@$(REMOVE) $(SRC:.c=.s)
	@$(REMOVE) $(SRC:.c=.d)
	@$(REMOVE) $(SRC:.c=.i)

clean: mostly_clean
	@$(REMOVE) *.hex
	@$(REMOVE) *.eep
	@$(REMOVE) *.cof
	@$(REMOVE) *.elf
	@$(REMOVE) *.map
	@$(REMOVE) *.sym
	@$(REMOVE) *.lss
	@$(REMOVEDIR) .dep

$(shell mkdir $(OBJDIR) 2>/dev/null)

-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
