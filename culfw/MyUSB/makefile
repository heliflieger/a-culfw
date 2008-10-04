#
#             MyUSB Library
#     Copyright (C) Dean Camera, 2008.
#              
#  dean [at] fourwalledcubicle [dot] com
#      www.fourwalledcubicle.com
#

# Makefile for the MyUSB library itself.

MYUSB_SRC_FILES =    ./Drivers/USB/LowLevel/LowLevel.c           \
                     ./Drivers/USB/LowLevel/Endpoint.c           \
                     ./Drivers/USB/LowLevel/Pipe.c               \
                     ./Drivers/USB/LowLevel/DevChapter9.c        \
                     ./Drivers/USB/LowLevel/HostChapter9.c       \
                     ./Drivers/USB/LowLevel/Host.c               \
                     ./Drivers/USB/HighLevel/USBTask.c           \
                     ./Drivers/USB/HighLevel/USBInterrupt.c      \
                     ./Drivers/USB/HighLevel/Events.c            \
                     ./Drivers/USB/HighLevel/StdDescriptors.c    \
                     ./Drivers/USB/Class/HIDParser.c             \
                     ./Drivers/USB/Class/ConfigDescriptor.c      \
                     ./Scheduler/Scheduler.c                     \
                     ./MemoryAllocator/DynAlloc.c                \
                     ./Drivers/Board/Temperature.c               \
                     ./Drivers/AT90USBXXX/Serial.c               \
                     ./Drivers/AT90USBXXX/Serial_Stream.c        \

all:
	
clean:
	rm -f $(MYUSB_SRC_FILES:%.c=%.o)
	
clean_list:
