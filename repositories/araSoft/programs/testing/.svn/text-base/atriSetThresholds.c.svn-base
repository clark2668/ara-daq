/*! \file atriSetVdly.c
  PSA: silly testing, set TDA thresholds
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
  int i;
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

int atriSetThresholds(unsigned int fd,
		      AtriDaughterStack_t stack,
		      unsigned short *thresholds,
		      AtriControlPacket_t *controlPacket,
		      AtriControlPacket_t *responsePacket) {
  unsigned char i2cDacWrite[12];

  // DAC command byte
  i2cDacWrite[0] = tdaThresh0 | dacWriteOnly;
  i2cDacWrite[1] = (thresholds[0] & 0xFF00)>>8;
  i2cDacWrite[2] = (thresholds[0] & 0x00FF);
  i2cDacWrite[3] = tdaThresh1 | dacWriteOnly;
  i2cDacWrite[4] = (thresholds[1] & 0xFF00)>>8;
  i2cDacWrite[5] = (thresholds[1] & 0x00FF);
  i2cDacWrite[6] = tdaThresh2 | dacWriteOnly;
  i2cDacWrite[7] = (thresholds[2] & 0xFF00)>>8;
  i2cDacWrite[8] = (thresholds[2] & 0x00FF);
  i2cDacWrite[9] = tdaThresh3 | dacWriteUpdateAll;
  i2cDacWrite[10] = (thresholds[3] & 0xFF00)>>8;
  i2cDacWrite[11] = (thresholds[3] & 0x00FF);
  if (atriI2CWrite(fd, stack, tdaDacAddress, 12, i2cDacWrite,
		   controlPacket, responsePacket) < 0) {
    fprintf(stderr, "%s: error writing to TDA thresholds\n",
	    __FUNCTION__);
    return -1;
  }
  return 0;
}			

int main(int argc, char **argv)
{
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  unsigned char stack_int;
  AtriDaughterStack_t stack;
  unsigned short thresholds[4];
  int i;
  int numBytes=0;
  // Prepare 
  if (argc != 6) {
    fprintf(stderr, "atriSetThresholds: need 5 arguments: daughter stack number, and threshold values\n");
    exit(1);
  }
  stack_int = strtoul(argv[1],0, NULL);
  stack = stack_int - 1;
  if (!stack_int || stack > D4) {
    fprintf(stderr, "atriSetThresholds: stack number out of bounds (1 to 4)\n");
    exit(1);
  }
  for (i=0;i<4;i++) {
    thresholds[i] = strtoul(argv[i+2], 0, NULL);
  }
  int fAtriControlSockFd=openAtriControlSocket();
  if(!fAtriControlSockFd) {
    fprintf(stderr, "%s : could not open atri_control socket!\n", argv[0]);
    exit(1);
  }
  atriSetThresholds(fAtriControlSockFd, stack, thresholds, &controlPacket, &responsePacket);
  printf("%s thresh0: %f (%4.4x)\n", atriDaughterStackStrings[stack],
	 dacToVolts(thresholds[0]), thresholds[0]);
  printf("%s thresh1: %f (%4.4x)\n", atriDaughterStackStrings[stack],
	 dacToVolts(thresholds[1]), thresholds[1]);
  printf("%s thresh2: %f (%4.4x)\n", atriDaughterStackStrings[stack],
	 dacToVolts(thresholds[2]), thresholds[2]);
  printf("%s thresh3: %f (%4.4x)\n", atriDaughterStackStrings[stack],
	 dacToVolts(thresholds[3]), thresholds[3]);

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
