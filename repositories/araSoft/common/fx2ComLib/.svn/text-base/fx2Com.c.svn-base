/* 
   FX2 Communication Library

   The library functions for sending and receiving data through the vendor commands

   rjn@hep.ucl.ac.uk, August 2011
*/

#include "fx2Com.h"
#include "araSoft.h"


//Stuff for socket
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>




int openConnectionToFx2ControlSocket()
{
  int len;
  struct sockaddr_un remote;

  int fFx2SockFd=0;
  
  if ((fFx2SockFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"socket -- %s\n",strerror(errno));
    fFx2SockFd=0;
  }
  
  ARA_LOG_MESSAGE(LOG_DEBUG,"Trying to connect...\n");
  
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, FX2_CONTROL_SOCKET);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(fFx2SockFd, (struct sockaddr *)&remote, len) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"connect --%s\n",strerror(errno));
    return 0;
  }
  
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Connected .\n",__FUNCTION__);

  struct timeval tv;
  tv.tv_sec = 10;
  tv.tv_usec=0;    
  if(setsockopt(fFx2SockFd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval)))
  {
    ARA_LOG_MESSAGE(LOG_ERR,"Error setting timeout --%s\n",strerror(errno));
    return 0;
  }
  return fFx2SockFd;
}

int closeConnectionToFx2ControlSocket(int fFx2SockFd)
{
  //Need to add some error checking
  if(fFx2SockFd)
    close(fFx2SockFd);  
  return 0;
}


int sendControlPacketToFx2(int fFx2SockFd,Fx2ControlPacket_t *pktPtr)
{
  if(fFx2SockFd==0) {
    ARA_LOG_MESSAGE(LOG_ERR,"FX2 control socket not open\n");
    return -1;
  }
  if (send(fFx2SockFd, (char*)pktPtr, sizeof(Fx2ControlPacket_t), 0) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"send --%s\n",strerror(errno));
    return -1;
  }
  return 0;
}

int readResponsePacketFromFx2(int fFx2SockFd,Fx2ResponsePacket_t *pktPtr)
{
  if(fFx2SockFd==0) {
    ARA_LOG_MESSAGE(LOG_ERR,"FX2 control socket not open\n");
    return -1;
  }
  int numBytes=recv(fFx2SockFd, (char*)pktPtr,sizeof(Fx2ResponsePacket_t), 0);
  if(numBytes<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: recv -- %s\n",__FUNCTION__,strerror(errno));
    return -1;
  }
  else if(numBytes==0) {
    return -2;
  }
  else {
    if(numBytes==sizeof(Fx2ResponsePacket_t)) {
      return 0;
    }
  }

  return -3;
}

int getFx2Version(int fFx2SockFd, unsigned short *vp) {
  Fx2ControlPacket_t cp;
  Fx2ResponsePacket_t rp;
  int ret;
  cp.bmRequestType = VR_DEVICE_TO_HOST;
  cp.bRequest = VR_VERSION;
  cp.dataLength = 2;
  sendControlPacketToFx2(fFx2SockFd, &cp);
  ret = readResponsePacketFromFx2(fFx2SockFd, &rp);
  if (ret < 0) return ret;
  // EPIPE: pre-1.0 version
  if (rp.status == -9) *vp = 0x0001;
  else {
    *vp = (rp.control.data[0] << 8) | (rp.control.data[1]);
  }
  return 0;
}

int atriReset(int fFx2SockFd) {
  Fx2ControlPacket_t cp;
  Fx2ResponsePacket_t rp;
  cp.bmRequestType = VR_HOST_TO_DEVICE;
  cp.bRequest = VR_ATRI_RESET;
  cp.dataLength = 0;
  sendControlPacketToFx2(fFx2SockFd, &cp);
  readResponsePacketFromFx2(fFx2SockFd, &rp);
  return rp.status;
}

int enableAtriComponents(int fFx2SockFd,uint8_t enableMask)
{
  Fx2ControlPacket_t controlPacket;
  Fx2ResponsePacket_t responsePacket;

  controlPacket.bmRequestType=VR_HOST_TO_DEVICE;
  controlPacket.bRequest=VR_ATRI_COMPONENT_ENABLE;
  controlPacket.wValue=enableMask | (enableMask<<8);
  controlPacket.wIndex=0;
  controlPacket.dataLength=0;
  sendControlPacketToFx2(fFx2SockFd,&controlPacket);
  readResponsePacketFromFx2(fFx2SockFd,&responsePacket);
  return responsePacket.status;

}

int disableAtriComponents(int fFx2SockFd,uint8_t disableMask)
{

  Fx2ControlPacket_t controlPacket;
  Fx2ResponsePacket_t responsePacket;

  controlPacket.bmRequestType=VR_HOST_TO_DEVICE;
  controlPacket.bRequest=VR_ATRI_COMPONENT_ENABLE;
  controlPacket.wValue=disableMask;
  controlPacket.wIndex=0;
  controlPacket.dataLength=0;
  sendControlPacketToFx2(fFx2SockFd,&controlPacket);
  readResponsePacketFromFx2(fFx2SockFd,&responsePacket);
  return responsePacket.status;


}

int writeToI2CRegister(int fFx2SockFd,uint16_t i2cAddress, uint8_t reg, uint8_t value)
{
  uint8_t data[2]={reg,value};
  return writeToI2C(fFx2SockFd,i2cAddress,2,data);
}


int writeToI2C(int fFx2SockFd,uint16_t i2cAddress, uint8_t length, uint8_t *data)
{
  int i=0;
  Fx2ControlPacket_t controlPacket;
  Fx2ResponsePacket_t responsePacket;

  controlPacket.bmRequestType=VR_HOST_TO_DEVICE;
  controlPacket.bRequest=VR_ATRI_I2C;
  controlPacket.wValue=i2cAddress;
  controlPacket.wIndex=0;
  controlPacket.dataLength=length;
  if(length>0 && length<MAX_FX2_BUFFER_SIZE) {
    for(i=0;i<length;i++) {
      controlPacket.data[i]=data[i];
    }
  }

  sendControlPacketToFx2(fFx2SockFd,&controlPacket);
  readResponsePacketFromFx2(fFx2SockFd,&responsePacket);
  return responsePacket.status;


}


int readFromI2C(int fFx2SockFd,uint16_t i2cAddress, uint8_t length, uint8_t *data)
{
  int i=0;
  Fx2ControlPacket_t controlPacket;
  Fx2ResponsePacket_t responsePacket;

  controlPacket.bmRequestType=VR_DEVICE_TO_HOST;
  controlPacket.bRequest=VR_ATRI_I2C;
  controlPacket.wValue=i2cAddress;
  controlPacket.wIndex=0;
  controlPacket.dataLength=length;
  sendControlPacketToFx2(fFx2SockFd,&controlPacket);
  readResponsePacketFromFx2(fFx2SockFd,&responsePacket);
  if(length>0 && length<MAX_FX2_BUFFER_SIZE) {
    for(i=0;i<length;i++) {
      data[i]=responsePacket.control.data[i];
    }
  }
  return responsePacket.status;

}
