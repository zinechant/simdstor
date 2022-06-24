#!/bin/bash
. ../../gem5.sh

D=036MiB
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d m5_${ARCH}_${D}${DSUFFIX} ~/gem5/configs/example/se.py $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c ./main -o "$DATA/$D f $streambuffer_addr"
# --param 'system.cpu[:].isa[:].sve_vl_se = 4'
# --interp-dir $HOME/arm/aarch64-none-linux-gnu/libc  --redirects /lib=$HOME/arm/aarch64-none-linux-gnu/libc/usr/lib --redirects /lib64=$HOME/arm/aarch64-none-linux-gnu/libc/usr/lib64
