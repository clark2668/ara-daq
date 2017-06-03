#!/bin/bash
export ARA_DAQ_DIR=~/WorkingDAQ
export ARA_CONFIG_DIR=${ARA_DAQ_DIR}/config 
export ARA_FIRMWARE_DIR=/home/ara/Firmware
export ARA_LOG_DIR=/tmp/LogFiles

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ARA_DAQ_DIR}/lib/
export DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}:${ARA_DAQ_DIR}/lib/
export PATH=${PATH}:${ARA_DAQ_DIR}/bin      
export PYTHONPATH=${PYTHONPATH}:${ARA_DAQ_DIR}/scripts/python/ 
