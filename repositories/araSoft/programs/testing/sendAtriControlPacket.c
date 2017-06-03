/*! \file sendAtriContolPacket.c
  \brief This command line program connects to the atri_control socket, sends a packet and waits for the response
    
  This command line program connects to the atri_control socket, sends a packet and waits for the response
  
  July 2011 rjn@hep.ucl.ac.uk
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




void usage(char *argv0);
int openAtriControlSocket();



int main(int argc, char **argv)
{

  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  int i;
  int numBytes=0;
  if(argc<2 || argc>68) {
    usage(argv[0]);
    return -1;
  }

  controlPacket.header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket.header.packetLocation=strtoul(argv[1],NULL,0);
  controlPacket.header.packetLength=argc-2;
  for(i=0;i<controlPacket.header.packetLength;i++) {
    controlPacket.data[i]=strtoul(argv[2+i],NULL,0);
  }
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



void usage(char *argv0)
{

  printf("Usage:\n");
  printf("\t %s   <destination> <data 0> <data 1> <data 2>...\n",basename(argv0));
  printf("\n All values should be sent as decimals... might upgrade to hex at soem point\n");
  printf("Maximum of 64 pieces of data\n");
 
}
