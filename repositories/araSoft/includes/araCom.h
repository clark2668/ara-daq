/*! \file araCom.h
    \brief Header file which specifies the ARA control and command structures
    
    The header file which specifies all of the ARA control and command structures. These are the routines which allow the run control system to work.
    
    May 2011 rjn@hep.ucl.ac.uk

*/
/*
 *
 * Current CVS Tag:
 * $Header: $
 */

#ifndef ARA_COM_H
#define ARA_COM_H
#include <stdint.h>



//!  The Ara Run Control Command Code enumerations
/*!
  When messages are sent to or from the ARA Run Control they are one of these
*/
enum {
  ARA_RC_RESPONSE=1, ///< Every command gets a repsonse
  ARA_RC_QUERY_STATUS, ///< Simple returns a reponse saying what the current status is
  ARA_RC_START_RUN,  ///< Command to start a run
  ARA_RC_STOP_RUN ///< Command to stop a run
}; 
typedef uint8_t AraRunControlTypeCode_t;


//!  The Ara Run Control Location Code enumerations
/*!
  Just tags where the message came from may ened up with more options (such as station number)
*/
enum {
  ARA_LOC_RC=0,
  ARA_LOC_ARAD_ARA1=1
}; 
typedef uint8_t AraRunControlLocationCode_t;


//!  The Ara Run Type
/*!
  These are the various run types possible for ARAAcqd
*/
enum {
  ARA_RUN_TYPE_NA=0x0, ///< Not applicable
  ARA_RUN_TYPE_NORMAL=0x1, ///< Standard data taken
  ARA_RUN_TYPE_PEDESTAL=0x2, ///< Pedestal run
  ARA_RUN_TYPE_THRESHOLD=0x3 ///< Threshold scan
} ;
typedef uint8_t AraRunType_t;



//!  The Ara Run Control Message Structure
/*!
  Here is the simple structure for sending messages to and from the runc
*/
typedef struct {
  uint32_t checkSum; ///< check sum
  AraRunControlLocationCode_t from; ///< Either AraRC or ARAd (one of them)
  AraRunControlLocationCode_t to; ///< Either AraRC or ARAd (one of them)
  AraRunControlTypeCode_t type; ///< Either command or response
  uint8_t length; ///< number of bytes in params
  AraRunType_t runType; ///Normal, pedestal or threshold
  int8_t params[256]; ///< Params or commands or repsonses
} AraRunControlMessageStructure_t;



//!  The Ara Program Status
/*!
  These are the four statuses that an ARA program can be in
*/
enum {
  ARA_PROG_NOTASTATUS=0, ///< Initialisation value
  ARA_PROG_IDLE=1, ///< Idle not doing anything
  ARA_PROG_PREPARING, ///< Setting up stuff ready to start
  ARA_PROG_PREPARED, ///< Ready to start
  ARA_PROG_RUNNING,  ///< Actively taking data
  ARA_PROG_STOPPING, ///< Stoping, closing files etc
  ARA_PROG_TERMINATE ///< Stop the program
}; 
typedef uint8_t AraProgramStatus_t;


//!  The Ara Program Control
/*!
  These are the control messages that are sent ot the programs
*/
enum {
  ARA_PROG_CONTROL_QUERY=0x100,
  ARA_PROG_CONTROL_PREPARE=0x101, ///< Prepare to start
  ARA_PROG_CONTROL_START, ///< Start taking data
  ARA_PROG_CONTROL_STOP, ///< Stop data taking
  ARA_PROG_CONTROL_TERMINATE ///< Stop and end the program
} ;
typedef uint32_t AraProgramControl_t;



//!  The Ara Program Command
/*!
  The command structure for control messages that are sent to the programs
*/
typedef struct {
  uint32_t feWord; //< 0xfefefefe
  AraProgramControl_t controlState; ///< What should the program do?
  AraRunType_t runType; ///<What kind of run
  uint32_t runNumber;
  uint32_t efWord; //< 0xefefefef
} AraProgramCommand_t;


//!  The Ara Program Response
/*!
  The command structure for control messages that are sent to the programs
*/
typedef struct {
  uint32_t abWord; ///< 0xabababab
  AraProgramControl_t controlState; ///< What has the program been told do?
  AraProgramStatus_t currentStatus; ///< What is the current status of the program?
  uint32_t baWord; ///< 0xbabababa
} AraProgramCommandResponse_t;





#endif //ARA_COM_H
