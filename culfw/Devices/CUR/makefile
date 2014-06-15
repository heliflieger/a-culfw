include board.mk
F_CPU = 8000000
FORMAT = ihex
TARGET = CUR
OBJDIR = .


SRC = $(TARGET).c                                                  \
		../../clib/Descriptors.c                              \
		../../clib/cc1100.c                                   \
		../../clib/cc1101_pllcheck.c                          \
		../../clib/cdcLUFA.c                                  \
		../../clib/clock.c                                    \
		../../clib/delay.c                                    \
		../../clib/display.c                                  \
		../../clib/fncollection.c                             \
		../../clib/ringbuffer.c                               \
		../../clib/spi.c                                      \
		../../clib/df.c                                       \
		../../clib/qfs.c                                      \
		../../clib/fswrapper.c                                \
		../../clib/stringfunc.c                               \
		../../clib/rf_send.c                                  \
		../../clib/rf_receive.c                               \
		../../clib/fht.c                                      \
		../../clib/ttydata.c                                  \
		../../clib/pcf8833.c                                  \
		../../clib/menu.c                                     \
		../../clib/battery.c                                  \
		../../clib/adcw.c                                     \
		../../clib/joy.c                                      \
		../../clib/mysleep.c                                  \
		../../clib/ds1339.c                                   \
		../../clib/log.c                                      \
		../../clib/more.c                                     \
		../../clib/fastrf.c                                   \
		../../clib/rf_router.c                                \
		../../clib/twimaster.c                                \
		../../lufa/Drivers/USB/LowLevel/DevChapter9.c         \
		../../lufa/Drivers/USB/LowLevel/Endpoint.c            \
		../../lufa/Drivers/USB/LowLevel/Host.c                \
		../../lufa/Drivers/USB/LowLevel/HostChapter9.c        \
		../../lufa/Drivers/USB/LowLevel/LowLevel.c            \
		../../lufa/Drivers/USB/LowLevel/Pipe.c                \
		../../lufa/Drivers/USB/HighLevel/Events.c             \
		../../lufa/Drivers/USB/HighLevel/USBInterrupt.c       \
		../../lufa/Drivers/USB/HighLevel/USBTask.c            \
		../../lufa/Drivers/USB/HighLevel/ConfigDescriptor.c   \



OPT = s
LUFA_OPTS  = -D USE_NONSTANDARD_DESCRIPTOR_NAMES
LUFA_OPTS += -D NO_STREAM_CALLBACKS
LUFA_OPTS += -D USB_DEVICE_ONLY
LUFA_OPTS += -D FIXED_CONTROL_ENDPOINT_SIZE=8
LUFA_OPTS += -D FIXED_NUM_CONFIGURATIONS=1
LUFA_OPTS += -D USE_FLASH_DESCRIPTORS
LUFA_OPTS += -D F_CLOCK=$(F_CPU)UL
LUFA_OPTS += -D USE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"

EXTRAINCDIRS = ../.. ../../lufa ../../clib
CSTANDARD = -std=gnu99
CDEFS  = -DF_CPU=$(F_CPU)UL -D$(BOARD)
CDEFS += -DLUFA $(LUFA_OPTS)

ADEFS = -DF_CPU=$(F_CPU)
CFLAGS += $(CDEFS)
CFLAGS += -O$(OPT)
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -ffunction-sections
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums
CFLAGS += -finline-limit=20
CFLAGS += -Wall
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
#CFLAGS += -Wunreachable-code
CFLAGS += -Wa,-adhlns=$(<:%.c=$(OBJDIR)/%.lst)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)
CFLAGS += -mcall-prologues
CFLAGS += -Wundef

ASFLAGS = $(ADEFS) -Wa,-adhlns=$(<:%.S=$(OBJDIR)/%.lst),-gstabs,--listing-cont-lines=100

LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--relax 
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(EXTMEMOPTS)
LDFLAGS += $(patsubst %,-L%,$(EXTRALIBDIRS))


DEBUG_MFREQ = $(F_CPU)
DEBUG_UI = insight
DEBUG_BACKEND = avarice
GDBINIT_FILE = __avr_gdbinit
JTAG_DEV = /dev/com1

DEBUG_PORT = 4242

DEBUG_HOST = localhost





SHELL = sh
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AR = avr-ar rcs
NM = avr-nm
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp

MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before: 
MSG_SIZE_AFTER = Size after:
MSG_COFF = Converting to AVR COFF:
MSG_EXTENDED_COFF = Converting to AVR Extended COFF:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_COMPILING_CPP = Compiling C++:
MSG_CLEANING = Cleaning project:


OBJ = $(SRC:%.c=$(OBJDIR)/%.o) $(CPPSRC:%.cpp=$(OBJDIR)/%.o) $(ASRC:%.S=$(OBJDIR)/%.o) 

LST = $(SRC:%.c=$(OBJDIR)/%.lst) $(CPPSRC:%.cpp=$(OBJDIR)/%.lst) $(ASRC:%.S=$(OBJDIR)/%.lst) 


GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d


ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)

all: sizebefore build sizeafter

build: elf hex eep lss sym


elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym
LIBNAME=lib$(TARGET).a
lib: $(LIBNAME)



begin:
	@echo
	@echo $(MSG_BEGIN)

end:
	@echo $(MSG_END)
	@echo


HEXSIZE = $(SIZE) $(TARGET).hex
ELFSIZE = $(SIZE) $(TARGET).elf

sizebefore:
	@if test -f $(TARGET).elf; then echo; echo $(MSG_SIZE_BEFORE); $(ELFSIZE); \
	2>/dev/null; echo; fi

sizeafter:
	@if test -f $(TARGET).elf; then echo; echo $(MSG_SIZE_AFTER); $(ELFSIZE); \
	2>/dev/null; echo; fi

usbprogram: $(TARGET).hex
	dfu-programmer $(MCU) erase
	dfu-programmer $(MCU) flash $(TARGET).hex
	dfu-programmer $(MCU) start


COFFCONVERT = $(OBJCOPY) --debugging
COFFCONVERT += --change-section-address .data-0x800000
COFFCONVERT += --change-section-address .bss-0x800000
COFFCONVERT += --change-section-address .noinit-0x800000
COFFCONVERT += --change-section-address .eeprom-0x810000



coff: $(TARGET).elf
	@echo
	@echo $(MSG_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-avr $< $(TARGET).cof


extcoff: $(TARGET).elf
	@echo
	@echo $(MSG_EXTENDED_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-ext-avr $< $(TARGET).cof



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


$(OBJDIR)/%.o : %.cpp
	@echo
	@echo $(MSG_COMPILING_CPP) $<
	$(CC) -c $(ALL_CPPFLAGS) $< -o $@ 


%.s : %.c
	$(CC) -S $(ALL_CFLAGS) $< -o $@


%.i : %.c
	$(CC) -E -mmcu=$(MCU) -I. $(CFLAGS) $< -o $@ 

clean: clean_list clean_binary

clean_binary:
	@$(REMOVE) $(TARGET).hex

clean_list:
	@echo $(MSG_CLEANING)
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
	@$(REMOVEDIR) .dep


$(shell mkdir $(OBJDIR) 2>/dev/null)


-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


.PHONY : all checkhooks checklibmode begin  \
finish end sizebefore sizeafter gccversion  \
build elf hex eep lss sym coff extcoff      \
clean clean_list clean_binary program debug \
gdb-config
