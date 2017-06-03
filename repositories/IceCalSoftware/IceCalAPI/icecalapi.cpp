/****************************************************************************/
/*                                                                          */
/* Program: ICECALAPI                                                       */
/* Version: 1.10                                                            */
/* Author: Mark A. Shaw                                                     */
/* Company: Data Design Corporation                                         */
/*                                                                          */
/* Copyright (C) 2009 Data Design Corporation                               */
/* All rights reserved.                                                     */
/*                                                                          */
/* Linux Port                                                               */
/* Author: Jonathan Wonders                                                 */
/* University of Maryland                                                   */
/*                                                                          */
/****************************************************************************/

#define STRICT
#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <memory.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "../ICECALICD.H"
#include "icecalapi.h"

/*--------------------------------------------------------------------------*/
/* DLL Private Functions And Data Declarations                              */
/*--------------------------------------------------------------------------*/

/* Primative hardware functions */
DWORD HW_SendCommand(const char *Token, BYTE *ArgumentCount, short *Arguments);
DWORD HW_SetDefaults(void);
DWORD HW_SerialOpen(char *SerialPortName);
DWORD HW_SerialClose(void);
DWORD HW_SerialGetByte(BYTE *Value);
DWORD HW_SerialSendByte(BYTE *Value);
DWORD HW_SerialClear(void);
DWORD HW_SeekResponse(void);

/* General purpose subroutine declarations */
short GetNumArg(char **ArgStr);
DWORD ErrorNoDevice(void);
DWORD ErrorBadValue(void);
DWORD ErrorBadLength(void);
DWORD ErrorBadAddress(void);
DWORD ErrorNullPointer(void);
DWORD ErrorInternal(void);

/* Local data definitions */

// Return values for HW_SendCommand routine
#define HW_SENDCOMMAND_ERROR        0       
#define HW_SENDCOMMAND_SUCCESS      1
#define HW_SENDCOMMAND_NACK         2
#define HW_SENDCOMMAND_SYNCERROR    3

// General text buffer
char msg[2000]; 

// Variables needed by many routines
BYTE ArgCount;
short Args[MAX_COMMAND_ARGUMENTS];

/*--------------------------------------------------------------------------*/
/* Exported Abstract Instrument Functions                                   */
/*--------------------------------------------------------------------------*/

/* ICECAL_OPEN attempts to open a connection to the serial port specified 
   by its symbolic link name (ie "COM1") and establish a dialog with an
   IceCal device on the port.  If all this succeeds, then a 1 is returned, 
   otherwise a 0 is returned.
*/

DWORD ICECAL_Open(char *SerialPortName)
{
    // std::cout << "ICECAL_Open" << std::endl;

    // Check arguments
    if(SerialPortName == NULL) 
        return ErrorNullPointer();

    if(strstr(SerialPortName, "/dev/ttyS") != SerialPortName)
    {
        sprintf(msg, "%s is not a serial port on a Windows system",
                SerialPortName);
        std::cerr << "Device Open Error: "
		  << msg << std::endl;
        return ICECAL_FAILURE;
    }

    // Attempt to open serial port
    if(!HW_SerialOpen(SerialPortName))
    {
        sprintf(msg, "Unable to open serial port %s", SerialPortName);
        std::cerr << "Device Open Error: "
		  << msg << std::endl;
        return ICECAL_FAILURE;
    }

    // Attempt to gather a conversation with an IceCal unit
    // Clear the serial port
    HW_SerialClear();
    // Send a bogus command, expecting a NACK response
    ArgCount = 0;
    Args[0] = 0;
    if(HW_SendCommand("XYZ", &ArgCount, Args) != HW_SENDCOMMAND_NACK)
    {
        sprintf(msg, "There does not appear to be an Ice Calibrator "
                "unit connected to %s", SerialPortName);
        std::cerr << "Device Open Error: " << msg << std::endl;
        ICECAL_Close();
        return ICECAL_FAILURE;
    }
    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECAL_CLOSE closes any open connection to a device.  This will also
   return the device to defaults, so we do not for example leave the 
   high voltage running or something transmitting.
*/

DWORD ICECAL_Close(void)
{
    // std::cout << "ICECAL_Close" << std::endl;

    HW_SetDefaults();
    return HW_SerialClose();
}

/*--------------------------------------------------------------------------*/

/* ICECAL_GETSURFACELINEVOLTAGE asks the surface unit for a line volage 
   measurement.  The received value or error message is printed as a 
   string in the argument.
*/

DCAPI ICECAL_GetSurfaceLineVoltage(char *Voltage)
{  
    short tenthvolts;

    // Check arguments
    if(Voltage == NULL) 
        return ErrorNullPointer();
 
    // Send the command and make sure we get an argument back
    ArgCount = 0;  
    if(HW_SendCommand(ICECAL_COMMAND_SURFACE_READ_LINE_VOLTAGE, &ArgCount, Args)
       != HW_SENDCOMMAND_SUCCESS || ArgCount != 1)
    {   
        strcpy(Voltage, "UNKNOWN");
        return ICECAL_FAILURE;
    }

    // If success, get the argument returned
    tenthvolts = Args[0];
    sprintf(Voltage, "%2i.%1i V", tenthvolts/10, tenthvolts%10);
    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECALL_GETSURFACEPROCESSORTEMPERATURE attempts to collect the temperature
   reading from the surface unit and returns it as a string for display.
*/

DCAPI ICECAL_GetSurfaceProcessorTemperature(char *Temperature)
{
    short temp;

    // Check arguments
    if(Temperature == NULL) 
        return ErrorNullPointer();
 
    // Send the command and make sure we get an argument back
    ArgCount = 0;  
    if(HW_SendCommand(ICECAL_COMMAND_SURFACE_READ_TEMPERATURE, &ArgCount, Args)
       != HW_SENDCOMMAND_SUCCESS || ArgCount != 1)
    {
        strcpy(Temperature, "UNKNOWN");
        return ICECAL_FAILURE;
    }

    // If success, get the argument returned
    temp = Args[0];
    sprintf(Temperature, "%3i K", temp);
    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECAL_GETREMOTELINEVOLTAGE asks the remote unit for a line volage 
   measurement.  The received value or error message is printed as a 
   string in the argument.
*/
 
DCAPI ICECAL_GetRemoteLineVoltage(char *Voltage)
{
    short tenthvolts;

    // Check arguments
    if(Voltage == NULL) 
        return ErrorNullPointer();
 
    // Send the command and make sure we get an argument back
    ArgCount = 0;
    if(HW_SendCommand(ICECAL_COMMAND_REMOTE_READ_LINE_VOLTAGE, &ArgCount, Args) 
       != HW_SENDCOMMAND_SUCCESS || ArgCount != 1)
    {
        strcpy(Voltage, "UNKNOWN");
        return ICECAL_FAILURE;
    }

    // If success, get the argument returned
    tenthvolts = Args[0];
    sprintf(Voltage, "%2i.%1i V", tenthvolts/10, tenthvolts%10);

    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECALL_GETREMOTEPROCESSORTEMPERATURE attempts to collect the temperature
   reading from the remote unit and returns it as a string for display.
*/

DCAPI ICECAL_GetRemoteProcessorTemperature(char *Temperature)
{
    short temp;

    // Check arguments
    if(Temperature == NULL) 
        return ErrorNullPointer();
 
    // Send the command and make sure we get an argument back
    ArgCount = 0;  
    if(HW_SendCommand(ICECAL_COMMAND_REMOTE_READ_TEMPERATURE, &ArgCount, Args) 
       != HW_SENDCOMMAND_SUCCESS || ArgCount != 1)
    {
        strcpy(Temperature, "UNKNOWN");
        return ICECAL_FAILURE;
    }

    // If success, get the argument returned
    temp = Args[0];
    sprintf(Temperature, "%3i K", temp);

    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECAL_SETPULSEATTENUATOR attempts to set the attenuator for the pulse
   signal to the number specified.  The meaning of the number is as defined
   in the ICD.  The number is limited to the requisite range.
*/

DCAPI ICECAL_SetPulseAttenuator(BYTE NewSetting)
{
    // Check argumemt
    if(NewSetting > 7) 
        return ErrorBadValue();

    // Send the command
    ArgCount = 1; 
    Args[0] = NewSetting; 
    if(HW_SendCommand(ICECAL_COMMAND_SET_PULSE_ATTENUATOR, &ArgCount, Args) 
       != HW_SENDCOMMAND_SUCCESS) 
        return ICECAL_FAILURE;
  
    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECAL_SETNOISEATTENUATOR attempts to set the attenuator for the noise
   signal to the number specified.  The meaning of the number is as defined
   in the ICD.  The number is limited to the requisite range.
*/

DCAPI ICECAL_SetNoiseAttenuator(BYTE NewSetting)
{
    // Check argumemt
    if(NewSetting > 63) 
        return ErrorBadValue();

    // Send the command
    ArgCount = 1; 
    Args[0] = NewSetting; 
    if(HW_SendCommand(ICECAL_COMMAND_SET_NOISE_ATTENUATOR, &ArgCount, Args) 
       != HW_SENDCOMMAND_SUCCESS) 
        return ICECAL_FAILURE;
      
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECAL_SETHIGHVOLTAGESTATE attempts to set the high voltage control to 
   on or off as specified by the argument with values defined in the ICD.
   The values are generally 1/0 for on/off, and are forced to so comply.
*/

DCAPI ICECAL_SetHighVoltageState(BYTE NewState)
{
    // Check argumemt
    if(NewState > 1) 
        NewState = 1;

    // Send the command
    ArgCount = 1; 
    Args[0] = NewState; 
    if(HW_SendCommand(ICECAL_COMMAND_SET_HIGH_VOLTAGE, &ArgCount, Args) 
       != HW_SENDCOMMAND_SUCCESS) 
        return ICECAL_FAILURE;
    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* ICECAL_SETOUTPUTSTATE attempts to set the output select relay to the 
   noise or pulse setting as determined by the argument as defined by 
   the ICD.  The values are generally 1/0 for noise/pulse(off), and are 
   forced to so comply.
*/

DCAPI ICECAL_SetOutputState(BYTE NewState)
{
    // Check argumemt
    if(NewState > 1) 
        NewState = 1;

    // Send the command
    ArgCount = 1; 
    Args[0] = NewState; 
    if(HW_SendCommand(ICECAL_COMMAND_SET_NOISE_OUTPUT, &ArgCount, Args) 
       != HW_SENDCOMMAND_SUCCESS) 
        return ICECAL_FAILURE;
    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/
/* Hardware Support Functions                                               */
/*--------------------------------------------------------------------------*/

/* The following global data items are used by the hardware driver routines
   here but are abstract to the callers.  That is, these data are shared 
   by the HW_ routines, but not the active routines. This abstracts hardware
   operations and makes such details more readily replaced.
*/

// Handle to the serial port opened for communicating with the device
// Generally the device is an IceCal surface unit, but it may also 
// be the remote unit directly
int fdSerialPort = 0;

// Structure for holding respose data as recieved from an IceCal unit
typedef struct response_st_
{
    BYTE NumArgs;
    short Args[MAX_COMMAND_ARGUMENTS];
    char String[MAX_COMMAND_CHARACTERS+1];
} TYPE_ICECAL_RESPONSE;
TYPE_ICECAL_RESPONSE Response;

/*--------------------------------------------------------------------------*/

/* HW_SENDCOMMAND sends the specified command and data to the device 
   over the serial port and recieves a response.  The token is a string 
   from the ICD and there are zero or more aguments as appropriate.  
   The arguments field is filled with return arguments and must have enough 
   room to hold the expected number of return agrumnets.  Relevant limits
   are in the ICD.  

   The return value is a typical 1/0 response plus other relevant values 
   as defined globally for use by callers to indicate failure, success, 
   or partial success (such as NACK).

   The commands are short stings and not expected to overwhelm the serial
   port buffers.  We are always expected to have the serial port's full 
   attention and we wait for a response, so it is not necessary to wait 
   for the serial port to have room to send command bytes.  If for some
   reasons this assumption fails, the write routine will return failure.
*/

DWORD HW_SendCommand(const char *Token, BYTE *ArgumentCount, short *Arguments)
{
    char command[MAX_COMMAND_CHARACTERS+1];
    char arg[10];
    BYTE i;
    DWORD timeout = 0;

    // std::cout << "HW_SendCommand" << std::endl;

    // Check arguments
    if(Token == NULL) 
        return ErrorNullPointer();  

    if(strlen(Token) != 3) 
        return ErrorBadValue();

    if(ArgumentCount == NULL) 
        return ErrorNullPointer();

    if(Arguments == NULL) 
        return ErrorNullPointer();

    if(*ArgumentCount > MAX_COMMAND_ARGUMENTS) 
        return ErrorBadValue();

    // Load the command
    command[0] = STX;
    strcpy(&command[1], Token);
    for(i=0;i<*ArgumentCount;i++)
    {
        if(i!=0) strcat(command, ",");
        sprintf(arg, "%i", Arguments[i]);
        strcat(command, arg);
    }
    for(i=4;(command[i]!=0)&&(i<MAX_COMMAND_CHARACTERS-1);i++);
    command[i] = EOT;
    command[i+1] = 0;

    // Test point
    // std::cout << "IceCal Command: " << command << std::endl;

    // Send the command
    for(i=0;((command[i]!=0) && (i<MAX_COMMAND_CHARACTERS));i++) 
        HW_SerialSendByte((BYTE *)&command[i]);

    // Clear the response
    Response.NumArgs = 0;
    Response.String[0] = 0;

    // Wait for a response - up to 400mS 
    for(timeout=0;(!HW_SeekResponse() && (timeout<200));timeout++) 
      usleep(2000);

    if(timeout == 200) 
      return HW_SENDCOMMAND_ERROR;

    // Evaluate the response

    // The NACK response is a special case
    if(!strcmp(Response.String, ICECAL_RESPONSE_NACK)) 
        return HW_SENDCOMMAND_NACK;

    // See if the response matches the command
    // If not, then we have a sync error, clean up and return trouble
    if(strstr(Response.String, Token) != Response.String) 
    {
        HW_SerialClear();
        return HW_SENDCOMMAND_SYNCERROR;
    }

    // The token matches, load up the arguments  
    *ArgumentCount = Response.NumArgs;
    for(i=0;i<Response.NumArgs;i++) 
        Arguments[i] = Response.Args[i];
 
    
    return HW_SENDCOMMAND_SUCCESS;
} 

/*--------------------------------------------------------------------------*/

/* HW_SETDEFAULTS returns the device to agreed default states.  User programs
   will generally also establish a default state, but this provides an 
   opportunity for the API to set a device to known values on startup and 
   on closing.  The settings here are generally self explanitory based on
   the ICD, but are subject to system level discussion and change.  If there
   are any failures, this routine just gives up without any indication other
   than the return value.
*/

DWORD HW_SetDefaults(void)
{
    // std::cout << "HW_SetDefaults" << std::endl;

    // If there is no device, then forget it
    if(fdSerialPort == 0) 
        return ICECAL_FAILURE;
  
    // Clear the serial port in case there is an issue
    HW_SerialClear();

    // Pulse attenuator to full attenuation
    if(!ICECAL_SetPulseAttenuator(7)) 
        return ICECAL_FAILURE;

    // Noise attenuator to full attenuation
    if(!ICECAL_SetNoiseAttenuator(63)) 
        return ICECAL_FAILURE;

    // High voltage off
    if(!ICECAL_SetHighVoltageState(STATE_HV_OFF)) 
        return ICECAL_FAILURE;

    // Output to pulse generator (ground when no pulse present)
    if(!ICECAL_SetOutputState(STATE_OUTPUT_PULSE)) 
        return ICECAL_FAILURE;

    
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* HW_SERIALOPEN attempts to open the specified serial port for service of
   the connected device.  This calls the generic Windows driver for a serial
   port and set the port as required for the device.  The serial port name
   should be a device which exists on this system.  For example, the port
   name might be "COM1" on most systems.  Some modern systems do not have
   serial port without added hardware, but this sort of symbolic link ot
   the serial port will generally exist if the hardware is installed.
*/

DWORD HW_SerialOpen(char *SerialPortName)
{
    struct termios portSettings;
    // std::cout << "HW_SerialOpen" << std::endl;

    // If a port is open already, close it here
    if(fdSerialPort != 0) 
	HW_SerialClose();  

    // Open the serial port
    fdSerialPort = open(SerialPortName, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fdSerialPort == -1)
        return ICECAL_FAILURE;

    if(tcgetattr(fdSerialPort, &portSettings) < 0)
        return ICECAL_FAILURE;

    cfsetispeed(&portSettings, B9600);
    cfsetospeed(&portSettings, B9600);

    // Set no parity mode
    portSettings.c_cflag &= ~PARENB;
    portSettings.c_cflag &= ~CSTOPB;

    // Set 8 bit characters
    portSettings.c_cflag &= ~CSIZE;
    portSettings.c_cflag |= CS8;

    // Setup raw input mode
    portSettings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Disable parity checking
    portSettings.c_iflag &= ~INPCK;
    
    // Disable flow control
    portSettings.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Set timeout values
    portSettings.c_cc[VMIN] = 0;
    portSettings.c_cc[VTIME] = 1; // in tenths of seconds

    tcsetattr(fdSerialPort, TCSANOW, &portSettings);

    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* HW_SerialClose closes any open serial port.

 */

DWORD HW_SerialClose(void) 
{
    // std::cout << "HW_SerialClose" std::endl;

    close(fdSerialPort);
    fdSerialPort = 0;
    return ICECAL_SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* HW_SERIALGETBYTE attempts to read a byte from the serial port and returns
   a pass/fail response.  The byte is returned by reference.  The timeouts 
   are set in the driver to allow timing appropriate to receipt of bytes
   at 9600 baud.
*/

DWORD HW_SerialGetByte(BYTE *Value)
{
    DWORD count = 0; 

    // std::cout << "HW_SerialGetByte" << std::endl;

    // Check arguments
    if(fdSerialPort == 0) 
        return ErrorNoDevice();
    if(Value == NULL) 
        return ErrorNullPointer();
  
    // Read a single byte, with timeout provided by the driver
    count = read(fdSerialPort, Value, 1);
  
    // Return the count we recieved within the timeout (0 or 1 here)
    return count;
}

/*--------------------------------------------------------------------------*/
 
/* HW_SERIALSENDBYTE sends a byte over the serial port and returns a 
   pass/fail response based on results indicated by the driver.  In this 
   application strings are short, so the only probable failure here is
   either the port is not open or some other call error.  There are 
   timeouts set in the driver to help it negotiate a buffer nearly full.
*/

DWORD HW_SerialSendByte(BYTE *Value)
{
    DWORD count = 0;

    // std::cout << "HW_SerialSendByte" << std::endl;

    // Check arguments
    if(fdSerialPort == 0) 
        return ErrorNoDevice();
    if(Value == NULL) 
        return ErrorNullPointer();
  
    // Write the byte
    count = write(fdSerialPort, Value, 1);
  
    // Return the number of bytes written (0 or 1) as a status
    return count;
}

/*--------------------------------------------------------------------------*/

/* HW_SERIALCLEAR clears anything in the port buffer.  Generally, the buffer will be clear
   but could have something in it if the port is opened before the device starts or if 
   there is an incoming stream which there should not be.  This routine tries up to clear 
   a modest number of bytes from the buffer.  As soon as the buffer is clear it returns
   success even if more bytes are coming in.  Keep in mind that the read routing will
   wait for a moment if it does not see a byte right away, thus this routine can wait
   for a small stream to fully arrive before declaring itself done.
*/

DWORD HW_SerialClear(void)
{
    DWORD i;
    BYTE bogus;

    // std::cout << "HW_SerialClear" << std::endl;

    // Read up to 1000 bytes off the buffer
    for(i=0;i<1000;i++)
        if(!HW_SerialGetByte(&bogus)) return ICECAL_SUCCESS;
   
    // If we are still getting data something does not match
    // what is expected of this application
    return ICECAL_FAILURE;
}

/*--------------------------------------------------------------------------*/

/* HW_SEEKRESPONSE looks for a framed response from the IceCal unit and writes 
   it to the global response structure.  If a response was found then a 1
   is returned.  If there is no response, then a 0 is returned and nothing
   is written to the response structure.  The routine looks first for a 
   start of frame character, without which the routine immediately returns.
   If a frame is found, then a timeout loop is begun for the rest of the 
   frame.  Generally, a caller will place a call to this routine in its own 
   timeout loop.  A caller can assume that the response will be correctly 
   limited in terms of number of arguments and string characters.
*/

DWORD HW_SeekResponse(void)
{
    BYTE count = 0;
    BYTE dat = 0;
    BYTE timeout;
    char *ArgStr;

    // Look for a byte
    if(!HW_SerialGetByte(&dat)) 
	return ICECAL_FAILURE;

    // If this is not an STX we have no frame, so quit here
    if(dat != STX) 
	return ICECAL_FAILURE;
  
    // We have an STX, so look for a the response string
    // The string must be packed immediately following STX
    // Get characters until EOT, max reached, or timeout
    count = timeout = 0;
    do
    {
        if(HW_SerialGetByte(&dat)) 
        { 
            Response.String[count] = (char)dat; 
            count += 1; 
            timeout = 0; 
        }
        else
        {
            // SerialGetByte waits 2ms
            timeout += 1;
        }
    }
    while((dat != EOT) && (count <= MAX_COMMAND_CHARACTERS) && (timeout < 100));
    // Note: <= is correct, we are looking for up to MAX_CHARS and one EOT

    // If we did not get an EOT, ignore this string
    // Exit here as we have taken enough time on this
    if(dat != EOT) 
	return ICECAL_FAILURE;  

    // If we did get an EOT, we have a full frame
    // The EOT occupies the space after the command in the string
    // Complete the string with a null termination
    count -= 1;
    Response.String[count] = 0;  
  
    // There must be at least a 3 letter command word
    if(count < 3) 
	return ICECAL_FAILURE;

    // Test point
    // std::cout << "IceCal Response" << Response.String << std::endl;

    // If the response is NACK, there are no arguments, so stop here
    // However, this is otherwise a valid response
    if(!strcmp(Response.String, ICECAL_RESPONSE_NACK)) 
    {
        Response.NumArgs = 0;
        return ICECAL_SUCCESS;            
    } 

    // Seek out and put the arguments if any in the response structure
    Response.NumArgs = 0;
    ArgStr = &Response.String[3];
    while((*ArgStr != 0) && (Response.NumArgs < MAX_COMMAND_ARGUMENTS))
    { 
        // Get the argument value
        Response.Args[Response.NumArgs] = GetNumArg(&ArgStr);
        // If its not the end of the string, use the next character
        // This typically skips the comma in the string, but could
        // also skip other non-digit chracters
        if(*ArgStr != 0) ArgStr++;
        // There was an argument here, so count it
        Response.NumArgs += 1;
    }
 
    return ICECAL_SUCCESS;
} 
 
/*--------------------------------------------------------------------------*/
/* General Purpose And Helper Subroutines                                   */
/*--------------------------------------------------------------------------*/

/* GETNUMARG collects ASCII digits from the specified string until a non-digit
   such as a comma is reached.  The number is progressively translated into 
   a binary number.  The return value is the number developed.  On exit the 
   given pointer is updated to point to the location of the first non-digit
   found in the string.  The number developed is 16-bit, so if there is a
   need for larger arguments lots of design points will need to be revisited.

   Arguments could be signed, so the '-' character is also evaluated if it 
   is found in the first position.
*/

short GetNumArg(char **ArgStr)
{
    short value = 0;
    short sign = 1;
    short digit = 0;
    char *argptr = *ArgStr;

    // If the first digit is a negative sign, reverse the sign
    if(*argptr == '-') 
    {
        sign = -1;
        argptr++;
    }
 
    // Collect digits        
    while((*argptr >= '0') && (*argptr <= '9'))
    {
        // Get a digit and keep track of the value based on a decimal number
        digit = *argptr - '0';
        value *= 10;
        value += digit;    

        // Advance the argument string pointer
        argptr++;
    }

    // Update the passed in argument pointer
    *ArgStr = argptr;

  
    // Return the value
    return sign * value;     
}
 
/*--------------------------------------------------------------------------*/
 
/* ERRORNODEVICE takes care of informing the user that a command could not
   be completed because the port has not been opened.  A zero is returned
   for convenience.  
*/

DWORD ErrorNoDevice(void)
{
    std::cerr << "API Parameter Error: Device not properly identified"
              << std::endl;

    return ICECAL_FAILURE;
}

/*---------------------------------------------------------------------------*/

/* ERRORBADVALUE takes care of informing the user that a command could not
   be completed because the value specified in an argument is not reasonable. 
   A zero is returned for convenience.  
*/

DWORD ErrorBadValue(void)
{
    std::cerr << "API Parameter Error: Specified value is out of range"
              << std::endl;

    return ICECAL_FAILURE;
}

/*---------------------------------------------------------------------------*/

/* ERRORBADLENGTH takes care of informing the user that a command could not
   be completed because the requested transfer length is not valid. 
   A zero is returned for convenience.  
*/

DWORD ErrorBadLength(void)
{
    std::cerr << "API Parameter Error: Requested length is not valid"
              << std::endl;

    return ICECAL_FAILURE;
}

/*---------------------------------------------------------------------------*/

/* ERRORBADADDRESS takes care of informing the user that a command could not
   be completed because requested location(s) could not be reached. 
   A zero is returned for convenience.  
*/

DWORD ErrorBadAddress(void)
{
    std::cerr << "API Parameter Error: Bad offset or length given"
              << std::endl;

    return ICECAL_FAILURE;
}

/*---------------------------------------------------------------------------*/

/* ERRORNULLPOINTER takes care of informing the user that a command could not
   be completed because the specified transfer buffer is a NULL Pointer.
   A zero is returned for convenience.  
*/

DWORD ErrorNullPointer(void)
{
    std::cerr << "API Parameter Error: Data pointer provided is NULL"
              << std::endl;

    return ICECAL_FAILURE;
}

/*---------------------------------------------------------------------------*/

/* ERRORINTERNAL takes care of informing the user that a command could not
   be completed because the driver complained. A zero is returned for convenience.  
*/

DWORD ErrorInternal(void)
{
    std::cerr << "Device Communication Error: The driver has reported an error"
              << std::endl;

    return ICECAL_FAILURE;
}

/*--------------------------------------------------------------------------*/

/* END OF MODULE */

