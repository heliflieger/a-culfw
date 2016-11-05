###############################################################
#####
##### Makefile for boop - OpenSource firmware for Betty
##### Created at 30.8.2007 02:26 am 
#####
##### boop V0.1 by netguy - ck@mamalala.net
##### Makefile V0.1 by alterego - alteregon@gmx.net
##### Makefile v0.2 by Tobias - tobias-betty@23.gs
#####
###############################################################

###############################################################
#####
##### PATHS (default installation)
#####
##### You can put your path-config into Makefile.local
##### to override these defaults
#####
###############################################################

#ARMBASE = F:/Tools/GNU_Tools_ARM_Embedded/4.8

INCLUDEPATH = $(ARMBASE)/arm-none-eabi/include
LIBPATH = $(ARMBASE)/arm-none-eabi/lib
#ARMPATH = $(ARMBASE)/bin
TOOLPREFIX = arm-none-eabi-
OPENOCDPATH = F:\Tools\OpenOCD
OPENOCD = $(OPENOCDPATH)\openocd.exe -f $(OPENOCDPATH)\target\sam7x256.cfg -f $(OPENOCDPATH)\interface\uniprog.cfg

GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d

###############################################################
#####
##### Compiler, Linker and Tools
#####
###############################################################

CC = $(ARMPATH)$(TOOLPREFIX)gcc
AS = $(ARMPATH)$(TOOLPREFIX)gcc
LD = $(ARMPATH)$(TOOLPREFIX)gcc
OC = $(ARMPATH)$(TOOLPREFIX)objcopy
OD = $(ARMPATH)$(TOOLPREFIX)objdump
SIZE = $(ARMPATH)$(TOOLPREFIX)size

CPUFLAGS = -mcpu=arm7tdmi-s
OPTFLAGS = -O2 -fno-isolate-erroneous-paths-dereference
CFLAGS = -Wall -mlong-calls -ffunction-sections -std=c99 -Dflash -DTRACE_LEVEL=0 -DDBGU_UNIT_IN -D$(OUTPUT) -g $(GENDEPFLAGS)
INC = -I. -I../ -I../../../at91lib -I../../../clib -I../../../at91lib/usb -I$(INCLUDEPATH)
ASFLAGS = -g $(OPTFLAGS) -I. -I../ -I../../../at91lib -D__ASSEMBLY__ -Dflash
LDFLAGS = -g -nostartfiles -Wl,--gc-sections -T"$(OUTPUT)_flash.lds"  -Xlinker -Map=$(OUTPUT).map
LIBS = -lc
THUMBFLAGS = -mthumb

COMPILE = $(CC) $(CPUFLAGS) $(CFLAGS) $(OPTFLAGS) $(INC) 

-include Makefile.local

###############################################################
#####
##### output
#####
###############################################################
OUTPUT = bootloader_HM_CFG

###############################################################
#####
##### Do it
#####
###############################################################
# Recursive expansion of Makefile rules.
define expand_dir
 # Reset vars for subdir for the case that Make.conf does not exist
 SUBDIRS :=
 SRCS := 
 THUMBSRCS :=
 THUMBSRCSUNOPT :=
 -include $(1)Make.conf
 ALLSRCS += $$(SRCS:%=$(1)%)
 ALLTHUMBSRCS += $$(THUMBSRCS:%=$(1)%)
 ALLTHUMBSRCSUNOPT += $$(THUMBSRCSUNOPT:%=$(1)%)
 DEPS += $(1).deps
 $$(foreach adir,$$(SUBDIRS),$$(eval $$(call expand_dir,$(1)$$(adir)/)))
endef

ALLSRCS := board_cstartup.S
ALLTHUMBSRCS :=
ALLTHUMBSRCSUNOPT :=

$(eval $(call expand_dir,))

OBJS := $(patsubst %.S,%.o,$(ALLSRCS:%.c=%.o)) $(ALLTHUMBSRCS:.c=.thumb.o)

all: $(OUTPUT).bin
	$(SIZE) $(OUTPUT).elf
		
%.bin: %.elf
	$(OC) -O binary $< $@

%.hex: %.elf
	$(OC) -O ihex $< $@

$(OUTPUT).elf: $(OBJS)
	$(LD)  -L$(LIBPATH) $(OPTFLAGS) -o $@  $^ $(LIBS) $(LDFLAGS)
	$(SIZE) $^ $(OUTPUT).elf
	
%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $<
	
%.o: %.c
	$(COMPILE) -c -o $@ $<

%.thumb.o: %.c
	$(COMPILE) $(THUMBFLAGS) -c -o $@ $<
		
programBL: $(OUTPUT).bin
	$(OPENOCD) -c init -c 'f_imageBL $<' -c shutdown

program: $(OUTPUT).bin
	$(OPENOCD) -c init -c 'f_image $<' -c shutdown

clean:
	-rm -Rf obj/*
	-rm -Rf .dep/*
	-rm -Rf .dep
	-rm -f $(OBJS) *.elf *.bin *.hex *.map *~

clean2:
	-rm -Rf obj/*
	-rm -Rf .dep/*
	-rm -Rf .dep
	-rm -f $(OBJS) $(OUTPUT).elf $(OUTPUT).bin $(OUTPUT).hex $(OUTPUT).map *~

clean3:
	-rm -Rf obj/*
	-rm -Rf .dep/*
	-rm -Rf .dep
	-rm -f $(OBJS) *.elf *.hex *.map *~
		
$(shell mkdir $(OBJDIR) 2>/dev/null)

-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
