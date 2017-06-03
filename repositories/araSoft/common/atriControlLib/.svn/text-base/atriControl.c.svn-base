/* 

   ATRI Control Lib  various helper functions for dealing with the ATR_CONTROL socket and sending receving messages from the ATRI board over the FX2.

   At the moment the USB interface is handled through libusb-1.0 at some point in the future.
   rjn@hep.ucl.ac.uk, July 2011
*/

#include "araSoft.h"
#include "atriControl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>

//static void stupidCBF(struct libusb_transfer *transfer);
//int stupidCBFDone=0;

static const int SUCCEED = 1;
static const int FAILED  = 0;


//Here be some magic

static void eventEndPointCallback(struct libusb_transfer *transfer);
int fNumAsyncRequests;
int fNumAsyncBuffers;
unsigned char asyncBuffer[NUM_ASYNC_BUFFERS][512];
unsigned char asyncBufferSoftware[NUM_ASYNC_BUFFERS][512];
int asyncBufferSize[NUM_ASYNC_BUFFERS];
int asyncBufferStatus[NUM_ASYNC_BUFFERS];

//Now for the worker functions
void initAtriControlSocket(char *socketPath)
{
  struct sockaddr_un u_addr; // unix domain addr
  int len;
  //Create socket
  if ((fAtriControlSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- socket -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }
  u_addr.sun_family = AF_UNIX;
  strcpy(u_addr.sun_path, socketPath);
  unlink(u_addr.sun_path);
  len = strlen(u_addr.sun_path) + sizeof(u_addr.sun_family);
  //Bind address
  if (bind(fAtriControlSocket, (struct sockaddr *)&u_addr, len) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- bind -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }	

  //Listen and have a queue of up to 20 connections waiting
  if (listen(fAtriControlSocket, 20)) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- listen -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }

  //Now set up the mutexs
  pthread_mutex_init(&atri_socket_list_mutex, NULL);    
  pthread_mutex_init(&atri_packet_list_mutex,NULL);
  pthread_mutex_init(&atri_packet_queue_mutex,NULL);

  pthread_mutex_lock(&atri_socket_list_mutex);
  fNumAtriSockets=0;
  fAtriControlSocketList = NULL;
  pthread_mutex_unlock(&atri_socket_list_mutex);

  pthread_mutex_lock(&atri_packet_list_mutex);
  fPacketList = NULL;
  pthread_mutex_unlock(&atri_packet_list_mutex);

  pthread_mutex_lock(&atri_packet_queue_mutex);
  fAtriPacketNumber=0;
  fAtriPacketFifo=NULL;
  pthread_mutex_unlock(&atri_packet_queue_mutex);

}


int checkForNewAtriControlConnections()
{
  // This is meant to be a simple function that just checks to see if we have an incoming
  // connection.
  struct pollfd fds[1];
  int s_dat = 0; 
  struct sockaddr_un u_inaddr; // incoming
  unsigned int u_len = sizeof(u_inaddr);
  int nfds=1;
  int pollVal;

  fds[0].fd = fAtriControlSocket;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  
  pollVal=poll(fds,nfds,1); //non-blocking, for now at least may change at some point if I put this in a thread
  if(pollVal==-1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fAtriControlSocket);
    return -1;
  }
  if(pollVal) {    
    if (fds[0].revents & POLLIN) {
      // incoming connection
      if ((s_dat = accept(fAtriControlSocket, (struct sockaddr *) &u_inaddr, &u_len)) == -1) {
	ARA_LOG_MESSAGE(LOG_ERR,"%s: accept -- %s\n",__FUNCTION__,strerror(errno));
	close(fAtriControlSocket);
	return -1;
      }
      ARA_LOG_MESSAGE(LOG_DEBUG,"%s: incoming connection\n",__FUNCTION__);
      addToAtriControlSocketList(s_dat);
    }
  }  
  return fNumAtriSockets;
}


int addToAtriPacketList(int socketFd, uint8_t packetNumber)
{
  //  fprintf(stderr,"addToAtriPacketList packetNumber=%u socketFd=%d fPacketList=%d\n",packetNumber,socketFd,(int)fPacketList);
  AtriPacketLinkedList_t *tempList=NULL;
  pthread_mutex_lock(&atri_packet_list_mutex);
  tempList=(AtriPacketLinkedList_t*)malloc(sizeof(AtriPacketLinkedList_t));
  tempList->socketFd=socketFd;
  tempList->atriPacketNumber=packetNumber;
  tempList->next=fPacketList;
  fPacketList=tempList;
  pthread_mutex_unlock(&atri_packet_list_mutex);
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"addToAtriPacketList end packetNumber=%u fPacketList=%d\n",packetNumber,(int)fPacketList);
  return 0;

}

int removeFromAtriPacketList(uint8_t packetNumber)
{
  AtriPacketLinkedList_t *tempPacketList=NULL;
  //  fprintf(stderr,"removeFromAtriPacketList: packetNumber=%u tempPacketList=%d\n",packetNumber,(int)tempPacketList);
  int count=0;
  int socketFd=0;
  pthread_mutex_lock(&atri_packet_list_mutex);
  tempPacketList=fPacketList;;
  //Search through list and remove the item with socketFd
  while(tempPacketList) {
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"Packet List: %d -- %d -- %d\n",count,tempPacketList->atriPacketNumber,packetNumber);
    if(tempPacketList->atriPacketNumber==packetNumber) {
      //Found our item in the list
      socketFd=tempPacketList->socketFd;
      //Only need to reassign fPacketList if we are deleting the first entry
      if(tempPacketList==fPacketList)
	fPacketList=tempPacketList->next;
      free(tempPacketList);
      pthread_mutex_unlock(&atri_packet_list_mutex);
      return socketFd;
      break;
    }
    tempPacketList=tempPacketList->next;
    count++;
  }
  pthread_mutex_unlock(&atri_packet_list_mutex);
  return 0;
}

void sendAtriControlPacketToSocket(AtriControlPacket_t *packetPtr)
{
  int socketFd=removeFromAtriPacketList(packetPtr->header.packetNumber);
  if(socketFd==0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error finding socket to return packet %d\n",packetPtr->header.packetNumber);
    return;
  }
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s sending packet=%d to socketFd=%d\n",__FUNCTION__,packetPtr->header.packetNumber,socketFd);
  if (send(socketFd, (char*)packetPtr, sizeof(AtriControlPacket_t), 0) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: send -- %s\n",__FUNCTION__,strerror(errno));
    exit(1);
  }
 
}


int addToAtriControlSocketList(int socketFd)
{
  AtriSocketLinkedList_t *tempSocketList=NULL;
  pthread_mutex_lock(&atri_socket_list_mutex);
  // Create new item in the list at the head of the list
  tempSocketList=(AtriSocketLinkedList_t*)malloc(sizeof(AtriSocketLinkedList_t));
  tempSocketList->socketFd=socketFd;
  tempSocketList->next=fAtriControlSocketList;
  fAtriControlSocketList=tempSocketList;
  fNumAtriSockets++;
  pthread_mutex_unlock(&atri_socket_list_mutex);
  return fNumAtriSockets;
}


int removeFromAtriControlSocketList(int socketFd)
{
  pthread_mutex_lock(&atri_socket_list_mutex);
  AtriSocketLinkedList_t *lastSocketLink=NULL;
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  int count=0;
  //Search through list and remove the item with socketFd
  while(tempSocketList) {
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"%d -- %d -- %d\n",count,tempSocketList->socketFd,socketFd);
    if(tempSocketList->socketFd==socketFd) {
      //Found our item in the list
      if(lastSocketLink) {
	lastSocketLink->next=tempSocketList->next;
      }
      else {
	fAtriControlSocketList=tempSocketList->next;
      }
      free(tempSocketList);
      fNumAtriSockets--;
      break;
    }
    lastSocketLink=tempSocketList;
    tempSocketList=tempSocketList->next;
    count++;
  }
  pthread_mutex_unlock(&atri_socket_list_mutex);
  return fNumAtriSockets;
}


void closeAtriControl(char *socketPath) 
{
  
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  while(tempSocketList) {
    close(tempSocketList->socketFd);
    tempSocketList=tempSocketList->next;
  }
  close(fAtriControlSocket);
  unlink(socketPath);
}


int serviceOpenAtriControlConnections()
{
  //Okay so this essentially checks the open connections and services them in some way
  //Could be combined with the check for new connections
  struct pollfd fds[MAX_ATRI_CONNECTIONS];

  AtriControlPacket_t controlPacket;
  int nbytes;
  int nfds=0;
  int tempInd=0;
  int pollVal;

  memset(&controlPacket,0,sizeof(AtriControlPacket_t));

  if(fNumAtriSockets==0) return 0;  
  pthread_mutex_lock(&atri_socket_list_mutex);
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  //Loop through the socket list to fill the pollfd array
  while(tempSocketList) {
    fds[nfds].fd = tempSocketList->socketFd;
    fds[nfds].events = POLLIN;
    fds[nfds].revents = 0;
    nfds++;
    tempSocketList=tempSocketList->next;
  }
  pthread_mutex_unlock(&atri_socket_list_mutex);
  if(nfds==0) return 0;

  //  ARA_LOG_MESSAGE(LOG_DEBUG,"nfds = %d\n",nfds);

  pollVal=poll(fds, nfds, 1);
  if ( pollVal == -1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fAtriControlSocket);
    return -1;
  }
  
  if(pollVal==0) { 
    //Nothing has happened
    return 0;
  }

  //If we are here then something has happened
  //Now loop through connections and see what has happened
  for(tempInd=0;tempInd<nfds;tempInd++) {
    int thisFd=fds[tempInd].fd;
    if (fds[tempInd].revents & POLLHUP) {
      // done
      ARA_LOG_MESSAGE(LOG_INFO,"%s: Connection closed\n",__FUNCTION__);
      close(thisFd);
      removeFromAtriControlSocketList(thisFd);
      thisFd = 0;
    } else if (fds[tempInd].revents & POLLIN) {
      // data on the pipe
      nbytes = recv(thisFd, &controlPacket, sizeof(AtriControlPacket_t), 0); 
      if (nbytes) {
	ARA_LOG_MESSAGE(LOG_DEBUG,"%s: %d bytes from control pipe\n", __FUNCTION__,nbytes);
	
	///Now add it to the queue and add it to the packet list
	addControlPacketToQueue(&controlPacket);
	addToAtriPacketList(thisFd,controlPacket.header.packetNumber);
	//	fprintf(stderr,"Got packet %d from thisFd %d\n",controlPacket.header.packetNumber,thisFd);
      } else {
	ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Connection closed\n",__FUNCTION__);
	close(thisFd);
	removeFromAtriControlSocketList(thisFd);
	thisFd = 0;
      }
    }  
  }
	 
  return 1;
}


void addControlPacketToQueue(AtriControlPacket_t *packetPtr)
{
  //should add a check packet link here
  ///but won't for now
  pthread_mutex_lock(&atri_packet_queue_mutex);
  AtriPacketQueueFifo_t *tempQueue=(AtriPacketQueueFifo_t*)malloc(sizeof(AtriPacketQueueFifo_t));
  AtriPacketQueueFifo_t *nextQueue;
  fAtriPacketNumber++;
  
  //The software breaks if it tries packet number 0
  if(fAtriPacketNumber==0) fAtriPacketNumber++;
  packetPtr->header.packetNumber=fAtriPacketNumber;
  memcpy(&(tempQueue->controlPacket),packetPtr,sizeof(AtriControlPacket_t));
  tempQueue->next=NULL;

  if(fAtriPacketFifo==NULL) {
    fAtriPacketFifo=tempQueue;
  }
  else {
    nextQueue=fAtriPacketFifo;
    while(nextQueue->next) {
      nextQueue=nextQueue->next;
    }
    nextQueue->next=tempQueue;
  }
  pthread_mutex_unlock(&atri_packet_queue_mutex);  

  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: packetNumber=%d\n",__FUNCTION__,packetPtr->header.packetNumber);

}

int getControlPacketFromQueue(AtriControlPacket_t *packetPtr)
{
  pthread_mutex_lock(&atri_packet_queue_mutex);
  AtriPacketQueueFifo_t *tempQueue=fAtriPacketFifo;
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"getControlPacketFromQueue tempQueue=%d\n",tempQueue);
  if(tempQueue==NULL) {
      pthread_mutex_unlock(&atri_packet_queue_mutex);  
      return 0;
  }
  AtriPacketQueueFifo_t *nextInQueue=fAtriPacketFifo->next;  
  memcpy(packetPtr,&(tempQueue->controlPacket),sizeof(AtriControlPacket_t));
  
  free(fAtriPacketFifo);
  fAtriPacketFifo=nextInQueue;     
  pthread_mutex_unlock(&atri_packet_queue_mutex);  
  ARA_LOG_MESSAGE(LOG_DEBUG,"Got packetNumber=%d from queue\n",packetPtr->header.packetNumber);
  return 1;

}



//Now for the worker functions
void initFx2ControlSocket(char *socketPath)
{
  struct sockaddr_un u_addr; // unix domain addr
  int len;
  //Create socket
  if ((fFx2ControlSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: socket -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }
  u_addr.sun_family = AF_UNIX;
  strcpy(u_addr.sun_path, socketPath);
  unlink(u_addr.sun_path);
  len = strlen(u_addr.sun_path) + sizeof(u_addr.sun_family);
  //Bind address
  if (bind(fFx2ControlSocket, (struct sockaddr *)&u_addr, len) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: bind -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }	

  //Listen and have a queue of up to 10 connections waiting
  if (listen(fFx2ControlSocket, 10)) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: listen -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }

  //Now set up the lists
  pthread_mutex_init(&fx2_socket_list_mutex, NULL);    

  pthread_mutex_lock(&fx2_socket_list_mutex);
  fNumFx2Sockets=0;
  fFx2ControlSocketList = NULL;
  pthread_mutex_unlock(&fx2_socket_list_mutex);

}


int checkForNewFx2ControlConnections()
{
  // This is meant to be a simple function that just checks to see if we have an incoming
  // connection.
  struct pollfd fds[1];
  int s_dat = 0; 
  struct sockaddr_un u_inaddr; // incoming
  unsigned int u_len=sizeof(u_inaddr);
  int nfds=1;
  int pollVal;

  fds[0].fd = fFx2ControlSocket;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  
  pollVal=poll(fds,nfds,1); //non-blocking, for now at least may change at some point if I put this in a thread
  if(pollVal==-1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fFx2ControlSocket);
    return -1;
  }
  if(pollVal) {    
    if (fds[0].revents & POLLIN) {
      // incoming connection
      if ((s_dat = accept(fFx2ControlSocket, (struct sockaddr *) &u_inaddr, &u_len)) == -1) {
	ARA_LOG_MESSAGE(LOG_ERR,"%s: accept -- %s\n",__FUNCTION__,strerror(errno));
	close(fFx2ControlSocket);
	return -1;
      }
      ARA_LOG_MESSAGE(LOG_DEBUG,"%s: incoming connection\n",__FUNCTION__);
      addToFx2ControlSocketList(s_dat);
    }
  }  
  return fNumFx2Sockets;
}


int addToFx2ControlSocketList(int socketFd)
{
  Fx2SocketLinkedList_t *tempSocketList=NULL;
  pthread_mutex_lock(&fx2_socket_list_mutex);
  // Create new item in the list at the head of the list
  tempSocketList=(Fx2SocketLinkedList_t*)malloc(sizeof(Fx2SocketLinkedList_t));
  tempSocketList->socketFd=socketFd;
  tempSocketList->next=fFx2ControlSocketList;
  fFx2ControlSocketList=tempSocketList;
  fNumFx2Sockets++;
  pthread_mutex_unlock(&fx2_socket_list_mutex);
  return fNumFx2Sockets;
}


int removeFromFx2ControlSocketList(int socketFd)
{
  pthread_mutex_lock(&fx2_socket_list_mutex);
  Fx2SocketLinkedList_t *lastSocketLink=NULL;
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  int count=0;
  //Search through list and remove the item with socketFd
  while(tempSocketList) {
    ARA_LOG_MESSAGE(LOG_DEBUG,"%s: %d -- %d -- %d\n",__FUNCTION__,count,tempSocketList->socketFd,socketFd);
    if(tempSocketList->socketFd==socketFd) {
      //Found our item in the list
      if(lastSocketLink) {
	lastSocketLink->next=tempSocketList->next;
      }
      else {
	fFx2ControlSocketList=tempSocketList->next;
      }
      free(tempSocketList);
      fNumFx2Sockets--;
      break;
    }
    lastSocketLink=tempSocketList;
    tempSocketList=tempSocketList->next;
    count++;
  }
  pthread_mutex_unlock(&fx2_socket_list_mutex);
  return fNumFx2Sockets;
}


void closeFx2Control(char *socketPath) 
{
  
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  while(tempSocketList) {
    close(tempSocketList->socketFd);
    tempSocketList=tempSocketList->next;
  }
  close(fFx2ControlSocket);
  unlink(socketPath);
}

int serviceOpenFx2ControlConnections()
{
  //Okay so this essentially checks the open connections and services them in some way
  //Could be combined with the check for new connections
  struct pollfd fds[MAX_FX2_CONNECTIONS];

  Fx2ControlPacket_t tempPacket;
  Fx2ResponsePacket_t responsePacket;
  int nbytes;
  int nfds=0;
  int tempInd=0;
  int pollVal;
  int retVal;

  if(fNumFx2Sockets==0) return 0;  
  pthread_mutex_lock(&fx2_socket_list_mutex);
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  //Loop through the socket list to fill the pollfd array
  while(tempSocketList) {
    fds[nfds].fd = tempSocketList->socketFd;
    fds[nfds].events = POLLIN;
    fds[nfds].revents = 0;
    nfds++;
    tempSocketList=tempSocketList->next;
  }
  pthread_mutex_unlock(&fx2_socket_list_mutex);
  if(nfds==0) return 0;

  //  ARA_LOG_MESSAGE(LOG_DEBUG,"nfds = %d\n",nfds);

  pollVal=poll(fds, nfds, 1);
  if ( pollVal == -1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fFx2ControlSocket);
    return -1;
  }
  
  if(pollVal==0) { 
    //Nothing has happened
    return 0;
  }

  //If we are here then something has happened
  //Now loop through connections and see what has happened
  for(tempInd=0;tempInd<nfds;tempInd++) {
    int thisFd=fds[tempInd].fd;
    if (fds[tempInd].revents & POLLHUP) {
      // done
      ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Connection closed\n",__FUNCTION__);
      close(thisFd);
      removeFromFx2ControlSocketList(thisFd);
      thisFd = 0;
    } else if (fds[tempInd].revents & POLLIN) {
      // data on the pipe
      nbytes = recv(thisFd, (char*)&tempPacket, sizeof(Fx2ControlPacket_t), 0); 
      if (nbytes) {
	ARA_LOG_MESSAGE(LOG_DEBUG,"%s: %d bytes from control pipe\n",__FUNCTION__, nbytes);
	retVal=sendVendorRequestStruct(&tempPacket);
	memcpy(&(responsePacket.control),&tempPacket,sizeof(Fx2ControlPacket_t));
	responsePacket.status=retVal;		
	
	if (send(thisFd, (char*)&responsePacket, sizeof(Fx2ResponsePacket_t), 0) == -1) {
	  ARA_LOG_MESSAGE(LOG_ERR,"%s: send -- %s\n",__FUNCTION__,strerror(errno));
	  exit(1);
	}	 
	
      } else {
	ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Connection closed\n",__FUNCTION__);
	close(thisFd);
	removeFromFx2ControlSocketList(thisFd);
	thisFd = 0;
      }
    }    
  }
  return 1;

}

/* Switch from USB to PCIe event readout */
void enablePcieEndPoint() {
    usePcieReadout = 1;
}

void disablePcieEndPoint() {
    usePcieReadout = 0;
}

/* openPcieDevice: open the char device providing events via PCIe */
int openPcieDevice() {
    pcieHandle = open(ATRI_PCIE_DEV, O_RDONLY | O_NONBLOCK);
    return (pcieHandle < 0);
}

/* closePcieDevice(): close the PCIe event char device */
int closePcieDevice() {
    if (pcieHandle >= 0)
        return close(pcieHandle);
    else
        return -1;
}

/* struct usb_device *findEzUsbAtriBoard(){  */
/*   ARA_LOG_MESSAGE(LOG_DEBUG,"findEzUsbAtriBoard()\n");     */

/*     struct usb_bus *usb_bus; */
/*     struct usb_device *dev; */

/*     /\* init libusb*\/ */
/*     usb_init(); */
/*     usb_find_busses(); */
/*     usb_find_devices(); */

/*     /\* usb_busses is a linked list which after above init function calls contains every single usb device in the computer. */
/*         We need to browse that linked list and find EZ USB-FX2 by VENDOR and PRODUCT ID *\/ */
/*     for (usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next) { */
/*         for (dev = usb_bus->devices; dev; dev = dev->next) { */
/*             if ((dev->descriptor.idVendor == USBFX2_ATRI_VENDOR_ID) && (dev->descriptor.idProduct == USBFX2_ATRI_PRODUCT_ID)) { */
/* //                printf("init: found device: %d\n", (int)dev); */
/*                 return dev; */
/*             } */
/*         } */
/*     } */

/*     printf("init: device not found)"); */
/*     /\* on failure (device not found) *\/ */
/*     return INVALID_HANDLE_VALUE; */
/* } */

int closeFx2Device() {
  if(currentHandle==INVALID_HANDLE_VALUE) {
    printf("wtf\n");
    return SUCCEED;
  }
  ARA_LOG_MESSAGE(LOG_DEBUG,"closeFx2Device()\n");
  /* release interface */
  printf("Releasing interface\n");
  int retval = libusb_release_interface(currentHandle, USBFX2_INTFNO);
  if (retval != 0) {
    printf("ret %d\n", retval);
    return FAILED;
  }
  printf("Closing handle\n");
  /* close usb handle */
  libusb_close(currentHandle);
  
  libusb_exit(NULL);

  /* all ok */
  return SUCCEED;
  
}


int openFx2Device() {
  ARA_LOG_MESSAGE(LOG_DEBUG,"openFx2Device()\n");
  int retVal=0,i;

  
  //  pthread_mutex_init(&async_buffer_mutex,NULL);
  fNumAsyncBuffers=0;
  fNumAsyncRequests=0;

  pthread_mutex_init(&libusb_command_mutex, NULL);  
  pthread_mutex_lock(&libusb_command_mutex);
  //  struct libusb_context *contextPtr=&fUsbContext;
  currentHandle=INVALID_HANDLE_VALUE;
  for(i=0;i<NUM_ASYNC_BUFFERS;i++) asyncBufferStatus[i]=0;
  //  retVal=libusb_init(&contextPtr);
  retVal=libusb_init(NULL);
  if(retVal) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error initialising libusb\n"); 
    pthread_mutex_unlock(&libusb_command_mutex);
    return -1;
  }

  currentHandle=     libusb_open_device_with_vid_pid(NULL,USBFX2_ATRI_VENDOR_ID,USBFX2_ATRI_PRODUCT_ID);
  if(currentHandle == INVALID_HANDLE_VALUE) {
    ARA_LOG_MESSAGE(LOG_ERR,"Failed to open the usb device with vid %#x and pid %#x\n",USBFX2_ATRI_VENDOR_ID,USBFX2_ATRI_PRODUCT_ID); 
    pthread_mutex_unlock(&libusb_command_mutex);
    return -1;
  }
  

  //Don't ever uncomment this
  // I don't know why, but without resetting the device, it doesn't
  // work more than once after configuration. The problem before with
  // resetting was whatever the Cypress/Keil framework does with the
  // USB reset interrupt. So we just need to fix that.
  retVal=libusb_reset_device(currentHandle);
  if (retVal < 0) {
     ARA_LOG_MESSAGE(LOG_ERR,"Failed to reset the device %d (%s)\n",retVal,getLibUsbErrorAsString(retVal));
     pthread_mutex_unlock(&libusb_command_mutex);
     return -1;
  }
  libusb_close(currentHandle);
  currentHandle = libusb_open_device_with_vid_pid(NULL,USBFX2_ATRI_VENDOR_ID,USBFX2_ATRI_PRODUCT_ID);
  if (currentHandle == NULL) 
     {
	ARA_LOG_MESSAGE(LOG_ERR, "Failed to reacquire device after reset.\n");
	pthread_mutex_unlock(&libusb_command_mutex);
	return -1;
     }
      
  libusb_device *thisDevice =libusb_get_device(currentHandle);
  ARA_LOG_MESSAGE(LOG_DEBUG,"Got device on Bus %d, Device %d\n",libusb_get_bus_number(thisDevice),libusb_get_device_address(thisDevice));


  int cfg=0;
  retVal = libusb_get_configuration(currentHandle,&cfg);
  if (cfg != USBFX2_CNFNO) {
    retVal = libusb_set_configuration(currentHandle, USBFX2_CNFNO);
    if (retVal != 0) {
      ARA_LOG_MESSAGE(LOG_ERR,"Failed to set the configuration %d: %d (%s)\n",USBFX2_CNFNO,retVal,getLibUsbErrorAsString(retVal));
      pthread_mutex_unlock(&libusb_command_mutex);
      return -1;
    }
  }
  
  retVal=libusb_claim_interface(currentHandle, USBFX2_INTFNO);
  if(retVal<0){
    ARA_LOG_MESSAGE(LOG_ERR,"Could not claim interface %d: %d (%s)\n", USBFX2_INTFNO,retVal,getLibUsbErrorAsString(retVal));
    pthread_mutex_unlock(&libusb_command_mutex);
    return retVal;
  }

  /*  
  retVal=libusb_set_interface_alt_setting(currentHandle, USBFX2_INTFNO,USBFX2_ALT_SETTING);
  if(retVal<0){
    ARA_LOG_MESSAGE(LOG_ERR,"Could not set interface %d to alternative %d : %d (%s)\n", USBFX2_INTFNO,USBFX2_ALT_SETTING,retVal,getLibUsbErrorAsString(retVal));
    pthread_mutex_unlock(&libusb_command_mutex);
    return retVal;
  }
  */
   pthread_mutex_unlock(&libusb_command_mutex);
  return 0;

}

int flushControlEndPoint() {
  //  int retVal;
  pthread_mutex_lock(&libusb_command_mutex);

  int maxPacketSize = libusb_get_max_packet_size(libusb_get_device(currentHandle), ATRI_CONTROL_EP_READ);

  unsigned char *dump = (unsigned char *) malloc(sizeof(unsigned char)*maxPacketSize);
  if (!dump) 
    {
      ARA_LOG_MESSAGE(LOG_ERR, "%s: could not allocate buffer to flush endpoint\n", __FUNCTION__);    
      pthread_mutex_unlock(&libusb_command_mutex);
      return -9;
    }
  

   int ret,i,actual;
   //First flush control endpoint
   while (1) 
     {
       ret = libusb_bulk_transfer(currentHandle, ATRI_CONTROL_EP_READ, dump, maxPacketSize, &actual, 5);
       if(ret != 0 && ret != LIBUSB_ERROR_TIMEOUT)
	 {
	   ARA_LOG_MESSAGE(LOG_ERR,"%s: flush unsuccessful (%d)\n", __FUNCTION__, ret);
	   pthread_mutex_unlock(&libusb_command_mutex);
	   return -9;
	  }
       ARA_LOG_MESSAGE(LOG_DEBUG,"flushed %d bytes from control endpoint\n", actual);
       for (i=0;i<actual;i++) 
	 {
	   ARA_LOG_MESSAGE(LOG_DEBUG,"%2.2x ", dump[i]);
	 }
       ARA_LOG_MESSAGE(LOG_DEBUG,"\n");
       if (ret == LIBUSB_ERROR_TIMEOUT)
	 break;
     }  
   pthread_mutex_unlock(&libusb_command_mutex);
   return 0;
}


int flushEventEndPoint() {
  int retVal = 0;
  // USB event readout  
  if (!usePcieReadout) {
      pthread_mutex_lock(&libusb_command_mutex);
      
      unsigned char *dump = (unsigned char *) malloc(sizeof(unsigned char)*512);
      if (!dump) 
          {
              ARA_LOG_MESSAGE(LOG_ERR, "%s: could not allocate buffer to flush endpoint\n", __FUNCTION__);    
              pthread_mutex_unlock(&libusb_command_mutex);
              return -9;
          }
      
      
      int ret,i,actual;
      while (1) 
          {
              ret = libusb_bulk_transfer(currentHandle, ATRI_EVENT_EP_READ, dump, 512, &actual, 5);
              if(ret != 0 && ret != LIBUSB_ERROR_TIMEOUT)
                  {
                      ARA_LOG_MESSAGE(LOG_ERR,"%s: event flush unsuccessful %s\n", __FUNCTION__,getLibUsbErrorAsString(ret));
                      pthread_mutex_unlock(&libusb_command_mutex);
                      return -9;
                  }
              ARA_LOG_MESSAGE(LOG_INFO,"flushed %d bytes from event endpoint\n", actual);
              for (i=0;i<actual;i++) 
                  {
                      ARA_LOG_MESSAGE(LOG_DEBUG,"%2.2x ", dump[i]);
                  }
              ARA_LOG_MESSAGE(LOG_DEBUG,"\n");
              //       usleep(50);
              //       count++;
              if (ret == LIBUSB_ERROR_TIMEOUT)
                  break;
          }    
      free(dump);
      pthread_mutex_unlock(&libusb_command_mutex);
  }
  else {
      // Flush the PCIE endpoint
      ioctl(pcieHandle, XPCIE_IOCTL_FLUSH);
  }
  return retVal;  
}

int sendVendorRequestStruct(Fx2ControlPacket_t *controlPacket)
{
  return sendVendorRequest(controlPacket->bmRequestType,
			   controlPacket->bRequest,
			   controlPacket->wValue,
			   controlPacket->wIndex,
			   controlPacket->data,
			   controlPacket->dataLength);
}


int sendVendorRequest(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength)
{
  int ind=0;
  if(currentHandle==INVALID_HANDLE_VALUE)
    return -1;
  pthread_mutex_lock(&libusb_command_mutex);

  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Got vendor request: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%d data[0]=%#x data[1]=%#x\n",__FUNCTION__,bmRequestType,bRequest,wValue,wIndex,wLength,data[0],data[1]);
  //  fprintf(stderr,"%s: Got vendor request: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%d data[0]=%#x data[1]=%#x\n",__FUNCTION__,bmRequestType,bRequest,wValue,wIndex,wLength,data[0],data[1]);
  if(bmRequestType&0x80) {
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"Read command\n");
    for(ind=0;ind<wLength;ind++) {
      data[ind]=0;
    }
  }
  //  else {
  //    ARA_LOG_MESSAGE(LOG_DEBUG,"Write command\n");
  //  }

  int retVal=libusb_control_transfer(currentHandle,bmRequestType,bRequest,wValue,wIndex,data,wLength,USB_TOUT_MS*10);
  //  fprintf(stderr, "In sendVendorRequest: DIRECT %d B requested. Return value: %d .\n", wLength,retVal);
  //  fprintf(stderr,"VR retVal=%d\n",retVal);
  if(retVal<0){
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Could not send libusb_control_transfer (vendor request): %d %s\n", __FUNCTION__,retVal,getLibUsbErrorAsString(retVal));
    pthread_mutex_unlock(&libusb_command_mutex);
    return retVal;
  }
  pthread_mutex_unlock(&libusb_command_mutex);
  return retVal;
}

/* int sendVendorRequest(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength) */
/* { */
/*   int retVal=0; */
/*   int i=0; */
/*   if(currentHandle==INVALID_HANDLE_VALUE) */
/*     return -1; */
/*   unsigned char cOutPtr[100]; //Arbitrary length warning */
/*   for(i=0;i<100;i++) cOutPtr[i]=0; */
/*   struct libusb_transfer *ctlTransfer=0; */
/*   ctlTransfer = libusb_alloc_transfer(0); */
/*   int direction=(bmRequestType>>7); */
  

/*   ARA_LOG_MESSAGE(LOG_DEBUG,"Got vendor request: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%d\n",bmRequestType,bRequest,wValue,wIndex,wLength); */

/*   libusb_fill_control_setup(cOutPtr, bmRequestType, bRequest, wValue, wIndex , wLength); */

/*   if(direction==0) */
/*     { */
/*       for(i=0; i<wLength; i++) */
/* 	cOutPtr[LIBUSB_CONTROL_SETUP_SIZE+i] = data[i];  */
/*     } */
  
/*   stupidCBFDone=0; */
/*   libusb_fill_control_transfer(ctlTransfer, currentHandle, cOutPtr, stupidCBF, 0, USB_TOUT_MS*100); */
/*   libusb_submit_transfer(ctlTransfer); */
  
/*   ///Now wait */
/*   while((!stupidCBFDone)) { */
/*     retVal=libusb_handle_events(fUsbContext); */
/*     if(retVal<0) { */
/*       ARA_LOG_MESSAGE(LOG_DEBUG,"libusb_handle_events %d\n",retVal); */
/*       break; */
/*     } */
/*   } */

/*   if(direction==1) */
/*     { */
/*       for(i=0; i<wLength; i++) */
/* 	data[i] = cOutPtr[LIBUSB_CONTROL_SETUP_SIZE+i]; */
/*     } */
  
/*   return retVal;  */
/* } */


int readControlEndPoint(unsigned char *buffer, int numBytes, int *numBytesRead)
{
  int retVal=0;
  if(currentHandle==INVALID_HANDLE_VALUE) return 0;
  pthread_mutex_lock(&libusb_command_mutex);
  *numBytesRead=0;
  int maxPacketSize = libusb_get_max_packet_size(libusb_get_device(currentHandle), ATRI_CONTROL_EP_READ);
  if(numBytes > maxPacketSize){
    //ARA_LOG_MESSAGE(LOG_DEBUG, "%s : %d B requested but max packet size is %d, downsizing\n", __FUNCTION__, numBytes, maxPacketSize);
    //numBytes = maxPacketSize;
  }
  retVal=libusb_bulk_transfer(currentHandle, ATRI_CONTROL_EP_READ, buffer, numBytes, numBytesRead,READ_USB_TIMEOUT);
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s - %d B read when %d Brequested with return value %d DIRECT.\n",__FUNCTION__,*numBytesRead, numBytes, retVal);
  //  fprintf(stderr, "In readControlEndPoint: DIRECT Reading %d B when %d B requested. Return value: %d .\n",*numBytesRead, numBytes, retVal);
  if(retVal<0) {

    if(retVal==LIBUSB_ERROR_TIMEOUT) {

      pthread_mutex_unlock(&libusb_command_mutex);

      if((*numBytesRead)>0) {
	ARA_LOG_MESSAGE(LOG_DEBUG,"%s - %s but actually read %d bytes\n",__FUNCTION__,getLibUsbErrorAsString(retVal),*numBytesRead);
	//    fprintf(stderr, "In readControlEndPoint: FAILED Reading %d B when %d B requested. Return value: %d .\n",*numBytesRead, numBytes, retVal);
      }

      return 0;
    }
    //  fprintf(stderr, "In readControlEndPoint: FAILED Reading %d B when %d B requested. Return value: %d .\n",*numBytesRead, numBytes, retVal);
    ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Request for bulk read  of %d bytes failed: %s (retVal %d)\n", __FUNCTION__,numBytes, getLibUsbErrorAsString(retVal),retVal);

    pthread_mutex_unlock(&libusb_command_mutex);
    return retVal;
  }
  pthread_mutex_unlock(&libusb_command_mutex);
  return retVal;
}

 int writeControlEndPoint(unsigned char *buffer, int numBytes, int *numBytesSent)
{
  int retVal;
  if(currentHandle==INVALID_HANDLE_VALUE) return 0;
  pthread_mutex_lock(&libusb_command_mutex);
  retVal=libusb_bulk_transfer(currentHandle, ATRI_CONTROL_EP_WRITE, buffer, numBytes, numBytesSent,WRITE_USB_TIMEOUT);
  // fprintf(stderr, "In writeControlEndPoint: DIRECT Writing %d B when %d B requested. Return value: %d .\n",*numBytesSent, numBytes, retVal);
  if(retVal<0) {
    if(retVal==LIBUSB_ERROR_TIMEOUT) {      
      if((*numBytesSent)>0) {
	ARA_LOG_MESSAGE(LOG_DEBUG,"%s - %s but actually sent %d bytes\n",__FUNCTION__,getLibUsbErrorAsString(retVal),*numBytesSent);
	pthread_mutex_unlock(&libusb_command_mutex);
	return 0;
      }
    }        
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Request for bulk write  of %d bytes failed: %s (retVal %d)\n", __FUNCTION__,numBytes, 
		    getLibUsbErrorAsString(retVal),retVal);
    pthread_mutex_unlock(&libusb_command_mutex);
    return retVal;
  }
  pthread_mutex_unlock(&libusb_command_mutex);
  return retVal;
}

int readEventEndPoint(unsigned char *buffer, int numBytes, int *numBytesRead)
{
  ARA_LOG_MESSAGE(LOG_DEBUG, "%s (usePcieReadout = %d)\n", __FUNCTION__, usePcieReadout);
  int retVal=0;
  int cnt;
  *numBytesRead=0;
  if (!usePcieReadout) {
      if(currentHandle==INVALID_HANDLE_VALUE) return 0;
      pthread_mutex_lock(&libusb_command_mutex);
      retVal=libusb_bulk_transfer(currentHandle, ATRI_EVENT_EP_READ, buffer, numBytes, numBytesRead, READ_USB_TIMEOUT);
      if(retVal<0) {
          if(retVal==LIBUSB_ERROR_TIMEOUT) {      
              if((*numBytesRead)>0) {
                  ARA_LOG_MESSAGE(LOG_DEBUG,"%s - %s but actually read %d bytes\n",__FUNCTION__,getLibUsbErrorAsString(retVal),*numBytesRead);
		  pthread_mutex_unlock(&libusb_command_mutex);
		  return 0;
              }
          }
          ARA_LOG_MESSAGE(LOG_ERR,"%s: Request for bulk read  of %d bytes failed: %s (retVal %d)\n",__FUNCTION__, numBytes, 
			  getLibUsbErrorAsString(retVal),retVal);
          pthread_mutex_unlock(&libusb_command_mutex);
          return retVal;
      }
      else {
          ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Read returned %d, %d bytes\n",__FUNCTION__, retVal,*numBytesRead);
      }
      pthread_mutex_unlock(&libusb_command_mutex);
  }
  else {
      // Read from the PCIe device file
      if (pcieHandle >= 0) {
          cnt = read(pcieHandle, buffer, numBytes);
	  ARA_LOG_MESSAGE(LOG_DEBUG, "%s: PCIe read returned %d of %d bytes\n", __FUNCTION__, cnt, numBytes);
          if (cnt >= 0)
              *numBytesRead = cnt;
      }
  }  
  return retVal;  
}

 int writeEventEndPoint(unsigned char *buffer, int numBytes, int *numBytesSent)
{

  int retVal = 0;
  if (!usePcieReadout) {
      if(currentHandle==INVALID_HANDLE_VALUE) return 0;
      pthread_mutex_lock(&libusb_command_mutex);
      retVal=libusb_bulk_transfer(currentHandle, ATRI_EVENT_EP_WRITE, buffer, numBytes, numBytesSent, WRITE_USB_TIMEOUT);
      if(retVal<0) {
          ARA_LOG_MESSAGE(LOG_ERR,"%s:Request for bulk write  of %d bytes failed: %s (retVal %d)\n",
                          __FUNCTION__,numBytes, getLibUsbErrorAsString(retVal),retVal);
          pthread_mutex_unlock(&libusb_command_mutex);
          return retVal;
      }
      pthread_mutex_unlock(&libusb_command_mutex);
  }
  return retVal;
}



/* static void stupidCBF(struct libusb_transfer *transfer) */
/* { */
/*   ARA_LOG_MESSAGE(LOG_DEBUG,"stupidCBF status: %d\tlength %d\tactual_length %d\n",transfer->status, */
/* 	 transfer->length,transfer->actual_length); */
/*   stupidCBFDone=1; */
/* } */



char *getLibUsbErrorAsString(int error) {

  switch(error) {
  case LIBUSB_SUCCESS: return "Success (no error)";
  case LIBUSB_ERROR_IO: return 	" Input/output error ";
  case LIBUSB_ERROR_INVALID_PARAM: return 	" Invalid parameter ";
  case LIBUSB_ERROR_ACCESS: return 	" Access denied (insufficient permissions) ";
  case LIBUSB_ERROR_NO_DEVICE: return 	" No such device (it may have been disconnected) ";
  case LIBUSB_ERROR_NOT_FOUND: return 	" Entity not found ";
  case LIBUSB_ERROR_BUSY: return 	" Resource busy ";
  case LIBUSB_ERROR_TIMEOUT: return 	" Operation timed out ";
  case LIBUSB_ERROR_OVERFLOW: return 	" Overflow ";
  case LIBUSB_ERROR_PIPE: return 	" Pipe error ";
  case LIBUSB_ERROR_INTERRUPTED: return 	" System call interrupted (perhaps due to signal) ";
  case LIBUSB_ERROR_NO_MEM: return " Insufficient memory ";    
  case LIBUSB_ERROR_NOT_SUPPORTED: return " Operation not supported or unimplemented on this platform ";	
  case LIBUSB_ERROR_OTHER: 
  default:return " Other error ";
  }
}



///Asynchronous fun
static void eventEndPointCallback(struct libusb_transfer *transfer)
{
  static int whichBuffer=0;
  //Need to ensure we don't overwrite a buffer
  if(transfer->status==LIBUSB_TRANSFER_COMPLETED) {
    //    fprintf(stderr,"Got buffer %d\n",whichBuffer);
    //    whichBuffer=(int)transfer->status.user_data;
    pthread_mutex_lock(&async_buffer_mutex);
    asyncBufferSize[whichBuffer]=transfer->actual_length;   
    memcpy(&asyncBufferSoftware[whichBuffer],transfer->buffer,asyncBufferSize[whichBuffer]); 
    asyncBufferStatus[whichBuffer]=1;
    fNumAsyncBuffers++;
    pthread_mutex_unlock(&async_buffer_mutex);
    //Got buffer do I need to copy it to the asyncBuffer?
    whichBuffer++;
    if(whichBuffer>=NUM_ASYNC_BUFFERS) whichBuffer-=NUM_ASYNC_BUFFERS;
  }
  else {
    //    submitEventEndPointRequest(whichBuffer);
  }
  fNumAsyncRequests--;
  //Check to see how we are doing and submit a request
  if(fNumAsyncBuffers<(NUM_ASYNC_BUFFERS-NUM_ASYNC_REQUESTS))
    submitEventEndPointRequest();
  libusb_free_transfer(transfer);
}

int readEventEndPointFromSoftwareBuffer(unsigned char* buffer, int numBytes, int *numBytesRead)
{
  int tryCount=0;
  int gotBuffer=0;
  static int nextBuffer=0;
  *numBytesRead=0;

  //Here allow up to 10 ms for buffer to arrive
  //Might switch to a interrupt mode at some point
  for(tryCount=0;tryCount<10;tryCount++) {
    pthread_mutex_lock(&async_buffer_mutex);
    if(asyncBufferStatus[nextBuffer]==1) {
      gotBuffer=1;
      break;
    }
    pthread_mutex_unlock(&async_buffer_mutex);
    usleep(1);
  }
  

  if(gotBuffer) {
    //    fprintf(stderr,"Read buffer %d %#x %#x\n",nextBuffer,(int)asyncBufferSoftware[nextBuffer][0],(int)asyncBufferSoftware[nextBuffer][1]);
    memcpy(buffer,&asyncBufferSoftware[nextBuffer],asyncBufferSize[nextBuffer]);
    *numBytesRead=asyncBufferSize[nextBuffer];
    asyncBufferSize[nextBuffer]=0;
    asyncBufferStatus[nextBuffer]=0;
    fNumAsyncBuffers--;
    pthread_mutex_unlock(&async_buffer_mutex);
    nextBuffer++;
    if(nextBuffer>=NUM_ASYNC_BUFFERS) nextBuffer-=NUM_ASYNC_BUFFERS;    
  }
  return 0;
}

void submitEventEndPointRequest()
{
  if(currentHandle==NULL) return;
  static int whichBuffer=0;
  //Step 1 allocate the transfer
  struct libusb_transfer *eventTransfer=0;
  eventTransfer = libusb_alloc_transfer(0);
  
  //Step 2 fill the transfer
  libusb_fill_bulk_transfer(eventTransfer,currentHandle,ATRI_EVENT_EP_READ,asyncBuffer[whichBuffer],512,eventEndPointCallback,NULL,1000);

  pthread_mutex_lock(&async_buffer_mutex);
  libusb_submit_transfer(eventTransfer);
  fNumAsyncRequests++;
  pthread_mutex_unlock(&async_buffer_mutex);

  whichBuffer++;
}


void pollForUsbEvents()
{
  const struct libusb_pollfd** thePollfds = libusb_get_pollfds(NULL);
 
  struct pollfd fds[10];
  int count=0;
  int pollVal;
  while(thePollfds[count]) {
    fds[count].fd=thePollfds[count]->fd;
    fds[count].events=thePollfds[count]->events;
    fds[count].revents=0;
  }

  if(libusb_try_lock_events(NULL)==0) {
    //Obtained event lock
    if(!libusb_event_handling_ok(NULL)) {
      libusb_unlock_events(NULL);
      return;
    }
    
    pollVal=poll(fds,count,50); 
    if(pollVal) {
      libusb_handle_events_locked(NULL,0);
    }
    libusb_unlock_events(NULL);
  }
}

int getNumAsyncBuffers()
{
  pthread_mutex_lock(&async_buffer_mutex);
  int val=fNumAsyncBuffers;
  pthread_mutex_unlock(&async_buffer_mutex);
  return val;
}

int getNumAsyncRequests()
{
  pthread_mutex_lock(&async_buffer_mutex);
  int val=fNumAsyncRequests;
  pthread_mutex_unlock(&async_buffer_mutex);
  return val;
}

