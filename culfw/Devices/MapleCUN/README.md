# Firmware für den MapleCUL/MapleCUN

## benötigte Tools

- STM32 Flash loader demonstrator: http://www.st.com/en/development-tools/flasher-stm32.html
- oder alternativ stm32flash : https://sourceforge.net/p/stm32flash/wiki/Home
- DFU Bootloader: https://github.com/rogerclarkmelbourne/STM32duino-bootloader/blob/master/binaries/maple_mini_boot20.bin
- dfu-util: http://dfu-util.sourceforge.net/
- oder alternativ MSC bootloader: https://github.com/Telekatz/MSC-stm32f103-bootloader/releases
- a-culfw: https://www.mediafire.com/folder/iuf7lue8r578c/a-culfw
- USB zu TTL Adapter. 


## Bootloader flashen

### Bootloader aktivieren
- RX1/TX1 am Maple Mini mit einem USB/TTL Adapter mit dem PC verbinden.
- USB Kabel am Maple Mini anschließen.
- Taste but=32 am Maple Mini drücken und gerdückt halten.
- Taste reset am Maple Mini drücken.
- Tasten loslassen.

### STM32 Flash loader demonstrator
- Flash loader demonstrator starten.
- Com Port des USB/TTL Wandler auswähle, Baud Rate 115200, Parity Even, Echo Disabled, Timeout 1.
- Auf Next drücken.
- Es sollte die Meldung erscheinen. Target is readable. Please click "Next" to proceed.
- Auf Next drücken.
- Auf Next drücken.
- Download to device auswählen.
- Datei maple_mini_boot20.bin oder MSCboot_maplemini.bin öffnen.
- Erase necessary pages auswählen.
- Auf Next drücken.
- Warten bis Flashvorgang abgeschlossen ist.
- USB Kabel am Maple Mini entfernen. 

### Stm32flash
`stm32flash -w maple_mini_boot20.bin -v /dev/ttyxx`

## a-culfw flashen

### DFU Bootloader
- Reset am Maple Mini drücken.
- Während die LED schnell blinkt (ca. 4Hz) dfu-util starten:
`dfu-util --verbose --device 1eaf:0003 --cfg 1 --alt 2 --download MapleCUNx4_W5100_BL.bin`

### MCS Bootloader
- Reset am Maple Mini zweimal innerhalb einer Sekunde drücken.
- Die Firmware Datei auf das vom Bootloader bereitgestellte USB-Laufwerk kopieren.


Hinweis: Für einen MapleCUL ist es egal, ob die Datei MapleCUNx4_W5100_BL.bin oder MapleCUNx4_W5500_BL.bin verwendet wird.
