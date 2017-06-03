#!/bin/bash

#search for sub devices using lsusb
#search ffor un-configured device
txt1=$(lsusb -d 04b4:8613 | awk '{print $2}')
txt2=$(lsusb -d 04b4:8613 | awk '{print $4}')

#search for usb configured with FX2_firmware
if [ -z "$txt1" ]; then
txt1=$(lsusb -d 04b4:9999 | awk '{print $2}')
txt2=$(lsusb -d 04b4:9999 | awk '{print $4}')
fi

#search for usb configured with test firmware
if [ -z "$txt1" ]; then
txt1=$(lsusb -d 04b4:1111 | awk '{print $2}')
txt2=$(lsusb -d 04b4:1111 | awk '{print $4}')
fi

if [ -z "$txt1" ]; then
    echo -e "FX2 device not found"
    exit -1
fi
if [ -a "$1" ]; then
    echo -e "Firmware file found"
    # We never need to change the permissions of loading the *first*
    # device: if hotplug is set up, it's already done.
    # If not, we need to run as sudo anyway.
    echo "Loading firmware"
    /sbin/fxload -D /dev/bus/usb/$txt1/${txt2:0:3} -t fx2lp -I $1
    sleep 5
    
    # If hotplug is set up, this will happen automatically.
    # Otherwise we try to do it ourselves.
    txt1=$(lsusb -d 04b4:9999 | awk '{print $2}')
    txt2=$(lsusb -d 04b4:9999 | awk '{print $4}')

    CURPERMS=`stat -c %a /dev/bus/usb/$txt1/${txt2:0:3}`
    MYPERM="666"
    echo Current permissions are $CURPERMS want $MYPERM
    if [ "$CURPERMS" != "$MYPERM" ] ; then
	echo "Changing new device permissions"
	chmod a+rw /dev/bus/usb/$txt1/${txt2:0:3}
    fi
else
    echo "unable to find firmware file. Usage - loadFX2Firmware.sh <firmware.hex>"
fi