/*---------------------------------------------------------------------------*/
/* ICECALAPI.H                                                               */
/*                                                                           */
/* Definitions for API.                                                      */
/*                                                                           */
/* Version 1.10                                                              */
/*                                                                           */
/* Copyright (C) 2009 Data Design Corporation                                */
/* All rights reserved.                                                      */
/*                                                                           */
/* Linux Port                                                                */
/* Author: Jonathan Wonders                                                  */
/* University of Maryland                                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#ifndef ICECALAPI_H
#define ICECALAPI_H

#include <stdint.h>

// Since this was originally written to be Windows-specific, we need to
// replace some of the OS-specific types.  The easiest and most flexible way
// to do this is through preprocessor macros.  A reference for the types is
// http://msdn.microsoft.com/en-us/library/aa383751%28v=vs.85%29.aspx
#define BOOL int32_t
#define BYTE uint8_t
#define WORD uint16_t
#define DWORD uint32_t
#define HANDLE void *

// Return values
#define ICECAL_SUCCESS 1
#define ICECAL_FAILURE 0
#define TRUE 1
#define FALSE 0

// This definition allows this header file to be used for export and import.
// Export names are defined in a .def file to be equivilent to the names 
// defined in this header file and not mangled.  Include the icecalapi.lib 
// file with the source files to link with a C project.
#define DCAPI extern "C" DWORD

/*---------------------------------------------------------------------------*/
/* API Exports                                                               */
/*---------------------------------------------------------------------------*/

// An open routine is needed because this device works with a 
// user selectable communication port
DWORD ICECAL_Open(char *SerialPortName);

// The API will automatically close the port on exit, but for good form the
// close routine should be called 
DWORD ICECAL_Close(void);

// Functions below relate directly to the ICD

// Get values are strings for display
// Argument is a return text string for which 20 characters should be sufficient
DCAPI ICECAL_GetSurfaceLineVoltage(char *Voltage);
DCAPI ICECAL_GetSurfaceProcessorTemperature(char *Temperature); 
DCAPI ICECAL_GetRemoteLineVoltage(char *Voltage);
DCAPI ICECAL_GetRemoteProcessorTemperature(char *Temperature); 

// Set control points per ICD
DCAPI ICECAL_SetPulseAttenuator(BYTE NewSetting);
DCAPI ICECAL_SetNoiseAttenuator(BYTE NewSetting);
DCAPI ICECAL_SetHighVoltageState(BYTE NewState);
DCAPI ICECAL_SetOutputState(BYTE NewState);

/*--------------------------------------------------------------------------*/

/* End of header file */

#endif // ICECALAPI_H
