#!/bin/bash
# 
# This Script flash the cul device
#
# To init call ./flash.sh
#

# The file for flashing the device
FLASH_FILE=CUL_V3.hex

# The MCU
MCU=atmega32u4

# working dir
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

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
echo " 1 = CUL_V2"
echo " 2 = CUL_V2_HM"
echo " 3 = CUL_V2_MAX"
echo " 4 = CUL_V3"
echo " 5 = CUL_V4"
read -p "Please select device (1-5): " device
if [ "X$device" == "X" -o "$device" != "1" -a "$device" != "2" -a "$device" != "3" -a "$device" != "4" -a "$device" != "5" ] ; then
   echo "Abort"
   exit
fi
if [ "$device" == "1" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2.hex
elif [ "$device" == "2" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_HM.hex
elif [ "$device" == "3" ] ; then
  MCU=at90usb162
  FLASH_FILE=CUL_V2_MAX.hex
elif [ "$device" == "4" ] ; then
  MCU=atmega32u4
  FLASH_FILE=CUL_V3.hex
elif [ "$device" == "5" ] ; then
  MCU=atmega32u2
  FLASH_FILE=CUL_V4.hex
fi

echo ""
echo "The device will now be flashed"
read -p "Continue (y/n)?" flashdevice

if [ "$flashdevice" == "y" -o "$flashdevice" == "Y" -o "$flashdevice" == "j" -o "$flashdevice" == "J" ] ; then
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



