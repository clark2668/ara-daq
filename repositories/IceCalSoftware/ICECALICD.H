/* ICECALICD.H 

   Relates to: ICECAL 

   Copyright (C) 2005 Data Design Corporation                                
   All rights reserved.                                                      

*/ 

#ifndef ICD_H
#define ICD_H


            
/* This header file works as an interface control document (ICD) as it establishes
   an agreement between the host and the device about what data packets mean.
   
   The ICECAL system has two ends to it, one at the surface and a remote unit generally 
   down a hole in the ice, connected with a coax cable with power and a modulated 
   signal containing a CAN protocol.  In operation the host talks to the surface unit 
   via a standard electrical link and the surface unit can execute a command locally 
   or talk to the remote unit.  The electrical link is RS232 by default, but it could
   also be I2C or some other future implemented form.  
   
   The data protocol will be the same for any stream link even if the transport may 
   be quite different.  Both units will be able to respond to commands from the RS232
   port or from the I2C (if implemented) or other such control stream (if implemented).
   Over the CAN link, only the remote unit will need to respond to commands and 
   the surface unit will need to know how to forward commands and responses.
   
   This code base is the same for both units and will have slightly different 
   functionality based on which role the processor is playing.  The processor 
   knows the assigned role by the state of a UNIT_ID pin.

   Establishing general terms:

   Commands and responses are ASCII strings, including framing characters,
   a three letter identifier, and a data field.  The data field can have multiple
   values seperated by commas, but in this application there is typically only 
   one value.  The general form of the command is 

   <STX>CCC[x,y,...]<EOT> 
   where 
    CCC is the three letter command
    x,y,... is the optional list of ASCII readable integers 
            numbers can be signed but not fractioned, limited to at most 10 arguments
   for example
    CCC
    CCC7
    CCC1,2,3
    CCC10,-124

   The typical ICECAL form is 

   <STX>CCCx<EOT> 

   The recipient of such command will echo the command to indicate it has been completed, 
   or return the command with data in the case of a query.  If the command could not be 
   completed then the host will eventually recieve a NACK frame as <STX>NACK<EOT>.  
   Any return packet will be sent only to the point which sent the command.

   CAN protocol:

   On the CAN bus the protocol is the same, except the framing is already provided by 
   the CAN protocol.  The framing characters have value in a streamed protocol, even if 
   commands are in fact expected to arrive one at a time.  In the CAN protcol, the packet 
   is limited to 8 bytes (at least without adding complication).  To avoid creating yet 
   another protcol, we define our end to end CAN bus as carrying the same protocol at 
   the data layer while using the CAN transport framing to take the place of the framing 
   characters required for a stream transport.  So over the CAN bus, a pack would contain 
   data such as "CCCx", recalling that x could be several characters (for example, "CCC256") 
   but there must be no more than 8 characters in total.

   So we define our CAN bus as having a simple two end points with two end point ids 
   and one recieve buffer and one transmit buffer each.  The data contents will be 
   these strings.

   RS232 port usage:

   The RS232 port is running 9600,N,8,1 and will always be available on both ends.  
   Both ends will come out of reset in a diagnostic mode.  An splash message will be sent, 
   such as 

   "UM110_ICECAL_X1 Surface Unit"

   A carriage return (CR) character will cause a menu to be displayed.  At any time if 
   an <STX> character is detected, the diagnostic mode is terminated and the unit starts 
   executing commands from this port based on the protocol.  In principle, regardless of 
   the state of the diagnostic mode on the RS232 port, other source of commands (CAN, other) 
   could operate the same way.  During debug, the RS232 port can also be used for printf 
   type debugging information.  

   In normal operation, using the dignostic menu on the surface unit will send and 
   recieve protocol commands on the CAN bus to the remote unit where appropriate.  
   The remote unit will have its RS232 port in diagnostic mode, but there is of course
   noone there to know it.
*/

/*--------------------------------------------------------------------------*/

/* General protocol definitions which will be needed by any code using this
   protocol to talk to a serial device.
*/

// Maximum command sizes supported by this application
#define MAX_COMMAND_CHARACTERS          20 
#define MAX_COMMAND_ARGUMENTS           2 

// ASCII framing character values
#define STX                             2
#define EOT                             4

/*--------------------------------------------------------------------------*/

/* Command Tokens */

// Conventions -
// Surface unit operations will start with S in the token
// Remote unit operations wills start with R in the token
// The surface unit will forward tokens to the remote unit and will
// return values recieved except that if a timeout passes or other 
// error occurs, NACK is returned automatically by the surface unit

// Values for setting the antenna selection switch
#define ANTENNA_A                       0
#define ANTENNA_B                       1
 
// Definition of system operating modes
#define OPMODE_OFF                      0   /* Pulser is disabled, all sources and antennas terminated */
#define OPMODE_PULSE_NORMAL             1   /* Pulser enabled, pulse source selected, pulse follows trigger (external source or manual software trigger) */
#define OPMODE_PULSE_AUTO               2   /* Same as 0 except a software trigger is applied once per second */
#define OPMODE_NOISE_ON                 3   /* Pulser disabled, noise source selected, steady noise output */
#define OPMODE_NOISE_BURST_NORMAL       4   /* Pulser disabled, noise source selected, antenna switch gates on output for one second upon arrival of trigger */
#define OPMODE_NOISE_BURST_AUTO         5   /* Pulser disabled, noise source selected, antenna switch gates on output for one second every other second */
/* Note - Software triggers are not time related to any other event.  A causal but asynchronous relationship exists in mode 4, with variation in the 
   millseconds for the start and stop time of the noise burst.  The software trigger applies a physical trigger pulse to the trigger comparator input, 
   which can conflict with a signal applied by an external source.  The software trigger will generally override an external source holding the input 
   low - see schematic.  The software trigger signal is high impedence except when in use (about a 1ms logic high pulse is applied).  An external trigger 
   signal can always cause a trigger.  Mode 5 is not affected by the external trigger or software trigger.
*/

// Exception response
// Returned by any unit if a command did not execute correctly
// An ackowledge response is an echo of the issued command
#define ICECAL_RESPONSE_NACK                                "NACK"

// Surface Unit Commands

// Read line voltage
#define ICECAL_COMMAND_SURFACE_READ_LINE_VOLTAGE            "SLV"
#define ICECAL_CMDID_SURFACE_READ_LINE_VOLTAGE 1

// Unit returns the line voltage in tenths of a volt.  For example, in the ideal case 
// of 12 volts on the line, the unit will return "SLV120"

// Read temperature
#define ICECAL_COMMAND_SURFACE_READ_TEMPERATURE             "SPT"
#define ICECAL_CMDID_SURFACE_READ_TEMPERATURE 2
// Unit returns the temperature of the processor chip in K units.  For example, at
// room temperature the unit should return "SRT298", but self heating will result
// in something more like "SRT310"
         
// Remote Unit Commands

// Read line voltage
#define ICECAL_COMMAND_REMOTE_READ_LINE_VOLTAGE             "RLV"
#define ICECAL_CMDID_REMOTE_READ_LINE_VOLTAGE 3
// Unit returns the line voltage in tenths of a volt.  For example, in the ideal case 
// of 12 volts on the line, the unit will return "RLV120"

// Read temperature
#define ICECAL_COMMAND_REMOTE_READ_TEMPERATURE              "RPT"
#define ICECAL_CMDID_REMOTE_READ_TEMPERATURE 4
// Unit returns the temperature of the processor chip in K unit.  For example, at
// room temperature the unit should return "RRT298", but self heating will result
// in something more like "RRT310"

// Set operating mode
#define ICECAL_COMMAND_SET_OPERATING_MODE                   "ROM"
#define ICECAL_CMDID_SET_OPERATING_MODE 5
// Command should be followed by a number corresponding to one of the 
// operating modes defined above.  This setting controls the source selection,
// the pulser enable, and a number of automatic operations.

// Set attenuator
#define ICECAL_COMMAND_SET_ATTENUATOR                       "RSA"
#define ICECAL_CMDID_SET_ATTENUATOR 6
// Command should be followed by one or two digits setting in the range 0-31 for the 
// attenuator setting.  This operates the 1dB attenuator steps.  The attenuator 
// is set as a linear binary value with an LSB equating to 1dB.  The return value 
// should match the command or NACK.  This attenuator affects either selected source.

// Antenna select  
#define ICECAL_COMMAND_ANTENNA_SELECT                       "RAS"
#define ICECAL_CMDID_ANTENNA_SELECT 7
// Command should be followed 0 or 1 to select antenna A or B as the active element.
// This command switches an RF switch which applies the output signal to one of 
// the antennas.  The unused antenna sees 50 ohms to ground.  

// Software trigger
#define ICECAL_COMMAND_SOFTWARE_TRIGGER                     "RST"
#define ICECAL_CMDID_SOFTWARE_TRIGGER 8
// This command causes a software trigger of the trigger input.  This will apply 
// a momentary (about 1ms) pulse to the trigger comparator input from the local 
// processor and return the signal to a high impedence state so that an external 
// source may again driver the trigger.  The timing has no relationship to any 
// other event.  This action is taken without regard to the timing of other 
// software trigger sources such as from an auto trigger mode.  The trigger will 
// have the effect determined by the operating mode.   

// Version request.
#define ICECAL_COMMAND_SURFACE_VERSION						"SVR"
#define ICECAL_CMDID_SURFACE_VERSION 9
// Remote version request
#define ICECAL_COMMAND_REMOTE_VERSION						"RVR"
#define ICECAL_CMDID_REMOTE_VERSION 10
// ID request.
#define ICECAL_COMMAND_SURFACE_ID							"SID"
#define ICECAL_CMDID_SURFACE_ID	11
// Remote ID request
#define ICECAL_COMMAND_REMOTE_ID							"RID"
#define ICECAL_CMDID_REMOTE_ID 12


/*--------------------------------------------------------------------------*/

#endif



