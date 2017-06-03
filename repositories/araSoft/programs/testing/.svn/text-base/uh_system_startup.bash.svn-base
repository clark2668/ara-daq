#!/bin/bash

cd ~/repositories/ara/araSoft/trunk/programs/testing
atriInitializeDaughters
../../scripts/clockset_real.sh
bash sendAtriControlPacketHex.sh 1 1 0 32 1
./atriSetVdly 4 0xB852
bash ./sendAtriControlPacketHex.sh 0x03 0x00 0x18 0x11 0x46 0x00

