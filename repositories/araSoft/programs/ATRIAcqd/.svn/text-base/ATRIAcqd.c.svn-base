/*! \file Acqd.c
  \brief The Acqd program that is repsonsible for starting and stopping runs (and other similar jobs).
    
  This is the updated Acqd program which is actually a daemon.

  May 2011 rjn@hep.ucl.ac.uk
*/

#include "araSoft.h"
#include "araCom.h"
#include "araRunControlLib/araRunControlLib.h"
#include "utilLib/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg);
//int socketHandler();


//Global Variables
//Socket Handler Thread
pthread_t socket_thread;
AraProgramStatus_t programState;


int main(int argc, char *argv[])
{
  int retVal;
  char* progName = basename(argv[0]);
  programState=ARA_PROG_IDLE;


  // Check PID file
  retVal=checkPidFile(ARA_ACQD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,ARA_ACQD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)",progName,retVal);
    exit(1);
  }
  // Write pid
  if( writePidFile(ARA_ACQD_PID_FILE) )
    exit(1);


  retVal=pthread_create(&socket_thread,NULL,(void*)socketHandler,NULL);
  if(retVal) {
    error("Can't make thread");
  }
  retVal=pthread_detach(socket_thread);
  if(retVal) {
    error("Can't detach thread");
  }
  
  //Now wait.
  while(1) {
    switch(programState) {
    case ARA_PROG_IDLE:
      break; //Don't do anything just wait
    case ARA_PROG_PREPARING:
      //read config file
      //open files do this do that
      programState=ARA_PROG_PREPARED;
    case ARA_PROG_PREPARED:
      break; // Don't do anything just wait
    case ARA_PROG_RUNNING:
      // Main event loop
      break;
    case ARA_PROG_STOPPING:
      //Close files and do everything neccessary
      programState=ARA_PROG_IDLE;
      break;
    default:
      break;
    }    
    sleep(1);
    printf("programState: %d\n",programState);
  }


  retVal=pthread_cancel(socket_thread);
  if(retVal) {
    error("Can't cancel thread");
  }

  unlink(ARA_ACQD_PID_FILE);
  return 0; 
}


void error(const char *msg)
{
    perror(msg);
    exit(1);
}



int socketHandler() {
  int sockfd, newsockfd, portno,retVal;
  socklen_t clilen;
  char buffer[256];
  AraProgramCommand_t progMsg;
  AraProgramCommandResponse_t progReply;
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     //Open a socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 9009;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding");
     
     //Now listen for clients
     while(1) {
       listen(sockfd,5);
       clilen = sizeof(cli_addr);

       //Now accept a new connection
       newsockfd = accept(sockfd,
			  (struct sockaddr *) &cli_addr,
                 &clilen);
       if (newsockfd < 0)
	 error("ERROR on accept");
       bzero(buffer,256);
       
       //Read stuff from socket
       n = read(newsockfd,(char*)&progMsg,sizeof(AraProgramCommand_t));
       if (n < 0) error("ERROR reading from socket");
       
       //Check that we have the correct header and footer
       if(progMsg.feWord==0xfefefefe && progMsg.efWord==0xefefefef) {
	 switch(progMsg.controlState) {
	 case ARA_PROG_CONTROL_PREPARE:
	   //Send a reply that we got the message and are preparing
	   //wait until we are in prepared and then send confirmation
	   if(programState==ARA_PROG_IDLE) {
	     programState=ARA_PROG_PREPARING;
	     sendProgramReply(newsockfd,ARA_PROG_CONTROL_PREPARE);
	     //now wait for prepared
	     while(programState==ARA_PROG_PREPARING) {
	       usleep(1);
	     }
	     if(programState==ARA_PROG_PREPARED) {
	       sendProgramReply(newsockfd,ARA_PROG_CONTROL_PREPARE);
	     }	       
	   }
	   
	   
	   //Send reply
	   n = write(newsockfd,(char*)&progReply,sizeof(AraProgramCommandResponse_t));
	   if (n < 0) error("ERROR writing to socket");
	   
	   break;
	 case ARA_PROG_CONTROL_START:
	   //Send confirmation that we got the command
	   //depending on current state either go
	   //Idle --> Preparing --> Prepared --> Runinng
	   //or
	   // Prepared --> Runinng
	   break;
	 case ARA_PROG_CONTROL_STOP:
	   // Send confirmation that we got the command and then depending on the current state go
	   /// Running --> Stopping ---> Idle
	   /// Preparing/Prepared --> Stopping ---> Idle
	   /// Idle (do nothing)

	 }


       }
	 
      
       
       //Close client socket
       close(newsockfd);
     }
     close(sockfd);
}


void sendProgramReply(int newsockfd ,AraProgramControl_t requestedState) {
 
  progReply.abWord=0xabababab;
  progReply.baWord=0xbabababa;
  progReply.controlState=requestedState;
  progReply.currentStaus=programState;
  
}
