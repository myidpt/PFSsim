#!/bin/bash

TARGET=PFSsim

for WORD in "$@";
do
  case $WORD in
    --omnetpp-prefix=*)
      OMNETPP_PREFIX=${WORD:17}
      if [ -n "$OMNETPP_PREFIX" ];
      then
        OMNETPP_PREFIX=${OMNETPP_PREFIX}/
      fi
      shift ;;
    --name=*)
      TARGET=${WORD:7}
      shift ;;
    --help)
      echo "Options:"
      echo "--omnetpp-prefix= # Provide the prefix of OMNeT++ if it is not included in system path."
      echo "--name=           # Provide the name of the executable file. If not set, PFSsim is used."
      echo "--help            # Print this information."
      exit ;;
    *) echo "Unrecognized argument:" $WORD
      exit ;;
  esac
done

${OMNETPP_PREFIX}opp_makemake -f --deep -o $TARGET

make
