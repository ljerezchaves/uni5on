#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
bold=$(tput bold)
normal=$(tput sgr0)

function PrintHelp () {
  echo "Usage: $0 <progname> <action>"
  echo
  echo "Available ${bold}actions${normal}:"
  echo "  ${bold}--single <seed> <prefix> [arg_list]${normal}:"
  echo "    Starts a single simulation with specific prefix and seed number."
  echo "    Arguments: seed     the seed number to use."
  echo "               prefix   the topology simulation prefix."
  echo "               arg_list the list of ns3 command line arguments (optional)."
  echo
  echo "  ${bold}--sequentialSeed <first_seed> <last_seed> <prefix> [arg_list]${normal}:"
  echo "    Starts sequential simulations with specific prefix, one for each seed in"
  echo "    given closed interval."
  echo "  ${bold}--parallelSeed   <first_seed> <last_seed> <prefix> [arg_list]${normal}:"
  echo "    Starts parallel background simulations with specific prefix, one for each"
  echo "    seed in given closed interval."
  echo "    Arguments: first_seed  the first seed number to use."
  echo "               last_seed   the last seed number to use."
  echo "               prefix      the topology simulation prefix."
  echo "               arg_list    the list of ns3 command line arguments (optional)."
  echo
  echo "  ${bold}--sequentialPrefix <seed> \"<prefix_list>\" [arg_list]${normal}"
  echo "    Starts sequential simulations with specific seed number, one for each prefix"
  echo "    in given list."
  echo "  ${bold}--parallelPrefix   <seed> \"<prefix_list>\" [arg_list]${normal}"
  echo "    Starts parallel background simulations with specific seed number, one for"
  echo "    each prefix in given list."
  echo "    Arguments: seed          the seed number to use."
  echo "               \"prefix_list\" the list of topology simulation prefixes."
  echo "               arg_list      the list of ns3 command line arguments (optional)."
  echo
  echo "  ${bold}--parallelSeedPrefix <first_seed> <last_seed> \"<prefix_list>\" [arg_list]${normal}"
  echo "    For each seed in given closed interval, starts parallel background"
  echo "    sequential simulations for the prefix in given list."
  echo "    Arguments: first_seed     the first seed number to use."
  echo "               last_seed      the last seed number to use."
  echo "               \"prefix_list\"  the list of topology simulation prefixes."
  echo "               arg_list       the list of ns3 command line arguments (optional)."
  echo
  echo "  ${bold}--abort${normal}"
  echo "    Abort all running simulations."
  exit 1
}

# Parsing positional arguments
if [ $# -lt 1 ];
then
  echo "Missing <progname> argument"
  PrintHelp
fi;
PROGNAME=$1
shift

if [ $# -lt 1 ];
then
  echo "Missing <action> argument"
  PrintHelp
fi;
ACTION=$1
shift

case "${ACTION}" in
  --single)
    # Parsing positional arguments
    if [ $# -lt 2 ];
    then
      echo "Missing required argument"
      PrintHelp
    fi;
    SEED=$1
    PREFIX=$2
    shift 2

    COMMAND="./waf --run=\"${PROGNAME} --RngRun=${SEED} --Prefix=${PREFIX} $@\""
    PREFIXBASENAME=$(basename ${PREFIX})
    OUTFILE=$(mktemp -p /tmp ${PROGNAME}-${PREFIXBASENAME}-${SEED}-tmpXXX)
    echo "${COMMAND}" > ${OUTFILE}

    echo "${green}[Start]${normal} ${COMMAND}"
    eval ${COMMAND} &>> ${OUTFILE}

    # Check for success
    if [ $? -ne 0 ];
    then
      echo "ERROR" >> ${OUTFILE}
      echo "${red}[Error]${normal} ${COMMAND}"
      echo "${yellow}===== Output file content =====${normal}"
      cat ${OUTFILE}
      echo "${yellow}===== Output file content =====${normal}"
    fi
  ;;

  --parallelSeed | --sequentialSeed)
    # Parsing positional arguments
    if [ $# -lt 3 ];
    then
      echo "Missing required argument"
      PrintHelp
    fi;
    FIRSTSEED=$1
    LASTSEED=$2
    PREFIX=$3
    shift 3

    for ((SEED=${FIRSTSEED}; ${SEED} <= ${LASTSEED}; SEED++))
    do
      if [ "${ACTION}" == "--parallelSeed" ];
      then
        $0 ${PROGNAME} --single ${SEED} ${PREFIX} "$@" &
      else
        $0 ${PROGNAME} --single ${SEED} ${PREFIX} "$@"
      fi;
      sleep 0.5
    done
  ;;

  --parallelPrefix | --sequentialPrefix)
    # Parsing positional arguments
    if [ $# -lt 2 ];
    then
      echo "Missing required argument"
      PrintHelp
    fi;
    SEED=$1
    PREFIXLIST=$2
    shift 2

    for PREFIX in ${PREFIXLIST};
    do
      if [ "${ACTION}" == "--parallelPrefix" ];
      then
        $0 ${PROGNAME} --single ${SEED} ${PREFIX} "$@" &
      else
        $0 ${PROGNAME} --single ${SEED} ${PREFIX} "$@"
      fi;
      sleep 0.5
    done
  ;;

  --parallelSeedPrefix)
    # Parsing positional arguments
    if [ $# -lt 3 ];
    then
      echo "Missing required argument"
      PrintHelp
    fi;
    FIRSTSEED=$1
    LASTSEED=$2
    PREFIXLIST=$3
    shift 3

    for ((SEED=${FIRSTSEED}; ${SEED} <= ${LASTSEED}; SEED++))
    do
      $0 ${PROGNAME} --sequentialPrefix ${SEED} "${PREFIXLIST}" "$@" &
      sleep 0.5
    done
  ;;

  --stop)
    echo "${red}You are about to stop all ns3 simulations in this machine.${normal}"
    echo -n "Are you sure you want to continue? (y/N) "
    read ANSWER
    if [ "${ANSWER}" == "y" ];
    then
      echo "Stopping..."
      killall $0 ${PROGNAME}
    else
      echo "Aborted."
    fi
  ;;

  *)
    echo "Invalid <action> argument"
    PrintHelp
  ;;
esac

exit 0;

