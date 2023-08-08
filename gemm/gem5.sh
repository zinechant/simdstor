#!/bin/bash
P=0
. ../gem5.sh

if [ -z $D ]; then
    D=0
fi

#DEBUGFLAGS="--debug-flags=ExecAll"
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 2' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./test -o "${D}" 2> ${D}${DSUFFIX}.err
