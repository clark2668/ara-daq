#ifndef ATRI_USEFUL_DEFINES_H
#define ATRI_USEFUL_DEFINES_H


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
  DRSV9 = 2,  ///< DRSV9 is bit 2
  DRSV10 = 3  ///< DRSV10 is bit 3
} AtriDaughterType_t;

//% Enum defining daughter stack numbers.
typedef enum atriDaughterStack {
  D1 = 0, ///< Daughter 1 (bottom stack on ATRI)
  D2 = 1, ///< Daughter 2 (next-to-bottom stack on ATRI)
  D3 = 2, ///< Daughter 3 (next-to-top stack on ATRI)
  D4 = 3  ///< Daughter 4 (top stack on ATRI)
} AtriDaughterStack_t;

//% Mapping to help print out name of a daughter based on bit number
const char *atriDaughterTypeStrings[4] = { "DDA", "TDA", "DRSV9", "DRSV10" };
//% Mapping to help print out daughter stack based on bit number
const char *atriDaughterStackStrings[4] = { "D1", "D2", "D3" , "D4" };

//% Address of the hot-swap controller for each daughter based on bit number
const unsigned char atriHotSwapAddressMap[4] = {0xE4, 0xFC, 0x00, 0x00};
//% Address of the temperature sensor for each daughter based on bit number
const unsigned char atriTempAddressMap[4] = {0x90, 0x92, 0x00, 0x00};
//% Address of the EEPROM for each daughter board based on bit number
const unsigned char atriEepromAddressMap[4] = {0xA0, 0xA8, 0x00, 0x00};

//% I2C address bitmask for a write (do address | i2cWrite)
const unsigned char i2cWrite = 0x0;
//% I2C address bitmask for a read (do address | i2cRead)
const unsigned char i2cRead = 0x1;
//% I2C address for a DDA DAC (Vdly, Vadj)
const unsigned char ddaDacAddress = 0x18;
//% I2C address for a TDA DAC (ch0thresh, ch1thresh, ch2thresh, ch3thresh)
const unsigned char tdaDacAddress = 0x3E;
//% I2C address for a surface DAC
const unsigned char surfaceDacAddrss = 0x20;

//% Enum defining TDA DAC address bits.
typedef enum tdaDacAddress {
  tdaThresh0 = 0x00, ///< Threshold for CH0 (top channel on TDA)
  tdaThresh1 = 0x02, ///< Threshold for CH1 (next-to-top channel on TDA)
  tdaThresh2 = 0x03, ///< Threshold for CH2 (next-to-bottom channel on TDA)
  tdaThresh3 = 0x01  ///< Threshold for CH3 (bottom channel on TDA)
} TdaDacAddress_t;

//% Enum defining DDA DAC address bits.
typedef enum ddaDacAddress {
  ddaVdly = 0x00,    ///< Vdly : Wilkinson clock delay voltage
  ddaVadj = 0x01     ///< Vadj : Sampling speed control voltage
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

typedef struct {
  unsigned char hotSwapStatus;
  unsigned char identPage[64];
} AtriDaughterStatus_t;

#endif
