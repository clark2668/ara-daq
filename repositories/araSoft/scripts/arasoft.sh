#!/bin/bash

# Program definitions

SETUP="/home/ara/araSoft/trunk/setupAraDaqTesting.sh"
ARAACQD="/home/ara/araSoft/trunk/programs/ARAAcqd/ARAAcqd"
TESTING_PATH="/home/ara/araSoft/trunk/programs/testing"
CLOCKSETUP="/home/ara/clockset_real.sh"

source $SETUP
$ARAACQD &> acqd.log &
export PATH=$PATH:$TESTING_PATH
sleep 1
$CLOCKSETUP
chmod 666 /tmp/atri_control
chmod 666 /tmp/fx2_control
