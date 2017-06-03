#ifndef FX2STANDALONE_H
#define FX2STANDALONE_H

#include <stdio.h>

#define MAX_FX2_BUFFER_SIZE 56

enum {
  VR_ATRI_I2C = 0xb4,
  VR_LED_DEBUG = 0xb5,
  VR_ATRI_COMPONENT_ENABLE=0xb6
};
typedef uint8_t Fx2VendorRequest_t;

enum {
  VR_HOST_TO_DEVICE = 0x40,
  VR_OUT = 0x40,
  VR_DEVICE_TO_HOST = 0xc0,
  VR_IN = 0xc0
};
typedef uint8_t Fx2VendorRequestDirection_t;

typedef struct {
  Fx2VendorRequestDirection_t bmRequestType;
  Fx2VendorRequest_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t dataLength;
  uint8_t data[MAX_FX2_BUFFER_SIZE];
} Fx2ControlPacket_t; ///<The Header associated with a packet

typedef struct {
  int16_t status;
  Fx2ControlPacket_t control;
} Fx2ResponsePacket_t; ///< The repsonse format

int openConnectionToFx2ControlSocket();
int sendControlPacketToFx2(int fd, Fx2ControlPacket_t *cp);
int readResponsePacketFromFx2(int fd, Fx2ResponsePacket_t *resp);
void fx2standalone_exit();

#endif
