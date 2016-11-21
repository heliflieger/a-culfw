# Firmware für den MapleCUL/MapleCUN


## benötigte Tools

STM32 Flash loader demonstrator: http://www.st.com/en/development-tools/flasher-stm32.html
a-culfw: https://www.mediafire.com/folder/tf16radvztfd9/a-culfw
USB zu TTL Adapter. 


## a-culfw flashen

- RX1/TX1 am Maple Mini mit einem USB/TTL Adapter mit dem PC verbinden.
- USB Kabel am Maple Mini anschließen.
- Taste but=32 am Maple Mini drücken und gerdückt halten.
- Taste reset am Maple Mini drücken.
- Tasten loslassen.
- Flash loader demonstrator starten.
- Com Port des USB/TTL Wandler auswähle, Baud Rate 115200, Parity Even, Echo Disabled, Timeout 1.
- Auf Next drücken.
- Es sollte die Meldung erscheinen. Target is readable. Please click "Next" to proceed.
- Auf Next drücken.
- Auf Next drücken.
- Download to device auswählen.
- Datei MapleCUL.bin bzw. MapleCUN.bin öffnen.
- Erase necessary pages auswählen.
- Auf Next drücken.
- Warten bis Flashvorgang abgeschlossen ist.
- USB Kabel am Maple Mini entfernen. 

