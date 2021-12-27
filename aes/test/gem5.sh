#!/bin/bash
. ../../gem5.sh

D=001MiB
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./main -o "4 $DATA/$D $DATA/gf_mul.data $DATA/gf_sbox.data $DATA/gf_ibox.data f $scratchpad_addr $streambuffer_addr"
