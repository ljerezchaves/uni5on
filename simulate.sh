#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: ./simulate.sh seed runs \"args\""
    echo "Where: seed is the first seed number to use."
    echo "       runs is the number of times to execute the simulation."
    echo "      \"args\" can be any other epc-of command line argument."
    exit 0
fi;

MIN=$1
RUNS=$2
ARGS=$3
let MAX=MIN+RUNS

for ((i=$MIN; i<$MAX; i++))
do
  OUT=$(mktemp --suffix=-epcof)
  echo "$0 $1 $2 $3" > $OUT
  ./waf --run="epc-of --RngRun=$i $ARGS" &>> $OUT &
done
wait

exit 0;

