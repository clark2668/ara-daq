#!/bin/bash

txt1=$(lsusb -d 04b4:8613 | awk '{print $2}')
txt2=$(lsusb -d 04b4:8613 | awk '{print $4}')

if [ -z "$txt1" ]; then
txt1=$(lsusb -d 04b4:9999 | awk '{print $2}')
txt2=$(lsusb -d 04b4:9999 | awk '{print $4}')
fi

if [ -z "$txt1" ]; then
txt1=$(lsusb -d 04b4:1111 | awk '{print $2}')
txt2=$(lsusb -d 04b4:1111 | awk '{print $4}')
fi


if [ "$1" = "ara_general" ]; then
    prog=~/ezusb_firmware/ATRI/VR_General/EZUSB_VR_GENERAL.hex
elif [ "$1" = "ara_fpga" ]; then
    prog=~/ezusb_firmware/ATRI/VR_FPGA_Program/EZUSB_VR_FPGA_PROG.hex
else
    prog=FAIL
fi
if [ "$prog" = "FAIL" ];
then
    echo "Correct usage: bash loadprog.sh < ara_general | ara_fpga >"
else
    /sbin/fxload -D /dev/bus/usb/$txt1/${txt2:0:3} -t fx2 -I $prog
    echo $1 loaded
fi