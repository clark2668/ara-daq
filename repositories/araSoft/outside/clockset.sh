#!/bin/bash

# Turn on the FPGA oscillator.
sendFx2ControlPacketHex.sh 0xB6 0x40 0x40 0x00
# If we initialize to the external Rubidium clock, we instead
# would disable the FPGA oscillator.


I2CRD="sendFx2ControlPacketHex.sh 0xB4 0xC0 0xD1 0x00 "
I2CWR="sendFx2ControlPacketHex.sh 0xB4 0x40 0xD0 0x00 "

# Register 2: BWSEL_REG[3:0], 0010. BWSEL = 0000.
$I2CWR 2 0x02

# Register 3: CKSEL_REG[1:0], DHOLD, SQ_ICAL, 0101. CKSEL_REG = 10: clock 3 (onboard clock)
$I2CWR 3 0x85
# If we initialize to the external Rubidium clock, we instead do
# $I2CWR 3 0x05

# Register 5: CKOUT2/CKOUT1 are LVDS
$I2CWR 5 0x3F
# Register 6: CKOUT4/CKOUT3 are LVDS
$I2CWR 6 0x3F
# Register 7: CKOUT5 is LVDS. CKIN3 is frequency offset alarms (who cares)
$I2CWR 7 0x3B
# Register 11: Power down CKIN4 input buffer.
$I2CWR 11 0x48
# Register 21: Ignore INC/DEC/FSYNC_ALIGN, tristate CKx_ACTV, ignore CKSEL
$I2CWR 21 0x00

# These determine output clock frequencies.
# CKOUTx is 5.6 GHz divided by N1_HS divided by NCx_LS
# CKOUT1 = D2REFCLK
# CKOUT2 = D3REFCLK
# CKOUT3 = D1REFCLK
# CKOUT4 = D4REFCLK
# CKOUT5 = FPGA_REFCLK

# Right now all daughter refclks are 25 MHz, and the FPGA refclk is 100 MHz.
# This should probably be changed so that the daughter refclks are at least
# 100 MHz.

# Register 25-27: N1_HS=000 (N1=4), NC1_LS = 0x37 (56)
$I2CWR 25 0x00
$I2CWR 26 0x00
$I2CWR 27 0x37
# Register 28-30: NC2_LS = 0x37 (56)
$I2CWR 28 0x00
$I2CWR 29 0x00
$I2CWR 30 0x37
# Register 31-33: NC3_LS = 0x37 (56) 
$I2CWR 31 0x00
$I2CWR 32 0x00
$I2CWR 33 0x37
# Register 34-36: NC4_LS = 0x37 (56)
$I2CWR 34 0x00
$I2CWR 35 0x00
$I2CWR 36 0x37
# Register 37-39: NC5_LS = 0x0D (14)
$I2CWR 37 0x00
$I2CWR 38 0x00
$I2CWR 39 0x0D

# These determine the VCO frequency.
# It's (CKINx/N3x)*(N2_HS)*(N2_LS).
# For CKIN1 or CKIN3 = 10 MHz, this is (10/5)*(10)*(280) = 5.6 GHz

# Register 40-42: N2_HS = 110 (10), N2_LS = 280
$I2CWR 40 0xC0
$I2CWR 41 0x01
$I2CWR 42 0x17
# Register 44-45: CKIN1 divider (N31) = 0x4 (5)
$I2CWR 43 0x00
$I2CWR 44 0x00
$I2CWR 45 0x04
# Register 46-48: CKIN2 divider (N32) = 0x4 (5)
$I2CWR 46 0x00
$I2CWR 47 0x00
$I2CWR 48 0x04
# Register 49-51: CKIN3 divider (N33) = 0x4 (5)
$I2CWR 49 0x00
$I2CWR 50 0x00
$I2CWR 51 0x04
# done, now do an ICAL
$I2CWR 136 0x40

# all done
