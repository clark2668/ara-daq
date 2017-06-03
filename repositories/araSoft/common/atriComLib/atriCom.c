/* 
   ATRI Communication Library

   The library functions for sending and receiving data through the vendor commands

   rjn@hep.ucl.ac.uk, August 2011
*/

#define ATRI_COM
#include "atriCom.h"
#include "araSoft.h"
#include "math.h"


//% Mapping to help print out name of a daughter based on bit number
const char *atriDaughterTypeStrings[4] = { "DDA", "TDA", "STDA", "DRSV10" };
//% Mapping to help print out daughter stack based on bit number
const char *atriDaughterStackStrings[4] = { "D1", "D2", "D3" , "D4" };

//% Address of the hot-swap controller for each daughter based on bit number
const unsigned char atriHotSwapAddressMap[4] = {0xE4, 0xFC, 0x00, 0x00};
//% Address of the temperature sensor for each daughter based on bit number
const unsigned char atriTempAddressMap[4] = {0x90, 0x92, 0x00, 0x00};
//% Address of the EEPROM for each daughter board based on bit number
const unsigned char atriEepromAddressMap[4] = {0xA0, 0xA8, 0x00, 0x00};

//Stuff for socket
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>


//int fAtriControlSockFd=0;

//AtriControlPacket_t lastControlPacket;
//AtriControlPacket_t lastResponsePacket;

int openConnectionToAtriControlSocket()
{
  int len;
  struct sockaddr_un remote;

  int fAtriControlSockFd=0;
  
  if ((fAtriControlSockFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- socket -- %s\n",__FUNCTION__,strerror(errno));    
  }
  
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s -- Trying to connect...\n",__FUNCTION__);
  
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, ATRI_CONTROL_SOCKET);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(fAtriControlSockFd, (struct sockaddr *)&remote, len) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- connect --%s\n",__FUNCTION__,strerror(errno));
    return -1;
  }
  
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s -- Connected.\n",__FUNCTION__);

  struct timeval tv;
  tv.tv_sec = 10;
  tv.tv_usec=0;    
  if(setsockopt(fAtriControlSockFd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval)))
  {
    ARA_LOG_MESSAGE(LOG_ERR,"Error setting timeout --%s\n",strerror(errno));
    return 0;
  }

  return fAtriControlSockFd;
}

int closeConnectionToAtriControlSocket(int fAtriControlSockFd)
{
  //Need to add some error checking
  if(fAtriControlSockFd)
    close(fAtriControlSockFd);  
  return 0;
}


int sendControlPacketToAtri(int fAtriControlSockFd,AtriControlPacket_t *pktPtr)
{
  if(fAtriControlSockFd==0) {
    ARA_LOG_MESSAGE(LOG_ERR,"ATRI control socket not open\n");
    return -1;
  }
  

  ARA_LOG_MESSAGE(LOG_DEBUG,"Sending control Packet\n");
  //ARA_LOG_MESSAGE(LOG_DEBUG,"Location %d, Length %d\n",pktPtr->header.packetLocation,pktPtr->header.packetLength);
  //  memcpy((char*)&lastControlPacket,(char*)pktPtr,sizeof(AtriControlPacket_t));

  if (send(fAtriControlSockFd, (char*)pktPtr, sizeof(AtriControlPacket_t), 0) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s (fAtriControlSockFd=%d) -- send --%s\n",__FUNCTION__,fAtriControlSockFd,strerror(errno));
    
    return -1;
  }
  return 0;
}

int readResponsePacketFromAtri(int fAtriControlSockFd,AtriControlPacket_t *pktPtr)
{
  if(fAtriControlSockFd==0) {
    ARA_LOG_MESSAGE(LOG_ERR,"ATRI control socket not open\n");
    return -1;
  }

  int numBytes=recv(fAtriControlSockFd, (char*)pktPtr,sizeof(AtriControlPacket_t), 0);
  //  if(numBytes<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) {
  //ARA_LOG_MESSAGE(LOG_ERR,"%s timed out reading from fAtriControlSockFd=%d, try again\n",__FUNCTION__,fAtriControlSockFd);
  //    numBytes=recv(fAtriControlSockFd, (char*)pktPtr,sizeof(AtriControlPacket_t), 0);
  //  }   
  ARA_LOG_MESSAGE(LOG_DEBUG, "%s Received %d bytes with readResponsePacketFromAtri.\n", __FUNCTION__, numBytes);

  if(numBytes<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s (fAtriControlSockFd=%d) -- recv -- %s\n",__FUNCTION__,fAtriControlSockFd,strerror(errno));
    return -1;
  }
  else if(numBytes==0) {
    return -2;
  }
  else {
    if(numBytes==sizeof(AtriControlPacket_t)) {
      //      memcpy((char*)&lastResponsePacket,(char*)pktPtr,sizeof(AtriControlPacket_t));
      return 0;
    }
  }
  return -3;
}



int writeToAtriI2CRegister(int fAtriControlSockFd,AtriDaughterStack_t stack,uint8_t i2cAddress, uint8_t reg, uint8_t value)
{
  uint8_t data[2]={reg,value};
  return writeToAtriI2C(fAtriControlSockFd,stack,i2cAddress,2,data);
}


// 0 indicates OK
// -1 indicates NACK
// -2 is all other errors
int writeToAtriI2C(int fAtriControlSockFd,AtriDaughterStack_t stack, uint8_t i2cAddress, uint8_t length, uint8_t *data)
{
  int retVal=0;
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  // Clear the R/~W bit just to be sure. Trying to write to
  // a read i2cAddress results in a timeout.
  i2cAddress &= ~(0x1);

  controlPacket.header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket.header.packetLocation=ATRI_LOC_I2C_DB1 + stack;
  controlPacket.header.packetLength=2+length;
  controlPacket.data[0] = I2C_DIRECT;
  controlPacket.data[1] = i2cAddress;
  memcpy(controlPacket.data+2, data, length);
  controlPacket.data[controlPacket.header.packetLength]=ATRI_CONTROL_FRAME_END;
  if(sendControlPacketToAtri(fAtriControlSockFd,&controlPacket)==0) {   
    if((retVal=readResponsePacketFromAtri(fAtriControlSockFd,&responsePacket))==0) {
      if(responsePacket.header.packetLength==3) {
	//All's well
	return 0;
      } else if (responsePacket.header.packetLength==1 &&
		 responsePacket.data[0] == 0xFF) {
	return -1;
      } else {
	ARA_LOG_MESSAGE(LOG_ERR,"%s : unknown packet src: %d len: %d received\n",
			__FUNCTION__,
			responsePacket.header.packetLocation,
			responsePacket.header.packetLength);
	return -2;
      }
    }
    else {
      ARA_LOG_MESSAGE(LOG_ERR,"%s : readResponsePacketFromAtri returned %d\n",__FUNCTION__,retVal);
    }
  }
  return -2;  
}


int readFromAtriI2C(int fAtriControlSockFd,AtriDaughterStack_t stack, uint8_t i2cAddress, uint8_t length, uint8_t *data)
{
  int retVal=0;
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;

  // Set the R/~W bit.
  i2cAddress |= 0x1;

  controlPacket.header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket.header.packetLocation=ATRI_LOC_I2C_DB1 + stack;
  controlPacket.header.packetLength=3;
  controlPacket.data[0] = I2C_DIRECT;
  controlPacket.data[1] = i2cAddress;
  controlPacket.data[2] = length;
  controlPacket.data[controlPacket.header.packetLength]=ATRI_CONTROL_FRAME_END;
  

  if(sendControlPacketToAtri(fAtriControlSockFd,&controlPacket)==0) {   
    if((retVal=readResponsePacketFromAtri(fAtriControlSockFd,&responsePacket))==0) {

      if(responsePacket.header.packetLength == length+2) {
	memcpy(data,responsePacket.data+2,length);
	return 0;
      } else if (responsePacket.header.packetLength == 1 &&
		 responsePacket.data[0] == 0xFF) {
	return -1;
      } 
      else {	
	ARA_LOG_MESSAGE(LOG_ERR, "%s : unknown packet src: %d len: %d received\n",
			__FUNCTION__,
			responsePacket.header.packetLocation,
			responsePacket.header.packetLength);
	return -2;
      }
    }   
    else {
      ARA_LOG_MESSAGE(LOG_ERR,"%s : readResponsePacketFromAtri returned %d\n",__FUNCTION__,retVal);
    }
  }
  return -2; 
}


int atriWishboneRead(int fAtriControlSockFd,uint16_t wishboneAddress,uint8_t length, uint8_t *data)
{
  int i,k;
  int retVal=0;
  uint8_t part;
  uint8_t maxPayload = 58;
  uint16_t newAddr;
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  controlPacket.header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket.header.packetLocation=ATRI_LOC_WISHBONE;
  controlPacket.header.packetLength=4;

  if(length<58){
    controlPacket.data[0] = WB_READ;
    controlPacket.data[1] = (wishboneAddress & 0xFF00)>>8;
    controlPacket.data[2] = wishboneAddress & 0xFF;
    controlPacket.data[3] = length;
    controlPacket.data[controlPacket.header.packetLength]=ATRI_CONTROL_FRAME_END;
  
    ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Starting Wishbone read.\n", __FUNCTION__);

  //Now send the message and wait for reply
    if(sendControlPacketToAtri(fAtriControlSockFd,&controlPacket)==0) {   
      ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Control read request to ATRI completed as: header: %2.2x %2.2x %2.2x %2.2x, data: %2.2x %2.2x %2.2x %2.2x %2.2x . \n", __FUNCTION__, controlPacket.header.frameStart, controlPacket.header.packetLocation, controlPacket.header.packetNumber, controlPacket.header.packetLength,
    		    controlPacket.data[0], controlPacket.data[1], controlPacket.data[2], controlPacket.data[3], controlPacket.data[4]);
    
      if((retVal=readResponsePacketFromAtri(fAtriControlSockFd,&responsePacket))==0) {
      //  ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Response read succesfully.", __FUNCTION__);       
      //	ARA_LOG_MESSAGE(LOG_DEBUG, "%s : packet number: %d src: %d len: %d received (wanted number: %d src: %d len %d)\n",
      //		__FUNCTION__,
      //		responsePacket.header.packetNumber,
      //		responsePacket.header.packetLocation,
      //		responsePacket.header.packetLength,
      //		controlPacket.header.packetNumber,
      //		controlPacket.header.packetLocation,
      //		length);	
	if(responsePacket.header.packetLength == length) {
	  for (i=0;i<length;i++) {
	    data[i] = responsePacket.data[i];
	  }
	  return 0;
	} else {	
	  ARA_LOG_MESSAGE(LOG_ERR, "%s : unknown packet src: %d len: %d received (wanted src: %d len %d)\n",
			__FUNCTION__,
			responsePacket.header.packetLocation,
			responsePacket.header.packetLength,
			controlPacket.header.packetLocation,
			length);	
	}
      }    
      else {
	ARA_LOG_MESSAGE(LOG_ERR,"%s : readResponsePacketFromAtri returned %d\n",__FUNCTION__,retVal);
      }
    }    
    return -1;
  }
    else{

      part = maxPayload;
      k=0;
      while(part==maxPayload)
        {
          if( length - maxPayload*k > maxPayload ) part = maxPayload;
          else part = length - maxPayload*k;
          newAddr = wishboneAddress+maxPayload*k;
          controlPacket.data[0] = WB_READ;
          controlPacket.data[1] = (newAddr & 0xFF00)>>8;
          controlPacket.data[2] = newAddr & 0xFF;
          controlPacket.data[3] = part;
          controlPacket.data[controlPacket.header.packetLength]=ATRI_CONTROL_FRAME_END;

	  //Now send the message and wait for reply
          if(sendControlPacketToAtri(fAtriControlSockFd,&controlPacket)==0) {
      ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Control read request to ATRI completed as: header: %2.2x %2.2x %2.2x %2.2x, data: %2.2x %2.2x %2.2x %2.2x %2.2x . \n", __FUNCTION__, controlPacket.header.frameStart, controlPacket.header.packetLocation, controlPacket.header.packetNumber, controlPacket.header.packetLength,
    		    controlPacket.data[0], controlPacket.data[1], controlPacket.data[2], controlPacket.data[3], controlPacket.data[4]);
    
            if((retVal=readResponsePacketFromAtri(fAtriControlSockFd,&responsePacket))==0) {
        ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Response read succesfully.", __FUNCTION__);       
      	ARA_LOG_MESSAGE(LOG_DEBUG, "%s : packet number: %d src: %d len: %d received (wanted number: %d src: %d len %d)\n",
      		__FUNCTION__,
      		responsePacket.header.packetNumber,
      		responsePacket.header.packetLocation,
      		responsePacket.header.packetLength,
      		controlPacket.header.packetNumber,
      		controlPacket.header.packetLocation,
      		length);	
              if(responsePacket.header.packetLength == part) {
                for (i=0;i<part;i++) {
                  data[i+maxPayload*k] = responsePacket.data[i];
                }
                k++;
                if(part==maxPayload) continue;
                else return 0;
              } else {
                ARA_LOG_MESSAGE(LOG_ERR, "%s : unknown packet src: %d len: %d received (wanted src: %d len %d)\n",
                                __FUNCTION__,
                                responsePacket.header.packetLocation,
                                responsePacket.header.packetLength,
                                controlPacket.header.packetLocation,
                                length);
              }
            }
            else {
              ARA_LOG_MESSAGE(LOG_ERR,"%s : readResponsePacketFromAtri returned %d\n",__FUNCTION__,retVal);
            }
          }
          return -1;
        }




    }
}


int atriWishboneWrite(int fAtriControlSockFd,uint16_t wishboneAddress,uint8_t length, uint8_t *data)
{
  int retVal=0;
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  controlPacket.header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket.header.packetLocation=ATRI_LOC_WISHBONE;
  controlPacket.header.packetLength=3+length;
  controlPacket.data[0] = WB_WRITE;
  controlPacket.data[1] = (wishboneAddress & 0xFF00)>>8;
  controlPacket.data[2] = wishboneAddress & 0xFF;
  memcpy(controlPacket.data+3, data, sizeof(unsigned char)*length);
  controlPacket.data[controlPacket.header.packetLength]=ATRI_CONTROL_FRAME_END;
  //Now send the message and wait for reply
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s : Start writing to Wishbone.\n", __FUNCTION__);
  if(sendControlPacketToAtri(fAtriControlSockFd,&controlPacket)==0) {   
    ARA_LOG_MESSAGE(LOG_DEBUG,"%s : Control transfer completed.\n ",__FUNCTION__);
    if((retVal=readResponsePacketFromAtri(fAtriControlSockFd,&responsePacket))==0) {    
      ARA_LOG_MESSAGE(LOG_DEBUG,"%s : Response read succesfully.\n",__FUNCTION__);
      if(responsePacket.header.packetLength==1)
	return 0;
      else {

	ARA_LOG_MESSAGE(LOG_ERR, "%s : unknown packet src: %d len: %d received\n",
			__FUNCTION__,
			responsePacket.header.packetLocation,
			responsePacket.header.packetLength);
	return -1;	
      }
    }
    else {
      ARA_LOG_MESSAGE(LOG_ERR,"%s : readResponsePacketFromAtri returned %d\n",__FUNCTION__,retVal);
    }
  }
  return -1;
}

// Determine the digitizer type (IRS1/2, IRS3B) for a given stack.
// IRS3B mode is determined by looking for the pedestal DAC.
int atriGetDigitizerType(int fAtriControlSockFd, AtriDaughterStack_t stack,
			 AtriDigitizerType_t *type) {
  uint8_t data = 0;
  int ret;
  if (type == NULL) return -1;
  // 0 byte write. Just look for an ack.
  if((ret=writeToAtriI2C(fAtriControlSockFd,stack, ddaPedDacAddress, 1, &data)) < 0) {  
    if (ret == -1) {
      // NACK - this is OK, it just means we have an IRS2.
      *type = digitizerType_IRS2;
      return 0;
    } else {
      ARA_LOG_MESSAGE(LOG_ERR, "%s : error probing for IRS3 pedestal DAC\n",
		      __FUNCTION__);
      return ret;
    }
  }
  // Probe returned an ACK. This is an IRS3.
  *type = digitizerType_IRS3;
  return 0;
}

int atriDaughterPowerOn(int fAtriControlSockFd,AtriDaughterStack_t stack, AtriDaughterType_t type, AtriDaughterStatus_t *status)
{
  
  unsigned char curControl;
  unsigned char toWrite;

  if(atriWishboneRead(fAtriControlSockFd,0x0010+stack,1,&curControl)<0) {    
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error reading current control\n",
	    __FUNCTION__);
    return -1;
  }
  
  curControl |= (1 << (unsigned char) type);
  if (atriWishboneWrite(fAtriControlSockFd,0x0010 + stack, 1, &curControl) < 0) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing new status\n",
	    __FUNCTION__);
    return -1;
  }



  // Sleep 50 milliseconds to allow the hotswap cycle to complete.
  usleep(50000);

  // Does this daughter have a swap controller?
  if (atriHotSwapAddressMap[type] == 0x00) {
    // no... just return
    return 0;
  }
  
  // Now write 0x50 to hotswap controller to read out the status byte.
  toWrite = 0x50;
  if (writeToAtriI2C(fAtriControlSockFd,stack, atriHotSwapAddressMap[type], 1, &toWrite) < 0) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error switching to status read on swap controller\n",
	    __FUNCTION__);
    return -1;
  }
  // and read the status byte
  if (readFromAtriI2C(fAtriControlSockFd,stack, atriHotSwapAddressMap[type], 1, &(status->hotSwapStatus)) < 0) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error reading status on swap controller\n",
	    __FUNCTION__);
    return -1;
  }
  // and restore to a normal read
  toWrite = 0x10;
  if (writeToAtriI2C(fAtriControlSockFd,stack, atriHotSwapAddressMap[type], 1, &toWrite) < 0) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error switching to normal read on swap controller\n",
	    __FUNCTION__);
    return -1;
  }
  if (status->hotSwapStatus & bmHS_OC) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: %s %s power on overcurrent (%2.2X)\n",
	    __FUNCTION__,
	    atriDaughterStackStrings[stack], atriDaughterTypeStrings[type],
	    status->hotSwapStatus);
    return -1;
  } else {

    // DDA TYPE DETERMINATION

    if (type == DDA) {
      AtriDigitizerType_t dig;
      int ret;
      unsigned char curMode;
      ret = atriGetDigitizerType(fAtriControlSockFd, stack, &dig);
      if (ret < 0) {
	ARA_LOG_MESSAGE(LOG_ERR, "%s: error %d determining %s digitizer type\n",
			__FUNCTION__,
			ret,
			atriDaughterStackStrings[stack]);
	return ret;
      }
      if((ret =atriWishboneRead(fAtriControlSockFd,
				ATRI_WISH_IRSEN+1,1,&curMode))<0) {
	ARA_LOG_MESSAGE(LOG_ERR, "%s: error reading IRS mode register\n",
			__FUNCTION__);
	return ret;
      }
      
      if (dig == digitizerType_IRS2) {
	ARA_LOG_MESSAGE(LOG_INFO, "%s: %sDDA has an IRS1/2\n",
			__FUNCTION__,
			atriDaughterStackStrings[stack]);
	curMode &= ~(0x10 << stack);
      }
      else {
	ARA_LOG_MESSAGE(LOG_INFO, "%s: %sDDA has an IRS3/3B\n",
			__FUNCTION__,
			atriDaughterStackStrings[stack]);
	curMode |= (0x10 << stack);
      }
      
      if ((ret = atriWishboneWrite(fAtriControlSockFd,
				   ATRI_WISH_IRSEN+1, 1, &curMode))<0) {
	ARA_LOG_MESSAGE(LOG_ERR, "%s: error setting IRS mode register\n",
			__FUNCTION__);
	return ret;
      }
    }

    // DRIVE ALL OUTPUTS

    if (atriSetDaughterDrive(fAtriControlSockFd, stack, type, 1) < 0) {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: error driving %s%s outputs\n",
		      __FUNCTION__,
		      atriDaughterStackStrings[stack],
		      atriDaughterTypeStrings[type]);
      return -1;
    }
  }	  
  // should read out EEPROM here for identification, but for now we return
  return 0;
}

int atriSetDaughterDrive(int fAtriControlSockFd, AtriDaughterStack_t stack,
			 AtriDaughterType_t type, int drive) {
  
  unsigned char curControl;

  if(atriWishboneRead(fAtriControlSockFd,0x0010+stack,1,&curControl)<0) {    
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error reading current control\n",
	    __FUNCTION__);
    return -1;
  }
  
  if (drive)
    curControl |= (0x10 << (unsigned char) type);
  else
    curControl &= ~(0x10 << (unsigned char) type);
  if (atriWishboneWrite(fAtriControlSockFd,0x0010 + stack, 
			1, &curControl) < 0) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing new status\n",
	    __FUNCTION__);
    return -1;
  }
  return 0;
}


unsigned int atriGetDaughterStatus(int fAtriControlSockFd)
{
  unsigned int dbStatus;
  int retVal=0;
  AtriControlPacket_t controlPacket;
  AtriControlPacket_t responsePacket;
  // Prep a daughterboard status request packet.
  controlPacket.header.frameStart=ATRI_CONTROL_FRAME_START;
  controlPacket.header.packetLocation=ATRI_LOC_DB_STATUS;
  controlPacket.header.packetLength=1;
  controlPacket.data[0] = 0;
  controlPacket.data[controlPacket.header.packetLength]=ATRI_CONTROL_FRAME_END;
  // Ship it off.
  if(sendControlPacketToAtri(fAtriControlSockFd,&controlPacket)==0) {   
    if((retVal=readResponsePacketFromAtri(fAtriControlSockFd,&responsePacket))==0) {             
      if(responsePacket.header.packetLength == 4) {
	dbStatus = responsePacket.data[0];
	dbStatus |= (responsePacket.data[1] << 8);
	dbStatus |= (responsePacket.data[2] << 16);
	dbStatus |= (responsePacket.data[3] << 24);
	return dbStatus;
      } else {
	ARA_LOG_MESSAGE(LOG_ERR, "%s : unknown packet src: %d len: %d received\n",
			__FUNCTION__,
			responsePacket.header.packetLocation,
			responsePacket.header.packetLength);
      }
    }
    else {
      ARA_LOG_MESSAGE(LOG_ERR,"%s : readResponsePacketFromAtri returned %d\n",__FUNCTION__,retVal);
    }
  }
  return -1;      
}


int setIRSClockDACValues(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vdlyDac, uint16_t vadjDac)
{
  int ret;

  uint8_t data[6];
  data[0]=0x0;
  data[1]=((vdlyDac & 0xFF00) >>8);
  data[2]=(vdlyDac & 0xFF);
  data[3]=0x11;
  data[4]=((vadjDac & 0xFF00) >>8);
  data[5]=(vadjDac & 0xFF);

  if((ret=writeToAtriI2C(fAtriControlSockFd,stack, ddaDacAddress, 6, data)) < 0) {
    if (ret == -1) {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: write to %sDDA IRS control DAC failed (NACK)\n",
		      __FUNCTION__, atriDaughterStackStrings[stack]);
      return -1;
    }
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing to %sDDA IRS control DAC\n",
		    __FUNCTION__, atriDaughterStackStrings[stack]);
    return -1;
  }
  return 0;  
}


int setIRSVadjDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vadjDac)
{
  int ret;

  uint8_t data[6];
  data[0]=ddaVadj | dacWriteUpdateOne;
  data[1]=((vadjDac & 0xFF00) >>8);
  data[2]=(vadjDac & 0xFF);
  
  if((ret=writeToAtriI2C(fAtriControlSockFd,stack, ddaDacAddress, 3, data)) < 0) {
    if (ret == -1) {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: write to %sDDA Vadj DAC failed (NACK)\n",
		      __FUNCTION__, atriDaughterStackStrings[stack]);
      return -1;
    }
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing to %sDDA Vadj DAC\n",
		    __FUNCTION__, atriDaughterStackStrings[stack]);
    return -1;
  }
  return 0;  
}


int setIRSVdlyDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vdlyDac)
{
  int ret;

  uint8_t data[6];
  data[0]=ddaVdly | dacWriteUpdateOne;
  data[1]=((vdlyDac & 0xFF00) >>8);
  data[2]=(vdlyDac & 0xFF);
  
  if((ret=writeToAtriI2C(fAtriControlSockFd,stack, ddaDacAddress, 3, data)) < 0) {
    if (ret == -1) {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: write to %sDDA Vdly DAC failed (NACK)\n",
		      __FUNCTION__, atriDaughterStackStrings[stack]);
      return -1;
    }
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing to %sDDA Vdly DAC\n",
		    __FUNCTION__, atriDaughterStackStrings[stack]);
    return -1;
  }
  return 0;  

}

int setIRSVbiasDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vbiasDac)
{
  int ret;

  uint8_t data[6];
  data[0]=ddaVbias | dacWriteUpdateOne;
  data[1]=((vbiasDac & 0xFF00) >>8);
  data[2]=(vbiasDac & 0xFF);

  if((ret=writeToAtriI2C(fAtriControlSockFd,stack, ddaDacAddress, 3, data)) < 0) {
    if (ret == -1) {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: write to %sDDA Vbias DAC failed (NACK)\n",
		      __FUNCTION__, atriDaughterStackStrings[stack]);
      return -1;
    }
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing to %sDDA Vbias DAC\n",
		    __FUNCTION__, atriDaughterStackStrings[stack]);
    return -1;
  }
  return 0;  

}

int setIRSIselDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t iselDac)
{
  int ret;
  uint8_t data[6];
  data[0]=ddaIsel | dacWriteUpdateOne;
  data[1]=((iselDac & 0xFF00) >>8);
  data[2]=(iselDac & 0xFF);
  
  if((ret=writeToAtriI2C(fAtriControlSockFd,stack, ddaDacAddress, 3, data)) < 0) {
    if (ret == -1) {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: write to %sDDA Isel DAC failed (NACK)\n",
		      __FUNCTION__, atriDaughterStackStrings[stack]);
      return -1;
    }
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing to %sDDA Isel DAC\n",
		    __FUNCTION__, atriDaughterStackStrings[stack]);
    return -1;
  }
  return 0;  

}




/* void getLastControlPacket(AtriControlPacket_t *pktPtr) */
/* { */
/*   memcpy(pktPtr,&lastControlPacket,sizeof(AtriControlPacket_t)); */
/* } */
/* void getLastResponsePacket(AtriControlPacket_t *rspPtr) */
/* { */
/*   memcpy(rspPtr,&lastResponsePacket,sizeof(AtriControlPacket_t)); */
/* } */


int setAtriThresholds(int fAtriControlSockFd,AtriDaughterStack_t stack,  uint16_t *thresholds)
{
  unsigned char i2cDacWrite[12];
  int ret;

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
  if((ret=writeToAtriI2C(fAtriControlSockFd,stack, tdaDacAddress, 12, i2cDacWrite)) < 0) {
    if (ret == -1) {
      ARA_LOG_MESSAGE(LOG_ERR, "%s : write to %sTDA threshold DAC failed (NACK)\n", __FUNCTION__, atriDaughterStackStrings[stack]);
      return -1;
    } else {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: error writing to %sTDA threshold DAC\n",
		      __FUNCTION__, atriDaughterStackStrings[stack]);
      return -1;
    }
  }
  return 0;  

}



int setSurfaceAtriThresholds(int fAtriControlSockFd,AtriDaughterStack_t stack,  uint16_t *thresholds)
{
  unsigned char i2cDacWrite[12];
  
  // DAC command byte
  i2cDacWrite[0] = surfaceThresh0 | dacWriteOnly;
  i2cDacWrite[1] = (thresholds[0] & 0xFF00)>>8;
  i2cDacWrite[2] = (thresholds[0] & 0x00FF);
  i2cDacWrite[3] = surfaceThresh1 | dacWriteOnly;
  i2cDacWrite[4] = (thresholds[1] & 0xFF00)>>8;
  i2cDacWrite[5] = (thresholds[1] & 0x00FF);
  i2cDacWrite[6] = surfaceThresh2 | dacWriteOnly;
  i2cDacWrite[7] = (thresholds[2] & 0xFF00)>>8;
  i2cDacWrite[8] = (thresholds[2] & 0x00FF);
  i2cDacWrite[9] = surfaceThresh3 | dacWriteUpdateAll;
  i2cDacWrite[10] = (thresholds[3] & 0xFF00)>>8;
  i2cDacWrite[11] = (thresholds[3] & 0x00FF);
 if(writeToAtriI2C(fAtriControlSockFd,stack, surfaceDacAddress, 12, i2cDacWrite) < 0) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error setting surface thresholds value\n",
		    __FUNCTION__);
    return -1;
  }
  return 0;  

}

int resetAtriTriggerAndDigitizer(int fAtriControlSockFd) {
  uint8_t val, trigctlSave;
  uint16_t addr;

  addr = ATRI_WISH_TRIGCTL;
  if (atriWishboneRead(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  // Mask triggers.
  trigctlSave = val;
  val |= trigctlT1Mask;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  addr = ATRI_WISH_IRSEN;
  // Disable digitizer and place in reset.
  val = irsctlResetMask;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  // Reset the triggering subsystem.
  addr = ATRI_WISH_TRIGCTL;
  val = trigctlResetMask | (trigctlSave & trigctlDbMask);
  // Trigger reset is just a flag.
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  // Check to see if the digitizer subsystem is done resetting.
  addr = ATRI_WISH_IRSEN;
  if (atriWishboneRead(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  if (!(val & irsctlResetAckMask)) {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: error resetting digitizer: RESET_ACK did not go high (%2.2x)\n", __FUNCTION__, val);
    return -1;
  }
  val = 0;
  // Pull the digitizing subsystem out of reset.
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;

  // And we're done. Note that the digitizer is not enabled, nor
  // triggering!
  return 0;
}

int setAtriL1HighDb(int fAtriControlSockFd, AtriDaughterStack_t stack) {
  uint16_t addr;
  uint8_t val;
  addr = ATRI_WISH_TRIGCTL;
  if (atriWishboneRead(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  val = (val & ~trigctlDbMask) | stack;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  return 0;
}

int setTriggerL1Mask(int fAtriControlSockFd, int mask) {
  uint16_t addr;
  uint8_t val[3]; // only 20 bits are valid
  
  val[0] = mask & 0xFF;
  val[1] = (mask>>8) & 0xFF;
  val[2] = (mask>>16) & 0xFF;
  addr = ATRI_WISH_L1MASK;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 3, val)<0) return -1;
  return 0;
}

int setTriggerL2Mask(int fAtriControlSockFd, int mask) {
  uint16_t addr;
  uint8_t val[2]; // only 16 bits are valid
  
  val[0] = mask & 0xFF;
  val[1] = (mask>>8) & 0xFF;
  addr = ATRI_WISH_L2MASK;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 2, val)<0) return -1;
  return 0;
}

int setTriggerL3Mask(int fAtriControlSockFd, int mask) {
  uint16_t addr;
  uint8_t val; // only 8 bits are valid

  val = mask & 0xFF;
  addr = ATRI_WISH_L3MASK;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  return 0;
}

int setTriggerL4Mask(int fAtriControlSockFd, int mask) {
  uint16_t addr;
  uint8_t val;
  
  val = mask & 0xFF; // only 8 bits are valid (only 4 used0
  addr = ATRI_WISH_L4MASK;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  return 0;
}

int setTriggerT1Mask(int fAtriControlSockFd, int mask) {
  uint16_t addr;
  uint8_t val;
  ARA_LOG_MESSAGE(LOG_DEBUG, "%s : Starting T1 enable Wishbone read an write.\n ", __FUNCTION__);
  addr = ATRI_WISH_TRIGCTL;
  int retVal = atriWishboneRead(fAtriControlSockFd, addr, 1, &val);
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s : atriWishboneRead returned %d.\n", __FUNCTION__, retVal);
    if(retVal<0)return -1;
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s : Wishbone read worked!\n",__FUNCTION__);
  if (!mask)
    val = (val & ~trigctlT1Mask);
  else
    val = val | trigctlT1Mask;
  retVal = atriWishboneWrite(fAtriControlSockFd, addr, 1, &val);
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s : atriWishboneWrite returned %d.\n", __FUNCTION__, retVal);
  if(retVal<0)return -1;
  return 0;
}

int setIRSEnable(int fAtriControlSockFd, int enable) {
  uint16_t addr;
  uint8_t val;
  
  if (enable) {
    val = irsctlEnableMask;
  } else {
    val = 0;
  }
  printf("Writing %2.2x to ATRI_WISH_IRSEN\n", val);
  addr = ATRI_WISH_IRSEN;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  return 0;
}

int setIRSMode(int fAtriControlSockFd, AtriDaughterStack_t stack,
	       IrsMode_t mode) {
  uint8_t val;
  uint16_t addr;
  addr = ATRI_WISH_IRSEN+1;
  if (atriWishboneRead(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  val = (val & (~(0x10 << stack)));
  val |= (mode << (stack+4));
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  return 0;
}

int setPedestalMode(int fAtriControlSockFd, PedMode_t mode) {
  uint8_t val;
  uint16_t addr;
  addr = ATRI_WISH_IRSPEDEN;
  val = (mode & 0x7); // only low 3 bits are settable
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val) < 0) return -1;
  return 0;
}

int atriSetTriggerNumBlocks(int fAtriControlSockFd, TriggerL4_t ttype,
			    unsigned char nblocks) {
  uint8_t val;
  uint16_t addr;
  addr = ATRI_WISH_RF0BLOCKS + ttype*2;
  val = nblocks;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val) < 0) return -1;
  return 0;
}

int atriSetNumPretrigBlocks(int fAtriControlSockFd, TriggerL4_t ttype,
			    unsigned char nblocks) {
  uint8_t val;
  uint16_t addr;
  addr = ATRI_WISH_RF0PRETRIG + ttype*2;
  val = nblocks;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val) < 0) return -1;
  return 0;
}

unsigned int atriConvertWilkCounterToNs(unsigned short rawValue) {
  // Wilkinson counter now counts number of 20.83 ns cycles to see 64 edges.
  // So the number of nanoseconds is (counter*(20.83)/16).
  return (unsigned int) rint((20.83)*((float) rawValue)/(64.));
}

int atriSetReadoutDelay(int fAtriControlSockFd, uint8_t delay){
  uint16_t addr;
  uint8_t val = delay;
  // delay is converted to clock ticks in firmware
  // no clock ticks = 4*delay + 8
  // 100MHz clock used
  // time in firmware = (4*delay+8)*10 ns
  // default delay setting is 18 -> 800ns

  addr = ATRI_WISH_RDDELAY;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  return 0;
}

int atriGetEventReadoutStatistics(int fAtriControlSockFd,
				  AtriEventReadoutStatistics_t *stat) {
  int retval;
  if (stat == NULL) return 0;
  retval = atriWishboneRead(fAtriControlSockFd, ATRI_WISH_READERR,
			    16,
			    stat);
  return retval;
}

int atriSetTriggerDelays(int fAtriControlSockFd, uint16_t *triggerDelays){
  uint8_t val;
  uint16_t addr;
  int i;
  //Mask off the L1 Scalers
  for(i=0;i<TDA_PER_ATRI;i++){
    val=0xff;
    addr=ATRI_WISH_L1MASK+i;
    if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  }
  
  for(i=0;i<THRESHOLDS_PER_ATRI;i++){ 
    val=i; 
    addr=ATRI_WISH_TRIG_DELAY_NUM; 
    if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1; 
    val=(uint8_t)triggerDelays[i]; 
    addr=ATRI_WISH_TRIG_DELAY_VALUE; 
    if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1; 
  } 
  //UnMask the L1 Scalers
  for(i=0;i<TDA_PER_ATRI;i++){
    val=0x00;
    addr=ATRI_WISH_L1MASK+i;
    if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  }
  return 0;
}

int atriSetTriggerWindow(int fAtriControlSockFd, uint16_t windowSize){
  uint8_t val;
  uint16_t addr;
  int i;
  //Mask off the L1 Scalers
  for(i=0;i<TDA_PER_ATRI;i++){
    val=0xff;
    addr=ATRI_WISH_L1MASK+i;
    if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  }

  //Now set the window size
  addr= ATRI_WISH_L1WINDOW;
  val=(uint8_t)windowSize;
  if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;

  //UnMask the L1 Scalers
  for(i=0;i<TDA_PER_ATRI;i++){
    val=0x00;
    addr=ATRI_WISH_L1MASK+i;
    if (atriWishboneWrite(fAtriControlSockFd, addr, 1, &val)<0) return -1;
  }
  return 0;
}

