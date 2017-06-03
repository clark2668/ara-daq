/*! \file atriDoThresholdScan.c
HL 2013
*/


#include "araSoft.h"
#include "atriDefines.h"
//#include "atriUsefulDefines.h"
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>

#include "common/atriComLib/atriCom.h"  // ~/WorkingDAQ/common/atriComLib/atriCom.h
//Stuff for socket
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int openAtriControlSocket();

/** \brief I2C write.
 *
 */
void copyTwoBytes(uint8_t *data, uint16_t *value);
void copyFourBytes(uint8_t *data, uint32_t *value);
void copyTwoBytes(uint8_t *data, uint16_t *value)
{
  memcpy(value,data,2);
}

void copyFourBytes(uint8_t *data, uint32_t *value)
{
  memcpy(value,data,4);
}



int atriI2CWrite(unsigned int fd,
		 AtriDaughterStack_t stack,
		 unsigned char address,
		 unsigned int length,
		 unsigned char *data,
		 AtriControlPacket_t *controlPacket,
		 AtriControlPacket_t *responsePacket) {
  unsigned int numBytes;
//int i = 0;
  // Clear the R/~W bit just to be sure. Trying to write to
  // a read address results in a timeout.
  address &= ~(0x1);
  
  controlPacket->header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket->header.packetLocation=ATRI_LOC_I2C_DB1 + stack;
  controlPacket->header.packetLength=2+length;
  controlPacket->data[0] = I2C_DIRECT;
  controlPacket->data[1] = address;
  memcpy(controlPacket->data+2, data, length);
  controlPacket->data[controlPacket->header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if (send(fd, (char*)controlPacket, sizeof(AtriControlPacket_t), 0) == -1) {
    perror("send");
    return 0;
  }
  //Now wait for response
  if ((numBytes=recv(fd, (char*)responsePacket,sizeof(AtriControlPacket_t),
		     0)) > 0) {      
    if(responsePacket->header.packetLength == 3) {
      return 0;
    } else {
      fprintf(stderr, "%s : unknown packet src: %d len: %d received\n",
	      __FUNCTION__,
	      responsePacket->header.packetLocation,
	      responsePacket->header.packetLength);
      return -1;
    }
  } else {
    perror("recv");
    return -1;
  }
  return 0;
}

/** \brief I2C read.
 *
 */
int atriI2CRead(unsigned int fd,
		AtriDaughterStack_t stack,
		 unsigned char address,
		 unsigned int length,
		 unsigned char *data,
		 AtriControlPacket_t *controlPacket,
		 AtriControlPacket_t *responsePacket) {
  unsigned int numBytes;
//int i = 0;
  // Set the R/~W bit.
  address |= 0x1;
  
  controlPacket->header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket->header.packetLocation=ATRI_LOC_I2C_DB1 + stack;
  controlPacket->header.packetLength=3;
  controlPacket->data[0] = I2C_DIRECT;
  controlPacket->data[1] = address;
  controlPacket->data[2] = length;
  controlPacket->data[controlPacket->header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if (send(fd, (char*)controlPacket, sizeof(AtriControlPacket_t), 0) == -1) {
    perror("send");
    return 0;
  }
  //Now wait for response
  if ((numBytes=recv(fd, (char*)responsePacket,sizeof(AtriControlPacket_t),
		     0)) > 0) {      
    if(responsePacket->header.packetLength == length+2) {
      memcpy(data, responsePacket->data+2, length);
      return 0;
    } else {
      fprintf(stderr, "%s : unknown packet src: %d len: %d received\n",
	      __FUNCTION__,
	      responsePacket->header.packetLocation,
	      responsePacket->header.packetLength);
      return -1;
    }
  } else {
    perror("recv");
    return -1;
  }
  return 0;
}

int atriSetThresholds(unsigned int fd,
		      AtriDaughterStack_t stack,
		      unsigned short *thresholds,
		      AtriControlPacket_t *controlPacket,
		      AtriControlPacket_t *responsePacket) {

  unsigned char i2cDacWrite[12];

  // DAC command byte
  i2cDacWrite[0] = tdaThresh0 | dacWriteOnly;
  i2cDacWrite[1] = (thresholds[0] & 0xFF00)>>8;
  i2cDacWrite[2] = (thresholds[0] & 0x00FF);
  i2cDacWrite[3] = tdaThresh1 | dacWriteOnly;
  i2cDacWrite[4] = (thresholds[1] & 0xFF00)>>8;
  i2cDacWrite[5] = (thresholds[1] & 0x00FF);
  i2cDacWrite[6] = tdaThresh2 | dacWriteOnly;
  i2cDacWrite[7] = (thresholds[2] & 0xFF00)>>8;
  i2cDacWrite[8] = (thresholds[2] & 0x00FF);
  i2cDacWrite[9] = tdaThresh3 | dacWriteUpdateAll;
  i2cDacWrite[10] = (thresholds[3] & 0xFF00)>>8;
  i2cDacWrite[11] = (thresholds[3] & 0x00FF);
  if (atriI2CWrite(fd, stack, tdaDacAddress, 12, i2cDacWrite,
		   controlPacket, responsePacket) < 0) {
    fprintf(stderr, "%s: error writing to TDA thresholds\n",
	    __FUNCTION__);
    return -1;
  }
  return 0;
}			

int main(int argc, char **argv)
{  
   int offset;
   printf ("argc = %d\n ", argc);
   if (argc != 2 ) {offset = 100;}
   else {offset  = strtoul(argv[1], 0, 0);}
  printf ("offset= %d \n",offset);
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  unsigned char stack_int;
  AtriDaughterStack_t stacki;
  //int stack;
  int i,j;
  int numBytes=0;
  // Prepare 



  int fAtriControlSockFd=openAtriControlSocket();

//  for( j=low_threshold ; j<=high_threshold ; j=j+step_size ) {
     
// Calculated on 25-Jan-2013   ARA 3 :
//   printf ("Masking %d \n",setTriggerL1Mask(fAtriControlSockFd, 8+32)); // Masking 

// For ARA2
/*   unsigned short thresholdsStack0[4]={29950*offset/100.,33100*offset/100.,31050*offset/100.,22500.*offset/100.};
   unsigned short thresholdsStack1[4]={34100*offset/100.,32950*offset/100.,33500*offset/100.,32950*offset/100.};
   unsigned short thresholdsStack2[4]={28150*offset/100.,29200*offset/100.,31750*offset/100.,31850*offset/100.};
   unsigned short thresholdsStack3[4]={28500*offset/100.,28500*offset/100.,34500*offset/100.,31000*offset/100.}; // For ARA1
*/
   unsigned short thresholdsStack0[4]={28000,28000,28000,28000};
   unsigned short thresholdsStack1[4]={28000,28000,28000,28000};
   unsigned short thresholdsStack2[4]={28000,28000,28000,28000};
   unsigned short thresholdsStack3[4]={28000,28000,28000,28000};


// For ARA3
/*   unsigned short thresholdsStack0[4]={24850*offset/100.,30425*offset/100.,30525*offset/100.,32000.*offset/100.};
   unsigned short thresholdsStack1[4]={29350*offset/100.,3290*offset/100.,32925*offset/100.,32450*offset/100.};
//   unsigned short thresholdsStack1[4]={29350*offset/100.,32900*offset/100.,32925*offset/100.,32450*offset/100.};
   unsigned short thresholdsStack2[4]={29825*offset/100.,29300*offset/100.,30700*offset/100.,29975*offset/100.};
   unsigned short thresholdsStack3[4]={30675*offset/100.,32320*offset/100.,33550*offset/100.,32120*offset/100.}; 
*/
     int stack;
// For ARA1
/*   unsigned short thresholdsStack0[4]={32355*offset/100,28464*offset/100,28913*offset/100,32570*offset/100};
   unsigned short thresholdsStack1[4]={28037*offset/100,20452*offset/100,27867*offset/100,28109*offset/100};
   unsigned short thresholdsStack2[4]={30571*offset/100,31028*offset/100,32964*offset/100,31100*offset/100};
   unsigned short thresholdsStack3[4]={29863*offset/100,31100*offset/100,31692*offset/100,30714*offset/100}; // For ARA1
*/

     /* atriSetThresholds( fAtriControlSockFd , 0 , thresholdsStack0 , &controlPacket , &responsePacket );   */
     /* atriSetThresholds( fAtriControlSockFd , 1 , thresholdsStack1 , &controlPacket , &responsePacket );   */
     /* atriSetThresholds( fAtriControlSockFd , 2 , thresholdsStack2 , &controlPacket , &responsePacket );   */
     /* atriSetThresholds( fAtriControlSockFd , 3 , thresholdsStack3 , &controlPacket , &responsePacket );   */

     /* printf ("Stack 0: "); for (i=0;i<4;i++) { printf("ch%d: %f %5d , ",i,thresholdsStack0[i],dacToVolts(thresholdsStack0[i]));} */
     /* printf ("\nStack 1: "); for (i=0;i<4;i++) { printf("ch%d: %f %5d , ",i,thresholdsStack1[i],dacToVolts(thresholdsStack1[i]));} */
     /* printf ("\nStack 2: "); for (i=0;i<4;i++) { printf("ch%d: %f %5d , ",i,thresholdsStack2[i],dacToVolts(thresholdsStack2[i]));} */
     /* printf ("\nStack 3: "); for (i=0;i<4;i++) { printf("ch%d: %f %5d , ",i,thresholdsStack3[i],dacToVolts(thresholdsStack3[i]));} */
     /* printf ("\n======================================\n"); */
 //        thresholds[i] = low_threshold;

//     printf("%f %5d ",thresholds[0],dacToVolts(thresholds[0]));

     // - set the thresholds
  sleep (2); //moshe
  uint8_t data[4]={0,0,0,0};
// fMainThreadAtriSockFd=openConnectionToAtriControlSocket();

  uint16_t l4Scaler[NUM_L4_SCALERS]; 
  uint16_t l1Scaler[16];
  uint16_t l3Scaler[4];
  uint16_t t1Scaler[16];
  uint8_t loop=0;
   while (1) {
    printf("%lu ",time(NULL));  

  atriWishboneRead(fAtriControlSockFd,ATRI_WISH_SCAL_L4_START, 2*NUM_L4_SCALERS,data);
  for(loop=0;loop<NUM_L4_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(l4Scaler[loop]));
  }

  atriWishboneRead(fAtriControlSockFd,ATRI_WISH_SCAL_L1_START, 2*NUM_L1_SCALERS,data);
  for(loop=0;loop<16;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(l1Scaler[loop]));
    printf ("%d  ",l1Scaler[loop]);
  }
  atriWishboneRead(fAtriControlSockFd,ATRI_WISH_SCAL_T1_START, 2*NUM_T1_SCALERS,data);
  for(loop=0;loop<NUM_T1_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(t1Scaler[loop]));
  }
  atriWishboneRead(fAtriControlSockFd,ATRI_WISH_SCAL_L3_START, 2*NUM_L3_SCALERS,data);
  loop=0;
  for(loop=0;loop<NUM_L3_SCALERS;loop++){
    copyTwoBytes(&(data[2*loop]),(uint16_t*)&(l3Scaler[loop]));
  }


  

  printf ("InIce: %d  Surface: %d t1Scaler: %d l3Scaler[0]: %d l3Scaler[1]: %d",l4Scaler[0],l4Scaler[1], t1Scaler[0], l3Scaler[0], l3Scaler[1]);
     
printf ("\n");


     fflush(stdout);
     sleep( 2 );

  } //  end of infinite loop;  end of FOR-loop over threshold values

  close(fAtriControlSockFd);
  return 0;  

} // end of main() program

int openAtriControlSocket()
{
  int fAtriControlSockFd;
  int len;
  struct sockaddr_un remote;
  
  if ((fAtriControlSockFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  
  printf("Trying to connect...\n");
  
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, ATRI_CONTROL_SOCKET);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(fAtriControlSockFd, (struct sockaddr *)&remote, len) == -1) {
    perror("connect");
    exit(1);
  }
  
  printf("Connected.\n");
  return fAtriControlSockFd;

}
