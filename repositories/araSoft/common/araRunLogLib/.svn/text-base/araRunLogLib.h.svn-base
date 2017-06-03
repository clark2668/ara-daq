//
// Declarations for functions which handle the run information / logging features [libARArunlog.a]
//
// -----------------------------------------------------------------------------------------------

#ifndef ARARUNINFO_H
#define ARARUNINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <syslog.h>
#include <sys/time.h>

typedef struct {

  int             runNumber;
  int             WasFirstEventFileOpenned; // 'first event file open' flag: =0 no / =1 yes
  struct timeval  runStartTime;
  int             firstEventNum;
  int             lastEventNum;
  int             numEvents;                // events in the last event file
  int             totNumEvents;             // events in the whole run
  float           eventRate;                // rate for last event file
  float           totEventRate;             // rate for whole run
  struct timeval  startTime;
  struct timeval  endTime;
  char           *firmwareVersion;          // firmware version
  char           *runComment;
  char           *ped_status;
  char	         *rf_status;
  char           *soft_trig_status;
  char           *rand_trig_status;
  char           *logDirName;
  char           *runInfoDirName;
} ARAacqdRunInfo_t;

float computeEventRate( int numEvents , struct timeval *startTime , struct timeval *endTime );
void  latchFirmwareVersion( ARAacqdRunInfo_t *RunInfo , char *firmwareVersion );
void  latchRunNumber( ARAacqdRunInfo_t *RunInfo , int runNumber );
void  latchRunStartTime( ARAacqdRunInfo_t *RunInfo );
void  latchStartTime( ARAacqdRunInfo_t *RunInfo , time_t seconds , suseconds_t usec );
void  latchEndTime( ARAacqdRunInfo_t *RunInfo );
void  latchFirstEventNum( ARAacqdRunInfo_t *RunInfo , int eventNumber );
void  latchLastEventNum( ARAacqdRunInfo_t *RunInfo , int eventNumber );
void  latchNumEvents( ARAacqdRunInfo_t *RunInfo );
//void  latchRunComment( ARAacqdRunInfo_t *RunInfo , char *runComment );
void  readRunComment( ARAacqdRunInfo_t *RunInfo );

  void  recordStartDAQ( ARAacqdRunInfo_t *RunInfo , int run_number , char *run_comment , char *log_dir, char* runInfo_dirName);
void  latchDAQConfiguration( ARAacqdRunInfo_t *RunInfo , int is_internal_trig_on , int is_soft_trig_on , int is_random_trig_on , int is_pedestal_run , float soft_trig_rate , float random_trig_rate , int threshold_value );
void  recordOpenEventFile( ARAacqdRunInfo_t *RunInfo , int lastEventNum , time_t seconds , suseconds_t usec );
void  recordCloseEventFile( ARAacqdRunInfo_t *RunInfo , int lastEventNum );

void  reportRunInfo( ARAacqdRunInfo_t *RunInfo );
  void  updateRunLogFile(ARAacqdRunInfo_t *RunInfo );

#ifdef __cplusplus
}
#endif
#endif // ARARUNINFO_H

