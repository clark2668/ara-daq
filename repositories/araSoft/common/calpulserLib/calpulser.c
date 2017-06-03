/*
 * Calpulser ARAAcqd interface.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#include "kvpLib/keyValuePair.h"
#include "fx2ComLib/fx2Com.h"
#include "calpulser.h"
#include "icecal.h"

void basicLogger(int debug_level, char *fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  if (debug_level < LOG_WARNING)
    vfprintf(stderr, fmt, argptr);
  else
    vfprintf(stdout, fmt, argptr);
  va_end(argptr);
}

CalpulserLogger_t calpulserLog = basicLogger;

void calpulser_SetLogger(CalpulserLogger_t logger) {
  calpulserLog = logger;
}

CalpulserConfig_t calpulserConfig;
CalpulserConfig_t *const cpConfig = &calpulserConfig;

#define SET_INT(x,d)    cpConfig->x=kvpGetInt(#x,d);
#define SET_FLOAT(x,f)  cpConfig->x=kvpGetFloat(#x,f);
#define SET_STRING(x,s) {char* tmpStr=kvpGetString(#x);strcpy(cpConfig->x,tmpStr?tmpStr:s);}

int calpulser_ParseConfig() {
  SET_INT(icecalPort,1);                // Unused if PROBE is specified.
  SET_STRING(icecalConnType, "PROBE");  // PROBE, ICECAL, or ACE2
  SET_INT(icecalIdProbe, 1);            // Probe for IDs.
  SET_STRING(iceA, "00000000");         // IceA ID. All 0s in iceA means skip.
  SET_STRING(iceB, "00000000");         // IceB ID. All 0s in iceB means skip.
  SET_INT(antennaIceA,0);
  SET_INT(antennaIceB,0);
  SET_INT(opIceA, 0);
  SET_INT(opIceB, 0);
  SET_INT(attIceA, 0);
  SET_INT(attIceB, 0);
  SET_INT(sensorReadout, 0);
  calpulserLog(LOG_INFO, "%s : parsed configuration file\n", __FUNCTION__);
}

void calpulser_DumpFullConfig() {
    calpulserLog(LOG_INFO, "=====   CURRENT CAL PULSER CONFIGURATION   =====\n");
  if (cpConfig->icecalPort < 0) {	
    calpulserLog(LOG_INFO, "Cal Pulser run control is disabled.\n");
    return;
  }
    calpulserLog(LOG_INFO, "Expansion Port  (icecalPort)        : %d\n", cpConfig->icecalPort);
    calpulserLog(LOG_INFO, "Connection Type (icecalConnType)    : %s\n", cpConfig->icecalConnType);
    calpulserLog(LOG_INFO, "Probe for IceCal ID (icecalIdProbe) : %d\n", cpConfig->icecalIdProbe);
    calpulserLog(LOG_INFO, "IceCal A ID (iceA)                  : %s\n", (strtoul(cpConfig->iceA, NULL, 16)==0) ? "skip" : cpConfig->iceA);
    calpulserLog(LOG_INFO, "IceCal B ID (iceB)                  : %s\n", (strtoul(cpConfig->iceB, NULL, 16)==0) ? "skip" : cpConfig->iceB);
    calpulser_DumpRunConfig();
}

const char *const calpulserRunModes[] = {
  "Off",
  "Triggered Pulse",
  "Automatic Pulse",
  "Noise Source",
  "Triggered Noise Burst",
  "Auto Noise Burst"
};

void calpulser_DumpRunConfig() {
  int doCalA, doCalB;
  doCalA = strtoul(cpConfig->iceA, NULL, 16);
  doCalB = strtoul(cpConfig->iceB, NULL, 16);

    calpulserLog(LOG_INFO, "===== CURRENT CAL PULSER RUN CONFIGURATION =====\n");
  if (!doCalA) {
    calpulserLog(LOG_INFO, "IceCal A configuration is skipped.\n");
  } else {
    calpulserLog(LOG_INFO, "IceCal A Antenna (antennaIceA)      : %s\n",
		 (cpConfig->antennaIceA == 0) ? "Vertical Pol." : "Horizontal Pol.");
    calpulserLog(LOG_INFO, "IceCal A Attenuation (attIceA)      : %d dB\n", cpConfig->attIceA);
    calpulserLog(LOG_INFO, "IceCal A Operating Mode (opIceA)    : %s\n", 
		 (cpConfig->opIceA <= 5) ? calpulserRunModes[cpConfig->opIceA] : "Invalid Mode!");		 
  }
  if (!doCalB) {
    calpulserLog(LOG_INFO, "IceCal B configuration is skipped.\n");
  } else {
    calpulserLog(LOG_INFO, "IceCal B Antenna (antennaIceB)      : %s\n",
		 (cpConfig->antennaIceB == 0) ? "Vertical Pol." : "Horizontal Pol.");
    calpulserLog(LOG_INFO, "IceCal B Attenuation (attIceB)      : %d dB\n", cpConfig->attIceB);
    calpulserLog(LOG_INFO, "IceCal B Operating Mode (opIceB)    : %s\n", 
		 (cpConfig->opIceB <= 5) ? calpulserRunModes[cpConfig->opIceB] : "Invalid Mode!");		 
  }
    calpulserLog(LOG_INFO, "================================================\n");
}

#undef SET_INT
#undef SET_FLOAT
#undef SET_STRING

int calpulser_ApplyConfig(int fd) {
  int allErr;
  unsigned int retVal;
  unsigned int probeFlag;
  int idA, idB;


  probeFlag = 0;
  idA = strtoul(cpConfig->iceA, NULL, 16);
  idB = strtoul(cpConfig->iceB, NULL, 16);
  allErr = CALPULSER_E_OK;
  if (cpConfig->icecalPort < 0)    return allErr; // skip if disabled
  if (!strcmp(cpConfig->icecalConnType, "PROBE")) {
    // Probe.
    cpConfig->icecalPort = probeForIceCal(fd);
    if (cpConfig->icecalPort < 0) return -CALPULSER_PROBE_FAIL;
    // probeForIceCal returns 0-3. icecalPort is 1-4.
    cpConfig->icecalPort++;
    probeFlag = 1;
  } else if (!strcmp(cpConfig->icecalConnType, "ICECAL")) {
    icecalSetConnType(icConnIceCal);
  } else if (!strcmp(cpConfig->icecalConnType, "ACE2")) {
    icecalSetConnType(icConnACE2);
  } else {
    return -CALPULSER_INVALID_CONNTYPE;
  }
  if (cpConfig->icecalPort < 0 || cpConfig->icecalPort > 4) {
    return -CALPULSER_INVALID_PORT;
  } else if (!probeFlag) {
    // If we probed, it's still active.
    retVal = enableAtriComponents(fd, ext_i2c[cpConfig->icecalPort-1]);
    if (retVal < 0) {
      return -CALPULSER_ERR_ENABLE_PORT;
    }
  }
  // ACTIVE EXPANSION PORT AFTER HERE
  if (icecalGetConnType() == icConnACE2 || cpConfig->icecalIdProbe) {
    retVal = initializeIceCal(fd);
    if (retVal < 0) {
      disableAtriComponents(fd, ext_i2c[cpConfig->icecalPort-1]);      
      return -CALPULSER_ERR_INITIALIZE;
    }
  } else {
    if (idA != 0) {
      retVal = icecalAssign(fd, idA, ICECAL_PORTA);
      if (retVal < 0) {
	disableAtriComponents(fd, ext_i2c[cpConfig->icecalPort-1]);
	return -CALPULSER_NO_RESPONSE_PORT_A;
      }
    }
    if (idB != 0) {
      retVal = icecalAssign(fd, idB, ICECAL_PORTB);
      if (retVal < 0) {
	disableAtriComponents(fd, ext_i2c[cpConfig->icecalPort-1]);
	return -CALPULSER_NO_RESPONSE_PORT_B;
      }
    }
  }
  // At this point we have a connection, and IceCals are assigned.
  // First set the attenuator.
  if (idA) {
     retVal = setAttenuatorOnIceCal(fd, ICECAL_PORTA, cpConfig->attIceA);
     if (retVal < 0) allErr |= CALPULSER_SET_ERR_FLAG | CALPULSER_SET_ATTA_ERR;

     // Next the antenna
     if (cpConfig->antennaIceA) {
       retVal = selectAntennaOnIceCal(fd, ICECAL_PORTA, ICECAL_ANTENNA_B);
     } else {
       retVal = selectAntennaOnIceCal(fd, ICECAL_PORTA, ICECAL_ANTENNA_A);
     }
     if (retVal < 0) allErr |= CALPULSER_SET_ERR_FLAG | CALPULSER_SET_ANTA_ERR;     

     retVal = selectModeOnIceCal(fd, ICECAL_PORTA, cpConfig->opIceA);
     if (retVal < 0) allErr |= CALPULSER_SET_ERR_FLAG | CALPULSER_SET_OPA_ERR;
  }
  if (idB) {     
     retVal = setAttenuatorOnIceCal(fd, ICECAL_PORTB, cpConfig->attIceB);
     if (retVal < 0) allErr |= CALPULSER_SET_ERR_FLAG | CALPULSER_SET_ATTB_ERR;
  
     if (cpConfig->antennaIceB) {
       retVal = selectAntennaOnIceCal(fd, ICECAL_PORTB, ICECAL_ANTENNA_B);
     } else {
       retVal = selectAntennaOnIceCal(fd, ICECAL_PORTB, ICECAL_ANTENNA_A);
     }

     if (retVal < 0) allErr |= CALPULSER_SET_ERR_FLAG | CALPULSER_SET_ANTB_ERR;     

     retVal = selectModeOnIceCal(fd, ICECAL_PORTB, cpConfig->opIceB);
     if (retVal < 0) allErr |= CALPULSER_SET_ERR_FLAG | CALPULSER_SET_OPB_ERR;  
  }
   
  // And we're done!
  return allErr;
}
