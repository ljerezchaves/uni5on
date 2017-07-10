#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
reset=$(tput sgr0)
bold=$(tput bold)
normal=$(tput sgr0)

PROGNAME="sdmn"
SIMPATH="/local1/luciano/sdmn-simulator"
MACHINE_LIST="atlas castor clio cronos demeter dionisio eco eolo esculapio heracles hercules hermed hestia hydra kratos morfeu nix pollux satiros tetis zeus"

function PrintHelp () {
  echo "Usage: $0 --action [ARGS]"
  echo
  echo "Available actions:"
  echo "  ${bold}--local [rev]${normal}:"
  echo "    Update and compile the local ${PROGNAME} source code at ${SIMPATH}."
  echo "    Arguments: rev The optional mercurial revision to checkout."
  echo
  echo "  ${bold}--all [rev]${normal}:"
  echo "    Update and compile the ${PROGNAME} source code on all LRC machines."
  echo "    Arguments: rev The optional mercurial revision to checkout."
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
    hg pull
    if [ $# -eq 2 ];
    then
      hg up -r $2
    else
      hg up
    fi;
    ./waf -j2
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
