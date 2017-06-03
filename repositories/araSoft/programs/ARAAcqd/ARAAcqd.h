/*! \file ARAAcqd.h
  \brief The ARAAcqd program is responsible for interfacing with ATRI board.
    
  This is the ARAAcqd program which is responsible for interfacing with ATRI board via the Cypress FX2 microcontroller. The program has a number of threads with separate responsibilities. These include (at least at the moment):
  i) A thread that actually communicates with the FX2 over the control end points
  ii) A thread which monitors the unix domain atri_contol and adds requests to the pending queue
  iii) A thread which reads out the event data
  iv) The run control thread which monitors a seperate unix domain socket for sending and receiving run control packets
  v) A thread which reads out the housekeeping data

  July 2011 rjn@hep.ucl.ac.uk
*/

#ifndef ARAACQD_H
#define ARAACQD_H

#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>

#include "araSoft.h"
#include "araCom.h"
#include "araAtriStructures.h"
#include "araRunControlLib/araRunControlLib.h"

#define ARAACQD_VER_MAJOR 1
#define ARAACQD_VER_MINOR 6
#define ARAACQD_VER_REV   2084

#define SERVO_INT_PERIOD 10

typedef struct {
  // Verbosity
  int printToScreen;
  int logLevel;
  int atriControlLog;
  int atriEventLog;
  int enableEventRateReport;
  float eventRateReportRateHz;
  // Output files
  char topDataDir[FILENAME_MAX];
  char eventTopDir[FILENAME_MAX];
  char sensorHkTopDir[FILENAME_MAX];
  char eventHkTopDir[FILENAME_MAX];
  char pedsTopDir[FILENAME_MAX];
  char linkDir[FILENAME_MAX];
  char runLogDir[FILENAME_MAX];
  char runNumFile[FILENAME_MAX];
  int linkForXfer;
  int filesPerDir;
  int eventsPerFile;
  int hkPerFile;
  int compressionLevel;
  // Run config
  int doAtriInitialisation;
  int standAlone;
  int numEvents;
  int enableHkRead;
  float eventHkReadRateHz; 
  int sensorHkReadPeriod;
  int pedestalMode;
  int pedestalNumTriggerBlocks;
  int pedestalSamples;
  int pedestalDebugMode;
  int vdlyScan;
  int enablePcieReadout;
  // Thresholds
  int thresholdScan;
  int thresholdScanSingleChannel;
  int thresholdScanSingleChannelNum;
  int thresholdScanStartPoint;
  int thresholdScanStep;
  int thresholdScanPoints;
  int setThreshold;
  int useGlobalThreshold;
  int globalThreshold;
  uint16_t thresholdValues[THRESHOLDS_PER_ATRI];
  int surfaceBoardStack;
  uint16_t surfaceThresholdValues[ANTS_PER_TDA];
  int setSurfaceThreshold;
  // Trigger config
  int numSoftTriggerBlocks;
  int numRF0TriggerBlocks;
  int numRF0PreTriggerBlocks;

  int enableRF0Trigger;
  int enableRF1Trigger;
  int enableCalTrigger;
  int enableExtTrigger;
  int enableSoftTrigger;
  int enableRandTrigger;

  uint16_t enableL1Trigger[NUM_L1_MASKS];
  uint16_t enableL2Trigger[NUM_L2_MASKS];

  int enableVPolTrigger; //jpd -- really masking l3[0]
  int enableHPolTrigger; //jpd -- really masking l3[1]

  float softTriggerRateHz;
  float randTriggerRateHz;
  
  int enableTriggerWindow;
  int triggerWindowSize;
  int enableTriggerDelays;
  uint16_t triggerDelays[THRESHOLDS_PER_ATRI];

  // Servo
  uint16_t initialVadj[DDA_PER_ATRI];
  uint16_t initialVdly[DDA_PER_ATRI];
  uint16_t initialVbias[DDA_PER_ATRI];
  uint16_t initialIsel[DDA_PER_ATRI]; 

  int enableVdlyServo;
  int wilkinsonGoal;
  float vdlyPGain;
  float vdlyIGain;
  float vdlyDGain;
  float vdlyIMax;
  float vdlyIMin;
  int enableAntServo;
  uint16_t scalerGoalValues[THRESHOLDS_PER_ATRI];
  float scalerPGain;
  float scalerIGain;
  float scalerDGain;
  float scalerIMax;
  float scalerIMin;
  int enableRateServo; 
  float servoCalcPeriodS; 
  float servoCalcDelayS;
  float rateGoalHz;
  float ratePGain;
  float rateIGain;
  float rateDGain;
  float rateIMax;
  float rateIMin;
  int enableSurfaceServo;
  uint16_t surfaceGoalValues[ANTS_PER_TDA];
  int stackEnabled[DDA_PER_ATRI];
  int numEnabledStacks;
} ARAAcqdConfig_t;


typedef struct {
    int dState;  // Last position input
    int iState;  // Integrator state
} DacPidStruct_t;


int readConfigFile(const char* configFileName,ARAAcqdConfig_t* theConfig);
void sigUsr1Handler(int sig); 
void sigUsr2Handler(int sig);
void error(const char *msg);
int runControlSocketHandler();
void *atriControlSocketHandler(void *ptr);
void *fx2ControlUsbHandlder(void *ptr);
void *araHkThreadHandler(void *ptr);
void sendProgramReply(int newsockfd ,AraProgramControl_t requestedState);
int checkDeltaT(struct timeval *currTime, struct timeval *lastTime, float deltaT);

int initialiseAtri(int fFx2SockFd,int fAtriSockFd,ARAAcqdConfig_t *theConfig);
int initialiseAtriClocks(int fFx2SockFd,ARAAcqdConfig_t *theConfig);
int initialiseAtriDaughterboards(int fAtriSockFd,ARAAcqdConfig_t *theConfig);
int initialiseAtriDACs(int fAtriSockFd,ARAAcqdConfig_t *theConfig);
int initialiseTriggers(int fAtriSockFd);
int readSensorHk(int fFx2SockFd,int fAtriSockFd,AraSensorHk_t *sensorPtr, struct timeval *currTime);
int readEventHk(int fAtriSockFd,AraEventHk_t *eventPtr, struct timeval *currTime);

int readAtriEventPatrick(char *eventBuffer);
int readAtriEvent(char *eventBuffer);
int readAtriEventV2(unsigned char *eventBuffer);
int unpackAtriEventV2(unsigned char *eventBuffer, unsigned char *outputBuffer, int numBytesIn);
int checkAtriEventBuffer(unsigned char *eventBuffer);

int sendSoftwareTrigger(int fAtriSockFd);
int sendSoftwareTriggerWithInfo(int fAtriSockFd,  AraAtriSoftTriggerInfo_t triggerInfo);
int sendSoftwareTriggerNumBlocks(int fAtriSockFd); //FIXME -- this is a first start at setting the number of blocks
void copyTwoBytes(uint8_t *data, uint16_t *value);
void copyFourBytes(uint8_t *data, uint32_t *value);

int setIceThresholds(int fAtriSockFd, uint16_t *thresholds);
int doThresholdScan(int fAtriSockFd);
int doPedestalRun(int fAtriSockFd,int fFx2SockFd);
int doPedestalRunNotPedestalMode(int fAtriSockFd,int fFx2SockFd);
int countTriggers(AraStationEventBlockHeader_t *blkHeader) ;
int pedIndex(int dda, int block, int chan, int sample);

int updateWilkinsonVdly(int fAtriSockFd,AraEventHk_t *eventPtr);
int updateWilkinsonVdlyUsingPID(int fAtriSockFd,AraEventHk_t *eventPtr);
int updateThresholdsUsingPID(int fAtriSockFd,AraEventHk_t *eventHkPtr); 

int setSurfaceThresholds(int fAtriSockFd, uint16_t *thresholds);
int updateSurfaceThresholdsUsingPID(int fAtriSockFd,AraEventHk_t *eventHkPtr); 

int doThresholdScan(int fAtriSockFd);
int doThresholdScanSingleChannel(int fAtriSockFd);

int doVdlyScan(int fAtriSockFd);

void writeHelpfulTempFile();

//int setThresholds(const ARAacqdConfig_t* theConfig);
//int doThresholdScan(const ARAacqdConfig_t* theConfig);
//int runPedestalMode(const ARAacqdConfig_t* theConfig,int *lastEventNumber);
//int readHkEvent( AraHkBody_t* hk, const struct timeval* trigTime );
//int readFullEvent( AraEventBody_t* event, const struct timeval* trigTime, int trigType );

#endif //ARAACQD_H
