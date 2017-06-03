#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fx2ComLib/fx2Com.h"
#define ICECALACE2_C
#include "ace2.h"
#include "icecalace2.h"

const unsigned char uart_addr = 0x9A;
const unsigned char ace2_hswap = 0x80;

void dumpBytes(char *p, unsigned char len);

enum icecalFrameChars {
  stx = 0x02,
  eot = 0x04
} IceCalFrameChar_t;

struct uartInitRegisterWrite uartInit[] = {
  { LCR_A, 0x83, "set UART A line control register (LCR) bit 7" },
  { LCR_B, 0x83, "set UART B line control register (LCR) bit 7" },
  { DLL_A, 0x60, "set UART A divisor latch low (DLL) to 96" },
  { DLH_A, 0x00, "set UART A divisor latch high (DLH) to 0" },
  { DLL_B, 0x60, "set UART B divisor latch low (DLL) to 96" },
  { DLH_B, 0x00, "set UART B divisor latch high (DLH) to 0" },
  { LCR_A, 0x03, "unset UART A line control register (LCR) bit 7" },
  { LCR_B, 0x03, "unset UART B line control register (LCR) bit 7" },
  { FCR_A, 0x01, "enable UART A FIFOs by setting FCR bit 0"},
  { FCR_B, 0x01, "enable UART B FIFOs by setting FCR bit 1"},   
  { FCR_A, 0x07, "reset UART A FIFOs by setting FCR bit 2,3"},
  { FCR_B, 0x07, "reset UART B FIFOs by setting FCR bit 2,3"}	 
};

// Read all sensors on the two icecal boards on UART A.
const unsigned char icecalHskMessageUART_A[] = 
  { 0x00, 
    stx, 'S', 'L', 'V', eot, stx, 'S', 'P', 'T', eot, 
    stx, 'R', 'L', 'V', eot, stx, 'R', 'P', 'T', eot 
  };

// Read all sensors on the two icecal boards on UART B.
const unsigned char icecalHskMessageUART_B[] = 
  { 0x02, 
    stx, 'S', 'L', 'V', eot, stx, 'S', 'P', 'T', eot, 
    stx, 'R', 'L', 'V', eot, stx, 'R', 'P', 'T', eot 
  };

#define NBYTES_ICECALHSKMESSAGE_A ((sizeof(icecalHskMessageUART_A))/sizeof(unsigned char))
#define NBYTES_ICECALHSKMESSAGE_B ((sizeof(icecalHskMessageUART_B))/sizeof(unsigned char))





// finds eot char, within n chars
char *findEndOfTransmission(char *p, unsigned char n);
// all commands are 3 chars long, so length on expect is 3
char *parseIceCalMessage(char *buf, unsigned char *remaining,
			 const char *expect, int *response);

// Sets UARTs to 9600 bps, 8N1. ACE2 crystal is 14.7456 kHz,
// and (DLH,DLL) needs to be set to 16X the line rate.
// To set DLH, DLL you need to enable the latch in the LCR.
// This also enables the 64 byte FIFOs.
int initializeIceCalACE2(int auxFd) {
  int i, retval;
  for (i = 0;i<N_INIT_STEPS; i++) {
    if ((
	 retval = writeToI2CRegister(auxFd, uart_addr,
				     uartInit[i].reg, uartInit[i].val)) < 0) {
      fprintf(stderr, "icecald: error %d at init step %d: %s\n",
	      retval, i, uartInit[i].descr);
      return retval;
    }
  }

  return 0;
}

int setAttenuatorOnIceCalACE2(int auxFd, unsigned char port,
			      unsigned char attVal) 
{
   unsigned char pointer_byte;
   unsigned char txByte[7] = {stx, 'R','S','A',0x00, 0x00, eot};
   unsigned char *rx;
   unsigned char rx_buf[7];
   unsigned char rxbytes;
   unsigned char pending;
   int retval;
   if (attVal > 31) return -1;
   if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;

   if (port == ICECAL_PORTA) {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, &pending, NULL)))
       return retval;
   } else {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, NULL, &pending)))
       return retval;
   }
   if (pending) clearReceiveFifoOnACE2(auxFd, uart_addr, port);

//   txByte[0] = port;
   snprintf(&(txByte[4]), 3, "%2.2d", attVal);
   txByte[6] = eot;
   printf("icecal: sending %2.2x %c%c%c%c%c %2.2x to port %c\n", txByte[0], txByte[1], txByte[2], txByte[3], txByte[4], txByte[5], txByte[6], PORT_NAME(port));
   if ((retval = writeBytesACE2(auxFd, uart_addr, port, 7, txByte)) < 0) {
      fprintf(stderr, "error %d writing set pulse attenuator command to UART\n",
	      retval);
      return retval;
   }
   // Parse response here.
   // We wait for 5 because we could get a NACK
   if ((retval = waitForBytesACE2(auxFd, uart_addr, port, 7, 1000000)) < 5) {
     if (retval >= 0) {
       fprintf(stderr, "icecal: no response in time to RSA message on port %c (%d bytes pending)\n", PORT_NAME(port), retval);
       return -1;
     } else {
       fprintf(stderr, "icecal: error %d waiting for response to RSA message on port %c\n", retval, PORT_NAME(port));
     }
   }
   rxbytes = retval;
   // Now read the bytes...
   if ((retval = readBytesACE2(auxFd, uart_addr, port, rxbytes, rx_buf)) < 0) {
     fprintf(stderr, "icecal: error %d reading response bytes to RSA message on part %c\n", retval, PORT_NAME(port));
     return -1;
   }
   printf("icecal: received message %c%c%c%c%c\n", rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5]);

   retval = -1;
   rx = rx_buf;
   while (rxbytes) {
     rx = parseIceCalMessage(rx, &rxbytes, "RSA", &retval);
     if (retval == -1) {
       fprintf(stderr, "icecal: received NACK from port %c?\n", PORT_NAME(port));
     } else if (retval == 0) {
       retval = 0;
       printf("icecal: response found to RSA on port %c\n", PORT_NAME(port));
       break;
     }
     if (!rx) break;
   }
   return retval;
}

int selectModeOnIceCalACE2(int auxFd, unsigned char port,
			     unsigned char mode)
{
   unsigned char pointer_byte;
   unsigned char txByte[6] = {stx, 'R','O','M',0x00, eot};
   unsigned char rx_buf[6];
   unsigned char rxbytes;
   unsigned char pending;
   unsigned char *rx;
   int retval;
   if (mode > ICECAL_OPMODE_NOISE_BURST_AUTO) return -1;
   if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;

   if (port == ICECAL_PORTA) {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, &pending, NULL)))
       return retval;
   } else {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, NULL, &pending)))
       return retval;
   }
   if (pending) clearReceiveFifoOnACE2(auxFd, uart_addr, port);


   txByte[4] = '0' + mode;
   printf("icecal: sending %2.2x %c%c%c%c %2.2x to port %c\n", txByte[0], txByte[1], txByte[2], 
	  txByte[3], txByte[4], txByte[5], PORT_NAME(port));
   if ((retval = writeBytesACE2(auxFd, uart_addr, port, 6, txByte)) < 0) {
      fprintf(stderr, "error %d writing set op mode command to UART\n",
	      retval);
      return retval;
   }
   // Parse response here.
   if ((retval = waitForBytesACE2(auxFd, uart_addr, port, 6, 1000000)) < 5) {
     if (retval >= 0) {
       fprintf(stderr, "icecal: no response in time to ROM message on port %c (%d bytes pending)\n", PORT_NAME(port), retval);
       return -1;
     } else {
       fprintf(stderr, "icecal: error %d waiting for response to ROM message on port %c\n", retval, PORT_NAME(port));
     }
   }
   rxbytes = retval;
   // Now read the bytes...
   if ((retval = readBytesACE2(auxFd, uart_addr, port, rxbytes, rx_buf)) < 0) {
     fprintf(stderr, "icecal: error %d reading response bytes to ROM message on part %c\n", retval, PORT_NAME(port));
     return -1;
   }
   printf("icecal: received message %c%c%c%c\n", rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4]);

   retval = -1;
   rx = rx_buf;
   while (rxbytes) {
     rx = parseIceCalMessage(rx, &rxbytes, "ROM", &retval);
     if (retval == -1) {
       fprintf(stderr, "icecal: received NACK from port %c?\n", PORT_NAME(port));
     } else if (retval == 0) {
       retval = 0;
       printf("icecal: response found to ROM on port %c\n", PORT_NAME(port));
       break;
     }
     if (!rx) break;
   }
   return retval;
}

int selectAntennaOnIceCalACE2(int auxFd, unsigned char port,
				 unsigned char antenna)
{
   unsigned char pointer_byte;
   unsigned char txByte[6] = {stx, 'R','A','S',0x00, eot};
   unsigned char rx_buf[6];
   unsigned char rxbytes;
   unsigned char pending;
   unsigned char *rx;
   int retval;
   if (antenna != ICECAL_ANTENNA_A && antenna != ICECAL_ANTENNA_B) return -1;
   if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;

   if (port == ICECAL_PORTA) {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, &pending, NULL)))
       return retval;
   } else {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, NULL, &pending)))
       return retval;
   }
   if (pending) clearReceiveFifoOnACE2(auxFd, uart_addr, port);


   txByte[4] = antenna;
   printf("icecal: sending %2.2x %c%c%c%c %2.2x to port %c\n", txByte[0], txByte[1], txByte[2], 
	  txByte[3], txByte[4], txByte[5], PORT_NAME(port));

   if ((retval = writeBytesACE2(auxFd, uart_addr, port, 6, txByte)) < 0) {
      fprintf(stderr, "error %d writing select antenna command to UART\n",
	      retval);
      return retval;
   }
   // Parse response here.
   if ((retval = waitForBytesACE2(auxFd, uart_addr, port, 6, 1000000)) < 5) {
     if (retval >= 0) {
       fprintf(stderr, "icecal: no response in time to RAS message on port %c (%d bytes pending)\n", PORT_NAME(port), retval);
       return -1;
     } else {
       fprintf(stderr, "icecal: error %d waiting for response to RAS message on port %c\n", retval, PORT_NAME(port));
     }
   }
   rxbytes = retval;
   // Now read the bytes...
   if ((retval = readBytesACE2(auxFd, uart_addr, port, rxbytes, rx_buf)) < 0) {
     fprintf(stderr, "icecal: error %d reading response bytes to RAS message on part %c\n", retval, PORT_NAME(port));
     return -1;
   }
   printf("icecal: received message %c%c%c%c\n", rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4]);

   retval = -1;
   while (rxbytes) {
     rx = parseIceCalMessage(rx, &rxbytes, "RAS", &retval);
     if (retval == -1) {
       fprintf(stderr, "icecal: received NACK from port %c?\n", PORT_NAME(port));
     } else if (retval == 0) {
       retval = 0;
       printf("icecal: response found to RAS on port %c\n", PORT_NAME(port));
       break;
     }
     if (!rx) break;
   }
   return retval;
}

int softTriggerIceCalACE2(int auxFd, unsigned char port)
{
   unsigned char pointer_byte;
   unsigned char txByte[5] = {stx, 'R','S','T', eot};
   unsigned char rx_buf[5];
   unsigned char *rx;
   unsigned char rxbytes;
   int retval;
   unsigned char pending;
   if (port != ICECAL_PORTA && port != ICECAL_PORTB) return -1;

   if (port == ICECAL_PORTA) {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, &pending, NULL)))
       return retval;
   } else {
     if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, NULL, &pending)))
       return retval;
   }
   if (pending) clearReceiveFifoOnACE2(auxFd, uart_addr, port);

   printf("icecal: sending %2.2x %c%c%c %2.2x to port %c\n", txByte[0], txByte[1], txByte[2], 
	  txByte[3], txByte[4], PORT_NAME(port));

   if ((retval = writeBytesACE2(auxFd, uart_addr, port, 5, txByte)) < 0) {
      fprintf(stderr, "error %d writing soft trigger command to UART\n",
	      retval);
      return retval;
   }
   // Parse response here.
   if ((retval = waitForBytesACE2(auxFd, uart_addr, port, 5, 1000000)) < 5) {
     if (retval >= 0) {
       fprintf(stderr, "icecal: no response in time to RST message on port %c (%d bytes pending)\n", PORT_NAME(port), retval);
       return -1;
     } else {
       fprintf(stderr, "icecal: error %d waiting for response to RST message on port %c\n", retval, PORT_NAME(port));
     }
   }
   rxbytes = retval;
   // Now read the bytes...
   if ((retval = readBytesACE2(auxFd, uart_addr, port, rxbytes, rx_buf)) < 0) {
     fprintf(stderr, "icecal: error %d reading response bytes to RST message on part %c\n", retval, PORT_NAME(port));
     return -1;
   }
   printf("icecal: received message %c%c%c\n", rx_buf[1], rx_buf[2], rx_buf[3]);
   retval = -1;
   rx = rx_buf;
   while (rxbytes) {
     rx = parseIceCalMessage(rx, &rxbytes, "RST", &retval);
     if (retval == -1) {
       fprintf(stderr, "icecal: received NACK from port %c?\n", PORT_NAME(port));
     } else if (retval == 0) {
       retval = 0;
       printf("icecal: response found to RST on port %c\n", PORT_NAME(port));
       break;
     }
     if (!rx) break;
   }
   return retval;
}

// finds eot char, within n chars
char *findEndOfTransmission(char *p, unsigned char n) 
{
   while (n) 
     {
	if (*p == eot) return p;
	p++; n--;
     }
   return NULL;
}

// all commands are 3 chars long, so length on expect is 3
char *parseIceCalMessage(char *buf, unsigned char *remaining,
			 const char *expect, int *response) {
   unsigned char bytesLeft;
   unsigned char bytesUsed;
   char *p;
   char *q;
   unsigned char rxBytes[32];
   if (remaining == NULL) return NULL;
   if (buf == NULL) return buf;
   bytesLeft = *remaining;
   p = buf;
   while (bytesLeft) {
    if (*p == stx) {
       if (expect == NULL) {
	  p = findEndOfTransmission(p, bytesLeft);
	  break;
       } else {
	bytesLeft--;
	if (bytesLeft < 3) {
	  p = NULL;
	  break;
	}
	p++;
	if (p[0] == expect[0] && p[1] == expect[1] && p[2] == expect[2]) {
	   p += 3;
	   bytesLeft -= 3;
	   q = findEndOfTransmission(p, bytesLeft);
	   if (q == NULL) break;
	   // q now points to eot, p to the first char after message
 	   // so q-p is number of bytes to copy
	   memcpy(rxBytes, p, q-p);
	   rxBytes[q-p] = 0x00;
	   if (response && ((q-p)>1)) 
	     {
		*response = atoi(rxBytes);
	     }
	   else if ((q-p)==1) {
	     *response = 0;
	   }
	   p = q;
	   break;
	} else if (p[0] == 'N' && p[1] == 'A' && p[2] == 'C') {
	   // nack
	   p = findEndOfTransmission(p, bytesLeft);
	   if (response) *response = -1;
	   break;
	} else {
	   // unknown message received, abandon all ye hope
	   p = NULL;
	   break;
	}
       }
    } else {
      p++; bytesLeft--;
    }
   }
   // if p == NULL ,return NULL
   if (p == NULL) return NULL;
   // otherwise increment by 1 (move past eot)
   p++;
   // and subtract off remaining bytes
   bytesUsed = p-buf;
   *remaining -= bytesUsed;
   return p;
}

// Reads sensors on ACE2. Right now this just prints out the received
// data, will parse later.
int readSensorsOnIceCalACE2(int auxFd,
			    IceCalSensors_t *sensorsA,
			    IceCalSensors_t *sensorsB) {
  unsigned char pendingA, pendingB;
  unsigned char remain;
  int retval;
  char rx_buf[64];
  char *p;
  int i;
  unsigned char pointer_byte;

  if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, &pendingA, &pendingB)))
    return retval;
  if (pendingA) clearReceiveFifoOnACE2(auxFd, uart_addr, ICECAL_PORTA);
  if (pendingB) clearReceiveFifoOnACE2(auxFd, uart_addr, ICECAL_PORTB);

  if ((retval = 
      writeToI2C(auxFd, uart_addr, NBYTES_ICECALHSKMESSAGE_A, 
		 icecalHskMessageUART_A)) < 0) {
    fprintf(stderr, "error %d writing HSK to UART A\n", retval);
    return retval;
  }  
  if ((retval = 
      writeToI2C(auxFd, uart_addr, NBYTES_ICECALHSKMESSAGE_B, 
		 icecalHskMessageUART_B)) < 0) {
    fprintf(stderr, "error %d writing HSK to UART B\n", retval);
    return retval;
  }  
  // now wait a while 
  sleep(1);
  if ((retval = getBytesPendingOnACE2(auxFd, uart_addr, &pendingA, &pendingB)))
    return retval;
  if (!pendingA) {
    fprintf(stderr, "icecald: hsk: no response on Port A\n");
    sensorsA->error = ICECAL_ERROR_NO_RESPONSE;
  } else {
    printf("icecald: hsk: %d bytes pending on port A\n", pendingA);
    pointer_byte = RHR_A;
    if ((retval = writeToI2C(auxFd, uart_addr, 1, &pointer_byte)) < 0) 
	{
	  fprintf(stderr, "error %d setting pointer register to RHR_A\n", retval);
	   return retval;
	}	  
    if ((retval = readFromI2C(auxFd, uart_addr | 0x1, pendingA, rx_buf)) < 0) {
      fprintf(stderr, "error %d reading %d bytes from UART A\n",
	      retval, pendingA);
      return retval;
    }

    dumpBytes(rx_buf, pendingA);

    p = rx_buf;
    remain = pendingA;
     // doofy parsing
    if ((p=parseIceCalMessage(p, &remain, "SLV", &sensorsA->surfaceVoltage)) == NULL) {
       fprintf(stderr, "%s : no SLV or NACK found as first message on port A\n", __FUNCTION__);
       sensorsA->error = ICECAL_ERROR_PROTOCOL;
       goto readPortB;
    }
    if ((p=parseIceCalMessage(p, &remain, "SPT", &sensorsA->surfaceTemp)) == NULL) {
       fprintf(stderr, "%s : no SPT or NACK found as second message on port A\n", __FUNCTION__);
       sensorsA->error = ICECAL_ERROR_PROTOCOL;
       goto readPortB;
    }
    if ((p=parseIceCalMessage(p, &remain, "RLV", &sensorsA->remoteVoltage)) == NULL) {
       fprintf(stderr, "%s : no RLV or NACK found as third message on port A\n", __FUNCTION__);
       sensorsA->error = ICECAL_ERROR_PROTOCOL;
       goto readPortB;
    }
    if ((p=parseIceCalMessage(p, &remain, "RPT", &sensorsA->remoteTemp)) == NULL) {
       fprintf(stderr, "%s : no RPT or NACK found as last message on port A\n", __FUNCTION__);
       sensorsA->error = ICECAL_ERROR_PROTOCOL;
       goto readPortB;
    }
    sensorsA->error = ICECAL_ERROR_NONE;
     /*
     for (i=0;i<pendingA;i++) {
      printf("%c", rx_buf[i]);
    }
     */
  }
 readPortB:
  if (!pendingB) {
     fprintf(stderr, "icecald: hsk: no response on Port B\n");
     sensorsB->error = ICECAL_ERROR_NO_RESPONSE;
  } else {
    printf("icecald: hsk: %d bytes pending on port B\n", pendingB);
    pointer_byte = RHR_B;
    if ((retval = writeToI2C(auxFd, uart_addr, 1, &pointer_byte)) < 0) 
	{
	  fprintf(stderr, "error %d setting pointer register to RHR_B\n", retval);
	   return retval;
	}	   
    if ((retval = readFromI2C(auxFd, uart_addr | 0x1, pendingB, rx_buf)) < 0) {
      fprintf(stderr, "error %d reading %d bytes from UART B\n",
	      retval, pendingB);
      return retval;
    }

    dumpBytes(rx_buf, pendingB);

    p = rx_buf;
    remain = pendingB;
    // doofy parsing
    if ((p=parseIceCalMessage(p, &remain, "SLV", &sensorsB->surfaceVoltage)) == NULL) {
       fprintf(stderr, "%s : no SLV or NACK found as first message on port B\n", __FUNCTION__);
       sensorsB->error = ICECAL_ERROR_PROTOCOL;
       goto done;
    }
    if ((p=parseIceCalMessage(p, &remain, "SPT", &sensorsB->surfaceTemp)) == NULL) {
       fprintf(stderr, "%s : no SPT or NACK found as second message on port B\n", __FUNCTION__);
       sensorsB->error = ICECAL_ERROR_PROTOCOL;
       goto done;
    }
    if ((p=parseIceCalMessage(p, &remain, "RLV", &sensorsB->remoteVoltage)) == NULL) {
       fprintf(stderr, "%s : no RLV or NACK found as third message on port B\n", __FUNCTION__);
       sensorsB->error = ICECAL_ERROR_PROTOCOL;
       goto done;
    }
    if ((p=parseIceCalMessage(p, &remain, "RPT", &sensorsB->remoteTemp)) == NULL) {
       fprintf(stderr, "%s : no RPT or NACK found as last message on port B\n", __FUNCTION__);
       sensorsB->error = ICECAL_ERROR_PROTOCOL;
       goto done;
    }
    sensorsB->error = ICECAL_ERROR_NONE;
     /*
     for (i=0;i<pendingB;i++) {
      printf("%c", rx_buf[i]);
    }
      */
  }
 done:
  return 0;
}

void dumpBytes(char *p, unsigned char len) {
  int i;
  for (i=0;i<len;i++) {
    printf("%2.2x ", p[i]);
    if (!(i+1)%10) printf("\n");
  }
}
