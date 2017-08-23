/*
 * Simple IceCal-through-ACE2 command interface.
 *
 * Example, to read housekeeping:
 * # Probe for ACE-2 using IceCal address, initialize, then read
 * icecald probe init hsk
 *
 * Subsequent calls to icecald don't need to use probe or init, however,
 * if they don't, they need to use "EX" as in
 * > icecald probe
 * 1: no device present
 * 2: no device present
 * 3: no device present
 * 4: device present - located an ACE2
 * > icecald EX4 init
 * > icecald EX4 allhsk
 * icecald: 29 bytes pending on UART A
 * ....
 * 
 * Available commands:
 * probe - locate the ACE-2 (and enable it for any subsequent commands)
 * ACEEX# - specifies the ACE-2 port (and enable) - e.g. ACEEX1 (must be 1-4)
 * ICEEX# - specifies the IceCal port (and enable) - e.g. ICEEX1 (must be 1-4)
 * init - initialize the UART (only needs to be done once for each power on)
 * icehsk - reads housekeeping from IceCal boards
 * acehsk - reads housekeeping from ACE-2 boards
 * allhsk - reads all housekeeping
 * pwron - turns the UART/RS-232 transceivers on (if it was off)
 * pwroff - turns the UART/RS-232 transceivers off
 * attA# - sets attenuator level on port A (# must be between 0-31)
 * (attB#) - same as attA for port B
 * opA# - sets operating mode (must be 0-5, see ICECALICD.H or icecal.h for what they do) for port A
 * opB# - sets operating mode for port B
 * trigA - software trigger on port A
 * trigB - software trigger on port B
 * antA# - select antenna on port A (must be 0 or 1, corresponding to antenna 'A' or 'B' respectively)
 * antB# - select antenna on port B (must be 0 or 1, corresponding to antenna 'A' or 'B' respectively)
 * iceA######## - assign IceCal with CPU ID to port A (e.g. iceA003E570F)
 * iceB######## - assign IceCal with CPU ID to port B (e.g. iceB003E5710)
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef USB_I2C_DIOLAN
#include "fx2ComLib/fx2Com.h"
#else
#include "i2cComLib/i2cCom.h"
#endif


#include "ace2.h"
#include "icecal.h"
#include "icecald.h"
#include "icecalace2.h"
#include "icecali2c.h"


#define PORT_CHECK(x) if ((x) < 0) { \
	fprintf(stderr, "icecald: port or probe must be specified before %s", *argv); \
	close(auxFd); \
	exit(1); \
      }

int icecald_readSensorsOnIceCal(int auxFd) 
{
   IceCalSensors_t sensorsA;
   IceCalSensors_t sensorsB;
   int retval;
   
   if ((retval = readSensorsOnIceCal(auxFd, &sensorsA, &sensorsB)) < 0) {
     printf("err %d reading sensors?\n", retval);
     return retval;
   }
   if (sensorsA.error == ICECAL_ERROR_NONE) {
      printf("IceCal on port A: surface voltage: ");
      if (sensorsA.surfaceVoltage >= 0) printf("%2.3f V\n", sensorsA.surfaceVoltage/10.);
      else printf(" not available\n");
      printf("IceCal on port A: surface temp: ");
      if (sensorsA.surfaceTemp >= 0) printf("%d K\n", sensorsA.surfaceTemp);
      else printf(" not available\n");
      printf("IceCal on port A: remote voltage: ");
      if (sensorsA.remoteVoltage >= 0) printf("%f V\n", sensorsA.remoteVoltage/10.);
      else printf(" not available\n");
      printf("IceCal on port A: remote temp: ");
      if (sensorsA.remoteTemp >= 0) printf("%d K\n", sensorsA.remoteTemp);
      else printf(" not available\n");
   } else {
     printf("got an error %d from sensorsA?\n", sensorsA.error);
   }
   if (sensorsB.error == ICECAL_ERROR_NONE) {
      printf("IceCal on port B: surface voltage: ");
      if (sensorsB.surfaceVoltage >= 0) printf("%2.3f V\n", sensorsB.surfaceVoltage/10.);
      else printf(" not available\n");
      printf("IceCal on port B: surface temp: ");
      if (sensorsB.surfaceTemp >= 0) printf("%d K\n", sensorsB.surfaceTemp);
      else printf(" not available\n");
      printf("IceCal on port B: remote voltage: ");
      if (sensorsB.remoteVoltage >= 0) printf("%f V\n", sensorsB.remoteVoltage/10.);
      else printf(" not available\n");
      printf("IceCal on port B: remote temp: ");
      if (sensorsB.remoteTemp >= 0) printf("%d K\n", sensorsB.remoteTemp);
      else printf(" not available\n");
   } else {
     printf("got an error %d from sensorsB?\n", sensorsB.error);
   }
   return 0;
}

   
   
   

int icecald_readSensorsOnACE2(int auxFd) 
{
   int retval;
   unsigned short vc, cc;
   float voltage, current;
   if ((retval = readSensorsOnACE2(auxFd, ace2_hswap, &vc, &cc)) < 0)
     return retval;
   // voltage, in volts
   voltage = (vc*1.6235)/(1000.);
   // current, in milliamps
   current = (cc*25.84)/(1000.);
   printf("ACE2 Voltage: %2.4f V Current: %3.4f mA\n",
	  voltage, current);
   return 0;
}

int icecald_powerOnACE2(int auxFd) 
{
    return 0;
}


int icecald_powerOffACE2(int auxFd)
{
   return 0;
}


int main(int argc, char **argv) {
  int ace2_port = -1;
  int i;
  int auxFd; // FD to the FX2 (aux control)
  int retval;
  char *dummy;
  unsigned char tx_byte;
  unsigned char rx_buf[64];

  argc--; argv++;
  if (!argc) { exit(1); }
  // do some connecty-thingy
  auxFd = openConnectionToFx2ControlSocket();
  if (auxFd == 0) {
    fprintf(stderr, "Error opening connection to FX2_CONTROL_SOCKET\n");
    exit(1);
  }

  while (argc) {
    if (strstr(*argv, "probe")) {
      ace2_port = probeForIceCal(auxFd);
      if (ace2_port < 0) 
	 {
	    close(auxFd);
	    exit(1);
	 }
    }
    if ((dummy = strstr(*argv, "ACEEX"))) {
      ace2_port = atoi(dummy+strlen("ACEEX")) - 1;
      if ((retval = enableAtriComponents(auxFd, ext_i2c[ace2_port])) < 0)
	{
	  fprintf(stderr, "icecald: error %d enabling port %s\n", retval, ext_i2c_str[ace2_port]);
	  close(auxFd);
	  exit(1);
	}
      icecalSetConnType(icConnACE2);
    }
    if ((dummy = strstr(*argv, "ICEEX"))) {
      ace2_port = atoi(dummy+strlen("ICEEX")) - 1;
      if ((retval = enableAtriComponents(auxFd, ext_i2c[ace2_port])) < 0)
	{
	  fprintf(stderr, "icecald: error %d enabling port %s\n", retval, ext_i2c_str[ace2_port]);
	  close(auxFd);
	  exit(1);
	}
      icecalSetConnType(icConnIceCal);
    }
    if (strstr(*argv, "iceA")) {
      unsigned int id;
      PORT_CHECK(ace2_port);
      if (icecalGetConnType() != icConnIceCal) {
	printf("icecald: ignoring iceA command, no IceCal connection present\n");
      }
      argv++;
      argc--;
      id = strtoul(*argv, NULL, 16);
      retval = icecalAssign(auxFd, id, ICECAL_PORTA);
      if (retval < 0) {
	fprintf(stderr, "icecald: error %d assigning ID %8.8x to port A.\n", retval, id);
	return retval;
      }
    }

    if (strstr(*argv, "iceB")) {
      unsigned int id;
      PORT_CHECK(ace2_port);
      if (icecalGetConnType() != icConnIceCal) {
	printf("icecald: ignoring iceB command, no IceCal connection present\n");
      }
      argv++;
      argc--;
      id = strtoul(*argv, NULL, 16);
      retval = icecalAssign(auxFd, id, ICECAL_PORTB);
      if (retval < 0) {
	fprintf(stderr, "icecald: error %d assigning ID %8.8x to port B.\n", retval, id);
	return retval;
      }
    }


    if (strstr(*argv, "init")) {
      PORT_CHECK(ace2_port);
      if (initializeIceCal(auxFd)) {
	close(auxFd);
	exit(1);
      }
    }
    if (strstr(*argv, "icehsk") || strstr(*argv, "allhsk")) {       
      PORT_CHECK(ace2_port);
      printf("Reading sensors on IceCal\n");
      if (icecald_readSensorsOnIceCal(auxFd)) {
	close(auxFd);
	exit(1);
      }
    }
    if (strstr(*argv, "acehsk") || strstr(*argv, "allhsk")) {	
      if (icecalGetConnType() != icConnACE2) {
	printf("icecald: no ACE2 present, ignoring ACE HSK read\n");
      } else {
	PORT_CHECK(ace2_port);
	if (icecald_readSensorsOnACE2(auxFd)) 
	  {
	    close(auxFd);
	    exit(1);
	  }
      }
    }
    if (strstr(*argv, "pwron")) {
      if (icecalGetConnType() != icConnACE2) {
	printf("icecald: no ACE2 present, ignoring power on/off\n");
      } else {
	PORT_CHECK(ace2_port);
	if (icecald_powerOnACE2(auxFd))
	  {
	    close(auxFd);
	    exit(1);
	  }
      }
    }
    if (strstr(*argv, "pwroff")) {
      if (icecalGetConnType() != icConnACE2) {
	printf("icecald: no ACE2 present, ignoring power on/off\n");
      } else {
	PORT_CHECK(ace2_port);
	if (icecald_powerOffACE2(auxFd))
	  {
	    close(auxFd);
	    exit(1);
	  }  
      }
    }
    if ((dummy=strstr(*argv, "attA"))) {
       unsigned int attval;
       PORT_CHECK(ace2_port);
       attval = atoi(dummy+strlen("attA"));
       if (setAttenuatorOnIceCal(auxFd, ICECAL_PORTA,
				 attval)) 
	 {
	    close(auxFd);
	    exit(1);
	 }       
    }
    if ((dummy=strstr(*argv, "attB"))) {	  
       unsigned int attval;
       PORT_CHECK(ace2_port);
       attval = atoi(dummy+strlen("attB"));
       if (setAttenuatorOnIceCal(auxFd, ICECAL_PORTB,
				 attval)) 
	 {
	    close(auxFd);
	    exit(1);
	 }       
    }     
    if ((dummy=strstr(*argv,"opA"))) {
       unsigned char mode;
       PORT_CHECK(ace2_port);
       mode = atoi(dummy+strlen("opA"));
       if (mode > ICECAL_OPMODE_NOISE_BURST_AUTO) {
	 fprintf(stderr, "icecald: syntax is opA[0-5]\n");
	 close(auxFd);
	 exit(1);
       }
       if (selectModeOnIceCal(auxFd, ICECAL_PORTA, mode)) {
	 close(auxFd);
	 exit(1);
       }
    }
    if ((dummy=strstr(*argv,"opB"))) {
       unsigned char mode;
       PORT_CHECK(ace2_port);
       mode = atoi(dummy+strlen("opB"));
       if (mode > ICECAL_OPMODE_NOISE_BURST_AUTO) {
	 fprintf(stderr, "icecald: syntax is opB[0-5]\n");
	 close(auxFd);
	 exit(1);
       }
       if (selectModeOnIceCal(auxFd, ICECAL_PORTB, mode)) {
	 close(auxFd);
	 exit(1);
       }
    }
    if ((dummy=strstr(*argv, "antA"))) {
      unsigned char ant = *(dummy + strlen("antA"));
      PORT_CHECK(ace2_port);
      if (ant != ICECAL_ANTENNA_A && ant != ICECAL_ANTENNA_B) {
	fprintf(stderr, "icecald: syntax is antA[0,1]\n");
	close(auxFd);
	exit(1);
      }
      if (selectAntennaOnIceCal(auxFd, ICECAL_PORTA, ant)) {
	close(auxFd);
	exit(1);
      }
    }
    if ((dummy=strstr(*argv, "antB"))) {
      unsigned char ant = *(dummy + strlen("antB"));
      PORT_CHECK(ace2_port);
      if (ant != ICECAL_ANTENNA_A && ant != ICECAL_ANTENNA_B) {
	fprintf(stderr, "icecald: syntax is antB[0,1]\n");
	close(auxFd);
	exit(1);
      }
      if (selectAntennaOnIceCal(auxFd, ICECAL_PORTB, ant)) {
	close(auxFd);
	exit(1);
      }
    }
    if (strstr(*argv, "trigA")) {
      PORT_CHECK(ace2_port);
      if (softTriggerIceCal(auxFd, ICECAL_PORTA)) {
	close(auxFd);
	exit(1);
      }
    }
    if (strstr(*argv, "trigB")) {
      PORT_CHECK(ace2_port);
      if (softTriggerIceCal(auxFd, ICECAL_PORTB)) {
	close(auxFd);
	exit(1);
      }
    }
    argc--;
    argv++;
  }
 if ((retval = disableAtriComponents(auxFd, ext_i2c[ace2_port])) < 0)
    {
      fprintf(stderr, "icecald: error enabling port %s\n", ext_i2c_str[ace2_port]);
      close(auxFd);
      exit(1);
    }  
 // do some close-y thingy
  close(auxFd);
  return 0;
}
