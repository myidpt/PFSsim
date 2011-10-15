#!/bin/bash

file=asym-dist

rm -f $file

for i in {0..127}
do
  let "group = $(expr $i / 64)"
  if [ $group = 1 ]
  then
    if [ $i -lt 96 ]
    then
      echo "$i [0 64k]" >> $file
    elif [ $i -lt 112 ]
    then
      echo "$i [1 64k]" >> $file
    elif [ $i -lt 120 ]
    then
      echo "$i [2 64k]" >> $file
    else
      echo "$i [3 64k]" >> $file
    fi
  else
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
  fi
done
