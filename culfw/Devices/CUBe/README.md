# Firmware f�r den MAX! Cube


## ben�tigte Tools

SAM-BA 2.xx: http://www.atmel.com/tools/ATMELSAM-BAIN-SYSTEMPROGRAMMER.aspx
ggf. USB CDC Treiber: http://www.atmel.com/tools/ATMELSAM-BAIN-SYSTEMPROGRAMMER.aspx
Tera Term: http://ttssh2.osdn.jp/index.html.en
a-culfw: https://www.mediafire.com/folder/iuf7lue8r578c/a-culfw

## Bootloader flashen

- Die Pins J1 auf dem Cube verbinden und die USB Verbindung kurz ein- und wieder ausstecken um die vorhandene Firmware zu l�schen und den Flashspeicher zu entsperren.

**Achtung:** Die original Firmware ist danach weg und kann auch nicht wiederhergestellt werden, da keine unverschl�sselte Version davon verf�gbar ist!
   
- Verbindung  J1 l�sen und USB Kabel wieder Verbinden.
- Unter Windows erscheint im Ger�temanager ein neuer COM Port.
- SAM-BA als Administrator ausf�hren.
- Den neuen COM Port unter "Select the connection" ausw�hlen.
- Unter "Select your Board" at91sam7x256-ek bzw. at91sam7x512-ek ausw�hlen (je nach verbauten �C).
- Auf Connect Dr�cken.
- Registerkarte Flash ausw�hlen.
- Unter "Send File Name" die Datei bootloader_CUBE.bin ausw�hlen.
- Auf Send File dr�cken. 
- Die folgende Frage, ob das Flash gelockt werden soll, mit nein beantworten.
- Im Feld Scripts �Boot from Flash (GPNVM2)� ausw�hlen.
- Auf Execute dr�cken.
- SAM-BA schlie�en und USB Kabel l�sen. 

Alternative f�r SAM-BA unter Linux: https://forum.fhem.de/index.php/topic,38404.msg378455.html#msg378455

## a-culfw flashen

- Zur aktivierung des Bootloaders den Knopfes an der Unterseite des Cubes dr�cken.
- USB Kabel verbinden.
- Knopf loslassen.
- Falls bereits die a-culfw auf dem Cube installiert ist kann der Bootloader auch in einer Terminalverbindung mit dem Kommando �B01� aktiviert werden.
- Bei aktivierten Bootloader blink D1 vier mal pro Sekunde.
- Unter Windows erscheint im Ger�temanager ein neuer COM Port "AT91 USB to Serial Converter".
- Tera Term starten.
- Datei --> Neue Verbindung --> Seriell ausw�hlen.
- Com Port ausw�hlen.
- Auf OK dr�cken.
- Datei --> Transfer --> XMODEM --> Senden ausw�hlen.
- Die Datei CUBE_BL.bin �ffnen.
- Nach erfolgreicher �betragung startet der Cube neu und D1 blinkt im Sekundentakt.

Alternative f�r Tera Term unter Linux: https://forum.fhem.de/index.php/topic,38404.msg348429.html#msg348429