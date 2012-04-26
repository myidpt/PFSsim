#!/bin/bash

CLT_NUM=16
TRC_PER_CLT=2
SVR_NUM=8
FILE_NUM=$(echo "$CLT_NUM*$TRC_PER_CLT" | bc)
FILE_SIZE=536870912
REQ_SIZE=1048576
READ_FLAG=1
STRIPE_SIZE=64K

cd input/synthetic
./gen.py $CLT_NUM $TRC_PER_CLT $FILE_SIZE $REQ_SIZE $READ_FLAG
cd ../..

cd config/pfslayout/synthetic/even-dist
./make-ed.py $SVR_NUM $FILE_NUM $STRIPE_SIZE

