# Alternative culfw for cul devices
___
**The orginal fimware is cloned from www.culfw.de (thanks for the work to R.K.)**

**This alternative firmware has additional send receive methods and code cleanups.**  
_The compiled firmware can be found at [MediaFire](https://www.mediafire.com/folder/tf16radvztfd9/a-culfw)_

## Changelog:


#### 1.20.00
- Implement receive of Manchester coded signals linke Oregon2, Oregon3 or Hideki
- Enable receive of revolt for all devices
- Change IT V1 send timing to old 1.05.01 timing

#### 1.10.02
- Fix ethernet on CUBe
- Fix UNKNOWNCODE ZERR30D (Forum #332883)

#### 1.10.01
- Fix receiving of revolt for all devices.

#### 1.10.00
- ARM Models: Fix bootloader for AT91SAM7 REV C
              Activate 433MHZ protocols
- Rewrite of receive method for TCM/IT/HE
  Receive the protocolls more stable
  Receive additional sync protocols like Eurochron EAS800
- Change receive datarate in slowrf to 5,603 kBaud.
  (You must make a reset of the cul eeprom, use command 
  "set CUL-NAME raw e" in fhem)
- Fix receiving of revolt.

#### 1.05.04
- ARM Models: Add ethernet, add multi CC1101 support
- Code Cleanup

#### 1.05.03
- Fix timing for IT-V1 sending and receive timing for temp sensors

#### 1.05.02
- Revert timing for Elro AB440 IT Switch changes

#### 1.05.01
- Fix timing for Elro AB440 IT Switch
- Fix size for nanoCul

#### 1.05.00
- CULV3: add belfox (Forum #36810)
- CUN*: retransmit patches from doubh (Forum #36529)
- CUL: due to mem space issues: disabled by default: RF_RWE / MBUS_TX
- added RF-Native Mode for RFM12 based protocols i.e. LaCrosse/IT+/PCA302,
  ie.  use "Nr1" 
- add Bff to test the watchdog (Forum #36215)
- CUNO2: added SOMFY RTS support to board definition (thdankert)
- completed wireless m-bus RX/TX support (for T or S mode)

#### 1.04.01
- Implement sending for HomeEasy EU protocol (command ie<value>)

#### 1.04.00
- Implement sending for HomeEasy HE800 protocol (command ih<value>)
- Receive HomeEasy protocols

#### 1.03.04
- Reenable IT-Sending with CUL 868Mhz version
- Change IT receive and sending timing to make it more stable
- Cleanup code to reduce memory usage

#### 1.03.03
- From culfw: add Bff to test the watchdog (Forum #36215)
- From culfw: CUNO2: added SOMFY RTS support to board definition (thdankert)
- Fix size of 868MHz CUL (disable Hoermann, disable IT-Sending with 868Mhz 
  version)

#### 1.03.02
- Enable Oregon3 in CUL for testing

#### 1.03.01
- Split CULs in 433MHz and 868MHz version
- Implement Oregon 3, disabled by default. Define HAS_OREGON3

#### 1.02.02
- completed wireless m-bus RX/TX support (for T or S mode)

#### 1.02.00
- receive of other temperatur sensors like Rubicon and Prologue
- Cleanup some code

#### 1.01.00
- add support for intertechno v3 dimmer

#### 1.00.01
- Bugfix in rf_receive

#### 1.00.00
- Receive for TCM sensors like No.97001, No.212836
- Send/receive for IT Protocoll 1 and 3
- Cleanup of receive methods 
- Additional devices like miniCul, nanoCul, CUL-Arduino


#### License
```
This program is free software; you can redistribute it and/or modify it under  
the terms of the GNU General Public License as published by the Free Software  
Foundation; either version 2 of the License, or (at your option) any later  
version.

This program is distributed in the hope that it will be useful, but  
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY  
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for  
more details.

You should have received a copy of the GNU General Public License along with  
this program; if not, write to the  
Free Software Foundation, Inc.,  
51 Franklin St, Fifth Floor, Boston, MA 02110, USA
```
