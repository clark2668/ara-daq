#ifndef FX2JTAG_H
#define FX2JTAG_H

int fx2JtagOpen(int fd);
int fx2JtagClose(int fd);
int fx2JtagSend(int fd, unsigned char *buf, unsigned int nbits);

#endif