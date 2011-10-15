#!/bin/bash

#Makes an even file distribution scheme for files [0..127] on data servers [0.3].

file=ed-128

rm -f $file

for i in {0..127}
do
  let "rem = $(expr $i % 4)"
  if [ $rem = 0 ]
  then
    echo "$i [0 64k] [1 64k] [2 64k] [3 64k]" >> $file
  elif [ $rem = 1 ]
  then
    echo "$i [1 64k] [2 64k] [3 64k] [0 64k]" >> $file
  elif [ $rem = 2 ]
  then
    echo "$i [2 64k] [3 64k] [0 64k] [1 64k]" >> $file
  elif [ $rem = 3 ]
  then
    echo "$i [3 64k] [0 64k] [1 64k] [2 64k]" >> $file
  fi
done
