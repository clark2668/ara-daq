#ifndef ATRICOMPONENTS_H
#define ATRICOMPONENTS_H


typedef enum atriExpansionPort {
  EX1 = 0,
  EX2 = 1,
  EX3 = 2,
  EX4 = 3
} AtriExpansionPort_t;

int enableExpansionPort(int auxFd, AtriExpansionPort_t port);
int disableExpansionPort(int auxFd, AtriExpansionPort_t port);
int probeExpansionPort(int auxFd, AtriExpansionPort_t port);

typedef enum atriComponentMask {
  EX1_mask = 0x01,
  EX2_mask = 0x02,
  EX3_mask = 0x04,
  EX4_mask = 0x08,
  gpsPowerMask = 0x20,
  fpgaOscillatorMask = 0x40
} AtriComponentMask_t;

extern const AtriComponentMask_t ext_i2c[4];
extern const char *ext_i2c_str[4];
#endif
