#!/bin/sh
# fuses for CPM
# 
# check with http://www.engbedded.com/fusecalc/
#
# --- Int. RC Osc. 8 MHz; Start-up time PWRDWN/RESET: 6 CK/14 CK + 64ms; [CKSEL=0010 SUT=10]
# OFF Clock output on PORTB2; [CKOUT=0]
# OFF Divide clock by 8 internally; [CKDIV8=0]
# --- Brown-out detection disabled; [BODLEVEL=111]
# OFF Preserve EEPROM memory through the Chip Erase cycle; [EESAVE=0]
# OFF Watch-dog Timer always on; [WDTON=0]
# ON  Serial program downloading (SPI) enabled; [SPIEN=0]
# OFF Debug Wire enable; [DWEN=0]
# OFF Reset Disabled (Enable PB3 as i/o pin); [RSTDISBL=0]
# OFF Self Programming enable; [SELFPRGEN=0]
#
/usr/bin/avrdude -p t84 -P /dev/ttyS0 -c stk500v2 -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m