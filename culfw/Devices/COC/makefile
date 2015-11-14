F_CPU = 8000000
MCU=atmega1284p
FORMAT = ihex
OBJDIR = .

SRC =           COC.c\
		../../clib/cc1100.c                                  \
		../../clib/cc1101_pllcheck.c                         \
		../../clib/clock.c                                   \
		../../clib/delay.c                                   \
		../../clib/display.c                                 \
		../../clib/stringfunc.c                              \
		../../clib/fncollection.c                            \
		../../clib/fband.c                                   \
		../../clib/ringbuffer.c                              \
		../../clib/rf_send.c                                 \
		../../clib/rf_receive.c                              \
		../../clib/rf_receive_bucket.c                       \
		../../clib/rf_receive_tcm97001.c                     \
		../../clib/rf_receive_it.c                           \
		../../clib/rf_receive_tx3.c                          \
		../../clib/rf_receive_revolt.c                       \
		../../clib/rf_receive_esa.c                          \
		../../clib/rf_receive_hms.c                          \
		../../clib/helper.c                                  \
		../../clib/rf_router.c                               \
		../../clib/rf_asksin.c                               \
		../../clib/rf_mbus.c                                 \
		../../clib/mbus/manchester.c                         \
		../../clib/mbus/3outof6.c                            \
		../../clib/mbus/mbus_packet.c                        \
		../../clib/mbus/crc.c                                \
		../../clib/rf_moritz.c                               \
		../../clib/rf_rwe.c                                  \
		../../clib/intertechno.c                             \
		../../clib/fht.c                                     \
		../../clib/fastrf.c                                  \
		../../clib/ttydata.c                                 \
		../../clib/serial.c                                  \
		../../clib/spi.c                                     \
		../../clib/twimaster.c                               \
		../../clib/onewire.c                                 \
		../../clib/ds1339.c                                  \
		../../clib/memory.c                                  \
                ../../clib/rf_mbus.c                                 \
                ../../clib/mbus/manchester.c                         \
                ../../clib/mbus/3outof6.c                            \
                ../../clib/mbus/mbus_packet.c                        \
                ../../clib/mbus/crc.c                                \


LUFA_OPTS  = -D USE_NONSTANDARD_DESCRIPTOR_NAMES
LUFA_OPTS += -D NO_STREAM_CALLBACKS
LUFA_OPTS += -D USB_DEVICE_ONLY
LUFA_OPTS += -D FIXED_CONTROL_ENDPOINT_SIZE=8
LUFA_OPTS += -D FIXED_NUM_CONFIGURATIONS=1
LUFA_OPTS += -D USE_FLASH_DESCRIPTORS
LUFA_OPTS += -D F_CLOCK=$(F_CPU)UL
LUFA_OPTS += -D USE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
CFLAGS  = -D$(TARGET)
CFLAGS += -D$(COCVERS)
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
CFLAGS += -I../../lufa -I../../clib
CFLAGS += -std=gnu99
#CFLAGS += -Wunreachable-code
GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)

EXTMEMOPTS =
LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--relax 
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(EXTMEMOPTS)

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AR = avr-ar rcs
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp

#AVRDUDE_NO_VERIFY = -V

AVRDUDE_PROGRAMMER = avr109
AVRDUDE_PORT = /dev/ttyAMA0 -b 38400
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)
AVRDUDE_FLAGS += $(AVRDUDE_NO_VERIFY)
AVRDUDE_FLAGS += $(AVRDUDE_VERBOSE)
AVRDUDE_FLAGS += $(AVRDUDE_ERASE_COUNTER)

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
	make TARGET=COC COCVERS=FULL mostly_clean build size
	make TARGET=COC_radio_only COCVERS=RADIO_ONLY mostly_clean build size

build: elf hex eep lss sym

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym


ELFSIZE = $(SIZE) --format=avr --mcu=$(MCU) $(TARGET).elf

size:
	@if test -f $(TARGET).elf; then echo $(MSG_SIZE); $(ELFSIZE); \
	2>/dev/null; echo; fi


program:
	@echo 'specify one of: program_full program_radio_only'

program_full: TARGET=COC
program_full: COCVERS=FULL
program_full: do_program

program_radio_only: TARGET=COC_radio_only
program_radio_only: COCVERS=RADIO_ONLY
program_radio_only: do_program

do_program:
	@echo
	@echo calling radio frontends bootloader ...
	@echo
	if test ! -d /sys/class/gpio/gpio17; then echo 17 > /sys/class/gpio/export; fi
	echo out > /sys/class/gpio/gpio17/direction
	echo 0 > /sys/class/gpio/gpio17/value

	if test ! -d /sys/class/gpio/gpio18; then echo 18 > /sys/class/gpio/export; fi
	echo out > /sys/class/gpio/gpio18/direction
	echo 0 > /sys/class/gpio/gpio18/value

	echo 1 > /sys/class/gpio/gpio17/value
	sleep 1
	echo 1 > /sys/class/gpio/gpio18/value
	echo in > /sys/class/gpio/gpio18/direction
	echo 18 > /sys/class/gpio/unexport

	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)

	if test -e /sys/bus/i2c/devices/0-0050/eeprom; then echo COC V1.2 $(COCVERS) `date +%F` > /sys/bus/i2c/devices/0-0050/eeprom; fi

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



.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo $(MSG_LINKING) $@
	@$(CC) $(ALL_CFLAGS) $^ --output $@ $(LDFLAGS)


$(OBJDIR)/%.o : %.c
	@echo $(MSG_COMPILING) $<
	@$(CC) -c $(ALL_CFLAGS) $< -o $@ 

mostly_clean:
	@echo $(MSG_CLEANING)
	@$(REMOVE) $(TARGET).hex
	@$(REMOVE) $(TARGET).eep
	@$(REMOVE) $(TARGET).cof
	@$(REMOVE) $(TARGET).elf
	@$(REMOVE) $(TARGET).map
	@$(REMOVE) $(TARGET).sym
	@$(REMOVE) $(TARGET).lss
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
