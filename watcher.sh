#!/bin/sh

trap "" 1 2 3 4 5 6 7 8 10 12 13 14 15 16

path="/home/user/dk/"
process="kernel"		#searched process
delay=10s			#watch every...
log=${path}start.log		#logfile

prepare(){
    # Kill all processes with this name exclude this
    pids=`ps --no-headers -o pid -C watcher.sh`
    for pid in $pids; do
	if [ $pid != $$ ]; then
	kill -9 $pid 1>/dev/null 2>/dev/null 
	echo `date` " [ Script with pid $pid Killed]" >> $log
        fi
    done
}

start(){
    #Start watching
    echo `date` " [ Script Started ]" >> $log

    while true;  do

	sleep $delay
    
        res=$(ps awx | grep "$process" | grep -v grep)

	if [ "$res" ] ; then 
	    echo "ok" > /dev/null
        else
	    echo "no" > /dev/null
	
	    ${path}${process} 1>/dev/null 2>/dev/null
	    echo `date` " [ Started ]" >> $log
        fi
    done    
}

stop(){
    echo `date` " [ Script Stopped ]" >> $log
    killall -9 ${process} 1>/dev/null 2>/dev/null
    killall -9 watcher.sh 1>/dev/null 2>/dev/null
}

case "$1" in
    start)
	prepare;
	start; 
	;;
	
    stop)
	stop;
	;;
    
    *)
	echo "please use $0 {start|stop}"
	;;
esac
