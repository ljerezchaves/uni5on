#!/bin/bash

ACTION=$1
FILELIST=$(find scratch/svelte/ -name '*.cc' -o -name '*.h')
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
