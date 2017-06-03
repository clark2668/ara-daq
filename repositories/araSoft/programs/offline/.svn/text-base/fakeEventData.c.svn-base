/*! \file fakeEventData.c
  \brief This command line program creates some fake data which can be used to test the offline software.
  
  August 2011 rjn@hep.ucl.ac.uk
*/


#include "araSoft.h"
#include "atriDefines.h"
#include "utilLib/util.h"
#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void usage(char *argv0);


ARAWriterStruct_t eventWriter;


int main(int argc, char **argv)
{
  int i,j,retVal,chan,block,dda;
  int numBytes=0;
  int run=0;
  int blocksPerEvent=2;
  unsigned char buffer[50000];
  if(argc<4) {
    usage(argv[0]);
    return -1;
  }
  run=atoi(argv[1]);
  int numEvents=atoi(argv[2]);
  blocksPerEvent=atoi(argv[3]);
  //First up just do one block per event with five channels per block

  char eventDir[FILENAME_MAX];
  sprintf(eventDir,"/tmp/fakeData/run_%06d/event",run);

  
  ///Initialize writer structures
  initWriter(&eventWriter,
	     1,
	     5, 
	     100, 
	     100,
	     EVENT_FILE_HEAD,
	     eventDir,
	     NULL);    
  
  
  if(numEvents<0) {
    printf("Can't do negative number of events\n");
    return -1;
  }
  printf("Number of events %d\n",numEvents);


  struct timeval nowTime;

  for(i=0;i<numEvents;i++) {

    gettimeofday(&nowTime,NULL);
    memset(buffer,0,50000);
    AraStationEventHeader_t *hdPtr = (AraStationEventHeader_t*) buffer;
    hdPtr->unixTime=nowTime.tv_sec;
    hdPtr->unixTimeUs=nowTime.tv_usec;
    hdPtr->eventNumber=i;
    hdPtr->ppsNumber=i/100;
    hdPtr->numBytes=0;
    hdPtr->recordId=0x52;
    hdPtr->timeStamp=hdPtr->unixTime;
    hdPtr->eventId=i;
    hdPtr->numReadoutBlocks=4*blocksPerEvent;
    int uptoByte=sizeof(AraStationEventHeader_t);
    for(block=0;block<blocksPerEvent;block++) {
      for(dda=0;dda<4;dda++) {
	AraStationEventBlockHeader_t *blkPtr = (AraStationEventBlockHeader_t*)&buffer[uptoByte];
	blkPtr->irsBlockNumber=block;
	blkPtr->channelMask=0x8f;
	blkPtr->atriDdaNumber=dda;
	uptoByte+=sizeof(AraStationEventBlockHeader_t);
	for(chan=0;chan<5;chan++) {
	  AraStationEventBlockChannel_t* chanPtr = (AraStationEventBlockChannel_t*)&buffer[uptoByte];
	  for(j=0;j<SAMPLES_PER_BLOCK;j++) 
	    chanPtr->samples[j]=1000+j;
	  
	  uptoByte+=sizeof(AraStationEventBlockChannel_t);
	}
      }
    }
    hdPtr->numBytes=uptoByte-EXTRA_SOFTWARE_HEADER_BYTES;
    fillGenericHeader(buffer,ARA_EVENT_TYPE,uptoByte);
    int new_file_flag = 0;
    retVal = writeBuffer( &eventWriter, (char*)buffer, uptoByte , &(new_file_flag) );
  }

  closeWriter(&eventWriter);


  return 0;  

}


void usage(char *argv0)
{

  printf("Usage:\n");
  printf("\t %s  <run> <num events> <blocks per event>...\n",basename(argv0));
 
}
