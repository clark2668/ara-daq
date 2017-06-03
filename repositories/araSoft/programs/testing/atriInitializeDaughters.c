/*! \file atriGetDaughters.c
  PSA: silly testing, get daughter status and display
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


/** \brief WISHBONE read.
 *
 */
int atriWishboneRead(unsigned int fd,
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
  controlPacket->header.packetLength=4;
  controlPacket->data[0] = WB_READ;
  // 0x0010 + stack: daughter control status
  controlPacket->data[1] = (wishboneAddress & 0xFF00)>>8;
  controlPacket->data[2] = wishboneAddress & 0xFF;
  controlPacket->data[3] = length;
  controlPacket->data[controlPacket->header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if (send(fd, (char*)controlPacket, sizeof(AtriControlPacket_t), 0) == -1) {
    perror("send");
    return 0;
  }
  //Now wait for response
  if ((numBytes=recv(fd, (char*)responsePacket,sizeof(AtriControlPacket_t),
		     0)) > 0) {      
    if(responsePacket->header.packetLength == length) {
      for (i=0;i<length;i++) {
	data[i] = responsePacket->data[i];
      }
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


/** \brief Turns on an ATRI daughter.
 *
 * Turns on an ATRI daughterboard, verifies that it has turned on,
 * and reads out the first page of the EEPROM for identification.
 *
 */
int atriDaughterPowerOn(unsigned int fd,
			AtriDaughterStack_t stack,
			AtriDaughterType_t type,
			AtriDaughterStatus_t *status,
			AtriControlPacket_t *controlPacket,
			AtriControlPacket_t *responsePacket) {
  unsigned char curControl;
  unsigned char toWrite[2];
  int i;
  if (atriWishboneRead(fd, 0x0010 + stack, 1, &curControl,
		       controlPacket, responsePacket) < 0) {
    fprintf(stderr, "%s: error reading current control\n",
	    __FUNCTION__);
    return -1;
  }
  curControl |= (1 << (unsigned char) type);
  if (atriWishboneWrite(fd, 0x0010 + stack, 1, &curControl,
			controlPacket, responsePacket) < 0) {
    fprintf(stderr, "%s: error writing new status\n",
	    __FUNCTION__);
    return -1;
  }
  // Sleep 50 milliseconds to allow the hotswap cycle to complete.
  usleep(50000);
  // Does this daughter have a swap controller?
  if (atriHotSwapAddressMap[type] == 0x00) {
    // no... just return
    return 0;
  }
  // Now write 0x50 to hotswap controller to read out the status byte.
  toWrite[0] = 0x50;
  if (atriI2CWrite(fd, stack, atriHotSwapAddressMap[type], 1, toWrite,
		   controlPacket, responsePacket) < 0) {
    fprintf(stderr, "%s: error switching to status read on swap controller\n",
	    __FUNCTION__);
    return -1;
  }
  // and read the status byte
  if (atriI2CRead(fd, stack, atriHotSwapAddressMap[type], 1, &(status->hotSwapStatus),
		  controlPacket, responsePacket) < 0) {
    fprintf(stderr, "%s: error reading status on swap controller\n",
	    __FUNCTION__);
    return -1;
  }
  // and restore to a normal read
  toWrite[0] = 0x10;
  if (atriI2CWrite(fd, stack, atriHotSwapAddressMap[type], 1, toWrite,
		   controlPacket, responsePacket) < 0) {
    fprintf(stderr, "%s: error switching to normal read on swap controller\n",
	    __FUNCTION__);
    return -1;
  }
  if (status->hotSwapStatus & bmHS_OC) {
    fprintf(stderr, "%s: %s %s power on overcurrent (%2.2X)\n",
	    __FUNCTION__,
	    atriDaughterStackStrings[stack], atriDaughterTypeStrings[type],
	    status->hotSwapStatus);
    return -1;
  }
  // should read out EEPROM here for identification, but for now we return
  if (atriEepromAddressMap[type] != 0x00) {	
     toWrite[0] = 0x00;
     toWrite[1] = 0x00;
     if (atriI2CWrite(fd, stack, atriEepromAddressMap[type], 2, toWrite, controlPacket,
		      responsePacket) < 0) {
	fprintf(stderr, "%s: error setting pointer register on EEPROM\n", __FUNCTION__);
	return -1;
     }
     for (i=0;i<8;i++) {
	if (atriI2CRead(fd, stack, atriEepromAddressMap[type], 8, &(status->identPage[i*8]),
		   controlPacket, responsePacket) < 0) {
	   fprintf(stderr, "%s: error reading bytes %d-%d from EEPROM\n", __FUNCTION__,
		   i*8, i*8+7);
	   return -1;
	}
     }
  }   
  return 0;
}			

/** \brief Gets ATRI daughter status.
 * 
 * Gets ATRI daughter status. Returns 32-bit integer, with daughter 4
 * in the MSB, and daughter 1 in the LSB. Can use an existing
 * controlPacket/responsePacket or its own (and malloc/free) if pointer
 * is NULL.
 *
 * Blocks waiting for a response from atri_control socket.
 * \param fd Socket for atri_control.
 * \param controlPacket Pointer to existing AtriControlPacket_t, or NULL.
 * \param responsePacket Pointer to existing AtriControlPacket_t, or NULL.
 */
unsigned int atriGetDaughterStatus(unsigned int fd,
				   AtriControlPacket_t *controlPacket,
				   AtriControlPacket_t *responsePacket) {
  unsigned int numBytes;
  unsigned int dbStatus;
  AtriControlPacket_t *cp;
  AtriControlPacket_t *rp;

  dbStatus = 0;
  if (controlPacket == NULL) {
    cp = (AtriControlPacket_t *) malloc(sizeof(AtriControlPacket_t));
    if (!cp) fprintf(stderr, "%s : memory allocation failure\n", __FUNCTION__);
    return 0;
  } else cp = controlPacket;
  if (responsePacket == NULL) {
    rp = (AtriControlPacket_t *) malloc(sizeof(AtriControlPacket_t));
    if (!rp) fprintf(stderr, "%s : memory allocation failure\n", __FUNCTION__);
    if (!controlPacket) free(cp);
    return 0;
  } else rp = responsePacket;

  // Prep a daughterboard status request packet.
  cp->header.frameStart=ATRI_CONTROL_FRAME_START;
  cp->header.packetLocation=ATRI_LOC_DB_STATUS;
  cp->header.packetLength=1;
  cp->data[0] = 0;
  cp->data[cp->header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if (send(fd, (char*)cp, sizeof(AtriControlPacket_t), 0) == -1) {
    perror("send");
    return 0;
  }
  //Now wait for response
  if ((numBytes=recv(fd, (char*)rp,sizeof(AtriControlPacket_t),
		     0)) > 0) {      
    if(rp->header.packetLength == 4) {
      dbStatus = rp->data[0];
      dbStatus |= (rp->data[1] << 8);
      dbStatus |= (rp->data[2] << 16);
      dbStatus |= (rp->data[3] << 24);
    } else {
      fprintf(stderr, "%s : unknown packet src: %d len: %d received\n",
	      __FUNCTION__,
	      rp->header.packetLocation,
	      rp->header.packetLength);
    }
  } else {
    perror("recv");
  }
  if (controlPacket == NULL) free(cp);
  if (responsePacket == NULL) free(rp);
  return dbStatus;
}

void dumpIdentPage(const char *type, unsigned int stack, AtriDaughterStatus_t *status) 
{
   char mystr[9];
   if (!status) return;
   if (status->identPage[0] == 0xFF) 
     {
	printf("%s: stack %d: unprogrammed EEPROM\n",
	       type, stack);
	return;
     }
   // byte 0 = version
   if (status->identPage[0] != 0) 
     {
	printf("%s: stack %d: unknown page 0 version %d\n",
	       type, stack, status->identPage[0]);
	return;
     }
   // byte 1-6 = type
   // byte 7 = revision
   memcpy(mystr, &(status->identPage[1]), 6);
   mystr[6] = 0x0;
   printf("%s: stack %d: %s rev %c #", type, stack, mystr, status->identPage[7]);
   // byte 8-15 : serial number
   memcpy(mystr, &(status->identPage[8]), 8);
   mystr[8] = 0x0;
   printf("%s\n", mystr);
}


#define DDA 0
#define TDA 1
#define DRSV9 2
#define DRSV10 3
#define D1 0
#define D2 1
#define D3 2
#define D4 3
#define PRESENT(stat,type,stack) ((stat) & ((1<<(2*(type))) << (8*(stack))))
#define POWERED(stat,type,stack) ((stat) & ((1<<(2*(type)+1)) << (8*(stack))))
#define DBANY(stat,stack) ((stat) & (0xFF << (8*(stack))))
int main(int argc, char **argv)
{
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  AtriDaughterStatus_t daughterStatus[4][4];
  int i;
  int numBytes=0;
  unsigned int dbStatus;
  // Prepare 

  int fAtriControlSockFd=openAtriControlSocket();
  if(!fAtriControlSockFd) {
    fprintf(stderr, "%s : could not open atri_control socket!\n", argv[0]);
    exit(1);
  }
  dbStatus = atriGetDaughterStatus(fAtriControlSockFd,
				   &controlPacket,
				   &responsePacket);
  printf("ATRI Daughter Status:\n");
  printf("D1: DDA   present: %c powered: %c  TDA    present: %c powered %c\n",
	 PRESENT(dbStatus,DDA,D1) ? 'Y' : 'N',POWERED(dbStatus,DDA,D1) ? 'Y' : 'N',
	 PRESENT(dbStatus,TDA,D1) ? 'Y' : 'N',POWERED(dbStatus,TDA,D1) ? 'Y' : 'N');
  printf("D1: DRSV9 present: %c powered: %c  DRSV10 present: %c powered %c\n",
	 PRESENT(dbStatus,DRSV9,D1) ? 'Y' : 'N',POWERED(dbStatus,DRSV9,D1) ? 'Y' : 'N',
	 PRESENT(dbStatus,DRSV10,D1) ? 'Y' : 'N',POWERED(dbStatus,DRSV10,D1) ? 'Y' : 'N');
  
  printf("D2: DDA   present: %c powered: %c  TDA    present: %c powered %c\n",
	 PRESENT(dbStatus,DDA,D2) ? 'Y' : 'N',POWERED(dbStatus,DDA,D2) ? 'Y' : 'N',
	 PRESENT(dbStatus,TDA,D2) ? 'Y' : 'N',POWERED(dbStatus,TDA,D2) ? 'Y' : 'N');
  printf("D2: DRSV9 present: %c powered: %c  DRSV10 present: %c powered %c\n",
	 PRESENT(dbStatus,DRSV9,D2) ? 'Y' : 'N',POWERED(dbStatus,DRSV9,D2) ? 'Y' : 'N',
	 PRESENT(dbStatus,DRSV10,D2) ? 'Y' : 'N',POWERED(dbStatus,DRSV10,D2) ? 'Y' : 'N');
  
  printf("D3: DDA   present: %c powered: %c  TDA    present: %c powered %c\n",
	 PRESENT(dbStatus,DDA,D3) ? 'Y' : 'N',POWERED(dbStatus,DDA,D3) ? 'Y' : 'N',
	 PRESENT(dbStatus,TDA,D3) ? 'Y' : 'N',POWERED(dbStatus,TDA,D3) ? 'Y' : 'N');
  printf("D3: DRSV9 present: %c powered: %c  DRSV10 present: %c powered %c\n",
	 PRESENT(dbStatus,DRSV9,D3) ? 'Y' : 'N',POWERED(dbStatus,DRSV9,D3) ? 'Y' : 'N',
	 PRESENT(dbStatus,DRSV10,D3) ? 'Y' : 'N',POWERED(dbStatus,DRSV10,D3) ? 'Y' : 'N');
  
  printf("D4: DDA   present: %c powered: %c  TDA    present: %c powered %c\n",
	 PRESENT(dbStatus,DDA,D4) ? 'Y' : 'N',POWERED(dbStatus,DDA,D4) ? 'Y' : 'N',
	 PRESENT(dbStatus,TDA,D4) ? 'Y' : 'N',POWERED(dbStatus,TDA,D4) ? 'Y' : 'N');
  printf("D4: DRSV9 present: %c powered: %c  DRSV10 present: %c powered %c\n",
	 PRESENT(dbStatus,DRSV9,D4) ? 'Y' : 'N',POWERED(dbStatus,DRSV9,D4) ? 'Y' : 'N',
	 PRESENT(dbStatus,DRSV10,D4) ? 'Y' : 'N',POWERED(dbStatus,DRSV10,D4) ? 'Y' : 'N');


  // yah, these should be looped
  if (DBANY(dbStatus, D1)) {
    printf("Initializing D1 daughters...\n");
    if (PRESENT(dbStatus,DDA, D1)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D1, DDA, &daughterStatus[0][0],
			  &controlPacket, &responsePacket);
      dumpIdentPage("DDA", 1, &daughterStatus[0][0]);
    }
    if (PRESENT(dbStatus,TDA, D1)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D1, TDA, &daughterStatus[0][1],
			  &controlPacket, &responsePacket);
      dumpIdentPage("TDA", 1, &daughterStatus[0][1]);
    }
    if (PRESENT(dbStatus,DRSV9, D1)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D1, DRSV9, &daughterStatus[0][2],
			  &controlPacket, &responsePacket);
    }
    if (PRESENT(dbStatus,DRSV10, D1)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D1, TDA, &daughterStatus[0][3],
			  &controlPacket, &responsePacket);
    }
  }


  if (DBANY(dbStatus, D2)) {
    printf("Initializing D2 daughters...\n");
    if (PRESENT(dbStatus,DDA, D2)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D2, DDA, &daughterStatus[1][0],
			  &controlPacket, &responsePacket);
      dumpIdentPage("DDA", 2, &daughterStatus[1][0]);
    }
    if (PRESENT(dbStatus,TDA, D2)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D2, TDA, &daughterStatus[1][1],
			  &controlPacket, &responsePacket);
      dumpIdentPage("TDA", 2, &daughterStatus[1][1]);
    }
    if (PRESENT(dbStatus,DRSV9, D2)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D2, DRSV9, &daughterStatus[1][2],
			  &controlPacket, &responsePacket);
    }
    if (PRESENT(dbStatus,DRSV10, D2)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D2, TDA, &daughterStatus[1][3],
			  &controlPacket, &responsePacket);
    }
  }


  if (DBANY(dbStatus, D3)) {
    printf("Initializing D3 daughters...\n");
    if (PRESENT(dbStatus,DDA, D3)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D3, DDA, &daughterStatus[2][0],
			  &controlPacket, &responsePacket);
      dumpIdentPage("DDA", 3, &daughterStatus[2][0]);
    }
    if (PRESENT(dbStatus,TDA, D3)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D3, TDA, &daughterStatus[2][1],
			  &controlPacket, &responsePacket);
      dumpIdentPage("TDA", 3, &daughterStatus[2][1]);
    }
    if (PRESENT(dbStatus,DRSV9, D3)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D3, DRSV9, &daughterStatus[2][2],
			  &controlPacket, &responsePacket);
    }
    if (PRESENT(dbStatus,DRSV10, D3)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D3, TDA, &daughterStatus[2][3],
			  &controlPacket, &responsePacket);
    }
  }


  if (DBANY(dbStatus, D4)) {
    printf("Initializing D4 daughters...\n");
    if (PRESENT(dbStatus,DDA, D4)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D4, DDA, &daughterStatus[3][0],
			  &controlPacket, &responsePacket);
      dumpIdentPage("DDA", 4, &daughterStatus[3][0]);
    }
    if (PRESENT(dbStatus,TDA, D4)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D4, TDA, &daughterStatus[3][1],
			  &controlPacket, &responsePacket);
      dumpIdentPage("TDA", 4, &daughterStatus[3][1]);
    }
    if (PRESENT(dbStatus,DRSV9, D4)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D4, DRSV9, &daughterStatus[3][2],
			  &controlPacket, &responsePacket);
    }
    if (PRESENT(dbStatus,DRSV10, D4)) {
      atriDaughterPowerOn(fAtriControlSockFd,
			  D4, TDA, &daughterStatus[3][3],
			  &controlPacket, &responsePacket);
    }
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
