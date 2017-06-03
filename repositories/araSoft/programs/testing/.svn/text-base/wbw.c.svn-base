/*! \file wbw.c
  PSA: silly testing, do wishbone write to a given address
*/


#include "araSoft.h"
#include "atriDefines.h"
#include "atriUsefulDefines.h"
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

/** \brief WISHBONE write.
 *
 */
int atriWishboneWrite(unsigned int fd,
		     unsigned short wishboneAddress,
		     unsigned int length,
		     unsigned char *data,
		     AtriControlPacket_t *controlPacket,
		     AtriControlPacket_t *responsePacket) {
  unsigned int numBytes;
  int i;
  // Read the current daughter status.
  controlPacket->header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket->header.packetLocation=ATRI_LOC_WISHBONE;
  controlPacket->header.packetLength=3+length;
  controlPacket->data[0] = WB_WRITE;
  // 0x0010 + stack: daughter control status
  controlPacket->data[1] = (wishboneAddress & 0xFF00)>>8;
  controlPacket->data[2] = wishboneAddress & 0xFF;
  memcpy(controlPacket->data+3, data, sizeof(unsigned char)*length);
  controlPacket->data[controlPacket->header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if (send(fd, (char*)controlPacket, sizeof(AtriControlPacket_t), 0) == -1) {
    perror("send");
    return 0;
  }
  //Now wait for response
  if ((numBytes=recv(fd, (char*)responsePacket,sizeof(AtriControlPacket_t),
		     0)) > 0) {      
    if(responsePacket->header.packetLength == 1) {
      return 0;
    } else {
      fprintf(stderr, "%s : unknown packet src: %d len: %d received\n",
	      __FUNCTION__,
	      responsePacket->header.packetLocation,
	      responsePacket->header.packetLength);
      return -1;
    }
  } else {
    perror("recv");
    return -1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  AtriDaughterStatus_t daughterStatus[4][4];
  int i;
  int numBytes=0;
  unsigned int addr;
  unsigned short wb_addr;
  unsigned int data;
  unsigned char wb_data;
  // Prepare 

  int fAtriControlSockFd=openAtriControlSocket();
  if(!fAtriControlSockFd) {
    fprintf(stderr, "%s : could not open atri_control socket!\n", argv[0]);
    exit(1);
  }
  argc--;
  argv++;
  if (argc != 2) {
    fprintf(stderr,"wbw: writes 1 byte to WISHBONE bus\n");
    fprintf(stderr,"wbw: syntax - wbw address data.\n");
    exit(1);
  }
  addr = strtoul(*argv, NULL, 0);
  if (addr > 0xFFFF) {
    fprintf(stderr,"wbw: address is 16-bits, must be 0xFFFF or less (got %8.8x)\n", addr);
    exit(1);
  }
  wb_addr = addr;
  argv++;
  data = strtoul(*argv, NULL, 0);
  if (data > 0xFF) {
    fprintf(stderr,"wbw: data is 8-bits, must be 0xFF or less (got %8.8x)\n",
	    data);
    exit(1);
  }
  wb_data = data;
  if (atriWishboneWrite(fAtriControlSockFd,
			wb_addr,
			1,
			&wb_data,
			&controlPacket,
			&responsePacket) < 0) {
    fprintf(stderr,"error writing to wishbone bus\n");
    exit(1);
  }
  close(fAtriControlSockFd);
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
  
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, ATRI_CONTROL_SOCKET);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(fAtriControlSockFd, (struct sockaddr *)&remote, len) == -1) {
    perror("connect");
    exit(1);
  }
  
  return fAtriControlSockFd;
  


}
