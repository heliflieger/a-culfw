#!/bin/bash
# 
# This Script flash the cul device
#
# To init call ./flash.sh
#

# The file for flashing the device
FLASH_FILE=CUL_V3_433MHZ.hex

# The MCU
MCU=atmega32u4

# working dir
DIR=$(cd -P -- "$(dirname -- "$0")" && pwd -P)

# Default Port
PORT=/dev/ttyACM0
# Programming baudrate
BAUD=57600
# The programmer
PROGRAMMER=dfu-programmer

if [ ! -e "$FLASH_FILE" ] ; then
  echo "Program file missing building first source code"
  make
fi 

echo "-------------------------------------------------------------"
echo "This program flash the cul device with new firmware."
echo "Please change the device into the bootloader"
echo "-------------------------------------------------------------"
echo "Please choose a device:"
echo " 1 = CUL_V2 868MHZ"
echo " 2 = CUL_V2_HM 868MHZ"
echo " 3 = CUL_V2_MAX 868MHZ"
echo " 4 = CUL_V3 868MHZ"
echo " 5 = CUL_V4 868MHZ"
echo " 6 = CUL_V2 433MHZ"
echo " 7 = CUL_V2_HM 433MHZ"
echo " 8 = CUL_V2_MAX 433MHZ"
echo " 9 = CUL_V3 433MHZ"
echo " 0 = CUL_V4 433MHZ"
read -p "Please select device (1-5): " device
if [ "X$device" = "X" -o "$device" != "1" -a "$device" != "2" -a "$device" != "3" -a "$device" != "4" -a "$device" != "5" -a "$device" != "6" -a "$device" != "7" -a "$device" != "8" -a "$device" != "9" -a "$device" != "0" ] ; then
   echo "Abort"
   exit
fi
if [ "$device" = "1" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_868MHZ.hex
elif [ "$device" = "2" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_HM_868MHZ.hex
elif [ "$device" = "3" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_MAX_868MHZ.hex
elif [ "$device" = "4" ] ; then
  MCU=atmega32u4
  FLASH_FILE=CUL_V3_868MHZ.hex
elif [ "$device" = "5" ] ; then
  MCU=atmega32u2
  FLASH_FILE=CUL_V4_868MHZ.hex
elif [ "$device" = "6" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_433MHZ.hex
elif [ "$device" = "7" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_HM_433MHZ.hex
elif [ "$device" = "8" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_MAX_433MHZ.hex
elif [ "$device" = "9" ] ; then
  MCU=atmega32u4
  FLASH_FILE=CUL_V3_433MHZ.hex
elif [ "$device" = "0" ] ; then
  MCU=atmega32u2
  FLASH_FILE=CUL_V4_433MHZ.hex
fi

echo ""
echo "The device will now be flashed"
read -p "Continue (y/n)?" flashdevice

if [ "$flashdevice" = "y" -o "$flashdevice" = "Y" -o "$flashdevice" = "j" -o "$flashdevice" = "J" ] ; then
  echo "Flash now device"
  echo "Call: ${PROGRAMMER} ${MCU} erase"
  ${PROGRAMMER} ${MCU} erase 
  echo "Call: ${PROGRAMMER} ${MCU} flash ${FLASH_FILE}"
	${PROGRAMMER} ${MCU} flash ${FLASH_FILE}
  echo "Call: ${PROGRAMMER} ${MCU} start"
	${PROGRAMMER} ${MCU} start
else
  echo "Abort flash"
fi


