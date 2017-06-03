/*! \file atriSetReadoutDelay.c
  October 2012
*/


#include "araSoft.h"
#include "atriDefines.h"
#include "atriComLib/atriCom.h"
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
int setAtriReadoutDelay(int fAtriControlSockFd, uint8_t delay);

int main(int argc, char **argv)
{

  if(argc<2){
    printf("usage : %s <delay int>\n", argv[0]);
    return -1;
  }
  uint8_t val, trigctlSave;
  uint16_t addr;
  uint8_t delay = atoi(argv[1]);
  unsigned  char data[100];
  int fAtriControlSockFd=openAtriControlSocket();
  if(fAtriControlSockFd){

    addr = ATRI_WISH_IRSEN+2;
    val=0;
    if (atriWishboneRead(fAtriControlSockFd, addr, 1, &val)<0) return -1;
    printf("current IRS readout Delay %i\n", val);


    printf("Setting IRS Readout Delay to %i (%i clocks)\n", delay, 4*delay+8);
    setAtriReadoutDelay(fAtriControlSockFd, delay);

    addr = ATRI_WISH_IRSEN+2;
    val=0;
    if (atriWishboneRead(fAtriControlSockFd, addr, 1, &val)<0) return -1;
    printf("current IRS readout Delay %i\n", val);




  }//fAtriControlSocketFd
  else
    {
      fprintf(stderr, "%s : Unable to open connection to AtriControlSocket\n", __FUNCTION__);
      return -1;
      
      
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
int setAtriReadoutDelay(int fAtriControlSockFd, uint8_t delay){
  uint8_t val, trigctlSave;
  uint16_t addr;

  /* addr = ATRI_WISH_IRSEN; */
  /* // Disable digitizer and place in reset. */
  /* val =0; */
  /* val = (0x01<<1)+(0x01<<5); */
  /* //  printf("setting IRSEN to %i\n", val); */
  /* if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1; */
  /* // Check to see if the digitizer subsystem is done resetting. */
  /* addr = ATRI_WISH_IRSEN; */
  /* if (atriWishboneRead(fAtriControlSockFd, addr, 1, &val)<0) return -1; */
  /* if (!(val & irsctlResetAckMask)) { */
  /*   ARA_LOG_MESSAGE(LOG_ERR, "%s: error resetting digitizer: RESET_ACK did not go high (%2.2x)\n", __FUNCTION__, val); */
  /*   return -1; */
  /* } */

  /* addr = ATRI_WISH_IRSEN; */
  /* val = 0; */
  /* // Pull the digitizing subsystem out of reset. */
  /* if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1; */


  //Now we set the readout delay
  addr = ATRI_WISH_IRSEN+2;
  val = delay;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;

  /* //Finally enable the digitiser */
  /* val = irsctlEnableMask; */
  /* addr = ATRI_WISH_IRSEN; */
  /* if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1; */


  // And we're done. Note that the digitizer is not enabled, nor
  // triggering!
  return 0;
}
