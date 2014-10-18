#MCU = atmega644p
MCU = atmega1284p
#MCU = atmega324p
BOARD  =
F_CPU = 8000000
FORMAT = ihex
TARGET = CSM
OBJDIR = .

VERS=1.42-6
DATE=$(shell date +%Y-%m-%d)
DESTDIR=culfw-s0logger.$(VERS)

SRC = $(TARGET).c\
		../../clib/cc1100.c                                  \
		../../clib/cc1101_pllcheck.c                         \
		../../clib/clock.c                                   \
		../../clib/delay.c                                   \
		../../clib/display.c                                 \
		../../clib/stringfunc.c                              \
		../../clib/fncollection.c                            \
		../../clib/ringbuffer.c                              \
		../../clib/rf_send.c                                 \
		../../clib/rf_asksin.c                               \
		../../clib/rf_moritz.c                               \
		../../clib/rf_receive.c                              \
		../../clib/rf_router.c                               \
		../../clib/intertechno.c                             \
		../../clib/somfy_rts.c                               \
		../../clib/fht.c                                     \
		../../clib/fastrf.c                                  \
		../../clib/memory.c                                  \
		../../clib/serial.c                                  \
		../../clib/ttydata.c                                 \
		../../clib/spi.c                                     \
                ../../clib/rf_mbus.c                                 \
                ../../clib/mbus/manchester.c                         \
                ../../clib/mbus/3outof6.c                            \
                ../../clib/mbus/mbus_packet.c                        \
                ../../clib/mbus/crc.c                                \
#		../../clib/dogm16x.c                                 \
#		../../clib/ir.c                                      \
#		../../clib/irmp.c                                    \


OPT = s
DEBUG = dwarf-2
EXTRAINCDIRS = ../../clib
CSTANDARD = -std=gnu99
CDEFS  = -DF_CPU=$(F_CPU)UL

ADEFS = -DF_CPU=$(F_CPU)
CPPDEFS = -DF_CPU=$(F_CPU)UL


CFLAGS = -g$(DEBUG)
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


CPPFLAGS = -g$(DEBUG)
CPPFLAGS += $(CPPDEFS)
CPPFLAGS += -O$(OPT)
CPPFLAGS += -funsigned-char
CPPFLAGS += -funsigned-bitfields
CPPFLAGS += -fpack-struct
CPPFLAGS += -fshort-enums
CPPFLAGS += -fno-exceptions
CPPFLAGS += -Wall
CFLAGS += -Wundef
CPPFLAGS += -Wa,-adhlns=$(<:%.cpp=$(OBJDIR)/%.lst)
CPPFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))


ASFLAGS = $(ADEFS) -Wa,-adhlns=$(<:%.S=$(OBJDIR)/%.lst),-gstabs,--listing-cont-lines=100


PRINTF_LIB_MIN = -Wl,-u,vfprintf -lprintf_min
PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt
PRINTF_LIB =
SCANF_LIB_MIN = -Wl,-u,vfscanf -lscanf_min
SCANF_LIB_FLOAT = -Wl,-u,vfscanf -lscanf_flt
SCANF_LIB =
MATH_LIB = -lm
EXTRALIBDIRS =
EXTMEMOPTS =


LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--relax
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(EXTMEMOPTS)
LDFLAGS += $(patsubst %,-L%,$(EXTRALIBDIRS))
LDFLAGS += $(PRINTF_LIB) $(SCANF_LIB) $(MATH_LIB)

#AVRDUDE_NO_VERIFY = -V

#AVRDUDE_PROGRAMMER = jtagmkII
#AVRDUDE_PORT = usb
#AVRDUDE_PROGRAMMER = stk500v2
AVRDUDE_PROGRAMMER = avr109 -b 38400
AVRDUDE_PORT = /dev/ttyUSB3
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)
AVRDUDE_FLAGS += $(AVRDUDE_NO_VERIFY)
AVRDUDE_FLAGS += $(AVRDUDE_VERBOSE)
AVRDUDE_FLAGS += $(AVRDUDE_ERASE_COUNTER)


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
AVRDUDE = avrdude
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp
WINSHELL = cmd

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
MSG_ASSEMBLING = Assembling:
MSG_CLEANING = Cleaning project:
MSG_CREATING_LIBRARY = Creating library:




OBJ = $(SRC:%.c=$(OBJDIR)/%.o) $(CPPSRC:%.cpp=$(OBJDIR)/%.o) $(ASRC:%.S=$(OBJDIR)/%.o)

LST = $(SRC:%.c=$(OBJDIR)/%.lst) $(CPPSRC:%.cpp=$(OBJDIR)/%.lst) $(ASRC:%.S=$(OBJDIR)/%.lst)


GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d


ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)
ALL_CPPFLAGS = -mmcu=$(MCU) -I. -x c++ $(CPPFLAGS) $(GENDEPFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)



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

checkhooks: build
	@echo
	@echo ------- Unhooked MyUSB Events -------
	@$(shell) (grep -s '^Event.*MyUSB/.*\\.o' $(TARGET).map | \
	           cut -d' ' -f1 | cut -d'_' -f2- | grep ".*") || \
			   echo "(None)"
	@echo ----- End Unhooked MyUSB Events -----

checklibmode:
	@echo
	@echo ----------- Library Mode -----------
	@$(shell) ($(CC) $(ALL_CFLAGS) -E -dM - < /dev/null \
	          | grep 'USB_\(DEVICE\|HOST\)_ONLY' | cut -d' ' -f2 | grep ".*") \
	          || echo "No specific mode (both device and host mode allowable)."
	@echo ------------------------------------

gccversion :
	@$(CC) --version


program: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM) -U lfuse:w:0xc2:m -U hfuse:w:0x99:m

program_serial: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)

erase:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -e


usbprogram: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)

deb: $(TARGET).hex
	echo $(PWD)
	rm -rf .f $(DESTDIR) *.deb
	mkdir -p .f/boot
	cp -r DEBIAN .f/
	rm -fr .f/DEBIAN/CVS
	cp $(TARGET).hex .f/boot/culfw-s0logger.hex
	find .f -type f -print | xargs perl -pi -e 's/=VERS=/$(VERS)/g;s/=DATE=/$(DATE)/g'
	chown -R root:root .f
	mv .f $(DESTDIR)
	dpkg-deb --build $(DESTDIR)
	rm -rf $(DESTDIR)
	scp $(DESTDIR).deb busware.de:/srv/ftp/debian/dists/tuxrail2/main/
	@echo run /root/update_apt.sh


gdb-config:
	@$(REMOVE) $(GDBINIT_FILE)
	@echo define reset >> $(GDBINIT_FILE)
	@echo SIGNAL SIGHUP >> $(GDBINIT_FILE)
	@echo end >> $(GDBINIT_FILE)
	@echo file $(TARGET).elf >> $(GDBINIT_FILE)
	@echo target remote $(DEBUG_HOST):$(DEBUG_PORT)  >> $(GDBINIT_FILE)
ifeq ($(DEBUG_BACKEND),simulavr)
	@echo load  >> $(GDBINIT_FILE)
endif
	@echo break main >> $(GDBINIT_FILE)

debug: gdb-config $(TARGET).elf
ifeq ($(DEBUG_BACKEND), avarice)
	@echo Starting AVaRICE - Press enter when "waiting to connect" message displays.
	@$(WINSHELL) /c start avarice --jtag $(JTAG_DEV) --erase --program --file \
	$(TARGET).elf $(DEBUG_HOST):$(DEBUG_PORT)
	@$(WINSHELL) /c pause

else
	@$(WINSHELL) /c start simulavr --gdbserver --device $(MCU) --clock-freq \
	$(DEBUG_MFREQ) --port $(DEBUG_PORT)
endif
	@$(WINSHELL) /c start avr-$(DEBUG_UI) --command=$(GDBINIT_FILE)




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



.SECONDARY : $(TARGET).a
.PRECIOUS : $(OBJ)
%.a: $(OBJ)
	@echo
	@echo $(MSG_CREATING_LIBRARY) $@
	$(AR) $@ $(OBJ)


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


%.s : %.cpp
	$(CC) -S $(ALL_CPPFLAGS) $< -o $@


$(OBJDIR)/%.o : %.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(CC) -c $(ALL_ASFLAGS) $< -o $@


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
