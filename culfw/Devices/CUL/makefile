F_CPU = 8000000
FORMAT = ihex
OBJDIR = .

SRC =           CUL.c\
		../../clib/Descriptors.c                             \
		../../clib/cc1100.c                                  \
		../../clib/cc1101_pllcheck.c                         \
		../../clib/cdcLUFA.c                                 \
		../../clib/clock.c                                   \
		../../clib/belfox.c                                  \
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
		../../clib/rf_router.c                               \
		../../clib/rf_asksin.c                               \
		../../clib/rf_moritz.c                               \
		../../clib/rf_rwe.c                                  \
		../../clib/rf_native.c                               \
		../../clib/rf_zwave.c                                \
		../../clib/intertechno.c                             \
		../../clib/rf_receive_it.c                           \
		../../clib/rf_receive_tx3.c                          \
		../../clib/rf_receive_revolt.c                       \
		../../clib/rf_receive_esa.c                          \
		../../clib/rf_receive_hms.c                          \
		../../clib/helper.c                                  \
		../../clib/somfy_rts.c				   				 \
		../../clib/kopp-fc.c				   				 \
		../../clib/fht.c                                     \
		../../clib/fastrf.c                                  \
		../../clib/ttydata.c                                 \
		../../clib/spi.c                                     \
		../../clib/memory.c                                  \
		../../clib/lacrosse.c                                \
		../../lufa/Drivers/USB/LowLevel/DevChapter9.c        \
		../../lufa/Drivers/USB/LowLevel/Endpoint.c           \
		../../lufa/Drivers/USB/LowLevel/Host.c               \
		../../lufa/Drivers/USB/LowLevel/HostChapter9.c       \
		../../lufa/Drivers/USB/LowLevel/LowLevel.c           \
		../../lufa/Drivers/USB/LowLevel/Pipe.c               \
		../../lufa/Drivers/USB/HighLevel/Events.c            \
		../../lufa/Drivers/USB/HighLevel/USBInterrupt.c      \
		../../lufa/Drivers/USB/HighLevel/USBTask.c           \
		../../lufa/Drivers/USB/HighLevel/ConfigDescriptor.c  \


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

ifeq ($(TARGET), CUL_V3)
SRC += ../../clib/rf_mbus.c ../../clib/mbus/manchester.c ../../clib/mbus/3outof6.c ../../clib/mbus/mbus_packet.c ../../clib/mbus/crc.c
endif

all:
	make TARGET=CUL_V2 FREQUENCE=_433MHZ MCU=at90usb162 FLASH_SIZE=16384 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V2 FREQUENCE=_868MHZ MCU=at90usb162 FLASH_SIZE=16384 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V2_HM FREQUENCE=_433MHZ MCU=at90usb162 FLASH_SIZE=16384 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V2_HM FREQUENCE=_868MHZ MCU=at90usb162 FLASH_SIZE=16384 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V2_MAX FREQUENCE=_433MHZ MCU=at90usb162 FLASH_SIZE=16384 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V2_MAX FREQUENCE=_868MHZ MCU=at90usb162 FLASH_SIZE=16384 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V3 FREQUENCE=_433MHZ MCU=atmega32u4 FLASH_SIZE=32768 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V3 FREQUENCE=_868MHZ MCU=atmega32u4 FLASH_SIZE=32768 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V4 FREQUENCE=_433MHZ MCU=atmega32u2 FLASH_SIZE=32768 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V4 FREQUENCE=_868MHZ MCU=atmega32u2 FLASH_SIZE=32768 BOOTLOADER_SIZE=2048 mostly_clean build size
	make TARGET=CUL_V3_ZWAVE FREQUENCE=_868MHZ MCU=atmega32u4 FLASH_SIZE=32768 BOOTLOADER_SIZE=2048 mostly_clean build size

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
	



usbprogram:
	@echo 'specify one of: usbprogram_v2_433mhz usbprogram_v2_hm_433mhz usbprogram_v3_433mhz usbprogram_v4_433mhz usbprogram_v2_868mhz usbprogram_v2_hm_868mhz usbprogram_v3_868mhz usbprogram_v4_868mhz'

usbprogram_v4_433mhz: TARGET=CUL_V4
usbprogram_v4_433mhz: MCU=atmega32u2
usbprogram_v4_433mhz: FREQUENCE=_433MHZ
usbprogram_v4_433mhz: do_usbprogram

usbprogram_v3_433mhz: TARGET=CUL_V3
usbprogram_v3_433mhz: MCU=atmega32u4
usbprogram_v3_433mhz: FREQUENCE=_433MHZ
usbprogram_v3_433mhz: do_usbprogram

usbprogram_v2_433mhz: TARGET=CUL_V2
usbprogram_v2_433mhz: MCU=at90usb162
usbprogram_v2_433mhz: FREQUENCE=_433MHZ
usbprogram_v2_433mhz: do_usbprogram

usbprogram_v2_hm_433mhz: TARGET=CUL_V2_HM
usbprogram_v2_hm_433mhz: MCU=at90usb162
usbprogram_v2_hm_433mhz: FREQUENCE=_433MHZ
usbprogram_v2_hm_433mhz: do_usbprogram

usbprogram_v2_max_433mhz: TARGET=CUL_V2_MAX
usbprogram_v2_max_433mhz: MCU=at90usb162
usbprogram_v2_max_433mhz: FREQUENCE=_433MHZ
usbprogram_v2_max_433mhz: do_usbprogram

usbprogram_v4_868mhz: TARGET=CUL_V4
usbprogram_v4_868mhz: MCU=atmega32u2
usbprogram_v4_868mhz: FREQUENCE=_868MHZ
usbprogram_v4_868mhz: do_usbprogram

usbprogram_v3_868mhz: TARGET=CUL_V3
usbprogram_v3_868mhz: MCU=atmega32u4
usbprogram_v3_868mhz: FREQUENCE=_868MHZ
usbprogram_v3_868mhz: do_usbprogram

usbprogram_v2_868mhz: TARGET=CUL_V2
usbprogram_v2_868mhz: MCU=at90usb162
usbprogram_v2_868mhz: FREQUENCE=_868MHZ
usbprogram_v2_868mhz: do_usbprogram

usbprogram_v2_hm_868mhz: TARGET=CUL_V2_HM
usbprogram_v2_hm_868mhz: MCU=at90usb162
usbprogram_v2_hm_868mhz: FREQUENCE=_868MHZ
usbprogram_v2_hm_868mhz: do_usbprogram

usbprogram_v2_max_868mhz: TARGET=CUL_V2_MAX
usbprogram_v2_max_868mhz: MCU=at90usb162
usbprogram_v2_max_868mhz: FREQUENCE=_868MHZ
usbprogram_v2_max_868mhz: do_usbprogram

do_usbprogram:
	dfu-programmer $(MCU) erase || true
	dfu-programmer $(MCU) flash $(TARGET)$(FREQUENCE).hex
	dfu-programmer $(MCU) start

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
