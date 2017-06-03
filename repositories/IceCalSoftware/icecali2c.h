#ifndef ICECALI2C_H
#define ICECALI2C_H


int initializeIceCalI2C(int auxFd);
#ifndef ICECALI2C_C
extern unsigned char icbc_addr;
extern unsigned char ica_addr;
extern unsigned char icb_addr;
#endif

#endif
