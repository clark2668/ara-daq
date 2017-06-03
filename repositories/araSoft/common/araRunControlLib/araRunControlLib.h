/*! \file araRunControlLib.h
    \brief Header file for unpacking run control commands
    
    The header file which specifies utilities for ARA station daq  run control
    
    May 2011 rjn@hep.ucl.ac.uk

*/
/*
 *
 * Current CVS Tag:
 * $Header: $
 */

#ifndef ARA_RUN_CONTROL_LIB_H
#define ARA_RUN_CONTROL_LIB_H

#include "araCom.h"

char *getRunControlTypeAsString(AraRunControlTypeCode_t type);
char *getRunControlLocAsString(AraRunControlLocationCode_t type);
const char *getRunTypeAsString(AraRunType_t runType);
int checkRunControlMessage(AraRunControlMessageStructure_t *msg);
// in utilLib
//unsigned int simpleIntCrc(unsigned int *p, unsigned int n);
void setChecksum(AraRunControlMessageStructure_t *msg);

#endif // ARA_RUN_CONTROL_LIB_H
