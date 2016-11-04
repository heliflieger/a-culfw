
FORMAT = ihex
OBJDIR = .

F_CPU = 8000000

CLIB_DIR = ../../clib
LUFA_DIR = ../../lufa

SRC =           CUL.c\
		$(CLIB_DIR)/Descriptors.c                             \
		$(CLIB_DIR)/cc1100.c                                  \
		$(CLIB_DIR)/cc1101_pllcheck.c                         \
		$(CLIB_DIR)/cdcLUFA.c                                 \
		$(CLIB_DIR)/clock.c                                   \
		$(CLIB_DIR)/belfox.c                                  \
		$(CLIB_DIR)/delay.c                                   \
		$(CLIB_DIR)/display.c                                 \
		$(CLIB_DIR)/stringfunc.c                              \
		$(CLIB_DIR)/fncollection.c                            \
		$(CLIB_DIR)/ringbuffer.c                              \
		$(CLIB_DIR)/fband.c                                   \
		$(CLIB_DIR)/helper.c                                  \
		$(CLIB_DIR)/rf_send.c                                 \
		$(CLIB_DIR)/rf_receive.c                              \
		$(CLIB_DIR)/rf_receive_bucket.c                       \
		$(CLIB_DIR)/rf_receive_it.c                           \
		$(CLIB_DIR)/rf_receive_tcm97001.c                     \
		$(CLIB_DIR)/rf_receive_tx3.c                          \
		$(CLIB_DIR)/rf_receive_revolt.c                       \
		$(CLIB_DIR)/rf_receive_esa.c                          \
		$(CLIB_DIR)/rf_receive_hms.c                          \
		$(CLIB_DIR)/rf_native.c                               \
		$(CLIB_DIR)/lacrosse.c                                \
		$(CLIB_DIR)/rf_router.c                               \
		$(CLIB_DIR)/rf_asksin.c                               \
		$(CLIB_DIR)/rf_moritz.c                               \
		$(CLIB_DIR)/rf_rwe.c                                  \
		$(CLIB_DIR)/intertechno.c                             \
		$(CLIB_DIR)/somfy_rts.c                               \
		$(CLIB_DIR)/kopp-fc.c				                				 \
		$(CLIB_DIR)/fht.c                                     \
		$(CLIB_DIR)/fastrf.c                                  \
		$(CLIB_DIR)/ttydata.c                                 \
		$(CLIB_DIR)/spi.c                                     \
		$(CLIB_DIR)/memory.c                                  \
		$(LUFA_DIR)/Drivers/USB/LowLevel/DevChapter9.c        \
		$(LUFA_DIR)/Drivers/USB/LowLevel/Endpoint.c           \
		$(LUFA_DIR)/Drivers/USB/LowLevel/Host.c               \
		$(LUFA_DIR)/Drivers/USB/LowLevel/HostChapter9.c       \
		$(LUFA_DIR)/Drivers/USB/LowLevel/LowLevel.c           \
		$(LUFA_DIR)/Drivers/USB/LowLevel/Pipe.c               \
		$(LUFA_DIR)/Drivers/USB/HighLevel/Events.c            \
		$(LUFA_DIR)/Drivers/USB/HighLevel/USBInterrupt.c      \
		$(LUFA_DIR)/Drivers/USB/HighLevel/USBTask.c           \
		$(LUFA_DIR)/Drivers/USB/HighLevel/ConfigDescriptor.c  \


LUFA_OPTS  = -D USE_NONSTANDARD_DESCRIPTOR_NAMES
LUFA_OPTS += -D NO_STREAM_CALLBACKS
LUFA_OPTS += -D USB_DEVICE_ONLY
LUFA_OPTS += -D FIXED_CONTROL_ENDPOINT_SIZE=8
LUFA_OPTS += -D FIXED_NUM_CONFIGURATIONS=1
LUFA_OPTS += -D USE_FLASH_DESCRIPTORS
LUFA_OPTS += -D F_CLOCK=$(F_CPU)UL
LUFA_OPTS += -D USE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
CFLAGS  = -D$(TARGET)
CFLAGS += -D$(FREQUENCE)
CFLAGS += -DF_CPU=$(F_CPU)UL
CFLAGS += -DLUFA $(LUFA_OPTS)
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
CFLAGS += -I$(LUFA_DIR) -I$(CLIB_DIR)
CFLAGS += -std=gnu99
#CFLAGS += -Wunreachable-code
GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)

EXTMEMOPTS =
LDFLAGS = -Wl,-Map=$(TARGET)$(FREQUENCE).map,--cref
LDFLAGS += -Wl,--relax
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(EXTMEMOPTS)

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AR = avr-ar rcs
NM = avr-nm
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp

MSG_SIZE = Size:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_CLEANING = Cleaning project:

OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

ifeq ($(GCC_TARGET),$(filter $(GCC_TARGET), CUL_ARDUINO))
SRC += $(CLIB_DIR)/rf_mbus.c $(CLIB_DIR)/mbus/manchester.c $(CLIB_DIR)/mbus/3outof6.c $(CLIB_DIR)/mbus/mbus_packet.c $(CLIB_DIR)/mbus/crc.c
endif

all:
	make TARGET=CUL_ARDUINO FREQUENCE=_433MHZ MCU=atmega32u4 mostly_clean build size
	make TARGET=CUL_ARDUINO FREQUENCE=_868MHZ MCU=atmega32u4 mostly_clean build size

build: elf hex eep lss sym

elf: $(TARGET)$(FREQUENCE).elf
hex: $(TARGET)$(FREQUENCE).hex
eep: $(TARGET)$(FREQUENCE).eep
lss: $(TARGET)$(FREQUENCE).lss
sym: $(TARGET)$(FREQUENCE).sym


ELFSIZE = $(SIZE) --format=avr --mcu=$(MCU) $(TARGET)$(FREQUENCE).elf

size:
	@if test -f $(TARGET)$(FREQUENCE).elf; then echo $(MSG_SIZE); $(ELFSIZE); \
	2>/dev/null; echo; fi


usbprogram:
	@echo 'specify one of: avdude_arduino433Mhz avdude_arduino868Mhz'

avdude_arduino868Mhz: TARGET=CUL_ARDUINO
avdude_arduino868Mhz: FREQUENCE=_868MHZ
avdude_arduino868Mhz: MCU=atmega32u4
avdude_arduino868Mhz: PORT=/dev/ttyACM0
avdude_arduino868Mhz: BAUD=57600
avdude_arduino868Mhz: do_avdude

avdude_arduino433Mhz: TARGET=CUL_ARDUINO
avdude_arduino433Mhz: FREQUENCE=_433MHZ
avdude_arduino433Mhz: MCU=atmega32u4
avdude_arduino433Mhz: PORT=/dev/ttyACM0
avdude_arduino433Mhz: BAUD=57600
avdude_arduino433Mhz: do_avdude


do_avdude:
	avrdude -p$(MCU) -cavr109 -P$(PORT) -b$(BAUD) -D -Uflash:w:./$(TARGET)$(FREQUENCE).hex:i

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
	@mkdir -p $(OBJDIR)
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
