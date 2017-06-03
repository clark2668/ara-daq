/*! \file atriGetDaughters.c
  PSA: silly testing, get daughter status and display
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

#define DDA 0
#define TDA 1
#define DRSV9 2
#define DRSV10 3
#define D1 0
#define D2 1
#define D3 2
#define D4 3
#define PRESENT(x,y,z) ((x) & ((1<<(2*(y))) << (8*(z))))
#define POWERED(x,y,z) ((x) & ((1<<(2*(y)+1)) << (8*(z))))
int main(int argc, char **argv)
{
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
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
  printf("ATRI Daughter Status: (%x)\n", dbStatus);
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
