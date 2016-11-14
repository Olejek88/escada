#!/bin/sh

path="/home/user/dk/"
process="kernel"		#searched process
delay=10s			#watch every...
log=${path}start.log		#logfile

echo `date` " [ Script Started ]" >> $log

trap 'killall -9 start.sh ; start.sh' 1 2 3 4 5 6 7 8 10 12 13 14 15 16

while true;  do

    sleep $delay
    
    res=$(ps awx | grep "$process" | grep -v grep)

    if [ "$res" ] ; then 
	echo "ok" > /dev/null
    else
	echo "no" > /dev/null
	
	((${path}${process}) & ) > /dev/null #2> /dev/null
	echo `date` " [ Started ]" >> $log
    fi

done