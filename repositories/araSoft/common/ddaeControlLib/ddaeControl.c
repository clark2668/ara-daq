/* 
   Port of atriControl.c to DDA_EVAL network interface.

   Note that these functions are named *exactly* the same as the
   atriControl.c functions. Therefore this MUST be a different library
   than atriControl. We then compile two versions of ARAAcqd, one
   against atriControl, one against ddaeControl. Might even be able
   to fudge it with LD_PRELOAD.
*/

#include "araSoft.h"
#include "ddaeControl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

/******
 * PHY VARIABLES
 ******/

// Sockets
int connSocket;
int ctrlSocket;
int eventSocket;
// Socket addresses.
struct sockaddr_in so_conn;
struct sockaddr_in so_ctrl;
struct sockaddr_in so_event;


//static void stupidCBF(struct libusb_transfer *transfer);
//int stupidCBFDone=0;

static const int SUCCEED = 1;
static const int FAILED  = 0;

/******
 * SOCKET INTERFACE STAYS UNCHANGED
 ******/

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

  //Now set up the lists
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
  AtriPacketLinkedList_t *tempPacketList=fPacketList;
  //  fprintf(stderr,"removeFromAtriPacketList: packetNumber=%u tempPacketList=%d\n",packetNumber,(int)tempPacketList);
  int count=0;
  int socketFd=0;
  pthread_mutex_lock(&atri_packet_list_mutex);
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
  AtriSocketLinkedList_t *lastSocketLink=NULL;
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  int count=0;
  pthread_mutex_lock(&atri_socket_list_mutex);
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
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  pthread_mutex_lock(&atri_socket_list_mutex);
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

  AtriPacketQueueFifo_t *tempQueue=(AtriPacketQueueFifo_t*)malloc(sizeof(AtriPacketQueueFifo_t));
  AtriPacketQueueFifo_t *nextQueue;
  pthread_mutex_lock(&atri_packet_queue_mutex);
  fAtriPacketNumber++;
  
  //The software breaks if it tries packet number 0
  if(fAtriPacketNumber==0) fAtriPacketNumber++;
  packetPtr->header.packetNumber=fAtriPacketNumber;
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: packetNumber=%d\n",__FUNCTION__,packetPtr->header.packetNumber);
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
  Fx2SocketLinkedList_t *lastSocketLink=NULL;
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  int count=0;
  pthread_mutex_lock(&fx2_socket_list_mutex);
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
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  pthread_mutex_lock(&fx2_socket_list_mutex);
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

/********
 * PHY BEGINS HERE
 ********/

int closeFx2Device() {
  ARA_LOG_MESSAGE(LOG_DEBUG,"(DDAE) closeFx2Device()\n");
  // No FX2 device, but we close all three of the open network sockets.
  close(connSocket);
  close(eventSocket);
  close(ctrlSocket);
  return SUCCEED;
}


int openFx2Device() {
  char *ipaddr;
  const char *ipaddr_default = "192.168.42.2";
  char buf[32];
  int flags;

  int retVal=0;
  // leave the mutexes alone
  pthread_mutex_init(&libusb_command_mutex, NULL);  
  pthread_mutex_lock(&libusb_command_mutex);
  // Now we open connection.

  ipaddr = getenv("DDA_EVAL_IP_ADDRESS");
  if (!ipaddr) 
     {
	ARA_LOG_MESSAGE(LOG_INFO, "%s: no DDA_EVAL_IP_ADDRESS, using 192.168.42.2\n", __FUNCTION__);
	ipaddr = ipaddr_default;
     }   
  // Create sockets.
  connSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (connSocket == -1) {
    perror("Error opening connection socket");
    return -1;
  }
  ctrlSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
  if (ctrlSocket == -1) {
    perror("Error opening control socket");
    return -1;
  }
  if ((flags=fcntl(ctrlSocket, F_GETFL, 0)) == -1)
    flags = 0;
  retVal = fcntl(ctrlSocket, F_SETFL, flags | O_NONBLOCK);


  eventSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (eventSocket == -1) {
    perror("Error opening event socket");
    return -1;
  }
  if ((flags=fcntl(eventSocket, F_GETFL, 0)) == -1)
    flags = 0;
  retVal = fcntl(eventSocket, F_SETFL, flags | O_NONBLOCK);
  if (retVal < 0) 
     {
	perror("Error setting socket to nonblocking mode");
	return -1;
     }
   

  struct sockaddr_in addr;
  // Bind the connection socket
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7000);
  addr.sin_addr.s_addr=htonl(INADDR_ANY);
  if (bind(connSocket, (struct sockaddr *) &(addr), sizeof(addr)) == -1) {
    perror("Error binding connection socket");
    close(connSocket); close(ctrlSocket); close(eventSocket);
    return -1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7001);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(ctrlSocket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    perror("Error binding control socket");
    close(connSocket); close(ctrlSocket); close(eventSocket);
    return -1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7002);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(eventSocket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    perror("Error binding event socket");
    close(connSocket); close(eventSocket); close(ctrlSocket);
    return -1;
  }
  // Initialize the outbounds.
  memset((char *) &so_conn, 0, sizeof(so_conn));
  memset((char *) &so_ctrl, 0, sizeof(so_ctrl));
  so_conn.sin_family = AF_INET;
  so_ctrl.sin_family = AF_INET;
  so_conn.sin_port = htons(7000);
  so_ctrl.sin_port = htons(7001);
  if (inet_aton(ipaddr, &(so_conn.sin_addr)) == 0) {
    perror("inet_aton() failed for conn port");
    ARA_LOG_MESSAGE(LOG_ERR, "%s: failed resolving %s\n", __FUNCTION__, ipaddr);
    close(connSocket); close(eventSocket); close(ctrlSocket);
    return -1;
  }
  if (inet_aton(ipaddr, &(so_ctrl.sin_addr)) == 0) {
    perror("inet_aton() failed for conn port");
    ARA_LOG_MESSAGE(LOG_ERR, "%s: failed resolving %s\n", __FUNCTION__, ipaddr);
    close(connSocket); close(eventSocket); close(ctrlSocket);
    return -1;
  }
  
  // OK! Everyone's open, let's fling off a connection request.
  if (retVal = sendto(connSocket, buf, 0, 0, (struct sockaddr *) &so_conn, sizeof(so_conn)) < 0) {
    perror("Error sending connection request");
    close(connSocket); close(eventSocket); close(ctrlSocket);
    return -1;
  }
  if ((retVal = recvfrom(connSocket, buf, 32, 0, NULL, NULL)) < 0) {
    perror("Error receiving response to connection request");
    close(connSocket); close(eventSocket); close(ctrlSocket);
    return -1;
  }
  ARA_LOG_MESSAGE(LOG_DEBUG, "(DDAE) openFx2Device() : %s\n", buf);
  // and we're all good!
  pthread_mutex_unlock(&libusb_command_mutex);
  return 0;

}

int flushControlEndPoint() {
  // whatever: this should never be an issue, I don't think
  return 0;
}


int flushEventEndPoint() {
  // this should never be an issue, I don't think
  return 0;
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
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: (DDAE) Got vendor request: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%d data[0]=%#x data[1]=%#x\n",__FUNCTION__,bmRequestType,bRequest,wValue,wIndex,wLength,data[0],data[1]);
  return 0;
}

int readControlEndPoint(unsigned char *buffer, int numBytes, int *numBytesRead)
{
  fd_set fds;
  struct timeval tv;
  int retVal=0;
  int max;
  FD_ZERO(&fds);
  FD_SET(ctrlSocket, &fds);
  max = ctrlSocket;
  tv.tv_sec = 0;
  tv.tv_usec = 5000;
  if (numBytesRead) *numBytesRead = 0;
  retVal = select(max+1, &fds, NULL, NULL, &tv);
  if (retVal == 0) return 0;
  if (FD_ISSET(ctrlSocket, &fds)) {
    retVal = recvfrom(ctrlSocket, buffer, numBytes, 0, NULL, NULL);
    if(retVal<0) {
      ARA_LOG_MESSAGE(LOG_ERR,"%s: (DDAE) Request for control socket read  of %d bytes failed: error %d)\n", __FUNCTION__,numBytes, retVal);
      return retVal;
    }
    if (numBytesRead) *numBytesRead = retVal;
  } else {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: (DDAE) select returned %d, ctrlSocket not readable\n", __FUNCTION__, retVal);
  }
  return 0;
}

int writeControlEndPoint(unsigned char *buffer, int numBytes, int *numBytesSent)
{
  int retVal;
  retVal=sendto(connSocket, buffer, numBytes, 0, (struct sockaddr *) &so_ctrl, sizeof(so_ctrl));
  if(retVal<0) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: (DDAE) Request for socket write  of %d bytes failed: error %d)\n", __FUNCTION__,numBytes, retVal);
    return retVal;
  }
  if (numBytesSent) *numBytesSent = retVal;
  return retVal;
}

int readEventEndPoint(unsigned char *buffer, int numBytes, int *numBytesRead)
{
  fd_set fds;
  struct timeval tv;
  int retVal=0;
  int max;
  FD_ZERO(&fds);
  FD_SET(eventSocket, &fds);
  max = eventSocket;
  tv.tv_sec = 0;
  tv.tv_usec = 5000;
  
  // Zero out the return...
  if (numBytesRead) *numBytesRead = 0;
  retVal = select(max+1, &fds, NULL, NULL, &tv);
  if (retVal == 0) return 0;
  if (FD_ISSET(eventSocket, &fds)) {
    retVal = recvfrom(eventSocket, buffer, numBytes, 0, NULL, NULL);
    if(retVal<0) {
      ARA_LOG_MESSAGE(LOG_ERR,"%s: (DDAE) Request for eventSocket read  of %d bytes failed: error %d)\n", __FUNCTION__,numBytes, retVal);
      return retVal;
    }
    if (numBytesRead) *numBytesRead = retVal;
  } else {
    ARA_LOG_MESSAGE(LOG_ERR, "%s: (DDAE) select returned %d, eventSocket not readable\n", __FUNCTION__, retVal);
  }
  return 0;
}

 int writeEventEndPoint(unsigned char *buffer, int numBytes, int *numBytesSent)
{
  ARA_LOG_MESSAGE(LOG_ERR,"%s: (DDAE) event writes not implemented yet\n",
		  __FUNCTION__);
  return -1;
}

/* static void stupidCBF(struct libusb_transfer *transfer) */
/* { */
/*   ARA_LOG_MESSAGE(LOG_DEBUG,"stupidCBF status: %d\tlength %d\tactual_length %d\n",transfer->status, */
/* 	 transfer->length,transfer->actual_length); */
/*   stupidCBFDone=1; */
/* } */

const char *noUsbErrors = "no USB errors";

char *getLibUsbErrorAsString(int error) 
{
   return noUsbErrors;
}


