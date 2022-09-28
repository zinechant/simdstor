#!/bin/bash
P=0
. ../../../gem5.sh

if [ -z $D ]; then
    D=0
fi

if [ -z $Q ]; then
    Q=q02_part
fi

# DEBUGFLAGS="--debug-flags=ExecAll,CacheAll,Minor"
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${D}_${Q}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ../test -o "${Q} ${D}" > ${D}_${Q}${DSUFFIX}.out 2>&1
