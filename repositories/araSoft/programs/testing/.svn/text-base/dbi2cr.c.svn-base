/*! \file dbi2cr.c
  PSA: silly testing, read from DBI2C (daughterboard I2C bus)
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


/** \brief I2C read.
 *
 */
int atriI2CRead(unsigned int fd,
		AtriDaughterStack_t stack,
		 unsigned char address,
		 unsigned int length,
		 unsigned char *data,
		 AtriControlPacket_t *controlPacket,
		 AtriControlPacket_t *responsePacket) {
  unsigned int numBytes;
  // Set the R/~W bit.
  address |= 0x1;
  
  controlPacket->header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket->header.packetLocation=ATRI_LOC_I2C_DB1 + stack;
  controlPacket->header.packetLength=3;
  controlPacket->data[0] = I2C_DIRECT;
  controlPacket->data[1] = address;
  controlPacket->data[2] = length;
  controlPacket->data[controlPacket->header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if (send(fd, (char*)controlPacket, sizeof(AtriControlPacket_t), 0) == -1) {
    perror("send");
    return 0;
  }
  //Now wait for response
  if ((numBytes=recv(fd, (char*)responsePacket,sizeof(AtriControlPacket_t),
		     0)) > 0) {      
    if(responsePacket->header.packetLength == length+2) {
      memcpy(data, responsePacket->data+2, length);
      return 0;
    } else {
      if (responsePacket->header.packetLocation == controlPacket->header.packetLocation && responsePacket->header.packetLength == 1) {
	if (responsePacket->data[0] == 0xFF) {
	  // NACK
	  fprintf(stderr, "%s : NACK on I2C read of address %2.2x\n",
		  __FUNCTION__, address);
	  return -1;
	} else {
	  fprintf(stderr, "%s : unknown packet src: %d len: %d received\n",
		  __FUNCTION__,
		  responsePacket->header.packetLocation,
		  responsePacket->header.packetLength);
	  return -1;
	}
      }
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
  int i;
  unsigned int stack;
  unsigned int addr;
  unsigned char i2c_addr;
  unsigned char *data;
  unsigned int length;
  // Prepare 

  int fAtriControlSockFd=openAtriControlSocket();
  if(!fAtriControlSockFd) {
    fprintf(stderr, "%s : could not open atri_control socket!\n", argv[0]);
    exit(1);
  }

  argc--;
  argv++;
  if (argc != 3) {
    fprintf(stderr,"dbi2cr: reads bytes from DBI2C bus\n");
    fprintf(stderr,"dbi2cr: syntax - dbi2c stack address nbytes.\n");
    exit(1);
  }
  stack = strtoul(*argv, NULL, 0);
  if (stack > 4 || stack == 0) {
    fprintf(stderr,"dbi2cr: stack must be 1, 2, 3, or 4 (got %d)\n",
	    stack);
    exit(1);
  }
  // stack starts at 1 outside here, but is 0-3 inside software
  stack--;
  argv++;
  addr = strtoul(*argv, NULL, 0);
  if (addr > 0xFF) {
    fprintf(stderr,"dbi2cr: address is 8-bits, must be 0xFF or less (got %8.8x)\n", addr);
    exit(1);
  }
  i2c_addr = addr;
  if (!(i2c_addr & 0x01)) {
    fprintf(stderr,"dbi2cr: read addresses must have low bit set, forcing (was %2.2x now %2.2x)\n", i2c_addr, i2c_addr | 0x1);
  }
  i2c_addr = i2c_addr | 0x1;
  argv++;
  length = strtoul(*argv, NULL, 0);
  if (length > 32) {
    fprintf(stderr,"dbi2cr: max read is 32 bytes (got %8.8x)\n", length);
    exit(1);
  }
  data = malloc(sizeof(unsigned char)*length);
  if (atriI2CRead(fAtriControlSockFd,
		  stack,
		  i2c_addr,
		  length,
		  data,
		  &controlPacket,
		  &responsePacket) < 0) {
    fprintf(stderr, "dbi2cr: error reading from I2C bus\n");
    exit(1);
  }
  for (i=0;i<length;i++) {
    printf("%2.2x", data[i]);
    if ((!((i+1) % 16)) || (i+1 == length)) printf("\n");
    else printf(" ");
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
