#ifndef ACE2_H
#define ACE2_H

// Note: a register can be transformed from _A to _B by |ing with these
// They are also the RHR/THR for each port as well (since RHR_A is 0x00)
#define ACE2_PORTA 0x00
#define ACE2_PORTB 0x02

#define PORT_NAME(x) (('A' + (x>>1)))

// there are more than this, but this is minimal
typedef enum uartRegisters {
  LCR_A = 0x18,
  LCR_B = 0x1A,
  DLL_A = 0x00,
  DLH_A = 0x08,
  DLL_B = 0x02,
  DLH_B = 0x0A,
  FCR_A = 0x10,
  FCR_B = 0x12,
  RHR_A = 0x00,
  THR_A = 0x00,
  RHR_B = 0x02,
  THR_B = 0x02,
  RXLVL_A = 0x48,
  RXLVL_B = 0x4A
} UartRegister_t;

struct uartInitRegisterWrite {
  UartRegister_t reg;
  unsigned char val;
  const char *descr;
};

#define N_INIT_STEPS ((sizeof(uartInit))/sizeof(struct uartInitRegisterWrite))


int getBytesPendingOnACE2(int auxFd,
			  unsigned char addr,
			  unsigned char *pendingA,
			  unsigned char *pendingB);
int readSensorsOnACE2(int auxFd,
		      int addr,
		      unsigned short *voltageCounts,
		      unsigned short *currentCounts);
int clearReceiveFifoOnACE2(int auxFd,
			   unsigned char addr,
			   unsigned char port);

int waitForBytesACE2(int auxFd,
		     unsigned char addr,
		     unsigned char port,
		     unsigned char bytes,
		     unsigned int timeout);

int writeBytesACE2(int auxFd,
		   unsigned char addr,
		   unsigned char port,
		   unsigned char nbytes,
		   unsigned char *tx_buf);

int readBytesACE2(int auxFd,
		  unsigned char addr,
		  unsigned char port,
		  unsigned char bytes,
		  unsigned char *rx_buf);

#endif
