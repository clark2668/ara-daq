/*! \file sendFx2ContolPacket.c
  \brief This command line program connects to the fx2_control socket, sends a packet and waits for the response
    
  This command line program connects to the fx2_control socket, sends a packet and waits for the response
  
  July 2011 rjn@hep.ucl.ac.uk
*/


#include "araSoft.h"
#include "fx2Defines.h"
#include "atriControlLib/atriControl.h"
#include "fx2ComLib/fx2Com.h"



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



void usage(char *argv0);



int main(int argc, char **argv)
{
  int count=0;
  int numBytes=0;
  if(argc<5 || argc>68) {
    usage(argv[0]);
    return -1;
  }
  
  Fx2ResponsePacket_t responsePacket;
  Fx2ControlPacket_t controlPacket;
  controlPacket.bmRequestType=strtoul(argv[1],NULL,0);
  controlPacket.bRequest=strtoul(argv[2],NULL,0);
  controlPacket.wValue=strtoul(argv[3],NULL,0);
  controlPacket.wIndex=strtoul(argv[4],NULL,0);
  controlPacket.dataLength=argc-5;
  for(count=0;count<controlPacket.dataLength;count++) {
    controlPacket.data[count]=strtoul(argv[5+count],NULL,0);
  }

  int fFx2SockFd=openConnectionToFx2ControlSocket();
  if(fFx2SockFd==0) {
    fprintf(stderr,"Error opening connection to FX2_CONTROL_SOCKET\n");
    return -1;
  }
  
  
  ///Print out the packet
  printf("sending: %#x %#x %#x %#x and %d data bytes\n",controlPacket.bmRequestType,controlPacket.bRequest,controlPacket.wValue,controlPacket.wIndex,controlPacket.dataLength);
  if(controlPacket.dataLength>0) {
    printf("data: ");
    for(count=0;count<controlPacket.dataLength;count++) {
      printf("%d ",controlPacket.data[count]);
    }
    printf("\n");
  }

  int retVal=sendControlPacketToFx2(fFx2SockFd,&controlPacket);
  if(retVal<0) {
    fprintf(stderr,"Error sending control packet -- %s\n",strerror(errno));
  }
  //Should check


  if(readResponsePacketFromFx2(fFx2SockFd,&responsePacket)==0) {
    printf("Got response\n");
    if(responsePacket.status<0) {
      printf("Error: %d -- %s\n",responsePacket.status,getLibUsbErrorAsString(responsePacket.status));
    }
    else {
      printf("Transferred %d bytes\n",responsePacket.status);
      
      //	printf("received: %d %d %d %d and %d data bytes\n",responsePacket.control.bmRequestType,responsePacket.control.bRequest,responsePacket.control.wValue,responsePacket.control.wIndex,responsePacket.control.dataLength);
      printf("received: %#x %#x %#x %#x and %d data bytes\n",responsePacket.control.bmRequestType,responsePacket.control.bRequest,responsePacket.control.wValue,responsePacket.control.wIndex,responsePacket.control.dataLength);
      if(responsePacket.control.dataLength>0) {
	printf("data: ");
	for(count=0;count<responsePacket.control.dataLength;count++) {
	  printf("%d ",responsePacket.control.data[count]);
	  }
	printf("\n");
      }
    }    
  }

  closeConnectionToFx2ControlSocket(fFx2SockFd);  
  return 0;  
}

void usage(char *argv0)
{

  printf("Usage:\n");
  printf("\t %s   <bmRequestType> <bRequest> <wValue> <wIndex> <data 0> <data 1> <data 2>...\n",basename(argv0));
  printf("\n All values should be sent as decimals... might upgrade to hex at soem point\n");
  printf("Maximum of 64 pieces of data\n");
 
}
