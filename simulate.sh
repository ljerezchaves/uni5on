#!/bin/bash

# Output text format
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
reset=$(tput sgr0)
bold=$(tput bold)
normal=$(tput sgr0)

PROGNAME="sdmn"

function PrintHelp () {
  echo "Usage: $0 --action [ARGS]"
  echo
  echo "Available actions:"
  echo "  ${bold}--single seed prefix [\"args\"]${normal}:" 
  echo "    Starts a single simulation with specific prefix and seed number."
  echo "    Arguments: seed     is the seed number to use."
  echo "               prefix   is the topology simulation prefix."
  echo "               [\"args\"] can be any command line argument (optional)."
  echo
  echo "  ${bold}--sequentialSeed firstSeed lastSeed prefix [\"args\"]${normal}:"
  echo "    Starts sequential simulations with specific prefix, one for each seed in"
  echo "    given closed interval."
  echo "  ${bold}--parallelSeed firstSeed lastSeed prefix [\"args\"]${normal}:"
  echo "    Starts parallel background simulations with specific prefix, one for each" 
  echo "    seed in given closed interval."
  echo "    Arguments: firstSeed is the first seed number to use."
  echo "               lastSeed  is the last seed number to use."
  echo "               prefix    is the topology simulation prefix."
  echo "               [\"args\"]  can be any command line argument (optional)."
  echo
  echo "  ${bold}--sequentialPrefix seed \"prefixes\" [\"args\"]${normal}"
  echo "    Starts sequential simulations with specific seed number, one for each prefix"
  echo "    in given list."
  echo "  ${bold}--parallelPrefix seed \"prefixes\" [\"args\"]${normal}"
  echo "    Starts parallel background simulations with specific seed number, one for"
  echo "    each prefix in given list."
  echo "    Arguments: seed       is the seed number to use."
  echo "               \"prefixes\" is the list of topology simulation prefixes."
  echo "               [\"args\"]   can be any command line argument (optional)."
  echo
  echo "  ${bold}--parallelSeedPrefix firstSeed lastSeed \"prefixes\" [\"args\"]${normal}"
  echo "    For each seed in given closed interval, starts parallel background"
  echo "    sequential simulations for the prefix in given list."
  echo "    Arguments: firstSeed  is the first seed number to use."
  echo "               lastSeed   is the last seed number to use."
  echo "               \"prefixes\" is the list of topology simulation prefixes."
  echo "               [\"args\"]   can be any command line argument (optional)."
  echo
  echo "  ${bold}--abort${normal}"
  echo "    Abort all running simulations."
  exit 1
}

# Test for action argument
if [ $# -lt 1 ]; 
then
  PrintHelp
fi;

ACTION=$1
case "${ACTION}" in 
  --single)
    # Test for arguments
    if [ $# -lt 3 ] || [ $# -gt 4 ]; 
    then 
      PrintHelp 
    fi;

    SEED=$2
    PREFIX=$3
    ARGS=$4
    
    COMMAND="./waf --run=\"${PROGNAME} --RngRun=${SEED} --prefix=${PREFIX} ${ARGS}\""
    OUTFILE=$(mktemp --suffix=-${PROGNAME})
    echo "${COMMAND}" > ${OUTFILE}
    
    echo "${green}[Start]${reset} ${COMMAND}"
    eval ${COMMAND} &>> ${OUTFILE}
    
    # Check for success
    if [ $? -ne 0 ];
    then
      echo "${red}[Error]${reset} ${COMMAND}"
      echo "${yellow}===== Output file content ===== (${OUTFILE})${reset}"
      cat ${OUTFILE}
      echo "${yellow}===== Output file content =====${reset}"
    fi
  ;;

  --parallelSeed)
    # Test for arguments
    if [ $# -lt 4 ] || [ $# -gt 5 ]; 
    then 
      PrintHelp 
    fi;
  
    FIRST=$2
    LAST=$3
    PREFIX=$4
    ARGS=$5
    for ((SEED=${FIRST}; ${SEED} <= ${LAST}; SEED++))
    do
      $0 --single ${SEED} ${PREFIX} "${ARGS}" &
      sleep 0.5
    done
  ;;

  --sequentialSeed)
    # Test for arguments
    if [ $# -lt 4 ] || [ $# -gt 5 ]; 
    then 
      PrintHelp 
    fi;
  
    FIRST=$2
    LAST=$3
    PREFIX=$4
    ARGS=$5
    for ((SEED=${FIRST}; ${SEED} <= ${LAST}; SEED++))
    do
      $0 --single ${SEED} ${PREFIX} "${ARGS}"
    done
  ;;

  --parallelPrefix)
    # Test for arguments
    if [ $# -lt 3 ] || [ $# -gt 4 ]; 
    then 
      PrintHelp 
    fi;
  
    SEED=$2
    PREFIX_LIST=$3
    ARGS=$4
    for PREFIX in ${PREFIX_LIST};
    do
      $0 --single ${SEED} ${PREFIX} "${ARGS}" &
      sleep 0.5
    done
  ;;

  --sequentialPrefix)
    # Test for arguments
    if [ $# -lt 3 ] || [ $# -gt 4 ]; 
    then 
      PrintHelp 
    fi;
  
    SEED=$2
    PREFIX_LIST=$3
    ARGS=$4
    for PREFIX in ${PREFIX_LIST};
    do
      $0 --single ${SEED} ${PREFIX} "${ARGS}"
    done
  ;;

  --parallelSeedPrefix)
    # Test for arguments
    if [ $# -lt 4 ] || [ $# -gt 5 ]; 
    then 
      PrintHelp 
    fi;
  
    FIRST=$2
    LAST=$3
    PREFIX_LIST=$4
    ARGS=$5
    for ((SEED=${FIRST}; ${SEED} <= ${LAST}; SEED++))
    do
      $0 --sequentialPrefix ${SEED} "${PREFIX_LIST}" "${ARGS}" &
      sleep 0.5
    done
  ;;

  --abort)
    # Test for arguments
    if [ $# -ne 1 ];
    then 
      PrintHelp 
    fi;
    
    echo "You are about to abort all running simulations in this machine."
    echo -n "Are you sure you want to continue? (y/N) "
    read ANSWER
    if [ "${ANSWER}" == "y" ];
    then
      killall $0 ${PROGNAME}
    else
      echo "Invalid answer."
    fi
  ;;

  *)
    echo "Invalid action"
    PrintHelp
  ;;
esac

exit 0;

