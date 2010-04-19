################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
CPM-Gardenabodenfeuchtesensor.lss \

FLASH_IMAGE += \
CPM-Gardenabodenfeuchtesensor.hex \

SIZEDUMMY += \
sizedummy \

AVRDUDEDUMMY += \
avrdudedummy \


# All Target
all: CPM-Gardenabodenfeuchtesensor.elf secondary-outputs

# Tool invocations
CPM-Gardenabodenfeuchtesensor.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,CPM-Gardenabodenfeuchtesensor.map -mmcu=attiny84 -o"CPM-Gardenabodenfeuchtesensor.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

CPM-Gardenabodenfeuchtesensor.lss: CPM-Gardenabodenfeuchtesensor.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S CPM-Gardenabodenfeuchtesensor.elf  >"CPM-Gardenabodenfeuchtesensor.lss"
	@echo 'Finished building: $@'
	@echo ' '

CPM-Gardenabodenfeuchtesensor.hex: CPM-Gardenabodenfeuchtesensor.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex CPM-Gardenabodenfeuchtesensor.elf  "CPM-Gardenabodenfeuchtesensor.hex"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: CPM-Gardenabodenfeuchtesensor.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=attiny84 CPM-Gardenabodenfeuchtesensor.elf
	@echo 'Finished building: $@'
	@echo ' '

avrdudedummy: CPM-Gardenabodenfeuchtesensor.elf
	@echo 'Invoking: AVRDude'
	C:\Programme\winavr\bin\avrdude -pt84 -cavrispmkII -Pusb -Uflash:w:CPM-Gardenabodenfeuchtesensor.hex:a
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(ELFS)$(ASM_DEPS)$(OBJS)$(AVRDUDEDUMMY)$(S_UPPER_DEPS)$(LSS)$(FLASH_IMAGE)$(S_DEPS)$(C_DEPS)$(SIZEDUMMY) CPM-Gardenabodenfeuchtesensor.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(SIZEDUMMY) $(AVRDUDEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
