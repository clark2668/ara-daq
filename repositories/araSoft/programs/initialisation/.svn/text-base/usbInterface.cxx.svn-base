////////////////////////////////////////////////////////////////////
//usbInterface.cxx
//author: jdavies@hep.ucl.ac.uk
//use: class used interface to EZ-USB chip
//date: May 2011
//
//
//
//
////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <iostream>
#include "usbInterface.h"

usbInterface::usbInterface(void) {
  usbInterface::stdHandle = INVALID_HANDLE_VALUE;
  createHandles(USBFX2_VENDOR_ID, USBFX2_PRODUCT_ID);
}
usbInterface::usbInterface(uint16_t vendor, uint16_t product) {
  usbInterface::stdHandle = INVALID_HANDLE_VALUE;
  createHandles(vendor, product);
}

usbInterface::~usbInterface(void) {
  if(stdHandle != INVALID_HANDLE_VALUE)
    freeHandle();
}

bool usbInterface::createHandles(uint16_t vendor, uint16_t product){
  int retval;
  if (stdHandle != INVALID_HANDLE_VALUE)
    goto ok;
  if(  retval = libusb_init(NULL)) goto fail;  
  stdHandle = libusb_open_device_with_vid_pid(NULL, vendor, product);
    
  retval = libusb_set_configuration(stdHandle, USBFX2_CNFNO);
  if( retval != 0){
    printf("create handles: failing at set config\n");
    goto fail;
  }
  
  retval = libusb_claim_interface(stdHandle, USBFX2_INTFNO);
  if (retval != 0){
    printf("creat handles: failing at claim interface\n");
    goto fail;
  }
  
  retval = libusb_set_interface_alt_setting(stdHandle, USBFX2_INTFNO, USBFX2_INTFNO);
  if (retval != 0){
    printf("failing at altinterface\n");
    goto fail;
  }
  
  goto ok;
  
 ok:
  return SUCCEED;
  
 fail:
  printf("createhandles: FAILED : retval %i : stdHandle %li\n",  retval, (long)stdHandle);
  return FAILED;
}

bool usbInterface::freeHandle(void){
   int retval = libusb_release_interface(stdHandle, USBFX2_INTFNO);
  if(retval!=0) printf("freeHandle: failed to free handle : retval: %i", retval);
  libusb_close(stdHandle);

}

// char * data types

bool usbInterface::syncWriteTransfer(char * data, int* actual_length){
  int length = strlen(data);
  int retval = libusb_bulk_transfer(stdHandle, USBFX2_EP_WRITE, (unsigned char*)data, length, actual_length, USB_TOUT_MS);
  if(retval == 0)
    {
      printf("sendData: sent %i bytes retval: %i\n", *actual_length, retval);
      return SUCCEED;
    }
  else
    {
      printf("sendData: error code %s : actual_length &i\n", strerror(-1*retval), *actual_length);
      return FAILED;
    }
}

bool usbInterface::syncReadTransfer(char* data, int length, int* actual_length) {
  int retval = libusb_bulk_transfer(stdHandle, USBFX2_EP_READ, (unsigned char*)data, length, actual_length, USB_TOUT_MS);
  if(retval == 0)
    {
      printf("readData: read %i of %i bytes\n", *actual_length, length);
      return SUCCEED;
    }
  else
    {
      printf("readData: error code %s : retval: %i actual_length %i\n", strerror(-1*retval),retval,  *actual_length);
      return FAILED;
    }
}
bool usbInterface::controlTransfer(unsigned char * data, char direction, uint8_t bRequest, uint16_t wValue, uint16_t wLength, libusb_transfer_cb_fn callback){
  char bmRequestType = 0;
  
  bmRequestType = (0x02 << 5); // request type = vendor request
  bmRequestType = bmRequestType | (direction << 7); // set the request direction
  int retVal=libusb_control_transfer(stdHandle,bmRequestType,bRequest,wValue,0,data,wLength,USB_TOUT_MS*10);
  if(retVal<0){ 
    printf("Could not send libusb_control_transfer (vendor request): %d\n", retVal);
    return FAILED;
  }
  return SUCCEED;
}
// bool usbInterface::controlTransfer(unsigned char * data, char direction, uint8_t bRequest, uint16_t wValue, uint16_t wLength, libusb_transfer_cb_fn callback){
//   char bmRequestType = 0;
//   unsigned char cOutPtr[LIBUSB_CONTROL_SETUP_SIZE+wLength];
//   for(int i=0; i<LIBUSB_CONTROL_SETUP_SIZE+wLength; i++)
//     cOutPtr[i]=0;
//   ctlTransfer = libusb_alloc_transfer(0);
  
//   bmRequestType = (0x02 << 5); // request type = vendor request
//   bmRequestType = bmRequestType | (direction << 7); // set the request direction

//   libusb_fill_control_setup(cOutPtr, bmRequestType, bRequest, wValue, 0 , wLength);
//   if(direction==0)
//     {
//       for(int i=0; i<wLength; i++)
// 	cOutPtr[LIBUSB_CONTROL_SETUP_SIZE+i] = data[i]; 
//     }

//   libusb_fill_control_transfer(ctlTransfer, stdHandle, cOutPtr, callback, 0, USB_TOUT_MS);
//   libusb_submit_transfer(ctlTransfer);



  
//   if(direction==1)
//     {
//       for(int i=0; i<wLength; i++)
// 	data[i] = cOutPtr[LIBUSB_CONTROL_SETUP_SIZE+i];
//     }
// }

int usbInterface::asyncTransferSetup(unsigned char* data, uint8_t direction, uint8_t EP, int length, libusb_transfer_cb_fn callback){
  int retval = 0;

  if(!direction) 
    {
      asyncWriteTransfer = libusb_alloc_transfer(0);
      if(!asyncWriteTransfer)
	std::cout << "error alocating transfer" << std::endl;
      libusb_fill_bulk_transfer(asyncWriteTransfer, stdHandle, EP, data, (int)length, callback, NULL,0);// USB_TOUT_MS);
      libusb_submit_transfer(asyncWriteTransfer); 
    }
  else if (direction)
    {  printf("createHandles: SUCCEED\n");

      asyncReadTransfer = libusb_alloc_transfer(0);
      if(!asyncReadTransfer)
	std::cout << "error alocating transfer" << std::endl;
      libusb_fill_bulk_transfer(asyncReadTransfer, stdHandle, EP, data, (int)length, callback, NULL,0);// USB_TOUT_MS);
      libusb_submit_transfer(asyncReadTransfer); 
    }

  return retval;
}


int usbInterface::asyncTransferSubmit(uint8_t direction){
  int retval = 0;

  if(!direction) 
    {
      retval = asyncWriteTransfer->actual_length;
      libusb_free_transfer(asyncWriteTransfer);
    }
  else if(direction)
    {
      retval = asyncReadTransfer->actual_length;
      libusb_free_transfer(asyncReadTransfer);
    }

  return retval;
}
