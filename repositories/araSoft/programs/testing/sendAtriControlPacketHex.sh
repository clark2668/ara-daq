#!/bin/bash


if [[ $1 = "" ]]; then
    echo "`basename $0` <destination> <data 0>... "
    exit 1
fi


command="sendAtriControlPacket"


for var in "$@"
do
#    echo $var
    dec=`printf '%d\n' $var`
 #   echo $dec
    command=`echo "${command} ${dec}"`
done
echo "$command"
$command




