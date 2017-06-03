ARAdPID=`ps x | grep ARAd | grep -v grep | grep -v emacs | awk '{print $1}'`
ARAAcqdPID=`(ps x | grep ARAAcqd | grep -v grep | grep -v emacs | awk '{print $1}')`

if [ "$ARAdPID" = "" ] 
    then
    echo ARAd not running
else
    killall -9 ARAd
    echo killed ARAd
fi

if [ "$ARAAcqdPID" = "" ] 
    then
    echo ARAcqd not running
else
    killall -9 ARAAcqd
    echo killed ARAAcqd
fi


