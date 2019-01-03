#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
bold=$(tput bold)
normal=$(tput sgr0)

SIMNAME="svelte"
BASEDIR="/local1/luciano"
SIMDIR="${BASEDIR}/svelte-simulator"
LIBDIR="${SIMDIR}/ofsoftswitch13-gtp"
LOGDIR="${SIMDIR}/logs"
MACHINELIST="atlas castor clio demeter esculapio heracles hercules hestia hydra kratos morfeu pollux satiros tetis zeus"

function PrintHelp () {
  echo "Usage: $0 <action>"
  echo
  echo "Available ${bold}actions${normal}:"
  echo "  ${bold}--all <-b|-i> <command>${normal}:"
  echo "    Execute the command on all remote machines in -b (background) or -i (interactive) mode."
  echo
  echo "  ${bold}--help${normal}:"
  echo "    Print this help message and exit."
  echo
  echo "  ${bold}--info${normal}:"
  echo "    List current configured information."
  echo
  echo "  ${bold}--local <command>${normal}:"
  echo "    Execute the command on this local machine."
  echo
  echo "Available ${bold}commands${normal}:"
  echo "  ${bold}build-lib${normal}:"
  echo "    Compile the switch library [${LIBDIR}]."
  echo
  echo "  ${bold}build-sim [waf args]${normal}:"
  echo "    Compile the simulator with optional waf arguments [${SIMDIR}]."
  echo
  echo "  ${bold}checkout-sim <ref>${normal}:"
  echo "    Checkout the simulator repository using the git reference [${SIMDIR}]."
  echo
  echo "  ${bold}pull-logs${normal}:"
  echo "    Pull changes for the logs output git repository [${LOGDIR}]."
  echo
  echo "  ${bold}pull-sim${normal}:"
  echo "    Pull changes for the simulator git repository [${SIMDIR}]."
  echo
  echo "  ${bold}stats-sim${normal}:"
  echo "    Show the status for the simulator git repository [${SIMDIR}]."
  exit 1
}

# Parsing positional arguments
if [ $# -lt 1 ];
then
  echo "Missing <action> argument"
  PrintHelp
fi;
ACTION=$1
shift

case "${ACTION}" in
  --help)
    PrintHelp
  ;;

  --info)
    echo "${green}Simulator name:  ${normal}${SIMNAME}"
    echo "${green}Simulator path:  ${normal}${SIMDIR}"
    echo "${green}Switch lib path: ${normal}${LIBDIR}"
    echo "${green}Log output path: ${normal}${LOGDIR}"
    echo "${green}LRC machines:    ${normal}${MACHINELIST}"
  ;;

  --all)
    # Parsing positional arguments
    if [ $# -lt 1 ];
    then
      echo "Missing <-b|-i> argument"
      PrintHelp
    fi;
    if [ "$1" != "-b" ] && [ "$1" != "-i" ];
    then
      echo "Invalid <-b|-i> argument"
      PrintHelp
    fi;
    MODE=$1
    shift

    for MACHINE in ${MACHINELIST};
      do
        echo "${green}Connecting to ${MACHINE}:${normal}"
        ssh -q ${MACHINE} exit
        if [ $? -eq 0 ];
        then
          EXECMD="ssh -t ${MACHINE} $0 --local $@"
          if [ "${MODE}" == "-b" ];
          then
            EXECMD+=" &>> /dev/null &"
          fi;
          echo "Executing command: ${EXECMD}"
          eval ${EXECMD}
          sleep 0.5
        else
          echo "${red}Connection to ${MACHINE} failed${normal}"
        fi;
        echo
      done
  ;;

  --local)
    # Parsing positional arguments
    if [ $# -lt 1 ];
    then
      echo "Missing <command> argument"
      PrintHelp
    fi;
    COMMAND=$1
    shift

    OLDPWD=$(pwd)
    case "${COMMAND}" in

      build-lib)
        cd ${LIBDIR}
        git clean -fxd
        ./boot.sh
        ./configure --enable-ns3-lib
        make

        cd ${SIMDIR}
        rm build/lib/libns3*-ofswitch13-*.so
      ;;

      build-sim)
        cd ${SIMDIR}
        ./waf $@
      ;;

      checkout-sim)
        cd ${SIMDIR}
        git checkout $@ && git submodule update --recursive
      ;;

      pull-logs)
        cd ${LOGDIR}
        git pull
      ;;

      pull-sim)
        cd ${SIMDIR}
        git pull --recurse-submodules && git submodule update --recursive
      ;;

      stats-sim)
        cd ${SIMDIR}
        echo "${yellow}** Simulator${normal}"
        git log -n1
        echo

        cd src/ofswitch13
        echo "${yellow}** Switch module:${normal}"
        git log -n1
        echo

        cd ${LIBDIR}
        echo "${yellow}** Switch library::${normal}"
        git log -n1
        echo
        echo
      ;;

      *)
        echo "Invalid <command> argument"
        PrintHelp
      ;;
    esac
    cd ${OLDPWD}
  ;;

  *)
    echo "Invalid <action> argument"
    PrintHelp
  ;;
esac

exit 0;
