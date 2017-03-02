# Firmware für den MapleCUL/MapleCUN


## benötigte Tools

STM32 Flash loader demonstrator: http://www.st.com/en/development-tools/flasher-stm32.html
oder alternativ stm32flash : https://sourceforge.net/p/stm32flash/wiki/Home
a-culfw: https://www.mediafire.com/folder/iuf7lue8r578c/a-culfw
USB zu TTL Adapter. 


## a-culfw flashen

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
- Datei MapleCUNx4_W5100.bin bzw. MapleCUNx4_W5500.bin öffnen. Für einen MapleCUL ist es egal, welche der beiden Dateien verwendet wird.
- Erase necessary pages auswählen.
- Auf Next drücken.
- Warten bis Flashvorgang abgeschlossen ist.
- USB Kabel am Maple Mini entfernen. 

### Stm32flash
`stm32flash -w MapleCUNx4_W5100.bin -v /dev/ttyxx`
