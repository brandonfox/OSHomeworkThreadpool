#!/bin/bash

./server $4 $1 $2 $3 &
noclients=$1
while [[ $noclients -gt 0 ]];
do 
    noclients=$(($noclients-1))
    ./clientconnect.sh $4 &
done
sleep 25
kill $(jobs -p)