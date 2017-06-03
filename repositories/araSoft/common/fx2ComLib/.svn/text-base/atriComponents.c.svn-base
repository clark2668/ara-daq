#include <stdio.h>
#include <stdlib.h>
#include "atriComponents.h"
#include "fx2ComLib/fx2Com.h"

#define LIBUSB_ERROR_TIMEOUT -9

const AtriComponentMask_t ext_i2c[4] = {EX1_mask, EX2_mask, EX3_mask, EX4_mask };

const char *ext_i2c_str[4] = {
  "EX1",
  "EX2",
  "EX3",
  "EX4"
};

int probeExpansionPort(int auxFd, AtriExpansionPort_t port) {
  int retValATRI;
  int retValMATRI;
  unsigned char dummy = 0;
  // Probe the EEPROM. ATRI
  // Probe the EEPROM. miniATRI 0xA3
  // If 0xA3 responds for port 3 and 4, return 0: miniATRIs do not have
  // EX3 and EX4.
  retValATRI = readFromI2C(auxFd, 0xA1, 1, &dummy);
  retValMATRI = readFromI2C(auxFd, 0xA3, 1, &dummy);
  
  if( retValATRI == LIBUSB_ERROR_TIMEOUT){
    if (retValMATRI == LIBUSB_ERROR_TIMEOUT){
      //Neither device present - expansion port has broken I2C bus
      fprintf(stderr, "%s: ATRI and MATRI timeout port %i\n", __FUNCTION__, port);//DEBUG
      return 0;
    }
    else if(retValMATRI>=0){
      //Found miniATRI EEPROM
      //But EX3 and EX4 don't exist, so probing them will always find device
      fprintf(stderr, "%s: found MATRI port %i\n", __FUNCTION__, port);//DEBUG
      if(port>=2) return 0;
      else return 1;
    }
    else {
      fprintf(stderr, "%s: error %d probing expansion port %s\n",
	      __FUNCTION__,
	      retValMATRI,
	      ext_i2c_str[port]);
      return retValMATRI;
    }
  }
  else if(retValATRI>=0){
      //Found ATRI EEPROM
    fprintf(stderr, "%s: found ATRI port %i\n", __FUNCTION__, port);//DEBUG
    return 1;
  }
  else {
    fprintf(stderr, "%s: error %d probing expansion port %s\n",
	    __FUNCTION__,
	    retValATRI,
	    ext_i2c_str[port]);
    return retValATRI;
  }//ATRI EEPROM
}
int enableExpansionPort(int auxFd, AtriExpansionPort_t port) {
  int retval;
  // Enable each one in turn.
  if ((retval = enableAtriComponents(auxFd, ext_i2c[port])) < 0) {
    fprintf(stderr, "%s: error %d probing expansion ports\n", __FUNCTION__,
	    retval);
    return retval;
  }
  return 0;
}

int disableExpansionPort(int auxFd, AtriExpansionPort_t port) {
  int retval;
  // Enable each one in turn.
  if ((retval = disableAtriComponents(auxFd, ext_i2c[port])) < 0) {
    fprintf(stderr, "%s: error %d probing expansion ports\n", __FUNCTION__,
	    retval);
    return retval;
  }
  return 0;
}

