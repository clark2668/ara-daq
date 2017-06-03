/*! \file atriReadWilkinsonSpeed.c
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

int main(int argc, char **argv)
{

  unsigned  char data[100];
  int fAtriControlSockFd=openAtriControlSocket();
  if(fAtriControlSockFd){
    atriWishboneRead(fAtriControlSockFd,ATRI_WISH_D1WILK, 16, data);
    int dda=0;
    for(dda=0;dda<4;dda++){
      uint16_t *temp=(uint16_t*)&data[dda*4];

      unsigned int wilkCounterNs = atriConvertWilkCounterToNs(*temp);
      printf("stack %i Wilk period %d (ns) Wilk Freq %f (MHz) TST Freq %f (kHz)\n", dda, wilkCounterNs, (wilkCounterNs*660.645)/6200, (wilkCounterNs*660.645) / (6.2*4096));
    }    

    close(fAtriControlSockFd);
  
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
