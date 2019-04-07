# Add the full path name of these tools if necessary
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

###############################################################
#####
##### Target
#####
###############################################################

# Deduce project name from name of directory in SW4STM32 directory
# Binaries will be generated with this name (.elf, .bin, .hex, etc)
BOARD := MapleCUL
MCU_FAMILY := STM32F1
DEVICE_ID := STM32F103CBTx
MPU_TYPE := STM32F103xB
MPU_TYPE_LOWER_CASE := stm32f103xb
CPU := cortex-m3

# Debugger options, must be empty or GDB
DEBUG ?= 

###############################################################
#####
##### Source
#####
###############################################################

SRCS += main.c

SRCS += stm32f1xx_hal_msp.c
SRCS += stm32f1xx_it.c

SRCS += ../../clib/cc1100.c
SRCS += ../../clib/cc1101_pllcheck.c  
SRCS += ../../clib/cdc.c
SRCS += ../../clib/cdc_uart.c
SRCS += ../../clib/clock.c
SRCS += ../../clib/delay.c
SRCS += ../../clib/display.c
SRCS += ../../clib/fastrf.c
SRCS += ../../clib/fband.c
SRCS += ../../clib/fht.c
SRCS += ../../clib/fncollection.c
SRCS += ../../clib/hw_autodetect.c
SRCS += ../../clib/intertechno.c 
SRCS += ../../clib/kopp-fc.c 
SRCS += ../../clib/multi_CC.c 
SRCS += ../../clib/rf_asksin.c
SRCS += ../../clib/rf_maico.c
SRCS += ../../clib/rf_mbus.c
SRCS += ../../clib/mbus/mbus_packet.c
SRCS += ../../clib/mbus/manchester.c
SRCS += ../../clib/mbus/3outof6.c
SRCS += ../../clib/mbus/crc.c
SRCS += ../../clib/rf_mode.c
SRCS += ../../clib/rf_moritz.c
SRCS += ../../clib/rf_native.c
SRCS += ../../clib/lacrosse.c
SRCS += ../../clib/rf_router.c
SRCS += ../../clib/rf_receive_bucket.c
SRCS += ../../clib/rf_receive_esa.c
SRCS += ../../clib/rf_receive_hms.c
SRCS += ../../clib/rf_receive_it.c
SRCS += ../../clib/rf_receive_revolt.c
SRCS += ../../clib/rf_receive_tcm97001.c
SRCS += ../../clib/rf_receive_tx3.c
SRCS += ../../clib/rf_receive.c
SRCS += ../../clib/rf_router.c
SRCS += ../../clib/rf_rwe.c
SRCS += ../../clib/rf_send.c
SRCS += ../../clib/rf_zwave.c
SRCS += ../../clib/ringbuffer.c
SRCS += ../../clib/serial.c
SRCS += ../../clib/somfy_rts.c
SRCS += ../../clib/stringfunc.c
SRCS += ../../clib/ttydata.c
SRCS += ../../clib/onewire.c

SRCS += ../../clib/ethernet_wiznet.c

SRCS += ../../STM32/hal_usart.c
SRCS += ../../STM32/hal_spi.c
SRCS += ../../STM32/hal_gpio.c
SRCS += ../../STM32/hal_timer.c
SRCS += ../../STM32/hal_twimaster.c
SRCS += ../../STM32/stdio.c

SRCS += ../../STM32/usbd/usb_device.c
SRCS += ../../STM32/usbd/usbd_cdc_if.c
SRCS += ../../STM32/usbd/usbd_conf.c
SRCS += ../../STM32/usbd/usbd_desc.c

SRCS += ../../STM32/avr/interrupt.c
SRCS += ../../STM32/avr/eeprom.c
SRCS += ../../STM32/avr/wdt.c
SRCS += ../../STM32/util/parity.c

SRCS += ../../STM32/utility/dbgu.c

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
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_hal_iwdg.c
SRCS += ../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Src/stm32f1xx_ll_usb.c

SRCS += ../../STM32/Drivers/CMSIS/Device/ST/$(MCU_FAMILY)xx/Source/Templates/system_stm32f1xx.c
SRCS += ../../STM32/Drivers/CMSIS/Device/ST/$(MCU_FAMILY)xx/Source/Templates/gcc/startup_stm32f103xb.s

SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
SRCS += ../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c

SRCS += ../../Wiznet/Ethernet/W5100/w5100.c
SRCS += ../../Wiznet/Ethernet/W5500/w5500.c
SRCS += ../../Wiznet/Ethernet/socket.c
SRCS += ../../Wiznet/Ethernet/wizchip_conf.c
SRCS += ../../Wiznet/Internet/DHCP/dhcp.c

# Every subdirectory with include files must be described here
H_SUBDIRS := \
../MapleCUN \
../../clib \
../../STM32 \
../../STM32/usbd \
../../STM32/utility \
../../STM32/Drivers/$(MCU_FAMILY)xx_HAL_Driver/Inc \
../../STM32/Drivers/CMSIS/Include \
../../STM32/Drivers/CMSIS/Device/ST/$(MCU_FAMILY)xx/Include \
../../STM32/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
../../STM32/Middlewares/ST/STM32_USB_Device_Library/Core/Inc/ \
../../Wiznet/Ethernet \
../../Wiznet/Ethernet/W5100 \
../../Wiznet/Internet 


OBJS := $(patsubst %.s,%.o,$(SRCS:%.c=%.o)) 

###############################################################
#####
##### Compiler, Linker and Tools
#####
###############################################################

ifeq ($(DEBUG),GDB)
OPTIMIZE	 = -O0
LTO_FLAGS	 = $(OPTIMIZE) -DTRACE_LEVEL=4
else
OPTIMIZE	 = -Os
LTO_FLAGS	 =  -flto -fuse-linker-plugin $(OPTIMIZE) -DTRACE_LEVEL=4
endif

CFLAGS := -mcpu=$(CPU) -mthumb
CFLAGS := $(CFLAGS) -D$(DEVICE_ID) -D$(MCU_FAMILY) -D$(TARGET) 
CFLAGS := $(CFLAGS) -DSTM32 -DDBGU_UNIT_IN -DUSE_HAL_DRIVER -D$(MPU_TYPE) 

ifeq ($(BOOTLOADER),TRUE)
LD_FILE := $(DEVICE_ID)_FLASH_BL.ld
CFLAGS := $(CFLAGS) -DBOOTLOADER
else
LD_FILE := $(DEVICE_ID)_FLASH.ld
endif

CFLAGS := $(CFLAGS) -std=c99 
CFLAGS := $(CFLAGS) $(foreach d,$(H_SUBDIRS),-I$(d))
CFLAGS := $(CFLAGS) $(LTO_FLAGS) -g3 -Wall -fmessage-length=0 -ffunction-sections -c 
GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d

ASMFLAGS := -mcpu=$(CPU) -mthumb 
ASMFLAGS := $(ASMFLAGS) -g -c

LDFLAGS := -mcpu=$(CPU) -mthumb 

LDFLAGS := $(LDFLAGS) -T$(LD_FILE)
LDFLAGS := $(LDFLAGS) -Wl,-Map=$(TARGET).map -Wl,--gc-sections -lm

COMPILE = $(CC) $(CFLAGS) $(GENDEPFLAGS)

###############################################################
#####
##### Do it
#####
###############################################################

.PHONY: proj

all:
	make TARGET=MapleCUNx4_W5100_BL BOOTLOADER=TRUE build clean2
	make TARGET=MapleCUNx4_W5500_BL BOOTLOADER=TRUE build clean2

MapleCUL: 
	make TARGET=MapleCUL build 

MapleCUN: 
	make TARGET=MapleCUN build 
		
MapleCULx4: 
	make TARGET=MapleCULx4 build 

MapleCUNx4: 
	make TARGET=MapleCUNx4 build

MapleCUNx4_W5100: 
	make TARGET=MapleCUNx4_W5100 build

MapleCUNx4_W5500: 
	make TARGET=MapleCUNx4_W5500 build

MapleCUNx4_W5100_BL: 
	make TARGET=MapleCUNx4_W5100_BL BOOTLOADER=TRUE build

MapleCUNx4_W5500_BL: 
	make TARGET=MapleCUNx4_W5500_BL BOOTLOADER=TRUE build
				
build: $(TARGET).elf

%.o: %.c
	$(COMPILE) -c -o $@ $<
		
%.o: %.s
	$(CC) $(ASMFLAGS) -c -o $@ $<

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@ 
	$(OBJCOPY) -O ihex $(TARGET).elf $(TARGET).hex
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin
	arm-none-eabi-size $(TARGET).elf

clean:
	-rm -Rf .dep/*
	-rm -Rf .dep
	rm -f $(OBJS) *.elf *.hex *.bin *.map

clean2:
	-rm -Rf .dep/*
	-rm -Rf .dep
	rm -f $(OBJS) *.elf *.hex *.map
	
info:
	@echo "Project Name: $(TARGET)"
	@echo "Board: $(BOARD)"
	@echo "MCU Family: $(MCU_FAMILY)"
	@echo "Device Id: $(DEVICE_ID)"
	@echo "MPU Type: $(MPU_TYPE)"
	@echo "CPU: $(CPU)"

-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
