#!/bin/bash
P=0
. ../../gem5.sh

if [ -z $D ]; then
    D=0
fi

# OPT=opt
# DEBUGFLAGS="--debug-flags=MinorTrace"
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./test -o "${D}"
