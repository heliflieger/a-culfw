Firmware for megaCUL (ATMEGA1284P) device.

See
http://forum.fhem.de/index.php
for details.

FLASHING FIRMWARE

For programming the device with a-culfw execute:

	make clean
	make

on the computer itself. This will require some tools installed upfront. 
Use:

	apt-get install make avrdude

to flash the recompiled megaCUL.hex file.
