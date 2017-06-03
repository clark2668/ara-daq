//
// Definitions for functions which handle the run information / logging features [libARArunlog.a]
//
// ----------------------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "araRunLogLib.h"
#include "utilLib/util.h"

float computeEventRate( int numEvents , struct timeval *startTime , struct timeval *endTime ) {

  float rate = 0.0;

//syslog(LOG_INFO,"numEvents = %d",numEvents);
  if( numEvents > 0 ){
    float timespan = endTime->tv_sec - startTime->tv_sec + 1e-6 * (float)endTime->tv_usec - 1e-6 * (float)startTime->tv_usec;
    rate = numEvents / timespan;
//  syslog(LOG_INFO,"timespan = %f -- numEvents = %d",timespan,numEvents);
  } else {
    rate = 0.0;
  }
//syslog(LOG_INFO,"Event rate %f Hz.",rate);

  return rate;

} // end of function computeEventRate()

void latchFirmwareVersion( ARAacqdRunInfo_t *RunInfo , char *firmwareVersion ) {

  //FIXME malloc
  RunInfo->firmwareVersion = malloc( sizeof(firmwareVersion)*sizeof(char) );
  sprintf(RunInfo->firmwareVersion, "%s", firmwareVersion);
  return;

} // end of function latchFirmwareVersion()

void latchRunNumber( ARAacqdRunInfo_t *RunInfo , int runNumber ) {

  RunInfo->runNumber = runNumber;
  return;

} // end of function latchRunNumber()

void latchRunStartTime( ARAacqdRunInfo_t *RunInfo ) {

  struct timeval startTime;
  gettimeofday( &(startTime) , NULL );

  RunInfo->runStartTime.tv_sec  = startTime.tv_sec;
  RunInfo->runStartTime.tv_usec = startTime.tv_usec;

  return;

} // end of function latchStartTime()

void latchStartTime( ARAacqdRunInfo_t *RunInfo , time_t seconds , suseconds_t usec ) {

#if 0
  struct timeval startTime;
  gettimeofday( &(startTime) , NULL );

  RunInfo->startTime.tv_sec  = startTime.tv_sec;
  RunInfo->startTime.tv_usec = startTime.tv_usec;
#endif

  RunInfo->startTime.tv_sec  = (time_t)      seconds;
  RunInfo->startTime.tv_usec = (suseconds_t) usec;

  return;

} // end of function latchStartTime()

void latchEndTime( ARAacqdRunInfo_t *RunInfo ) {

  struct timeval endTime;
  gettimeofday( &(endTime) , NULL );

  RunInfo->endTime.tv_sec  = endTime.tv_sec;
  RunInfo->endTime.tv_usec = endTime.tv_usec;

  return;

} // end of function latchEndTime()

void latchFirstEventNum( ARAacqdRunInfo_t *RunInfo , int eventNumber ) {

  RunInfo->firstEventNum = eventNumber;

  return;

} // end of function latchFirstEventNum()

void latchLastEventNum( ARAacqdRunInfo_t *RunInfo , int eventNumber ) {

  RunInfo->lastEventNum = eventNumber;

  return;

} // end of function latchLastEventNum()

void latchNumEvents( ARAacqdRunInfo_t *RunInfo ) {

  RunInfo->numEvents = (RunInfo->lastEventNum - RunInfo->firstEventNum) + 1;

  return;

} // end of function latchNumEvents()

void latchDAQConfiguration( ARAacqdRunInfo_t *RunInfo , int is_internal_trig_on , int is_soft_trig_on , int is_random_trig_on , int is_pedestal_run , float soft_trig_rate , float random_trig_rate , int threshold_value ) {

  if ( is_pedestal_run ) {
     RunInfo->ped_status       = "  PEDS  ";
     RunInfo->rf_status        = "        ";
     RunInfo->soft_trig_status = "        ";
     RunInfo->rand_trig_status = "        ";
  } else {
     RunInfo->ped_status = "        ";
     if ( is_internal_trig_on ) {
        RunInfo->rf_status = malloc( 9*sizeof(char) );
        sprintf( RunInfo->rf_status , " RF%4.4d " , threshold_value );
     } else {
	RunInfo->rf_status = "        ";
     }
     if ( is_soft_trig_on ) {
        RunInfo->soft_trig_status = malloc( 9*sizeof(char) );
        sprintf( RunInfo->soft_trig_status , "SOFT%-4.1f" , soft_trig_rate );
     } else {
	RunInfo->soft_trig_status = "        ";
     }
     if ( is_random_trig_on ) {
        RunInfo->rand_trig_status = malloc( 9*sizeof(char) );
        sprintf( RunInfo->rand_trig_status , "RAND%-4.1f" , random_trig_rate );
     } else {
	RunInfo->rand_trig_status = "        ";
     }
  }
   
  return;
   
} // end of function latchDAQConfiguration()
   
void recordStartDAQ( ARAacqdRunInfo_t *RunInfo , int run_number , char *run_comment , char *log_dir, char *runInfo_dirName) {

  RunInfo->WasFirstEventFileOpenned = 0; /* =0, No */
  //  fprintf(stderr, "copying latchRunNumber\n");
  latchRunNumber( RunInfo , run_number );
  //  fprintf(stderr, "copying latchRunStartTime\n");
  latchRunStartTime( RunInfo );
  /* fprintf(stderr, "copying logDirName %s mallocing %lu\n", log_dir, sizeof(log_dir)*sizeof(char)); */
  /* RunInfo->logDirName = malloc(sizeof(log_dir)*sizeof(char)); */
  //  fprintf(stderr, "copying logDirName %s mallocing %lu\n", log_dir, FILENAME_MAX*sizeof(char));
  RunInfo->logDirName = malloc(FILENAME_MAX*sizeof(char));

  sprintf(RunInfo->logDirName, "%s", log_dir);
  //  fprintf(stderr, "logDirName %s\n",   RunInfo->logDirName);

  //  fprintf(stderr, "copying runInfoDirName %s mallocing %lu\n", log_dir, FILENAME_MAX*sizeof(char));
  RunInfo->runInfoDirName = malloc(FILENAME_MAX*sizeof(char));

  sprintf(RunInfo->runInfoDirName, "%s", runInfo_dirName);
  //  fprintf(stderr, "runInfoDirName %s\n",   RunInfo->runInfoDirName);

  //  fprintf(stderr, "readRunComment\n");
  readRunComment( RunInfo );
  //  fprintf(stderr, "readRunComment\n");
  RunInfo->numEvents    = 0;
  RunInfo->totNumEvents = 0;
  RunInfo->eventRate    = 0.0;
  RunInfo->totEventRate = 0.0;
  RunInfo->firstEventNum= 0;
  RunInfo->lastEventNum = 0;
  
  return;

} // end of function recordStartDAQ()

void recordOpenEventFile( ARAacqdRunInfo_t *RunInfo , int lastEventNum , time_t seconds , suseconds_t usec ) {

  latchStartTime( RunInfo , seconds , usec );
  latchFirstEventNum( RunInfo , (lastEventNum) );

  return;

} // end of function recordOpenEventFile()

void recordCloseEventFile( ARAacqdRunInfo_t *RunInfo , int lastEventNum ) {

  //  fprintf(stderr, "lastEventNum %d firstEventNum %d\n", lastEventNum-1, RunInfo->firstEventNum);
  latchLastEventNum ( RunInfo , lastEventNum-1 );
  latchNumEvents( RunInfo );
  latchEndTime( RunInfo );

  RunInfo->eventRate = computeEventRate( RunInfo->numEvents , &(RunInfo->startTime) , &(RunInfo->endTime) );

  RunInfo->totNumEvents += RunInfo->numEvents;
  RunInfo->totEventRate  = computeEventRate( RunInfo->totNumEvents , &(RunInfo->runStartTime) , &(RunInfo->endTime) );

//reportRunInfo( RunInfo );

  return;

} // end of function handleCloseEventFile()

void reportRunInfo( ARAacqdRunInfo_t *RunInfo ) {

  char time_as_string[40];
  syslog(LOG_INFO," Run Number = %d",RunInfo->runNumber);
  syslog(LOG_INFO,"   Run Type = %s %s %s %s",
	 RunInfo->ped_status,RunInfo->rf_status,RunInfo->soft_trig_status,RunInfo->rand_trig_status);
  if ( RunInfo->runComment == NULL ) {
     syslog(LOG_INFO,"Run Comment = No Comment Availabe");
  } else {
     syslog(LOG_INFO,"Run Comment = %s",RunInfo->runComment);
  }
  strftime(time_as_string,40,"%Y/%m/%d@%H:%M:%S",localtime( &(RunInfo->startTime.tv_sec)) );
  syslog(LOG_INFO," Start Time = %d.%6.6d [%s]",(int) RunInfo->startTime.tv_sec,(int) RunInfo->startTime.tv_usec,time_as_string);
  syslog(LOG_INFO,"   End Time = %d.%6.6d",(int) RunInfo->endTime.tv_sec,(int) RunInfo->endTime.tv_usec);
  syslog(LOG_INFO,"First Event = %d",RunInfo->firstEventNum);
  syslog(LOG_INFO," Last Event = %d",RunInfo->lastEventNum);
  syslog(LOG_INFO,"     Events = %d [TOTAL=%d]",RunInfo->numEvents,RunInfo->totNumEvents);

  return;

} // end of function reportRunInfo()

void updateRunLogFile( ARAacqdRunInfo_t *RunInfo ) {

  static int first_entry_into_routine = 0;

  if ( first_entry_into_routine == 0 ) {  /* don't write a record for the first call */
     first_entry_into_routine = 1;        /* as we have not yet collected any events */
     return;
  }

  char     starttime_as_string[40];
  strftime(starttime_as_string,40,"%Y/%m/%d@%H:%M:%S",localtime( &(RunInfo->startTime.tv_sec)) );
  char     endtime_as_string[40];
  strftime(endtime_as_string,40,"%Y/%m/%d@%H:%M:%S",localtime( &(RunInfo->endTime.tv_sec)) );

  //  FILE *runlog_fp = fopen("/data/runlog","a");
  char runlogName[FILENAME_MAX];
  sprintf(runlogName, "%s/runlog", RunInfo->runInfoDirName);
  FILE *runlog_fp = fopen(runlogName,"a");

  if ( runlog_fp == NULL ) {
     syslog(LOG_ERR,"unable to open /data/runlog for append");
     exit(1);
  }

  fprintf(runlog_fp,"run%6.6d %10d.%6.6d %20s %20s %9s %9s %9s %9s %s %6d %10d %7.3f %s\n",
          RunInfo->runNumber,
          (unsigned int) RunInfo->startTime.tv_sec , (unsigned int) RunInfo->startTime.tv_usec ,
          starttime_as_string,
          endtime_as_string,
          RunInfo->ped_status,RunInfo->rf_status,RunInfo->soft_trig_status,RunInfo->rand_trig_status,
	  RunInfo->firmwareVersion,
          RunInfo->numEvents,
          RunInfo->totNumEvents,
          RunInfo->eventRate,
	  RunInfo->runComment);

  fclose(runlog_fp);

  return;

} // end of function updateRunLogFile()

void readRunComment( ARAacqdRunInfo_t *RunInfo )
{
   
  int read_run_comment = 0; /* flag: =0, unable to read comment, =1 read run comment */
  
  char fileName[100];
  sprintf(fileName, "%s/runStart.run%6.6d.dat", RunInfo->logDirName, RunInfo->runNumber);
  FILE *in_fp = fopen(fileName,"r");

  if( in_fp == NULL )
  {
     syslog(LOG_ERR,"unable to open RunCommentFile, name = %s",fileName);
     exit(1);
  }
  
  char         *line = NULL;
  size_t         len = 0;
  ssize_t bytes_read = getline(&line, &len, in_fp); /* getline() allocates sufficient memory for line, so free is required   */
  bytes_read = getline(&line, &len, in_fp);
  bytes_read = getline(&line, &len, in_fp); /* third read returns the line containing run comments */

  if ( read > 0 ) 
  {
     RunInfo->runComment = (char *) malloc( sizeof(char)*(bytes_read+1) );
     strncpy( RunInfo->runComment , line , bytes_read );
     RunInfo->runComment[bytes_read] = '\0';      /* write 0 over last character to remove '\n' from the end of the string */
     read_run_comment = 1;
  }
  else 
  {
     RunInfo->runComment = (char *) malloc( sizeof(char)*22 );
     strncpy( RunInfo->runComment , "no comment available" , 21 );
     RunInfo->runComment[22] = '\0';
     read_run_comment = 0;
  }
  
  //  fprintf(stderr, "Read run comment %s\n", RunInfo->runComment);

  if (line) free(line);
  
  fclose(in_fp);

  //  fprintf(stderr, "finished reading run comment\n");

  return;
   
} // end of function readRunComment()

