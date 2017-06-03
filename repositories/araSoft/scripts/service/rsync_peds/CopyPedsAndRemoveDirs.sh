#!/bin/bash
#
##############################################################################
#
# CopyPedsAndRemoveDirs.sh
# ------------------------------------------------------------
# A simple script to copy pedestal files and remove empty run directories
#

PEDRESULTS_LOGFILE_DIRECTORY=/tmp/LogFiles/PedResultsPush/
mkdir -p ${PEDRESULTS_LOGFILE_DIRECTORY}

#  Time between successful rsync invocations in seconds
SLEEPTIME=120

#Where is the data on the local machine
LOCAL_ARADAQ_DIRECTORY=/tmp/ARAAcqdData

#Station name and year
STATION_NAME=`cat /etc/stationName`
STATION_YEAR=`cat /etc/stationYear`

SERVER_ARCHIVE_DIRECTORY=radio@radproc:/home/radio/data/${STATION_NAME}/${STATION_YEAR}/raw_data/          # For South Pole 2013 Season (ARA-02)
#SERVER_ARCHIVE_DIRECTORY=jdavies@pc148:/unix/ara/data/miniATRI_ucl/raw_data/

#      B - tack on a trailing '/'
SERVER_ARCHIVE_DIRECTORY=`echo "${SERVER_ARCHIVE_DIRECTORY}/" | sed -r -e 's|\/{2,}$|/|'`

#  5 - set the location for the 'running' logfile from rsync
RSYNC_XFER_LOGFILE=${PEDRESULTS_LOGFILE_DIRECTORY}/rsync_xfer_logfile

#  7 - enter an infinite loop to do each rsync transfer

while [ 1 ]; do

#Loop over all but the most recent run directory
    currentRun=`readlink -f ${LOCAL_ARADAQ_DIRECTORY}/current`
    for rundir in `ls -tr  ${LOCAL_ARADAQ_DIRECTORY} | grep run_ | head -n -1`; do
	
	if [ "$currentRun" != "$LOCAL_ARADAQ_DIRECTORY/$rundir" ]; then	
	    if [ "$(ls -A ${LOCAL_ARADAQ_DIRECTORY}/${rundir})" ]; then
		cd ${LOCAL_ARADAQ_DIRECTORY}/$rundir
		ionice -c2 -n0 nice -n10 rsync -av -e 'ssh -c arcfour' --remove-source-files * ${SERVER_ARCHIVE_DIRECTORY}/${rundir} >>${RSYNC_XFER_LOGFILE} 2>&1
		RSYNC_EXIT_CODE=$?
	    else
		RSYNC_EXIT_CODE=0
	    fi
	
	    if [ ${RSYNC_EXIT_CODE} -eq 0 ]; then
		cd ${LOCAL_ARADAQ_DIRECTORY}
		find ${rundir} -empty -type d -print0 | xargs -0 rmdir
	    else
		echo "(`date`) :  RSYNC_EXIT_CODE = ${RSYNC_EXIT_CODE} <- PROBLEM: PedResultsPush experienced an error"
	    fi
	fi
    done
   
    sleep $SLEEPTIME

done # end of INFINITE WHILE-loop for doing the rsync

#  8 - out of the infinite loop ... should be impossible to reach here; actually
exit 0

