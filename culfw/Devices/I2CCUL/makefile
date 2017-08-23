MCU = atmega328p
FLASH_SIZE = 32768
BOOTLOADER_SIZE = 2048
F_CPU = 16000000
FORMAT = ihex
OBJDIR = .

DATE=$(shell date +%Y-%m-%d)
DESTDIR=culfw-I2CCUL

SRC = I2CCUL.c                                            \
		../../clib/cc1100.c                                  \
		../../clib/cc1101_pllcheck.c                         \
		../../clib/clock.c                                   \
		../../clib/belfox.c                                  \
		../../clib/delay.c                                   \
		../../clib/display.c                                 \
		../../clib/stringfunc.c                              \
		../../clib/fncollection.c                            \
		../../clib/fband.c                                   \
		../../clib/ringbuffer.c                              \
		../../clib/fht.c                                     \
		../../clib/rf_send.c                                 \
		../../clib/rf_receive.c                              \
		../../clib/rf_receive_bucket.c                       \
		../../clib/rf_receive_tcm97001.c                     \
		../../clib/rf_receive_it.c                           \
		../../clib/rf_receive_tx3.c                          \
		../../clib/rf_receive_revolt.c                       \
		../../clib/rf_receive_esa.c                          \
		../../clib/rf_receive_hms.c                          \
		../../clib/rf_native.c                               \
		../../clib/lacrosse.c                                \
		../../clib/helper.c                                  \
		../../clib/rf_asksin.c                               \
		../../clib/rf_moritz.c                               \
		../../clib/rf_rwe.c                                  \
		../../clib/somfy_rts.c                               \
		../../clib/fastrf.c                               	 \
		../../clib/rf_zwave.c                                \
		../../clib/intertechno.c                             \
		../../clib/kopp-fc.c                                 \
		../../clib/memory.c                                  \
		../../clib/i2cslave.c                                \
		../../clib/ttydata.c                                 \
		../../clib/spi.c                                     \
		../../clib/rf_mbus.c                                 \
		../../clib/mbus/manchester.c                         \
		../../clib/mbus/3outof6.c                            \
		../../clib/mbus/mbus_packet.c                        \
		../../clib/mbus/crc.c                                \
		../../clib/ir.c                                \
		../../clib/irmp.c                                \
		../../clib/irsnd.c                                

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
CFLAGS += -D$(TARGET)
CFLAGS += -Wa,-adhlns=$(<:%.c=$(OBJDIR)/%.lst)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)
CFLAGS += -mcall-prologues
CFLAGS += -DF_CPU=$(F_CPU)UL
#CFLAGS += -Wunreachable-code


CPPFLAGS = -g$(DEBUG)
CPPFLAGS += $(CPPDEFS)
CPPFLAGS += -O$(OPT)
CPPFLAGS += -funsigned-char
CPPFLAGS += -funsigned-bitfields
CPPFLAGS += -fpack-struct
CPPFLAGS += -fshort-enums
CPPFLAGS += -fno-exceptions
CPPFLAGS += -Wall
CPPFLAGS += -Wundef
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
#AVRDUDE_PROGRAMMER = avr109
AVRDUDE_PROGRAMMER = arduino
AVRDUDE_PORT = /dev/ttyUSB0 -b 57600
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
AVRDUDE_FLAGS = -D -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)
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



all:
	make all-sw
	make all-hw

all-hw:
	make TARGET=I2CCUL868 mostly_clean buildhw
	make TARGET=I2CCUL433 mostly_clean buildhw


all-sw: 
	make TARGET=I2CCUL868 I2CSLAVE_ADDR=0x70 mostly_clean buildsw
	make TARGET=I2CCUL433 I2CSLAVE_ADDR=0x71 mostly_clean buildsw
	make TARGET=I2CCUL868 I2CSLAVE_ADDR=0x72 mostly_clean buildsw
	make TARGET=I2CCUL433 I2CSLAVE_ADDR=0x73 mostly_clean buildsw

buildhw: CFLAGS += -DI2CSLAVE_HWADDR
buildhw: elfhw hexhw eephw lsshw symhw
	
buildsw: CFLAGS += -DI2CSLAVE_ADDR=$(I2CSLAVE_ADDR)
buildsw: elfsw hexsw eepsw lsssw symsw

elfhw: $(TARGET)-hw.elf
hexhw: $(TARGET)-hw.hex
eephw: $(TARGET)-hw.eep
lsshw: $(TARGET)-hw.lss
symhw: $(TARGET)-hw.sym
LIBNAME=lib$(TARGET)-$(I2CSLAVE_ADDR).a

elfsw: $(TARGET)-$(I2CSLAVE_ADDR).elf
hexsw: $(TARGET)-$(I2CSLAVE_ADDR).hex
eepsw: $(TARGET)-$(I2CSLAVE_ADDR).eep
lsssw: $(TARGET)-$(I2CSLAVE_ADDR).lss
symsw: $(TARGET)-$(I2CSLAVE_ADDR).sym
LIBNAME=lib$(TARGET)-$(I2CSLAVE_ADDR).a

lib: $(LIBNAME)



begin:
	@echo
	@echo $(MSG_BEGIN)

end:
	@echo $(MSG_END)
	@echo


HEXSIZE = $(SIZE) $(TARGET).hex
ELFSIZE = $(SIZE) --format=avr --mcu=$(MCU) $(TARGET).elf
PROGRAM_SIZE = $(shell $(SIZE) --format=avr --mcu=$(MCU) $(TARGET).elf | grep Program | tr -s " " | cut -d " " -f 2 ; echo +$(BOOTLOADER_SIZE))
PROGRAM_SIZE_PLUS_BL=$(shell echo $(PROGRAM_SIZE) | bc)

sizebefore:
	@if test -f $(TARGET).elf; then echo; echo $(MSG_SIZE_BEFORE); $(ELFSIZE); \
	2>/dev/null; echo; fi

sizeafter:
	@if test -f $(TARGET).elf; then echo; echo $(MSG_SIZE_AFTER); $(ELFSIZE); \
	2>/dev/null; echo; fi
	@echo "PROGRAM_SIZE_PLUS_BL: $(PROGRAM_SIZE_PLUS_BL), FLASH_SIZE: $(FLASH_SIZE) \n"
	@if test $(PROGRAM_SIZE_PLUS_BL) -gt $(FLASH_SIZE); then echo "FLASH SIZE OVERFLOW ($(PROGRAM_SIZE_PLUS_BL)>$(FLASH_SIZE))"; exit 2; fi; 

gccversion : 
	@$(CC) --version

program-433: 
	make TARGET=I2CCUL433 buildhw program

program-868:
	make TARGET=I2CCUL868 buildhw program

program: $(TARGET).hex
	#@if test ! -d /sys/class/gpio/gpio17; then echo 17 > /sys/class/gpio/export; fi
	#@if test ! -d /sys/class/gpio/gpio18; then echo 18 > /sys/class/gpio/export; fi
	#echo out > /sys/class/gpio/gpio17/direction
	#echo out > /sys/class/gpio/gpio18/direction
	#echo 0 > /sys/class/gpio/gpio17/value
	#echo 0 > /sys/class/gpio/gpio18/value
	#sleep 1
	#echo 1 > /sys/class/gpio/gpio17/value
	#sleep 1
	#echo 1 > /sys/class/gpio/gpio18/value
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)

deb: $(TARGET).hex
	echo $(PWD)
	rm -rf .f $(DESTDIR) *.deb
	mkdir -p .f/tmp
	cp -r DEBIAN .f/
	rm -fr .f/DEBIAN/CVS
	cp $(TARGET).hex .f/tmp/CSM-nanoCUL.hex	
	find .f -type f -print |\
		xargs perl -pi -e 's/=VERS=/$(VERS)/g;s/=DATE=/$(DATE)/g'
	chown -R root:root .f
	mv .f $(DESTDIR)
	dpkg-deb --build $(DESTDIR)
	rm -rf $(DESTDIR)

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
	@echo $(CC) -c $(ALL_CFLAGS) $< -o $@ 
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

clean_list:
	@echo $(MSG_CLEANING)
	@$(REMOVE) *.eep
	@$(REMOVE) *.cof
	@$(REMOVE) *.elf
	@$(REMOVE) *.map
	@$(REMOVE) *.sym
	@$(REMOVE) *.lss
	@$(REMOVE) *.hex
	@$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.o)
	@$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.lst)
	@$(REMOVE) $(SRC:.c=.s)
	@$(REMOVE) $(SRC:.c=.d)
	@$(REMOVE) $(SRC:.c=.i)
	@$(REMOVEDIR) .dep


$(shell mkdir $(OBJDIR) 2>/dev/null)


-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


.PHONY : all begin  \
finish end sizebefore sizeafter gccversion  \
build elf hex eep lss sym coff extcoff      \
clean clean_list clean_binary program debug \
gdb-config
