#!/bin/bash

if [ ! -n "$ARA_DAQ_DIR" ]; then
    export ARA_DAQ_DIR=/home/barawn/repositories/ara/araSoft/trunk
    echo "ARA_DAQ_DIR is not set: using ${ARA_DAQ_DIR}"
fi

SDIR=${ARA_DAQ_DIR}/scripts/
HEXRDIR=`dirname $1`
HEXNAME=`basename $1`
HEXDIR=`cd $HEXRDIR;pwd`
BINRDIR=`dirname $2`
BINNAME=`basename $2`
BINDIR=`cd $BINRDIR;pwd`
HEXPATH="$HEXDIR/$HEXNAME"
BINPATH="$BINDIR/$BINNAME"

OLDDIR=`pwd`
cd $SDIR
bash loadFX2Firmware.sh $HEXPATH
bash programFPGA.sh $BINPATH
cd $OLDDIR