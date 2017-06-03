#!/bin/bash

####
#
# To monitor the temperature of the ARA-1 DAQ system, this script prints out
# an abbrievated output of the "atatd" program to the standard output every
# 5 minutes.  To run this script, use the following command:
#
#  nohup ./ATATMonitor.abbrievated.bash 2>&1 >>ATATMonitor.abbrievated.log 
#
# ----------------------------------------------------------------------------------
#
####

# 1 - specify the directory to which the logfile is written and make sure that it
#     exists

#export ATATMONITOR_OUTPUT_DIRECTORY=/raid/ARA/ara/scripts/service/ATATMonitor/ATATMonitor_logfiles
export ATATMONITOR_OUTPUT_DIRECTORY=/home/ara/ATATMonitor_logfiles
if [ -z "${ATATMONITOR_OUTPUT_DIRECTORY}" ]; then
   echo "$0: ATATMONITOR_OUTPUT_DIRECTORY, empty string -- fix the script ... exitting -1"
   sleep 60
   exit -1
fi
if [ ! -d "${ATATMONITOR_OUTPUT_DIRECTORY}" ]; then
   mkdir ${ATATMONITOR_OUTPUT_DIRECTORY}
fi

export LD_LIBRARY_PATH=/usr/local/lib:/usr/lib

# 2 - generate the timestamp for today so that it can used as a suffix
#     to the name of the output logfile.

UNFORMATTED_DATETIME_OF_NOW=`date --iso-8601=seconds`
export DATE_OF_NOW=`echo "${UNFORMATTED_DATETIME_OF_NOW}" | /usr/bin/awk -FT '{print $1}' | /bin/sed -e 's|-||g'`
export TIME_OF_NOW=`echo "${UNFORMATTED_DATETIME_OF_NOW}" | /usr/bin/awk -FT '{print $2}' | /usr/bin/awk -F- '{print $1}' | sed -e 's|:||g'`
export NOW_TIMESTAMP=`echo "${DATE_OF_NOW}${TIME_OF_NOW}"`

ln -sf ${ATATMONITOR_OUTPUT_DIRECTORY}/ATATMonitor.${NOW_TIMESTAMP} ${ATATMONITOR_OUTPUT_DIRECTORY}/ATATMonitor.current

# 3 - enter a loop which does the 'df -h' every 5 minutes.  This loop will
#     halt at midnight.

export LOOP_COUNT=0
export THIS_DATE=`/bin/date --utc +%s`
export MOD_IN_DAY=`echo "${THIS_DATE} % 86400" | /usr/bin/bc`
export LAST_MOD_IN_DAY=${MOD_IN_DAY}
while [ ${MOD_IN_DAY} -ge ${LAST_MOD_IN_DAY} ]; do
   export LAST_MOD_IN_DAY=${MOD_IN_DAY}
   export LOOP_COUNT=$((LOOP_COUNT+1))
   echo "( `date` - `date +%s` ) : `/home/ara/repositories/ara/araSoft/branches/psa/working/bin/atatd EX1 | /usr/bin/tr \"\\n\" \" \"`" \
        >>${ATATMONITOR_OUTPUT_DIRECTORY}/ATATMonitor.${NOW_TIMESTAMP}
   export THIS_DATE=`/bin/date --utc +%s`
   export MOD_IN_DAY=`echo "${THIS_DATE} % 86400" | /usr/bin/bc`
#  echo "(`/bin/date`) - `/bin/date +%s` : MOD_IN_DAY = ${MOD_IN_DAY} - LAST_MOD_IN_DAY = ${LAST_MOD_IN_DAY}" 
   sleep 300
done

# 4 - exit the script ... the loop above ends at the end of each day so
#     that there is one logfile for each day.

echo "(`/bin/date`) - `/bin/date +%s` : Exitting Loop As A New Day As Dawned"
exit 0


