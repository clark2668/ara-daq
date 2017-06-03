#!/bin/bash                                                                 
#0. check ARA_DAQ_DIR exists

#1. Stop the DAQ software

bash ${ARA_DAQ_DIR}/scripts/stopAraSoft.sh

#2. Program the FX2 and FPGA

bash ${ARA_DAQ_DIR}/scripts/loadFX2Firmware.sh ${ARA_FIRMWARE_DIR}/currentFX2Firmware.ihx

bash ${ARA_DAQ_DIR}/scripts/programFPGA.sh ${ARA_FIRMWARE_DIR}/currentATRIFirmware.bin

#3. Start the DAQ software

bash ${ARA_DAQ_DIR}/scripts/startAraSoftWithTimeRecord.sh