#!/bin/bash

function setupDAQ()
{
    ${ARA_DAQ_DIR}/scripts/progFwStartDAQ.sh
}

function startDAQ()
{
    ${ARA_DAQ_DIR}/scripts/stopAraSoft.sh
    ${ARA_DAQ_DIR}/scripts/startAraSoftWithTimeRecord.sh
}

function stopDAQ(){
    ${ARA_DAQ_DIR}/scripts/stopAraSoft.sh
}

function checkDAQ(){
    echo "acqd.log last 30 lines ----------------------"
    tail -n 30 ${ARA_LOG_DIR}/acqd.log 
    echo "arad.log last 30 lines ----------------------"
    tail -n 30 ${ARA_LOG_DIR}/arad.log 
}


#tools for araRunBuildDaemon
function startRunBuilder(){
    nohup ~/miniATRI/processingDaemon/araRunBuildDaemon > ${ARA_LOG_DIR}/araRunBuildLog.txt 2>&1 &
}



function checkDataQueue(){
    ls ~/miniATRI/LogFiles/araRunBuildDaemon/queue/
}

