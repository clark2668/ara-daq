/*! \file atriReadThresholdScalars.c
  JD: Program to read the threshold scalars from the FPGA wishbone bus
      August 2011
*/


#include "araSoft.h"
#include "atriDefines.h"
#include <unistd.h>
#include <libgen.h>


//Stuff for socket
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


int openAtriControlSocket();

int main(int argc, char **argv)
{

  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  int i;
  int numBytes=0;

                                                            /* -- read 8 bytes from 0x0100 of the wishbone                      */
  controlPacket.header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket.header.packetLocation= ATRI_LOC_WISHBONE;   /* read from wishbone                                               */
  controlPacket.header.packetLength=4;                      /* packet length is the number of data words, in this case 4        */
  controlPacket.data[0]=0;                                  /* 0x00 - read / 0x01 - write                                       */
  controlPacket.data[1]=0x01;                               /* 0x10 - high order byte of address for scaler registers at 0x0100 */
  controlPacket.data[2]=0x00;                               /* 0x00 - low order byte of address for scaler registers at 0x0100  */
  controlPacket.data[3]=32;                                 /*   32 - read 32 bytes, i.e., all TDA card scalers                 */

  controlPacket.data[controlPacket.header.packetLength]=ATRI_CONTROL_FRAME_END;


  int fAtriControlSockFd=openAtriControlSocket();
  if(fAtriControlSockFd) {
    //some debugging print out
    printf("sending %d bytes to %#x\n",controlPacket.header.packetLength,controlPacket.header.packetLocation);
    if(controlPacket.header.packetLength) {
      printf("data: ");
      for(i=0;i<controlPacket.header.packetLength;i++) {
	printf("%#x ",controlPacket.data[i]);
      }
      printf("\n");      
    }
    

    //Now send the packet
    if (send(fAtriControlSockFd, (char*)&controlPacket, sizeof(AtriControlPacket_t), 0) == -1) {
      perror("send");
      exit(1);
    }

    //Now wait for response
    if ((numBytes=recv(fAtriControlSockFd, (char*)&responsePacket,sizeof(AtriControlPacket_t), 0)) > 0) {      
      printf("Got response\n");
      printf("received %d bytes from %#x\n",responsePacket.header.packetLength,responsePacket.header.packetLocation);
      if(responsePacket.header.packetLength) {
	printf("data: ");
	for(i=0;i<responsePacket.header.packetLength;i++) {
	  printf("%#x ",responsePacket.data[i]);
	}
	printf("\n");      
      }
    }     
    close(fAtriControlSockFd);
  }
  
  return 0;  


}

int openAtriControlSocket()
{
  int fAtriControlSockFd;
  int len;
  struct sockaddr_un remote;

  
  if ((fAtriControlSockFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  
  printf("Trying to connect...\n");
  
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, ATRI_CONTROL_SOCKET);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(fAtriControlSockFd, (struct sockaddr *)&remote, len) == -1) {
    perror("connect");
    exit(1);
  }
  
  printf("Connected.\n");
  return fAtriControlSockFd;
  


}

