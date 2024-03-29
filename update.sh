#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
bold=$(tput bold)
normal=$(tput sgr0)

SIMNAME="uni5on"
BASEDIR="/local1/luciano"
SIMDIR="${BASEDIR}/${SIMNAME}"
LIBDIR="${SIMDIR}/ofsoftswitch13-gtp"
LOGDIR="${SIMDIR}/logs/thesis"
MACHINELIST="atlas castor clio esculapio heracles hercules satiros tetis"

function PrintHelp () {
  echo "Usage: $0 <action>"
  echo
  echo "Available ${bold}actions${normal}:"
  echo "  ${bold}--exec <target> <mode> <command>${normal}:"
  echo "    Execute the command."
  echo
  echo "  ${bold}--help${normal}:"
  echo "    Print this help message and exit."
  echo
  echo "  ${bold}--info${normal}:"
  echo "    List current configured information."
  echo
  echo "Available ${bold}targets${normal}:"
  echo -e "  ${bold}all${normal}:  \tExecute the command on all remote machines."
  echo -e "  ${bold}local${normal}:\tExecute the command on this local machine."
  echo -e "  ${bold}name${normal}: \tExecute the command on the named remote machine."
  echo
  echo "Available ${bold}modes${normal}:"
  echo -e "  ${bold}bg${normal}:   \tExecute the command in background mode."
  echo -e "  ${bold}fg${normal}:   \tExecute the command in foreground mode."
  echo
  echo "Available ${bold}commands${normal}:"
  echo -e "  ${bold}make [args]${normal}:     \tExecute the make [args] command on the ${SIMDIR} directory."
  echo -e "  ${bold}waf [args]${normal}:      \tExecute the ./waf [args] command on the ${SIMDIR} directory."
  echo -e "  ${bold}git-sim [args]${normal}:  \tExecute the git [args] command on the ${SIMDIR} directory."
  echo -e "  ${bold}git-log [args]${normal}:  \tExecute the git [args] command on the ${LOGDIR} directory."
  echo -e "  ${bold}pull${normal}:            \tRecursively pull changes on the ${SIMDIR} directory."
  echo -e "  ${bold}head${normal}:            \tShow the HEAD commit log for all git repositories."
  exit 0
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
  --exec)
    # Parsing positional arguments
    if [ $# -lt 1 ];
    then
      echo "Missing <target> argument"
      PrintHelp
    fi;
    TARGET=$1
    shift

    if [ $# -lt 1 ];
    then
      echo "Missing <mode> argument"
      PrintHelp
    fi;
    MODE=$1
    shift

    if [ "${MODE}" != "fg" ] && [ "${MODE}" != "bg" ];
    then
      echo "Invalid <mode> argument"
      PrintHelp
    fi;

    if [ $# -lt 1 ];
    then
      echo "Missing <command> argument"
      PrintHelp
    fi;
    COMMAND=$1
    shift

    case "${TARGET}" in
      local)
        OLDPWD=$(pwd)
        case "${COMMAND}" in
          make)
            cd ${SIMDIR}
            if [ $? -eq 0 ];
            then
              make $@
            fi;
          ;;

          waf)
            cd ${SIMDIR}
            if [ $? -eq 0 ];
            then
              ./waf $@
            fi;
          ;;

          git-sim)
            cd ${SIMDIR}
            if [ $? -eq 0 ];
            then
              git $@
            fi;
          ;;

          git-log)
            cd ${LOGDIR}
            if [ $? -eq 0 ];
            then
              git $@
            fi;
          ;;

          pull)
            cd ${SIMDIR}
            if [ $? -eq 0 ];
            then
              git pull --recurse-submodules && git submodule update --recursive
            fi;
          ;;

          head)
            cd ${SIMDIR}
            if [ $? -eq 0 ];
            then
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

              cd ${LOGDIR}
              echo "${yellow}** Output logs::${normal}"
              git log -n1
              echo
            fi;
            echo
          ;;

          *)
            echo "Invalid <command> [arguments]"
            PrintHelp
          ;;
        esac
        cd ${OLDPWD}
      ;;

      all)
        for MACHINE in ${MACHINELIST};
        do
          $0 --exec ${MACHINE} ${MODE} ${COMMAND} $@
        done
      ;;

      *)
        echo "${green}Connecting to ${TARGET}:${normal}"
        ssh -q ${TARGET} exit
        if [ $? -eq 0 ];
        then
          EXECMD="ssh -t ${TARGET} $0 --exec local ${MODE} ${COMMAND} $@"
          if [ "${MODE}" == "bg" ];
          then
            EXECMD+=" &>> /dev/null &"
          fi;
          echo "Executing command: ${EXECMD}"
          eval ${EXECMD}
          sleep 0.5
        else
          echo "${red}Connection to ${TARGET} failed${normal}"
        fi;
        echo
      ;;
    esac
  ;;

  --help)
    PrintHelp
  ;;

  --info)
    echo "${yellow}Simulator name:  ${normal}${SIMNAME}"
    echo "${yellow}Simulator path:  ${normal}${SIMDIR}"
    echo "${yellow}Switch lib path: ${normal}${LIBDIR}"
    echo "${yellow}Log output path: ${normal}${LOGDIR}"
    echo "${yellow}LRC machines:    ${normal}${MACHINELIST}"
  ;;

  *)
     echo "Invalid <action> argument"
     PrintHelp
   ;;
esac

exit 0;
