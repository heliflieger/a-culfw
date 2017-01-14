# Firmware für den HM-CFG-USB-2


## benötigte Tools

SAM-BA 2.xx: http://www.atmel.com/tools/ATMELSAM-BAIN-SYSTEMPROGRAMMER.aspx
ggf. USB CDC Treiber: http://www.atmel.com/tools/ATMELSAM-BAIN-SYSTEMPROGRAMMER.aspx
Tera Term: http://ttssh2.osdn.jp/index.html.en
a-culfw: https://www.mediafire.com/folder/iuf7lue8r578c/a-culfw

## Bootloader flashen

- Jumper J2 auf dem Stick verbinden und den Stick kurz ein- und wieder ausstecken um die vorhandene Firmware zu löschen und den Flashspeicher zu entsperren.

**Achtung:** Die original Firmware ist danach weg und kann auch nicht wiederhergestellt werden, da keine unverschlüsselte Version davon verfügbar ist!
   
- Jumper J2 lösen und Jumper J1 verbinden.
- Den Stick für etwa 10s einstecken um den SAM-BA Bootloader zu aktivieren.
- Stick ausstecken und Jumper J1 lösen.
- Stick wieder einstecken.
- Einen 10kOhm Widerstand zwischen +5V und Data+ an der Rückseite des USB Stecker in den USB Stecker stecken.
- Unter Windows erscheint im Gerätemanager ein neuer COM Port.
- SAM-BA als Administrator ausführen.
- Den neuen COM Port unter "Select the connection" auswählen.
- Unter "Select your Board" at91sam7s128-ek auswählen.
- Auf Connect Drücken.
- Registerkarte Flash auswählen.
- Unter "Send File Name" die Datei bootloader_HM_CFG.bin auswählen.
- Auf Send File drücken. 
- Die folgende Frage, ob das Flash gelockt werden soll, mit nein beantworten.
- SAM-BA schließen und Stick entfernen. 

Alternative für SAM-BA unter Linux: https://forum.fhem.de/index.php/topic,38404.msg378455.html#msg378455

## a-culfw flashen

- Falls noch keine a-culfw auf dem Stick installiert wurde startet nach dem einstecken des Sticks automatisch der Bootloader.
- Falls bereits die a-culfw auf dem Cube installiert ist kann der Bootloader auch in einer Terminalverbindung mit dem Kommando „B01“ aktiviert werden.
- Der Bootloader kann auch dur verbinden der Pins 3 und 4 von ST1 während des einstecken des Sticks aktiviert werden.
- Bei aktivierten Bootloader blink D1 vier mal pro Sekunde.
- Unter Windows erscheint im Gerätemanager ein neuer COM Port "AT91 USB to Serial Converter".
- Tera Term starten.
- Datei --> Neue Verbindung --> Seriell auswählen.
- Com Port auswählen.
- Auf OK drücken.
- Datei --> Transfer --> XMODEM --> Senden auswählen.
- Die Datei HM_CFG_BL.bin öffnen.
- Nach erfolgreicher Übetragung startet der Stick neu und D1 blinkt im Sekundentakt.

Alternative für Tera Term unter Linux: https://forum.fhem.de/index.php/topic,38404.msg348429.html#msg348429
