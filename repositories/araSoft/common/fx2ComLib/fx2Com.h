/* 
   FX2 Communication Library

   The library functions for sending and receiving data through the vendor commands

   rjn@hep.ucl.ac.uk, August 2011
*/

#ifndef FX2_COM_H
#define FX2_COM_H
#include "fx2Defines.h"
#include "atriComponents.h"


int openConnectionToFx2ControlSocket(); //returns file descriptor to make multi-threaded control easier
int sendControlPacketToFx2(int fFx2SockFd,Fx2ControlPacket_t *pktPtr);
int readResponsePacketFromFx2(int fFx2SockFd,Fx2ResponsePacket_t *pktPtr);
int closeConnectionToFx2ControlSocket(int fFx2SockFd);

int atriReset(int fFx2SockFd);

int getFx2Version(int fFx2SockFd, unsigned short *vp);

int enableAtriComponents(int fFx2SockFd,uint8_t enableMask);
int disableAtriComponents(int fFx2SockFd,uint8_t disableMask);

int writeToI2CRegister(int fFx2SockFd,uint16_t i2cAddress, uint8_t reg, uint8_t value);
int writeToI2C(int fFx2SockFd,uint16_t i2cAddress, uint8_t length, uint8_t *data);
int readFromI2C(int fFx2SockFd,uint16_t i2cAddress, uint8_t length, uint8_t *data);


#endif // FX2_COM_H
