
#!/bin/bash
#
##########################################################################################
#
# DataPush.for_ATRI.RsyncViaFilesFrom.With_IPv6.bash
# --------------------------------------------------
#
# This script pushes the event and hk data from the SBC to a remote server and, as files
# are successfully transferred, deletes them from the SBC.  To avoid copying data that
# is being written by the araSoft ARA DAQ while the transfer is underway, the script
# intentionally does not last subdirectory of event, eventHk, sensorHk, etc. data for
# the most recent (i.e., the one with the highest run number) run directory.
#
# ========================================================================================
#
# To configure this script, please update the following, hard-code environment variables:
#
# (1) LOCAL_ARADAQ_DIRECTORY
#
#     this variable should point to the location of the event, eventHk, etc. data files;
#     use full path names and remember to include a trailing '/' as that is needed.
#
# (2) SERVER_ARCHIVE_DIRECTORY
#
#     should point to the location on the remote server where the event, eventHk, etc.
#     data files will be stored; use full path names and remember to include a trailing
#     '/' as that is needed.  For example, the following setting:
#
#                SERVER_ARCHIVE_DIRECTORY=radio@anitarf:/home/radio/ARA/data
#
#     places the files in:
#
#                                 /home/radio/ARA/data
#
#     subdirectory of the radio account on anitarf.
#
# (3) DATAPUSH_LOGFILE_DIRECTORY
#
#     should point to the directory into which logfiles are stored; for example, the file
#     into which rsync writes log information containing the list of files that have been
#     transferred successfully.
#
# (4) SLEEPTIME
#
#     should be set to number of seconds between rsync passes
#
# Next, you will need to make sure that the ssh public keys are in place so that the SBC
# can log into the main server without using a password.
#
# To run this script, enter a command like:
#
#                   ./DataPush.for_ATRI.RsyncViaFilesFrom.With_IPv6.bash
#
# Usually, this script will be run by /service daemon.  If run by hand, it's a good idea
# to run it using nohup.
#
##########################################################################################

#  0 - general script settings
#Switch VERBOSE to a number greater than 0 to make the script print much more debugging informatopm
VERBOSE=0
safeecho() {
    if [ ${VERBOSE} -gt 0 ]; then echo $@; fi
}

safeecho "(`date`) : [START_UP] START OF SCRIPT - $0"

#      A - DEBUG_MODE : files will not be removed after rsync if debug mode is on

#export DEBUG_MODE=YES  # =NO, debug mode off / =anything else, debug mode on 
export DEBUG_MODE=NO  # =NO, debug mode off / =anything else, debug mode on 

if [ ! -z "${DEBUG_MODE}" ]; then
   if [ "${DEBUG_MODE}" == "NO" ]; then
      safeecho "(`date`) : [START_UP] DEBUG_MODE = NO ... files will be removed after successful rsync"
   else
      safeecho "(`date`) : [START_UP] DEBUG_MODE = ${DEBUG_MODE} ... files will *NOT* be removed after successful rsync"
   fi
else
   safeecho "(`date`) : [START_UP] DEBUG_MODE = empty string ... files will *NOT* be removed after successful rsync"
fi

#      B - logfile directory for this script

export DATAPUSH_LOGFILE_DIRECTORY=/tmp/LogFiles/DataPush/
mkdir -p ${DATAPUSH_LOGFILE_DIRECTORY}
if [ -z "${DATAPUSH_LOGFILE_DIRECTORY}" ]; then
   echo "(`date`) : [START_UP] DATAPUSH_LOGFILE_DIRECTORY = empty string <- fix the script and then rerun it"
   echo "(`date`) : [START_UP] so no go ... exitting -1"
   exit -1
else
   safeecho -n "(`date`) : [START_UP] DATAPUSH_LOGFILE_DIRECTORY = ${DATAPUSH_LOGFILE_DIRECTORY}"
   if [ ! -d ${DATAPUSH_LOGFILE_DIRECTORY} ]; then
      echo " - not found or not a directory <- make it or fix the script and then try again"
      echo "(`date`) : [START_UP] so no go ... exitting -1"
      exit -1
   else
      safeecho " - dir found"
   fi
fi

#  1 - setup software enviroment

#      A - specify which gawk is present on the SBC

export AWK=`which gawk`
WHICH_EXIT_CODE=$?
if [ ${WHICH_EXIT_CODE} -ne 0 ]; then
   export AWK=`which awk`
   WHICH_EXIT_CODE=$?
   if [ ${WHICH_EXIT_CODE} -ne 0 ]; then
      echo "(`date`) : [START_UP] unable to find awk or gawk on the SBC"
      echo "(`date`) : [START_UP] so no go ... exitting -1"
      exit -1
   fi
fi

safeecho "(`date`) : ---------------------------------------------------------------------------------------------------------------------------------------"

#  2 - specify delay time between successful rsync invokations [current value 60 seconds/1 minute]

export SLEEPTIME=60

#  3 - specify the local directory in which the event and hk data are stored by the ARAAacqd DAQ
#
#      NOTE: the final slash on the directory name is important as it affects the copy behavior of rsync.  If the
#      ----  slash is missing, rsync will copy as a subdirectory instead of copying just the contents.  To avoid
#            this issue, the script adds a '/' to the directory name when one is not already present.
#
#      A - set the variable

#export LOCAL_ARADAQ_DIRECTORY=/data/ARAAcqdData/
export LOCAL_ARADAQ_DIRECTORY=/tmp/ARAAcqdData/

#      B - tack on a trailing '/'

export LOCAL_ARA_DAQ_DIRECTORY=`echo "${LOCAL_ARADAQ_DIRECTORY}/" | sed -r -e 's|\/{2,}$|/|'`

#      C - validate the setting

safeecho -n "(`date`) : [START_UP]   LOCAL_ARADAQ_DIRECTORY = ${LOCAL_ARADAQ_DIRECTORY}"
if [ -z "${LOCAL_ARADAQ_DIRECTORY}" ]; then
   echo " <- PROBLEM: null string"
   echo "(`date`) : [START_UP] so no go ... exitting -1"
   exit -1
fi
if [ ! -d "${LOCAL_ARADAQ_DIRECTORY}" ]; then
   echo " <- PROBLEM: not found or not a directory"
   echo "(`date`) : [START_UP] so no go ... exitting -1"
   exit -1
fi
safeecho " <- found directory on SBC - okay"

#  4 - specify the directory in which the raw ATRI (event, hk, etc.) data are archived on the remote server
#
#      NOTE: the final slash on the directory name is important as it affects the copy behavior of rsync.  To
#      ----  avoid this issue, the script adds a '/' to the directory name when one is not already present.
#
#      A - set the variable
#
#          gluon              - radio@fe80::21d:9ff:fe2d:887%eth0
#          cray-supercomputer - radio@fe80::222:19ff:fef3:382b%eth0
export STATION_NAME=`cat /etc/stationName`
export STATION_YEAR=`cat /etc/stationYear`
export USE_IPv6=NO
#export SERVER_ARCHIVE_DIRECTORY='radio@radproc.sps.icecube.southpole.usap.gov:/home/radio/data/ARA01/2013/raw_data/'         # For South Pole 2013 Season (ARA-01B)
#export SERVER_ARCHIVE_DIRECTORY='radio@radproc.sps.icecube.southpole.usap.gov:/home/radio/data/ARA02/2013/raw_data/'          # For South Pole 2013 Season (ARA-02)
export SERVER_ARCHIVE_DIRECTORY=radio@radproc:/home/radio/data/${STATION_NAME}/${STATION_YEAR}/raw_data/         # For South Pole 2013 Season (ARA-02)
echo "${SERVER_ARCHIVE_DIRECTORY}"
#      B - tack on a trailing '/'

export SERVER_ARCHIVE_DIRECTORY=`echo "${SERVER_ARCHIVE_DIRECTORY}/" | sed -r -e 's|\/{2,}$|/|'`

#      C - validate the setting

safeecho "(`date`) : [START_UP] SERVER_ARCHIVE_DIRECTORY = ${SERVER_ARCHIVE_DIRECTORY}"
safeecho "(`date`) : [START_UP]                 USE_IPv6 = ${USE_IPv6}"

if [ "${USE_IPv6}" == "YES" ]; then
   export REMOTE_ACCOUNT=`echo "${SERVER_ARCHIVE_DIRECTORY}" | ${AWK} -F\] '{print $1}' | sed -e 's|\[||'`
   export REMOTE_DIRECTORY=`echo "${SERVER_ARCHIVE_DIRECTORY}" | ${AWK} -F\] '{print $2}' | sed -e 's|^\:||'`
   export CHECK_FOR_DIRECTORY=`ssh -6 ${REMOTE_ACCOUNT} "ls -al ${REMOTE_DIRECTORY} | wc -l"`
   safeecho "(`date`) : [START_UP]      CHECK_FOR_DIRECTORY = ${CHECK_FOR_DIRECTORY}"
else
   export REMOTE_ACCOUNT=`echo ${SERVER_ARCHIVE_DIRECTORY} | ${AWK} -F\: '{print $1}'`
   export REMOTE_DIRECTORY=`echo ${SERVER_ARCHIVE_DIRECTORY} | ${AWK} -F\: '{print $2}'`
   export CHECK_FOR_DIRECTORY=`ssh ${REMOTE_ACCOUNT} "ls -al ${REMOTE_DIRECTORY} | wc -l"`
   safeecho "(`date`) : [START_UP]      CHECK_FOR_DIRECTORY = ${CHECK_FOR_DIRECTORY}"
fi
SSH_EXIT_CODE=$?
if [ ${SSH_EXIT_CODE} -eq 0 ]; then
   if [ ${CHECK_FOR_DIRECTORY} -ne 0 ]; then
      safeecho "(`date`) : [START_UP] SERVER_ARCHIVE_DIRECTORY = ${SERVER_ARCHIVE_DIRECTORY}"
      safeecho "(`date`) : [START_UP]                            -> found ${REMOTE_DIRECTORY} on remote server ${REMOTE_ACCOUNT}"
   else
      echo "(`date`) : [START_UP]                            -> PROBLEM: unable to locate ${REMOTE_DIRECTORY} on remote server ${REMOTE_ACCOUNT}"
      echo "(`date`) : [START_UP] exitting -1"
      exit -1
   fi
else
   echo "(`date`) : [START_UP]                               -> PROBLEM: unable to ssh to remote server, ssh status code = ${SSH_EXIT_CODE}"
   echo "(`date`) : [START_UP] exitting -1"
   exit -1
fi

#  5 - set the location for the 'running' logfile from rsync

export RSYNC_XFER_LOGFILE=${DATAPUSH_LOGFILE_DIRECTORY}/rsync_xfer_logfile

#  6 - echo all final settings for completeness

safeecho "(`date`) : [SETTINGS]   LOCAL_ARADAQ_DIRECTORY = ${LOCAL_ARADAQ_DIRECTORY}"
safeecho "(`date`) : [SETTINGS] SERVER_ARCHIVE_DIRECTORY = ${SERVER_ARCHIVE_DIRECTORY}"
safeecho "(`date`) : [SETTINGS]       RSYNC_XFER_LOGFILE = ${RSYNC_XFER_LOGFILE}"

safeecho "(`date`) : [START_UP] end of script initialization section"
safeecho "(`date`) : ---------------------------------------------------------------------------------------------------------------------------------------"

#  7 - enter an infinite loop to do each rsync transfer

while [ 1 ]; do

  if [ ${VERBOSE} -gt 0 ]; then 
      echo "(`date`) : [DISK_RPT] START - BEFORE TRANSFER DISK SPACE DUMP"
      DF_LINE=`df -h | grep Filesystem`
      printf "(`date`) : [DISK_RPT]       - %10.10s : %s\n" "Drive" "${DF_LINE}"
      
      for ITEM in /home    \
          /data    \
          /var/run \
          /tmp     \
	  ; do
	  DF_LINE=`df -h | grep -e "${ITEM}$"`
	  printf "(`date`) : [DISK_RPT]       - %10.10s : %s\n" "${ITEM}" "${DF_LINE}"
      done
      echo "(`date`) : [DISK_RPT] __END - BEFORE TRANSFER DISK SPACE DUMP"
  fi

  safeecho "(`date`) : ---------------------------------------------------------------------------------------------------------------------------------------"

  export NOW_TIMESTAMP=`date +%s`
  export RSYNC_INCLUDE_LIST=${DATAPUSH_LOGFILE_DIRECTORY}/rsync_include_list.${NOW_TIMESTAMP}.txt
  if [ ! -e ${RSYNC_INCLUDE_LIST} ]; then # -e files exists / ! -e file does not exist
     touch ${RSYNC_INCLUDE_LIST}.absolute_pathnames
  fi
  safeecho "(`date`) : [RSYNC_STARTUP] NOW_TIMESTAMP            = ${NOW_TIMESTAMP}"
  safeecho "(`date`) : [RSYNC_STARTUP] RSYNC_INCLUDE_LIST       = ${RSYNC_INCLUDE_LIST}"

  for THIS_ITEM in EXCLUDE_LAST_FILE::ev::ev_::event                \
                   EXCLUDE_LAST_FILE::eventHk::eventHk_::eventHk    \
                   EXCLUDE_LAST_FILE::sensorHk::sensorHk_::sensorHk \
                   EXCLUDE_LAST_FILE::monitor::monitor::monitorHk      \
                   EXCLUDE_LAST_FILE::runStop::runStop::logs      \
                   EXCLUDE_LAST_FILE::configFile::configFile::logs      \
                   EXCLUDE_LAST_FILE::runStart::runStart::logs      \
                   EXCLUDE_LAST_FILE::atriLog::atriEvent.log::.     \
		   EXCLUDE_LAST_FILE::pedestalValues::pedestalValues::. \
		   EXCLUDE_LAST_FILE::pedestalWidths::pedestalWidths::. \
  ; do


      export MODE=`echo "${THIS_ITEM}" | ${AWK} -F\:\: '{printf("%s",$1)}'`          # arg 1 : the mode (EXCLUDE_LAST_FILE or EXCLUDE_ALL_FILES)
      export TAG_xx=`echo "${THIS_ITEM}" | ${AWK} -F\:\: '{printf("%s",$2)}'`        # arg 2 : the tag for script logging purpose
      export FINDTAG_xx=`echo "${THIS_ITEM}" | ${AWK} -F\:\: '{printf("%s",$3)}'`    # arg 3 : the tag for finding the desired files
      export DIRECTORY_TAG_xx=`echo "${THIS_ITEM}" | ${AWK} -F\:\: '{print $4}'`     # arg 4 : the directory in the run with the desired files

      safeecho "(`date`) : **************************************** DataPush Making Include List For ${TAG_xx} Data Files ****************************************"

#      printf "(`date`) : [RSYNC_STARTUP] [%8.8s] DIRECTORY_TAG_xx         = ${DIRECTORY_TAG_xx}\n" "${TAG_xx}"
#      printf "(`date`) : [RSYNC_STARTUP] [%8.8s] TAG_xx                   = ${TAG_xx}\n"           "${TAG_xx}"
      if [ ${MODE} == "EXCLUDE_LAST_FILE" ]; then
        # printf "(`date`) : [RSYNC_STARTUP] [%8.8s]  -> EXECUTING : find ${LOCAL_ARADAQ_DIRECTORY} -name \"${FINDTAG_xx}*\" -not -type d -print | grep -v current | grep -v link | sort | head -n -1 >>${RSYNC_INCLUDE_LIST}\n" "${TAG_xx}"
         find ${LOCAL_ARADAQ_DIRECTORY} -name "${FINDTAG_xx}*" -not -type d -print | grep -v current | grep -v link | sort | head -n -1 >>${RSYNC_INCLUDE_LIST}.absolute_pathnames
      else
         #printf "(`date`) : [RSYNC_STARTUP] [%8.8s]  -> EXECUTING : find ${LOCAL_ARADAQ_DIRECTORY} -name \"${FINDTAG_xx}*\" -not -type d -print | grep -v current | grep -v link | sort >>${RSYNC_INCLUDE_LIST}\n" "${TAG_xx}"
         find ${LOCAL_ARADAQ_DIRECTORY} -name "${FINDTAG_xx}*" -not -type d -print | grep -v current | grep -v link | sort              >>${RSYNC_INCLUDE_LIST}.absolute_pathnames
      fi

#      if [ ! -s ${RSYNC_INCLUDE_LIST}.absolute_pathnames ]; then
 #        printf "(`date`) : [RSYNC_STARTUP] [%8.8s] FILE_COUNT (abs)         = empty file\n"                          "${TAG_xx}"
  #    else
   #      printf "(`date`) : [RSYNC_STARTUP] [%8.8s] FILE_COUNT (abs)         = `cat ${RSYNC_INCLUDE_LIST}.absolute_pathnames | wc -l`\n" "${TAG_xx}"
    #  fi

  done

  sed -e "s|^${LOCAL_ARADAQ_DIRECTORY}||" ${RSYNC_INCLUDE_LIST}.absolute_pathnames >${RSYNC_INCLUDE_LIST}.relative_pathnames

#  if [ ! -s ${RSYNC_INCLUDE_LIST}.relative_pathnames ]; then
 #    printf "(`date`) : [RSYNC_STARTUP] [%8.8s] FILE_COUNT (rel)         = empty file\n" "FINAL"
  #else
   #  printf "(`date`) : [RSYNC_STARTUP] [%8.8s] FILE_COUNT (rel)         = `cat ${RSYNC_INCLUDE_LIST}.relative_pathnames | wc -l`\n" "FINAL"
#  fi#

  safeecho "(`date`) : ********************************************************* DataPush RSYNC Files ********************************************************"

  if [ "${USE_IPv6}" == "YES" ]; then
     export REMOTE_ACCOUNT=`echo "${SERVER_ARCHIVE_DIRECTORY}" | ${AWK} -F\] '{print $1}' | sed -e 's|\[||'`
     export REMOTE_DIRECTORY=`echo "${SERVER_ARCHIVE_DIRECTORY}" | ${AWK} -F\] '{print $2}' | sed -e 's|^\:||'`
     ssh -6 ${REMOTE_ACCOUNT} "if [ ! -d ${REMOTE_DIRECTORY} ] ; then mkdir ${REMOTE_DIRECTORY} ; exit 1 ; else exit 2 ; fi"
     SSH_EXIT_CODE=$?
  else
     export REMOTE_ACCOUNT=`echo ${SERVER_ARCHIVE_DIRECTORY} | ${AWK} -F\: '{print $1}'`
     export REMOTE_DIRECTORY=`echo ${SERVER_ARCHIVE_DIRECTORY} | ${AWK} -F\: '{print $2}'`
     ssh ${REMOTE_ACCOUNT} "if [ ! -d ${REMOTE_DIRECTORY} ] ; then mkdir ${REMOTE_DIRECTORY} ; exit 1 ; else exit 2 ; fi"
     export SSH_EXIT_CODE=$?
  fi

  if [ ${SSH_EXIT_CODE} -eq 1 ]; then
     safeecho "(`date`) : [RSYNC_XFER] No remote directory ${REMOTE_DIRECTORY} - made it"
  elif [ ${SSH_EXIT_CODE} -eq 2 ]; then
     safeecho "(`date`) : [RSYNC_XFER] Found remote directory ${REMOTE_DIRECTORY}"
  else
     echo "(`date`) : [RSYNC_XFER] Problem making remote directory ${REMOTE_DIRECTORY}, ssh exit code = ${SSH_EXIT_CODE}"
     echo "(`date`) : so no go ... exitting -1"
     exit -1
  fi

  if [ -s "${RSYNC_INCLUDE_LIST}.relative_pathnames" ]; then

     if [ "${DEBUG_MODE}" == "NO" ]; then
        safeecho "(`date`) : [RSYNC_XFER] -> (delete mode) EXECUTING : ionice -c2 -n0 nice -n10 rsync -e 'ssh -c arcfour' --remove-source-files --files-from=${RSYNC_INCLUDE_LIST}.relative_pathnames -av ${LOCAL_ARADAQ_DIRECTORY} ${SERVER_ARCHIVE_DIRECTORY} >>${RSYNC_XFER_LOGFILE} 2>&1"
        ionice -c2 -n0 nice -n10 rsync -e 'ssh -c arcfour' --remove-source-files --files-from=${RSYNC_INCLUDE_LIST}.relative_pathnames -av ${LOCAL_ARADAQ_DIRECTORY}/ ${SERVER_ARCHIVE_DIRECTORY} >>${RSYNC_XFER_LOGFILE} 2>&1 
        export RSYNC_EXIT_CODE=$?
     else
        safeecho "(`date`) : [RSYNC_XFER] -> (debug mode) EXECUTING : ionice -c2 -n0 nice -n10 rsync -e 'ssh -c arcfour' --files-from=${RSYNC_INCLUDE_LIST}.relative_pathnames -av ${LOCAL_ARADAQ_DIRECTORY} ${SERVER_ARCHIVE_DIRECTORY} >>${RSYNC_XFER_LOGFILE} 2>&1"
        ionice -c2 -n0 nice -n10 rsync -e 'ssh -c arcfour'                       --files-from=${RSYNC_INCLUDE_LIST}.relative_pathnames -av ${LOCAL_ARADAQ_DIRECTORY}/ ${SERVER_ARCHIVE_DIRECTORY} >>${RSYNC_XFER_LOGFILE} 2>&1 
        export RSYNC_EXIT_CODE=$?
     fi

  else # end of IF-block for the file list has items in it, so do the rsync

     if [ "${DEBUG_MODE}" == "NO" ]; then
        safeecho "(`date`) : [RSYNC_XFER] -> (delete mode) EXECUTING : rsync not executed - file list is empty"
        export RSYNC_EXIT_CODE=0
     else
        safeecho "(`date`) : [RSYNC_XFER] -> (debug mode) EXECUTING : rsync not executed - file list is empty"
        export RSYNC_EXIT_CODE=0
     fi

  fi  # end of IF-block for the file list was empty, so don't bother with the rsync

  if [ ${RSYNC_EXIT_CODE} -eq 0 ]; then
     safeecho "(`date`) : [RSYNC_XFER] RSYNC_EXIT_CODE = ${RSYNC_EXIT_CODE} <- DataPush okay"
  else
     echo "(`date`) : [RSYNC_XFER] RSYNC_EXIT_CODE = ${RSYNC_EXIT_CODE} <- PROBLEM: DataPush experienced an error"
  fi

  if [ ${VERBOSE} -eq 0 ]; then
      rm ${RSYNC_INCLUDE_LIST}.absolute_pathnames
      rm ${RSYNC_INCLUDE_LIST}.relative_pathnames
  fi
  

  safeecho "(`date`) : ---------------------------------------------------------------------------------------------------------------------------------------"

  if [ ${VERBOSE} -gt 0 ]; then 
      echo "(`date`) : [DISK_RPT] START - AFTER TRANSFER DISK SPACE DUMP"
      DF_LINE=`df -h | grep Filesystem`
      printf "(`date`) : [DISK_RPT]       - %10.10s : %s\n" "Drive" "${DF_LINE}"
      
      for ITEM in /home    \
          /data    \
          /var/run \
          /tmp     \
	  ; do
	  DF_LINE=`df -h | grep -e "${ITEM}$"`
	  printf "(`date`) : [DISK_RPT]       - %10.10s : %s\n" "${ITEM}" "${DF_LINE}"
      done
      echo "(`date`) : [DISK_RPT] __END - AFTER TRANSFER DISK SPACE DUMP"
  fi      
  
  safeecho "(`date`) : [RSYNC_WAIT] going to sleep for ${SLEEPTIME} seconds"
  safeecho "(`date`) : ---------------------------------------------------------------------------------------------------------------------------------------"
  sleep $SLEEPTIME

done # end of INFINITE WHILE-loop for doing the rsync

#  8 - out of the infinite loop ... should be impossible to reach here; actually

safeecho "(`date`) : [_DA_END_] END OF SCRIPT - $0 (exit 0)"
exit 0

