#!/bin/bash
DIR=pfs_result
PARV=valid/ibm18es.parv
mkdir $DIR
src/physim 8800 $PARV $DIR/outv0 $DIR/out0 & 
src/physim 8801 $PARV $DIR/outv1 $DIR/out1 &
src/physim 8802 $PARV $DIR/outv2 $DIR/out2 &
src/physim 8803 $PARV $DIR/outv3 $DIR/out3 &
#src/physim 8804 $PARV $DIR/outv4 $DIR/out4 & 
#src/physim 8805 $PARV $DIR/outv5 $DIR/out5 &
#src/physim 8806 $PARV $DIR/outv6 $DIR/out6 &
#src/physim 8807 $PARV $DIR/outv7 $DIR/out7 &
#src/physim 8808 $PARV $DIR/outv8 $DIR/out8 & 
#src/physim 8809 $PARV $DIR/outv9 $DIR/out9 &
#src/physim 8810 $PARV $DIR/outv10 $DIR/out10 &
#src/physim 8811 $PARV $DIR/outv11 $DIR/out11 &
#src/physim 8812 $PARV $DIR/outv12 $DIR/out12 & 
#src/physim 8813 $PARV $DIR/outv13 $DIR/out13 &
#src/physim 8814 $PARV $DIR/outv14 $DIR/out14 &
#src/physim 8815 $PARV $DIR/outv15 $DIR/out15 &

