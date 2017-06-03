#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>

#include "fx2standalone.h"

#define ATRI_FX2_VID 0x04b4
#define ATRI_FX2_PID 0x9999
#define ATRI_FX2_INTERFACE 0
#define ATRI_FX2_CONTROL_TIMEOUT 50
#define LIBUSB_DEBUG_LEVEL 3

libusb_context *ctx;
libusb_device_handle *dev;

// Response packets. The JTAG interface ignores the 'write' ack, so
// this only needs to be one deep.
Fx2ControlPacket_t *rp;

libusb_device_handle *find_fx2();

int openConnectionToFx2ControlSocket() {
  int fd;
  int ret;
  ctx = NULL;
  libusb_init(&ctx);
  if ((dev = find_fx2()) == NULL) {
    fprintf(stderr, "%s : error opening device\n", __FUNCTION__);
    return -1;
  }
  // set configuration 1
  if (libusb_set_configuration(dev, 1) < 0) {
    fprintf(stderr, "%s : could not set configuration\n", __FUNCTION__);
    libusb_close(dev);
    libusb_exit(ctx);
    return -1;
  }
  if (libusb_claim_interface(dev, ATRI_FX2_INTERFACE) < 0) {
    fprintf(stderr, "%s : could not claim interface\n", __FUNCTION__);
    libusb_close(dev);
    libusb_exit(ctx);
    return -1;
  }
  rp = (Fx2ControlPacket_t *) malloc(sizeof(Fx2ControlPacket_t));
  if (!rp) {
    fprintf(stderr, "%s : could not allocate space for temp buffer!\n",
	    __FUNCTION__);
    libusb_close(dev);
    libusb_exit(ctx);
    return -1;
  }
  // Fake a file descriptor.
  return dup(1);
}

int sendControlPacketToFx2(int fd, Fx2ControlPacket_t *cp) {
  int ret, j;
  ret = libusb_control_transfer(dev, 
			cp->bmRequestType,
			cp->bRequest,
			cp->wValue,
			cp->wIndex,
			cp->data, cp->dataLength,
			ATRI_FX2_CONTROL_TIMEOUT);
  memcpy(rp->data, cp->data, cp->dataLength);
  rp->dataLength = cp->dataLength;
  return ret;
}

int readResponsePacketFromFx2(int fd, Fx2ResponsePacket_t *resp) {
  memcpy(resp->control.data, rp->data, rp->dataLength);
  resp->control.dataLength = rp->dataLength;
  return 0;
}

void fx2standalone_exit() {
  free(rp);
  libusb_close(dev);
  libusb_exit(ctx);
}


libusb_device_handle *find_fx2()  
{ 
  dev = libusb_open_device_with_vid_pid(ctx, ATRI_FX2_VID, ATRI_FX2_PID);
  if (dev == NULL) {
    fprintf(stderr, "%s : could not open device with VID/PID %4.4x/%4.4x\n",
	    __FUNCTION__,
	    ATRI_FX2_VID,
	    ATRI_FX2_PID);
    return NULL;
  }
  return dev;
}
