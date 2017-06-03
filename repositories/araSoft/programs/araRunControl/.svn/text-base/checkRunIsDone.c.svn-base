

/*! \file checkRunIsDone.c
  \brief Polls ARAd until the status of ARAAcqd is IDLE (i.e. the run is finished)
    
  June 2013 jdavies@hep.ucl.ac.uk
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

int poll(int debug);

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  int retVal=0;
  int debug=0;
  while(1){
    retVal=poll(debug);
    if(retVal <= 0) break;
    if(retVal == 1){
      fprintf(stdout, "Run completed\n");
      break;
    }
  }
}


int poll(int debug){

  AraRunControlMessageStructure_t rcMsg;
  AraRunControlMessageStructure_t rcReply;
  
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char *hostName="localhost";
    char buffer[256];
    portno = 9009;

    //Open socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(hostName);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
	//        exit(0);
	return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
      {
        error("ERROR connecting");


      }
    if(debug!=0) printf("Sending message to ARAd:\n");
    rcMsg.from=ARA_LOC_RC;
    rcMsg.to=ARA_LOC_ARAD_ARA1;
    rcMsg.type=ARA_RC_QUERY_STATUS;
    rcMsg.length=0;    
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
      if(debug!=0) fprintf(stderr,"Got RC Message: %s\n",getRunControlTypeAsString(rcReply.type));
      if(rcReply.type==ARA_RC_RESPONSE) {
	if(debug!=0) fprintf(stderr,"Response: %s\n",rcReply.params);
	int acqdStatus = rcReply.params[43] -'0';
	return acqdStatus;
      }
    }
    else {
      fprintf(stderr,"Got malformed RC message: %s\n",getRunControlTypeAsString(rcReply.type));    
    }
    
    return -1;
}
