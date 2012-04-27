#!/bin/bash

CLT_NUM=16
TRC_PER_CLT=2
SVR_NUM=8
FILE_NUM=$(echo "$CLT_NUM*$TRC_PER_CLT" | bc)
FILE_SIZE=536870912
REQ_SIZE=1048576
READ_FLAG=1
STRIPE_SIZE=64K

for WORD in "$@";
do
  case $WORD in
    --python-prefix=*)
      PYTHON_PREFIX=${WORD:16}
      if [ -n "$PYTHON_PREFIX" ];
      then
        PYTHON_PREFIX=${PYTHON_PREFIX}/
      fi
      shift ;;
    --help)
      echo "Options:"
      echo "--python-prefix= # Provide the path to the python binary directory."
      echo "--help           # Print this information."
      exit ;;
    *)
      echo "Unrecognized argument: " $WORD
      exit ;;
  esac
done


cd input/synthetic
${PYTHON_PREFIX}python ./gen.py $CLT_NUM $TRC_PER_CLT $FILE_SIZE $REQ_SIZE $READ_FLAG
cd ../..

cd config/pfslayout/synthetic/even-dist
${PYTHON_PREFIX}python ./make-ed.py $SVR_NUM $FILE_NUM $STRIPE_SIZE
cd ../../../..

cd config/lfslayout/in/synthetic
${PYTHON_PREFIX}python ./gen.py $SVR_NUM $FILE_NUM
