#####THIS SCRIPT FIXES IT ALL##################################################
#This script assumes the conditions of the README, make sure it is so         #
#This runs the ssdmodel patch, then the 64bit patch, then fixes the patch and #
#cleans up the niceties of the build process                                  #
###############################################################################

#!/bin/bash
cd ../
patch -p1 < ssdmodel/ssd-patch
patch -p1 < 64bit-ssd-patch-files/patch-files/64bit-with-dixtrac-ssd-patch
cd 64bit-ssd-patch-files/modified-source-files
cp dixtrac/Makefile ../../dixtrac/Makefile
cp libparam/util.c ../../libparam/util.c
cp memsmodel/Makefile ../../memsmodel/Makefile
cp src/* ../../src/.
cp ssdmodel/include/ssdmodel/ssd.h ../../ssdmodel/include/ssdmodel/ssd.h
cp ssdmodel/ssd* ../../ssdmodel/.
cp run_pfs.sh ../../.
