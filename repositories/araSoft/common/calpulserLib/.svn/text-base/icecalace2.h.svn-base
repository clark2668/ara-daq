#ifndef ICECALACE2_H
#define ICECALACE2_H

#include "icecal.h"

int initializeIceCalACE2(int auxFd);
int setAttenuatorOnIceCalACE2(int auxFd, unsigned char port,
			      unsigned char attVal);
int selectModeOnIceCalACE2(int auxFd, unsigned char port,
			   unsigned char mode);
int selectAntennaOnIceCalACE2(int auxFd, unsigned char port,
			      unsigned char antenna);
int softTriggerIceCalACE2(int auxFd, unsigned char port);

// Reads sensors on ACE2. Right now this just prints out the received
// data, will parse later.
int readSensorsOnIceCalACE2(int auxFd,
			    IceCalSensors_t *sensorsA,
			    IceCalSensors_t *sensorsB);
#ifndef ICECALACE2_C
extern unsigned char uart_addr;
extern unsigned char ace2_hswap;
#endif

#endif
