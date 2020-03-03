#!/bin/bash
port=$2

while [[ $(ss -tulpn | grep :$port) != "" ]]; do
    port=$(($port+1))
done
echo "Trying $1 with 1 numloops"
./collectdata.sh $1 1 "test-$1-1.txt" $port
port=$(($port+1))
while [[ $(ss -tulpn | grep :$port) != "" ]]; do
    port=$(($port+1))
done
echo "Trying $1 with 100 numloops"
./collectdata.sh $1 100 "test-$1-100.txt" $port
port=$(($port+1))
while [[ $(ss -tulpn | grep :$port) != "" ]]; do
    port=$(($port+1))
done
echo "Trying $1 with 1000 numloops"
./collectdata.sh $1 1000 "test-$1-1000.txt" $port
port=$(($port+1))
while [[ $(ss -tulpn | grep :$port) != "" ]]; do
    port=$(($port+1))
done
echo "Trying $1 with 10000 numloops"
./collectdata.sh $1 10000 "test-$1-10000.txt" $port
port=$(($port+1))
while [[ $(ss -tulpn | grep :$port) != "" ]]; do
    port=$(($port+1))
done
echo "Trying $1 with 100000 numloops"
./collectdata.sh $1 100000 "test-$1-100000.txt" $port
port=$(($port+1))
while [[ $(ss -tulpn | grep :$port) != "" ]]; do
    port=$(($port+1))
done
echo "Trying $1 with 500000 numloops"
./collectdata.sh $1 500000 "test-$1-500000.txt" $port