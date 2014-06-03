#!/bin/sh

/etc/init.d/fhem stop
sleep 3
echo "calling rpiaddon bootloader..."
    if test ! -d /sys/class/gpio/gpio17; then echo 17 > /sys/class/gpio/export; fi
    if test ! -d /sys/class/gpio/gpio18; then echo 18 > /sys/class/gpio/export; fi
    echo out > /sys/class/gpio/gpio17/direction
    echo out > /sys/class/gpio/gpio18/direction
    echo 0 > /sys/class/gpio/gpio17/value
    echo 0 > /sys/class/gpio/gpio18/value
    sleep 1
    echo 1 > /sys/class/gpio/gpio17/value
    sleep 1
    echo 1 > /sys/class/gpio/gpio18/value

    echo "Programming rpiaddon"
    avrdude -p atmega644p -P /dev/ttyAMA0 -b 38400 -c avr109 -U flash:w:rpiaddon.hex

/etc/init.d/fhem start
