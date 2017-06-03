/*! \file araRunControlLib.c
    \brief Header file for unpacking run control commands
    
    The header file which specifies utilities for ARA station daq  run control
    
    May 2011 rjn@hep.ucl.ac.uk

*/
/*
 *
 * Current CVS Tag:
 * $Header: $
 */

#include "araRunControlLib.h"


const char *getRunTypeAsString(AraRunType_t runType) {
  switch(runType) {
  case ARA_RUN_TYPE_NA: return "ARA_RUN_TYPE_NA"; ///< Not applicable
  case ARA_RUN_TYPE_NORMAL: return "ARA_RUN_TYPE_NORMAL"; ///< Standard data taken
  case ARA_RUN_TYPE_PEDESTAL: return "ARA_RUN_TYPE_PEDESTAL"; ///< Pedestal run
  case ARA_RUN_TYPE_THRESHOLD: return "ARA_RUN_TYPE_THRESHOLD"; ///< Threshold scan
  default: break;
  }
  return "Unknown Run Type";
}

char *getRunControlTypeAsString(AraRunControlTypeCode_t type)
{
  switch(type) {
  case ARA_RC_RESPONSE: return "ARA_RC_RESPONSE";
  case ARA_RC_QUERY_STATUS: return "ARA_RC_QUERY_STATUS";
  case ARA_RC_START_RUN: return "ARA_RC_START_RUN";
  case ARA_RC_STOP_RUN: return "ARA_RC_STOP_RUN";
  default:
    break;
  }
  return "Unknown Command";
}




char *getRunControlLocAsString(AraRunControlLocationCode_t loc)
{

  switch(loc) {
  case ARA_LOC_RC: return "ARA_LOC_RC";
  case ARA_LOC_ARAD_ARA1: return "ARA_LOC_ARAD_ARA1";
  default:
    break;
  }
  return "Unknown Location";

}

int checkRunControlMessage(AraRunControlMessageStructure_t *msg)
{
  //Will implement something that checks specifically for each command, 
  //but for now just do the checksum check
  unsigned int checkSum=simpleIntCrc((unsigned int*)&(msg->from),sizeof(AraRunControlMessageStructure_t)-sizeof(unsigned int));
  if(checkSum=msg->checkSum)
    return 1;
  return 0;
}


/*
unsigned int simpleIntCrc(unsigned int *p, unsigned int n)
{
  unsigned int sum = 0;
  unsigned int i;
  for (i=0L; i<n; i++) {
    //
    sum += *p++;
  }
  //    printf("%u %u\n",*p,sum);
  return ((0xffffffff - sum) + 1);

}
*/

void setChecksum(AraRunControlMessageStructure_t *msg)
{
  msg->checkSum=simpleIntCrc((unsigned int*)&(msg->from),sizeof(AraRunControlMessageStructure_t)-sizeof(unsigned int));
}
