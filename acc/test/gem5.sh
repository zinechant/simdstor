#!/bin/bash
. ../../gem5.sh

D=032MiB
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./main -o "$DATA/$D $streambuffer_addr"
