#!/bin/bash

#Make sure that we source the setupScript
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $SCRIPT_DIR/../setupAraDaq.sh

if [ -e ${ARA_LOG_DIR}/acqd.log ]
then
    rm ${ARA_LOG_DIR}/acqd.log
fi

ACQD_LOG=${ARA_LOG_DIR}/acqd_`date +%d%b%y_%I%P.%M`.log

nohup ${ARA_DAQ_DIR}/bin/ARAAcqd > ${ACQD_LOG} 2>&1 &

ln -s ${ACQD_LOG} ${ARA_LOG_DIR}/acqd.log