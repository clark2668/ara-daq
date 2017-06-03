/*! \file startNewRun.c
  \brief The startNewRun program is the standalone program for starting a new run

    
  This is the updated startNewRun 

  May 2011 rjn@hep.ucl.ac.uk
*/

#include "araCom.h"
#include "araRunControlLib/araRunControlLib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <readline/readline.h>
#include <readline/history.h>


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  //Things to do:
  //a) read config file
  //b) do something more useful like allow the user to select an option

  AraRunControlMessageStructure_t rcMsg;
  AraRunControlMessageStructure_t rcReply;
  
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char *hostName="localhost";
  char buffer[1024];
  char *usersName;
  char *runDescription;

 
  read_history("/tmp/araRC.hist");
  printf("\n\nYou can use the up arrow key to retrieve previous input to the logger (if you don't want to retype your name, etc.\n\n");
  
  usersName=readline("Enter User's name:\n");
  if(usersName && *usersName)
    add_history(usersName);
  runDescription=readline("Enter Run Description (ends with Enter):\n");
  if(runDescription && *runDescription)
    add_history(runDescription);
  write_history("/tmp/araRC.hist");
  
  portno = 9009;
  
  //Open socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  server = gethostbyname(hostName);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
	(char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    error("ERROR connecting");
 
    sprintf(buffer,"User: %s ; Log: %s",usersName,runDescription);
    strncpy((char*)rcMsg.params,buffer,255);
    int length=strlen((char*)rcMsg.params);
    if(length>255) rcMsg.length=255;
    else rcMsg.length=length;
    


    printf("Sending message to ARAd:\n");
    rcMsg.from=ARA_LOC_RC;
    rcMsg.to=ARA_LOC_ARAD_ARA1;
    rcMsg.type=ARA_RC_START_RUN;
    rcMsg.runType=ARA_RUN_TYPE_NORMAL;
    //  rcMsg.length=0;    
    setChecksum(&rcMsg);
    
    n = write(sockfd,(char*)&rcMsg,sizeof(AraRunControlMessageStructure_t));
    if (n < 0) 
      error("ERROR writing to socket");
    
    
    n = read(sockfd,(char*)&rcReply,sizeof(AraRunControlMessageStructure_t));
    if (n < 0) 
      error("ERROR reading from socket");
    close(sockfd);
    
    
      //Do something
    if(checkRunControlMessage(&rcReply)) {    
      //Message is valid, should do something
      fprintf(stderr,"Got RC Message: %s -- %s\n",getRunControlTypeAsString(rcReply.type),getRunTypeAsString(rcReply.runType));
	if(rcReply.type==ARA_RC_RESPONSE) {
	  fprintf(stderr,"Response: %s\n",rcReply.params);
	}
    }
    else {
      fprintf(stderr,"Got malformed RC message: %s\n",getRunControlTypeAsString(rcReply.type));    
    }
    
    return 0;
}
