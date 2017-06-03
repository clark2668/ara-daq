/* 
   I2C Communication Library

   The library functions for sending and receiving data through the Diolan USB I2C converter

   jdavies@hep.ucl.ac.uk, November 2011
*/

#ifndef I2C_COM_H
#define I2C_COM_H
#include "i2cDefines.h"

#define USB_I2C_ISS_DIR "/dev/ttyACM0"

int openConnectionToFx2ControlSocket(); //returns file descriptor to make multi-threaded control easier
int sendControlPacketToFx2(int fFx2SockFd,Fx2ControlPacket_t *pktPtr);
int readResponsePacketFromFx2(int fFx2SockFd,Fx2ResponsePacket_t *pktPtr);
int closeConnectionToFx2ControlSocket(int fFx2SockFd);

int enableAtriComponents(int fFx2SockFd,uint8_t enableMask);
int disableAtriComponents(int fFx2SockFd,uint8_t disableMask);

int writeToI2CRegister(int fFx2SockFd,uint16_t i2cAddress, uint8_t reg, uint8_t value);
int writeToI2C(int fFx2SockFd,uint16_t i2cAddress, uint8_t length, uint8_t *data);
int readFromI2C(int fFx2SockFd,uint16_t i2cAddress, uint8_t length, uint8_t *data);


#endif // FX2_COM_H
