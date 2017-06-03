/*! \file ARAAcqd.c
  \brief The ARAAcqd program is responsible for interfacing with ATRI board.
    
  This is the ARAAcqd program which is responsible for interfacing with ATRI board via the Cypress FX2 microcontroller. The program has a number of threads with separate responsibilities. These include (at least at the moment):
  i) A thread that actually communicates with the FX2 over the control end points
  ii) A thread which monitors the unix domain atri_contol and adds requests to the pending queue
  iii) A thread which reads out the event data
  iv) The run control thread which monitors a seperate unix domain socket for sending and receiving run control packets
a  v) A thread which reads out the housekeeping data

  July 2011 rjn@hep.ucl.ac.uk
*/
#include "araSoft.h"
#include "ARAAcqd.h"
#include "utilLib/util.h"
#include "atriControlLib/atriControl.h"
#include "fx2ComLib/fx2Com.h"
#include "atriComLib/atriCom.h"
#include "kvpLib/keyValuePair.h"
#include "configLib/configLib.h"
#include "araOneStructures.h"


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <libgen.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <poll.h>


//Global Variables
//#define NO_USB 0


#define MAX_SOFT_TRIG_RATE 50
#define MAX_RAND_TRIG_RATE MAX_SOFT_TRIG_RATE


// Writer objects are global, so they can be accessed by signal handler
ARAWriterStruct_t eventHkWriter;
ARAWriterStruct_t sensorHkWriter;
ARAWriterStruct_t eventWriter;


//Threads
pthread_t fRunControlSocketThread;
pthread_t fAtriControlSocketThread;
pthread_t fFx2ControlUsbThread;
pthread_t fAraHkThread;

//Event loop variables
int fHkThreadPrepared=0;
int fHkThreadStopped=0;
AraProgramStatus_t fProgramState;
int32_t fCurrentRun;
int32_t fCurrentEvent;

//Config is global so other threads can read it
ARAAcqdConfig_t theConfig;


FILE *fpAtriEventLog=NULL;
FILE *fpDumpFile=NULL;

#define CONDITION_MET( yes, nt, tt ) ( yes && ( nt.tv_sec > tt.tv_sec || ( nt.tv_sec == tt.tv_sec && nt.tv_usec >= tt.tv_usec ) ) )


int main(int argc, char *argv[])
{
  char filename[FILENAME_MAX];

  printToScreen=0;
  int retVal;
  int numBytesRead;
  int fMainThreadAtriSockFd;
  int fMainThreadFx2SockFd;
  float randScale;
  //  char* configFileName="/Users/rjn/ara/repositories/araSoft/trunk/config/ARAAcqd.config";
  char* configFileName="ARAAcqd.config";
  void *status;
  char* progName = basename(argv[0]);
  fProgramState=ARA_PROG_IDLE;
  AraSimpleStationEvent_t theEvent;


    

  // Time values
  struct timeval nowTime;
  struct timeval nextSoftTrig,nextRandTrig;//,nextHkRead,nextServoCalc;
  struct timeval nextSoftStep,nextRandStep;//,nextHkStep,nextServoStep;


  // Check PID file
  retVal=checkPidFile(ARA_ACQD_PID_FILE);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,ARA_ACQD_PID_FILE);
    exit(1);
  }
  // Write pid
  if( writePidFile(ARA_ACQD_PID_FILE) )
    exit(1);
  
  /* Set signal handlers */
  signal(SIGUSR2, sigUsr2Handler);
  signal(SIGTERM, sigUsr2Handler);
  signal(SIGINT, sigUsr2Handler);


  // Load configuration
  retVal=readConfigFile(ARA_ACQD_CONFIG_FILE,&theConfig);
  if(retVal!=CONFIG_E_OK) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error reading config file %s: bailing\n",configFileName);
    fprintf(stderr,"Arrrgghh\n");
    exit(1);
  }    

  printToScreen=theConfig.printToScreen;
  printf("printToScreen == %d\n",printToScreen);

  // Setup console logging
  if( theConfig.printToScreen ){
    closelog();
    openlog (progName, LOG_PID|LOG_PERROR, ARA_LOG) ;
  }

  // Setup log level
  switch(theConfig.logLevel){
  case 0: setlogmask(LOG_UPTO(LOG_ERR)); break;
  case 1: setlogmask(LOG_UPTO(LOG_WARNING)); break;
  case 2: setlogmask(LOG_UPTO(LOG_NOTICE)); break;
  case 3: setlogmask(LOG_UPTO(LOG_INFO)); break;
  case 4: setlogmask(LOG_UPTO(LOG_DEBUG)); break;
  default: setlogmask(LOG_UPTO(LOG_ERR)); break;
  }


  

  
  //Will probably move the opending of te USB connection into the main thread
  retVal=openFx2Device();
  ///Need to check the return for when we can't talk to the ATRI board


  pthread_attr_t attr;
  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  


  //Need to decide which threads should be detached and which ones joinable
  retVal=pthread_create(&fRunControlSocketThread,&attr,(void*)runControlSocketHandler,NULL);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can't make run control thread");
  }


  //Now make the atriControlThread
  retVal=pthread_create(&fAtriControlSocketThread,&attr,atriControlSocketHandler,NULL);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can't make ATRI control thread");
  }

  
  //Now make the fx2ControlUsbThread
  retVal=pthread_create(&fFx2ControlUsbThread,&attr,fx2ControlUsbHandlder,NULL);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can't make FX2 control USB thread");
  }

  sleep(1);


  fMainThreadFx2SockFd=openConnectionToFx2ControlSocket();
  if(!fMainThreadFx2SockFd) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can not open connection to FX2_CONTROL\n");
    return -1;
  }

  fMainThreadAtriSockFd=openConnectionToAtriControlSocket();
  if(!fMainThreadAtriSockFd) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can not open connection to ATRI_CONTROL\n");
    return -1;
  }


  retVal=pthread_create(&fAraHkThread,&attr,araHkThreadHandler,NULL);
  if (retVal!=0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can't make ARA Hk Thread\n");
  }
  
  //Now wait.
  while(fProgramState!=ARA_PROG_TERMINATE) {
    switch(fProgramState) {
    case ARA_PROG_IDLE:
      usleep(1000);
      break; //Don't do anything just wait

    case ARA_PROG_PREPARING:
      //read config file

      retVal=readConfigFile(configFileName,&theConfig);
      if(retVal!=CONFIG_E_OK) {
	ARA_LOG_MESSAGE(LOG_ERR,"Error reading config file %s: bailing\n",configFileName);
	exit(1);
      }    

      //open files do this do that
#ifndef NO_USB
      if(theConfig.doAtriInitialisation) initialiseAtri(fMainThreadAtriSockFd,fMainThreadFx2SockFd,&theConfig);
#endif


      //Setup output directories
      makeDirectories(theConfig.eventTopDir);
      makeDirectories(theConfig.eventHkTopDir);
      makeDirectories(theConfig.sensorHkTopDir);
      makeDirectories(theConfig.pedsTopDir);
      makeDirectories(theConfig.runLogDir);
      if( theConfig.linkForXfer )
	makeDirectories(theConfig.linkDir);


      
      sprintf(filename,"%s/current/atriEvent.log",theConfig.topDataDir);
      if(theConfig.atriEventLog) {
	fpAtriEventLog=fopen(filename,"w");
	if(fpAtriEventLog==NULL) {
	  ARA_LOG_MESSAGE (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
			   filename);
	}
      }
            
    // --- yyyy
      if ( fpDumpFile == NULL ) {
	 fpDumpFile = fopen("DUMP_FILE","w");
         if ( fpDumpFile != NULL ) {
	    ARA_LOG_MESSAGE( LOG_INFO , "fopen: DUMP_FILE openned" );
	 }
      } else {
	 ARA_LOG_MESSAGE( LOG_ERR , "fopen: error openning DUMP_FILE, errno = %s",strerror(errno) );
      }
// --- yyyy
      

      ///Initialize writer structures
      initWriter(&eventWriter,
		 fCurrentRun,
		 theConfig.compressionLevel,
		 theConfig.filesPerDir,
		 theConfig.eventsPerFile,
		 EVENT_FILE_HEAD,
		 theConfig.eventTopDir,
		 theConfig.linkForXfer?theConfig.linkDir:NULL);    

      gettimeofday(&nowTime,NULL);
      if(  theConfig.enableSoftTrigger 
	   && theConfig.softTriggerRateHz > 0.0
	   && theConfig.softTriggerRateHz < MAX_SOFT_TRIG_RATE ){
	setTimeInterval(&nextSoftStep, 1.0/theConfig.softTriggerRateHz);
	setNextTime(&nextSoftTrig,&nowTime,&nextSoftStep);
      }else{
	if( theConfig.enableSoftTrigger )
	  ARA_LOG_MESSAGE(LOG_WARNING,"Invalid soft trigger rate, disabling soft trigger.");
	theConfig.enableSoftTrigger = 0;
      }
      if(  theConfig.enableRandTrigger
	   && theConfig.randTriggerRateHz > 0.0 
	   && theConfig.randTriggerRateHz < MAX_RAND_TRIG_RATE ){
	srand(nowTime.tv_sec);
	randScale = 2.0 / theConfig.randTriggerRateHz / RAND_MAX;
	setTimeInterval(&nextRandStep,rand()*randScale);
	setNextTime(&nextRandTrig,&nowTime,&nextRandStep);
      }else{
	if( theConfig.enableRandTrigger )
	  ARA_LOG_MESSAGE(LOG_WARNING,"Invalid random trigger rate, disabling random trigger.");
	theConfig.enableRandTrigger = 0;   
      }
      
      
      if(theConfig.setThreshold)
	setAllThresholds(fMainThreadAtriSockFd,theConfig.thresholdValues);
      

      while(!fHkThreadPrepared && fProgramState==ARA_PROG_PREPARING) usleep(1000);
      fProgramState=ARA_PROG_PREPARED;
    case ARA_PROG_PREPARED:
      fCurrentEvent=0;
      break; // Don't do anything just wait
    case ARA_PROG_RUNNING:
      // Main event loop

      if(theConfig.thresholdScan) {
	fprintf(stderr,"Doing threshold scan\n");
	doThresholdScan(fMainThreadAtriSockFd);
	fProgramState=ARA_PROG_TERMINATE;
	break;
      }
      gettimeofday(&nowTime,NULL);

      numBytesRead=0;

      memset(&theEvent,0,sizeof(AraSimpleStationEvent_t));
      retVal=readAtriEvent(&theEvent);
      if(retVal==1) {
	theEvent.unixTime=nowTime.tv_sec;
	theEvent.unixTimeUs=nowTime.tv_usec;
	fillGenericHeader(&theEvent,ARA_EVENT_TYPE,sizeof(AraSimpleStationEvent_t));  

	fprintf(stderr,"Got event: %d -- %d %d %d %d\n",
		theEvent.eventNumber,theEvent.eventId[0],theEvent.eventId[1],theEvent.eventId[2],theEvent.eventId[3]);	
	// Store event
	int new_file_flag = 0;
	retVal = writeBuffer( &eventWriter, (char*)&theEvent, sizeof(AraSimpleStationEvent_t) , &(new_file_flag) );
	if( retVal != sizeof(AraSimpleStationEvent_t) )
	  syslog(LOG_WARNING,"Failed to write event\n");	
      }
      else {
	usleep(1000);
      }

      ///Now we check if we want to send a software trigger
      if( CONDITION_MET( theConfig.enableSoftTrigger, nowTime, nextSoftTrig ) ){
	retVal=sendSoftwareTrigger(fMainThreadAtriSockFd);
	if(retVal < 0){
	  ARA_LOG_MESSAGE(LOG_ERR,"Error %d soft triggering",  retVal);
	  fProgramState = ARA_PROG_TERMINATE;
	  continue;
	}
	else {
	  fprintf(stderr,"Soft Trig\n");
	}
	setNextTime(&nextSoftTrig,&nextSoftTrig,&nextSoftStep);
      }
      

      break;
    case ARA_PROG_STOPPING:
      //Close files and do everything neccessary

// --- yyyy
      if ( fpDumpFile != NULL ) {
	 fclose(fpDumpFile);
	 ARA_LOG_MESSAGE( LOG_INFO , "fclose: DUMP_FILE closed" );
      }
// --- yyyy

      closeWriter(&eventWriter);

      if(fpAtriEventLog) fclose(fpAtriEventLog);
      fpAtriEventLog=NULL;

      while(!fHkThreadStopped && fProgramState==ARA_PROG_STOPPING) usleep(1000);
      fProgramState=ARA_PROG_IDLE;
      break;
    case ARA_PROG_TERMINATE:
      //Immediately exit
      break;
    default:
      break;
    }    
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"fProgramState: %d\n",fProgramState);
  }


  closeConnectionToAtriControlSocket(fMainThreadAtriSockFd);
  closeConnectionToFx2ControlSocket(fMainThreadFx2SockFd);

  
  /* Free attribute and wait for the other threads */
  pthread_attr_destroy(&attr);
  retVal = pthread_join(fAtriControlSocketThread,&status);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"ERROR; return code from pthread_join() is %d\n", retVal);
  }
  retVal = pthread_join(fFx2ControlUsbThread,&status);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"ERROR; return code from pthread_join() is %d\n", retVal);
  }
  retVal = pthread_join(fRunControlSocketThread,&status);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"ERROR; return code from pthread_join() is %d\n", retVal);
  }
  closeFx2Device();

  unlink(ARA_ACQD_PID_FILE);
  return 0; 
}





int runControlSocketHandler() {
  struct pollfd fds[5];
  int sockfd, newsockfd, pollVal;
  AraProgramCommand_t progMsg;
  struct sockaddr_un u_addr; // incoming
  unsigned int u_len = sizeof(u_addr);
  int len;
  int numBytes;
  int nfds,iter;
  
  //Open a socket
  if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"socket -- %s\n",strerror(errno));
    return -1;
  }
  
  u_addr.sun_family = AF_UNIX;
  strcpy(u_addr.sun_path, ACQD_RC_SOCKET);
  unlink(u_addr.sun_path);
  len = strlen(u_addr.sun_path) + sizeof(u_addr.sun_family);

  //Bind address
  if (bind(sockfd, (struct sockaddr *)&u_addr, len) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"bind -- %s\n",strerror(errno));
    return -1;
  }

  //Listen and have a queue of up to 10 connections waiting
  if (listen(sockfd, 5)) {
    ARA_LOG_MESSAGE(LOG_ERR,"listen -- %s\n",strerror(errno));
    return -1;
  }


  fds[0].fd = sockfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  nfds=1;


  while(fProgramState!=ARA_PROG_TERMINATE) {
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;
    fds[0].revents=0;
    fds[1].revents=0;
    fds[2].revents=0;
    fds[3].revents=0;
    fds[4].revents=0;
    
    pollVal=poll(fds,nfds,1);
    if(pollVal==0)
      continue;
    //    fprintf(stderr,"pollVal %d, nfds %d\n",pollVal,nfds);
    if(pollVal==-1) {
      ARA_LOG_MESSAGE(LOG_ERR,"Error calling poll: %s",strerror(errno));
      close(sockfd);
      return -1;
    }
  
    if (fds[0].revents & POLLIN) {      
      //Now accept a new connection
      newsockfd = accept(sockfd,
			 (struct sockaddr *) &u_addr,
			 &u_len);
      if (newsockfd < 0) {
	ARA_LOG_MESSAGE(LOG_ERR,"ERROR on accept");
	return -1;
      }
      if(nfds<5) {
	fds[nfds].fd=newsockfd;
	fds[nfds].events=POLLIN;
	fds[nfds].revents=0;
	nfds++;
      }
      else {
	close(newsockfd);
      }
    }
    if(nfds>1) {      
      for(iter=1;iter<nfds;iter++) {
	newsockfd=fds[iter].fd;
	if (fds[iter].revents & POLLHUP) {
	  // done
	  ARA_LOG_MESSAGE(LOG_INFO,"Connection closed\n");
	  close(newsockfd);
	  nfds--;
	  break;
	} 
	else if (fds[iter].revents & POLLIN) {
	  
	  bzero((char*)&progMsg,sizeof(AraProgramCommand_t));       
	  //Read stuff from socket
	  numBytes = read(newsockfd,(char*)&progMsg,sizeof(AraProgramCommand_t));
	  if(numBytes!=sizeof(AraProgramCommand_t)) {
	    ARA_LOG_MESSAGE(LOG_ERR,"ERROR reading from socket numBytes=%d-- %s\n",numBytes,strerror(errno));
	  }
	  
	  //Check that we have the correct header and footer
	  if(progMsg.feWord==0xfefefefe && progMsg.efWord==0xefefefef) {
	    switch(progMsg.controlState) {
	    case ARA_PROG_CONTROL_QUERY:
	      sendProgramReply(newsockfd,progMsg.controlState);
	      break;
	    case ARA_PROG_CONTROL_PREPARE:
	      //Send a reply that we got the message and are preparing
	      //wait until we are in prepared and then send confirmation
	      if(fProgramState==ARA_PROG_IDLE) {
		//Set Run Number
		fCurrentRun=progMsg.runNumber;
		fProgramState=ARA_PROG_PREPARING;
		sendProgramReply(newsockfd,progMsg.controlState);
		//now wait for prepared
		while(fProgramState==ARA_PROG_PREPARING) {
		  usleep(1000);
		}
		if(fProgramState==ARA_PROG_PREPARED) {
		  sendProgramReply(newsockfd,progMsg.controlState);
		}
	      }
	      else {
		sendProgramReply(newsockfd,progMsg.controlState);
	      }
	      break;
	    case ARA_PROG_CONTROL_START:
	      //Send confirmation that we got the command
	      //depending on current state either go
	      //Idle --> Preparing --> Prepared --> Runinng
	      //or
	      // Prepared --> Runinng
	      if(fProgramState==ARA_PROG_IDLE) {
		//Set Run Number
		fCurrentRun=progMsg.runNumber;
		fProgramState=ARA_PROG_PREPARING;
		sendProgramReply(newsockfd,progMsg.controlState);
		//now wait for prepared
		while(fProgramState==ARA_PROG_PREPARING) {
		  usleep(1000);
		}
		sendProgramReply(newsockfd,progMsg.controlState);
	      }
	      if(fProgramState==ARA_PROG_PREPARED) {
		fProgramState=ARA_PROG_RUNNING;
	      }
	      sendProgramReply(newsockfd,progMsg.controlState);
	      break;
	    case ARA_PROG_CONTROL_STOP:
	      // Send confirmation that we got the command and then depending on the current state go
	      /// Running --> Stopping ---> Idle
	      /// Preparing/Prepared --> Stopping ---> Idle
	      /// Idle (do nothing)
	      if(fProgramState==ARA_PROG_RUNNING || fProgramState==ARA_PROG_PREPARED || fProgramState==ARA_PROG_PREPARING) {
		fProgramState=ARA_PROG_STOPPING;
		sendProgramReply(newsockfd,progMsg.controlState);
		while(fProgramState==ARA_PROG_STOPPING) {
		  usleep(1000);
		}
	      }
	      sendProgramReply(newsockfd,progMsg.controlState);

	      break;
	    default:
	      sendProgramReply(newsockfd,progMsg.controlState);
	      break;
	    }
	  
	  }	  	  
	}	                        
      }
    }
  }
  close(sockfd);
  unlink(ACQD_RC_SOCKET);
  pthread_exit(NULL);
  return -1;
}


void sendProgramReply(int newsockfd ,AraProgramControl_t requestedState) {
  AraProgramCommandResponse_t progReply;
  int numBytes;
  progReply.abWord=0xabababab;
  progReply.baWord=0xbabababa;
  progReply.controlState=requestedState;
  progReply.currentStaus=fProgramState;


  //Send reply
  numBytes = write(newsockfd,(char*)&progReply,sizeof(AraProgramCommandResponse_t));
  if (numBytes < 0)  {
    ARA_LOG_MESSAGE(LOG_ERR,"ERROR writing to socket -- %s\n",strerror(errno));  
  }
  
}



//This thread does...
/// b) sets up the atri_control and fx2_control sockets
/// c) reads stuff from the connections and queues them
/// d) reads/writes stuff to the control port
/// e) sends vendor requests
void *atriControlSocketHandler(void *ptr)
{
  int numAtriControlCons=0;
  int numFx2ControlCons=0;
  int somethingHappened=0;
  int needToSleep=0;

  //This is the atri control socket handler thread
  initAtriControlSocket(ATRI_CONTROL_SOCKET);
  initFx2ControlSocket(FX2_CONTROL_SOCKET);

  while (fProgramState!=ARA_PROG_TERMINATE) {    
    needToSleep=1;
    numAtriControlCons=checkForNewAtriControlConnections();    
    if(numAtriControlCons) {
      somethingHappened=serviceOpenAtriControlConnections();
      if(somethingHappened) {
	needToSleep=0;
	ARA_LOG_MESSAGE(LOG_DEBUG,"Something happened\n");
	ARA_LOG_MESSAGE(LOG_DEBUG,"We have %d connections to atri_cont\n",numAtriControlCons);    
      }     
    }      

    numFx2ControlCons=checkForNewFx2ControlConnections();    
    if(numFx2ControlCons) {
      somethingHappened=serviceOpenFx2ControlConnections();
      if(somethingHappened) {
	needToSleep=0;
	ARA_LOG_MESSAGE(LOG_DEBUG,"Something happened\n");
	ARA_LOG_MESSAGE(LOG_DEBUG,"We have %d connections to fx2_cont\n",numFx2ControlCons);
      }     
    }      

    if(needToSleep)
      usleep(1000);
  }
  //Now need to close the devices
  closeAtriControl(ATRI_CONTROL_SOCKET);
  closeFx2Control(FX2_CONTROL_SOCKET);

  pthread_exit(NULL);

}


/// a) opens the connection to the FX2 microcontroller
void *fx2ControlUsbHandlder(void *ptr)
{
  ARA_LOG_MESSAGE(LOG_DEBUG,"Starting fx2ControlUsbHandlder\n");
  AtriControlPacket_t controlPacket;
  FILE *fpAtriControlLog;
  char filename[FILENAME_MAX];
  FILE *fpAtriControlSentLog;
  char filenameSent[FILENAME_MAX];
  int retVal=0,i=0,count=0;;
  int numBytesRead=0;
  int numBytesLeft=0;
  int thisPacketSize=0;
  int startByte=0;
  int numBytesWritten=0;
  unsigned char buffer[2048]; //Shoiuld be large enough for now
  unsigned char outBuffer[2048]; //Shoiuld be large enough for now
  sprintf(filename,"%s/current/atriControl.log",theConfig.topDataDir);
  sprintf(filenameSent,"%s/current/atriControlSent.log",theConfig.topDataDir);
  //  int loopCount=0;
  while (fProgramState!=ARA_PROG_TERMINATE) {
    //Need to read stuff from the control port
    numBytesRead=0;
    startByte = 0; //need to reset to beginning
    //    fprintf(stderr,"fx2ControlUsbHandlder loopCount=%d\n",loopCount);
    //    loopCount++;
    retVal=readControlEndPoint(buffer,2043,&numBytesRead);
    if(retVal<0) {
      if(retVal!=-7) ///timeout
	ARA_LOG_MESSAGE(LOG_ERR,"Can't read from usb control endpoint\n");
    }
    else if(numBytesRead>0) {
      if(theConfig.atriControlLog) {
	fpAtriControlLog=fopen(filename,"a");
	if(fpAtriControlLog==NULL) {
	  ARA_LOG_MESSAGE(LOG_ERR,"Error opening %s -- %s\n",filename,strerror(errno));
	}
	else {
	  for(i=0;i<numBytesRead;i++) {
	    fprintf(fpAtriControlLog,"%#2.2x ",buffer[i]);
	  }
	  fprintf(fpAtriControlLog,"\n");
	  fclose(fpAtriControlLog);
	}
      }
      ARA_LOG_MESSAGE(LOG_DEBUG,"Read %d bytes from control endpoint\n",numBytesRead);
      numBytesLeft=numBytesRead;
      //Now we have read an atri control packet
      //do something with it
      while(numBytesLeft>4) {
	memset(&controlPacket,0,sizeof(AtriControlPacket_t));
	controlPacket.header.frameStart=buffer[startByte+0];
	controlPacket.header.packetLocation=buffer[startByte+1];
	controlPacket.header.packetNumber=buffer[startByte+2];
	controlPacket.header.packetLength=buffer[startByte+3];
	thisPacketSize=5+controlPacket.header.packetLength;
	if(numBytesRead>=buffer[startByte+3]+5) {
	  //Have read enough bytes to fill data array
	  for(i=0;i<controlPacket.header.packetLength+1;i++) {
	    controlPacket.data[i]=buffer[startByte+4+i];
	  }
	  if(controlPacket.header.frameStart==0x3c && 
	     buffer[startByte+4+controlPacket.header.packetLength]==0x3e) {
	    ARA_LOG_MESSAGE(LOG_DEBUG,"Read Good Packet -- %#x %#x %#x %d -- data %#x %#x %#x %#x\n",
			    controlPacket.header.frameStart,
			    controlPacket.header.packetLocation,
			    controlPacket.header.packetNumber,
			    controlPacket.header.packetLength,
			    controlPacket.data[0],
			    controlPacket.data[1],
			    controlPacket.data[2],
			    controlPacket.data[3]);
	    if(controlPacket.header.packetNumber>0  ) sendAtriControlPacketToSocket(&controlPacket);
	  }
	  else {  	    	   
	    for(i=0;i<thisPacketSize;i++) {
	      ARA_LOG_MESSAGE(LOG_WARNING,"Bad Packet -- %d -- %#x\n",i,buffer[startByte+i]);
	    }
	  }
	}
	else {
	  ARA_LOG_MESSAGE(LOG_WARNING,"Not eneough data bytes malformed packet. Expected %d but only have %d\n",buffer[3]+5,numBytesRead);
	}
	startByte+=thisPacketSize;
	numBytesLeft-=thisPacketSize;
      }
      if(numBytesLeft>0) {
	ARA_LOG_MESSAGE(LOG_WARNING,"Not enough header bytes read, only %d\n",numBytesLeft);
      }	
    }
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"Trying to get controlPacket\n");
    retVal=getControlPacketFromQueue(&controlPacket);
    if(retVal==1) {
      ARA_LOG_MESSAGE(LOG_DEBUG,"Got a control packet to write to USB endpoint\n");
      ///Got a control packet     
      outBuffer[0]=controlPacket.header.frameStart;
      outBuffer[1]=controlPacket.header.packetLocation;
      outBuffer[2]=controlPacket.header.packetNumber;
      outBuffer[3]=controlPacket.header.packetLength;
      //The +1 includes the frameEnd byte
      for(i=0;i<controlPacket.header.packetLength+1;i++) {
	outBuffer[4+i]=controlPacket.data[i];
      }      
      count=5+controlPacket.header.packetLength;

      //      for(i=0;i<count;i++) {
      //	printf("writing %d -- %#x\n",i,outBuffer[i]);
      //      }
	  

      if(theConfig.atriControlLog) {
	fpAtriControlSentLog=fopen(filenameSent,"a");
	if(fpAtriControlSentLog==NULL) {
	  ARA_LOG_MESSAGE(LOG_ERR,"Error opening %s -- %s\n",filenameSent,strerror(errno));
	}
	else {
	  for(i=0;i<count;i++) {
	    fprintf(fpAtriControlSentLog,"%#2.2x ",outBuffer[i]);
	  }
	  fprintf(fpAtriControlSentLog,"\n");
	  fclose(fpAtriControlSentLog);
	}
      }


      numBytesWritten=0;
      retVal=writeControlEndPoint(outBuffer,count,&numBytesWritten);
      if(retVal<0) {
	ARA_LOG_MESSAGE(LOG_ERR,"Can't write to usb control endpoint\n");
      }
      else if(numBytesWritten!=count) {
	ARA_LOG_MESSAGE(LOG_ERR,"Tried to write %d only did %d bytes\n",count,numBytesWritten);
      }
    }		    
    usleep(1000);
  }
  pthread_exit(NULL);

}



void sigUsr1Handler(int sig)
{
  fProgramState=ARA_PROG_STOPPING;
} 

void sigUsr2Handler(int sig)
{
  fProgramState=ARA_PROG_TERMINATE;
} 

int initialiseAtri(int fAtriSockFd,int fFx2SockFd,ARAAcqdConfig_t *theConfig)
{
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s\n",__FUNCTION__);
  //This function sets up the wilkinson
  int retVal=initialiseAtriClocks(fFx2SockFd,theConfig);
  retVal=initialiseAtriDaughterboards(fAtriSockFd,theConfig);
  retVal=initialiseAtriDACs(fAtriSockFd,theConfig);

  uint8_t enableIRS=0x1;
  retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_IRSEN,1,&enableIRS);
  
  uint8_t enableWilkinson=0x0F;
  retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_WILKMONEN,1,&enableWilkinson);

  return retVal;
}

int initialiseAtriClocks(int fFx2SockFd,ARAAcqdConfig_t *theConfig)
{
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s\n",__FUNCTION__);
  //This is taken almost completely from Patrick's clockset_real

  //Reset the Si5367/8
  writeToI2CRegister(fFx2SockFd,0xd0,0x88,0x80);
  writeToI2CRegister(fFx2SockFd,0xd0,0x88,0x00);


  //This fucntion is basically just a rehash of Patrick's clockset.sh
  //Needs some error checking

  //Turn on the FPGA oscillator.  
  uint8_t enableMask=FX2_ATRI_FPGA_OSC_MASK;
  int retVal=enableAtriComponents(fFx2SockFd,enableMask);
  

  // If we initialize to the external Rubidium clock, we instead
  // would disable the FPGA oscillator.

  // Register 0: No free run, no clockout always on, CKOUT5 is output, no bypass
  writeToI2CRegister(fFx2SockFd,0xd0,0x00,0x14);

  // Register 1: Clock priorities. 1, then 2, then 3.
  writeToI2CRegister(fFx2SockFd,0xd0,0x01,0xE4);

  // Register 2: BWSEL_REG[3:0], 0010. BWSEL = 0000.
  writeToI2CRegister(fFx2SockFd,0xd0,0x02,0x02);

  // Register 3: CKSEL_REG[1:0], DHOLD, SQ_ICAL, 0101. CKSEL_REG = 10: clock 1 (ext. Rb)
  writeToI2CRegister(fFx2SockFd,0xd0,0x03,0x05);

  // Register 4: Autoselect clocks enabled (1, then 2, then 3)
  writeToI2CRegister(fFx2SockFd,0xd0,0x04,0x92);

  // Register 5: CKOUT2/CKOUT1 are LVDS
  writeToI2CRegister(fFx2SockFd,0xd0,0x05,0x3F);
  // Register 6: CKOUT4/CKOUT3 are LVDS
  writeToI2CRegister(fFx2SockFd,0xd0,0x06,0x3F);
  // Register 7: CKOUT5 is LVDS. CKIN3 is frequency offset alarms (who cares)
  writeToI2CRegister(fFx2SockFd,0xd0,0x07,0x3B);
  // Register 8: Hold logic. All normal ops.
  writeToI2CRegister(fFx2SockFd,0xd0,0x08,0x00);
  // Register 9: Histogram generation, plus hold logic.
  writeToI2CRegister(fFx2SockFd,0xd0,0x09,0xC0);
  // Register 10: Output buffer power.
  writeToI2CRegister(fFx2SockFd,0xd0,0x0A,0x00);
  // Register 11: Power down CKIN4 input buffer.
  writeToI2CRegister(fFx2SockFd,0xd0,0x0B,0x48);
  // Register 19: Frequency offset alarn, who cares.
  writeToI2CRegister(fFx2SockFd,0xd0,0x13,0x2C);
  // Register 20: Alarm pin setups.
  writeToI2CRegister(fFx2SockFd,0xd0,0x14,0x02);
  // Register 21: Ignore INC/DEC/FSYNC_ALIGN, tristate CKx_ACTV, ignore CKSEL
  // bleh this is default now 
  writeToI2CRegister(fFx2SockFd,0xd0,0x15,0xE0);
  // Register 22: Default
  writeToI2CRegister(fFx2SockFd,0xd0,0x16,0xDF);
  // Register 23: mask everything
  writeToI2CRegister(fFx2SockFd,0xd0,0x17,0x1F);
  // Register 24: mask everything
  writeToI2CRegister(fFx2SockFd,0xd0,0x18,0x3F);

  // These determine output clock frequencies.
  // CKOUTx is 5 GHz divided by N1_HS divided by NCx_LS
  // CKOUT1 = D2REFCLK
  // CKOUT2 = D3REFCLK
  // CKOUT3 = D1REFCLK
  // CKOUT4 = D4REFCLK
  // CKOUT5 = FPGA_REFCLK

  // Right now all daughter refclks are 25 MHz, and the FPGA refclk is 100 MHz.
  // This should probably be changed so that the daughter refclks are at least
  // 100 MHz.

  // Register 25-27: N1_HS=001 (N1=5), NC1_LS = 0x09 (10)
  writeToI2CRegister(fFx2SockFd,0xd0,0x19,0x20);
  writeToI2CRegister(fFx2SockFd,0xd0,0x1A,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x1B,0x09);
  // Register 28-30: NC2_LS = 0x09 (10)
  writeToI2CRegister(fFx2SockFd,0xd0,0x1C,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x1D,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x1E,0x09);
  // Register 31-33: NC3_LS = 0x09 (10) 
  writeToI2CRegister(fFx2SockFd,0xd0,0x1F,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x20,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x21,0x09);
  // Register 34-36: NC4_LS = 0x09 (10)
  writeToI2CRegister(fFx2SockFd,0xd0,0x22,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x23,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x24,0x09);
  // Register 37-39: NC5_LS = 0x09 (10)
  writeToI2CRegister(fFx2SockFd,0xd0,0x25,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x26,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x27,0x09);

  // These determine the VCO frequency.
  // It's (CKINx/N3x)*(N2_HS)*(N2_LS).
  // For CKIN1 or CKIN3 = 10 MHz, this is (10/1)*(10)*(500) = 5 GHz

  // Register 40-42: N2_HS = 110 (10), N2_LS = 500
  writeToI2CRegister(fFx2SockFd,0xd0,0x28,0xC0);
  writeToI2CRegister(fFx2SockFd,0xd0,0x29,0x01);
  writeToI2CRegister(fFx2SockFd,0xd0,0x2A,0xF4);
  // Register 44-45: CKIN1 divider (N31) = 0x0 (1)
  writeToI2CRegister(fFx2SockFd,0xd0,0x2B,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x2C,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x2D,0x00);
  // Register 46-48: CKIN2 divider (N32) = 0x0 (1)
  writeToI2CRegister(fFx2SockFd,0xd0,0x2E,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x2F,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x30,0x00);
  // Register 49-51: CKIN3 divider (N33) = 0x0 (1)
  writeToI2CRegister(fFx2SockFd,0xd0,0x31,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x32,0x00);
  writeToI2CRegister(fFx2SockFd,0xd0,0x33,0x00);
  // done, now do an ICAL
  writeToI2CRegister(fFx2SockFd,0xd0,0x88,0x40);


  //all done
  return retVal;
}

int initialiseAtriDaughterboards(int fAtriSockFd,ARAAcqdConfig_t *theConfig)
{

  ARA_LOG_MESSAGE(LOG_DEBUG,"%s\n",__FUNCTION__);
  //At some point we will actually look at the config to fdetermine if we wnat things on off
  //For now it is just all on no matter what

#define PRESENT(stat,type,stack) ((stat) & ((1<<(2*(type))) << (8*(stack))))
#define POWERED(stat,type,stack) ((stat) & ((1<<(2*(type)+1)) << (8*(stack))))
#define DBANY(stat,stack) ((stat) & (0xFF << (8*(stack))))
  
  int stack,type;
  AtriDaughterStatus_t daughterStatus[4][4];
  unsigned int dbStatus=atriGetDaughterStatus(fAtriSockFd);  
  
  //Wil do something more worthy than just print it out
  printf("ATRI Daughter Status:\n");
  for(stack=D1;stack<=D4;stack++) {
    for(type=DDA;type<=DRSV10;type++) {
      printf("%s: %s present: %c powered: %c\n",
	     atriDaughterStackStrings[stack],
	     atriDaughterTypeStrings[type],
	     PRESENT(dbStatus,type,stack) ? 'Y' : 'N',
	     POWERED(dbStatus,type,stack) ? 'Y' : 'N');
    }
    printf("\n");
  }
 
  for(stack=D1;stack<=D4;stack++) {
    printf("Initialising D%d daughters...\n",(int)(stack+1));
    for(type=DDA;type<=DRSV10;type++) {
      if(PRESENT(dbStatus,type,stack)) {
	atriDaughterPowerOn(fAtriSockFd,stack,type,&daughterStatus[stack][type]);
      }
    }
  }   
  return 0;
}

int initialiseAtriDACs(int fAtriSockFd,ARAAcqdConfig_t *theConfig)
{
  //Again will use the config at some point maybe
  int stack=0;
  for(stack=D1;stack<=D4;stack++) {    
    //        setIRSClockDACValues(fAtriSockFd,stack,0xB852,0x4600);
    setIRSVdlyDACValue(fAtriSockFd,stack,0xB852);
  }
  for(stack=D1;stack<=D4;stack++) {    
    //        setIRSClockDACValues(fAtriSockFd,stack,0xB852,0x4600);
    setIRSVadjDACValue(fAtriSockFd,stack,0x4600);
  }
  return 0;
}



int readConfigFile(const char* configFileName,ARAAcqdConfig_t* theConfig)
{

  int status = 0, allStatus = 0;
  KvpErrorCode kvpStatus = 0;
  const char* configSection[] = { "log","acq","thresholds","trigger",
				  "servo",NULL };
  const char** currSection = configSection;

  kvpReset();
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"Here\n");
  allStatus = configLoad(ARAD_CONFIG_FILE,"output");
  if(allStatus != CONFIG_E_OK) {
    ARA_LOG_MESSAGE(LOG_ERR,"Problem reading %s -- %s\n",ARAD_CONFIG_FILE,"output");
  }
  

  while( *currSection ){
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"Here2\n");
    status = configLoad((char*)configFileName,(char*)*currSection);
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"Here3 --%d\n",status);
    allStatus += status;
    if(status != CONFIG_E_OK) {
      ARA_LOG_MESSAGE(LOG_ERR,"Problem reading %s -- %s\n",configFileName,*currSection);
    }
    ++currSection;
  }

#define SET_INT(x,d)    theConfig->x=kvpGetInt(#x,d);
#define SET_FLOAT(x,f)  theConfig->x=kvpGetFloat(#x,f);
#define SET_STRING(x,s) {char* tmpStr=kvpGetString(#x);strcpy(theConfig->x,tmpStr?tmpStr:s);}

  if( allStatus == CONFIG_E_OK) {
    // Get logging 
    SET_INT(printToScreen,0);
    SET_INT(logLevel,0);
    SET_INT(atriControlLog,0);
    SET_INT(atriEventLog,0);
    // Get data directories
    SET_STRING(topDataDir,".");
    sprintf(theConfig->eventTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_EVENT_DIR);
    sprintf(theConfig->eventHkTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_EVENT_HK_DIR);
    sprintf(theConfig->sensorHkTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_SENSOR_HK_DIR);
    sprintf(theConfig->pedsTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_PED_DIR);
    sprintf(theConfig->runLogDir,"%s/current/%s",theConfig->topDataDir,DAQ_RUNLOG_DIR);
    SET_STRING(runNumFile,"run_number")
      SET_STRING(linkDir,"./link");
    SET_INT(linkForXfer,0);
    SET_INT(filesPerDir,100);
    SET_INT(eventsPerFile,100);
    SET_INT(hkPerFile,1000);
    // Run parameters
    SET_INT(doAtriInitialisation,0);
    SET_INT(standAlone,0);
    SET_INT(numEvents,1);
    SET_INT(enableHkRead,1);
    SET_FLOAT(eventHkReadRateHz,10);
    SET_INT(sensorHkReadPeriod,60);
    SET_INT(pedestalMode,0);
    SET_INT(pedestalSamples,1000);
    // Thresholds
    SET_INT(thresholdScan,0);
    SET_INT(thresholdScanStep,1);
    SET_INT(thresholdScanStartPoint,1);
    SET_INT(thresholdScanPoints,1);
    SET_INT(setThreshold,1);
    SET_INT(useGlobalThreshold,0);
    SET_INT(globalThreshold,2068);
    if(theConfig->useGlobalThreshold){
      // Copy global threshold onto threshold array
      int i;
      for( i = 0; i < THRESHOLDS_PER_ATRI; ++i )
	theConfig->thresholdValues[i]=theConfig->globalThreshold;
    }else{ // Read individual thresholds
      int tempNum=THRESHOLDS_PER_ATRI;
      kvpStatus=kvpGetUnsignedShortArray("thresholdValues",theConfig->thresholdValues,&tempNum);
      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus));
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(thresholdValues): %s",kvpErrorString(kvpStatus));
	if( theConfig->setThreshold )
	  status = 1; // Terminate
      }
    }  
    // Trigger
    SET_INT(enableSelfTrigger,1);
    SET_INT(enableSoftTrigger,0);
    SET_INT(enableRandTrigger,0);
    SET_FLOAT(softTriggerRateHz,0.0);
    SET_FLOAT(randTriggerRateHz,0.0);
    // Servo
    SET_INT(enableServo,0);
    SET_FLOAT(servoCalcPeriodS,1);
    SET_FLOAT(servoCalcDelayS,10);
    SET_FLOAT(rateGoalHz,5); 
    SET_FLOAT(ratePGain,0.5);
    SET_FLOAT(rateIGain,0.01);
    SET_FLOAT(rateDGain,0.0);
    SET_FLOAT(rateIMax,100);
    SET_FLOAT(rateIMin,-100);
  }

  return allStatus;

}



void *araHkThreadHandler(void *ptr)
{
  ARA_LOG_MESSAGE(LOG_DEBUG,"Starting %s\n",__FUNCTION__);
  struct timeval lastSensorHkTime;
  struct timeval lastEventHkTime;
  struct timeval currentTime;

  AraSensorHk_t sensorHk;
  AraEventHk_t eventHk;
  int retVal;


  int fHkThreadFx2SockFd=openConnectionToFx2ControlSocket();
  int fHkThreadAtriSockFd=openConnectionToAtriControlSocket();


  lastSensorHkTime.tv_sec=0;
  lastEventHkTime.tv_sec=0;

  float sensorHkPeriod=1;
  float eventHkPeriod=1;
  fHkThreadPrepared=0;
  fHkThreadStopped=0;

  while (fProgramState!=ARA_PROG_TERMINATE) {
    switch(fProgramState) {
    case ARA_PROG_IDLE:
      usleep(1000);
      fHkThreadPrepared=0;
      fHkThreadStopped=0;
      break; //Don't do anything just wait

    case ARA_PROG_PREPARING:
      //Setup output directories
      if(!fHkThreadPrepared) {
	printf("ARA_PROG_PREPARING in araHkThreadHandler\n");
	makeDirectories(theConfig.eventHkTopDir);
	makeDirectories(theConfig.sensorHkTopDir);
	
	///Initialize writer structures
	initWriter(&eventHkWriter,
		   fCurrentRun,
		   theConfig.compressionLevel,
		   theConfig.filesPerDir,
		   theConfig.hkPerFile,
		   EVENT_HK_FILE_HEAD,
		   theConfig.eventHkTopDir,
		   theConfig.linkForXfer?theConfig.linkDir:NULL);    
	
	initWriter(&sensorHkWriter,
		   fCurrentRun,
		   theConfig.compressionLevel,
		   theConfig.filesPerDir,
		   theConfig.hkPerFile,
		   SENSOR_HK_FILE_HEAD,
		   theConfig.sensorHkTopDir,
		   theConfig.linkForXfer?theConfig.linkDir:NULL);        
	
	eventHkPeriod=1./theConfig.eventHkReadRateHz;
	sensorHkPeriod=theConfig.sensorHkReadPeriod;
	fHkThreadPrepared=1;
      }
      break;
    case ARA_PROG_PREPARED:
      usleep(1000);
      break; // Don't do anything just wait
    case ARA_PROG_RUNNING:
      // Main event loop
      //      printf("ARA_PROG_RUNNING in araHkThreadHandler\n");
      fHkThreadStopped=1;
      gettimeofday(&currentTime,NULL);
      if(checkDeltaT(&currentTime,&lastSensorHkTime,sensorHkPeriod)) {
	//Need to read out a sensor hk thingy
	lastSensorHkTime=currentTime;
	retVal=readSensorHk(fHkThreadFx2SockFd,fHkThreadAtriSockFd,&sensorHk,&currentTime);
	//Now need to add it to the writer thingy

	// Store hk event
	int new_hk_file_flag = 0;
	retVal = writeBuffer( &sensorHkWriter, (char*)&sensorHk, sizeof(AraSensorHk_t) , &(new_hk_file_flag) );
	if( retVal != sizeof(AraSensorHk_t) )
	  syslog(LOG_WARNING,"Failed to write sensor housekeeping event");
	
      }
      
      if(checkDeltaT(&currentTime,&lastEventHkTime,eventHkPeriod)) {
	//Need to read out a sensor hk thingy
	lastEventHkTime=currentTime;
	retVal=readEventHk(fHkThreadAtriSockFd,&eventHk,&currentTime);
	//Now need to add it to the writer thingy

	// Store hk event
	int new_hk_file_flag = 0;
	retVal = writeBuffer( &eventHkWriter, (char*)&eventHk, sizeof(AraEventHk_t) , &(new_hk_file_flag) );
	if( retVal != sizeof(AraEventHk_t) )
	  syslog(LOG_WARNING,"Failed to write event housekeeping event");
      }
      usleep(1000);

      break;
    case ARA_PROG_STOPPING:
      //Close files and do everything neccessary
      if(!fHkThreadStopped)  {
	closeWriter(&sensorHkWriter);
	closeWriter(&eventHkWriter);
	fHkThreadStopped=1;
      }      
      break;
    case ARA_PROG_TERMINATE:
      //Immediately exit
      break;
    default:
      break;
    }    
  }
  closeConnectionToFx2ControlSocket(fHkThreadFx2SockFd);
  closeConnectionToAtriControlSocket(fHkThreadAtriSockFd);

  pthread_exit(NULL);
}


int checkDeltaT(struct timeval *currTime, struct timeval *lastTime, float deltaT)
{
  float timeDiff=(currTime->tv_usec-lastTime->tv_usec)*1e-6;
  timeDiff+=(currTime->tv_sec-lastTime->tv_sec);
  return (timeDiff>deltaT);
}


void copyTwoBytes(uint8_t *data, uint16_t *value)
{
  memcpy(value,data,2);
}

void copyFourBytes(uint8_t *data, uint32_t *value)
{
  memcpy(value,data,4);
}



int readSensorHk(int fFx2SockFd,int fAtriSockFd,AraSensorHk_t *sensorPtr, struct timeval *currTime)
{
  int stack;
  sensorPtr->unixTime=currTime->tv_sec;
  sensorPtr->unixTimeUs=currTime->tv_usec;
  
#ifndef NO_USB
  //From Patrick's email
  /* 4: ATRI voltage/current: 
     write 0x4 to FX2 I2C address 0x96, 
     read 1 byte from FX2 I2C address 0x97 (ATRI voltage), 
     write 0x5 to FX2 I2C address 0x96, 
     read 1 byte from FX2 I2C address 0x97 (ATRI current) */
  /* 5: DDA/TDA voltage/temperature/current: For all daughterboards: 
     write 0x0C to I2C address 0xE4, 
     write 0x0C to I2C address 0xFC, 
     read 2 bytes from I2C address 0x91 (DDA temp), 
     read 2 bytes from I2C address 0x93 (TDA temp), 
     read 3 bytes from I2C address 0xE5 (DDA voltage/current),
     read 3 bytes from I2C address 0xFD (TDA voltage/current). */

  //ATRI Voltage
  uint8_t data[4]={0,0,0,0};
  uint16_t value16=0;
  uint16_t value32=0;
  data[0]=0x4;
  writeToI2C(fFx2SockFd,0x96,1,data);
  readFromI2C(fFx2SockFd,0x97,1,data);
  sensorPtr->atriVoltage=data[0];

  //ATRI Current
  data[0]=0x5;
  writeToI2C(fFx2SockFd,0x96,1,data);
  readFromI2C(fFx2SockFd,0x97,1,data);
  sensorPtr->atriCurrent=data[0];

  //DDA Temp/voltage/current
  for(stack=D1;stack<=D4;stack++) {  
    data[0]=0x0c;
    writeToAtriI2C(fAtriSockFd,stack,0xe4,1,data);
    writeToAtriI2C(fAtriSockFd,stack,0xfc,1,data);
  }

  for(stack=D1;stack<=D4;stack++) {  
    //Read DDA temp
    readFromAtriI2C(fAtriSockFd,stack,0x91,2,data);
    value16=data[1];
    value16=(value16<<8);
    value16|=data[0];
    sensorPtr->ddaTemp[stack]=value16;
    //Read TDA temp
    readFromAtriI2C(fAtriSockFd,stack,0x93,2,data);
    value16=data[1];
    value16=(value16<<8);
    value16|=data[0];
    sensorPtr->tdaTemp[stack]=value16;
    //Read DDA voltage/current
    readFromAtriI2C(fAtriSockFd,stack,0xe5,3,data);
    value32=data[2];
    value32=(value32<<16);
    value16=data[1];
    value16=(value16<<8);
    value32|=value16;
    value32|=data[0];
    sensorPtr->ddaVoltageCurrent[stack]=value32;
    //Read TDA voltage/current
    readFromAtriI2C(fAtriSockFd,stack,0xfd,3,data);
    value32=data[2];
    value32=(value32<<16);
    value16=data[1];
    value16=(value16<<8);
    value32|=value16;
    value32|=data[0];
    sensorPtr->tdaVoltageCurrent[stack]=value32;
  }
#endif
  fillGenericHeader(sensorPtr,ARA_SENSOR_HK_TYPE,sizeof(AraSensorHk_t));  
  return 0;
  
}
		     
int readEventHk(int fAtriSockFd,AraEventHk_t *eventPtr, struct timeval *currTime)
{
  //At the moment we are doing everything with small reads. At some point will chnage to big reads when we need the extra speed.
  
  //  int stack;
  //  uint32_t value32;
  //  uint32_t temp32;
  uint8_t data[64];

  eventPtr->unixTime=currTime->tv_sec;
  eventPtr->unixTimeUs=currTime->tv_usec;
#ifndef NO_USB

  /*  1: read address 0x2C-0x3B on the WISHBONE bus (Wilkinson counter and */
  /* sample speed monitors) */
  /*  2: read address 0x30-0x37 on the WISHBONE bus (clock count/PPS count) */
  /*  3: read address 0x0100-0x15F on the WISHBONE bus (scalers) */

  atriWishboneRead(fAtriSockFd,ATRI_WISH_IDREG,8,data);
  printf("ATRI_WISH_IDREG %d %d %d %d -- %c %c %c %c\n",
	 data[0],data[1],data[2],data[3],
	 data[3],data[2],data[1],data[0]);
  printf("ATRI_WISH_VERREG %d %d %d %d\n",
	 data[4],data[5],data[6],data[7]);

  //Firmware version
  //  atriWishboneRead(fAtriSockFd,ATRI_WISH_VERREG,4,(uint8_t*)&(eventPtr->firmwareVersion));
  copyFourBytes(&(data[4]),&(eventPtr->firmwareVersion));


  //Clock counters
  atriWishboneRead(fAtriSockFd,ATRI_WISH_D1WILK,16,data);

  copyTwoBytes(&(data[ATRI_WISH_D1WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[0]));
  copyTwoBytes(&(data[ATRI_WISH_D1DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[0]));
  copyTwoBytes(&(data[ATRI_WISH_D2WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[1]));
  copyTwoBytes(&(data[ATRI_WISH_D2DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[1]));
  copyTwoBytes(&(data[ATRI_WISH_D3WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[2]));
  copyTwoBytes(&(data[ATRI_WISH_D3DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[2]));
  copyTwoBytes(&(data[ATRI_WISH_D4WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[3]));
  copyTwoBytes(&(data[ATRI_WISH_D4DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[3]));

  //PPS counter
  atriWishboneRead(fAtriSockFd,ATRI_WISH_PPSCNT,4,(uint8_t*)&(eventPtr->ppsCounter));

  //Clock counter
  atriWishboneRead(fAtriSockFd,ATRI_WISH_CLKCNT,4,(uint8_t*)&(eventPtr->clockCounter));
  
  //L1 Scalers -- 
  atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_D1_L1_C1,44,data);
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D1_L1_C1-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[0][0]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D1_L1_C2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[0][1]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D1_L1_C3-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[0][2]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D1_L1_C4-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[0][3]));  
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D2_L1_C1-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[1][0]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D2_L1_C2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[1][1]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D2_L1_C3-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[1][2]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D2_L1_C4-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[1][3]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D3_L1_C1-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[2][0]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D3_L1_C2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[2][1]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D3_L1_C3-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[2][2]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D3_L1_C4-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[2][3]));  
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D4_L1_C1-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[3][0]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D4_L1_C2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[3][1]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D4_L1_C3-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[3][2]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D4_L1_C4-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l1Scaler[3][3]));
  
  //L2 Scalers
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D1_L2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l2Scaler[0]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D2_L2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l2Scaler[1]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D3_L2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l2Scaler[2]));
  copyTwoBytes(&(data[ATRI_WISH_SCAL_D4_L2-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l2Scaler[3]));

  //L3 Scaler
  copyTwoBytes(&(data[ATRI_WISH_SCAL_L3-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->l3Scaler));

  //L3 Scaler
  copyTwoBytes(&(data[ATRI_WISH_SCAL_TRIG-ATRI_WISH_SCAL_D1_L1_C1]),(uint16_t*)&(eventPtr->triggerScaler));
  
#endif
  
  fillGenericHeader(eventPtr,ARA_EVENT_HK_TYPE,sizeof(AraEventHk_t));  
  return 0;
}

int sendSoftwareTrigger(int fAtriSockFd)
{
  //Do we just send the lower 8 bits or should we send the four bits to enable the daughter triggers?
  uint8_t softwareTrigger=0x01;
  return atriWishboneWrite(fAtriSockFd,ATRI_WISH_SOFTTRIG,1,&softwareTrigger);
}


int readAtriEvent(AraSimpleStationEvent_t *eventPtr)
{
  //Try to read an event, returns 0 if there is no event, 1 if there is an event and -1 if there is a protocol error
  int retVal=0;
  int numBytesRead,i;
  unsigned char buffer[4096];
  
  int expectFrame=0;
  int eventLength=0;
  int expectStack=0;
  uint32_t eventId;
  uint16_t blockId;
  int uptoSample=0;
  int wordsLeft=0;

  while (1) {
    retVal=readEventEndPoint(buffer,512,&numBytesRead);
    if(retVal==0) {
      if(numBytesRead>0) {
	//	fprintf(stderr,"\nRead %d bytes from event end point\n",numBytesRead);	 	  
	//	fprintf(stderr,"expectFrame %d, expectStack %d\n",expectFrame,expectStack);
	//Try and unpack the event
	if(expectFrame==0) {
	  if(buffer[0]==0x65 && buffer[1]==0) {
	    uptoSample=0;
	    //Correct frame number	    
	    eventLength = buffer[2];
	    eventLength |= (buffer[3] << 8);
	    
	    
	    if(expectStack==0)
	      fCurrentEvent++;
	    eventPtr->eventNumber=fCurrentEvent;

	    //Unpack eventId
	    eventId = (buffer[4] << 16);
	    eventId |= (buffer[5] << 24);
	    eventId |= (buffer[6]);
	    eventId |= (buffer[7] << 8);
	    eventPtr->eventId[expectStack]=eventId;

	    //Unpack blockId
	    blockId = buffer[8];
	    blockId |= (buffer[9] << 8);
	    eventPtr->blockId[expectStack]=blockId;
	    
	    wordsLeft=numBytesRead-10;
	    wordsLeft/=2;
	    for(i=0;i<wordsLeft;i++) {
	      eventPtr->samples[expectStack][uptoSample]=buffer[10+(2*i)];
	      eventPtr->samples[expectStack][uptoSample]|=(buffer[10+(2*i)+1]<<8);
// --- yyyy (dump stack 3 to output)
	      if ( expectStack == 2 ) { fprintf(fpDumpFile,"%d %d\n",uptoSample,eventPtr->samples[expectStack][uptoSample]); }
// --- yyyy
	      uptoSample++;
	    }
// --- yyyy (dump stack 3 to output)
	    fflush(fpDumpFile);
// --- yyyy
	    expectFrame++;
	    if(eventLength==numBytesRead-4) {
	      expectFrame=0;
	      expectStack++;
	      if(expectStack==4) {
		//Finished event
		expectStack=0;
		return 1;
	      }
	    }
	  }
	  else {
	    ARA_LOG_MESSAGE(LOG_ERR,"Malformed packet %#x %#x\n",buffer[0],buffer[1]);
	  }
	}
	else {
	  if(buffer[0]==0x45 && buffer[1]==expectFrame) {
	    //Correct frame number	    
	    eventLength = buffer[2];
	    eventLength |= (buffer[3] << 8);	    

	    wordsLeft=numBytesRead-4;
	    wordsLeft/=2;
	    for(i=0;i<wordsLeft;i++) {
	      eventPtr->samples[expectStack][uptoSample]=buffer[10+(2*i)];
	      eventPtr->samples[expectStack][uptoSample]|=(buffer[10+(2*i)+1]<<8);
// --- yyyy (dump stack 3 to output)
	      if ( expectStack == 2 ) { fprintf(fpDumpFile,"%d %d\n",uptoSample,eventPtr->samples[expectStack][uptoSample]); }
// --- yyyy
	      uptoSample++;
	    }
// --- yyyy (dump stack 3 to output)
	    fflush(fpDumpFile);
// --- yyyy
	    expectFrame++;
	    if(eventLength==numBytesRead-4) {
	      expectFrame=0;
	      expectStack++;
	      if(expectStack==4) {
		//Finished event
		expectStack=0;
		return 1;
	      }
	    }
	  }
	  else {
	    ARA_LOG_MESSAGE(LOG_ERR,"Malformed packet %#x %#x\n",buffer[0],buffer[1]);
	  }
	}

	//Log the event
	if(fpAtriEventLog!=NULL && theConfig.atriEventLog) {
	  for (i=0;i<numBytesRead;i++) 
	    {
              fprintf(fpAtriEventLog,"%2.2x", buffer[i]); 
              if (!((i+1)%16) || (i+1==numBytesRead)) fprintf(fpAtriEventLog, "\n"); 
              else fprintf(fpAtriEventLog," ");      
	    }
	  fprintf(fpAtriEventLog,"\n");	
// --- xxxx
#if 0
	  for (i=0;i<numBytesRead;i=i+2) 
	    {
	      unsigned int word = buffer[i] | ((buffer[i+1]&0x0f)<<8);
              fprintf(stdout,"%d %d\n", i,word); 
	    }
	  fprintf(stdout,"\n");
#endif
// --- xxxx
	}
      }
      else {
	//	fprintf(stderr,"*");
	return 0;
      }      
    }
    else {
      ARA_LOG_MESSAGE(LOG_ERR,"readEventEndPoint returned %d\n",retVal);
      return -1;
    }
  }
}


int setAllThresholds(int fAtriSockFd, uint16_t *thresholds)
{
  int stack,retVal=0;
  for(stack=D1;stack<=D4;stack++) {  
    fprintf(stderr,"Setting thresholds on stack %s to %d %d %d %d\n",
	    atriDaughterStackStrings[stack],
	    thresholds[(stack*ANTS_PER_TDA)+0],
	    thresholds[(stack*ANTS_PER_TDA)+1],
	    thresholds[(stack*ANTS_PER_TDA)+2],
	    thresholds[(stack*ANTS_PER_TDA)+3]);
    retVal+=setAtriThresholds(fAtriSockFd,stack,&(thresholds[stack*ANTS_PER_TDA]));
  }
  return retVal;
}


int doThresholdScan(int fAtriSockFd)
{
  //First kind of scan will be all channels together
  FILE *fpThresholdScan;
  char filename[FILENAME_MAX];
  uint16_t thresholds[16]={0};
  uint32_t ppsCounter;
  uint16_t l1Scalers[16];
  int value=0,i=0;
  int j=0;
  sprintf(filename,"%s/current/thresholdScan.txt",theConfig.topDataDir);
  fpThresholdScan=fopen(filename,"w");
  for(i=0;i<theConfig.thresholdScanPoints;i++) {
    value=theConfig.thresholdScanStep*i+theConfig.thresholdScanStartPoint;
    for(j=0;j<THRESHOLDS_PER_ATRI;j++) {
      thresholds[j]=value;
    }
    setAllThresholds(fAtriSockFd,thresholds);
    sleep(2);

    //Read out PPS counter
    
    //PPS counter
    atriWishboneRead(fAtriSockFd,ATRI_WISH_PPSCNT,4,(uint8_t*)&ppsCounter);

    //Read out scalers -- not sure this formatting works
    atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_D1_L1_C1,32,(uint8_t*)l1Scalers);

    fprintf(fpThresholdScan,"%u %d ",ppsCounter,value);
    for(j=0;j<THRESHOLDS_PER_ATRI;j++) 
      fprintf(fpThresholdScan,"%d ",l1Scalers[j]);
    fprintf(fpThresholdScan,"\n");
  }
  fclose(fpThresholdScan);
  return 0;
}
