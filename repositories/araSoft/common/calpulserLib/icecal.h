#ifndef ICECAL_H
#define ICECAL_H

#include "ace2.h"

#define ICECAL_PORTA ACE2_PORTA
#define ICECAL_PORTB ACE2_PORTB
// these are the two sources : "0" is pulse generator, "1" is noise source
#define ICECAL_OPMODE_OFF 0
#define ICECAL_OPMODE_PULSE_NORMAL 1
#define ICECAL_OPMODE_PULSE_AUTO 2
#define ICECAL_OPMODE_NOISE_ON 3
#define ICECAL_OPMODE_NOISE_BURST_NORMAL 4
#define ICECAL_OPMODE_NOISE_BURST_AUTO 5

// on/off for high voltage
#define ICECAL_ANTENNA_A '0'
#define ICECAL_ANTENNA_B '1'

#define ICECAL_ERROR_NONE 0
#define ICECAL_ERROR_PROTOCOL -1
#define ICECAL_ERROR_NO_RESPONSE -2

typedef struct icecalSensors 
{
   int surfaceVoltage;
   int surfaceTemp;
   int remoteVoltage;
   int remoteTemp;
   int error;
} IceCalSensors_t; 

typedef enum icConnType {
  icConnNone = 0,
  icConnACE2 = 1,
  icConnIceCal = 2
} eIcConnType;


eIcConnType icecalGetConnType();
void icecalSetConnType(eIcConnType new_type);

int readSensorsOnIceCal(int auxFd,
			    IceCalSensors_t *sensorsA,
			    IceCalSensors_t *sensorsB);
int initializeIceCal(int auxFd);
int probeForIceCal(int auxFd);
int setAttenuatorOnIceCal(int auxFd, unsigned char port,
			      unsigned char attVal);
int selectModeOnIceCal(int auxFd, unsigned char port,
			     unsigned char source);
int selectAntennaOnIceCal(int auxFd, unsigned char port,
			      unsigned char antenna);
int softTriggerIceCal(int auxFd, unsigned char port);
int icecalAssign(int auxFd, unsigned int cpuId, unsigned char port);

#endif
