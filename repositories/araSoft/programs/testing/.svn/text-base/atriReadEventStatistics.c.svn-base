/*! \file atriReadEventStatistics.c
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
  AtriEventReadoutStatistics_t stat;
  unsigned int ppsNum;
  int retval;

  int fAtriControlSockFd=openAtriControlSocket();
  if(fAtriControlSockFd){
    atriWishboneRead(fAtriControlSockFd,ATRI_WISH_PPSCNT, 4, &ppsNum);    
    retval = atriGetEventReadoutStatistics(fAtriControlSockFd,
				  &stat);
    printf("ret %d\n", retval);
    printf("PPS %010.10d: event error %03.3d\n", ppsNum, stat.error);
    printf("PPS %010.10d: event buffer: average %03.1f%% empty minimum space: %05.5d words\n",
	   ppsNum, (stat.bufspace_avg)/640., stat.bufspace_min);
    printf("PPS %010.10d: block buffer: average %03.0f/256 max: %03.3d\n",
	   ppsNum,
	   ((stat.blocks_avg)*512. + 256.)/1000, stat.blocks_min);
    printf("PPS %010.10d: deadtime: irs: %06.2f%% usb: %06.2f%% total: %6.2f%%\n",
	   ppsNum, 
	   (((stat.irs_deadtime)*16.)/1.E6)*100.,
	   (((stat.usb_deadtime)*16.)/1.E6)*100.,
	   (((stat.tot_deadtime)*16.)/1.E6)*100.);
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
