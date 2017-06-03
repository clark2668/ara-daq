#!/bin/bash
#
#  Simple script that:
#  a) Checks the link dir for completed files
#  b) Assembles an rsync command to copy those files to a specified network dir
#  c) Copies those files using rsync
#  d) Optionally deletes the original files
#
#    To configure this script, please update the following, hard-coded
#    environment variables:
#
#                 (1) LOCAL_ARASOFT_DATA_DIRECTORY
#
#                     this variable should point to the location of the event and the hk
#                     data files; use full path names and remember to include a trailing
#                     trailing '/' as that is needed.
#
#                 (2) SERVER_ARCHIVE_DIRECTORY
#
#                     should point to the location on the remote server where the event
#                     and the hk data files will be stored; use full path names and
#                     remember to include a trailing '/' as that is needed.  For example,
#                     the following setting:
#
#                        SERVER_ARCHIVE_DIRECTORY=radio@anitarf:/home/radio/ARA/data
#
#                     places the files in:
#
#                                            /home/radio/ARA/data
#
#                     subdirectory of the radio account on anitarf.
#
#                 Next, you will need to make sure that the public keys are in place
#                 so that the SBC can log into the main server without using a password.
#
#                 To run this script, enter a command like:
#
#                             nohup ./DataPush.bash 2>&1 >>DataPush.logfile
#
################################################################################


#  0 - general script settings
echo "(`date`) : [START_UP] START OF SCRIPT - $0"

#      A - DEBUG_MODE : files will not be removed after rsync if debug mode is on

export DEBUG_MODE=NO # =NO, debug mode off / =anything else, debug mode on 

if [ ! -z "${DEBUG_MODE}" ]; then
   if [ "${DEBUG_MODE}" == "NO" ]; then
      echo "(`date`) : [START_UP] DEBUG_MODE = NO ... files will be removed after successful rsync"
   else
      echo "(`date`) : [START_UP] DEBUG_MODE = ${DEBUG_MODE} ... files will *NOT* be removed after successful rsync"
   fi
else
   echo "(`date`) : [START_UP] DEBUG_MODE = empty string ... files will *NOT* be removed after successful rsync"
fi


#  1 - setup software enviroment

#      A - specify which awk is present on the SBC

export AWK=`which awk`
WHICH_EXIT_CODE=$?
if [ ${WHICH_EXIT_CODE} -ne 0 ]; then
   export AWK=`which gawk`
   WHICH_EXIT_CODE=$?
   if [ ${WHICH_EXIT_CODE} -ne 0 ]; then
      echo "(`date`) : [START_UP] unable to find awk or gawk on the SBC"
      echo "(`date`) : [START_UP] so no go ... exitting -1"
      exit -1
   fi
fi

echo "(`date`) : --------------------------------------------------------------------------------------------------------------------------------------"


#  2 - specify delay time between successful rsync invokations [current value 900 seconds/15 minutes]

export SLEEPTIME=180

#  3 - specify the local directory in which the event and hk data are stored by ARAacqd

#      A - set the variable

export LOCAL_ARASOFT_DATA_DIRECTORY=/data
#export LOCAL_ARASOFT_DATA_DIRECTORY=/home/ara/Freezer_Testing/data/

#      B - validate the setting

echo -n "(`date`) : [START_UP] LOCAL_ARASOFT_DATA_DIRECTORY = ${LOCAL_ARASOFT_DATA_DIRECTORY}"
if [ -z "${LOCAL_ARASOFT_DATA_DIRECTORY}" ]; then
   echo " - null string"
   echo "(`date`) : [START_UP] so no go ... exitting -1"
   exit -1
fi
if [ ! -d "${LOCAL_ARASOFT_DATA_DIRECTORY}" ]; then
   echo " - not found or not a directory"
   echo "(`date`) : [START_UP] so no go ... exitting -1"
   exit -1
fi
echo " - found directory on SBC - okay"


#  4 - specify the directory in which the event and hk data are archived on the remote server

#      A - set the variable

export SERVER_ARCHIVE_DIRECTORY=radio@anitarf:/home/radio/ARA/StationOne/SBC_TestData
echo "(`date`) : [START_UP] SERVER_ARCHIVE_DIRECTORY = ${SERVER_ARCHIVE_DIRECTORY}"

#      B - validate the setting

export REMOTE_ACCOUNT=`echo ${SERVER_ARCHIVE_DIRECTORY} | ${AWK} -F\: '{print $1}'`
echo "$REMOTE_ACCOUNT"
export REMOTE_DIRECTORY=`echo ${SERVER_ARCHIVE_DIRECTORY} | ${AWK} -F\: '{print $2}'`
echo "$REMOTE_DIRECTORY"
CHECK_FOR_DIRECTORY=`ssh ${REMOTE_ACCOUNT} "ls -al ${REMOTE_DIRECTORY} | wc -l"`
SSH_EXIT_CODE=$?
if [ ${SSH_EXIT_CODE} -eq 0 ]; then
   if [ ${CHECK_FOR_DIRECTORY} -ne 0 ]; then
      echo "(`date`) : [START_UP] found ${REMOTE_DIRECTORY} on remote server ${REMOTE_ACCOUNT}"
   else
      echo "(`date`) : [START_UP] unable to locate ${REMOTE_DIRECTORY} on remote server ${REMOTE_ACCOUNT}"
      echo "(`date`) : [START_UP] exitting -1"
      exit -1
   fi
else
   echo "(`date`) : [START_UP] unable to ssh to remote server, ssh status code = ${SSH_EXIT_CODE}"
   echo "(`date`) : [START_UP] exitting -1"
   exit -1
fi


#  5 - echo all final settings for completeness

echo "(`date`) : [SETTINGS]   LOCAL_ARASOFT_DATA_DIRECTORY = ${LOCAL_ARASOFT_DATA_DIRECTORY}"
echo "(`date`) : [SETTINGS] SERVER_ARCHIVE_DIRECTORY = ${SERVER_ARCHIVE_DIRECTORY}"

echo "(`date`) : [START_UP] end of script initialization section"
echo "(`date`) : --------------------------------------------------------------------------------------------------------------------------------------"

#Enter infinite loop

while [ 1 ]; do
    

    rm -f $LOCAL_ARASOFT_DATA_DIRECTORY/xfer.list
    rm -f $LOCAL_ARASOFT_DATA_DIRECTORY/xferlink.list
    
    for xferlink in $LOCAL_ARASOFT_DATA_DIRECTORY/link/*; do
	CURRENT_LINK=`readlink $xferlink`
	LOSE_DAT=${CURRENT_LINK%.*}
	RUNNUM=${LOSE_DAT#*.run}
	FULL_LINK=run_${RUNNUM}/${CURRENT_LINK#*/current/}
	echo $FULL_LINK >> $LOCAL_ARASOFT_DATA_DIRECTORY/xfer.list
	echo $xferlink >> $LOCAL_ARASOFT_DATA_DIRECTORY/xferlink.list      
    done
    
    if  test `cat ${LOCAL_ARASOFT_DATA_DIRECTORY}/xfer.list | wc -l` -gt 1 ; then
	#Call three times because the UH network is crap, should really just test taht the files copy
	rsync -av --files-from=$LOCAL_ARASOFT_DATA_DIRECTORY/xfer.list ${LOCAL_ARASOFT_DATA_DIRECTORY} ${SERVER_ARCHIVE_DIRECTORY}

	rsync -av --files-from=$LOCAL_ARASOFT_DATA_DIRECTORY/xfer.list ${LOCAL_ARASOFT_DATA_DIRECTORY} ${SERVER_ARCHIVE_DIRECTORY}

	rsync -av --files-from=$LOCAL_ARASOFT_DATA_DIRECTORY/xfer.list ${LOCAL_ARASOFT_DATA_DIRECTORY} ${SERVER_ARCHIVE_DIRECTORY}
		
	if [ ! -z "${DEBUG_MODE}" ]; then
	    if [ "${DEBUG_MODE}" == "NO" ]; then
		
		for alink in `cat $LOCAL_ARASOFT_DATA_DIRECTORY/xferlink.list`; do
		    echo "Will remove $alink"
		    rm $alink
		done
		
		
		for afile in `cat $LOCAL_ARASOFT_DATA_DIRECTORY/xfer.list`; do
		    echo "Will remove ${LOCAL_ARASOFT_DATA_DIRECTORY}/${afile}"
		    rm ${LOCAL_ARASOFT_DATA_DIRECTORY}/${afile}
		done

	    fi
	fi	
    fi
    
    
    echo "(`date`) : going to sleep for ${SLEEPTIME} seconds"
    echo "(`date`) : ---------------------------------------------------------------------------------------------------------------------------------------"
    sleep ${SLEEPTIME}
done