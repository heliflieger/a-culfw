#!/bin/bash
# 
# This Script flash the cul device
#
# To init call ./flash.sh
#

# The file for flashing the device
FLASH_FILE=CUL_ARDUINO.hex

# The MCU
MCU=atmega32u4

# working dir
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Default Port
PORT=/dev/ttyACM0
# Programming baudrate
BAUD=57600
# The programmer
PROGRAMMER=avrdude

if [ ! -e "$FLASH_FILE" ] ; then
  echo "Program file missing building first source code"
  make
fi 

echo "-------------------------------------------------------------"
echo "This program flash the cul device with new firmware."
echo "Please change the device into the bootloader"
echo "-------------------------------------------------------------"
read -p "Please insert the port for your device [default $PORT]: " port
if [ "X$port" == "X" ] ; then
   port="$PORT"
fi
if [ ! -e "$port" ] ; then
  echo "ERROR: Port $port does not exists!"
  exit 1
fi

echo ""
echo "The device will now be flashed"
read -p "Continue (y/n)?" flashdevice

if [ "$flashdevice" == "y" -o "$flashdevice" == "Y" -o "$flashdevice" == "j" -o "$flashdevice" == "J" ] ; then
  echo "Call now ${PROGRAMMER} -p${MCU} -cavr109 -P${port} -b${BAUD} -D -Uflash:w:./${FLASH_FILE}:i"
  ${PROGRAMMER} -p${MCU} -cavr109 -P${port} -b${BAUD} -D -Uflash:w:./${FLASH_FILE}:i
else
  echo "Abort flash"
fi



