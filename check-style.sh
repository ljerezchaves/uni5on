#!/bin/bash

function PrintHelp () {
  echo "Usage: $0 <path_to_check> [--in-place]"
  echo
  exit 1
}

# Check for valid arguments
if [ $# -lt 1 ] || [ $# -gt 2 ];
then
  echo "Invalid number of arguments"
  PrintHelp
fi;
if [ $# -eq 2 ] && [ "$2" != "--in-place" ];
then
  echo "Invalid optional argument"
  PrintHelp
fi;

BASEDIR=$1
ACTION=$2
FILELIST=$(find ${BASEDIR} -name '*.cc' -o -name '*.h')
for I in ${FILELIST}
do
    echo $I
    case "${ACTION}" in
      --in-place)
        ./utils/check-style.py -l3 --in-place --check-file=$I
      ;;

      *)
        ./utils/check-style.py -l3 --diff --check-file=$I
      ;;
    esac
done
