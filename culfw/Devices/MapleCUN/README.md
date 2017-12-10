# Firmware f�r den MapleCUL/MapleCUN

## ben�tigte Tools

- STM32 Flash loader demonstrator: http://www.st.com/en/development-tools/flasher-stm32.html
- oder alternativ stm32flash : https://sourceforge.net/p/stm32flash/wiki/Home
- Maple mini Bootloader: https://github.com/rogerclarkmelbourne/STM32duino-bootloader/blob/master/binaries/maple_mini_boot20.bin
- dfu-util: http://dfu-util.sourceforge.net/
- a-culfw: https://www.mediafire.com/folder/iuf7lue8r578c/a-culfw
- USB zu TTL Adapter. 


## Maple mini Bootloader flashen

### Bootloader aktivieren
- RX1/TX1 am Maple Mini mit einem USB/TTL Adapter mit dem PC verbinden.
- USB Kabel am Maple Mini anschlie�en.
- Taste but=32 am Maple Mini dr�cken und gerd�ckt halten.
- Taste reset am Maple Mini dr�cken.
- Tasten loslassen.

### STM32 Flash loader demonstrator
- Flash loader demonstrator starten.
- Com Port des USB/TTL Wandler ausw�hle, Baud Rate 115200, Parity Even, Echo Disabled, Timeout 1.
- Auf Next dr�cken.
- Es sollte die Meldung erscheinen. Target is readable. Please click "Next" to proceed.
- Auf Next dr�cken.
- Auf Next dr�cken.
- Download to device ausw�hlen.
- Datei maple_mini_boot20.bin �ffnen.
- Erase necessary pages ausw�hlen.
- Auf Next dr�cken.
- Warten bis Flashvorgang abgeschlossen ist.
- USB Kabel am Maple Mini entfernen. 

### Stm32flash
`stm32flash -w maple_mini_boot20.bin -v /dev/ttyxx`

## a-culfw flashen
- Reset am Maple Mini dr�cken.
- W�hrend die LED schnell blinkt (ca. 4Hz) dfu-util starten:
`dfu-util --verbose --device 1eaf:0003 --cfg 1 --alt 2 --download MapleCUNx4_W5100_BL.bin`

Hinweis: F�r einen MapleCUL ist es egal, ob die Datei MapleCUNx4_W5100_BL.bin oder MapleCUNx4_W5500_BL.bin verwendet wird.
