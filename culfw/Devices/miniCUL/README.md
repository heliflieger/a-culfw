Firmware for miniCUL device

See
https://forum.fhem.de/index.php/topic,42998.0.html
for details.

------------------------------------------------------------------------
Hint:
To create a 433MHz miniCUL insert a resistor of 4,7k at pin A0 (PC0) to GND
------------------------------------------------------------------------


FLASHING FIRMWARE

For programming the device simple call ./flash.sh
or execute:

	make clean
	make

on the computer itself. This will require some tools installed upfront. 
Use:

	apt-get install make avrdude

to flash the recompiled miniCUL.hex file.

USING

The a-culfw stack is available on: /dev/ttyUSBx, where x is a number.
