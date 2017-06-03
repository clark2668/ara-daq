#!/bin/bash

#Make sure that we source the setupScript
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $SCRIPT_DIR/../setupAraDaq.sh


if [ -e ${ARA_LOG_DIR}/arad.log ]
then
    rm ${ARA_LOG_DIR}/arad.log
fi

ARAD_LOG=${ARA_LOG_DIR}/arad_`date +%d%b%y_%I%P.%M`.log

nohup ${ARA_DAQ_DIR}/bin/ARAd >  ${ARAD_LOG} 2>&1 &

ln -s ${ARAD_LOG} ${ARA_LOG_DIR}/arad.log