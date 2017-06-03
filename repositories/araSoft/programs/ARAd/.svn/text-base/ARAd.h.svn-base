/*! \file ARAd.h
  \brief The ARAd program that is repsonsible for starting and stopping runs (and other similar jobs).
    
  This is the updated ARAd program which is actually a daemon.

  May 2011 rjn@hep.ucl.ac.uk
*/

#ifndef ARAD_H
#define ARAD_H

#include "araSoft.h"
#include "araCom.h"
#include "araRunControlLib/araRunControlLib.h"
#include "utilLib/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>
#include <libgen.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <netinet/in.h>



typedef struct {
  // Verbosity
  int printToScreen;
  int logLevel;
  // Output files
  char topDataDir[FILENAME_MAX];
  char eventTopDir[FILENAME_MAX];
  char sensorHkTopDir[FILENAME_MAX];
  char eventHkTopDir[FILENAME_MAX];
  char monitorHkTopDir[FILENAME_MAX];
  char pedsTopDir[FILENAME_MAX];
  char linkDir[FILENAME_MAX];
  char runLogDir[FILENAME_MAX];
  char runNumFile[FILENAME_MAX];
  int linkForXfer;
  int filesPerDir;
  int eventsPerFile;
  int hkPerFile;
  int compressionLevel;
  int monitorPeriod;
  //Run Control
  int startRunOnStart;
  int restartRunsEvery;
  int pedestalRunEveryNRuns; ///< Will take a pedestal run every Nth run
  int startRunIfAcqdIdle; ///< Starts a run if Acqd is idle
  int startAcqdIfNotRunning; ///< Starts a run if Acqd is idle
} ARAdConfig_t;

uint32_t getNewRunNumber(char *baseDir);
int socketHandler();
int handleMessage(AraRunControlMessageStructure_t *message, AraRunControlMessageStructure_t *reply);

void checkAcqdSocket();
int openAcqdRcSocket();
int sendCommandToAcqd(AraProgramControl_t requestedState, AraProgramStatus_t desiredState, uint32_t runNumber, AraRunType_t runType);
int prepareNewRun();
int startNewRun(char *logMessage,AraRunType_t runType);
int stopRun(char *logMessage);
int terminateProgram();
void updateAcqdStatus();
void signalHandler();
int readConfigFile(ARAdConfig_t *theConfig);
int makeNewRunDirs(char *logMessage);
int startAcqd();
void updateMonitorFile(time_t currentTime);



#endif // ARAD_H
