/* 
   ATRI Communication Library

   This library encapsulates control/monitoring functions for the
   ATRI firmware. It corresponds to the register map in v0.9. Note
   that it is NOT entirely compatible with older firmware versions
   due to the remapping of the IRS control registers and the
   addition of the trigger control registers.

   rjn@hep.ucl.ac.uk, August 2011
   allison.122@osu.edu, August 2012
*/

#ifndef ATRI_COM_H
#define ATRI_COM_H
#include "atriDefines.h"

#define ATRI_I2C_IRC_CLOCK_ADDRESS 0x18


// Packet controller errors.
typedef enum atriPacketErrorCode {
  PC_ERR_RESET = 0x00,
  PC_ERR_BAD_PACKET = 0xFF,
  PC_ERR_BAD_DEST = 0xFE,
  PC_ERR_I2C_FULL = 0xFD
} AtriPacketErrorCode_t;

//0x70-0x73 are D1DEAD, D2DEAD, D3DEAD, D4DEAD. 8-bit measures of
//deadtime (multiply by 4096, divide by 1e6).
//0x74-0x77 are D1OCCU, D2OCCU, D3OCCU, D4OCCU. Average occupancy over
//the last 16 milliseconds.
//0x78-0x7B are D1MAX, D2MAX, D3MAX, D4MAX. Maximum occupancy seen in
//the previous second.


typedef enum atriWishboneAddress {
  ATRI_WISH_IDREG=0x0000,
  ATRI_WISH_VERREG=0x0004,
  ATRI_WISH_DCM=0x0008,
  ATRI_WISH_D1CTRL=0x0010,
  ATRI_WISH_D2CTRL=0x0011,
  ATRI_WISH_D3CTRL=0x0012,
  ATRI_WISH_D4CTRL=0x0013,
  ATRI_WISH_IRSEN=0x0020,
  ATRI_WISH_RDDELAY=0x0022,
  // 0x22/0x23 were WILKMONEN/TSAMONEN. Temporarily reserved.
  ATRI_WISH_IRSPEDEN=0x0024,
  ATRI_WISH_IRSPED=0x0026,
  // 0x28 was TRGEN. This is moved to a trigger control block.
  // 0x28 is now the IRS channel mask pointer,
  // and 0x29 is now the IRS channel mask value.
  ATRI_WISH_IRSMASKPTR=0x0028,
  ATRI_WISH_IRSMASK=0x0029,
  // ATRI_WISH_SOFTTRIG now contains the software
  // trigger flag generation (bit 0), and the
  // repetition count (bits 3:1, for up to 8
  // triggers) and the delay (bits 7:4) -
  // 0000 = 10 microseconds
  // 0001 = 100 microseconds
  // 0010 = 1 millisecond
  // 0011 = 5 milliseconds
  // 0100 = 10 milliseconds
  // 0101 = 20 milliseconds
  // 0110 = 40 milliseconds
  // 0111 = 80 milliseconds
  // 1000 = 100 milliseconds
  // 1001 = 200 milliseconds
  // 1010 = 400 milliseconds
  // 1011 = 800 milliseconds
  // 1100 = 1.6 seconds
  // 1101 = 3.2 seconds
  // 1110 = 6.4 seconds
  // 1111 = 12.8 seconds
  ATRI_WISH_SOFTTRIG=0x002A,
  // ATRI_WISH_SOFTINFO is the software trigger info.
  ATRI_WISH_SOFTINFO=0x002B,
  // 2C->3A are reserved for Wilkinson and sampling monitoring.
  ATRI_WISH_D1WILK=0x002C,
  ATRI_WISH_D1DELAY=0x002E,
  ATRI_WISH_D2WILK=0x0030,
  ATRI_WISH_D2DELAY=0x0032,
  ATRI_WISH_D3WILK=0x0034,
  ATRI_WISH_D3DELAY=0x0036,
  ATRI_WISH_D4WILK=0x0038,
  ATRI_WISH_D4DELAY=0x003A,
  // 0x003C and 0x003E are currently reserved.
  // They're intended to allow manipulation of IRS3B registers
  // They will also probably manipulate Wilkinson conversion.
  ATRI_WISH_IRS3REGPTR=0x003C,

  ATRI_WISH_IRS3REG=0x003E,

  // Somehow these were backwards? PPS count is 0x44, clock counter is 0x40.
  ATRI_WISH_CLKCNT=0x0040,
  ATRI_WISH_PPSCNT=0x0044,
  ATRI_WISH_PPSCTRL=0x0048,

  ATRI_WISH_TTRIG_MAJOR=0x004C, //< Major time (20480 ns intervals) of timed trigger.
  ATRI_WISH_TTRIG_MINOR=0x004E, //< Minor time (80 ns intervals) of timed trigger
  ATRI_WISH_TTRIG_CTRL=0x004F,  //<  Timed trigger control. Bit 0: enable if set. Bit1: if set, trigger every second. If not set, one-shot trigger.


  // Slave 5 (0x50-0x5F) is reserved (for timed/cal trigger)
  // Slave 6 is trigger control.
  ATRI_WISH_TRIGVER=0x0060,        //%< Trigger version register (0x60-0x64)
  ATRI_WISH_L1MASK=0x0064,         //%< L1 mask (0x64-0x67).
  ATRI_WISH_L2MASK=0x0068,         //%< L2 mask (0x68-0x6B).
  ATRI_WISH_L3MASK=0x006C,         //%< L3 mask (0x6C-0x6D).
  ATRI_WISH_L4MASK=0x006E,         //%< L4 mask (0x6E-0x6F).
  ATRI_WISH_RF0BLOCKS=0x0070,      //%< Number of blocks for L4_RF0
  ATRI_WISH_RF0PRETRIG=0x0071,     //%< Number of pretrigger cycles for L4_RF0
  ATRI_WISH_RF1BLOCKS=0x0072,      //%< Number of blocks for L4_RF1
  ATRI_WISH_RF1PRETRIG=0x0073,     //%< Number of pretrigger cycles for L4_RF1
  ATRI_WISH_CPUBLOCKS=0x0074,      //%< Number of blocks for L4_CPU
  ATRI_WISH_CPUPRETRIG=0x0075,     //%< Number of pretrigger cycles for L4_CPU
  ATRI_WISH_CALBLOCKS=0x0076,      //%< Number of blocks for L4_CAL
  ATRI_WISH_CALPRETRIG=0x0077,     //%< Number of pretrigger cycles for L4_CAL
  ATRI_WISH_EXTBLOCKS=0x0078,      //%< Number of blocks for L4_EXT
  ATRI_WISH_EXTPRETRIG=0x0079,      //%< Number of pretrigger cycles for L4_EXT
  
  ATRI_WISH_TRIG_DELAY_NUM=0x007A,  //%< The L1 Scaler number to delay
  ATRI_WISH_TRIG_DELAY_VALUE=0x007B,  //%< The amount to delays the L1 Scaler by in 10ns
  
  ATRI_WISH_L1WINDOW=0x007E,        //%< Width of window in 10ns
  ATRI_WISH_TRIGCTL=0x007F,        //%< Master trigger control

  ATRI_WISH_EVERROR=0x0080,        //%< Error code for event readout
  ATRI_WISH_EVCOUNTAVG=0x0082,     //%< Average number of bytes available in event count FIFO
  ATRI_WISH_EVCOUNTMIN=0x0084,        //%< Minimum number of bytes available in event count FIFO
  ATRI_WISH_BLKCOUNTAVG=0x0086,        //%< Average number of blocks in the block buffer
  ATRI_WISH_BLKCOUNTMAX=0x0088,        //%< Minimum number of blocks in the block buffer
  ATRI_WISH_DIGDEADTIME=0x008A,        //%< Deadtime originating from digitisation in miliseconds, pre-scale 16

  ATRI_WISH_READERR=0x0080,        //%< Readout error.
  ATRI_WISH_BUFFER_SPACE_AVG=0x0082, //% Avg. buffer space remaining
  ATRI_WISH_BUFFER_SPACE_MIN=0x0084, //% Minimum buffer space remaining
  ATRI_WISH_BLOCK_COUNT_AVG=0x0086,  //% Avg. blocks pending
  ATRI_WISH_BLOCK_COUNT_MAX=0x0088,  //% Maximum blocks pending
  ATRI_WISH_IRS_DEADTIME=0x008A,     //% Deadtime due to IRS readout
  ATRI_WISH_USB_DEADTIME=0x008C,     //% Deadtime due to no buffer space (USB)
  ATRI_WISH_TOT_DEADTIME=0x008E,     //% Total (combined) deadtime
  // 0x0090-0x00FF are reserved (unassigned)

  // L1 scalers. These need to be renamed.
  ATRI_WISH_SCAL_L1_START=0x0100,

  ATRI_WISH_SCAL_D1_L1_C1=0x0100,
  ATRI_WISH_SCAL_D1_L1_C2=0x0102,
  ATRI_WISH_SCAL_D1_L1_C3=0x0104,
  ATRI_WISH_SCAL_D1_L1_C4=0x0106,
  ATRI_WISH_SCAL_D2_L1_C1=0x0108,
  ATRI_WISH_SCAL_D2_L1_C2=0x010A,
  ATRI_WISH_SCAL_D2_L1_C3=0x010C,
  ATRI_WISH_SCAL_D2_L1_C4=0x010E,
  ATRI_WISH_SCAL_D3_L1_C1=0x0110,
  ATRI_WISH_SCAL_D3_L1_C2=0x0112,
  ATRI_WISH_SCAL_D3_L1_C3=0x0114,
  ATRI_WISH_SCAL_D3_L1_C4=0x0116,
  ATRI_WISH_SCAL_D4_L1_C1=0x0118,
  ATRI_WISH_SCAL_D4_L1_C2=0x011A,
  ATRI_WISH_SCAL_D4_L1_C3=0x011C,
  ATRI_WISH_SCAL_D4_L1_C4=0x011E,

  // Surface scalers. These are just L1s.
  ATRI_WISH_SURF_SCAL_L1_START=0x120,

  ATRI_WISH_SURF_SCAL_L1_C1=0x120,
  ATRI_WISH_SURF_SCAL_L1_C2=0x122,
  ATRI_WISH_SURF_SCAL_L1_C3=0x124,
  ATRI_WISH_SURF_SCAL_L1_C4=0x126,

  // L2 scalers. These need to be renamed.
  ATRI_WISH_SCAL_L2_START=0x0140,

  /* ATRI_WISH_SCAL_P12V_1_OF_4=0x0140, */
  /* ATRI_WISH_SCAL_P12V_2_OF_4=0x0142, */
  /* ATRI_WISH_SCAL_P12V_3_OF_4=0x0144, */
  /* ATRI_WISH_SCAL_P12V_SPARE_1=0x0146, */
  /* ATRI_WISH_SCAL_P12V_1_OF_4_COPY=0x0148, */
  /* ATRI_WISH_SCAL_P12V_2_OF_4_COPY=0x014A, */
  /* ATRI_WISH_SCAL_P12V_3_OF_4_COPY=0x014C, */
  /* ATRI_WISH_SCAL_P12V_SPARE_2=0x014E, */
  /* ATRI_WISH_SCAL_P12H_1_OF_4=0x0150, */
  /* ATRI_WISH_SCAL_P12H_2_OF_4=0x0152, */
  /* ATRI_WISH_SCAL_P12H_3_OF_4=0x0154, */
  /* ATRI_WISH_SCAL_P12H_SPARE_1=0x0156, */
  /* ATRI_WISH_SCAL_P12H_1_OF_4_COPY=0x0158, */
  /* ATRI_WISH_SCAL_P12H_2_OF_4_COPY=0x015A, */
  /* ATRI_WISH_SCAL_P12H_3_OF_4_COPY=0x015C, */
  /* ATRI_WISH_SCAL_P12H_SPARE_2=0x015E, */


  /* ATRI_WISH_SCAL_P34V_1_OF_4=0x0160, */
  /* ATRI_WISH_SCAL_P34V_2_OF_4=0x0162, */
  /* ATRI_WISH_SCAL_P34V_3_OF_4=0x0164, */
  /* ATRI_WISH_SCAL_P34V_SPARE_1=0x0166, */
  /* ATRI_WISH_SCAL_P34V_1_OF_4_COPY=0x0168, */
  /* ATRI_WISH_SCAL_P34V_2_OF_4_COPY=0x016A, */
  /* ATRI_WISH_SCAL_P34V_3_OF_4_COPY=0x016C, */
  /* ATRI_WISH_SCAL_P34V_SPARE_2=0x016E, */
  /* ATRI_WISH_SCAL_P34H_1_OF_4=0x0170, */
  /* ATRI_WISH_SCAL_P34H_2_OF_4=0x0172, */
  /* ATRI_WISH_SCAL_P34H_3_OF_4=0x0174, */
  /* ATRI_WISH_SCAL_P34H_SPARE_1=0x0176, */
  /* ATRI_WISH_SCAL_P34H_1_OF_4_COPY=0x0178, */
  /* ATRI_WISH_SCAL_P34H_2_OF_4_COPY=0x017A, */
  /* ATRI_WISH_SCAL_P34H_3_OF_4_COPY=0x017C, */
  /* ATRI_WISH_SCAL_P34H_SPARE_2=0x017E, */


  // L3 scalers.
  ATRI_WISH_SCAL_L3_START=0x0180,

  ATRI_WISH_SCAL_L3_0=0x180,
  ATRI_WISH_SCAL_L3_1=0x182,
  ATRI_WISH_SCAL_L3_2=0x184,
  ATRI_WISH_SCAL_L3_3=0x186,
  ATRI_WISH_SCAL_L3_4=0x188,
  ATRI_WISH_SCAL_L3_5=0x18A,
  ATRI_WISH_SCAL_L3_6=0x18C,
  ATRI_WISH_SCAL_L3_7=0x18E,

  // L4 scalers.
  ATRI_WISH_SCAL_L4_START=0x01A0,

  ATRI_WISH_SCAL_L4_0=0x1A0,
  ATRI_WISH_SCAL_L4_1=0x1A2,
  ATRI_WISH_SCAL_L4_2=0x1A4,
  ATRI_WISH_SCAL_L4_3=0x1A6,

  // T1 scaler.
  ATRI_WISH_SCAL_T1_START=0x01B0,

  ATRI_WISH_SCAL_T1=0x1B0,

  ATRI_WISH_BLOCKSCALER=0x01B8,   ///<Digitization block (20 ns) scaler
  ATRI_WISH_L1SCALGATE=0x01C0,    ///<Gated L1 scaler (read) and select (write)
  ATRI_WISH_L2SCALGATE=0x01C2,    ///<Gated L2 scaler (read) and select (write)
  ATRI_WISH_L3SCALGATE=0x01C4,    ///<Gated L3 scaler (read) and select (write)
  ATRI_WISH_L4SCALGATE=0x01C6,    ///<Gated L4 scaler (read) and select (write)

  ATRI_WISH_GATEWINDOW=0x01C8    ///<Width of the gate started by ext. trigger (in 10 ns units)
  

} AtriWishboneAddress_t;



typedef enum daughterSwapStatusBits {
  bmADC_OC = 0x1,        ///< An ADC conversion went over current
  bmADC_ALERT = 0x2,     ///< ALERT bit indicating ADC over current
  bmHS_OC = 0x4,         ///< The last hot-swap was aborted due to over current
  bmHS_ALERT = 0x8,      ///< ALERT bit indicating hot swap failure
  bmOFF_STATUS = 0x10,   ///< The (inverted) status of the ON bit
  bmOFF_ALERT = 0x20     ///< ALERT bit indicating ON pin or SWOFF caused alert
} DaughterSwapStatusBits_t;

//% Commands to/from I2C controller.
typedef enum atriI2CType {
  I2C_DIRECT = 0 ///< Tell I2C controller exactly what I2C commands to send
} AtriI2CType_t;

//% WISHBONE actions
typedef enum atriWishboneType {
  WB_READ = 0,   ///< WISHBONE read
  WB_WRITE = 1   ///< WISHBONE write
} AtriWishboneType_t;

//% Enum defining ATRI daughter types
typedef enum atriDaughterType {
  DDA = 0,    ///< DDA is bit 0
  TDA = 1,    ///< TDA is bit 1
  STDA = 2,  ///<  STDA is bit 2
  DRSV10 = 3  ///< DRSV10 is bit 3
} AtriDaughterType_t;

//% Enum defining daughter stack numbers.
typedef enum atriDaughterStack {
  D1 = 0, ///< Daughter 1 (bottom stack on ATRI)
  D2 = 1, ///< Daughter 2 (next-to-bottom stack on ATRI)
  D3 = 2, ///< Daughter 3 (next-to-top stack on ATRI)
  D4 = 3  ///< Daughter 4 (top stack on ATRI)
} AtriDaughterStack_t;

//% Enum defining TDA DAC address bits.
typedef enum tdaDacAddress {
  tdaThresh0 = 0x00, ///< Threshold for CH0 (top channel on TDA)
  tdaThresh1 = 0x02, ///< Threshold for CH1 (next-to-top channel on TDA)
  tdaThresh2 = 0x03, ///< Threshold for CH2 (next-to-bottom channel on TDA)
  tdaThresh3 = 0x01  ///< Threshold for CH3 (bottom channel on TDA)
} TdaDacAddress_t;

//% Enum defining SURFACE DAC address bits.
typedef enum surfaceDacAddress {
  surfaceThresh0 = 0x00, ///< Threshold for CH0 (top channel on SURFACE)
  surfaceThresh1 = 0x02, ///< Threshold for CH1 (next-to-top channel on SURFACE)
  surfaceThresh2 = 0x03, ///< Threshold for CH2 (next-to-bottom channel on SURFACE)
  surfaceThresh3 = 0x01  ///< Threshold for CH3 (bottom channel on SURFACE)
} SurfaceDacAddress_t;

//% Enum defining DDA DAC address bits.
typedef enum ddaDacAddress {
  ddaVdly = 0x00,    ///< Vdly : Wilkinson clock delay voltage
  ddaVadj = 0x01,    ///< Vadj : Sampling speed control voltage
  ddaVbias = 0x02,   ///< Vbias : Sample->storage control voltage
  ddaIsel = 0x03
} DdaDacAddress_t;

//% Simple function for converting nanoDAC values to voltage
static inline float dacToVolts(unsigned short Vdly) {
  return (Vdly*(2.5/65535.));
}

//% Enum defining DAC command (same for all nanoDAC DACs - TDA, DDA)
typedef enum dacCommand {
  dacWriteOnly = 0x00,      ///< Write DAC value, but don't update output
  dacUpdateOnly = 0x08,     ///< Update output only
  dacWriteUpdateAll = 0x10, ///< Write DAC value, update all outputs
  dacWriteUpdateOne = 0x18, ///< Write and update individual DAC output
  dacPowerUpDown = 0x20,    ///< Power up or down the DAC
  dacReset = 0x21,          ///< Reset the DAC
  dacLdacSetup = 0x30,      ///< Set up the LDAC register
  dacReferenceSetup = 0x38  ///< Set up the internal reference
} DacCommand_t;

//% Enum defining IRS modes.
typedef enum irsMode {
  irsMode1 = 0x00,          ///< Use interface for IRS1 & 2
  irsMode2 = 0x01           ///< Use interface for IRS3B
} IrsMode_t;

//% Enum defining the L4 triggers. THIS SHOULD PROBABLY BE MOVED ELSEWHERE
typedef enum triggerL4 {
  triggerL4_RF0 = 0,                ///< Normal RF (deep) trigger
  triggerL4_RF1 = 1,                ///< Secondary RF (surface) trigger
  triggerL4_CPU = 2,                ///< CPU (soft) trigger
  triggerL4_CAL = 3,                 ///< Cal (timed) trigger
  triggerL4_EXT = 4                 ///< Ext (external) trigger

} TriggerL4_t;

//% Enum defining pedestal modes.
typedef enum pedMode {
  pedModeNone = 0x00,               ///< Pedestal mode is OFF
  pedModeAutoAdvance = 0x01,        ///< Ped addr increments on each sample
  pedModeAutoTrig = 0x02,           ///< Soft trig generated for each sample
  pedModeAutoAdvanceTrig = 0x03     ///< Ped addr increments, and soft trig
} PedMode_t;

//% Enum defining bits in the TRIGCTL register
typedef enum trigctlMask {
  trigctlDbMask = 0x03,
  trigctlT1Mask = 0x10,
  trigctlResetMask = 0x80
} TrigctlMask_t;

//% Enum defining bits in the IRSEN register (should be called IRSCTL)
typedef enum irsctlMask {
  irsctlEnableMask = 0x01,
  irsctlResetMask = 0x02,
  irsctlResetAckMask = 0x04,
  irsctlTestModeMask = 0x08
} IrsctlMask_t;

typedef struct {
  unsigned char hotSwapStatus;
  unsigned char identPage[64];
} AtriDaughterStatus_t;

typedef struct {
  unsigned char error;
  unsigned char unused;
  unsigned short bufspace_avg;
  unsigned short bufspace_min;
  unsigned short blocks_avg;
  unsigned short blocks_min;
  unsigned short irs_deadtime;
  unsigned short usb_deadtime;
  unsigned short tot_deadtime;
} AtriEventReadoutStatistics_t;

#ifndef ATRI_COM
//% Mapping to help print out name of a daughter based on bit number
extern const char *atriDaughterTypeStrings[4];
//% Mapping to help print out daughter stack based on bit number
extern const char *atriDaughterStackStrings[4];

//% Address of the hot-swap controller for each daughter based on bit number
extern const unsigned char atriHotSwapAddressMap[4];
//% Address of the temperature sensor for each daughter based on bit number
extern const unsigned char atriTempAddressMap[4];
//% Address of the EEPROM for each daughter board based on bit number
extern const unsigned char atriEepromAddressMap[4];
#endif

typedef enum i2cMasks {
  i2cWrite = 0x0,
  i2cRead = 0x1
} I2cMask_t;

typedef enum dacAddresses {
  ddaDacAddress = 0x18,
  tdaDacAddress = 0x3E,
  surfaceDacAddress = 0x20,
  ddaPedDacAddress = 0x1C
} DacAddress_t;


typedef enum atriDigitizerType {
  digitizerType_IRS2 = 0,
  digitizerType_IRS3 = 1
} AtriDigitizerType_t;


//Underlying socket functions
int openConnectionToAtriControlSocket(); ///<Returns fd
int sendControlPacketToAtri(int fAtriControlSockFd,AtriControlPacket_t *pktPtr);
int readResponsePacketFromAtri(int fAtriControlSockFd,AtriControlPacket_t *pktPtr);
int closeConnectionToAtriControlSocket(int fAtriControlSockFd);


//Useful wrappers to read write to the wishbone bus and I2C
int writeToAtriI2CRegister(int fAtriControlSockFd,AtriDaughterStack_t stack,uint8_t i2cAddress, uint8_t reg, uint8_t value);
int writeToAtriI2C(int fAtriControlSockFd,AtriDaughterStack_t stack, uint8_t i2cAddress, uint8_t length, uint8_t *data);
int readFromAtriI2C(int fAtriControlSockFd,AtriDaughterStack_t stack, uint8_t i2cAddress, uint8_t length, uint8_t *data);
int atriWishboneRead(int fAtriControlSockFd,uint16_t wishboneAddress,uint8_t length, uint8_t *data);
int atriWishboneWrite(int fAtriControlSockFd,uint16_t wishboneAddress,uint8_t length, uint8_t *data);

//Utility functions that do something

/** \brief Turns on an ATRI daughter.
 *
 * Turns on an ATRI daughterboard, verifies that it has turned on,
 * and reads out the first page of the EEPROM for identification.
 *
 */
int atriDaughterPowerOn(int fAtriControlSockFd,AtriDaughterStack_t stack, AtriDaughterType_t type, AtriDaughterStatus_t *status);

/** \brief Drives the outputs for an ATRI daughter.
 *
 * When a daughterboard is first inserted, outputs that go to that
 * daughterboards are undriven, and the daughterboard is unpowered.
 * After the daughterboard is powered on, the outputs to that
 * daughterboard stay undriven until this function is called. The
 * only outputs that are active without this function being called
 * are the I2C outputs.
 *
 * (Note that the triggering daughterboards don't really need
 *  this function called, since the FPGA has no outputs that go
 *  to the daughterboards.)
 *
 * The drive bit is typically set inside atriDaughterPowerOn automatically.
 */
int atriSetDaughterDrive(int fAtriControlSockFd, AtriDaughterStack_t stack,
			 AtriDaughterType_t type, int drive);

/** \brief Gets ATRI daughter status.
 * 
 * Gets ATRI daughter status. Returns 32-bit integer, with daughter 4
 * in the MSB, and daughter 1 in the LSB. Can use an existing
 * controlPacket/responsePacket or its own (and malloc/free) if pointer
 * is NULL.
 *
 * Blocks waiting for a response from atri_control socket.
 * \param fd Socket for atri_control.
 * \param controlPacket Pointer to existing AtriControlPacket_t, or NULL.
 * \param responsePacket Pointer to existing AtriControlPacket_t, or NULL.
 */
unsigned int atriGetDaughterStatus(int fAtriControlSockFd);

/** \brief Sets the DAC values for controlling the clock and sampling speed
 * 
 * Sets the two DAC values for clock control and sampling spped
 *
 * Blocks waiting for a response from atri_control socket.
 * \param stack Daughter board stack (0-3)
 * \param vdlyDac DAC to control Vdly, default value 0xB852
 * \param vadjDac DAC to control Vadj, default value 0x4600
 */
int setIRSClockDACValues(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vdlyDac, uint16_t vadjDac);

/** \brief Sets the Vadj DAC value
 * 
 * Sets the Vadj DAC value for clock control and sampling spped
 *
 * Blocks waiting for a response from atri_control socket.
 * \param stack Daughter board stack (0-3)
 * \param vadjDac DAC to control Vadj, default value 0x4600
 */
int setIRSVadjDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vadjDac);


/** \brief Sets the Vdly DAC value
 * 
 * Sets the Vdly DAC value for clock control and sampling spped
 *
 * Blocks waiting for a response from atri_control socket.
 * \param stack Daughter board stack (0-3)
 * \param vdlyDac DAC to control Vdly, default value 0x4600
 */
int setIRSVdlyDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vdlyDac);

/** \brief Sets the Vbias DAC value
 * 
 * Sets the Vbias DAC value for sample->storage bias.
 *
 * Blocks waiting for a response from atri_control socket.
 * \param stack Daughter board stack (0-3)
 * \param vdlyDac DAC to control Vbias, default value 0.6 V
 */
int setIRSVbiasDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t vbiasDac);

/** \brief Sets the Isel DAC value
 * 
* Sets the Isel DAC value for Wilkinson ramp current.
 *
 * Blocks waiting for a response from atri_control socket.
 * \param stack Daughter board stack (0-3)
 * \param vdlyDac DAC to control Isel, default value 39000
 */
int setIRSIselDACValue(int fAtriControlSockFd,AtriDaughterStack_t stack, uint16_t iselDac);

/** \brief Sets the threshold values
 * 
 * Sets the trigger threshold values
 * Blocks waiting for a response from atri_control socket.
 * \param fAtriControlSockFd The file descriptor for the control socket
 * \param stack Daughter board stack (0-3)
 * \param thresholds Array of 4 threshold values
 */
int setAtriThresholds(int fAtriControlSockFd,AtriDaughterStack_t stack,  uint16_t *thresholds);



/** \brief Sets the threshold values
 * 
 * Sets the trigger threshold values for the surface board
 * Blocks waiting for a response from atri_control socket.
 * \param fAtriControlSockFd The file descriptor for the control socket
 * \param stack Daughter board stack (0-3)
 * \param thresholds Array of 4 threshold values
 */
int setSurfaceAtriThresholds(int fAtriControlSockFd, AtriDaughterStack_t stack,  uint16_t *thresholds);

/** \brief ATRI trigger/digitizer reset.
 *
 * Resets both the ATRI trigger and digitizer subsystem.
 * Triggers are first masked, then the trigger subsystem is
 * placed into reset, then the digitizer subsystem is put into
 * reset. Then the digitizer subsystem is taken out of reset,
 * and finally the trigger subsystem is taken out of reset.
 *
 * Note that both the trigger and digitizer subsystems
 * come up disabled, so they will have to be enabled after this.
 *
 * \param fAtriControlSockFd The file descriptor for the control socket
 *
 */
int resetAtriTriggerAndDigitizer(int fAtriControlSockFd);

/** \brief Select which stack should point to the L1[19:16] triggers.
 *
 * L1[19:16] - the surface triggers - are connected to one
 * of the 4 stacks. Which stack it is connected
 * to can be selected here.
 * 
 * \param fAtriControlSockFd The file descriptor for the control socket
 * \param stack Stack to connect L1[19:16] to.
 */
int setAtriL1HighDb(int fAtriControlSockFd, AtriDaughterStack_t stack);

/** \brief Set the L1 mask.
 *
 * Sets the (currently 20-bit) L1 mask. L1 triggers are the lowest
 * level single-antenna triggers.
 *
 * \param fAtriControlSockFd The file descriptor to the control socket
 * \param mask The 20-bit L1 mask (LSB=L1[0], bit 20=L1[19])
 */
int setTriggerL1Mask(int fAtriControlSockFd, int mask);

/** \brief Set the L2 mask.
 *
 * Sets the (currently 16-bit) L2 mask. L2 triggers are combinations
 * of L1 triggers on the same daughterboard.
 *
 * \param fAtriControlSockFd The file descriptor to the control socket
 * \param mask The 16-bit L2 mask (LSB=L2[0], bit 16=L2[15])
 */
int setTriggerL2Mask(int fAtriControlSockFd, int mask);

/** \brief Set the L3 mask.
 *
 * Sets the (currently 8-bit) L3 mask. L3 triggers are combinations
 * of L2 triggers.
 *
 * \param fAtriControlSockFd The file descriptor to the control socket
 * \param mask The 8-bit L3 mask (LSB=L3[0], bit 8=L3[7])
 */
int setTriggerL3Mask(int fAtriControlSockFd, int mask);

/** \brief Set the L4 mask.
 *
 * Sets the (currently 4-bit) L4 mask. L4 triggers are top-level
 * triggers (the OR of all L4s becomes a T1 event).
 *
 * \param fAtriControlSockFd The file descriptor to the control socket
 * \param mask The 4-bit L4 mask. See TriggerL4_t for what bits are what.
 */
int setTriggerL4Mask(int fAtriControlSockFd, int mask);

/** \brief Set the T1 mask.
 *
 * Sets or unsets the T1 mask. Setting this disables all triggers.
 * Note that the T1 mask comes up set (i.e. no triggers) after a reset.
 *
 * \param fAtriControlSockFd The file descriptor to the control socket
 * \param mask The T1 mask. Only the LSB is used.
 */
int setTriggerT1Mask(int fAtriControlSockFd, int mask);

/** \brief Enables/disables the digitizer subsystem.
 *
 * Sets/unsets the low bit of IRSCTL[15:0] (enable).
 */
int setIRSEnable(int fAtriControlSockFd, int enable);

/** \brief Sets up IRS mode for a daughter.
 *
 * Sets the IRS mode (switch between IRS1/2 and IRS3B modes)
 */
int setIRSMode(int fAtriControlSockFd, AtriDaughterStack_t stack,
	       IrsMode_t mode);

/** \brief Enables/disables pedestal mode.
 *
 */
int setPedestalMode(int fAtriControlSockFd, PedMode_t mode);

/** \brief Sets the number of blocks for an L4 trigger
 *
 *
 */
int atriSetTriggerNumBlocks(int fAtriControlSockFd, TriggerL4_t ttype,
			    unsigned char nblocks);

/** \brief Sets the number of pre-trigger blocks for an L4 trigger
 *
 *
 */
int atriSetNumPretrigBlocks(int fAtriControlSockFd, TriggerL4_t ttype,
			    unsigned char nblocks);

//void getLastControlPacket(AtriControlPacket_t *pktPtr);
//void getLastResponsePacket(AtriControlPacket_t *rspPtr);

/** \brief Gets the digitizer type currently installed on a DDA.
 *
 */
int atriGetDigitizerType(int fAtriControlSockFd, AtriDaughterStack_t stack,
			 AtriDigitizerType_t *type);

/** \brief Sets the number of blocks, minus 1, to read out for an L4 type.
 *
 */
int atriSetTriggerNumBlocks(int fAtriControlSockFd, TriggerL4_t ttype,
			    unsigned char nblocks);

/** \brief Sets the number of pretrigger blocks for an L4 type.
 *
 * Note: a brief explanation is in order here. This parameter essentially
 * sets the location of the trigger in the middle of the readout window.
 * If this number is set to 0, then the trigger would appear in the first
 * block read out (nominally). If this number is set to 10, the trigger
 * would appear in the 10th block read out.
 *
 * nblocks can only be from 0 to 15 in current firmware. It can be expanded
 * to 0-31 by changing PRETRG_BITS to 5 in trigger_defs.vh.
 */
int atriSetTriggerNumPretrigBlocks(int fAtriControlSockFd, TriggerL4_t ttype,
				   unsigned char nblocks);

/** \brief Converts the Wilkinson counter raw value to nanoseconds.
 *
 */
unsigned int atriConvertWilkCounterToNs(unsigned short rawValue);

/** \brief sets the delay between sampling and digitisation.
 *
 *  The number of clock cycles delay = 4*delay+8
 *  e.g. the default delay = 18 -> 18*4+8 = 80 clock cycles
 *  The clock used is 100MHz, so default delay ns = 800ns
 *
 */
int atriSetReadoutDelay(int fAtriControlSockFd, uint8_t delay);


/** \brief Reads event readout statistics.
 *
 */
int atriGetEventReadoutStatistics(int fAtriControlSockFd,
				  AtriEventReadoutStatistics_t *stat);


int atriSetTriggerDelays(int fAtriControlSockFd, uint16_t *triggerDelays);

int atriSetTriggerWindow(int fAtriControlSockFd, uint16_t windowSize);

#endif // ATRI_COM_H


