#!/bin/bash


if [[ $4 = "" ]]; then
    echo "`basename $0` <bmRequestType> <bRequest> <wValue> <wIndex> <data 0>... "
    exit 1
fi


command="sendFx2ControlPacket"


for var in "$@"
do
#    echo $var
    dec=`printf '%d\n' $var`
 #   echo $dec
    command=`echo "${command} ${dec}"`
done
echo "$command"
$command




