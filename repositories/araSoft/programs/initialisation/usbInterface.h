////////////////////////////////////////////////////////////////////
// usbInterface.h
// author: jdavies@hep.ucl.ac.uk
// use: class used interface to EZ-USB chip
// date: May 2011
//
//
//
//
////////////////////////////////////////////////////////////////////

#ifndef USBINTERFACE_H
#define USBINTERFACE_H

//Includes
#include "libusb.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>


class usbInterface {
 public:
  usbInterface(void);
  usbInterface(uint16_t vendor, uint16_t product);
  ~usbInterface(void);
  
  bool createHandles(uint16_t vendor, uint16_t product);
  bool freeHandle(void);
  bool syncWriteTransfer(char * data, int* actual_length);
  bool syncReadTransfer(char * data, int length, int* actual_length);
  bool transfer(char* data, uint16_t ep, int length, int * actual_length);
  
  bool controlTransfer(unsigned char * data, char direction, uint8_t bRequest, uint16_t wValue, uint16_t wLength, libusb_transfer_cb_fn callback);

  int asyncTransfer(unsigned char* data, uint8_t direction, uint8_t EP, int length, libusb_transfer_cb_fn callback);

  int asyncTransferSetup(unsigned char* data, uint8_t direction, uint8_t EP, int length, libusb_transfer_cb_fn callback);
  
  int asyncTransferSubmit(uint8_t direction);

  struct libusb_device_handle* stdHandle;
  static const bool SUCCEED = true;
  static const bool FAILED = false;
  struct libusb_transfer *asyncReadTransfer;
  struct libusb_transfer *asyncWriteTransfer;
  struct libusb_transfer *ctlTransfer;

 private:
 
#define INVALID_HANDLE_VALUE NULL
#define USB_TOUT_MS 50 // 50ms
#define USBFX2_EP_WRITE 0x02 //USBFX2 end point address for bulk write
#define USBFX2_EP_READ 0x86 //USBFX2 end point address for bulk read (EP6IN)
#define USBFX2_INTFNO 0 //USBFX2 interface number
#define USBFX2_CNFNO 1 //USBFX2 configuration number

  /*Endpoint address's
EP1 IN 0x81  EP2 0x02  EP4 0x04  EP6 0x86  EP8 0x88
  */
  
  static const uint16_t USBFX2_VENDOR_ID = 0x04b4;
  static const uint16_t USBFX2_PRODUCT_ID = 0x1003;
  
};

#endif //USBINTERFACE_H
