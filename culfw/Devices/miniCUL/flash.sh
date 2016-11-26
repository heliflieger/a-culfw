#!/bin/bash
# 
# This Script flash the cul device
#
# To init call ./flash.sh
#

# The file for flashing the device
FLASH_FILE=miniCUL

# The MCU
MCU=atmega328p

# working dir
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Default Port
PORT=/dev/ttyUSB0
# Programming baudrate
BAUD=57600
# The programmer
PROGRAMMER=avrdude

if [ ! -e "${FLASH_FILE}_433MHZ.hex" ] ; then
  echo "Program file missing building first source code"
  make
fi 

echo "-------------------------------------------------------------"
echo "This program flash the miniCUL device with new firmware."
echo "Please set the device into the bootloader mode"
echo "-------------------------------------------------------------"
echo "Please select a device:"
echo " 1 = miniCUL 868MHz"
echo " 2 = miniCUL 433MHz"
read -p "Please select device (1-2): " frequence
if [ "X$frequence" = "X" -o "$frequence" != "1" -a "$frequence" != "2" ] ; then
   echo "Abort"
   exit
fi
if [ "$frequence" = "1" ] ; then
  FREQ=_868MHZ
elif [ "$frequence" = "2" ] ; then
  FREQ=_433MHZ
fi

echo "Programming mode:"
echo " 1 = Programming via USB"
echo " 2 = Programming over LAN (miniCUL with WLAN)"
read -p "Please select device (1-2): " mode
if [ "X$mode" = "X" -o "$mode" != "1" -a "$mode" != "2" ] ; then
   echo "Abort"
   exit
fi
if [ "$mode" = "1" ] ; then
  	read -p "Please insert the port for your device [default $PORT]: " port
	if [ "X$port" = "X" ] ; then
	   port="$PORT"
	fi
	if [ ! -e "$port" ] ; then
	  echo "ERROR: Port $port does not exists!"
	  exit 1
	fi
elif [ "$mode" = "2" ] ; then
	read -p "Please enter the ipaddress for your device e.g. 192.168.4.1: " ipaddress
	if [ "X$ipaddress" == "X" ] ; then
	   echo "ERROR: Please enter a ipaddress"
           exit 1
	fi
fi



echo ""
echo "The device will now be flashed"
read -p "Continue (y/n)?" flashdevice

if [ "$flashdevice" = "y" -o "$flashdevice" = "Y" -o "$flashdevice" = "j" -o "$flashdevice" = "J" ] ; then  
	if [ "$mode" = "1" ] ; then
  		echo "Call now ${PROGRAMMER} -p ${MCU} -c arduino -P ${port} -b ${BAUD} -D -U flash:w:./${FLASH_FILE}${FREQ}.hex:i"
		${PROGRAMMER} -p ${MCU} -c arduino -P ${port} -b ${BAUD} -D -U flash:w:./${FLASH_FILE}${FREQ}.hex:i
	else
		echo "Change console baudrate to 57600"
		wget -q http://$ipaddress/console/baud?rate=57600
                rm baud?rate=57600

 		echo "Call now 		${PROGRAMMER} -p ${MCU} -c arduino -avrdude -P net:$ipaddress:23 -b 57600 -U flash:w:${FLASH_FILE}${FREQ}.hex:i"
		${PROGRAMMER} -p ${MCU} -c arduino -P net:$ipaddress:23 -b 57600 -U flash:w:${FLASH_FILE}${FREQ}.hex:i
		echo "Change console baudrate to 38400"
		wget -q http://$ipaddress/console/baud?rate=38400
	fi
else
  echo "Abort flash"
fi
