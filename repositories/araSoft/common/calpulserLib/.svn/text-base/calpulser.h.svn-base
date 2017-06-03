/*! \file calpulser.h
 *  \brief ARAAcqd calpulser configuration interface.
 *
 *  Functions for setting up the calpulser configuration and
 *  applying it. These mostly call IceCal functions.
 *
 *  Monitoring integration needs to go here too.
 *
 */

#ifndef CALPULSER_H
#define CALPULSER_H

typedef struct {
  int icecalPort;
  char icecalConnType[16];
  int icecalIdProbe;
  char iceA[16];
  char iceB[16];
  int antennaIceA;
  int antennaIceB;
  int opIceA;
  int opIceB;
  int attIceA;
  int attIceB;
  int sensorReadout;
} CalpulserConfig_t;

// This is for accessing logging stuff in external code.
// By default calpulser.h functions just log to stdout/stderr.
typedef void (*CalpulserLogger_t)(int debug_level, char *fmt, ...);

void calpulser_SetLogger(CalpulserLogger_t logger);
int calpulser_ParseConfig();
int calpulser_ApplyConfig(int sockFd);
void calpulser_DumpFullConfig();
void calpulser_DumpRunConfig();

#define CALPULSER_E_OK 0
#define CALPULSER_PROBE_FAIL 1
#define CALPULSER_INVALID_PORT 2
#define CALPULSER_INVALID_CONNTYPE 3
#define CALPULSER_ERR_ENABLE_PORT 4
#define CALPULSER_ERR_INITIALIZE 5
#define CALPULSER_NO_RESPONSE_PORT_A 6
#define CALPULSER_NO_RESPONSE_PORT_B 7

#define CALPULSER_SET_ERR_FLAG 64
#define CALPULSER_SET_ANTA_ERR 1
#define CALPULSER_SET_ANTB_ERR 2
#define CALPULSER_SET_OPA_ERR 4
#define CALPULSER_SET_OPB_ERR 8
#define CALPULSER_SET_ATTA_ERR 16
#define CALPULSER_SET_ATTB_ERR 32

#endif

