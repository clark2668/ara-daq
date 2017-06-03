/*! \file ARAd.c
  \brief The ARAd program that is repsonsible for starting and stopping runs (and other similar jobs).
    
  This is the updated ARAd program which is actually a daemon.

  May 2011 rjn@hep.ucl.ac.uk
*/
#include "ARAd.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"


#include <signal.h>


//ARAAcqd management
time_t lastRunStarted=0;
time_t nextRunScheduled=0;

//Monitor stuff
time_t lastMonitor=0;

//Socket Handler Thread
pthread_t socket_thread;

//Socket fd
int fAcqdSocketFd;

//Program states
AraProgramStatus_t fAcqdStatus;
int fCurrentRun;
int fExitProgram;


ARAdConfig_t theConfig;

int main(int argc, char *argv[])
{
  printToScreen=8;
  fAcqdSocketFd=0;
  fAcqdStatus=ARA_PROG_NOTASTATUS;
  fExitProgram=0;

  void *status;
  int retVal;
  char* progName = basename(argv[0]);

  
  // Check PID file
  retVal=checkPidFile(ARAD_PID_FILE);
  if(retVal) {
    syslog(LOG_ERR,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,ARAD_PID_FILE);
    exit(1);
  }
  // Write pid
  if( writePidFile(ARAD_PID_FILE) )
    exit(1);


  retVal=readConfigFile(&theConfig);
  if(retVal!=CONFIG_E_OK) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error reading config file %s: bailing\n",ARAD_CONFIG_FILE);
    exit(1);
  }    


  fCurrentRun=getRunNumber(theConfig.topDataDir);


  
  /* Set signal handlers */
  signal(SIGUSR2, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGSEGV, signalHandler);


  //Things that need to be added here
  //a) reading config file
  //b) starting up logging and PID file
  //d) create a thread for the socket listening


  pthread_attr_t attr;
  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  retVal=pthread_create(&socket_thread,&attr,(void*)socketHandler,NULL);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can't make thread");
  }

  retVal=openAcqdRcSocket();			
  if(retVal<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Problem opening connection to ACQD_RC socket\n");
  }
    

  
  //Now wait.
  time_t currentTime;
  int lastAcqdStatus=-777;
  int countAcqdIdle=0;
  int needToStart=0;
  int runsSincePedestalRun=0;
  int countAcqdNotRunning=0;
  int acqdPid=0;
  while(!fExitProgram) {
    needToStart=0;
    currentTime=time(NULL);
    //First up check if we need to get new disk info stuff
    if(lastMonitor+theConfig.monitorPeriod<currentTime) {
      //Need new monitor point
      updateMonitorFile(currentTime);
      lastMonitor=currentTime;
    }


    updateAcqdStatus();
    if(fAcqdSocketFd==0) {
      //Maybe acqd is not running
      acqdPid=checkPidFile(ARA_ACQD_PID_FILE);
      //      fprintf(stderr,"countAcqdNotRunning %d \t startAcqdIfNotRunning %d acqdPid %d\n",countAcqdNotRunning,theConfig.startAcqdIfNotRunning,acqdPid);

      if(acqdPid==0) {
	//Acqd is not running
	//options are do nothing or restart Acqd	
	countAcqdNotRunning++;
	if(countAcqdNotRunning>theConfig.startAcqdIfNotRunning && theConfig.startAcqdIfNotRunning>0) {
	  //Start Acqd
	  acqdPid=startAcqd();
	  ARA_LOG_MESSAGE(LOG_DEBUG,"Acqd not running so will restart, acqdPid=%d\n",acqdPid);	  
	  countAcqdNotRunning=0;
	}
      }
      else {
	ARA_LOG_MESSAGE(LOG_ERR,"Acqd socket not up but Acqd is running\n");
      }
      sleep(1);      
      continue;
    }
    countAcqdNotRunning=0;
    if(fAcqdStatus!=lastAcqdStatus) 
      printf("Current acqdStatus: %d\n",fAcqdStatus);
    lastAcqdStatus=fAcqdStatus;
    
    if(fAcqdStatus==ARA_PROG_IDLE) {
      countAcqdIdle++;
    }
    else {
      countAcqdIdle=0;
    }
    
    if(countAcqdIdle>theConfig.startRunIfAcqdIdle && theConfig.startRunIfAcqdIdle) {
      ARA_LOG_MESSAGE(LOG_DEBUG,"Acqd in idle mode for %d seconds will start run\n",countAcqdIdle);
      needToStart=1;
    }
    
    if(theConfig.restartRunsEvery>0) {
      if(currentTime>nextRunScheduled  && (nextRunScheduled>0 || theConfig.startRunOnStart)) {
	needToStart=1;
      }      
    }

    if(needToStart) {
      countAcqdIdle=0;
      retVal=stopRun("Stopped by ARAd");
      if(retVal==0) {
	if(runsSincePedestalRun>=theConfig.pedestalRunEveryNRuns) {
	  retVal=startNewRun("Timed start of pedestal run by ARAd",ARA_RUN_TYPE_PEDESTAL);
	  runsSincePedestalRun=0;
	}
	else {
	  retVal=startNewRun("Timed start by ARAd",ARA_RUN_TYPE_NORMAL);
	  runsSincePedestalRun++;
	}
	if(retVal!=0) {
	  ARA_LOG_MESSAGE(LOG_ERR,"Error starting run\n");
	}
      }
      else {
	ARA_LOG_MESSAGE(LOG_ERR,"Error stopping run\n");
      }    
    }


    sleep(1);
  }


  if(fAcqdSocketFd) close(fAcqdSocketFd);


  
  /* Free attribute and wait for the other threads */
  pthread_attr_destroy(&attr);
  retVal = pthread_join(socket_thread,&status);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can't join socket_thread -- %d\n",retVal);
  }
  unlink(ARAD_PID_FILE);
  return 0; 
}




int socketHandler() {
  int sockfd, newsockfd, portno,retVal;
     socklen_t clilen;
     AraRunControlMessageStructure_t rcMsg;
     AraRunControlMessageStructure_t rcReply;
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     struct pollfd fds[1];
     int nfds,pollVal;

     //Open a socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        ARA_LOG_MESSAGE(LOG_ERR,"ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 9009;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     /*
       just set the re-use address option
      */

     int opt=1; 
     if (setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR,(const char *)&opt, sizeof(int)) < 0){ 
       ARA_LOG_MESSAGE(LOG_ERR,"%s -- ERROR on setting socket option -- %s\n", __FUNCTION__, strerror(errno)); 
     } 

     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) {
       
       ARA_LOG_MESSAGE(LOG_ERR,"%s -- ERROR on binding -- %s\n", __FUNCTION__, strerror(errno));
       
     }

     listen(sockfd,5);
     clilen = sizeof(cli_addr);     
     


     //Now listen for clients
     while(!fExitProgram) {
       fds[0].fd=sockfd;
       fds[0].events=POLLIN;
       fds[0].revents=0;
       nfds=1;
       
       
       pollVal=poll(fds,nfds,1);
       if(pollVal==0) continue; ///All fine
       if(pollVal==-1) {
	 ARA_LOG_MESSAGE(LOG_ERR,"Error calling poll %s\n",strerror(errno));
	 close(sockfd);
	 break;
       }
       
       if (fds[0].revents & POLLIN) {
	 
	 //Now accept a new connection
	 newsockfd = accept(sockfd, 
			    (struct sockaddr *) &cli_addr, 
			    &clilen);
	 if (newsockfd < 0) 
	   ARA_LOG_MESSAGE(LOG_ERR,"ERROR on accept");
	 
	 //Read stuff from socket
	 fprintf(stderr,"Reading\n");
	 n = read(newsockfd,(char*)&rcMsg,sizeof(AraRunControlMessageStructure_t));
	 if (n < 0) ARA_LOG_MESSAGE(LOG_ERR,"ERROR reading from socket");
	 
	 //should probably check that it is the correct length before passing it on
	 retVal=handleMessage(&rcMsg,&rcReply);
       
	 //Send reply
	 n = write(newsockfd,(char*)&rcReply,sizeof(AraRunControlMessageStructure_t));
	 if (n < 0) ARA_LOG_MESSAGE(LOG_ERR,"ERROR writing to socket");
	 
	 //Close client socket
	 close(newsockfd);
       }     
     }
     close(sockfd);
     pthread_exit(NULL);
}

int handleMessage(AraRunControlMessageStructure_t *message, AraRunControlMessageStructure_t *reply)
{
  //Do something
  if(checkRunControlMessage(message)) {    
    //Message is valid, should do something
    ARA_LOG_MESSAGE(LOG_INFO,"Got RC Message: %s\n",getRunControlTypeAsString(message->type));
    
    reply->from=ARA_LOC_ARAD_ARA1;
    reply->to=message->from;
    reply->type=ARA_RC_RESPONSE;
    reply->runType=message->runType;
    
    switch(message->type) {
    case ARA_RC_QUERY_STATUS:
      updateAcqdStatus();
      break;
    case ARA_RC_START_RUN:
      startNewRun((char*)message->params,message->runType);
      break;
    case ARA_RC_STOP_RUN:
      stopRun((char*)message->params);
      break;
    default:
      updateAcqdStatus();
      break;      
    }
    sprintf((char*)reply->params,"%s -- acqdFd=%d acqdStatus=%d run=%d",getRunControlTypeAsString(message->type),fAcqdSocketFd,fAcqdStatus,fCurrentRun);
      

    reply->length=strlen((char*)reply->params);
    setChecksum(reply);
  }
  else {
    ARA_LOG_MESSAGE(LOG_ERR,"Got malformed RC message: %s\n",getRunControlTypeAsString(message->type));    
  }
  return 0;
}

uint32_t getNewRunNumber(char *baseDir) {
  static int firstTime=1;
  static uint32_t runNumber=0;
  int retVal=0;
  /* This is just to get the lastRunNumber in case of program restart. */
  if(firstTime) {
    runNumber=getRunNumber(baseDir);
  }
  runNumber++;
  fprintf(stderr,"getNewRunNumber -- %u\n",runNumber);

  char runFileName[FILENAME_MAX];
  sprintf(runFileName, "%s/../%s", baseDir, ARA_RUN_NUMBER_FILE);

  FILE *pFile = fopen (runFileName, "w");
  if(pFile == NULL) {
    ARA_LOG_MESSAGE (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
            runFileName);
  }
  else {
    retVal=fprintf(pFile,"%u\n",runNumber);
    if(retVal<0) {
      ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
              runFileName);
    }
    fclose (pFile);
  }
  return runNumber;
    
}

int openAcqdRcSocket()
{
  int len;
  struct sockaddr_un remote;
  static int lastRetVal=-7;
  if(fAcqdSocketFd) return 0;
  
  if ((fAcqdSocketFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    if(lastRetVal!=-1)
      ARA_LOG_MESSAGE(LOG_ERR,"socket -- %s\n",strerror(errno));
    lastRetVal=-1;
    return -1;
  }
  
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"Trying to connect to %s...\n",ACQD_RC_SOCKET);
  
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, ACQD_RC_SOCKET);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(fAcqdSocketFd, (struct sockaddr *)&remote, len) == -1) {
    if(lastRetVal!=-2)
      ARA_LOG_MESSAGE(LOG_ERR,"connect --%s\n",strerror(errno));
    close(fAcqdSocketFd);
    fAcqdSocketFd=0;
    lastRetVal=-2;
    return -2;
  }
  
  ARA_LOG_MESSAGE(LOG_DEBUG,"Connected.\n");
  return 0;
}

int sendCommandToAcqd(AraProgramControl_t requestedState, AraProgramStatus_t desiredState, uint32_t runNumber,AraRunType_t runType) { 
  int numBytes;
  AraProgramCommand_t progCommand;
  AraProgramCommandResponse_t progResponse;
  
  checkAcqdSocket();
  if(fAcqdSocketFd==0) return -1;

  //Fill AraProgramCommand_t
  progCommand.feWord=0xfefefefe;
  progCommand.controlState=requestedState;
  progCommand.runNumber=runNumber;
  progCommand.efWord=0xefefefef;
  progCommand.runType=runType;

  numBytes=write(fAcqdSocketFd,(char*)&progCommand,sizeof(AraProgramCommand_t));
  if(numBytes<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error sending message to ACQD_RC_SOCKET\n");
    return -1;
  }

  numBytes=read(fAcqdSocketFd,(char*)&progResponse,sizeof(AraProgramCommandResponse_t));
  if(numBytes<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error reading message from ACQD_RC_SOCKET\n");
    return -1;
  }
  fAcqdStatus=progResponse.currentStatus;
  int lastStatus=0;
  int firstTime=1;
  while(progResponse.currentStatus!=desiredState) {
    if(firstTime || lastStatus!=progResponse.currentStatus) 
      ARA_LOG_MESSAGE(LOG_INFO,"Current ACQD Status = %d, Desired Status = %d\n",progResponse.currentStatus,desiredState);    
    firstTime=0;
    lastStatus=progResponse.currentStatus;
    numBytes=read(fAcqdSocketFd,(char*)&progResponse,sizeof(AraProgramCommandResponse_t));
    if(numBytes<0) {
      ARA_LOG_MESSAGE(LOG_ERR,"Error reading message from ACQD_RC_SOCKET\n");
      return -1;
    }
    fAcqdStatus=progResponse.currentStatus;
  }
  
  return 0;
}



int prepareNewRun(char *logMessage)
{

  updateAcqdStatus();
  if(fAcqdStatus==ARA_PROG_IDLE || fAcqdStatus==ARA_PROG_NOTASTATUS) {
    fCurrentRun=getNewRunNumber(theConfig.topDataDir);
    makeNewRunDirs(logMessage);
    return sendCommandToAcqd(ARA_PROG_CONTROL_PREPARE,ARA_PROG_PREPARED,fCurrentRun,ARA_RUN_TYPE_NORMAL);  
  }
  return 0;
  
}

int startNewRun(char *logMessage, AraRunType_t runType)
{
  updateAcqdStatus();
  if(fAcqdStatus==ARA_PROG_IDLE|| fAcqdStatus==ARA_PROG_NOTASTATUS) {
    fCurrentRun=getNewRunNumber(theConfig.topDataDir);
    makeNewRunDirs(logMessage);
  }
  int retVal=sendCommandToAcqd(ARA_PROG_CONTROL_START,ARA_PROG_RUNNING,fCurrentRun,runType);
  if(retVal==0) {
    lastRunStarted=time(NULL);
    if(theConfig.restartRunsEvery>0) {
      nextRunScheduled=lastRunStarted+theConfig.restartRunsEvery;
      fprintf(stderr,"Started run %d at %lu, next run scheduled for %lu\n",
	      fCurrentRun,lastRunStarted,nextRunScheduled);
    }
    char filename[FILENAME_MAX];
    sprintf(filename,"%s/runStart.run%6.6d.dat",theConfig.runLogDir,fCurrentRun);   
    FILE *pFile = fopen (filename, "w");
    if(pFile == NULL) {
      ARA_LOG_MESSAGE (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		       filename);
    }
    else {
      retVal=fprintf(pFile,"Run: %u\n",fCurrentRun);    
      if(retVal<0) {
	ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
			 filename);
      }
      retVal=fprintf(pFile,"Time: %lu\n",time(NULL));    
      if(retVal<0) {
	ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
			 filename);
      }
      retVal=fprintf(pFile,"Message: %s\n",logMessage);    
      if(retVal<0) {
	ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
			 filename);
      }
      retVal=fclose (pFile);
    }
  }
  return retVal;
}

int stopRun(char *logMessage)
{ 

  char filename[FILENAME_MAX];
  int retVal=0;
  sprintf(filename,"%s/runStop.run%6.6d.dat",theConfig.runLogDir,fCurrentRun);   
  FILE *pFile = fopen (filename, "w");
  if(pFile == NULL) {
    ARA_LOG_MESSAGE (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
            filename);
  }
  else {
    retVal=fprintf(pFile,"Run: %u\n",fCurrentRun);    
    if(retVal<0) {
      ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
              filename);
    }
    retVal=fprintf(pFile,"Time: %lu\n",time(NULL));    
    if(retVal<0) {
      ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
              filename);
    }
    retVal=fprintf(pFile,"Message: %s\n",logMessage);    
    if(retVal<0) {
      ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		       filename);
    }
    fclose (pFile);
    if(theConfig.linkForXfer)
      makeLink(filename,theConfig.linkDir);

  }

  return sendCommandToAcqd(ARA_PROG_CONTROL_STOP,ARA_PROG_IDLE,fCurrentRun,ARA_RUN_TYPE_NA);
}


int terminateProgram()
{ 
  return sendCommandToAcqd(ARA_PROG_CONTROL_TERMINATE,ARA_PROG_TERMINATE,fCurrentRun,ARA_RUN_TYPE_NA);
}

void updateAcqdStatus() 
{
  static int lastStatus=1;
  checkAcqdSocket();
  if(fAcqdSocketFd==0) {
    if(lastStatus!=fAcqdSocketFd) 
      ARA_LOG_MESSAGE(LOG_ERR,"fAcqdSocketFd==%d\n",fAcqdSocketFd);
    lastStatus=0;
    return;
  }
  
  int numBytes;
  AraProgramCommand_t progCommand;
  AraProgramCommandResponse_t progResponse;
  
  //Fill AraProgramCommand_t
  progCommand.feWord=0xfefefefe;
  progCommand.controlState=ARA_PROG_CONTROL_QUERY;
  progCommand.runNumber=fCurrentRun;
  progCommand.efWord=0xefefefef;
  
  //Somehow need to check the scoket is still alive
  numBytes=write(fAcqdSocketFd,(char*)&progCommand,sizeof(AraProgramCommand_t));
  if(numBytes<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error sending message to ACQD_RC_SOCKET\n");
    return;
  }

  numBytes=read(fAcqdSocketFd,(char*)&progResponse,sizeof(AraProgramCommandResponse_t));
  if(numBytes<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error reading message from ACQD_RC_SOCKET\n");
    return;
  }
  fAcqdStatus=progResponse.currentStatus;
}

void checkAcqdSocket() {
  struct pollfd fds[1];
  int nfds;
  int pollVal;
  if(fAcqdSocketFd) {
    
    fds[0].fd = fAcqdSocketFd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    nfds=1;
    
    pollVal=poll(fds,nfds,1);
    if(pollVal==0) return; ///All fine
    if(pollVal==-1) {
      ARA_LOG_MESSAGE(LOG_ERR,"Error calling poll: %s",strerror(errno));
      close(fAcqdSocketFd);
      fAcqdSocketFd=0;
      return;
    }
    if(fds[0].revents & POLLHUP) {
      fprintf(stderr,"Connection closed... what to do now..\n");
      close(fAcqdSocketFd);
      fAcqdSocketFd=0;
    }
  }
  if(fAcqdSocketFd==0) {
    openAcqdRcSocket();
  }
}


void signalHandler(int sig) {
  ARA_LOG_MESSAGE(LOG_ERR,"ARAd got signal %d\n",sig);
  fExitProgram=1;

}


int makeNewRunDirs(char *logMessage) 
{
  fprintf(stderr,"makeNewRunDirs\n");
  char filename[FILENAME_MAX];
  char newRunDir[FILENAME_MAX];
  char currentDirLink[FILENAME_MAX];
  char mistakeDirName[FILENAME_MAX];
  int retVal=0;

  sprintf(newRunDir,"%s/run_%06d",theConfig.topDataDir,fCurrentRun);
  sprintf(currentDirLink,"%s/current",theConfig.topDataDir);

  fprintf(stderr,"newRunDir -- %s\n",newRunDir);
  fprintf(stderr,"currentDirLink -- %s\n",currentDirLink);

  makeDirectories(newRunDir);
  removeFile(currentDirLink);
  if(is_dir(currentDirLink)) {
    sprintf(mistakeDirName,"%s/run%06d.current",theConfig.topDataDir,fCurrentRun-1);
    rename(currentDirLink,mistakeDirName);	   
  }
  symlink(newRunDir,currentDirLink);
  makeDirectories(theConfig.eventTopDir);
  makeDirectories(theConfig.eventHkTopDir);
  makeDirectories(theConfig.sensorHkTopDir);
  makeDirectories(theConfig.monitorHkTopDir);
  makeDirectories(theConfig.runLogDir);
  
  sprintf(filename,"%s/runStart.run%6.6d.dat",theConfig.runLogDir,fCurrentRun);   
  FILE *pFile = fopen (filename, "w");
  if(pFile == NULL) {
    ARA_LOG_MESSAGE (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
            filename);
  }
  else {
    retVal=fprintf(pFile,"Run: %u\n",fCurrentRun);    
    if(retVal<0) {
      ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
              filename);
    }
    retVal=fprintf(pFile,"Time: %lu\n",time(NULL));    
    if(retVal<0) {
      ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
              filename);
    }
    retVal=fprintf(pFile,"Message: %s\n",logMessage);    
    if(retVal<0) {
      ARA_LOG_MESSAGE (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		       filename);
    }
    fclose (pFile);
    if(theConfig.linkForXfer)
      makeLink(filename,theConfig.linkDir);
  }


  return retVal;
}



int readConfigFile(ARAdConfig_t *theConfigPtr)
{

  int allStatus = 0;
  //  KvpErrorCode kvpStatus = 0;

  kvpReset();
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"Here %s\n",ARAD_CONFIG_FILE);
  allStatus = configLoad(ARAD_CONFIG_FILE,"output");
  allStatus = configLoad(ARAD_CONFIG_FILE,"log");
  allStatus = configLoad(ARAD_CONFIG_FILE,"runControl");
  if(allStatus != CONFIG_E_OK) {
    ARA_LOG_MESSAGE(LOG_ERR,"Problem reading %s -- %s\n",ARAD_CONFIG_FILE,"output");
  }
  
#define SET_INT(x,d)    theConfigPtr->x=kvpGetInt(#x,d);
#define SET_FLOAT(x,f)  theConfigPtr->x=kvpGetFloat(#x,f);
#define SET_STRING(x,s) {char* tmpStr=kvpGetString(#x);strcpy(theConfigPtr->x,tmpStr?tmpStr:s);}

  if( allStatus == CONFIG_E_OK) {
    // Get logging 
    SET_INT(printToScreen,0);
    SET_INT(logLevel,0);
    // Get data directories
    SET_STRING(topDataDir,".");
    sprintf(theConfigPtr->eventTopDir,"%s/current/%s",theConfigPtr->topDataDir,DAQ_EVENT_DIR);
    sprintf(theConfigPtr->eventHkTopDir,"%s/current/%s",theConfigPtr->topDataDir,DAQ_EVENT_HK_DIR);
    sprintf(theConfigPtr->sensorHkTopDir,"%s/current/%s",theConfigPtr->topDataDir,DAQ_SENSOR_HK_DIR);
    sprintf(theConfigPtr->monitorHkTopDir,"%s/current/%s",theConfigPtr->topDataDir,MONITOR_HK_DIR);
    sprintf(theConfigPtr->pedsTopDir,"%s/current/%s",theConfigPtr->topDataDir,DAQ_PED_DIR);
    sprintf(theConfigPtr->runLogDir,"%s/current/%s",theConfigPtr->topDataDir,DAQ_RUNLOG_DIR);
    SET_STRING(runNumFile,"run_number");
    SET_STRING(linkDir,"./link");
    SET_INT(linkForXfer,0);
    SET_INT(filesPerDir,100);
    SET_INT(eventsPerFile,100);
    SET_INT(hkPerFile,1000);
    SET_INT(compressionLevel,5);
    SET_INT(monitorPeriod,60);
    // Get runControl
    SET_INT(startRunOnStart,1);
    SET_INT(restartRunsEvery,3600);
    SET_INT(pedestalRunEveryNRuns,4);
    SET_INT(startRunIfAcqdIdle,60);
    SET_INT(startAcqdIfNotRunning,120);
  }

  return allStatus;

}


int startAcqd() {
  int retVal;
  retVal=system("bash ~/WorkingDAQ/scripts/startARAAcqd.sh");
  sleep(2);
  return checkPidFile(ARA_ACQD_PID_FILE);
}

#define NUM_DISKS 5
void updateMonitorFile(time_t currentTime)
{
  int diskNum=0;
  int firstTime=0;
  //  char *diskLocations[NUM_DISKS]={"/","/tmp","/var/run","/home","/data"};
  char *diskLocations[NUM_DISKS]={"/","/tmp","/var/run","/home",theConfig.topDataDir};
  char filename[FILENAME_MAX];


  sprintf(filename,"%s/monitor.run%6.6d.dat",theConfig.monitorHkTopDir,fCurrentRun);   
  FILE *pFile = fopen (filename, "r");
  if(pFile == NULL) {
    //File does not exist
    pFile = fopen (filename, "w");
    firstTime=1;
  }
  else {
    fclose(pFile);
    pFile = fopen (filename, "a");
  }
  if(pFile == NULL) {	
    ARA_LOG_MESSAGE (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		     filename);
  }
  else {
    if(firstTime) {
      fprintf(pFile,"time\t");
      for(diskNum=0;diskNum<NUM_DISKS;diskNum++) {
	fprintf(pFile,"%s\t",diskLocations[diskNum]);
      }
      fprintf(pFile,"(MB)\n");
    }
    fprintf(pFile,"%d\t",(int)currentTime);
    for(diskNum=0;diskNum<NUM_DISKS;diskNum++) {
      fprintf(pFile,"%d\t",getDiskSpace(diskLocations[diskNum]));
    }
    fprintf(pFile,"\n");        
    fclose(pFile);
  }  
}
