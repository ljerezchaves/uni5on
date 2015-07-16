#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: ./simulate.sh runs \"args\""
    echo "Where: runs is the number of times to execute the simulation."
    echo "      \"args\" can be any other epc-of command line argument."
    exit 0
fi;

RUNS=$1
ARGS=$2

OUT=$(mktemp --suffix=-epcof)
echo "$0 $1 $2" > $OUT
echo "Check output at $OUT"

for (( i=1; i<=$RUNS; i++))
do
  ./waf --run="epc-of --RngRun=$i $ARGS" &>> $OUT
done

exit 0;


