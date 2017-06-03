#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


#ifndef STANDALONE
#include "araSoft.h"
#include "fx2ComLib/fx2Com.h"
#else
#include "fx2standalone.h"
#endif

Fx2ControlPacket_t cp;
Fx2ControlPacket_t cp2;
Fx2ResponsePacket_t rp;

#define VR_JTAG 0xb9

int fx2JtagConnect() 
{
   int fd;
   fd = openConnectionToFx2ControlSocket();
   if (fd < 0) {
     printf("Error connecting!\n");
     return -1;
   }
   printf("connected\n");
   return fd;
}


int fx2JtagOpen(int fd) 
{
   int retVal;
   cp.bmRequestType = 0x40;
   cp.bRequest = VR_JTAG;
   cp.wValue = 2;
   cp.wIndex = 0;
   cp.dataLength = 0;
   retVal=sendControlPacketToFx2(fd,&cp);
   if (retVal < 0) 
     {
	printf("send control packet returned %d\n", retVal);
	return retVal;
     }
   retVal=readResponsePacketFromFx2(fd, &rp);
   if (retVal < 0)
     {
	printf("read response packet returned %d\n", retVal);
	return retVal;
     }
   printf("initialized\n");
   return 0;
}

int fx2JtagClose(int fd) 
{
   int retVal;
   cp.bmRequestType = 0x40;
   cp.bRequest = VR_JTAG;
   cp.wValue = 1;
   cp.wIndex = 0;
   cp.dataLength = 0;

   retVal=sendControlPacketToFx2(fd, &cp);
   if (retVal < 0) 
     {
	printf("send control packet returned %d\n", retVal);
	return retVal;
     }
   retVal = readResponsePacketFromFx2(fd, &rp);
   if (retVal < 0)
     {
	printf("read response packet returned %d\n", retVal);
	return retVal;
     }   
   return 0;
}

int fx2JtagExit(int fd)
{
#ifdef STANDALONE
  fx2standalone_exit();
#endif
  return close(fd);
}


   
int fx2JtagSend(int fd, unsigned char *buf, unsigned int nbits) 
{
   unsigned int nbytes = (nbits/4) + (nbits%4 ? 1 : 0);
   int retVal;
   int i;
   
   cp.bmRequestType = 0x40;
   cp.bRequest = VR_JTAG;
   cp.wValue = 0;
   cp.wIndex = (nbits%4 ? nbits%4 : 4);
   cp.dataLength = nbytes;
   memcpy(cp.data, buf, nbytes);

   retVal=sendControlPacketToFx2(fd, &cp);
   if (retVal < 0 ) 
     {
	printf("err %d sending control packet\n", retVal);
	return retVal;
     }
   if (nbytes & 0x1)
     nbytes = (nbytes/2) + 1;
   else
     nbytes = nbytes/2;

   cp2.bmRequestType = 0xC0;
   cp2.bRequest = VR_JTAG;
   cp2.wValue = 0;
   cp2.wIndex = 0;
   cp2.dataLength = nbytes;
   retVal = sendControlPacketToFx2(fd, &cp2);
   
   retVal=readResponsePacketFromFx2(fd, &rp);
   if (retVal < 0) 
     {
	printf("err %d getting ack on sent data\n", retVal);
     }
   retVal=readResponsePacketFromFx2(fd, &rp);
   if (retVal < 0)
     {
	printf("err %d getting JTAG data\n", retVal);
     }   
   memcpy(buf, rp.control.data, rp.control.dataLength);
   return rp.control.dataLength;
}

   
