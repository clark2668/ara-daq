#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef USB_I2C_DIOLAN
#include "i2cComLib/i2cCom.h"
#else
#include "fx2ComLib/fx2Com.h"
#endif

#include "ace2.h"

int clearReceiveFifoOnACE2(int auxFd,
			   unsigned char addr,
			   unsigned char port) {
   unsigned char txByte[2];
   int retval;
   if (port != ACE2_PORTA && port != ACE2_PORTB) {
      fprintf(stderr, "%s : bad port (%d)?\n", __FUNCTION__, port);
      return -1;
   }
   // registers can be transformed from A to B by |ing with ACE2_PORTB
   txByte[0] = FCR_A | port;
   // FIFO enable and reset receive FIFO
   txByte[1] = 0x03;
   if ((retval = writeToI2C(auxFd, addr, 2, txByte)) < 0) {
      fprintf(stderr, "%s : error %d resetting receive FIFO\n", 
	      __FUNCTION__, retval);
      return retval;
   }
   return 0;
}  

// Gets number of bytes pending on the two UARTs on an ACE2.
int getBytesPendingOnACE2(int auxFd,
				unsigned char addr,
				unsigned char *pendingA,
				unsigned char *pendingB) {
  unsigned char tx_byte;
  int retval;

  if (pendingA) {
    tx_byte = RXLVL_A;
    if ((retval = writeToI2C(auxFd, addr, 1, &tx_byte)) < 0) {
      fprintf(stderr, "error %d setting register to RXLVL_A\n", retval);
      return retval;
    }
    if ((retval = readFromI2C(auxFd, addr | 0x1, 1, &tx_byte)) < 0) {
      fprintf(stderr, "error %d reading RXLVL_A\n",
	      retval);
      return retval;
    }
    *pendingA = tx_byte;
  }

  if (pendingB) {
    tx_byte = RXLVL_B;
    if ((retval = writeToI2C(auxFd, addr, 1, &tx_byte)) < 0) {
      fprintf(stderr, "error %d setting register to RXLVL_B\n", retval);
      return retval;
    }
    if ((retval = readFromI2C(auxFd, addr | 0x1, 1, &tx_byte)) < 0) {
      fprintf(stderr, "error %d reading RXLVL_B\n",
	      retval);
      return retval;
    }
    *pendingB = tx_byte;
  }
  return 0;
}

int writeBytesACE2(int auxFd,
		   unsigned char addr,
		   unsigned char port,
		   unsigned char nbytes,
		   unsigned char *tx_buf) {
  unsigned char tx_data[255];
  int retval;
  memcpy(&tx_data[1], tx_buf, nbytes);
  tx_data[0] = port;
  if ((retval = writeToI2C(auxFd, addr, nbytes+1, tx_data)) < 0) {
    fprintf(stderr, "ace2: error %d writing %d bytes to port %c\n", retval, nbytes+1, PORT_NAME(port));
    return retval;
  }
  return nbytes;
}

int readBytesACE2(int auxFd,
		  unsigned char addr,
		  unsigned char port,
		  unsigned char bytes,
		  unsigned char *rx_buf) {
  unsigned char tx_byte;
  int retval;
  tx_byte = RHR_A | port;
  if ((retval = writeToI2C(auxFd, addr, 1, &tx_byte)) < 0) 
    {
      fprintf(stderr, "error %d setting pointer register to RHR_%c\n", retval, PORT_NAME(port));
      return retval;
    }	   
  if ((retval = readFromI2C(auxFd, addr | 0x1, bytes, rx_buf)) < 0) {
    fprintf(stderr, "error %d reading %d bytes from UART %c\n",
	    retval, bytes, PORT_NAME(port));
    return retval;
  }  
  return bytes;
}



int waitForBytesACE2(int auxFd,
		      unsigned char addr,
		      unsigned char port,
		      unsigned char bytes,
		      unsigned int timeout) {
  unsigned char tx_byte;
  unsigned char pending = 0;
  int retval;
  tx_byte = RXLVL_A | port;
  if ((retval = writeToI2C(auxFd, addr, 1, &tx_byte)) < 0) {
    fprintf(stderr, "error %d setting register to RXLVL_%c\n", retval, PORT_NAME(port));
    return retval;
  }
  do {
    if ((retval = readFromI2C(auxFd, addr | 0x1, 1, &pending)) < 0) {
      fprintf(stderr, "error %d reading RXLVL_%c\n", retval, PORT_NAME(port));
      return retval;
    }
    if (pending >= bytes) break;
    // A full read cycle at 400 kHz should take ~50 us.
    // So this is approximately 100 us per.
    usleep(50);
    if (timeout >= 100) timeout -= 100;
  } while (timeout > 100);
  return pending;
}

int readSensorsOnACE2(int auxFd,
		      int addr,
		      unsigned short *voltageCounts,
		      unsigned short *currentCounts) 
{
   unsigned char txByte;
   unsigned char rxByte[3];
   int retval;
   
   txByte = 0x1A;
   if ((retval = writeToI2C(auxFd, addr, 1, &txByte)) < 0) {
      fprintf(stderr, "error %d starting conversion on ACE2 sensors\n", retval);
      return retval;
   }
   if ((retval = readFromI2C(auxFd, addr | 0x1, 3, rxByte)) < 0) 
     {
	fprintf(stderr, "error %d reading voltage/current bytes from ACE2\n", retval);
	return retval;
     }
   if (voltageCounts)
     *voltageCounts = (rxByte[0] << 4) | ((rxByte[2] & 0xF0) >> 4);
   if (currentCounts)
     *currentCounts = (rxByte[1] << 4) | ((rxByte[2] & 0x0F));
   return 0;
}

   
   
