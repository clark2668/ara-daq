#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef USB_I2C_DIOLAN
#include "i2cComLib/i2cCom.h"
#else
#include "fx2ComLib/fx2Com.h"
#endif

#include "ace2.h"
#include "icecal.h"
#include "icecali2c.h"
#include "icecalace2.h"

eIcConnType connType = icConnNone;

eIcConnType icecalGetConnType() {
  return connType;
}

void icecalSetConnType(eIcConnType newType) {
  connType = newType;
}

// not true - this is EPIPE - but it's what it returns now
#define LIBUSB_ERROR_TIMEOUT -9

int initializeIceCal(int auxFd) {
  if (connType == icConnNone)
    return -1;
  if (connType == icConnACE2)
    return initializeIceCalACE2(auxFd);
  if (connType == icConnIceCal)
    return initializeIceCalI2C(auxFd);
}

int setAttenuatorOnIceCal(int auxFd, unsigned char port,
			  unsigned char attVal) {
  if (connType == icConnNone) 
    return -1;
  if (connType == icConnACE2) 
    return setAttenuatorOnIceCalACE2(auxFd, port, attVal);
  if (connType == icConnIceCal)
    return setAttenuatorOnIceCalI2C(auxFd, port, attVal);

  // Determine which one to call...
  return 0;
}

int selectModeOnIceCal(int auxFd, unsigned char port,
		       unsigned char mode) {
  if (connType == icConnNone)
    return -1;
  if (connType == icConnACE2)
    return selectModeOnIceCalACE2(auxFd, port, mode);
  if (connType == icConnIceCal)
    return selectModeOnIceCalI2C(auxFd, port, mode);
  // Determine which one to call...
  return 0;
}
  
int selectAntennaOnIceCal(int auxFd, unsigned char port,
			  unsigned char antenna) {
  if (connType == icConnNone)
    return -1;
  if (connType == icConnACE2)
    return selectAntennaOnIceCalACE2(auxFd, port, antenna);
  if (connType == icConnIceCal)
    return selectAntennaOnIceCalI2C(auxFd, port, antenna);
  // Determine which one to call...
  return 0;
}

int softTriggerIceCal(int auxFd, unsigned char port) {
  if (connType == icConnNone)
    return -1;
  if (connType == icConnACE2)
    return softTriggerIceCalACE2(auxFd, port);
  if (connType == icConnIceCal)
    return softTriggerIceCalI2C(auxFd, port);
  // Determine which one to call...
  return 0;
}

int readSensorsOnIceCal(int auxFd,
			IceCalSensors_t *sensorsA,
			IceCalSensors_t *sensorsB) {
  if (connType == icConnNone)
    return -1;
  if (connType == icConnACE2)
    return readSensorsOnIceCalACE2(auxFd,sensorsA,sensorsB);
  if (connType == icConnIceCal)
    return readSensorsOnIceCalI2C(auxFd,sensorsA,sensorsB);
      
  // Determine which one to call
  return 0;
}


// Probe all of the expansion ports for an IceCal. We look for either
// an ACE2 (at address 0x9A) or an IceCal broadcast acknowledge
// (at address 0x08).
int probeForIceCal(int auxFd) {
  int found;
  unsigned char dummy;
  int i;
  int retval;
   
  found = 0;
  connType = icConnNone;
  // Probe all of the expansion ports.
  for (i=0;i<4;i=i+1) {
    printf("%s:", ext_i2c_str[i]);
    // Enable each one in turn.
    if ((retval = enableExpansionPort(auxFd, i)) < 0) {
      fprintf(stderr, "icecald: error %d enabling expansion ports\n", retval);
      return retval;
    }
    retval = probeExpansionPort(auxFd, i);
    if (retval > 0) 
      {
	printf(" device present ");
	printf(" auxFd = %d ", auxFd);
	// Probe for an assigned IceCal at 0x0A.
	dummy = 0x0;
	if ((retval=writeToI2C(auxFd, ica_addr, 0, &dummy)) == LIBUSB_ERROR_TIMEOUT) {
	  printf(" - no IceCal_A ");
	} else {
	  printf(" - IceCal_A located ");
	  connType = icConnIceCal;
	  found = 1;
	  if ((retval=writeToI2C(auxFd, icb_addr, 0, &dummy)) == LIBUSB_ERROR_TIMEOUT) {
	    printf(" - no IceCal_B\n");
	  } else {
	    printf(" - IceCal_B located\n");
	  }
	}
	
	if (!found) {
	  
	  dummy = 0x0;
	  if ((retval=writeToI2C(auxFd, icb_addr, 0, &dummy)) 
	      == LIBUSB_ERROR_TIMEOUT) {
	    
	    // Probe for the IceCal broadcast address
	    dummy = 0x0;
	    if ((retval=writeToI2C(auxFd, icbc_addr, 0, &dummy)) 
		== LIBUSB_ERROR_TIMEOUT) {
	      printf(" - not an IceCal ");
	      dummy = 0x18;
	      if ((retval = writeToI2C(auxFd, uart_addr, 1, &dummy)) == LIBUSB_ERROR_TIMEOUT) {
		printf(" - not an ACE2\n");
		continue;
	      } else if (retval >= 0) {
		printf(" - located an ACE2\n");
		found = 1;
		connType = icConnACE2;
	      } else {
		fprintf(stderr, "got return value %d probing UART?\n", retval);
		exit(1);
	      }
	    } else if (retval >= 0) {
	      printf(" - located an IceCal\n");
	      found = 1;
	      connType = icConnIceCal;
	    } else {
	      fprintf(stderr, "got return value %d probing IceCal?\n", retval);
	      exit(1);
	    }	  
	  }
	}
      } else if (retval == 0) {
      printf(" no device present\n");
    } else {
      fprintf(stderr, "%s : error %d probing EX%d\n",
	      __FUNCTION__, retval, i+1);
    }
    if (found) return i;
    
    retval = disableExpansionPort(auxFd, i);
    if (retval < 0) {
      fprintf(stderr, "%s: error %d disabling expansion port %s\n",
	      __FUNCTION__,
	      retval,
	      ext_i2c_str[i]);
      return retval;
    }
  }
  return -1;
}

