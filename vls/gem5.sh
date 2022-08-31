#!/bin/bash
P=0
. ../gem5.sh

D=10
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./vls -o "$D $streambuffer_addr"

D=12
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./vls -o "$D $streambuffer_addr"

D=14
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./vls -o "$D $streambuffer_addr"

D=16
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./vls -o "$D $streambuffer_addr"

D=18
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./vls -o "$D $streambuffer_addr"
