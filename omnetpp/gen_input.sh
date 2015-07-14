#!/bin/bash

CLT_NUM=16
TRC_PER_CLT=2
SVR_NUM=8
FILE_SIZE=536870912
REQ_SIZE=1048576
READ_FLAG=1
STRIPE_SIZE=64K
FILE_PER_PROCESS=1

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
      echo "--client= # Provide the number of client"
      echo "--trace-num= # Provide the number of trace per client"
      echo "--server= # Provide the number of server"
      echo "--file-size= # Provide the size of file (in byte)"
      echo "--req-size= # Provide the size of each quantity of data asks by client in one time (in byte)"
      echo "--read= # 0 for write file/1 read file"
      echo "--stripe-size= # Provide the stripe-size : Don't forget the unit at the end : K for KB, M for MB"
      echo "--uniq-shared-file= #Provide one file shared between clients"
      echo "--help           # Print this information."
      exit ;;
    --client=*)
		CLT_NUM=${WORD:9}
		shift ;;
    --trace-num=*)
		TRC_PER_CLT=${WORD:12}
		shift ;;
	--server=*)
		SVR_NUM=${WORD:9}
		shift ;;
	--file-size=*)
		FILE_SIZE=${WORD:12}
		shift ;;
	--req-size=*)
		REQ_SIZE=${WORD:11}
		shift ;;
	--read=*)
		READ_FLAG=${WORD:7}
		shift ;;
	--stripe-size=*)
		STRIPE_SIZE=${WORD:14}
		shift ;;
	--uniq-shared-file)
		FILE_PER_PROCESS=0
		shift ;;
    *)
      echo "Unrecognized argument: " $WORD
      exit ;;
  esac
done

if [ $FILE_PER_PROCESS -eq 1 ];
then
	FILE_NUM=$(echo "$CLT_NUM*$TRC_PER_CLT" | bc)
else
	FILE_NUM=1
fi

cd input/synthetic
${PYTHON_PREFIX}python ./gen.py $CLT_NUM $TRC_PER_CLT $FILE_SIZE $REQ_SIZE $READ_FLAG $FILE_PER_PROCESS
cd ../..

cd config/pfslayout/synthetic/even-dist
${PYTHON_PREFIX}python ./make-ed.py $SVR_NUM $FILE_NUM $STRIPE_SIZE
cd ../../../..

cd config/lfslayout/in/synthetic
${PYTHON_PREFIX}python ./gen.py $SVR_NUM $FILE_NUM $FILE_SIZE
