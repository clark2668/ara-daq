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
#include "araRunLogLib/araRunLogLib.h"
#include "araAtriStructures.h"
#include "calpulserLib/calpulser.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <libgen.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <poll.h>
#include <math.h>


//Global Variables
//#define NO_USB 0
//#define CLOCK_DEBUG 1

#define MAX_SOFT_TRIG_RATE 50
#define MAX_RAND_TRIG_RATE MAX_SOFT_TRIG_RATE
#define MAX_EVENT_RATE_REPORT_RATE 1

// Writer objects are global, so they can be accessed by signal handler
ARAWriterStruct_t eventHkWriter;
ARAWriterStruct_t sensorHkWriter;
ARAWriterStruct_t eventWriter;


//Threads
pthread_t fRunControlSocketThread;
pthread_t fAtriControlSocketThread;
pthread_t fFx2ControlUsbThread;
pthread_t fAraHkThread;
pthread_t fLibusbPollThread;

//Event loop variables
int fHkThreadPrepared=0;
int fHkThreadStopped=0;
AraProgramStatus_t fProgramState;
int32_t fInPedestalMode=0;
int32_t fCurrentRun;
int32_t fCurrentEvent;
int32_t fLastEventCount;
int32_t fCurrentPps;

int32_t numGoodEvents=0;
int32_t numBadEvents=0;

//servo variables
uint16_t currentVdly[DDA_PER_ATRI];
uint16_t currentVadj[DDA_PER_ATRI];
uint16_t currentIsel[DDA_PER_ATRI];
uint16_t currentVbias[DDA_PER_ATRI];


uint16_t currentThresholds[NUM_L1_SCALERS];
uint16_t currentSurfaceThresholds[ANTS_PER_TDA];

//Config is global so other threads can read it
ARAAcqdConfig_t theConfig;

//RunLog global variable for recording of runInfo
ARAacqdRunInfo_t runInfo;

//libusb mutex
pthread_mutex_t libusb_command_mutex;


FILE *fpAtriEventLog=NULL;

AraStationEventHeader_t *fEventHeader;
unsigned char *fEventReadBuffer;
unsigned char *fEventWriteBuffer;

//Stuff for readAtrEvetV2
int minimumBlockSize=4100;

///Stuff for pretty temp file
time_t lastSoftwareTrigger;
time_t lastEventRead;

#define MAX_EVENT_BUFFER_SIZE 200000


#define CONDITION_MET( yes, nt, tt ) ( yes && ( nt.tv_sec > tt.tv_sec || ( nt.tv_sec == tt.tv_sec && nt.tv_usec >= tt.tv_usec ) ) )

void ARAAcqdLogger(int debug_level, char *fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  ARA_VLOG_MESSAGE(debug_level, fmt, argptr);
  va_end(argptr);
}

int main(int argc, char *argv[])
{
  char filename[FILENAME_MAX];
  fEventReadBuffer = malloc(MAX_EVENT_BUFFER_SIZE);
  fEventWriteBuffer = malloc(MAX_EVENT_BUFFER_SIZE);
  fEventHeader = (AraStationEventHeader_t*)fEventWriteBuffer;


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

  // Time values
  struct timeval nowTime;
  struct timeval nextPrintTrig, nextPrintStep;
  struct timeval nextPrintEvent, nextPrintEventStep;
  struct timeval nextSoftTrig,nextRandTrig,nextEventRateReport;//,nextHkRead,nextServoCalc;
  struct timeval nextSoftStep,nextRandStep,nextEventRateReportStep;//,nextHkStep,nextServoStep;
  unsigned int softTrigCount = 0;

  // Check PID file
  retVal=checkPidFile(ARA_ACQD_PID_FILE);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,ARA_ACQD_PID_FILE);
    exit(1);
  }
  // Write pid
  if( writePidFile(ARA_ACQD_PID_FILE) ) {
    
    exit(1);
  }
  /* Set signal handlers */
  signal(SIGUSR2, sigUsr2Handler);
  signal(SIGTERM, sigUsr2Handler);
  signal(SIGINT, sigUsr2Handler);
   
  calpulser_SetLogger(ARAAcqdLogger);

  // Load configuration
  retVal=readConfigFile(ARA_ACQD_CONFIG_FILE,&theConfig);
  if(retVal!=CONFIG_E_OK) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error reading config file %s: bailing\n",configFileName);
    //    fprintf(stderr,"Arrrgghh\n");
    //    fprintf(stderr,"Syntax error in config file\n");
    exit(1);
  }    

  printToScreen=theConfig.printToScreen;
  printf("ARAAcqd: Output verbosity level: %d\n", printToScreen);
   
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

  ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: This is ARAAcqd, version %d.%d.%d\n",
		  ARAACQD_VER_MAJOR,
		  ARAACQD_VER_MINOR,
		  ARAACQD_VER_REV);
  
  //Will probably move the opending of te USB connection into the main thread
  retVal=openFx2Device();
  ARA_LOG_MESSAGE(LOG_DEBUG, "ARAAcqd: Flushing control endpoint\n");
  ///Need to check the return for when we can't talk to the ATRI board
  retVal=flushControlEndPoint();

  // Are we reading event data from the PCIe device?
  if (theConfig.enablePcieReadout) {
      ARA_LOG_MESSAGE(LOG_DEBUG, "ARAAcqd: opening PCIe device\n");      
      enablePcieEndPoint();
      retVal=openPcieDevice();
      if (retVal) {
          ARA_LOG_MESSAGE(LOG_ERR,"Couldn't open PCIe device\n");
      }
  }
  else {
      disablePcieEndPoint();
  }
  
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


//Now make the libusbPollThreadHandler
//  retVal=pthread_create(&fLibusbPollThread,&attr,libusbPollThreadHandler,NULL);
  //  if(retVal) {
    //    ARA_LOG_MESSAGE(LOG_ERR,"Can't make Libusb poll thread");
    //  }

  sleep(1);


  fMainThreadFx2SockFd=openConnectionToFx2ControlSocket();
  if(!fMainThreadFx2SockFd) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can not open connection to FX2_CONTROL\n");
    return -1;
  }
  else {
    ARA_LOG_MESSAGE(LOG_DEBUG,"fMainThreadFx2SockFd=%d\n",fMainThreadFx2SockFd);
  }
	

  fMainThreadAtriSockFd=openConnectionToAtriControlSocket();
  if(!fMainThreadAtriSockFd) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can not open connection to ATRI_CONTROL\n");
    return -1;
  }
  else {
    ARA_LOG_MESSAGE(LOG_DEBUG,"fMainThreadAtriSockFd=%d\n",fMainThreadAtriSockFd);
  }



  retVal=pthread_create(&fAraHkThread,&attr,araHkThreadHandler,NULL);
  if (retVal!=0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can't make ARA Hk Thread\n");
  }

  {
    unsigned short fx2Version = 0;
    int rv;
    rv = getFx2Version(fMainThreadFx2SockFd, &fx2Version);
    if (rv < 0) {
      ARA_LOG_MESSAGE(LOG_ERR, "Error %d requesting FX2 Version.\n",
		      rv);
    } else {
      ARA_LOG_MESSAGE(LOG_INFO,"ARAAcqd: FX2 firmware version %d.%d\n",
		      (fx2Version & 0xFF00)>>8, (fx2Version & 0xFF));
    }
    ARA_LOG_MESSAGE(LOG_INFO,"ARAAcqd: Pulling FPGA out of reset.\n");
    fflush(stdout);
    // Pull the FPGA out of reset (or issue a reset)
    atriReset(fMainThreadFx2SockFd);

    // Print out ID. This should be in its own function or something.
    uint8_t idver_data[8];
    atriWishboneRead(fMainThreadAtriSockFd,ATRI_WISH_IDREG,8,idver_data);
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: This is an %c%c%c%c v%d, v%d.%d.%d comp. %d/%d\n",
		    idver_data[3],idver_data[2],idver_data[1],idver_data[0],
		    (idver_data[7] >> 4),
		    (idver_data[5] >> 4),
		    (idver_data[5] & 0x0F),
		    (idver_data[4]),
		    (idver_data[7] & 0x0F),
		    (idver_data[6]));
    
    ///RJN Hack
    if(idver_data[3]=='M' && idver_data[2]=='A' && idver_data[1]=='T' && idver_data[0]=='R') {
      minimumBlockSize=1034;
    }
    char firmware_version[10];
    sprintf(firmware_version, "v%d.%d.%d",    
	    idver_data[5] >> 4,
	    idver_data[5] & 0x0F,
	    idver_data[4]);
    latchFirmwareVersion( &(runInfo) , firmware_version );
    
  }
  sleep(1);

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
      // Debugging: Dump the cal pulser configuration.
      calpulser_DumpFullConfig();

      if(theConfig.pedestalMode || fInPedestalMode) {

	theConfig.enableRF0Trigger=0;
	theConfig.enableRF1Trigger=0;
	theConfig.enableCalTrigger=0;
	theConfig.enableSoftTrigger=1;
	theConfig.enableExtTrigger=0;

	theConfig.numSoftTriggerBlocks=theConfig.pedestalNumTriggerBlocks;
  
	theConfig.useGlobalThreshold=1;
	theConfig.globalThreshold=0;
	theConfig.enableVdlyServo=0;
      }

      // Apply cal pulser configuration. Once this works, I'll have it
      // check last known calpulser configuration, and if nothing has changed,
      // it'll just return.
      calpulser_ApplyConfig(fMainThreadFx2SockFd);
       
      // Main ATRI initialization: turn on daughterboards, etc.

#ifndef NO_USB
      if(theConfig.doAtriInitialisation) initialiseAtri(fMainThreadAtriSockFd,fMainThreadFx2SockFd,&theConfig);
#endif
      
      // Now reset everything.
  
      // Reset trigger/digitizer subsystem.
      ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Resetting digitizer/triggering subsystem.\n");
      
      resetAtriTriggerAndDigitizer(fMainThreadAtriSockFd);


      // Flush event end point. The entire event path should be clear now.
      retVal=flushEventEndPoint();

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

      // Set how often we're going to print out events.
      setTimeInterval(&nextPrintEventStep, 5.0);
      setNextTime(&nextPrintEvent, &nowTime, &nextPrintEventStep);


      // Enable soft triggering. Also set up how often we're going to
      // actually *output* that we're soft triggering.
      if(  theConfig.enableSoftTrigger 
	   && theConfig.softTriggerRateHz > 0.0
	   && theConfig.softTriggerRateHz < MAX_SOFT_TRIG_RATE ){
	setTimeInterval(&nextSoftStep, 1.0/theConfig.softTriggerRateHz);
	setNextTime(&nextSoftTrig,&nowTime,&nextSoftStep);
	// Only output once every 5 seconds maximum.
	setTimeInterval(&nextPrintStep, 5.0);
	setNextTime(&nextPrintTrig,&nowTime,&nextPrintStep);
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
      if(theConfig.enableEventRateReport
	 &&theConfig.eventRateReportRateHz > 0
	 &&theConfig.eventRateReportRateHz < MAX_EVENT_RATE_REPORT_RATE){
	setTimeInterval(&nextEventRateReportStep, 1.0/theConfig.eventRateReportRateHz);
	setNextTime(&nextEventRateReport, &nowTime, &nextEventRateReportStep);
      }
      else{
	if(theConfig.enableEventRateReport)
	  ARA_LOG_MESSAGE(LOG_WARNING, "Invalid event rate report rate. Disabling event rate reporting\n");
	theConfig.enableEventRateReport = 0;
      }
      if(theConfig.setThreshold) {
	setIceThresholds(fMainThreadAtriSockFd,theConfig.thresholdValues);
	if(theConfig.setSurfaceThreshold) 
	  setSurfaceThresholds(fMainThreadAtriSockFd,theConfig.surfaceThresholdValues);
      }
      

      while(!fHkThreadPrepared && fProgramState==ARA_PROG_PREPARING) usleep(1000);

      /////
      // PSA: I don't understand this here: I think this should go in a state
      //      prior to RUNNING. We don't want to enable the digitizer/
      //      trigger until we're ready to catch data flying at us.
      ///// 

      char RunComment[180];
      sprintf(RunComment,"First Attempt at runInfo JPD");
      recordStartDAQ( &(runInfo) , fCurrentRun, RunComment, theConfig.runLogDir, theConfig.topDataDir); 
      latchDAQConfiguration( &(runInfo) ,   
			     theConfig.enableRF0Trigger , theConfig.enableSoftTrigger , theConfig.enableRandTrigger , theConfig.pedestalMode||fInPedestalMode ,  
			     theConfig.softTriggerRateHz , theConfig.randTriggerRateHz ,  
			     theConfig.thresholdValues[0]//FIXME -- which threshold?  
			     );  

      char runLogConfigFile[100];
      char configFileFullPath[100];
      char* configPath = 0 ;
      configPath = getenv ("ARA_CONFIG_DIR");
      sprintf(configFileFullPath, "%s/%s", configPath, configFileName);
      sprintf(runLogConfigFile, "%s/configFile.run%6.6d.dat", theConfig.runLogDir, fCurrentRun);
      retVal = copyFileToFile(configFileFullPath, runLogConfigFile);

      ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling digitizer subsystem.\n");
      setIRSEnable(fMainThreadAtriSockFd, 1);      

      if(theConfig.enableTriggerDelays==1){
	atriSetTriggerDelays(fMainThreadAtriSockFd, theConfig.triggerDelays);
      }
      if(theConfig.enableTriggerWindow==1){
	atriSetTriggerWindow(fMainThreadAtriSockFd, theConfig.triggerWindowSize);
      }

      //FIXME -- set the number of trigger blocks for each trigger type
      initialiseTriggers(fMainThreadAtriSockFd);
      

      fProgramState=ARA_PROG_PREPARED;
    case ARA_PROG_PREPARED:
      fCurrentEvent=0;
      fLastEventCount = 0;
      sleep(5);
      break; // Don't do anything just wait
    case ARA_PROG_RUNNING:
      // Main event loop
      ARA_LOG_MESSAGE(LOG_DEBUG,"Entering running state.\n");
      if(theConfig.pedestalMode || fInPedestalMode) {
	ARA_LOG_MESSAGE(LOG_DEBUG,"Running in pedestal mode!\n");
	theConfig.enableAntServo=0;
	int retVal = doPedestalRunNotPedestalMode(fMainThreadAtriSockFd,fMainThreadFx2SockFd);
	if(retVal<0){
	  fprintf(stderr, "%s: pedestalRun failed!\n", __FUNCTION__);
	  fProgramState=ARA_PROG_TERMINATE;
	}
	fProgramState=ARA_PROG_STOPPING;
	//Toggle out of pedestal mode
	fInPedestalMode=0;
	break;
      }
      else if(theConfig.thresholdScan) {
	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Threshold scan.\n");
	theConfig.enableAntServo=0;

      	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Disabling Triggers.\n");
	uint8_t trigMask=0xff;
	setTriggerL4Mask(fMainThreadAtriSockFd, trigMask);

	theConfig.enableVdlyServo=0;
	doThresholdScan(fMainThreadAtriSockFd);
	fProgramState=ARA_PROG_STOPPING;
	break;
      }
      else if(theConfig.thresholdScanSingleChannel){
	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Threshold scan single channel, channel %d.\n", theConfig.thresholdScanSingleChannelNum);
	theConfig.enableAntServo=0;

      	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Disabling Triggers.\n");
	uint8_t trigMask=0xff;
	setTriggerL4Mask(fMainThreadAtriSockFd, trigMask);

	doThresholdScanSingleChannel(fMainThreadAtriSockFd);
	fProgramState=ARA_PROG_STOPPING;
	break;
      }
      else if(theConfig.vdlyScan) {
	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Vdly scan.\n");
	theConfig.enableAntServo=0;
	theConfig.enableVdlyServo=0;


      	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Disabling Triggers.\n");
	uint8_t trigMask=0xff;
	setTriggerL4Mask(fMainThreadAtriSockFd, trigMask);

	doVdlyScan(fMainThreadAtriSockFd);
	fProgramState=ARA_PROG_STOPPING;
	break;
      }


      gettimeofday(&nowTime,NULL);


      numBytesRead=0;

      retVal = readAtriEventV2(fEventReadBuffer);
      if(retVal>0){
	ARA_LOG_MESSAGE(LOG_DEBUG, "We are reading %d bytes of event %d at %ld and %ld\n", retVal, fCurrentEvent, nowTime.tv_sec, nowTime.tv_usec);  //FIXME: Added this line: FIXED: Changed to LOG_DEBUG
        retVal = unpackAtriEventV2(fEventReadBuffer, fEventWriteBuffer,retVal); 
	if(retVal>0){
	  numBytesRead=retVal;
	  time(&lastEventRead); ///Set last event read time
	  fEventHeader->unixTime=nowTime.tv_sec;
	  fEventHeader->unixTimeUs=nowTime.tv_usec;
	  fEventHeader->eventNumber=fCurrentEvent;
	  fCurrentEvent++;
	  
	  //	  fEventHeader->ppsNumber=fCurrentPps;
	  fillGenericHeader(fEventHeader, ARA_EVENT_TYPE, numBytesRead);
	  
	  //Store Event
	  int new_file_flag = 0;
	  retVal = writeBuffer( &eventWriter, (char*)fEventWriteBuffer, numBytesRead, &(new_file_flag));
	  if( retVal != numBytesRead ){
	    ARA_LOG_MESSAGE(LOG_WARNING,"Error writing event %d\n",retVal);
	    numBadEvents++;
	  }
	  else numGoodEvents++;
	  //jpd runInfo
	   if(new_file_flag != 0){ 
	     recordCloseEventFile( &(runInfo) , fCurrentEvent ); 
	     updateRunLogFile( &(runInfo) ); 
	     recordOpenEventFile( &(runInfo) ,  fCurrentEvent, eventWriter.startTime.tv_sec , eventWriter.startTime.tv_usec ); 
	   } 
	}
	else {
	  // TEMP FIXME
	  ARA_LOG_MESSAGE(LOG_ERR, "unpackAtriEventV2 of event %d failed (%d)\n", fCurrentEvent, retVal);
	  numBadEvents++;
	}
      }
      else if(retVal==0){
      	usleep(1000);
      }
      else {
      	ARA_LOG_MESSAGE(LOG_WARNING,"Error reading event %d\n",retVal);
	numBadEvents++;
      }	

      ///Now we check if we want to send a software trigger
      if( CONDITION_MET( theConfig.enableSoftTrigger, nowTime, nextSoftTrig ) ){
      //retVal = sendSoftwareTriggerWithInfo(fMainThreadAtriSockFd, softTrigger_FORCED);
	retVal = sendSoftwareTrigger(fMainThreadAtriSockFd);

	if(retVal < 0){
	  ARA_LOG_MESSAGE(LOG_ERR,"Error %d soft triggering",  retVal);
	  fProgramState = ARA_PROG_TERMINATE;
	  continue;
	}
	else {
	  softTrigCount++;
	  if (CONDITION_MET(1, nowTime, nextPrintTrig)) {
	    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: %d soft %s (since last update)\n", softTrigCount, 
			    (softTrigCount > 1) ? "trigs" : "trig");
	    softTrigCount = 0;
	    setNextTime(&nextPrintTrig, &nowTime, &nextPrintStep);
	  }
	}
	setNextTime(&nextSoftTrig,&nextSoftTrig,&nextSoftStep);
      }
      
      if( theConfig.enableEventRateReport &&
	  CONDITION_MET(1, nowTime, nextEventRateReport)){
	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Event Rate %0.2f Hz - %i good events %i bad events since last update\n", numGoodEvents*theConfig.eventRateReportRateHz, numGoodEvents, numBadEvents);
	writeHelpfulTempFile();
	numGoodEvents=0;
	numBadEvents=0;
	setNextTime(&nextEventRateReport, &nowTime, &nextEventRateReportStep);
      }


      break;
    case ARA_PROG_STOPPING:
      //Close files and do everything neccessary
      ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: stopping run.\n");
      ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: masking T1 trigger.\n");
      setTriggerT1Mask(fMainThreadAtriSockFd, 1);
      
      if(fpAtriEventLog) fclose(fpAtriEventLog);
      fpAtriEventLog=NULL;


      closeWriter(&eventWriter);

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
  //  retVal = pthread_join(fLibusbPollThread,&status);
  //  if(retVal) {
  //    ARA_LOG_MESSAGE(LOG_ERR,"ERROR; return code from pthread_join() is %d\n", retVal);
  //  }
  printf("Closing FX2 device...\n");
  closeFx2Device();

  if (theConfig.enablePcieReadout)
      closePcieDevice();
  
  free(fEventReadBuffer);
  free(fEventWriteBuffer);


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
		if(progMsg.runType==ARA_RUN_TYPE_PEDESTAL) {
		  fInPedestalMode=1;
		}
		else {
		  fInPedestalMode=0;
		}

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
		if(progMsg.runType==ARA_RUN_TYPE_PEDESTAL) {
		  fInPedestalMode=1;
		}
		else {
		  fInPedestalMode=0;
		}
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
  progReply.currentStatus=fProgramState;


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
  unsigned char buffer[512]; //Shoiuld be large enough for now
  unsigned char outBuffer[2048]; //Shoiuld be large enough for now
  sprintf(filename,"%s/current/atriControl.log",theConfig.topDataDir);
  sprintf(filenameSent,"%s/current/atriControlSent.log",theConfig.topDataDir);
  int loopCount=0;
  while (fProgramState!=ARA_PROG_TERMINATE) {
    //Need to read stuff from the control port
    numBytesRead=0;
    startByte = 0; //need to reset to beginning
    //    fprintf(stderr,"fx2ControlUsbHandlder loopCount=%d\n",loopCount);
    loopCount++;
    //    ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Starting read from control endpoint.", __FUNCTION__);
    retVal=readControlEndPoint(buffer,512,&numBytesRead);
    //    ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Finished read from control endpoint.", __FUNCTION__);

    //  fprintf(stderr,"fx2ControlUsbHandlder loopCount=%d retVal=%d\n",loopCount,retVal);
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
	    else {
	      // Packet number 0 are broadcast messages. Parse them
	      // intelligently here.
	      if (controlPacket.header.packetLocation == 0) {
		// Packet controller broadcast.
		switch(controlPacket.data[0]) {
		case PC_ERR_RESET:
		  ARA_LOG_MESSAGE(LOG_WARNING,"ARAAcqd: System reset received\n");
		  break;
		case PC_ERR_BAD_PACKET:
		  ARA_LOG_MESSAGE(LOG_ERR,"ARAAcqd: PC sent Bad Packet error\n");
		  break;
		case PC_ERR_BAD_DEST:
		  ARA_LOG_MESSAGE(LOG_ERR, "ARAAcqd: PC sent Bad Destination error\n");
		  break;
		case PC_ERR_I2C_FULL:
		  ARA_LOG_MESSAGE(LOG_ERR, "ARAAcqd: PC sent I2C Full error\n");
		  break;
		}
	      } else if (controlPacket.header.packetLocation == 2) {
		// DB status update
		ARA_LOG_MESSAGE(LOG_WARNING,"ARAAcqd: Daughterboard status update (%2.2x%2.2x%2.2x%2.2x)\n", controlPacket.data[0],controlPacket.data[1],controlPacket.data[2],controlPacket.data[3]);
	      } else {
		ARA_LOG_MESSAGE(LOG_ERR,"What is up with this packet -- %#x %#x %#x %d -- data %#x %#x %#x %#x\n",
				controlPacket.header.frameStart,
				controlPacket.header.packetLocation,
				controlPacket.header.packetNumber,
				controlPacket.header.packetLength,
				controlPacket.data[0],
				controlPacket.data[1],
				controlPacket.data[2],
				controlPacket.data[3]);
	      }
	    }
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
  int retVal=initialiseAtriDaughterboards(fAtriSockFd,theConfig);
  retVal=initialiseAtriClocks(fFx2SockFd,theConfig);
  retVal=initialiseAtriDACs(fAtriSockFd,theConfig);

  // Not enabling the IRS right away now.
  // Pedmode handling doesn't need to be done here, that's automatically
  // reset at the digitizer reset.
  
  uint8_t resetDCM=0x80; 
  retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_DCM,1,&resetDCM);
  
  // This also needs to be moved - it will be reset at triggering subsystem
  // reset. There should probably be a function called initializeRun
  // which sets things up.
  /*
  softwareTrigger|=(theConfig->numSoftTriggerBlocks-1)<<12;  
  do {
    retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_SOFTTRIG,2,(uint8_t*)&softwareTrigger);
    usleep(50000);
    retVal=atriWishboneRead(fAtriSockFd,ATRI_WISH_SOFTTRIG,2,(uint8_t*)&readBack);
    count++;
  } while((readBack&0xff00)!=(softwareTrigger&0xff00) && count<100);
  fprintf(stderr,"Set software trigger high part to %#x\n",readBack);
  */
    
  


  return retVal;
}

int initialiseAtriClocks(int fFx2SockFd,ARAAcqdConfig_t *theConfig)
{
  int retVal=0;
  //  fprintf(stderr,"%s\n",__FUNCTION__);
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s\n",__FUNCTION__);
  //This is taken almost completely from Patrick's clockset_real

  //Reset the Si5367/8
  retVal=writeToI2CRegister(fFx2SockFd,0xd0,0x88,0x80);
  //  fprintf(stderr,"retVal %d\n",retVal);


  retVal=writeToI2CRegister(fFx2SockFd,0xd0,0x88,0x00);

  ARA_LOG_MESSAGE(LOG_DEBUG,"Done the Reset Si5367\n");

  //This fucntion is basically just a rehash of Patrick's clockset.sh
  //Needs some error checking

  //Turn on the FPGA oscillator.  
  uint8_t enableMask=FX2_ATRI_FPGA_OSC_MASK;
  //RJN hack to try and understand what the heck is going on with the oscillator enabling
  retVal=enableAtriComponents(fFx2SockFd,enableMask);
  

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
  
  for(stack=D1;stack<=D4;stack++) {
    if (theConfig->stackEnabled[stack]) {
      for(type=DDA;type<=DRSV10;type++) {
	if(PRESENT(dbStatus,type,stack)) {
	  atriDaughterPowerOn(fAtriSockFd,stack,type,&daughterStatus[stack][type]);
	}
      }
    } else {
      ARA_LOG_MESSAGE(LOG_WARNING, "ARAAcqd: Skipping stack %s since it's not enabled\n",
		      atriDaughterStackStrings[stack]);
    }
  }
  
  dbStatus=atriGetDaughterStatus(fAtriSockFd);  

  ARA_LOG_MESSAGE(LOG_DEBUG, "%s: ATRI Daughter Status:\n", __FUNCTION__);
  for(stack=D1;stack<=D4;stack++) {
    for(type=DDA;type<=DRSV10;type++) {
      ARA_LOG_MESSAGE(LOG_DEBUG, "%s: %s: %s present: %c powered: %c\n",
		      __FUNCTION__,
		      atriDaughterStackStrings[stack],
		      atriDaughterTypeStrings[type],
		      PRESENT(dbStatus,type,stack) ? 'Y' : 'N',
		      POWERED(dbStatus,type,stack) ? 'Y' : 'N');
    }
  }
   
  return 0;
}

int initialiseAtriDACs(int fAtriSockFd,ARAAcqdConfig_t *theConfig)
{
  //Again will use the config at some point maybe
  int stack=0;
  for(stack=D1;stack<=D4;stack++) {    
    if (theConfig->stackEnabled[stack]) {
      //        setIRSClockDACValues(fAtriSockFd,stack,0xB852,0x4600);
      setIRSVdlyDACValue(fAtriSockFd,stack,theConfig->initialVdly[stack]);
      currentVdly[stack]=theConfig->initialVdly[stack];
    }
  }
  //  ARA_LOG_MESSAGE(LOG_DEBUG, "Set Vadj to ");
  for(stack=D1;stack<=D4;stack++) {    
    if (theConfig->stackEnabled[stack]) {
      //        setIRSClockDACValues(fAtriSockFd,stack,0xB852,0x4600);
      setIRSVadjDACValue(fAtriSockFd,stack,theConfig->initialVadj[stack]);
      currentVadj[stack]=theConfig->initialVadj[stack];
      //      fprintf(stderr, "%d ", theConfig->initialVadj[stack]);
    } else {
      //      fprintf(stderr, "(skip) ");
    }
  }
  /* //FIXME //IRS3 -- this should have an IRS type condition here */
  /* fprintf(stderr, "Set Vbias to ");  */
  /* for(stack=D1;stack<=D4;stack++) {     */
  /*   if (theConfig->stackEnabled[stack]) {  */
  /*     setIRSVbiasDACValue(fAtriSockFd,stack,theConfig->initialVbias[stack]);  */
  /*     currentVbias[stack]=theConfig->initialVbias[stack];  */
  /*     fprintf(stderr, "%d ", theConfig->initialVbias[stack]);  */
  /*   } else {  */
  /*     fprintf(stderr, "(skip) ");  */
  /*   }  */
  /* }  */

  /* //FIXME //IRS3 -- this should have an IRS type condition here */
  /* fprintf(stderr, "Set Isel to "); */
  /* for(stack=D1;stack<=D4;stack++) {      */
  /*   if (theConfig->stackEnabled[stack]) {  */
  /*     setIRSIselDACValue(fAtriSockFd,stack,theConfig->initialIsel[stack]);  */
  /*     currentIsel[stack]=theConfig->initialIsel[stack];  */
  /*     fprintf(stderr, "%d ", theConfig->initialIsel[stack]);  */
  /*   } else {  */
  /*     fprintf(stderr, "(skip) ");  */
  /*   }  */
  /* }  */
  ///  fprintf(stderr, "\n");
  return 0;
}


int initialiseTriggers(int fAtriSockFd)
{
  int index=0;
  uint16_t trigMask=0xffff;
  uint16_t trigMask16=0xffff;
  uint8_t numTrigBlocks=0;
  TriggerL4_t trigType;


  //Masking of the l4Triggers
  if(theConfig.enableRF0Trigger){
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling L4 RF0 trigger.\n");
    trigType = triggerL4_RF0;
    trigMask = trigMask & ~(0x01<<trigType);

    numTrigBlocks=theConfig.numRF0TriggerBlocks-1;
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Setting L4 RF0 trigger numBlocks to %i.\n", numTrigBlocks);
    atriSetTriggerNumBlocks(fAtriSockFd,  trigType, (unsigned char)numTrigBlocks);

    numTrigBlocks=theConfig.numRF0PreTriggerBlocks;
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Setting L4 RF0 pre trigger numBlocks to %i.\n", numTrigBlocks);
    atriSetNumPretrigBlocks(fAtriSockFd,  trigType, (unsigned char)numTrigBlocks);

  }
  else 	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Not Enabling L4 RF0 trigger.\n");
  
  if(theConfig.enableRF1Trigger){
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling L4 RF1 trigger.\n");
    trigType = triggerL4_RF1;
    trigMask = trigMask & ~(0x01<<trigType);
  }
  else 	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Not Enabling L4 RF1 trigger.\n");
  
  if(theConfig.enableCalTrigger){
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling L4 Cal trigger.\n");
    trigType = triggerL4_CAL;
    trigMask = trigMask & ~(0x01<<trigType);
  }
  else 	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Not Enabling L4 Cal trigger.\n");
  
  if(theConfig.enableExtTrigger){
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling L4 Ext trigger.\n");
    trigType = triggerL4_EXT;
    trigMask = trigMask & ~(0x01<<trigType);
  }
  else 	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Not Enabling L4 Ext trigger.\n");
  
  if(theConfig.enableSoftTrigger){
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling L4 Soft trigger.\n");
    trigType = triggerL4_CPU;
    trigMask = trigMask & ~(0x01<<trigType);
    numTrigBlocks=theConfig.numSoftTriggerBlocks-1;
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Setting L4 Soft trigger numBlocks to %i.\n", numTrigBlocks);
    atriSetTriggerNumBlocks(fAtriSockFd,  trigType, (unsigned char)numTrigBlocks);
  }
  else 	ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Not Enabling L4 Soft trigger.\n");
  

  setTriggerL4Mask(fAtriSockFd, trigMask);

  //Set the L1Mask
  trigMask16=0xffff;

  ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Disabling the following L1 triggers ");
  for(index=0;index<NUM_L1_MASKS;index++){

    if(theConfig.enableL1Trigger[index])
      trigMask16 = trigMask16& ~(0x01<<index);
    else   ARA_LOG_MESSAGE(LOG_INFO, "%i ", index);

  }
  ARA_LOG_MESSAGE(LOG_INFO, "\n");

  setTriggerL1Mask(fAtriSockFd, trigMask16);

  //Set the L2Mask
  trigMask16=0xffff;
  ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Disabling the following L2 triggers ");
  for(index=0;index<NUM_L2_MASKS;index++){
    if(theConfig.enableL2Trigger[index])
      trigMask16 = trigMask16& ~(0x01<<index);
    else   ARA_LOG_MESSAGE(LOG_INFO, "%i ", index);
  }
  ARA_LOG_MESSAGE(LOG_INFO, "\n");

  setTriggerL2Mask(fAtriSockFd, trigMask16);


  //L3 Triggers
  //First mask off the l3 scalers by setting to 0xff (1 = off, 0 = on)
  //Then check if VPol / HPol should be enabled and set the corresponding bit to 0 / 1 for yes / no
  //The config defaults to on
  trigMask=0xff;
  

  //Really VPol trigger is l3[0] masking on / off
  if(theConfig.enableVPolTrigger){
    
    //turn on the l3 VPol mask
    trigMask = trigMask & ~(0x01<<0);
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling VPolTrigger.\n");
    
  }
  else 
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Not Enabling VPolTrigger.\n");
  
  //Really HPol trigger is l3[1] masking on / off
  if(theConfig.enableHPolTrigger){

    //turn on the l3 VPol mask
    trigMask = trigMask & ~(0x01<<1);
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling HPolTrigger.\n");
      
  }
  else
    ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Not Enabling HPolTrigger.\n");

  setTriggerL3Mask(fAtriSockFd, trigMask);

  
  ARA_LOG_MESSAGE(LOG_INFO, "ARAAcqd: Enabling T1 trigger.\n"); 
  setTriggerT1Mask(fAtriSockFd, 0);      
  
  return 0;
}



int readConfigFile(const char* configFileName,ARAAcqdConfig_t* theConfig)
{

  int status = 0, allStatus = 0;
  KvpErrorCode kvpStatus = 0;
  const char* configSection[] = { "log","acq","thresholds","trigger",
				  "servo","calpulser",NULL };
  const char** currSection = configSection;

  kvpReset();
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"Here\n");
  allStatus = configLoad(ARAD_CONFIG_FILE,"output");
  if(allStatus != CONFIG_E_OK) {
    ARA_LOG_MESSAGE(LOG_ERR,"Problem reading %s -- %s\n",ARAD_CONFIG_FILE,"output");
  }
  

  while( *currSection ){
    status = configLoad((char*)configFileName,(char*)*currSection);
    
    allStatus += status;
    if(status != CONFIG_E_OK) {
      ARA_LOG_MESSAGE(LOG_ERR,"Problem reading %s -- %s\n",configFileName,*currSection);
    } else {
      // PSA: Cal pulser configuration parsing.      
      if (!strcmp(*currSection, "calpulser")) {
	 ARA_LOG_MESSAGE(LOG_ERR, "Section %s matched %s\n", *currSection, "calpulser");
	 if (status == CONFIG_E_OK) calpulser_ParseConfig();
      } else { 
	 ARA_LOG_MESSAGE(LOG_DEBUG, "Section %s did not match %s\n", *currSection, "calpulser");
      }	    
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
    SET_INT(enableEventRateReport, 0);
    SET_FLOAT(eventRateReportRateHz, 0.2);

    // Get data directories
    SET_STRING(topDataDir,".");
    sprintf(theConfig->eventTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_EVENT_DIR);
    sprintf(theConfig->eventHkTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_EVENT_HK_DIR);
    sprintf(theConfig->sensorHkTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_SENSOR_HK_DIR);
    sprintf(theConfig->pedsTopDir,"%s/current/%s",theConfig->topDataDir,DAQ_PED_DIR);
    sprintf(theConfig->runLogDir,"%s/current/%s",theConfig->topDataDir,DAQ_RUNLOG_DIR);
    SET_STRING(runNumFile,"run_number");
    SET_STRING(linkDir,"./link");
    SET_INT(linkForXfer,0);
    SET_INT(filesPerDir,100);
    SET_INT(eventsPerFile,100);
    SET_INT(hkPerFile,500);
    // Run parameters
    SET_INT(doAtriInitialisation,0);
    SET_INT(standAlone,0);
    SET_INT(numEvents,1);
    SET_INT(enableHkRead,1);
    SET_FLOAT(eventHkReadRateHz,1);
    SET_INT(sensorHkReadPeriod,10);
    SET_INT(pedestalMode,0);
    SET_INT(pedestalNumTriggerBlocks,2);
    if(theConfig->pedestalNumTriggerBlocks%2==1) {
      theConfig->pedestalNumTriggerBlocks++;
      ARA_LOG_MESSAGE(LOG_WARNING,"Must have even number of pedestal trigger blocks, setting to %d\n",
		      theConfig->pedestalNumTriggerBlocks);
    }

    SET_INT(pedestalSamples,1000);
    SET_INT(pedestalDebugMode,1);
    SET_INT(vdlyScan,0);
    SET_INT(enablePcieReadout, 0);
    //    SET_INT(usePatrickEvent,0);
    
    // Thresholds
    SET_INT(thresholdScan,0);
    SET_INT(thresholdScanSingleChannel,0);
    SET_INT(thresholdScanSingleChannelNum,0);
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
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus));
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(thresholdValues): %s",kvpErrorString(kvpStatus));
	if( theConfig->setThreshold )
	  status = 1; // Terminate
      }
    }  
    SET_INT(surfaceBoardStack,1);
    SET_INT(setSurfaceThreshold,1);
    { // Read Surface
      int tempNum=DDA_PER_ATRI;
      kvpStatus=kvpGetUnsignedShortArray("surfaceThresholdValues",theConfig->surfaceThresholdValues,&tempNum);
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus));
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(surfaceThresholdValues): %s",kvpErrorString(kvpStatus));
      }
    }  

    

    // Trigger
    SET_INT(numSoftTriggerBlocks,2);
    if(theConfig->numSoftTriggerBlocks%2==1) {
      theConfig->numSoftTriggerBlocks++;
      ARA_LOG_MESSAGE(LOG_WARNING,"Must have even number of software trigger blocks, setting to %d\n",
		      theConfig->numSoftTriggerBlocks);
    }
    SET_INT(numRF0TriggerBlocks,32);
    if(theConfig->numRF0TriggerBlocks%2==1) {
      theConfig->numRF0TriggerBlocks++;
      ARA_LOG_MESSAGE(LOG_WARNING,"Must have even number of software trigger blocks, setting to %d\n",
		      theConfig->numRF0TriggerBlocks);
    }
    SET_INT(numRF0PreTriggerBlocks,10);
    


    SET_INT(enableRF0Trigger,0);
    SET_INT(enableRF1Trigger,0);
    SET_INT(enableCalTrigger,0);
    SET_INT(enableExtTrigger,0);
    SET_INT(enableSoftTrigger,0);
    SET_INT(enableRandTrigger,0);
    SET_INT(enableVPolTrigger,1); //jpd -- really masking l3[0] - default to on
    SET_INT(enableHPolTrigger,1); //jpd -- really masking l3[0] - default to on

    { // read L1Trigger enables
      int tempNum=NUM_L1_MASKS;
      kvpStatus=kvpGetUnsignedShortArray("enableL1Trigger",theConfig->enableL1Trigger,&tempNum);
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(enableL1Trigger): %s",kvpErrorString(kvpStatus));
      }
    }

    { // read L2Trigger enables
      int tempNum=NUM_L2_MASKS;
      kvpStatus=kvpGetUnsignedShortArray("enableL2Trigger",theConfig->enableL2Trigger,&tempNum);
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(enableL2Trigger): %s",kvpErrorString(kvpStatus));
      }
    }


    SET_FLOAT(softTriggerRateHz,0.0);
    SET_FLOAT(randTriggerRateHz,0.0);




    SET_INT(enableTriggerWindow,0);
    SET_INT(triggerWindowSize,10);
    SET_INT(enableTriggerDelays,0);

    { // Read individual trigger delays
      int tempNum=THRESHOLDS_PER_ATRI;
      kvpStatus=kvpGetUnsignedShortArray("triggerDelays",theConfig->triggerDelays,&tempNum);
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(triggerDelays): %s",kvpErrorString(kvpStatus));
      }
    }

    // Servo
    {
      int tempNum=DDA_PER_ATRI;
      kvpStatus=kvpGetUnsignedShortArray("initialVadj",theConfig->initialVadj,&tempNum);
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus));
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(initialVadj): %s",kvpErrorString(kvpStatus));
      }

      kvpStatus=kvpGetUnsignedShortArray("initialVdly",theConfig->initialVdly,&tempNum);
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus));
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(initialVdly): %s",kvpErrorString(kvpStatus));
      }

      kvpStatus=kvpGetUnsignedShortArray("initialVbias",theConfig->initialVbias,&tempNum); 
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus)); 
      if(kvpStatus!=KVP_E_OK){ 
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(initialVbias): %s",kvpErrorString(kvpStatus)); 
      } 
      kvpStatus=kvpGetUnsignedShortArray("initialIsel",theConfig->initialIsel,&tempNum); 
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus)); 
      if(kvpStatus!=KVP_E_OK){ 
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(initialIsel): %s",kvpErrorString(kvpStatus)); 
      } 


    }

    SET_INT(enableVdlyServo,1);
    SET_INT(wilkinsonGoal,62000);
    SET_FLOAT(vdlyPGain,0.5);
    SET_FLOAT(vdlyIGain,0.01);
    SET_FLOAT(vdlyDGain,0.0);
    SET_FLOAT(vdlyIMax,100);
    SET_FLOAT(vdlyIMin,-100);

    
    SET_INT(enableAntServo,0);
    { // Read individual scaler goals
      int tempNum=THRESHOLDS_PER_ATRI;
      kvpStatus=kvpGetUnsignedShortArray("scalerGoalValues",theConfig->scalerGoalValues,&tempNum);
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus));
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(scalerGoalValues): %s",kvpErrorString(kvpStatus));
      }
    }  
    SET_FLOAT(scalerPGain,0.5);
    SET_FLOAT(scalerIGain,0.01);
    SET_FLOAT(scalerDGain,0.0);
    SET_FLOAT(scalerIMax,100);
    SET_FLOAT(scalerIMin,-100);

    SET_INT(enableRateServo,0);
    SET_FLOAT(servoCalcPeriodS,1);
    SET_FLOAT(servoCalcDelayS,10);
    SET_FLOAT(rateGoalHz,5); 
    SET_FLOAT(ratePGain,0.5);
    SET_FLOAT(rateIGain,0.01);
    SET_FLOAT(rateDGain,0.0);
    SET_FLOAT(rateIMax,100);
    SET_FLOAT(rateIMin,-100);

    SET_INT(enableSurfaceServo,0);
    { // Read individual scaler goals
      int tempNum=ANTS_PER_TDA;
      kvpStatus=kvpGetUnsignedShortArray("surfaceGoalValues",theConfig->surfaceGoalValues,&tempNum);
      //      fprintf(stderr,"kvpGetUnsignedShortArray %s\n",kvpErrorString(kvpStatus));
      if(kvpStatus!=KVP_E_OK){
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(surfaceGoalValues): %s",kvpErrorString(kvpStatus));
      }
    }  

    {
      int tempNum=DDA_PER_ATRI;
      int se_i;
      kvpStatus=kvpGetIntArray("stackEnabled",theConfig->stackEnabled, &tempNum);
      if (kvpStatus != KVP_E_OK) {
	ARA_LOG_MESSAGE(LOG_WARNING,"kvpGetUnsignedShortArray(stackEnabled): %s", kvpErrorString(kvpStatus));
      }
      theConfig->numEnabledStacks = 0;
      for (se_i=0;se_i<DDA_PER_ATRI;se_i++) {
	if (theConfig->stackEnabled[se_i]) theConfig->numEnabledStacks++;
      }
      ARA_LOG_MESSAGE(LOG_INFO,"There are %d stacks enabled\n", theConfig->numEnabledStacks);
    }
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
  if(!fHkThreadFx2SockFd) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can not open connection to FX2_CONTROL from %s\n",__FUNCTION__);
  }
  else {
    ARA_LOG_MESSAGE(LOG_DEBUG,"fHkThreadFx2SockFd=%d\n",fHkThreadFx2SockFd);
  }  


  int fHkThreadAtriSockFd=openConnectionToAtriControlSocket();
  if(!fHkThreadAtriSockFd) {
    ARA_LOG_MESSAGE(LOG_ERR,"Can not open connection to ATRI_CONTROL from %s\n",__FUNCTION__);
  }
  else {
    ARA_LOG_MESSAGE(LOG_DEBUG,"fHkThreadAtriSockFd=%d\n",fHkThreadAtriSockFd);
  }



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
      fHkThreadStopped=0;
      gettimeofday(&currentTime,NULL);
      if(checkDeltaT(&currentTime,&lastSensorHkTime,sensorHkPeriod)) {
	//Need to read out a sensor hk thingy
	lastSensorHkTime=currentTime;
	retVal=readSensorHk(fHkThreadFx2SockFd,fHkThreadAtriSockFd,&sensorHk,&currentTime);
	//Now need to add it to the writer thingy

	// Store hk event
	int new_hk_file_flag = 0;
	retVal = writeBuffer( &sensorHkWriter, (char*)&sensorHk, sizeof(AraSensorHk_t) , &(new_hk_file_flag) );
	if( retVal != sizeof(AraSensorHk_t) ) {
	  ARA_LOG_MESSAGE(LOG_WARNING,"%s: failed to write sensor housekeeping event (return %d)\n", __FUNCTION__, retVal);
	}
	else {
	  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: wrote %lu bytes of sensor housekeeping\n", __FUNCTION__, sizeof(AraSensorHk_t));
	}
	
      }
      
      if(checkDeltaT(&currentTime,&lastEventHkTime,eventHkPeriod)) {
	//Need to read out a sensor hk thingy

	retVal=readEventHk(fHkThreadAtriSockFd,&eventHk,&currentTime);
	//Now need to add it to the writer thingy

	if(retVal) {
	  lastEventHkTime=currentTime;
	  // Store hk event
	  int new_hk_file_flag = 0;
	  retVal = writeBuffer( &eventHkWriter, (char*)&eventHk, sizeof(AraEventHk_t) , &(new_hk_file_flag) );
	  if( retVal != sizeof(AraEventHk_t) )
	    ARA_LOG_MESSAGE(LOG_WARNING,"Failed to write event housekeeping event\n");


	  //Got data from a new second
	  if(theConfig.enableVdlyServo) updateWilkinsonVdly(fHkThreadAtriSockFd, &eventHk);

	    //updateWilkinsonVdlyUsingPID(fHkThreadAtriSockFd, &eventHk);

	  //Try and servo on thresholds if we want that
	  if(theConfig.enableAntServo) updateThresholdsUsingPID(fHkThreadAtriSockFd,&eventHk);

	  //Try and servo on the surface thresholds
	  if(theConfig.enableSurfaceServo) updateSurfaceThresholdsUsingPID(fHkThreadAtriSockFd,&eventHk);

	}
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
      fprintf(stderr,"ARAAcqd is exiting\n");
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
     read 1 byte from FX2 I2C address 0x97 (ATRI current), 
     write 0x5 to FX2 I2C address 0x96, 
     read 1 byte from FX2 I2C address 0x97 (ATRI voltage) */
  /* 5: DDA/TDA voltage/temperature/current: For all daughterboards: 
     write 0x0C to I2C address 0xE4, 
     write 0x0C to I2C address 0xFC, 
     read 2 bytes from I2C address 0x91 (DDA temp), 
     read 2 bytes from I2C address 0x93 (TDA temp), 
     read 3 bytes from I2C address 0xE5 (DDA voltage/current),
     read 3 bytes from I2C address 0xFD (TDA voltage/current). */
  
  //jpd - previous version of software (<4.2) had the ATRI current and voltage
  //      the wrong way around

  //ATRI Voltage
  uint8_t data[4]={0,0,0,0};
  uint16_t value16=0;
  uint32_t value32=0;
  data[0]=0x4;
  writeToI2C(fFx2SockFd,0x96,1,data);
  readFromI2C(fFx2SockFd,0x97,1,data);
  sensorPtr->atriCurrent=data[0];

  //ATRI Current
  data[0]=0x5;
  writeToI2C(fFx2SockFd,0x96,1,data);
  readFromI2C(fFx2SockFd,0x97,1,data);
  sensorPtr->atriVoltage=data[0];

  //DDA Temp/voltage/current
  for(stack=D1;stack<=D4;stack++) {  
    if (theConfig.stackEnabled[stack]) 
       {	  
	 data[0]=0x1A; //email from psa 11th Nov 2012
	 writeToAtriI2C(fAtriSockFd,stack,atriHotSwapAddressMap[DDA]+i2cWrite,1,data);
	 writeToAtriI2C(fAtriSockFd,stack,atriHotSwapAddressMap[TDA]+i2cWrite,1,data);
       }
  }

  for(stack=D1;stack<=D4;stack++) {  
    if (theConfig.stackEnabled[stack]) {	  
    //Read DDA temp
    readFromAtriI2C(fAtriSockFd,stack,atriTempAddressMap[DDA]+i2cRead,2,data);
    value16=data[1];
    value16=(value16<<8);
    value16|=data[0];
    sensorPtr->ddaTemp[stack]=value16;
    //Read TDA temp
    readFromAtriI2C(fAtriSockFd,stack,atriTempAddressMap[TDA]+i2cRead,2,data);
    value16=data[1];
    value16=(value16<<8);
    value16|=data[0];
    sensorPtr->tdaTemp[stack]=value16;
    //Read DDA voltage/current
    readFromAtriI2C(fAtriSockFd,stack,atriHotSwapAddressMap[DDA]+i2cRead,3,data);
    value32=data[2];
    value32=(value32<<16);
    value16=data[1];
    value16=(value16<<8);
    value32|=value16;
    value32|=data[0];
    sensorPtr->ddaVoltageCurrent[stack]=value32;
    //Read TDA voltage/current
    readFromAtriI2C(fAtriSockFd,stack,atriHotSwapAddressMap[TDA]+i2cRead,3,data);
    value32=data[2];
    value32=(value32<<16);
    value16=data[1];
    value16=(value16<<8);
    value32|=value16;
    value32|=data[0];
    sensorPtr->tdaVoltageCurrent[stack]=value32;
    }
  }
#endif
  fillGenericHeader(sensorPtr,ARA_SENSOR_HK_TYPE,sizeof(AraSensorHk_t));  
  return 0;
  
}
		     
int readEventHk(int fAtriSockFd,AraEventHk_t *eventPtr, struct timeval *currTime)
{
  //At the moment we are doing everything with small reads. At some point will chnage to big reads when we need the extra speed.
  static unsigned int lastPpsCounter=0;
  static int firstTime=1;
  
  int stack;
  int ant;
  //  uint32_t value32;
  //  uint32_t temp32;
  //  int dda;//,i;
  uint8_t data[64];

  eventPtr->unixTime=currTime->tv_sec;
  eventPtr->unixTimeUs=currTime->tv_usec;
#ifndef NO_USB

  /*  1: read address 0x2C-0x3B on the WISHBONE bus (Wilkinson counter and */
  /* sample speed monitors) */
  /*  2: read address 0x30-0x37 on the WISHBONE bus (clock count/PPS count) */
  /*  3: read address 0x0100-0x15F on the WISHBONE bus (scalers) */


  //PPS counter
  atriWishboneRead(fAtriSockFd,ATRI_WISH_PPSCNT,4,(uint8_t*)&(eventPtr->ppsCounter));
  fCurrentPps=eventPtr->ppsCounter;
  if(lastPpsCounter==eventPtr->ppsCounter && lastPpsCounter!=0) {
    return 0;
  }
  lastPpsCounter=eventPtr->ppsCounter;
 


  /*
  atriWishboneRead(fAtriSockFd,ATRI_WISH_IDREG,8,data);
  printf("ATRI_WISH_IDREG %d %d %d %d -- %c %c %c %c\n",
	 data[0],data[1],data[2],data[3],
	 data[3],data[2],data[1],data[0]);
  printf("ATRI_WISH_VERREG %d %d %d %d\n",
	 data[4],data[5],data[6],data[7]);

  //Firmware version
  //  atriWishboneRead(fAtriSockFd,ATRI_WISH_VERREG,4,(uint8_t*)&(eventPtr->firmwareVersion));

  */
  atriWishboneRead(fAtriSockFd,ATRI_WISH_IDREG,8,data);
  copyFourBytes(&(data[4]),&(eventPtr->firmwareVersion));



  //Clock counters
  atriWishboneRead(fAtriSockFd,ATRI_WISH_D1WILK,16,data);

  
  /* fprintf(stderr,"Wilk\t"); */
  /* for(i=0;i<16;i++) { */
  /*   fprintf(stderr,"%d ",data[i]); */
  /* } */
  /* fprintf(stderr,"\n"); */

  copyTwoBytes(&(data[ATRI_WISH_D1WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[0]));
  copyTwoBytes(&(data[ATRI_WISH_D1DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[0]));
  copyTwoBytes(&(data[ATRI_WISH_D2WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[1]));
  copyTwoBytes(&(data[ATRI_WISH_D2DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[1]));
  copyTwoBytes(&(data[ATRI_WISH_D3WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[2]));
  copyTwoBytes(&(data[ATRI_WISH_D3DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[2]));
  copyTwoBytes(&(data[ATRI_WISH_D4WILK-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonCounter[3]));
  copyTwoBytes(&(data[ATRI_WISH_D4DELAY-ATRI_WISH_D1WILK]),(uint16_t*)&(eventPtr->wilkinsonDelay[3]));

  /*
  fprintf(stderr,"WILK %d %d %d %d\n",eventPtr->wilkinsonCounter[0],eventPtr->wilkinsonCounter[1],eventPtr->wilkinsonCounter[2],eventPtr->wilkinsonCounter[2]);
  fprintf(stderr,"Vdly %d %d %d %d\n",currentVdly[0],currentVdly[1],currentVdly[2],currentVdly[3]);
  */

  //Clock counter
  atriWishboneRead(fAtriSockFd,ATRI_WISH_CLKCNT,4,(uint8_t*)&(eventPtr->clockCounter));

  //Deadtime 
  // PSA: these don't exist yet, and will be heavily reordered anyway
  /*
  atriWishboneRead(fAtriSockFd,ATRI_WISH_D1DEAD,4,(uint8_t*)&(eventPtr->deadTime));
  atriWishboneRead(fAtriSockFd,ATRI_WISH_D1OCCU,4,(uint8_t*)&(eventPtr->avgOccupancy));
  atriWishboneRead(fAtriSockFd,ATRI_WISH_D1MAX,4,(uint8_t*)&(eventPtr->maxOccupancy));
  for(dda=0;dda<4;dda++) {
    fprintf(stderr,"Dead time: %f\tAvg. Occ: %d\tMax. Occ: %d\n",
	    ((float)eventPtr->deadTime[dda])*4096./1e6,eventPtr->avgOccupancy[dda],eventPtr->maxOccupancy[dda]);
  }
  */

  

  //JPD new L1 structure
  //  ATRI_WISH_SCAL_L1_START=0x0100,
  atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_L1_START,2*NUM_L1_SCALERS,data);
  uint8_t loop=0;
  for(loop=0;loop<NUM_L1_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(eventPtr->l1Scaler[loop]));
    //    if(loop<4)    fprintf(stderr, "%s : l1Scaler[%i] %i\n", __FUNCTION__, loop, eventPtr->l1Scaler[loop]);

  }
  //JPD copy the l1ScalerSurface
  loop=0;
  for(loop=0;loop<16;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(eventPtr->l1ScalerSurface[loop]));
  }

  //JPD new L2 structure
  //  ATRI_WISH_SCAL_L2_START=0x0140
  atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_L2_START,2*NUM_L2_SCALERS,data);
  loop=0;
  for(loop=0;loop<NUM_L2_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(eventPtr->l2Scaler[loop]));
    //    if(loop<4)    fprintf(stderr, "%s : l2Scaler[%i] %i\n", __FUNCTION__, loop, eventPtr->l2Scaler[loop]);
  }
  //JPD new L3 structure
  //  ATRI_WISH_SCAL_L3_START=0x0180,
  atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_L3_START, 2*NUM_L3_SCALERS,data);
  loop=0;
  for(loop=0;loop<NUM_L3_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(eventPtr->l3Scaler[loop]));
  }
  //JPD new L4 structure
  //  ATRI_WISH_SCAL_L4_START=0x01A0,
  atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_L4_START, 2*NUM_L4_SCALERS,data);
  loop=0;
  for(loop=0;loop<NUM_L4_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(eventPtr->l4Scaler[loop]));
  }
  //JPD new T1 structure
  //  ATRI_WISH_SCAL_T1_START=0x01B0,
  atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_T1_START, 2*NUM_T1_SCALERS,data);
  loop=0;
  for(loop=0;loop<NUM_T1_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(eventPtr->t1Scaler[loop]));
  }

  //JPD deadtime statistics
  atriWishboneRead(fAtriSockFd, ATRI_WISH_EVERROR, 1,data);
  eventPtr->evReadoutError=data[0];

  atriWishboneRead(fAtriSockFd, ATRI_WISH_EVCOUNTAVG, 2,data);
  copyTwoBytes(&(data[0]),(uint16_t*)&(eventPtr->evReadoutCountAvg));
  atriWishboneRead(fAtriSockFd, ATRI_WISH_EVCOUNTMIN, 2,data);
  copyTwoBytes(&(data[0]),(uint16_t*)&(eventPtr->evReadoutCountMin));

  atriWishboneRead(fAtriSockFd, ATRI_WISH_BLKCOUNTAVG, 2,data);
  copyTwoBytes(&(data[0]),(uint16_t*)&(eventPtr->blockBuffCountAvg));
  atriWishboneRead(fAtriSockFd, ATRI_WISH_BLKCOUNTMAX, 2,data);
  copyTwoBytes(&(data[0]),(uint16_t*)&(eventPtr->blockBuffCountMax));

  atriWishboneRead(fAtriSockFd, ATRI_WISH_IRS_DEADTIME, 2,data);
  copyTwoBytes(&(data[0]),(uint16_t*)&(eventPtr->digDeadTime));

  atriWishboneRead(fAtriSockFd, ATRI_WISH_USB_DEADTIME, 2,data);
  copyTwoBytes(&(data[0]),(uint16_t*)&(eventPtr->buffDeadTime));

  atriWishboneRead(fAtriSockFd, ATRI_WISH_TOT_DEADTIME, 2,data);
  copyTwoBytes(&(data[0]),(uint16_t*)&(eventPtr->totalDeadTime));
  
#endif
  //Tend to readout garbage in the first second
  if(firstTime) {
    firstTime=0;
    return 0;
  }

  for(stack=D1;stack<=D4;stack++)  {
    eventPtr->vdlyDac[stack]=currentVdly[stack];
    eventPtr->vadjDac[stack]=currentVadj[stack];
  }
  //  for(ant=0;ant<ANTS_PER_TDA;ant++) {
  uint8_t i=0;
  for(i=0;i<NUM_L1_SCALERS;i++){
    eventPtr->thresholdDac[i]=currentThresholds[i];
    //    if (i<4) fprintf(stderr, "%s : currentThresholds[%i] %i thresholdDac[%i] %i\n", __FUNCTION__,  i, currentThresholds[i], i, eventPtr->thresholdDac[i]); 
  }
  
  for(ant=0;ant<ANTS_PER_TDA;ant++) { 
    eventPtr->surfaceThresholdDac[ant]=currentSurfaceThresholds[ant]; 
  } 
  


  fillGenericHeader(eventPtr,ARA_EVENT_HK_TYPE,sizeof(AraEventHk_t));  
  return 1;
}

int sendSoftwareTrigger(int fAtriSockFd){

  int retVal=0;
  uint8_t softwareTrigger=0x1;
  retVal = atriWishboneWrite(fAtriSockFd, ATRI_WISH_SOFTTRIG, 1, (uint8_t*)&softwareTrigger);
  time(&lastSoftwareTrigger);

  return retVal;
}


int sendSoftwareTriggerWithInfo(int fAtriSockFd, AraAtriSoftTriggerInfo_t triggerInfo)
{
  int retVal;

  uint8_t softTrigInfo = (0x01);
  softTrigInfo = (softTrigInfo<<triggerInfo);
  uint8_t  softTrigInfoReadBack=0;
  retVal = atriWishboneWrite(fAtriSockFd, ATRI_WISH_SOFTINFO, 1, (uint8_t*)&softTrigInfo);
  retVal = atriWishboneRead(fAtriSockFd, ATRI_WISH_SOFTINFO, 1, (uint8_t*)&softTrigInfoReadBack);

  if(retVal < 0) return retVal;

  uint8_t softwareTrigger=0x1;
  retVal = atriWishboneWrite(fAtriSockFd, ATRI_WISH_SOFTTRIG, 1, (uint8_t*)&softwareTrigger);
  time(&lastSoftwareTrigger);

  return retVal;


}

int sendSoftwareTriggerNumBlocks(int fAtriSockFd){

  int retVal;
  uint8_t softTriggerBlocks=theConfig.numSoftTriggerBlocks-1;
  TriggerL4_t ttype = triggerL4_CPU;
  retVal =  atriSetTriggerNumBlocks(fAtriSockFd,  ttype, (unsigned char)softTriggerBlocks);
  if(retVal < 0) return retVal;
  uint8_t softwareTrigger=0x1;
  retVal = atriWishboneWrite(fAtriSockFd, ATRI_WISH_SOFTTRIG, 2, (uint8_t*)&softwareTrigger);

  time(&lastSoftwareTrigger);

  return retVal;
}



int readAtriEventPatrick(char *eventBuffer)
{
  //Try to read an event, returns 0 if there is no event, 1 if there is an event and -1 if there is a protocol error
  int retVal=0;
  int numBytesRead,i;
  unsigned char buffer[4096];
  
  int expectFrame=0;
  int eventLength=0;
  int expectStack=0;
  //  uint32_t eventId;
  uint16_t blockId;
  int uptoSample=0;
  int uptoByte=0;
  int uptoByteOutput=0;
  
  uint32_t tempVal=0;

  AraStationEventHeader_t *evtHeaderPtr=(AraStationEventHeader_t*)eventBuffer;
  memset(evtHeaderPtr,0,sizeof(AraStationEventHeader_t));
  uptoByteOutput+=sizeof(AraStationEventHeader_t);
  AraStationEventBlockHeader_t *blkHeaderPtr=NULL;
  AraStationEventBlockChannel_t *blkChanPtr=NULL;


  while (1) {
    retVal=readEventEndPoint(buffer,512,&numBytesRead);
    if(retVal==0) {
      if(numBytesRead>0) {
	ARA_LOG_MESSAGE(LOG_INFO,"\nRead %d bytes from event end point\n",numBytesRead);	 	  
	//	fprintf(stderr,"expectFrame %d, expectStack %d\n",expectFrame,expectStack);
	//Try and unpack the event
	if(expectFrame==0) {
	  if(buffer[0]==0x65 && buffer[1]==0) {
	    uptoSample=0;
	    //Correct frame number

	    eventLength = buffer[2];
	    tempVal=buffer[3];
	    eventLength |= (tempVal << 8);
	    
	    

	    evtHeaderPtr->numReadoutBlocks=4;

	    //Unpack eventId
	    /* eventId = (buffer[4] << 16); */
	    /* eventId |= (buffer[5] << 24); */
	    /* eventId |= (buffer[6]); */
	    /* eventId |= (buffer[7] << 8); */
	    /* eventPtr->eventId[expectStack]=eventId; */

	    //Now we are starting a new block
	    blkHeaderPtr=(AraStationEventBlockHeader_t*)&eventBuffer[uptoByteOutput];
	    uptoByteOutput+=sizeof(AraStationEventBlockHeader_t);

	    //Reading out all of the channels
	    blkHeaderPtr->channelMask=0xf;

	    uptoByte=10;

	    //Unpack blockId
	    blockId = buffer[uptoByte++];
	    tempVal=buffer[uptoByte++];
	    blockId |= (tempVal << 8);
	    blkHeaderPtr->irsBlockNumber=blockId;

	    //Now comes the sample data
	    blkChanPtr=(AraStationEventBlockChannel_t*)&eventBuffer[uptoByteOutput];
	    uptoByteOutput+=sizeof(AraStationEventBlockChannel_t);

	    
	    uptoSample=0;
	    while(uptoByte+1<numBytesRead) {
	      blkChanPtr->samples[uptoSample]=buffer[uptoByte++];
	      tempVal=buffer[uptoByte++];
	      blkChanPtr->samples[uptoSample]=(tempVal<<8);
	      uptoSample++;
	      if(uptoSample==SAMPLES_PER_BLOCK) {
		blkChanPtr=(AraStationEventBlockChannel_t*)&eventBuffer[uptoByteOutput];
		uptoByteOutput+=sizeof(AraStationEventBlockChannel_t);
		uptoSample=0;
	      }
	    }	    

	    expectFrame++;
	    if(eventLength==numBytesRead-4) {
	      expectFrame=0;
	      expectStack++;
	      if(expectStack==4) {
		//Finished event
		expectStack=0;
		return uptoByteOutput;
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
	    tempVal=buffer[3];
	    eventLength |= (tempVal << 8);	    
	    uptoByte=4;


	    while(uptoByte+1<numBytesRead) {
	      blkChanPtr->samples[uptoSample]=buffer[uptoByte++];
	      tempVal=buffer[uptoByte++];
	      blkChanPtr->samples[uptoSample]=(tempVal<<8);
	      uptoSample++;
	      if(uptoSample==SAMPLES_PER_BLOCK) {
		blkChanPtr=(AraStationEventBlockChannel_t*)&eventBuffer[uptoByteOutput];
		uptoByteOutput+=sizeof(AraStationEventBlockChannel_t);
		uptoSample=0;
	      }
	    }	

	    expectFrame++;
	    if(eventLength==numBytesRead-4) {
	      expectFrame=0;
	      expectStack++;
	      if(expectStack==4) {
		//Finished event
		expectStack=0;
		return uptoByteOutput;
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
	}
      }
      else {
	//	fprintf(stderr,"*");
	usleep(1);
	return 0;
      }      
    }
    else {
      ARA_LOG_MESSAGE(LOG_ERR,"readEventEndPoint returned %d\n",retVal);
      return -1;
    }
  }
}
// nearest multiple of 512 to maximum block size
#define MAX_BLOCK_SIZE  4608

int readAtriEventV2(unsigned char *eventBuffer)
{
  int ret = 0;
  int nb,i;

  int upToByteIn=0;
  int upToByteOut=0;

  unsigned char buffer[MAX_BLOCK_SIZE];
  uint16_t frameLength=0;
  int loopCount=0;
  //  uint16_t tempVal=0;

  uint16_t expectedBytes=0;
  
  //added to fix event loss issue. Before dropping an event, the last buffer is checked, to make sure that the event readout did not get interrupted half way.
  unsigned char lastBuffer[2];

  //buffer is temporary, it is 
  //  buffer = (unsigned char *) malloc(sizeof(unsigned char)*MAX_BLOCK_SIZE);

  ARA_LOG_MESSAGE(LOG_DEBUG,  "%s : Starting event read!\n", __FUNCTION__);    //FIXME: Added this line: FIXED: Changed to LOG_DEBUG

  while(1) {
    ret = readEventEndPoint(buffer, MAX_BLOCK_SIZE, &nb);
    if (ret == 0) {
      // nothing to do if no bytes...
      if (nb == 0 && lastBuffer[0]!=FIRST_BLOCK_FRAME_OTHER && lastBuffer[0]!=MIDDLE_BLOCK_FRAME_OTHER ){ //FIXME: This is to help not breaking up events!!!
	//	free(buffer);
	return 0;
      }
      else if(nb==0){//FIXME: This is to help not breaking up events!!!
	ARA_LOG_MESSAGE(LOG_DEBUG,  "%s : Skipping this round due to empty buffer in the middle of an event after block: %c with number %d !\n", __FUNCTION__, lastBuffer[0], lastBuffer[1]);
	continue;
      }
      
      // otherwise, start counting
      ARA_LOG_MESSAGE(LOG_DEBUG,  "%s : read %d bytes\n", __FUNCTION__, nb);

      if(fpAtriEventLog!=NULL && theConfig.atriEventLog) {
	for (i=0;i<nb;i++) 
	  {
	    fprintf(fpAtriEventLog,"%2.2x", buffer[i]); 
	    if (!((i+1)%16) || (i+1==nb)) fprintf(fpAtriEventLog, "\n"); 
	    else fprintf(fpAtriEventLog," ");      
	  }
	fprintf(fpAtriEventLog,"\n");
      }

      if ( (buffer[0] != FIRST_BLOCK_FRAME_OTHER &&
	    buffer[0] != MIDDLE_BLOCK_FRAME_OTHER &&
	    buffer[0] != LAST_BLOCK_FRAME_OTHER &&
	    buffer[0] != ONLY_BLOCK_FRAME_OTHER)) {
	ARA_LOG_MESSAGE(LOG_DEBUG, "%s : malformed block (%2.2x) (last expectedBytes %d, nb %d)\n",
			__FUNCTION__, buffer[0],expectedBytes,nb);
	//	fprintf(stderr, "%s : malformed block (%2.2x) (last expectedBytes %d, nb %d)\n",
	//			__FUNCTION__, buffer[0],expectedBytes,nb);
	//	free(buffer);
	return -1;
      }
      else {
	frameLength =  ((buffer[2] << 8)|(buffer[3]));
	lastBuffer[0]=buffer[0];//FIXME: This is to help not breaking up events!!!
	lastBuffer[1]=buffer[1];
	//	tempVal=buffer[2];
	//	fraameLength |= (tempVal << 8);
	//	fprintf(stderr,"Frame Length %d, nb %d\n",frameLength,nb);
	expectedBytes=(2*frameLength)+4;

	if(expectedBytes>MAX_BLOCK_SIZE) {
	  //	  fprintf(stderr, "%s : Expect more bytes than is reasonable %d compared to %d (nb=%d)\n",
	  //		  __FUNCTION__, expectedBytes,MAX_BLOCK_SIZE,nb);
	  ARA_LOG_MESSAGE(LOG_ERR, "%s : Expect more bytes than is reasonable %d compared to %d (nb=%d)\n",
			  __FUNCTION__, expectedBytes,MAX_BLOCK_SIZE,nb);
	  return -1;
	}


	if(nb<expectedBytes) {	  
	  while(nb<expectedBytes && loopCount<100) {
	    ARA_LOG_MESSAGE(LOG_WARNING,"Need to multi read %d bytes of %d so far\n",nb,expectedBytes);
	    int tempNb=0;
	    usleep(1); //RJN magic usleep
	    ret = readEventEndPoint(&buffer[nb],MAX_BLOCK_SIZE-nb,&tempNb);
	    if(ret==0) {
	      nb+=tempNb;
	    }
	    else {
	      //	      fprintf(stderr, "%s : error %d reading event endpoint\n", __FUNCTION__,   ret);
	      ARA_LOG_MESSAGE(LOG_DEBUG, "%s : error %d multi reading event endpoint\n", __FUNCTION__,  ret);  
	      //      free(buffer);
	      return -1;
	    }
	    loopCount++;
	  }
	  if(loopCount>=100) {
	    ARA_LOG_MESSAGE(LOG_ERR,"Didn't converge after %d loops of trying to multi-read\n",loopCount);
	    return -1;
	  }	    

	}
	else if(nb>expectedBytes) {
	  //	  fprintf(stderr, "%s : Read more bytes than expected %d compared to %d\n",
	  //		  __FUNCTION__, nb,expectedBytes);
	  ARA_LOG_MESSAGE(LOG_ERR, "%s : Read more bytes than expected %d compared to %d\n",
			  __FUNCTION__, nb,expectedBytes);
	  return -1;
	}
	
	if(upToByteOut+nb>MAX_EVENT_BUFFER_SIZE) {
	  ARA_LOG_MESSAGE(LOG_ERR,"%s : Oh dear we already have %d bytes and have got %d more, maximum size is %d\n",__FUNCTION__, upToByteOut,nb,MAX_EVENT_BUFFER_SIZE);
	  //	  fprintf(stderr,"%s : Oh dear we already have %d bytes and have got %d more, maximum size is %d\n",__FUNCTION__, upToByteOut,nb,MAX_EVENT_BUFFER_SIZE);
	  return -1;
	}

        
	upToByteIn =0;
	while(upToByteIn < nb){
	  eventBuffer[upToByteOut++]=buffer[upToByteIn++];
	}

	ARA_LOG_MESSAGE(LOG_DEBUG, "%s : %c block %d received ( %d payload words), total read: %d bytes\n", __FUNCTION__, buffer[0], buffer[1], frameLength, upToByteOut);
	
	if (buffer[0] == LAST_BLOCK_FRAME_OTHER ||
	    buffer[0] == ONLY_BLOCK_FRAME_OTHER) {
	  //fprintf(stderr, "%s : event complete (%d blocks, %d bytes)\n",			  __FUNCTION__, buffer[1], upToByteOut);
	  ARA_LOG_MESSAGE(LOG_DEBUG, "%s : event complete ( %d blocks, %d bytes) -- ", __FUNCTION__, buffer[1], upToByteOut);  //FIXME: Uncommented line and changed from LOG_DEBUG to LOG_ERR
	  
	  //	  free(buffer);
	  return upToByteOut;
	}	
      }
    } else {
      //      fprintf(stderr, "%s : error %d reading event endpoint\n", __FUNCTION__,   ret);
      ARA_LOG_MESSAGE(LOG_ERR, "%s : error %d reading event endpoint\n", __FUNCTION__,  ret);
      //      free(buffer);
      return -1;
    }
  }
}

int unpackAtriEventV2(unsigned char *inputBuffer, unsigned char *outputBuffer, int numBytesIn){
  //RJN added a variable to unpackAtriEventV2 which is the number of input bytes
  //This function takes an event from the inputBuffer and stuffs ATRI events into the outputBuffer
  int up_to_byte_input=0;
  int up_to_byte_output=0;
  uint32_t temp_value=0;


  //Framing
  uint8_t frame_type=0;
  uint8_t frame_number=0;
  uint16_t frame_length=0;
  uint8_t expected_frame_number=0;

  //Block Header Checks
  //RJN commented out as not used in function
  //  uint8_t stack_enabled[DDA_PER_ATRI]={0};
  //  uint8_t num_blocks[DDA_PER_ATRI]={0};
  
  //Data Formats
  AraStationEventHeader_t *evtHeaderPtr=(AraStationEventHeader_t*)outputBuffer;
  memset(evtHeaderPtr,0,sizeof(AraStationEventHeader_t));
  up_to_byte_output+=sizeof(AraStationEventHeader_t);
  AraStationEventBlockHeader_t *blkHeaderPtr=NULL;
  AraStationEventBlockChannel_t *blkChanPtr=NULL;

  //event header
  uint16_t numReadOutBlocks=0;

  //l4 triggers
  uint8_t l4_triggers_new=0;
  uint8_t l4_triggers=0;
  uint32_t l4_trigger_info=0;

  //block_info
  uint16_t block_info=0;
  uint16_t block_id=0;
  uint8_t dda_mask=0;

  //block_header
  uint16_t block_header=0;
  uint8_t channel_mask=0;
  
  while(1){

    //framing
    frame_type = inputBuffer[up_to_byte_input];
    frame_number = inputBuffer[up_to_byte_input+1];
    frame_length = inputBuffer[up_to_byte_input+3];
    temp_value = inputBuffer[up_to_byte_input+2];
    frame_length |= temp_value << 8;
    up_to_byte_input+=4;
    //framing

    ARA_LOG_MESSAGE(LOG_DEBUG, "%s: unpacked: %x %x len %d...\n", __FUNCTION__, frame_type, frame_number, frame_length);

    if(up_to_byte_input>numBytesIn || up_to_byte_input>MAX_EVENT_BUFFER_SIZE) {
      ARA_LOG_MESSAGE(LOG_ERR,"%s : Whoops! we only had %d bytes but we are already unpacking up to %d bytes (buffer size %d)\n",__FUNCTION__, up_to_byte_input,numBytesIn,MAX_EVENT_BUFFER_SIZE);
      return -1;
    }


    if((frame_type == FIRST_BLOCK_FRAME_OTHER) ||
       (frame_type == ONLY_BLOCK_FRAME_OTHER)){
      if(frame_number!=0 || expected_frame_number!=0){
	//We weren't expecting this frame
	//fprintf(stderr,  "%s :  expected_frame_number %d frame_number %d frame_type %c\n", __FUNCTION__, expected_frame_number, frame_number, frame_type);
	ARA_LOG_MESSAGE(LOG_DEBUG, "%s :  expected_frame_number %d frame_number %d frame_type %c\n", __FUNCTION__, expected_frame_number, frame_number, frame_type);
	return -1;
      }

      //Event Header
      temp_value=inputBuffer[up_to_byte_input++];
      evtHeaderPtr->versionNumber= (temp_value<<8) | (inputBuffer[up_to_byte_input++]);
      temp_value=inputBuffer[up_to_byte_input++];
      evtHeaderPtr->ppsNumber = (temp_value << 8) |(inputBuffer[up_to_byte_input++]);


      evtHeaderPtr->timeStamp=0;
      temp_value=inputBuffer[up_to_byte_input++];
      evtHeaderPtr->timeStamp |= (temp_value << 24); 
      temp_value=inputBuffer[up_to_byte_input++];
      evtHeaderPtr->timeStamp |= (temp_value << 16);
      temp_value=inputBuffer[up_to_byte_input++];
      evtHeaderPtr->timeStamp |= (temp_value << 8);
      temp_value=inputBuffer[up_to_byte_input++];
      evtHeaderPtr->timeStamp |= temp_value;
      //NB the timestamp is in gray code - this is changed to binary in AraRoot

      temp_value=inputBuffer[up_to_byte_input++];
      evtHeaderPtr->eventId = (temp_value << 8) | (inputBuffer[up_to_byte_input++]);
      //Event Header


      //fprintf(stderr,  "%s : Event Header\n", __FUNCTION__);
      //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Event Header\n", __FUNCTION__);
      //fprintf(stderr, "%s : versionNumber %d ppsNumber %d timeStamp %d eventId %d\n", __FUNCTION__, evtHeaderPtr->versionNumber, evtHeaderPtr->ppsNumber, evtHeaderPtr->timeStamp, evtHeaderPtr->eventId);
      //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : versionNumber %d ppsNumber %d timeStamp %d eventId %d\n", __FUNCTION__, evtHeaderPtr->versionNumber, evtHeaderPtr->ppsNumber, evtHeaderPtr->timeStamp, evtHeaderPtr->eventId);
    }//First or only frame
    
    if(expected_frame_number==frame_number){

      //Got the frame we expected
      //      fprintf(stderr, "%s : frame_type %c frame_number %d frame_length %d\n", __FUNCTION__, frame_type, frame_number, frame_length);
      //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : frame_type %c frame_number %d frame_length %d\n", __FUNCTION__, frame_type, frame_number, frame_length);

      //l4_triggers
      l4_triggers_new = inputBuffer[up_to_byte_input++];
      l4_triggers = inputBuffer[up_to_byte_input++];
    
      //unpack l4 triggers
      int bitMask=0;
      for(bitMask=0;bitMask<MAX_TRIG_BLOCKS;bitMask++){
	if(((l4_triggers_new>>bitMask)&0x01)==0x01){
	  l4_trigger_info=0;
	  temp_value = 0;
	  temp_value = inputBuffer[up_to_byte_input++];
	  l4_trigger_info |= (temp_value <<24);
	  temp_value = inputBuffer[up_to_byte_input++];
	  l4_trigger_info |= (temp_value <<16);
	  temp_value = inputBuffer[up_to_byte_input++];
	  l4_trigger_info |= (temp_value <<8);
	  temp_value = inputBuffer[up_to_byte_input++];
	  l4_trigger_info |= (temp_value);

	  if(bitMask==triggerL4_CPU){
	    l4_trigger_info = 0x01;
	  }

	  evtHeaderPtr->triggerInfo[bitMask]=l4_trigger_info;
	  evtHeaderPtr->triggerBlock[bitMask]=frame_number;
	}
      }//unpack l4 triggers

      //block_info
      temp_value = inputBuffer[up_to_byte_input++];
      block_info = (temp_value << 8) | inputBuffer[up_to_byte_input++];
      block_id = (block_info & 0x1ff); //block_id [8:0]
      dda_mask = (block_info >> 9) &0xf;      


      //      fprintf(stderr, "%s : block_info %d block_id %d dda_mask %d\n", __FUNCTION__, block_info, block_id, dda_mask);
      //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : block_info %d block_id %d dda_mask %d\n", __FUNCTION__, block_info, block_id, dda_mask);

      //Now check each bit in dda_mask and process the enabled dda's channels
      uint8_t dda_mask_bit=0;
 
     //dda loop
      for(dda_mask_bit=0;dda_mask_bit<DDA_PER_ATRI;dda_mask_bit++)
	{
	  if(!(dda_mask & (0x1<<dda_mask_bit))) {
	    //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : dda %d disabled\n", __FUNCTION__, dda_mask_bit);
	    continue;
	  }
	  //	  fprintf(stderr, "%s : dda %d enabled\n", __FUNCTION__, dda_mask_bit);
	  //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : dda %d enabled\n", __FUNCTION__, dda_mask_bit);

	  //Got a block to fill
	  blkHeaderPtr = (AraStationEventBlockHeader_t*)&outputBuffer[up_to_byte_output];
	  up_to_byte_output+=sizeof(AraStationEventBlockHeader_t);
	  numReadOutBlocks++;
	  //Ready to fill


	  temp_value = inputBuffer[up_to_byte_input++];
	  block_header = (temp_value << 8) | (inputBuffer[up_to_byte_input++]);
	  channel_mask = (block_header & 0xff);
	  uint8_t this_dda=0;
	  this_dda = (block_header >> 8) & 0x3;

	  blkHeaderPtr->irsBlockNumber=block_id;
	  blkHeaderPtr->channelMask=block_header; //DDA [9:8] Channels [7:0]

	  //	  fprintf(stderr, "%s : block_header %d channel_mask %2.2x this_dda %d\n", __FUNCTION__, block_header, channel_mask, this_dda);
	  //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : block_header %d channel_mask %2.2x this_dda %d\n", __FUNCTION__, block_header, channel_mask, this_dda);

	  if((dda_mask_bit ) != this_dda){
	    //	    fprintf(stderr, "%s : Wrong DDA got %d expected %d\n", __FUNCTION__, this_dda,  dda_mask_bit);
	    //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Wrong DDA got %d expected %d\n", __FUNCTION__, this_dda,  dda_mask_bit);
	    return -1;
	  }
	  
	  uint8_t channel_mask_bit=0;
	  //channel loop
	  for(channel_mask_bit=0;channel_mask_bit<RFCHAN_PER_DDA;channel_mask_bit++){
	    //remember that the channel_mask is a disable - any high bits mean the channel is off
	    if(((channel_mask >> channel_mask_bit) & 0x01) != 0x01){
	      //channel disabled
	      continue;
	    }
	    blkChanPtr = (AraStationEventBlockChannel_t*)&outputBuffer[up_to_byte_output];
	    up_to_byte_output+=sizeof(AraStationEventBlockChannel_t);
	    int sample=0;
	    for(sample=0;sample<SAMPLES_PER_BLOCK;sample++){
	      temp_value=inputBuffer[up_to_byte_input++];
	      blkChanPtr->samples[sample]=(temp_value<<8) | (inputBuffer[up_to_byte_input++]);
	    }
	  }//channel loop


	  if(up_to_byte_input>numBytesIn || up_to_byte_input>MAX_EVENT_BUFFER_SIZE) {
	    ARA_LOG_MESSAGE(LOG_ERR,"%s : Whoops! we only had %d bytes but we are already unpacking up to %d bytes (buffer size %d) (dda loop %d)\n",__FUNCTION__, up_to_byte_input,numBytesIn,MAX_EVENT_BUFFER_SIZE,dda_mask_bit);
	    return -1;
	  }
	  if(up_to_byte_output>MAX_EVENT_BUFFER_SIZE) {
	    ARA_LOG_MESSAGE(LOG_ERR,"%s : Whoops! we only have an outputBuffer of %d bytes but we are already unpacking up to %d bytes into it (dda loop %d)\n",__FUNCTION__, MAX_EVENT_BUFFER_SIZE,up_to_byte_output,dda_mask_bit);
	    return -1;
	  }   
	  

	} //dda loop
      
      expected_frame_number++;


      //finish the event
      if((frame_type==LAST_BLOCK_FRAME_OTHER)||
	 (frame_type==ONLY_BLOCK_FRAME_OTHER)){

	//	fprintf(stderr, "%s : Finished event\n", __FUNCTION__);
	//ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Finished event\n", __FUNCTION__);
	evtHeaderPtr->numReadoutBlocks = numReadOutBlocks;
	evtHeaderPtr->numBytes=up_to_byte_output - sizeof(AraStationEventHeader_t);
	//	fprintf(stderr, "%s : Num blocks %d Event Size: %d %d\n", __FUNCTION__, evtHeaderPtr->numReadoutBlocks, evtHeaderPtr->numBytes, up_to_byte_output);
	ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Num blocks %d Event Size: %d %d\n", __FUNCTION__, evtHeaderPtr->numReadoutBlocks, evtHeaderPtr->numBytes, up_to_byte_output);
	return up_to_byte_output;
      }

    }//all frames

    else{
      //Didn't get the expected frame_number
      //      fprintf(stderr, "%s :  expected_frame_number %d frame_number %d frame_type %c\n", __FUNCTION__, expected_frame_number, frame_number, frame_type);
      ARA_LOG_MESSAGE(LOG_DEBUG, "%s :  expected_frame_number %d frame_number %d frame_type %c\n", __FUNCTION__, expected_frame_number, frame_number, frame_type);
      return -1;
    }
    
  }//while(1)
  
  return -1;

}

int checkAtriEventBuffer(unsigned char *eventBuffer){
  //This function is to check that we are writing to the buffer OK
  uint16_t data=0;
  //  uint16_t temp=0;
  int header_bytes=24;
  int data_bytes=1048;
  int byte=0;

  fprintf(stderr, "%s : Header information\n", __FUNCTION__);
  for(byte=0;byte<header_bytes;byte++){
    data = eventBuffer[byte];
    fprintf(stderr, "%2.2x ", data);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "%s : Data\n", __FUNCTION__);
  for(byte=header_bytes;byte<data_bytes;byte++){
    data = eventBuffer[byte];
    fprintf(stderr, "%2.2x ", data);
  }
  fprintf(stderr, "\n");
    
  return 0;  
  
}

	    
int readAtriEvent(char *eventBuffer)
{
  //  if(theConfig.usePatrickEvent) return readAtriEventPatrick(eventBuffer);

  //Try to read an event, returns 0 if there is no event, num bytes if there is an event and -1 if there is a protocol error
  int retVal=0;
  int numBytesRead,i;
  int uptoByte=0;
  
  unsigned char buffer[512];

  int gotEventStart=0;
  int uptoByteOutput=0;
  int expectFrame=0;
  int eventLength=0;
  int expectStack=0;
  int expectStart=1;
  int chanCounter=0;
  int ddaNumber=0;

  int panicCounter=0;
  //  uint32_t eventId;
  //  uint16_t blockIdq;
  //  uint16_t tempTrigPattern;
  //  uint16_t tempTrigInfo;
  uint16_t versionNumber;
  uint32_t tempVal;
  int uptoSample=0;
  int numBlocks[DDA_PER_ATRI]={0};
  unsigned char expectFrameByte=0;

  //Commented out RJN 25/02/13
  //  unsigned char doneTrigPattern=0;
  //  unsigned char currentTrigPattern=0;
  //  int testBit=0;
  //  int testMask=0;

#ifdef CLOCK_DEBUG
  unsigned long long startTime=rdtsc();
  unsigned long long preReadTime;
  unsigned long long postReadTime;
  int clockReadCounter=0;

  char clockFile[FILENAME_MAX];
  sprintf(clockFile,"/tmp/clock_%d.txt",fCurrentEvent);
  FILE *fpClock = fopen(clockFile,"w");
#endif

  AraStationEventHeader_t *evtHeaderPtr=(AraStationEventHeader_t*)eventBuffer;
  AraStationEventBlockHeader_t *blkHeaderPtr=NULL;
  AraStationEventBlockChannel_t *blkChanPtr=NULL;

  //  fprintf(stderr,"Num requests %d, Num buffers %d\n",getNumAsyncRequests(),getNumAsyncBuffers());
  

  while (1) {
#ifdef CLOCK_DEBUG
    preReadTime=rdtsc();
#endif
    // RJN change to event end point
    retVal=readEventEndPoint(buffer,512,&numBytesRead);
    //    retVal=readEventEndPointFromSoftwareBuffer(buffer,512,&numBytesRead);
#ifdef CLOCK_DEBUG
    postReadTime=rdtsc();
    if(fpClock)fprintf(fpClock,"%d %llu %llu %llu\n",clockReadCounter,startTime,preReadTime,postReadTime);
    clockReadCounter++;
#endif

    if(retVal==0) {
      if(numBytesRead>0) {
	ARA_LOG_MESSAGE(LOG_INFO,"Got frame %#x %#x\n",buffer[0],buffer[1]);
	//Log the event
	if(fpAtriEventLog!=NULL && theConfig.atriEventLog) {
	  for (i=0;i<numBytesRead;i++) 
	    {
              fprintf(fpAtriEventLog,"%2.2x", buffer[i]); 
              if (!((i+1)%16) || (i+1==numBytesRead)) fprintf(fpAtriEventLog, "\n"); 
              else fprintf(fpAtriEventLog," ");      
	    }
	  fprintf(fpAtriEventLog,"\n");
	}

	//	fprintf(stderr,"\nRead %d bytes from event end point\n",numBytesRead);	 	  
	//	fprintf(stderr,"expectFrame %d, expectStack %d, expectFrameByte %#x\n",expectFrame,expectStack,expectFrameByte); 

	//	fprintf(stderr,"Start: %#x %#x\n",buffer[0],buffer[1]);
	//Try and unpack the event 
     	if(expectFrame==0) { 
	  //e
	  //b
	  //f
	  //o
	  int goodFrame=0;   
	  eventLength = buffer[2]; 
	  
	  tempVal=buffer[3];
	  eventLength |= (tempVal << 8); 
	  
	  //e,E
	  uptoByte=4;

	  if((buffer[0]==FIRST_BLOCK_FRAME_START && buffer[1]==0) ||
	     (buffer[0]==ONLY_BLOCK_FRAME_START && buffer[1]==0)) {
     	    //Correct frame number
	    if(!expectStart && expectStack==0) {
	      ARA_LOG_MESSAGE(LOG_WARNING,"Got FIRST_BLOCK_FRAME_START when we weren't expecting it\n");
	    }
	    else {
	      ARA_LOG_MESSAGE(LOG_INFO,"Got FIRST_BLOCK_FRAME_START\n");
	    }
	    expectStart=0;
	    if(!gotEventStart) {
	      //Reset the output buffer
	      memset(evtHeaderPtr,0,sizeof(AraStationEventHeader_t));
	      uptoByteOutput+=sizeof(AraStationEventHeader_t);	     

	      for(i=0;i<DDA_PER_ATRI;i++)
		numBlocks[i]=0;
	      gotEventStart=1;
	    }

	    goodFrame=1;
	    if(buffer[0]==FIRST_BLOCK_FRAME_START)
	      expectFrameByte=FIRST_BLOCK_FRAME_OTHER;
	    if(buffer[0]==ONLY_BLOCK_FRAME_START)
	      expectFrameByte=ONLY_BLOCK_FRAME_OTHER;
	    
	    //	    fprintf(stderr,"versionNumber %#x %#x -- uptoByte=%d\n",buffer[uptoByte],buffer[uptoByte+1],uptoByte);
	    versionNumber=buffer[uptoByte++];
	    tempVal=buffer[uptoByte++];
	    versionNumber|=(tempVal<<8);
	    

	    //	    if(expectStack==0) {
	    evtHeaderPtr->versionNumber=versionNumber;
	    
	    //Now comes the event header
	    if(versionNumber==1) {
	      //PPS Number
	      evtHeaderPtr->ppsNumber=buffer[uptoByte++];

	      tempVal=buffer[uptoByte++];
	      evtHeaderPtr->ppsNumber|=(tempVal<<8);
	      //Timestamp (need to check that this bit works)
	      evtHeaderPtr->timeStamp=buffer[uptoByte++];

	      tempVal=buffer[uptoByte++];
	      evtHeaderPtr->timeStamp|=(tempVal<<8);
	      tempVal=buffer[uptoByte++];
	      evtHeaderPtr->timeStamp|=(tempVal<<16);
	      tempVal=buffer[uptoByte++];
	      evtHeaderPtr->timeStamp|=(tempVal<<24);
	      evtHeaderPtr->timeStamp=grayToBinary(evtHeaderPtr->timeStamp);
	      //Event Id
	      evtHeaderPtr->eventId=buffer[uptoByte++];

	      tempVal=buffer[uptoByte++];
	      evtHeaderPtr->eventId|=(tempVal<<8);
	    }	  
	   
	  }
	  else if(buffer[0]==MIDDLE_BLOCK_FRAME_START && buffer[1]==0) {
	    if(gotEventStart) {
	      goodFrame=1;
	      expectFrameByte=MIDDLE_BLOCK_FRAME_OTHER;
	    }
	  }
	  else if(buffer[0]==LAST_BLOCK_FRAME_START && buffer[1]==0) {
	    if(gotEventStart) {
	      goodFrame=1;
	      expectFrameByte=LAST_BLOCK_FRAME_OTHER;
	    }
	  }
	  ARA_LOG_MESSAGE(LOG_INFO,"Frame %#x %d is goodFrame %d\n",buffer[0],buffer[1],goodFrame);
	  if(goodFrame) {
 
	    //Now comes the block header
	    blkHeaderPtr=(AraStationEventBlockHeader_t*)&eventBuffer[uptoByteOutput];
	    uptoByteOutput+=sizeof(AraStationEventBlockHeader_t);
	    //irsBlockNumber
	    //fprintf(stderr,"irsBlockNumber %#x %#x -- uptoByte=%d\n",buffer[uptoByte],buffer[uptoByte+1],uptoByte);
	    blkHeaderPtr->irsBlockNumber=buffer[uptoByte++];
	    
	    tempVal=buffer[uptoByte++];
	    blkHeaderPtr->irsBlockNumber|=(tempVal<<8);


	    //channelMask
	    blkHeaderPtr->channelMask=buffer[uptoByte++];
	    tempVal=buffer[uptoByte++];
	    blkHeaderPtr->channelMask|=(tempVal<<8);
	    ARA_LOG_MESSAGE(LOG_INFO,"Got block header version %d irsBlockNumber %#x channelMask %#x\n",
		    versionNumber,blkHeaderPtr->irsBlockNumber,blkHeaderPtr->channelMask);
	    ddaNumber=(blkHeaderPtr->channelMask&0xf00)>>8;
	    numBlocks[ddaNumber]++;
	    ARA_LOG_MESSAGE(LOG_INFO,"Num Blocks %d %d %d %d\n",numBlocks[0],numBlocks[1],numBlocks[2],numBlocks[3]);


	    /* if(versionNumber>1) { */
	    /*   //Now comes the trigger pattern(s)	     */
	    /*   currentTrigPattern=((blkHeaderPtr->irsBlockNumber)>>9)&0xf; */
	    /*   testBit=0; */
	    /*   testMask=1; */
	    /*   while(doneTrigPattern!=currentTrigPattern) { */
	    /* 	testMask=(1<<testBit); */
	    /* 	if((currentTrigPattern&testMask) && !(doneTrigPattern&testMask)) { */
	    /* 	  //Expect a new trigger */
	    /* 	  evtHeaderPtr->triggerPattern[testBit]=(buffer[uptoByte++]); */

	    /* 	  tempVal=buffer[uptoByte++]; */
	    /* 	  evtHeaderPtr->triggerPattern[testBit]|=(tempVal<<8); */
	    /* 	  evtHeaderPtr->triggerInfo[testBit]=(buffer[uptoByte++]); */

	    /* 	  tempVal=buffer[uptoByte++]; */
	    /* 	  evtHeaderPtr->triggerInfo[testBit]|=(tempVal<<8); */
	    /* 	  doneTrigPattern|=testMask; */
	    /* 	} */
	    /*   } */
	    /* } */
	    
	    //Now comes the sample data
	    chanCounter=0;
	    //	    fprintf(stderr,"Got channel %d\n",chanCounter);
	    //    fprintf(stderr,"Got channel %d (uptoByte=%d)\n",chanCounter,uptoByte);
	    blkChanPtr=(AraStationEventBlockChannel_t*)&eventBuffer[uptoByteOutput];
	    //	    fprintf(stderr,"Got channel %d (uptoByte=%d  output=%d)\n",chanCounter,uptoByte,uptoByteOutput);
	    uptoByteOutput+=sizeof(AraStationEventBlockChannel_t);
	    


	    uptoSample=0;
	    while(uptoByte+1<numBytesRead) {
	      blkChanPtr->samples[uptoSample]=buffer[uptoByte++];
	      tempVal=buffer[uptoByte++];
	      blkChanPtr->samples[uptoSample]|=(tempVal<<8);
	      //	      if(uptoSample<2)
	      //		fprintf(stderr,"chan %d samp %d --  %d\n",chanCounter,uptoSample,blkChanPtr->samples[uptoSample]);

	      uptoSample++;
	      if(uptoSample==SAMPLES_PER_BLOCK) {
		chanCounter++;		
		if(chanCounter<8) {
		  blkChanPtr=(AraStationEventBlockChannel_t*)&eventBuffer[uptoByteOutput];
		  //		  fprintf(stderr,"Got channel %d (uptoByte=%d  output=%d)\n",chanCounter,uptoByte,uptoByteOutput);
	
		  uptoByteOutput+=sizeof(AraStationEventBlockChannel_t);
		  uptoSample=0;
		}

		//		fprintf(stderr,"Got channel %d (uptoByte=%d  output=%d)\n",chanCounter,uptoByte,uptoByteOutput);
	      }
	    }	    

	    expectFrame++; 	    
	  }
	  else {
	    ARA_LOG_MESSAGE(LOG_ERR,"Badly formed frame\n");
	    return -1;
	  }	  
	}
	else {
	  int goodFrame=0;

	  eventLength = buffer[2]; 

	  tempVal=buffer[3];
	  eventLength |= (tempVal << 8); 
	  uptoByte=4;

	  if(buffer[0]==expectFrameByte && buffer[1]==expectFrame) { 
	    //Now comes the sample data
	    goodFrame=1;

	    while(uptoByte+1<numBytesRead) {
	      blkChanPtr->samples[uptoSample]=buffer[uptoByte++];
	      tempVal=buffer[uptoByte++];
	      blkChanPtr->samples[uptoSample]|=(tempVal<<8);
	      //	      if(uptoSample<2)
		//		fprintf(stderr,"chan %d samp %d --  %d\n",chanCounter,uptoSample,blkChanPtr->samples[uptoSample]);
	      uptoSample++;
	      if(uptoSample==SAMPLES_PER_BLOCK) {		
		chanCounter++;
		if(chanCounter<8) {
		  blkChanPtr=(AraStationEventBlockChannel_t*)&eventBuffer[uptoByteOutput];
		  //		  fprintf(stderr,"Got channel %d (uptoByte=%d  output=%d)\n",chanCounter,uptoByte,uptoByteOutput);
		  uptoByteOutput+=sizeof(AraStationEventBlockChannel_t);
		  uptoSample=0;
		}

		//		fprintf(stderr,"Got channel %d (uptoByte=%d  output=%d)\n",chanCounter,uptoByte,uptoByteOutput);
	      }
	    }	    
	    
	    //if end
	    if(eventLength==numBytesRead-4) {
	      goodFrame=2;
	    }
	  }


	  if(goodFrame==1) 
	    expectFrame++;
	  else if(goodFrame==2) {
	    //Frame finished
	    expectFrame=0;
	    expectStack++;
	    ARA_LOG_MESSAGE(LOG_DEBUG,"Read %d stack (expect %d total)\n", expectStack, theConfig.numEnabledStacks);
	    if(expectStack==theConfig.numEnabledStacks) {
	      ARA_LOG_MESSAGE(LOG_DEBUG, "Finished block set (type: %c, end on %c/%c)\n",
			      buffer[0], ONLY_BLOCK_FRAME_OTHER, LAST_BLOCK_FRAME_OTHER);
	      //Finished this set of blocks
	      expectStack=0;
	      if(buffer[0]==ONLY_BLOCK_FRAME_OTHER || buffer[0]==LAST_BLOCK_FRAME_OTHER) {
		//Finished event

		fEventHeader->numReadoutBlocks=numBlocks[0]+numBlocks[1]+numBlocks[2]+numBlocks[3];
		ARA_LOG_MESSAGE(LOG_INFO,"Num Blocks %d %d %d %d -- %d\n",numBlocks[0],numBlocks[1],numBlocks[2],numBlocks[3],fEventHeader->numReadoutBlocks);
		
		fEventHeader->numBytes=uptoByteOutput-sizeof(AraStationEventHeader_t);
		ARA_LOG_MESSAGE(LOG_INFO,"Event size: %d %d\n",fEventHeader->numBytes,uptoByteOutput);

#ifdef CLOCK_DEBUG
		if(fpClock)fclose(fpClock);
#endif
		return uptoByteOutput;

	      }
	    }
	  }
	  else {
	    ARA_LOG_MESSAGE(LOG_ERR,"Badly formed frame\n");
#ifdef CLOCK_DEBUG
	    if(fpClock)fclose(fpClock);
#endif
	    return -1;
	  }
	}
     
      }
      else {
	if(gotEventStart) {
	  //	  ARA_LOG_MESSAGE(LOG_INFO,"Couldn't read new frame\n");
	  panicCounter++;
	  usleep(1);
	  if(panicCounter>1000) {
	    ARA_LOG_MESSAGE(LOG_ERR,"Giving up after failing to read the end of the event\n");
#ifdef CLOCK_DEBUG
	    if(fpClock)fclose(fpClock);
#endif
	    return -1;
	  }
	}
	else {
#ifdef CLOCK_DEBUG
	  if(fpClock)fclose(fpClock);
#endif
	  return 0;
	}
      }      
    }
    else {
      ARA_LOG_MESSAGE(LOG_ERR,"readEventEndPoint returned %d\n",retVal);
#ifdef CLOCK_DEBUG
      if(fpClock)fclose(fpClock);
#endif
      return -1;
    }
  }
}



int setIceThresholds(int fAtriSockFd, uint16_t *thresholds)
{
  int stack,ant,retVal=0;
  for(stack=D1;stack<=D4;stack++) {  
    if (theConfig.stackEnabled[stack]) {
      ARA_LOG_MESSAGE(LOG_DEBUG,"Setting thresholds on stack %s to %d %d %d %d\n",
	      atriDaughterStackStrings[stack],
	      thresholds[(stack*ANTS_PER_TDA)+0],
	      thresholds[(stack*ANTS_PER_TDA)+1],
	      thresholds[(stack*ANTS_PER_TDA)+2],
	      thresholds[(stack*ANTS_PER_TDA)+3]);
      
      for(ant=0;ant<ANTS_PER_TDA;ant++) {
	currentThresholds[(stack*ANTS_PER_TDA)+ant]=thresholds[(stack*ANTS_PER_TDA)+ant];
	//	fprintf("%s : currentThresholds[%i] %i\n", (stack*ANTS_PER_TDA)+ant, currentThresholds[(stack*ANTS_PER_TDA)+ant]);
      }
      retVal+=setAtriThresholds(fAtriSockFd,stack,&(thresholds[stack*ANTS_PER_TDA]));
    } else {
      ARA_LOG_MESSAGE(LOG_DEBUG, "Skipping stack %s since it's not enabled\n",
		      atriDaughterStackStrings[stack]);
    }
  }

  /* int i=0; */
  /* for(i=0;i<16;i++){ */
  /*   if(i==0)  fprintf(stderr, "currentThresholds %d,", currentThresholds[i]); */
  /*   else if(i<15) fprintf(stderr, " %d,", currentThresholds[i]); */
  /*   else  fprintf(stderr, " %d\n", currentThresholds[i]); */
  /* } */
  
  
  return retVal;
}



int setSurfaceThresholds(int fAtriSockFd, uint16_t *thresholds)
{
  int stack=theConfig.surfaceBoardStack;
  int ant,retVal=0;
  /* fprintf(stderr,"Setting surface thresholds on stack %s to %d %d %d %d\n", */
  /* 	  atriDaughterStackStrings[stack], */
  /* 	  thresholds[0], */
  /* 	  thresholds[1], */
  /* 	  thresholds[2], */
  /* 	  thresholds[3]); */
  
  for(ant=0;ant<ANTS_PER_TDA;ant++) {
    currentSurfaceThresholds[ant]=thresholds[ant];
  }

  retVal+=setSurfaceAtriThresholds(fAtriSockFd,stack,thresholds);
  return retVal;
}


int doThresholdScan(int fAtriSockFd)
{
  //First kind of scan will be all channels together
  FILE *fpThresholdScan;
  char filename[FILENAME_MAX];
  uint16_t thresholds[16]={0};
  uint32_t ppsCounter;
  uint32_t lastPpsCounter;
  uint16_t l1Scalers[16];
  uint16_t l1SurfaceScalers[4];
  uint32_t value=0,i=0;
  int j=0;
  sprintf(filename,"%s/current/thresholdScan.run%6.6d.dat",theConfig.topDataDir,fCurrentRun);
  fpThresholdScan=fopen(filename,"w");
  for(i=0;i<theConfig.thresholdScanPoints;i++) {
    value=theConfig.thresholdScanStep*i+theConfig.thresholdScanStartPoint;
    if(value > 0xffff) break; //Stops us looping back through low nums
    for(j=0;j<THRESHOLDS_PER_ATRI;j++) {
      thresholds[j]=value;
    }
    setIceThresholds(fAtriSockFd,thresholds);
    if(theConfig.setSurfaceThreshold)
      setSurfaceThresholds(fAtriSockFd,thresholds);

    //Read out PPS counter
    
    //PPS counter
    do{
      usleep(10000);
      atriWishboneRead(fAtriSockFd,ATRI_WISH_PPSCNT,4,(uint8_t*)&ppsCounter);
    } while(ppsCounter-lastPpsCounter==0);
    
    lastPpsCounter=ppsCounter;
    usleep(100000);

    //Read out scalers -- not sure this formatting works
    atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_L1_START,32,(uint8_t*)l1Scalers);

    //Read out surface scalers -- not sure this formatting works
    atriWishboneRead(fAtriSockFd,ATRI_WISH_SURF_SCAL_L1_START,8,(uint8_t*)l1SurfaceScalers);

    fprintf(fpThresholdScan,"%u %d ",ppsCounter,value);
    for(j=0;j<THRESHOLDS_PER_ATRI;j++) 
      fprintf(fpThresholdScan,"%d ",l1Scalers[j]);
    for(j=0;j<ANTS_PER_TDA;j++) 
      fprintf(fpThresholdScan,"%d ",l1SurfaceScalers[j]);
    fprintf(fpThresholdScan,"\n");
  }
  fclose(fpThresholdScan);
  if(theConfig.linkForXfer)
    makeLink(filename,theConfig.linkDir);

  return 0;
}


int doThresholdScanSingleChannel(int fAtriSockFd)
{
  //First kind of scan will be all channels together
  FILE *fpThresholdScan;
  uint32_t channel=theConfig.thresholdScanSingleChannelNum;
  char filename[FILENAME_MAX];
  uint16_t thresholds[16]={0};
  uint32_t ppsCounter;
  uint16_t l1Scalers[16];
  uint16_t l1SurfaceScalers[4];
  uint32_t value=0,i=0;
  int j=0;

  //set the other thresholds as usual
  setIceThresholds(fAtriSockFd,theConfig.thresholdValues);

  sprintf(filename,"%s/current/thresholdScanSingleChannel.run%6.6d.dat",theConfig.topDataDir,fCurrentRun);
  fpThresholdScan=fopen(filename,"w");
  for(i=0;i<theConfig.thresholdScanPoints;i++) {
    value=theConfig.thresholdScanStep*i+theConfig.thresholdScanStartPoint;
    if(value > 0xffff) break; //Stops us looping back through low nums
    for(j=0;j<THRESHOLDS_PER_ATRI;j++) {
      if(j==channel) thresholds[j]=value;
      else thresholds[j]=theConfig.thresholdValues[j];
    }
    setIceThresholds(fAtriSockFd,thresholds);
    if(theConfig.setSurfaceThreshold)
      setSurfaceThresholds(fAtriSockFd,thresholds);

    sleep(2);

    //Read out PPS counter
    
    //PPS counter
    atriWishboneRead(fAtriSockFd,ATRI_WISH_PPSCNT,4,(uint8_t*)&ppsCounter);

    //Read out scalers -- not sure this formatting works
    atriWishboneRead(fAtriSockFd,ATRI_WISH_SCAL_L1_START,32,(uint8_t*)l1Scalers);

    //Read out surface scalers -- not sure this formatting works
    atriWishboneRead(fAtriSockFd,ATRI_WISH_SURF_SCAL_L1_START,8,(uint8_t*)l1SurfaceScalers);

    fprintf(fpThresholdScan,"%u %d ",ppsCounter,value);
    for(j=0;j<THRESHOLDS_PER_ATRI;j++) 
      fprintf(fpThresholdScan,"%d ",l1Scalers[j]);
    for(j=0;j<ANTS_PER_TDA;j++) 
      fprintf(fpThresholdScan,"%d ",l1SurfaceScalers[j]);
    fprintf(fpThresholdScan,"\n");
  }
  fclose(fpThresholdScan);
  if(theConfig.linkForXfer)
    makeLink(filename,theConfig.linkDir);

  return 0;
}


int countTriggers(AraStationEventBlockHeader_t *blkHeader) 
{
  //Need to check the magic 27 number here
  int bit=0;
  int numTriggers=0;
  for(bit=0;bit<MAX_TRIG_BLOCKS;bit++) {
    if(blkHeader->irsBlockNumber & (1<<(27+bit))) {
      numTriggers++;
    }
  }
  return numTriggers;
}

int pedIndex(int dda, int block, int chan, int sample)
{
  return sample+(chan*SAMPLES_PER_BLOCK)+(block*RFCHAN_PER_DDA*SAMPLES_PER_BLOCK)+(dda*BLOCKS_PER_DDA*RFCHAN_PER_DDA*SAMPLES_PER_BLOCK);
}


int doPedestalRun(int fAtriSockFd,int fFx2SockFd)
{
  //  This  is a pedestal run will loop over all the blocks reading out pedestals
  FILE *fpPedestals;
  FILE *fpPedestalsRMS;
  char filename[FILENAME_MAX];
  char filenameWidth[FILENAME_MAX];
  sprintf(filename,"%s/current/pedestalValues.run%6.6d.dat",theConfig.topDataDir,fCurrentRun);
  fpPedestals=fopen(filename,"w");
  sprintf(filenameWidth,"%s/current/pedestalWidths.run%6.6d.dat",theConfig.topDataDir,fCurrentRun);
  fpPedestalsRMS=fopen(filenameWidth,"w");


  float *pedMean=malloc(sizeof(float)*DDA_PER_ATRI*BLOCKS_PER_DDA*RFCHAN_PER_DDA*SAMPLES_PER_BLOCK);
  //  fprintf(stderr,"pedMean %ld\n",(long)pedMean);
  float *pedMeanSq=malloc(sizeof(float)*DDA_PER_ATRI*BLOCKS_PER_DDA*RFCHAN_PER_DDA*SAMPLES_PER_BLOCK);
  //  fprintf(stderr,"pedMeanSq %ld\n",(long)pedMeanSq);
  
  //  float ****pedMean = (float ****) pedMean;
  //  float ****pedMeanSq = (float ****) pedMeanSq;


  int numBytesRead=0;
  uint16_t block=0;
  int event=0,dda=0,sample=0,chan=0;//,i=0;
  uint8_t pedestalMode=1;


  //  int sillyDouble=0;

  //Zero the arrays
  for(dda=0;dda<DDA_PER_ATRI;dda++) {
    for(block=0;block<BLOCKS_PER_DDA;block++) {
      for(chan=0;chan<RFCHAN_PER_DDA;chan++) {
	for(sample=0;sample<SAMPLES_PER_BLOCK;sample++) {
	  pedMean[pedIndex(dda,block,chan,sample)]=0;
	  pedMeanSq[pedIndex(dda,block,chan,sample)]=0;
	}
      }
    }
  }

  
  // One of the thing s we have to do is to disable the clocks
  // Register 10: Output buffer power.
  writeToI2CRegister(fFx2SockFd,0xd0,0x0A,0xf);

  

  //  int i=0;
  int eventByte=0;
  //  AraStationEventBlockHeader_t *blkHeader;
  AraStationEventBlockChannel_t *blkChannel;
  struct timeval nowTime;


  while(readAtriEventV2(fEventReadBuffer)) {
    fprintf(stderr,"Dumping event:\n");
  }
  
  //enable pedestal mode 
  theConfig.numSoftTriggerBlocks=1;

  
  pedestalMode=1;
  int retVal;//=atriWishboneWrite(fAtriSockFd,ATRI_WISH_IRSPED,2,(uint8_t*)&block);
  //  retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_IRSPEDEN,1,&pedestalMode);
  
  //Will change this at some point
  for(block=0;block<BLOCKS_PER_DDA;block++) {
  //  for(block=0;block<10;block++) {
    retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_IRSPED,2,(uint8_t*)&block); 
    usleep(50000);
    fprintf(stderr,"Starting pedestals for block %d\n",block);
    for(event=0;event<theConfig.pedestalSamples;event++) {
              
      pedestalMode=0x3;
      retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_IRSPEDEN,1,&pedestalMode);
      usleep(5000);

      //sendSoftwareTriggerWithInfo(fAtriSockFd, softTrigger_PED);
      sendSoftwareTrigger(fAtriSockFd);

      //      for(sillyDouble=0;sillyDouble<2;sillyDouble++) {
      retVal=0;
      do {
	retVal=readAtriEventV2(fEventReadBuffer);	
	if(retVal<0) {
	  ARA_LOG_MESSAGE(LOG_ERR,"Error reading event\n");
	  break;
	}
	else if(retVal>0) {
	  retVal=unpackAtriEventV2(fEventReadBuffer, fEventWriteBuffer,retVal);	
	  if(retVal<0) {
	    ARA_LOG_MESSAGE(LOG_ERR,"Error unpacking event\n");
	    break;
	  }
	}
	
      } while(retVal<1);
      eventByte=sizeof(AraStationEventHeader_t);

      //RJN no idea what this bit of code was for
      /* fprintf(stderr,"Press a key to continue\n"); */
      /* getc(stdin); */

      /* fprintf(stderr,"Got event: block "); */


      if(theConfig.pedestalDebugMode) {
	numBytesRead=retVal;
	
	gettimeofday(&nowTime,NULL);
	fEventHeader->unixTime=nowTime.tv_sec;
	fEventHeader->unixTimeUs=nowTime.tv_usec;
	fEventHeader->eventNumber=fCurrentEvent;
	fCurrentEvent++;

	//	fprintf(stderr,"Got event: %d\n",
	//		fEventHeader->eventNumber);

	//	fEventHeader->ppsNumber=fCurrentPps;	  
	fillGenericHeader(fEventHeader,ARA_EVENT_TYPE,numBytesRead);  
	
	// Store event
	int new_file_flag = 0;
	retVal = writeBuffer( &eventWriter, (char*)fEventWriteBuffer, numBytesRead , &(new_file_flag) );
	if( retVal != numBytesRead )
	  syslog(LOG_WARNING,"Failed to write event\n");
      }

      
      //Now do the pedestal calculation
      for(dda=0;dda<DDA_PER_ATRI;dda++) {
	
	//	blkHeader=(AraStationEventBlockHeader_t*)&fEventWriteBuffer[eventByte];
	eventByte+=sizeof(AraStationEventBlockHeader_t);
	//	fprintf(stderr,"%#x ",blkHeader->irsBlockNumber);
	
	
	for(chan=0;chan<RFCHAN_PER_DDA;chan++) {
	  blkChannel=(AraStationEventBlockChannel_t*)&fEventWriteBuffer[eventByte];
	  //	  fprintf(stderr,"dda %d chan %d startByte %d\n",dda,chan,eventByte);
	  eventByte+=sizeof(AraStationEventBlockChannel_t);
	  for(sample=0;sample<SAMPLES_PER_BLOCK;sample++) {	
	    //	    if(sample<2) {
	    //	      fprintf(stderr,"dda %d chan %d sample %d value %d\n",dda,chan,sample,blkChannel->samples[sample]);
	    //	    }

	    pedMean[pedIndex(dda,block,chan,sample)]+=(blkChannel->samples[sample]);
	    pedMeanSq[pedIndex(dda,block,chan,sample)]+=(blkChannel->samples[sample]*blkChannel->samples[sample]);
	    
	  }
	}
      }
      //      fprintf(stderr,"\n");
      
      
      
      //Now send ped clear


    }
    pedestalMode=0x0;
    retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_IRSPEDEN,1,&pedestalMode);
    usleep(50000);

  }



  int counter=0;
  while(1) {
    counter=0;
    do {
      counter++;
      retVal=readAtriEventV2(fEventReadBuffer);	
      if(retVal<0) {
	ARA_LOG_MESSAGE(LOG_ERR,"Error reading event\n");
      }
    } while(retVal<1 && counter<10);
    //    fprintf(stderr,"Counter %d\n",counter);
    if(retVal>0) {
      //      fprintf(stderr,"Dumping event\n");    
      //      sleep(1);
    }
    else { 
      break;
    }
    
    if(counter>10) break;

  }
  pedestalMode=0;
  retVal=atriWishboneWrite(fAtriSockFd,ATRI_WISH_IRSPEDEN,1,&pedestalMode);
    
  
  ///Now print out some stuff

  int mean;
  int rms;
  for(dda=0;dda<DDA_PER_ATRI;dda++) {
    for(block=0;block<BLOCKS_PER_DDA;block++) {
      for(chan=0;chan<RFCHAN_PER_DDA;chan++) {
	fprintf(fpPedestals,"%d %d %d",dda,block,chan);
	fprintf(fpPedestalsRMS,"%d %d %d",dda,block,chan);
	for(sample=0;sample<SAMPLES_PER_BLOCK;sample++) {
	  pedMean[pedIndex(dda,block,chan,sample)]/=theConfig.pedestalSamples;
	  pedMeanSq[pedIndex(dda,block,chan,sample)]/=theConfig.pedestalSamples;
	  
	  mean=rint(pedMean[pedIndex(dda,block,chan,sample)]);
	  rms=rint(sqrt(pedMeanSq[pedIndex(dda,block,chan,sample)]-pedMean[pedIndex(dda,block,chan,sample)]*pedMean[pedIndex(dda,block,chan,sample)]));
	  if(rms<0) rms=0;
	  //	  if(block<1 && sample<5 && chan<1) {
	  //	    fprintf(stderr,"Chan(%d %d %d %d -- %d) -- Mean %d, RMS %d\n",dda,block,chan,sample,pedIndex(dda,block,chan,sample),mean,rms);
	  //	  }
	  fprintf(fpPedestals," %d",mean);
	  fprintf(fpPedestalsRMS," %d",rms);
	}
	fprintf(fpPedestals,"\n");
	fprintf(fpPedestalsRMS,"\n");
      }
    }
  }


  fclose(fpPedestals);
  fclose(fpPedestalsRMS);
  if(theConfig.linkForXfer) {
    makeLink(filename,theConfig.linkDir);
    makeLink(filenameWidth,theConfig.linkDir);
  }


  free(pedMeanSq);
  free(pedMean);
  
  // Register 10: Output buffer power.
  writeToI2CRegister(fFx2SockFd,0xd0,0x0A,0x0);

  return 0;
}


int updateWilkinsonVdly(int fAtriSockFd,AraEventHk_t *eventPtr)
{
  if(!theConfig.enableVdlyServo) return 0;

  // Cleaned up a bit. The Wilkinson goal is now in nanoseconds.
  // wilkinsonToNanoseconds() converts it. 

  //MHz equals  4096*wilkinsonCounter./(69636*10*1e-9)
  //wilkinsonCounter = 5.9Mhz
  //50 Mhz is 0.6V
  //dacCounts 2.5V /65535
  //50MHz is 15728 dac counts
  //5Mhz is 1573 dacCounts
  
  //For now we will just do a sleazy adjust
  static uint32_t lastPPSUpdate=0;
  //  int retVal;
  //  uint8_t enableWilkinsonMonitor=0x0F;;
  static int lastVals[DDA_PER_ATRI]={0};
  static int stuckCount[DDA_PER_ATRI]={0};



  if(eventPtr->ppsCounter<lastPPSUpdate+4)
    return 0;
  lastPPSUpdate=eventPtr->ppsCounter;
  int tempNum=0;
  int stack=0;
  for(stack=D1;stack<=D4;stack++) {    
    unsigned int wilkCounterNs = 
      atriConvertWilkCounterToNs(eventPtr->wilkinsonCounter[stack]);
    int diff = (wilkCounterNs - theConfig.wilkinsonGoal);
    // Wilkinson counter's resolution is a little more than 1 ns, and
    // its standard deviation is about +/-2 counts so we'll guardband 
    // by about 10 ns.
    if((diff > 10) || (diff < -10)) {
      tempNum=wilkCounterNs;
      if(tempNum>0)  {
	// We're comparing ns here: so if it's too *low*, the value needs to
	// go *down*. If it's too high, the value needs to go *up*.
	// Rough guess is about 32767 DAC counts for 6830 ns. So that's
	// 5 DAC counts per ns.
	currentVdly[stack]+=5*(tempNum-theConfig.wilkinsonGoal);
	setIRSVdlyDACValue(fAtriSockFd,stack,currentVdly[stack]);
	ARA_LOG_MESSAGE(LOG_DEBUG,"Setting dda %d vdly to %d\n",stack,currentVdly[stack]);      
	// The stuck detection stuff isn't needed anymore: the counter is
	// free running.
	if(lastVals[stack]==eventPtr->wilkinsonCounter[stack]) {
	  stuckCount[stack]++;
	}
	lastVals[stack]=eventPtr->wilkinsonCounter[stack];
      }
    }
    else {
      stuckCount[stack]=0;
      lastVals[stack]=eventPtr->wilkinsonCounter[stack];
    }
  }
  
  return 1;
}




int doPedestalRunNotPedestalMode(int fAtriSockFd,int fFx2SockFd)
{
  //  This  is a pedestal run will loop over all the blocks reading out pedestals
  FILE *fpPedestals;
  FILE *fpPedestalsRMS;
  char filename[FILENAME_MAX];
  char filenameWidth[FILENAME_MAX];
  sprintf(filename,"%s/current/pedestalValues.run%6.6d.dat",theConfig.topDataDir,fCurrentRun);
  fpPedestals=fopen(filename,"w");
  sprintf(filenameWidth,"%s/current/pedestalWidths.run%6.6d.dat",theConfig.topDataDir,fCurrentRun);
  fpPedestalsRMS=fopen(filenameWidth,"w");


  float *pedMean=malloc(sizeof(float)*DDA_PER_ATRI*BLOCKS_PER_DDA*RFCHAN_PER_DDA*SAMPLES_PER_BLOCK);
  //  fprintf(stderr,"pedMean %ld\n",(long)pedMean);
  float *pedMeanSq=malloc(sizeof(float)*DDA_PER_ATRI*BLOCKS_PER_DDA*RFCHAN_PER_DDA*SAMPLES_PER_BLOCK);
  //  fprintf(stderr,"pedMeanSq %ld\n",(long)pedMeanSq);
  uint16_t *pedCount=malloc(sizeof(uint16_t)*DDA_PER_ATRI*BLOCKS_PER_DDA*RFCHAN_PER_DDA*SAMPLES_PER_BLOCK);
  
  //  float ****pedMean = (float ****) pedMean;
  //  float ****pedMeanSq = (float ****) pedMeanSq;


  int numBytesRead=0;
  uint16_t block=0;
  int realDda;
  int dda=0,sample=0,chan=0;//,i=0;


  //  int sillyDouble=0;

  //Zero the arrays
  for(dda=0;dda<DDA_PER_ATRI;dda++) {
    for(block=0;block<BLOCKS_PER_DDA;block++) {
      for(chan=0;chan<RFCHAN_PER_DDA;chan++) {
	for(sample=0;sample<SAMPLES_PER_BLOCK;sample++) {
	  pedMean[pedIndex(dda,block,chan,sample)]=0;
	  pedMeanSq[pedIndex(dda,block,chan,sample)]=0;
	  pedCount[pedIndex(dda,block,chan,sample)]=0;
	}
      }
    }
  }

  
  // One of the thing s we have to do is to disable the clocks
  // Register 10: Output buffer power.
  writeToI2CRegister(fFx2SockFd,0xd0,0x0A,0xf);

  

  //  int i=0;
  int eventByte=0;
  AraStationEventBlockHeader_t *blkHeader;
  AraStationEventBlockChannel_t *blkChannel;
  struct timeval nowTime;


  while(readAtriEventV2(fEventReadBuffer)) {
    fprintf(stderr,"Dumping event:\n");
  }
  

  int retVal,blockReadout;
  int gotBlock[512]={0};
  int count=0;
  int sentTrigger=0;
  int loopNum=0;
  int notDone=1;
  while(notDone && loopNum<2*512*theConfig.pedestalSamples) {   
    if(loopNum%100==0) {
      fprintf(stderr,"Loop number %d\n",loopNum);
      dda=0;
      chan=0;
      sample=0;
      for(block=0;block<BLOCKS_PER_DDA;block++) {
	fprintf(stderr,"%d ",pedCount[pedIndex(dda,block,chan,sample)]);
	
      }
      fprintf(stderr,"\n");
    }


    retVal=0;
    sentTrigger=0;
    count=0;
    do {
      retVal=readAtriEventV2(fEventReadBuffer);	
      if(retVal<0) {
	  ARA_LOG_MESSAGE(LOG_ERR,"Error reading event\n");
	  //	  break;
	  return -1;
      }
      if(retVal>0) {
	retVal=unpackAtriEventV2(fEventReadBuffer, fEventWriteBuffer,retVal);
	if(retVal<0){
	  ARA_LOG_MESSAGE(LOG_ERR,"Error unpacking event\n");
	  break;
	}
      }
      count++;
      if(count>10 || sentTrigger==0) {
	//retVal = sendSoftwareTriggerWithInfo(fAtriSockFd, softTrigger_PED);
	retVal = sendSoftwareTrigger(fAtriSockFd);

	sentTrigger=1;
      }
    } while(retVal<1);
    eventByte=sizeof(AraStationEventHeader_t);
    
    if(theConfig.pedestalDebugMode) {
      numBytesRead=retVal;
      
      gettimeofday(&nowTime,NULL);
      fEventHeader->unixTime=nowTime.tv_sec;
      fEventHeader->unixTimeUs=nowTime.tv_usec;
      fEventHeader->eventNumber=fCurrentEvent;
      fCurrentEvent++;

      //      fEventHeader->ppsNumber=fCurrentPps;	  
      fillGenericHeader(fEventHeader,ARA_EVENT_TYPE,numBytesRead);  
      // Store event
      int new_file_flag = 0;
      retVal = writeBuffer( &eventWriter, (char*)fEventWriteBuffer, numBytesRead , &(new_file_flag) );
      if( retVal != numBytesRead )
	syslog(LOG_WARNING,"Failed to write event\n");
    }
    
    
    //Now do the pedestal calculation
    for(blockReadout=0;blockReadout<theConfig.pedestalNumTriggerBlocks;blockReadout++) {
      for(dda=0;dda<DDA_PER_ATRI;dda++) {
	if (theConfig.stackEnabled[dda]) {		
	  blkHeader=(AraStationEventBlockHeader_t*)&fEventWriteBuffer[eventByte];
	  eventByte+=sizeof(AraStationEventBlockHeader_t);
	  //	  fprintf(stderr,"block number %#x\n",(blkHeader->irsBlockNumber)&0x1ff);
	  //	  fprintf(stderr, "%s : block number %i\n", __FUNCTION__, (blkHeader->irsBlockNumber)&0x1ff);
	  block=(blkHeader->irsBlockNumber)&0x1ff;
	  if(blockReadout>0) gotBlock[block]++;
	  realDda= (blkHeader->channelMask&0x300)>>8;
	  if(realDda!=dda  ) {
	    ARA_LOG_MESSAGE(LOG_ERR,"Didn't get the right dda: %d %d\n",dda,realDda);
	    break;
	  }
	  else {
	    for(chan=0;chan<RFCHAN_PER_DDA;chan++) {
	      blkChannel=(AraStationEventBlockChannel_t*)&fEventWriteBuffer[eventByte];
	      //	      fprintf(stderr,"dda %d chan %d startByte %d",dda,chan,eventByte);
	      eventByte+=sizeof(AraStationEventBlockChannel_t);
	      //	      float block_mean=0;
	      if(blockReadout>0){
		for(sample=0;sample<SAMPLES_PER_BLOCK;sample++) {	
		  //		block_mean+=(blkChannel->samples[sample])/SAMPLES_PER_BLOCK;
		  pedMean[pedIndex(dda,block,chan,sample)]+=(blkChannel->samples[sample]);
		  pedMeanSq[pedIndex(dda,block,chan,sample)]+=(blkChannel->samples[sample]*blkChannel->samples[sample]);
		  pedCount[pedIndex(dda,block,chan,sample)]++;
		}
		//	      fprintf(stderr," block_mean %f\n", block_mean);
	      }//blockReadout>0
	    }
	  }       
	  }
	}	 
      }
      notDone=0;
      for(dda=0;dda<DDA_PER_ATRI;dda++) {
	if (theConfig.stackEnabled[dda]) {	      
	  if(notDone) break;
	  for(block=0;block<BLOCKS_PER_DDA;block++) {
	    if(notDone) break;
	    for(chan=0;chan<RFCHAN_PER_DDA;chan++) {
	      if(notDone) break;
	      sample=0;
	      if(pedCount[pedIndex(dda,block,chan,sample)]<theConfig.pedestalSamples) {
		notDone=1;
		break;
	      }	    
	    }
	  }
	}
	
      } 
      loopNum++;
  }//while (notDone)
  
  ///Now print out some stuff
  int mean;
  int rms;
  for(dda=0;dda<DDA_PER_ATRI;dda++) {
    for(block=0;block<BLOCKS_PER_DDA;block++) {
      for(chan=0;chan<RFCHAN_PER_DDA;chan++) {
	fprintf(fpPedestals,"%d %d %d",dda,block,chan);
	fprintf(fpPedestalsRMS,"%d %d %d",dda,block,chan);
	for(sample=0;sample<SAMPLES_PER_BLOCK;sample++) {
	  if(pedCount[pedIndex(dda,block,chan,sample)]) {
	    /* if(block==0 && chan==0 && sample<2) { */
	    /*   fprintf(stderr,"%d %d %d %d %f %f %d %d\n",dda,block,chan,sample, */
	    /* 	      pedMean[pedIndex(dda,block,chan,sample)], */
	    /* 	      pedMeanSq[pedIndex(dda,block,chan,sample)], */
	    /* 	      pedCount[pedIndex(dda,block,chan,sample)], */
	    /* 	      pedIndex(dda,block,chan,sample)); */
	    /* } */
	    
	    
	    pedMean[pedIndex(dda,block,chan,sample)]/=pedCount[pedIndex(dda,block,chan,sample)];
	    pedMeanSq[pedIndex(dda,block,chan,sample)]/=pedCount[pedIndex(dda,block,chan,sample)];
	  
	    mean=rint(pedMean[pedIndex(dda,block,chan,sample)]);
	    rms=rint(sqrt(pedMeanSq[pedIndex(dda,block,chan,sample)]-pedMean[pedIndex(dda,block,chan,sample)]*pedMean[pedIndex(dda,block,chan,sample)]));
	  }
	  else {
	    mean=2000;
	    rms=0;
	  }
	  if(rms<0) rms=0;
	  fprintf(fpPedestals," %d",mean);
	  fprintf(fpPedestalsRMS," %d",rms);
	}
	fprintf(fpPedestals,"\n");
	fprintf(fpPedestalsRMS,"\n");
      }
    }
  }


  fclose(fpPedestals);
  fclose(fpPedestalsRMS);
  if(theConfig.linkForXfer) {
    makeLink(filename,theConfig.linkDir);
    makeLink(filename,theConfig.linkDir);
  }

  free(pedMeanSq);
  free(pedMean);
  
  // Register 10: Output buffer power.
  writeToI2CRegister(fFx2SockFd,0xd0,0x0A,0x0);

  return 0;
}


int updateThresholdsUsingPID(int fAtriSockFd,AraEventHk_t *eventHkPtr)
{
  static int lastPpsNumber=0;
  static DacPidStruct_t thePids[NUM_L1_SCALERS];
  static int firstTime=1;
  uint16_t nextThresholds[NUM_L1_SCALERS];
  int error,value,change,proposed;
  float pTerm, dTerm, iTerm;
  int chanGoal;

  //Only update if thresholds have had effect
  if(lastPpsNumber>0 && (lastPpsNumber+2)>eventHkPtr->ppsCounter)
    return 0;
  
  /* for(tda=0;tda<TDA_PER_ATRI;tda++) { */
  /*   for(ant=0;ant<ANTS_PER_TDA;ant++) { */
  uint8_t l1num=0;
  for(l1num=0;l1num<16;l1num++){//FIXME

      if(firstTime) {
	thePids[l1num].iState=0;
	thePids[l1num].dState=0;
      }

      chanGoal=theConfig.scalerGoalValues[l1num];
      value=eventHkPtr->l1Scaler[l1num];
      error=chanGoal-value;
                 
      // Proportional term
      pTerm = theConfig.scalerPGain * error;
      
      // Calculate integral with limiting factors
      thePids[l1num].iState+=error;

      if (thePids[l1num].iState > theConfig.scalerIMax) 
	thePids[l1num].iState = theConfig.scalerIMax;
      else if (thePids[l1num].iState < theConfig.scalerIMin) 
	thePids[l1num].iState = theConfig.scalerIMin;
      
      // Integral and Derivative Terms
      iTerm = theConfig.scalerIGain * (float)(thePids[l1num].iState);  	
      if(!firstTime) {
	dTerm = theConfig.scalerDGain * (float)(value -thePids[l1num].dState);
      }
      else {
	dTerm=0;
      }
      thePids[l1num].dState = value;
      
      //Put them together		   
      change = (int) (pTerm + iTerm - dTerm);
      proposed=currentThresholds[l1num]+change;

      if(proposed<0) proposed=0;

      if(proposed<65535) 
	nextThresholds[l1num]=proposed;
      else 
	nextThresholds[l1num]=65535;

      
      if(l1num==0) {
	ARA_LOG_MESSAGE(LOG_DEBUG,"PID PPS %d\n",eventHkPtr->ppsCounter);
	ARA_LOG_MESSAGE(LOG_DEBUG,"Status %d %d %d\n",value,chanGoal,error);
	ARA_LOG_MESSAGE(LOG_DEBUG,"PID %f %f %f\n",pTerm,iTerm,-1*dTerm);
	ARA_LOG_MESSAGE(LOG_DEBUG,"Outcome %d %d %d\n",currentThresholds[l1num],nextThresholds[l1num],change);
      }

      //		printf("%d %d\n",thePids[l1num].dState,
      //		       thePids[l1num].iState);
      //		printf("%d %d %f %f %f\n",change,
      //		       thresholdArray[tdaIndex[tda]-1][ant],
	  //		       pTerm,iTerm,dTerm);
  

    //    }


  }


  lastPpsNumber=eventHkPtr->ppsCounter;
  
  if(firstTime) {
    firstTime=0;
    return 0;
  }
  setIceThresholds(fAtriSockFd,(uint16_t*)&nextThresholds[0]);

  return 1;
}


int updateSurfaceThresholdsUsingPID(int fAtriSockFd,AraEventHk_t *eventHkPtr)
{
  static int lastPpsNumber=0;
  static DacPidStruct_t thePids[ANTS_PER_TDA];
  static int firstTime=1;
  uint16_t nextThresholds[ANTS_PER_TDA];
  int ant,error,value,change,proposed;
  float pTerm, dTerm, iTerm;
  int chanGoal;

  //Only update if thresholds have had effect
  if(lastPpsNumber>0 && (lastPpsNumber+2)>eventHkPtr->ppsCounter)
    return 0;
  
  for(ant=0;ant<ANTS_PER_TDA;ant++) {
    if(firstTime) {
      thePids[ant].iState=0;
      thePids[ant].dState=0;
    }
    
    chanGoal=theConfig.surfaceGoalValues[ant];
    value=eventHkPtr->l1ScalerSurface[ant]; 
    error=chanGoal-value;
    
    // Proportional term
    pTerm = theConfig.scalerPGain * error;
    
    // Calculate integral with limiting factors
    thePids[ant].iState+=error;
    
    if (thePids[ant].iState > theConfig.scalerIMax) 
	thePids[ant].iState = theConfig.scalerIMax;
      else if (thePids[ant].iState < theConfig.scalerIMin) 
	thePids[ant].iState = theConfig.scalerIMin;
      
      // Integral and Derivative Terms
      iTerm = theConfig.scalerIGain * (float)(thePids[ant].iState);  	
      if(!firstTime) {
	dTerm = theConfig.scalerDGain * (float)(value -thePids[ant].dState);
      }
      else {
	dTerm=0;
      }
      thePids[ant].dState = value;
      
      //Put them together		   
      change = (int) (pTerm + iTerm - dTerm);
      proposed=currentSurfaceThresholds[ant]+change;
      if(proposed<65535) 
	nextThresholds[ant]=proposed;
      else 
	nextThresholds[ant]=65535;

      
      if(ant==0) {
	ARA_LOG_MESSAGE(LOG_DEBUG,"PID PPS %d\n",eventHkPtr->ppsCounter);
	ARA_LOG_MESSAGE(LOG_DEBUG,"Status %d %d %d\n",value,chanGoal,error);
	ARA_LOG_MESSAGE(LOG_DEBUG,"PID %f %f %f\n",pTerm,iTerm,-1*dTerm);
	ARA_LOG_MESSAGE(LOG_DEBUG,"Outcome %d %d %d\n",currentSurfaceThresholds[ant],nextThresholds[ant],change);
      }

      //		printf("%d %d\n",thePids[ant].dState,
      //		       thePids[ant].iState);
      //		printf("%d %d %f %f %f\n",change,
      //		       thresholdArray[tdaIndex-1][ant],
	  //		       pTerm,iTerm,dTerm);
  }
   


  lastPpsNumber=eventHkPtr->ppsCounter;
  
  if(firstTime) {
    firstTime=0;
    return 0;
  }
  setSurfaceThresholds(fAtriSockFd,nextThresholds);

  return 1;
}


unsigned long long rdtsc()
{
  #define rdtsc(low, high) \
         __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

  unsigned long low, high;
  rdtsc(low, high);
  return ((unsigned long long)high << 32) | low;
}


/// a) opens the connection to the FX2 microcontroller
void *libusbPollThreadHandler(void *ptr)
{  
  ARA_LOG_MESSAGE(LOG_DEBUG,"Starting libusbPollThreadHandler\n");
  //  int request=0;

  /* while (fProgramState!=ARA_PROG_TERMINATE) { */
  /*   if(fProgramState==ARA_PROG_RUNNING) { */
  /*     if(getNumAsyncRequests()<NUM_ASYNC_REQUESTS) */
  /* 	submitEventEndPointRequest(); */

  /*     pollForUsbEvents(); */
  /*   } */
  /* } */
  pthread_exit(NULL);

}


int doVdlyScan(int fAtriSockFd){

  fprintf(stderr, "%s : starting scan\n", __FUNCTION__);

  uint32_t vdlyMax = 0xffff;
  uint32_t vdlyStep = vdlyMax/100;
  uint32_t vdlyCurrent =0;
  uint8_t data[64];  
  uint16_t wilkinsonCounter[DDA_PER_ATRI];

  fprintf(stderr, "vdly\twilk[0]\twilk[1]\twilk[2]\twilk[3]\n");
  while(vdlyCurrent < vdlyMax)
    {
      int stack=0;
      for(stack=D1;stack<=D4;stack++){
	if(!theConfig.stackEnabled[stack]) continue;
	setIRSVdlyDACValue(fAtriSockFd,stack,vdlyCurrent);
	sleep(1);
	atriWishboneRead(fAtriSockFd,ATRI_WISH_D1WILK,16,data);   	copyTwoBytes(&(data[4*stack]),(uint16_t*)&wilkinsonCounter[stack]);      
	
      }
      fprintf(stderr, "\n%u\t", vdlyCurrent);
      for(stack=D1;stack<=D4;stack++){
	if(!theConfig.stackEnabled[stack]) continue;
	fprintf(stderr, "%u\t", atriConvertWilkCounterToNs(wilkinsonCounter[stack]));

      }      
      vdlyCurrent+=vdlyStep;
    }
  fprintf(stderr, "\nfinished vdlyScan\n");
  
  
  
  return 0;
  
}
int updateWilkinsonVdlyUsingPID(int fAtriSockFd,AraEventHk_t *eventHkPtr){
  static int lastPpsNumber=0;
  static DacPidStruct_t vdlyPIDs[DDA_PER_ATRI];
  static int firstTime=1;
  //  static int nextVals[DDA_PER_ATRI];
  static int iVals[DDA_PER_ATRI][SERVO_INT_PERIOD]={{0}};
  static int iCounter=0;
  int stack,error,value,change,proposed;
  float pTerm, dTerm, iTerm;
  int goal=theConfig.wilkinsonGoal;

  if(lastPpsNumber>0 && (lastPpsNumber)>eventHkPtr->ppsCounter)
    return 0;
  for(stack=D1;stack<=D4;stack++){
    if(!theConfig.stackEnabled[stack]) continue;
    if(firstTime) {
      vdlyPIDs[stack].iState=0;
      vdlyPIDs[stack].dState=0;
      setIRSVdlyDACValue(fAtriSockFd,stack,currentVdly[stack]);
      //      fprintf(stderr,"Setting dda %d vdly to %d\n",stack,currentVdly[stack]);   
    }
    value = atriConvertWilkCounterToNs(eventHkPtr->wilkinsonCounter[stack]);

    error=goal-value;
    
    // Proportional term
    pTerm = theConfig.vdlyPGain * error;
    
    // Calculate integral with limiting factors
    vdlyPIDs[stack].iState-=iVals[stack][iCounter%SERVO_INT_PERIOD];
    vdlyPIDs[stack].iState+=error;
    iVals[stack][iCounter%SERVO_INT_PERIOD]=error;
    iCounter++;
    
    if (vdlyPIDs[stack].iState > theConfig.vdlyIMax)
      vdlyPIDs[stack].iState = theConfig.vdlyIMax;
    else if (vdlyPIDs[stack].iState < theConfig.vdlyIMin)
      vdlyPIDs[stack].iState = theConfig.vdlyIMin;
    
    // Integral and Derivative Terms
    iTerm = theConfig.vdlyIGain * (float)(vdlyPIDs[stack].iState);
    if(!firstTime) {
      dTerm = theConfig.vdlyDGain * (float)(value -vdlyPIDs[stack].dState);
    }
    else {
      dTerm=0;
    }
    vdlyPIDs[stack].dState = value;
    
    //Put them together
    change = (int) (pTerm + iTerm - dTerm);
    proposed=currentVdly[stack]+change;
    //fprintf(stderr, "vdly - proposed %i change %i pTerm %f iTerm %f dTerm %f wilkinsonNs %i\n", proposed, change, pTerm, iTerm, dTerm, value);    
    if(proposed<20000) proposed=20000;

    if(!firstTime){
      if(proposed<65533){
	currentVdly[stack] = proposed;
	setIRSVdlyDACValue(fAtriSockFd,stack,currentVdly[stack]);
	//	fprintf(stderr,"Setting dda %d vdly to %d\n",stack,currentVdly[stack]);      
      }
      else{
	currentVdly[stack] = 65533;
	setIRSVdlyDACValue(fAtriSockFd,stack,currentVdly[stack]);
	//	fprintf(stderr,"Setting dda %d vdly to %d\n",stack,currentVdly[stack]);      
	
      }
    }
  }//stack
  
  lastPpsNumber=eventHkPtr->ppsCounter;
  firstTime=0;
  return 0;
}


void writeHelpfulTempFile()
{
  FILE *outFile=fopen("/tmp/araacqd.status","w");
  time_t rawTime;
  time(&rawTime);
  //Add the current time
  fprintf(outFile,"Date: %s\n",ctime(&rawTime));  
  fprintf(outFile,"Run: %d\n",fCurrentRun);
  fprintf(outFile,"Good event rate - %0.2f Hz\n",numGoodEvents*theConfig.eventRateReportRateHz);
  fprintf(outFile,"Good Events %d, Bad Events %d\n",numGoodEvents,numBadEvents);
  fprintf(outFile,"Last software trigger sent %d seconds ago\n",(int)(rawTime-lastSoftwareTrigger));
  fprintf(outFile,"Last event readout %d seconds ago\n",(int)(rawTime-lastEventRead));
  fprintf(outFile,"Next software event number %d\n",fCurrentEvent);
  fclose(outFile);  
}
