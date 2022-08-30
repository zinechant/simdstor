#!/bin/bash
. ../gem5.sh

D=12
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 4' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./st1 -o "$D $streambuffer_addr"