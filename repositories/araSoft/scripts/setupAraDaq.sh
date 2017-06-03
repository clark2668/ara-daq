#!/bin/bash

unset ARA_DAQ_DIR
unset LD_LIBRARY_PATH
unset PATH

export ARA_DAQ_DIR=~/WorkingDAQ/
export ARA_CONFIG_DIR=${ARA_DAQ_DIR}/config 
export ARA_FIRMWARE_DIR=/home/ara/Firmware
export ARA_LOG_DIR=/home/ara/LogFiles

export LD_LIBRARY_PATH=${ARA_DAQ_DIR}/lib/
export DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}:${ARA_DAQ_DIR}/lib/
export PATH=/usr/local/bin:/usr/bin:/bin:${ARA_DAQ_DIR}/bin
export PYTHONPATH=${PYTHONPATH}:${ARA_DAQ_DIR}/scripts/python/ 