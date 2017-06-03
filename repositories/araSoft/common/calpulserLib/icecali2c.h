#ifndef ICECALI2C_H
#define ICECALI2C_H

#include "icecal.h"

int initializeIceCalI2C(int auxFd);
int icecalAssign(int auxFd, unsigned int cpuId, unsigned char port);
int setAttenuatorOnIceCalI2C(int auxFd, unsigned char port,
			      unsigned char attVal);
int selectModeOnIceCalI2C(int auxFd, unsigned char port,
			   unsigned char mode);
int selectAntennaOnIceCalI2C(int auxFd, unsigned char port,
			      unsigned char antenna);
int softTriggerIceCalI2C(int auxFd, unsigned char port);

int readSensorsOnIceCalI2C(int auxFd,
			    IceCalSensors_t *sensorsA,
			    IceCalSensors_t *sensorsB);
int icecalAssignI2C(int auxFd, unsigned int cpuId, unsigned char port);
#ifndef ICECALI2C_C
extern unsigned char icbc_addr;
extern unsigned char ica_addr;
extern unsigned char icb_addr;
#endif

#endif
