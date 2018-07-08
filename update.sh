#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
reset=$(tput sgr0)
bold=$(tput bold)
normal=$(tput sgr0)

PROGNAME="svelte"
SIMPATH="/local1/luciano/svelte-simulator"
MACHINE_LIST="atlas castor clio cronos demeter eolo esculapio heracles hercules hestia hydra kratos morfeu pollux satiros tetis"

function PrintHelp () {
  echo "Usage: $0 --action [ARGS]"
  echo
  echo "Available actions:"
  echo "  ${bold}--local [args]${normal}:"
  echo "    Checkout and compile the local ${PROGNAME} source code at ${SIMPATH}."
  echo "    Arguments: The optional git checkout parameters."
  echo
  echo "  ${bold}--all [args]${normal}:"
  echo "    Update and compile the ${PROGNAME} source code on all LRC machines."
  echo "    Arguments: The optional git checkout parameters."
  exit 1
}

# Test for action argument
if [ $# -lt 1 ];
then
  PrintHelp
fi;

ACTION=$1
case "${ACTION}" in
  --local)
    OLDPWD=$(pwd)
    
    cd ${SIMPATH}/sim
    git pull
    cd ../
    git fetch
    git chechout $2
    ./waf
    cd ${OLDPWD}
  ;;

  --all)
  for MACHINE in ${MACHINE_LIST};
    do
      ssh ${MACHINE} $0 --local $2 &>> /dev/null & 
      sleep 0.5
    done
  ;;
esac

exit 0;
