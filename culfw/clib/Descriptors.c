/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2008  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/
#include "Descriptors.h"
#include "board.h"

#ifdef LUFA
#define NO_DESCRIPTOR_STRING NO_DESCRIPTOR
#endif

const USB_Descriptor_Device_t DeviceDescriptor PROGMEM =
{
  Header:                 {Size: sizeof(USB_Descriptor_Device_t), Type: DTYPE_Device},
          
  USBSpecification:       VERSION_BCD(01.10),
  Class:                  0x02,
  SubClass:               0x00,
  Protocol:               0x00,
                          
  Endpoint0Size:          8,
          
  VendorID:               0x03EB,
  ProductID:              0x204B,
  ReleaseNumber:          0x0000,
          
  ManufacturerStrIndex:   0x01,
  ProductStrIndex:        0x02,
#ifdef LUFA
  SerialNumStrIndex:      USE_INTERNAL_SERIAL,
#else
  SerialNumStrIndex:      NO_DESCRIPTOR_STRING,
#endif
          
  NumberOfConfigurations: 1
};
	
const USB_Descriptor_Configuration_t ConfigurationDescriptor PROGMEM =
{
  Config:
    {
      Header:           {Size: sizeof(USB_Descriptor_Configuration_Header_t),
                         Type: DTYPE_Configuration},
      TotalConfigurationSize: sizeof(USB_Descriptor_Configuration_t),
      TotalInterfaces:        2,
      ConfigurationNumber:    1,
      ConfigurationStrIndex:  NO_DESCRIPTOR_STRING,
      ConfigAttributes:       (USB_CONFIG_ATTR_BUSPOWERED),
      MaxPowerConsumption:    USB_CONFIG_POWER_MA(USB_MAX_POWER)
    },
          
  CCI_Interface:
    {
      Header:           {Size: sizeof(USB_Descriptor_Interface_t),
                         Type: DTYPE_Interface},
      InterfaceNumber:        0,
      AlternateSetting:       0,
      TotalEndpoints:         1,
      Class:                  0x02,
      SubClass:               0x02,
      Protocol:               0x01,
      InterfaceStrIndex:      NO_DESCRIPTOR_STRING
    },

  CDC_Functional_IntHeader:
    {
      FuncHeader:             {Header:
                                 {Size: sizeof(CDC_FUNCTIONAL_DESCRIPTOR(2)),
                                  Type: 0x24},
                                  SubType: 0x00},
      Data:                   {0x01, 0x10}
    },

  CDC_Functional_CallManagement:
    {
      FuncHeader:             {Header:
                                 {Size: sizeof(CDC_FUNCTIONAL_DESCRIPTOR(2)),
                                  Type: 0x24},
                                  SubType: 0x01},
      Data:                   {0x03, 0x01}
    },

  CDC_Functional_AbstractControlManagement:
    {
      FuncHeader:             {Header:
                                 {Size: sizeof(CDC_FUNCTIONAL_DESCRIPTOR(1)),
                                  Type: 0x24},
                                  SubType: 0x02},
      Data:                   {0x06}
    },
          
  CDC_Functional_Union:
    {
      FuncHeader:             {Header:
                               {Size: sizeof(CDC_FUNCTIONAL_DESCRIPTOR(2)),
                                Type: 0x24},
                                SubType: 0x06},
      Data:                   {0x00, 0x01}
    },	

  ManagementEndpoint:
    {
      Header:                 {Size: sizeof(USB_Descriptor_Endpoint_t),
                               Type: DTYPE_Endpoint},
      EndpointAddress:        (ENDPOINT_DESCRIPTOR_DIR_IN |
                               CDC_NOTIFICATION_EPNUM),
      Attributes:             EP_TYPE_INTERRUPT,
      EndpointSize:           CDC_NOTIFICATION_EPSIZE,
      PollingIntervalMS:      0xFF
    },

  DCI_Interface:
    {
      Header:                 {Size: sizeof(USB_Descriptor_Interface_t),
                               Type: DTYPE_Interface},
      InterfaceNumber:        1,
      AlternateSetting:       0,
      TotalEndpoints:         2,
      Class:                  0x0A,
      SubClass:               0x00,
      Protocol:               0x00,
      InterfaceStrIndex:      NO_DESCRIPTOR_STRING
    },

  DataOutEndpoint:
    {
      Header:                 {Size: sizeof(USB_Descriptor_Endpoint_t),
                               Type: DTYPE_Endpoint},
      EndpointAddress:        (ENDPOINT_DESCRIPTOR_DIR_OUT | CDC_RX_EPNUM),
      Attributes:             EP_TYPE_BULK,
      EndpointSize:           USB_BUFSIZE,
      PollingIntervalMS:      0x00
    },
          
  DataInEndpoint:
    {
      Header:                 {Size: sizeof(USB_Descriptor_Endpoint_t),
                               Type: DTYPE_Endpoint},
      EndpointAddress:        (ENDPOINT_DESCRIPTOR_DIR_IN | CDC_TX_EPNUM),
      Attributes:              EP_TYPE_BULK,
      EndpointSize:            USB_BUFSIZE,
      PollingIntervalMS:       0x00
    }
};

const USB_Descriptor_String_t LanguageString PROGMEM =
{
  Header:           {Size: USB_STRING_LEN(1),
                     Type: DTYPE_String},
  UnicodeString:    {LANGUAGE_ID_ENG}
};

const USB_Descriptor_String_t ManufacturerString PROGMEM =
{
  Header:           {Size: USB_STRING_LEN(10),
                     Type: DTYPE_String},
  UnicodeString:    L"busware.de"
};

const USB_Descriptor_String_t ProductString PROGMEM =
{
  Header:           {Size: USB_STRING_LEN(20),
                     Type: DTYPE_String},
  UnicodeString:    BOARD_ID_USTR
};

#ifdef MULTI_FREQ_DEVICE
const USB_Descriptor_String_t ProductString433 PROGMEM =
{
  Header:           {Size: USB_STRING_LEN(20),
                     Type: DTYPE_String},
  UnicodeString:    BOARD_ID_USTR433
};
#endif


#ifdef LUFA
#define DESCRIPTOR_ADDRESS(x) ((void*)&x)
uint16_t
CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                           const uint8_t wIndex,
                           void** const DescriptorAddress)
#else
bool USB_GetDescriptor (const uint16_t wValue,
                        const uint8_t wIndex,
                        void** const DescriptorAddress,
                        uint16_t* const DescriptorSize)
#endif
{
  void*    Address = NULL;
  uint16_t Size    = 0;

  switch (wValue >> 8) {
    case DTYPE_Device:
      Address = DESCRIPTOR_ADDRESS(DeviceDescriptor);
      Size    = sizeof(USB_Descriptor_Device_t);
      break;
    case DTYPE_Configuration:
      Address = DESCRIPTOR_ADDRESS(ConfigurationDescriptor);
      Size    = sizeof(USB_Descriptor_Configuration_t);
      break;
    case DTYPE_String:
      switch (wValue & 0xFF) {
        case 0x00:
          Address = DESCRIPTOR_ADDRESS(LanguageString);
          Size    = pgm_read_byte(&LanguageString.Header.Size);
          break;
        case 0x01:
          Address = DESCRIPTOR_ADDRESS(ManufacturerString);
          Size    = pgm_read_byte(&ManufacturerString.Header.Size);
          break;
        case 0x02:
#ifdef MULTI_FREQ_DEVICE
          if (!bit_is_set(PINB, PB6))
            Address = DESCRIPTOR_ADDRESS(ProductString433);
          else
#endif
          Address = DESCRIPTOR_ADDRESS(ProductString);
          Size    = pgm_read_byte(&ProductString.Header.Size);
          break;
      }
      break;
  }
  
#ifdef LUFA
  *DescriptorAddress = Address;
  return Size;
#else
  if(Address != NULL) {
    *DescriptorAddress = Address;
    *DescriptorSize    = Size;
    return true;
  }
  return false;
#endif
}
