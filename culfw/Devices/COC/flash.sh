#!/bin/bash
# 
# This Script flash the COC device
#
# To init call ./flash.sh
#

# The file for flashing the device
FLASH_FILE=COC.hex

# The MCU
MCU=atmega32u4

# working dir
DIR=$(cd -P -- "$(dirname -- "$0")" && pwd -P)

# Default Port
PORT=/dev/ttyAMA0
# Programming baudrate
BAUD=38400
# The programmer
PROGRAMMER=avrdude

if [ ! -e "$FLASH_FILE" ] ; then
  echo "Program file missing building first source code"
  make
fi 

echo "-------------------------------------------------------------"
echo "This program flash the COC device with new firmware."
echo "Please change the device into the bootloader"
echo "-------------------------------------------------------------"
echo "Please choose a device:"
echo " 1 = COC"
echo " 2 = COC_radio_only"
read -p "Please select device (1-2): " device
if [ "X$device" = "X" -o "$device" != "1" -a "$device" != "2" ] ; then
   echo "Abort"
   exit
fi
if [ "$device" = "1" ] ; then
  MCU=atmega1284p
  FLASH_FILE=COC.hex
elif [ "$device" = "2" ] ; then
  MCU=atmega1284p
  FLASH_FILE=COC_radio_only.hex
fi

read -p "Please insert the port for your device [default $PORT]: " port
if [ "X$port" = "X" ] ; then
   port="$PORT"
fi
if [ ! -e "$port" ] ; then
  echo "ERROR: Port $port does not exists!"
  exit 1
fi

echo ""
echo "The device will now be flashed"
read -p "Continue (y/n)?" flashdevice

if [ "$flashdevice" = "y" -o "$flashdevice" = "Y" -o "$flashdevice" = "j" -o "$flashdevice" == "J" ] ; then
  echo
	echo calling radio frontends bootloader ...
	echo
	if test ! -d /sys/class/gpio/gpio17; then echo 17 > /sys/class/gpio/export; fi
	echo out > /sys/class/gpio/gpio17/direction
	echo 0 > /sys/class/gpio/gpio17/value

	if test ! -d /sys/class/gpio/gpio18; then echo 18 > /sys/class/gpio/export; fi
	echo out > /sys/class/gpio/gpio18/direction
	echo 0 > /sys/class/gpio/gpio18/value

	echo 1 > /sys/class/gpio/gpio17/value
	sleep 1
	echo 1 > /sys/class/gpio/gpio18/value
	echo in > /sys/class/gpio/gpio18/direction
	echo 18 > /sys/class/gpio/unexport
   
  echo "Call now ${PROGRAMMER} -p${MCU} -cavr109 -P${port} -b${BAUD} -D -Uflash:w:./${FLASH_FILE}:i"
  ${PROGRAMMER} -p${MCU} -cavr109 -P${port} -b${BAUD} -D -Uflash:w:./${FLASH_FILE}:i

  if test -e /sys/bus/i2c/devices/0-0050/eeprom; then echo COC V1.1 $(COCVERS) `date +%F` > /sys/bus/i2c/devices/0-0050/eeprom; fi
else
  echo "Abort flash"
fi



