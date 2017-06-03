#include <stdio.h>
#include <stdlib.h>
#define ICECALI2C_C
#include "icecal.h"
#include "icecali2c.h"
#include "ICECALICD.H"
#include "fx2ComLib/fx2Com.h"

const unsigned char icbc_addr = 0x08;
const unsigned char ica_addr = 0x0A;
const unsigned char icb_addr = 0x0C;

int icecalAssign(int auxFd,
		 unsigned int cpuId,
		 unsigned char port) {
  unsigned char data[5];
  int retval;
  
  if (!auxFd || ((port != ICECAL_PORTA) && (port != ICECAL_PORTB)))
    return -1;

  data[0] = cpuId & 0xFF;
  data[1] = (cpuId >> 8) & 0xFF;
  data[2] = (cpuId >> 16) & 0xFF;
  data[3] = (cpuId >> 24) & 0xFF;
  data[4] = (port == ICECAL_PORTA) ? 0x0A : 0x0C;
  retval = writeToI2C(auxFd, icbc_addr, 5, data);  
  if (retval < 0) {
    fprintf(stderr, "icecalAssign: error %d writing to broadcast address\n", retval);
    return retval;
  }
  printf("icecalAssign: assigned %8.8x to address %2.2x\n",
	 cpuId, data[4]);
  return 0;
}

int icecalScan(int auxFd, unsigned int prefix, 
	       unsigned int *matchA,
	       unsigned int *matchB,
	       int byte);
int icecalI2CCmd(int auxFd, unsigned char port,
		 unsigned char *argL,
		 unsigned char *argH,
		 unsigned char cmd);
int readHousekeepingFromIceCalI2C(int auxFd, unsigned char port,
				  IceCalSensors_t *sensors);

int initializeIceCalI2C(int auxFd) {
  int i, j, retval;
  unsigned int icecal_A = 0;
  unsigned int icecal_B = 0;
  unsigned int tmp = 0;
  unsigned char msg[4];

  unsigned int matchA, matchB = 0;
  int foundA = 0;
  int foundB = 0;
  int scanB = 0;
  for (j=0;j<4;j++) {
    int nmatch = 0;
    // Write 1 byte to broadcast. Start with icecal_A
    nmatch = icecalScan(auxFd, icecal_A, &matchA, &matchB, j);
    if (nmatch < 0) {
      fprintf(stderr, "error %d scanning\n", nmatch);
      return -1;
    }
    if (nmatch == 0) {
      fprintf(stderr, "initializeIceCalI2C: previously matched prefix %8.8x did not match any on byte %d\n", icecal_A, j);
      return -1;
    } else if (nmatch == 1) {
      if (!foundA) foundA = 1;
      icecal_A = icecal_A | (matchA << 8*j);	
    } else if (nmatch > 1 && foundB) {
      fprintf(stderr, "initializeIceCal: matched more than 2 IceCal addresses\n");
      fprintf(stderr, "initializeIceCal: icecal_A matched: %8.8x\n", icecal_A | (matchA << 8*j));
      fprintf(stderr, "initializeIceCal: icecal_B matched: %8.8x\n", icecal_B);
      fprintf(stderr, "initializeIceCal: new address matched %8.8x\n", icecal_A | (matchB << 8*j));
      fprintf(stderr, "initializeIceCal: don't know what to do, bailing...\n");
      return -1;
    } else {
      foundB = 1;
      icecal_B = icecal_A | (matchB << 8*j);
      icecal_A = icecal_A | (matchA << 8*j);
    }
    if (scanB) {
      int nmatch = 0;
      nmatch = icecalScan(auxFd, icecal_B, &matchA, &matchB, j);
      if (nmatch < 0) {
	fprintf(stderr, "initializeIceCal: error %d scanning\n", nmatch);
	return -1;
      }
      if (nmatch == 0) {
	fprintf(stderr, "initializeIceCalI2C: previously matched prefix %8.8x did not match any on byte %d\n", icecal_B, j);
	return -1;
      }
      else if (nmatch == 1) {
	icecal_B = icecal_B | (matchA << 8*j);
      } else {
	fprintf(stderr, "initializeIceCal: matched more than 2 IceCal addresses\n");
	fprintf(stderr, "initializeIceCal: icecal_A matched: %8.8x\n", icecal_A);
	fprintf(stderr, "initializeIceCal: icecal_B matched: %8.8x\n", icecal_B | (matchA << 8*j));
	fprintf(stderr, "initializeIceCal: new address matched %8.8x\n", icecal_B | (matchB << 8*j));
	fprintf(stderr, "initializeIceCal: don't know what to do, bailing...\n");
	return -1;
      }
    }
    if (foundB) scanB = 1;	
  }
  if (foundA) {
    unsigned char data[5];
    printf("IceCal with serial number %8.8x assigned I2C address 0x0A\n",
	   icecal_A);
    // Assign it to 0x0A.
    retval = icecalAssign(auxFd, icecal_A, ICECAL_PORTA);
    /*
    data[0] = icecal_A & 0xFF;
    data[1] = (icecal_A >> 8) & 0xFF;
    data[2] = (icecal_A >> 16) & 0xFF;
    data[3] = (icecal_A >> 24) & 0xFF;
    data[4] = 0x0A;
    retval = writeToI2C(auxFd, icbc_addr, 5, data);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d writing to broadcast address?\n",
	      __FUNCTION__, retval);
    }
    */    
  }
  if (foundB) {
    unsigned char data[5];
    printf("IceCal with serial number %8.8x assigned I2C address 0x0C\n",
	   icecal_B);
    // Assign it to 0x0C.
    retval = icecalAssign(auxFd, icecal_B, ICECAL_PORTB);
    /*
    data[0] = icecal_B & 0xFF;
    data[1] = (icecal_B >> 8) & 0xFF;
    data[2] = (icecal_B >> 16) & 0xFF;
    data[3] = (icecal_B >> 24) & 0xFF;
    data[4] = 0x0C;
    retval = writeToI2C(auxFd, icbc_addr, 5, data);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d writing to broadcast address?\n",
	      __FUNCTION__, retval);
    }
    */
  }
  return 0;
}

int icecalScan(int auxFd, unsigned int prefix, 
	       unsigned int *matchA,
	       unsigned int *matchB,
	       int byte) {
  int i,j;
  int nmatch = 0;
  unsigned char data[5];
  int retval;

  if (byte >= 4) return 0;
  for (i=0;i<256;i++) {    
    data[0] = (prefix & 0xFF);
    data[1] = (prefix & 0xFF00)>>8;
    data[2] = (prefix & 0xFF0000)>>16;
    data[3] = (prefix & 0xFF000000)>>24;
    data[byte] = i & 0xFF;
    /*
    printf("try %2.2x%2.2x%2.2x%2.2x ",
	   data[0],
	   data[1],
	   data[2],
	   data[3]);
    */
    retval = writeToI2C(auxFd, icbc_addr,
			byte+1, data);
    if (retval >= 0) 
      {
	/*
	printf(" yes\n");
	*/
	nmatch++;
	if (nmatch == 1) *matchA = i;
	else *matchB = i;
	if (nmatch == 2) break;
      }     
    /*
    else printf(" no\n");
    */
  }
  /*
   printf("scan %d matched %d: %8.8x %8.8x\n",
	  byte, nmatch, *matchA, *matchB);
  */
   return nmatch;
}

// Note: argH is unused in everything so far.
int icecalI2CCmd(int auxFd, unsigned char port,
		 unsigned char *argL,
		 unsigned char *argH,
		 unsigned char cmd) {
  unsigned char data[4];
  int retval;
  unsigned char done;
  int len;
   unsigned char i2ccmd;
   switch (cmd) 
     {
      case ICECAL_CMDID_SET_OPERATING_MODE: i2ccmd = 0; break;
      case ICECAL_CMDID_SET_ATTENUATOR: i2ccmd = 1; break;
      case ICECAL_CMDID_ANTENNA_SELECT: i2ccmd = 2; break;
      case ICECAL_CMDID_SOFTWARE_TRIGGER: i2ccmd = 3; break;
      case ICECAL_CMDID_SURFACE_VERSION: i2ccmd = 4; break;
      case ICECAL_CMDID_SURFACE_ID: i2ccmd = 5; break;
      case ICECAL_CMDID_REMOTE_VERSION: i2ccmd = 6; break;
      case ICECAL_CMDID_REMOTE_ID: i2ccmd = 7; break;
      default: fprintf(stderr, "%s: unknown cmd %d\n", __FUNCTION__, cmd);
	return -1;
     }   
   done = 0;
  if (argL && argH) { 
    data[0] = 0x09;
    data[1] = *argH;
    data[2] = *argL;
    data[3] = i2ccmd;
    len = 4;
  } else if (argL && !argH) {
    data[0] = 0x0A;
    data[1] = *argL;
    data[2] = i2ccmd;
    len = 3;
  } else {
    data[0] = 0x0B;
    data[1] = i2ccmd;
    len = 2;
  }
  retval = writeToI2C(auxFd, ica_addr + port, len, data);
  if (retval < 0) {
    fprintf(stderr, "%s : error %d writing to IceCal I2C address %2.2x\n",
	    __FUNCTION__, retval, ica_addr + port);
    return retval;
  }
  done = 0;
  while (!done) {
    data[0] = 0x0B;
    retval = writeToI2C(auxFd, ica_addr + port, 1, data);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d writing to IceCal I2C address %2.2x\n",
	      __FUNCTION__, retval, ica_addr + port);
      return retval;
    }
    retval = readFromI2C(auxFd, (ica_addr | 0x1) + port, 1, data);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d reading from IceCal I2C address %2.2x\n",
	      __FUNCTION__, retval, (ica_addr| 0x1) + port);
      return retval;
    }
     printf("got %2.2x\n", data[0]);
    if ((data[0] & 0xC0) == 0x80) {
      printf("Command completed successfully\n");
      return 0;
    } else if ((data[0] & 0xC0) == 0xC0) {
      fprintf(stderr, "%s : command returned with ERR bit set\n",
	      __FUNCTION__);
      return -1;
    }
  }
  return 0;
}

int setAttenuatorOnIceCalI2C(int auxFd, unsigned char port,
			     unsigned char attVal) {
  int retval;
  if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;
  retval = icecalI2CCmd(auxFd, port, &attVal, NULL,
			ICECAL_CMDID_SET_ATTENUATOR);
  if (retval < 0) {
    fprintf(stderr, "%s : error %d setting attenuator on port %s\n",
	    __FUNCTION__, retval, (port == ICECAL_PORTA) ? "A" : "B");
    return retval;
  }
  return 0;
}

int selectModeOnIceCalI2C(int auxFd, unsigned char port,
			     unsigned char mode) {
  int retval;
  if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;
  if (mode > ICECAL_OPMODE_NOISE_BURST_AUTO) return -1;
  retval = icecalI2CCmd(auxFd, port, &mode, NULL,
			ICECAL_CMDID_SET_OPERATING_MODE);
  if (retval < 0) {
    fprintf(stderr, "%s : error %d setting operating mode on port %s\n",
	    __FUNCTION__, retval, (port == ICECAL_PORTA) ? "A" : "B");
    return retval;
  }
  return 0;
}

int selectAntennaOnIceCalI2C(int auxFd, unsigned char port,
			     unsigned char antenna) {
  int retval;
  unsigned char arg;
  if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;
  if (antenna != ICECAL_ANTENNA_A && antenna != ICECAL_ANTENNA_B) return -1;
  arg = (antenna == ICECAL_ANTENNA_A) ? 0 : 1;
  retval = icecalI2CCmd(auxFd, port, &arg, NULL,
			ICECAL_CMDID_ANTENNA_SELECT);
  if (retval < 0) {
    fprintf(stderr, "%s : error %d selecting antenna on port %s\n",
	    __FUNCTION__, retval, (port == ICECAL_PORTA) ? "A" : "B");
    return retval;
  }
  return 0;
}

int softTriggerIceCalI2C(int auxFd, unsigned char port) {
  int retval;
  if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;
  retval = icecalI2CCmd(auxFd, port, NULL, NULL,
			ICECAL_CMDID_SOFTWARE_TRIGGER);
  if (retval < 0) {
    fprintf(stderr, "%s : error %d software triggering port %s\n",
	    __FUNCTION__, retval, (port == ICECAL_PORTA)? "A" : "B");
    return retval;
  }
  return 0;
}

int readSensorsOnIceCalI2C(int auxFd,
			   IceCalSensors_t *sensorsA,
			   IceCalSensors_t *sensorsB) {
  int retval;
  unsigned char data[2];
  unsigned char Aok;
  unsigned char Bok;
  Aok = 0;
  Bok = 0;
  // IceCal A
  if (sensorsA) {
    data[0] = 0x00;
    data[1] = 0x0F;
    retval = writeToI2C(auxFd, ica_addr, 2, data);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d writing to IceCal A\n",
	      __FUNCTION__, retval);
    } else {
      Aok = 1;
    }
  }
  if (sensorsB) {
    data[0] = 0x00;
    data[1] = 0x0F;
    retval = writeToI2C(auxFd, icb_addr, 2, data);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d writing to IceCal B\n",
	      __FUNCTION__, retval);
    } else {
      Bok = 1;
    }
  }
  if (!Aok && !Bok) return -1;
  if (Aok) {
    retval = readHousekeepingFromIceCalI2C(auxFd, ICECAL_PORTA, sensorsA);    
    if (retval < 0) return retval;
  }
  if (Bok) {
    retval = readHousekeepingFromIceCalI2C(auxFd, ICECAL_PORTB, sensorsB);
    if (retval < 0) return retval;
  }
  return 0;
}

int readHousekeepingFromIceCalI2C(int auxFd, unsigned char port,
				  IceCalSensors_t *sensors) {
  int ntries;
  int retval;
  unsigned char checkdat;
  ntries = 0;
  while (1) {
    // poll for completion
    checkdat = 0;
    retval = writeToI2C(auxFd, ica_addr+port, 1, &checkdat);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d writing to IceCal %s\n",
	      __FUNCTION__, retval, (port == ICECAL_PORTA) ? "A" : "B");
      return -1;
    }
    //Huh, multi-byte reads don't work. 
    retval = readFromI2C(auxFd, (ica_addr+port) | 0x1, 1, &checkdat);
    if (retval < 0) {
      fprintf(stderr, "%s : error %d reading from IceCal %s\n",
	      __FUNCTION__, retval, (port == ICECAL_PORTA) ? "A" : "B");
      return -1;
    }
    if (!(checkdat & 0xF)) 
       {
	  unsigned char hskdat[8];
	  int i;
	  for (i=0;i<8;i++) 
	    {
	       unsigned char tmp;
	       unsigned char tmp2;
	       int j;
	       tmp = i+1;
	       tmp2 = 0;
	       writeToI2C(auxFd, (ica_addr+port), 1, &tmp);
	       readFromI2C(auxFd, (ica_addr+port)|0x1, 1, &(tmp2));
	       hskdat[i] = tmp2;
	    }
	  sensors->surfaceVoltage = (hskdat[0] << 8) + hskdat[1];
	  sensors->surfaceTemp = (hskdat[2] << 8) + hskdat[3];
	  if (!(checkdat & 0x80)) 
	    sensors->remoteVoltage = (hskdat[4] << 8) + hskdat[5];
	  else
	    sensors->remoteVoltage = -1;
	  if (!(checkdat & 0x40))
	    sensors->remoteTemp = (hskdat[6] << 8) + hskdat[7];
	  else
	    sensors->remoteTemp = -1;
	  sensors->error = (checkdat >> 4) & 0x3;
	  return 0;
       }
    ntries++;
    if (ntries > 1000) return -1;
  }
}
