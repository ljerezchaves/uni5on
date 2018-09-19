#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
reset=$(tput sgr0)
bold=$(tput bold)
normal=$(tput sgr0)

PROGNAME="svelte"
BASEDIR="/local1/luciano"
SIMDIR="svelte-simulator"
MACHINE_LIST="atlas castor clio demeter esculapio heracles hercules hestia hydra kratos morfeu pollux satiros tetis"

function PrintHelp () {
  echo "Usage: $0 <action> [command]"
  echo
  echo "Where ${bold}action${normal} can be:"
  echo "  ${bold}--all [command]${normal}:"
  echo "    Silently execute the command on all LRC machines."
  echo
  echo "  ${bold}--local [command]${normal}:"
  echo "    Execute the command on this local machine."
  echo
  echo "  ${bold}--info${normal}:"
  echo "    List current configure information."
  echo
  echo "  ${bold}--help${normal}:"
  echo "    Print this help message and exit."
  echo
  echo "Where ${bold}command${normal} can be:"
  echo "  ${bold}up-logs${normal}:"
  echo "    Update the ${BASEDIR}/${PROGNAME}-logs git repository."
  echo
  echo "  ${bold}up-sim${normal}:"
  echo "    Update the ${BASEDIR}/${SIMDIR} git repository."
  echo
  echo "  ${bold}compile [threads]${normal}:"
  echo "    Compile the ${BASEDIR}/${SIMDIR} simulator with a custom number of threads."

  exit 1
}

# Test for action argument
if [ $# -lt 1 ];
then
  PrintHelp
fi;

WHERE=$1
COMMAND=$2
case "${WHERE}" in
  --local)
    OLDPWD=$(pwd)
    cd ${BASEDIR}
    case "${COMMAND}" in
      up-logs)
        cd ${PROGNAME}-logs/
        git pull
        cd ../
      ;;

      up-sim)
        cd ${SIMDIR}
        git pull --recurse-submodules && git submodule update --recursive
        cd ../
      ;;

      compile)
        if [ $# -lt 3 ];
        then
          PrintHelp
        fi;
        THREADS=$3
        cd ${SIMDIR}
          ./waf -j${THREADS}
        cd ../
      ;;
    esac
    cd ${OLDPWD}
  ;;

  --all)
    if [ $# -lt 2 ];
    then
      PrintHelp
    fi;
    for MACHINE in ${MACHINE_LIST};
      do
        echo "${green}${MACHINE}${normal}"
        ssh -q ${MACHINE} exit
        if [ $? -eq 0 ];
        then
          ssh ${MACHINE} $0 --local ${COMMAND} &>> /dev/null &
          sleep 0.5
        fi;
      done
  ;;

  --help)
    PrintHelp
  ;;

  --info)
    echo "Program name: ${PROGNAME}"
    echo "Simulation path: ${BASEDIR}/${SIMDIR}"
    echo "Machine list: ${MACHINE_LIST}"
  ;;
esac

exit 0;
