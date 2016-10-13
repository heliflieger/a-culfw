# Add the full path name of these tools if necessary
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

OBJDIR = .

###############################################################
#####
##### Target
#####
###############################################################

# Deduce project name from name of directory in SW4STM32 directory
# Binaries will be generated with this name (.elf, .bin, .hex, etc)
PROJ_NAME := MapleCUL
BOARD := MapleCUL
MCU_FAMILY := STM32F1
DEVICE_ID := STM32F103CBTx
MPU_TYPE := STM32F103xB
MPU_TYPE_LOWER_CASE := stm32f103xb
CPU := cortex-m3

# Debugger optons, must be empty or GDB
DEBUG ?= GDB

###############################################################
#####
##### Source
#####
###############################################################

SRCS := gpio.c
SRCS += main.c

SRCS += stm32f1xx_hal_msp.c
SRCS += stm32f1xx_it.c

SRCS += ../../clib/ringbuffer.c
SRCS += ../../clib/cdc.c
SRCS += ../../clib/clock.c
SRCS += ../../clib/ttydata.c
SRCS += ../../clib/display.c
SRCS += ../../clib/fncollection.c
SRCS += ../../clib/cc1100.c
SRCS += ../../clib/stringfunc.c
SRCS += ../../clib/fband.c


SRCS += ../../STM32/hal_usart.c
SRCS += ../../STM32/hal_spi.c
SRCS += ../../STM32/usb_device.c
SRCS += ../../STM32/usbd_cdc_if.c
SRCS += ../../STM32/usbd_conf.c
SRCS += ../../STM32/usbd_desc.c

SRCS += ../../STM32/avr/interrupt.c
SRCS += ../../STM32/avr/eeprom.c

SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_pcd.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_pcd_ex.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_spi.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_spi_ex.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_tim.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_ll_usb.c

SRCS += ../../STM32/Drivers/CMSIS/Device/ST/$(MCU_FAMILY)xx/Source/Templates/system_stm32f1xx.c
SRCS += ../../STM32/Drivers/CMSIS/Device/ST/$(MCU_FAMILY)xx/Source/Templates/gcc/startup_stm32f103xb.s

SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c

# Every subdirectory with include files must be described here
H_SUBDIRS := \
../MapleCUL \
../../STM32 \
../../STM32/utility \
../../clib \
../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Inc \
../../STM32/Drivers/CMSIS/Include \
../../STM32/Drivers/CMSIS/Device/ST/$(MCU_FAMILY)xx/Include \
../../STM32/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Inc/

LD_FILE := $(DEVICE_ID)_FLASH.ld

OBJS := $(patsubst %.s,%.o,$(SRCS:%.c=%.o)) 

###############################################################
#####
##### Compiler, Linker and Tools
#####
###############################################################

ifeq ($(DEBUG),GDB)
OPTIMIZE	 = -O0
LTO_FLAGS	 = $(OPTIMIZE)
else
OPTIMIZE	 = -Os
LTO_FLAGS	 =  -flto -fuse-linker-plugin $(OPTIMIZE)
endif

CFLAGS := -mcpu=$(CPU) -mthumb 
CFLAGS := $(CFLAGS) -D$(DEVICE_ID) -D$(MCU_FAMILY) -D$(BOARD) -DSTM32 -DDEBUG -DUSE_HAL_DRIVER -D$(MPU_TYPE)
CFLAGS := $(CFLAGS) -std=c99
CFLAGS := $(CFLAGS) $(foreach d,$(H_SUBDIRS),-I$(d))
CFLAGS := $(CFLAGS) $(LTO_FLAGS) -g3 -Wall -fmessage-length=0 -ffunction-sections -c 
GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d

ASMFLAGS := -mcpu=$(CPU) -mthumb 
ASMFLAGS := $(ASMFLAGS) -g -c

LDFLAGS := -mcpu=$(CPU) -mthumb 

LDFLAGS := $(LDFLAGS) -T$(LD_FILE)
LDFLAGS := $(LDFLAGS) -Wl,-Map=$(PROJ_NAME).map -Wl,--gc-sections -lm

COMPILE = $(CC) $(CFLAGS) $(GENDEPFLAGS)

###############################################################
#####
##### Do it
#####
###############################################################

.PHONY: proj

all: proj

proj: $(PROJ_NAME).elf

%.o: %.c
	$(COMPILE) -c -o $@ $<
		
%.o: %.s
	$(CC) $(ASMFLAGS) -c -o $@ $<

$(PROJ_NAME).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@ 
	$(OBJCOPY) -O ihex $(PROJ_NAME).elf $(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin
	arm-none-eabi-size $(PROJ_NAME).elf

clean:
	-rm -Rf .dep/*
	-rm -Rf .dep
	rm -f $(OBJS) $(PROJ_NAME).elf $(PROJ_NAME).hex $(PROJ_NAME).bin $(PROJ_NAME).map


info:
	@echo "Project Name: $(PROJ_NAME)"
	@echo "Board: $(BOARD)"
	@echo "MCU Family: $(MCU_FAMILY)"
	@echo "Device Id: $(DEVICE_ID)"
	@echo "MPU Type: $(MPU_TYPE)"
	@echo "CPU: $(CPU)"

-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)