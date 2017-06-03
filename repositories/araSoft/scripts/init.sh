#!/bin/bash

# This is a standalone BASIC initialization script.

if [ ! -n "$ARA_DAQ_DIR" ]; then
    echo ARA_DAQ_DIR is not set!
    exit
fi

DIR=${ARA_DAQ_DIR}/scripts/
OLDDIR=`pwd`
cd $DIR
echo Initializing Si5367 clock multiplier
bash clockset_real.sh &> /dev/null
sleep 1
echo Resetting internal DCM
../programs/testing/wbw 0x08 0x80 &> /dev/null
cd $OLDDIR