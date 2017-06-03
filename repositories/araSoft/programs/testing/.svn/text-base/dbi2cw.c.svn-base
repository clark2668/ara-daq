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


/** \brief I2C write.
 *
 */
int atriI2CWrite(unsigned int fd,
		 AtriDaughterStack_t stack,
		 unsigned char address,
		 unsigned int length,
		 unsigned char *data,
		 AtriControlPacket_t *controlPacket,
		 AtriControlPacket_t *responsePacket) {
  unsigned int numBytes;
  int i;
  // Clear the R/~W bit just to be sure. Trying to write to
  // a read address results in a timeout.
  address &= ~(0x1);
  
  controlPacket->header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket->header.packetLocation=ATRI_LOC_I2C_DB1 + stack;
  controlPacket->header.packetLength=2+length;
  controlPacket->data[0] = I2C_DIRECT;
  controlPacket->data[1] = address;
  memcpy(controlPacket->data+2, data, length);
  controlPacket->data[controlPacket->header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if (send(fd, (char*)controlPacket, sizeof(AtriControlPacket_t), 0) == -1) {
    perror("send");
    return 0;
  }
  //Now wait for response
  if ((numBytes=recv(fd, (char*)responsePacket,sizeof(AtriControlPacket_t),
		     0)) > 0) {      
    if(responsePacket->header.packetLength == 3) {
      return 0;
    } else {
      if (responsePacket->header.packetLocation == controlPacket->header.packetLocation && responsePacket->header.packetLength == 1) {
	if (responsePacket->data[0] == 0xFF) {
	  // NACK
	  fprintf(stderr, "%s : NACK on I2C write to address %2.2x\n",
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
  unsigned int addr;
  unsigned char i2c_addr;
  unsigned int data;
  unsigned char *i2c_data;
  unsigned char *p;
  unsigned char len;
  unsigned int stack;
  // Prepare 

  int fAtriControlSockFd=openAtriControlSocket();
  if(!fAtriControlSockFd) {
    fprintf(stderr, "%s : could not open atri_control socket!\n", argv[0]);
    exit(1);
  }
  argc--;
  argv++;
  // argc=num arguments
  if (argc < 3) {
    fprintf(stderr,"dbi2cw: writes bytes to DBI2C bus\n");
    fprintf(stderr,"dbi2cw: syntax - dbi2cw stack address data (data data...).\n");
    exit(1);
  }
  // argv is stack, argc=num bytes + 2
  stack = strtoul(*argv, NULL, 0);
  if (stack > 4 || stack == 0) {
    fprintf(stderr,"dbi2cr: stack must be 1, 2, 3, or 4 (got %d)\n",
	    stack);
    exit(1);
  }
  // stack starts at 1 outside here, but is 0-3 inside software
  stack--;

  // move to argv=address, argc=num bytes + 1
  argc--;
  argv++;
  addr = strtoul(*argv, NULL, 0);
  if (addr > 0xFF) {
    fprintf(stderr,"dbi2cw: address is 8-bits, must be 0xFF or less (got %8.8x)\n", addr);
    exit(1);
  }
  i2c_addr = addr;
  if (i2c_addr & 0x1) {
    fprintf(stderr,"dbi2cw: address low bit must be 0, forcing (was %2.2x now %2.2x)\n", i2c_addr, i2c_addr & 0xFE);
    i2c_addr = i2c_addr & 0xFE;
  }
  
  // move to argv=first data, argc=num bytes
  argc--;
  argv++;

  if (argc > 32) {
    printf("dbi2cw: can write maximum of 32 bytes (saw %d)\n",
	   argc);
    exit(1);
  }
  len = argc;
  i2c_data = malloc(sizeof(unsigned char)*argc);
  if (!i2c_data) {
    fprintf(stderr, "error allocating %d bytes\n", argc);
    exit(1);
  }
  p = i2c_data;
  while (argc) {
    data = strtoul(*argv, NULL, 0);
    if (data > 0xFF) {
      fprintf(stderr,"dbi2cw: data is 8-bits, must be 0xFF or less (got %8.8x)\n",
	      data);
      free(i2c_data);
      exit(1);
    }
    *p = (unsigned char) data;
    p++;    
    argc--;
    argv++;
  }
  if (atriI2CWrite(fAtriControlSockFd,
		   stack,
		   i2c_addr,
		   len,
		   i2c_data,
		   &controlPacket,
		   &responsePacket) < 0) {
    fprintf(stderr,"error writing to I2C bus\n");
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
