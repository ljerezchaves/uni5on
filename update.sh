#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
reset=$(tput sgr0)
bold=$(tput bold)
normal=$(tput sgr0)

BACKGROUND=0
PROGNAME="svelte"
BASEDIR="/local1/luciano"
SIMDIR="svelte-simulator"
LIBDIR="ofsoftswitch13-gtp"
MACHINE_LIST="atlas castor clio demeter esculapio heracles hercules hestia hydra kratos morfeu pollux satiros tetis"

function PrintHelp () {
  echo "Usage: $0 <action> [command [options]]"
  echo
  echo "Available ${bold}actions${normal}:"
  echo "  ${bold}--all [command [options]]${normal}:"
  echo "    Silently execute the command on all LRC machines."
  echo
  echo "  ${bold}--local [command [options]]${normal}:"
  echo "    Execute the command on this local machine."
  echo
  echo "  ${bold}--info${normal}:"
  echo "    List current configured information."
  echo
  echo "  ${bold}--help${normal}:"
  echo "    Print this help message and exit."
  echo
  echo "Available ${bold}command${normal}:"
  echo "  ${bold}pull-logs${normal}:"
  echo "    Pull changes for the ${BASEDIR}/${PROGNAME}-logs git repository."
  echo
  echo "  ${bold}pull-sim${normal}:"
  echo "    Pull changes for the ${BASEDIR}/${SIMDIR} git repository."
  echo
  echo "  ${bold}repo-status${normal}:"
  echo "    Show status for the ${BASEDIR}/${SIMDIR} git repository."
  echo
  echo "  ${bold}compile-lib${normal}:"
  echo "    Compile the ${BASEDIR}/${LIBDIR} library."
  echo
  echo "  ${bold}compile-sim <threads>${normal}:"
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
      pull-logs)
        cd ${PROGNAME}-logs/
        git pull
        cd ../
      ;;

      pull-sim)
        cd ${SIMDIR}
        git pull --recurse-submodules && git submodule update --recursive
        cd ../
      ;;

      repo-status)
        cd ${SIMDIR}
        echo "${red}"svelte:"${normal}"
        git log HEAD^..HEAD
        cd src/ofswitch13
        echo "${red}"ofswitch13:"${normal}"
        git log HEAD^..HEAD      
        cd ../../${LIBDIR}
        echo "${red}"ofsoftswitch13-gtp:"${normal}"
        git log HEAD^..HEAD
        cd ../../
      ;;

      compile-sim)
        if [ $# -lt 3 ];
        then
          echo "Number of threads missing."
          PrintHelp
        fi;
        THREADS=$3
        cd ${SIMDIR}
        ./waf -j${THREADS}
        cd ../
      ;;

      compile-lib)
        cd ${SIMDIR}/${LIBDIR}
        git clean -fxd
        ./boot.sh
        ./configure --enable-ns3-lib
        make
        cd ../
        rm build/libns3.28-ofswitch13-*.so
        cd ../
      ;;

      *)
        echo "Invalid command"
        PrintHelp
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
          if [ ${BACKGROUND} -eq 0 ];
          then
            ssh -t ${MACHINE} $0 --local ${COMMAND} $3
          else
            ssh -t ${MACHINE} $0 --local ${COMMAND} $3 &>> /dev/null &
          fi;
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

  *)
    echo "Invalid action"
    PrintHelp
  ;;
esac

exit 0;
