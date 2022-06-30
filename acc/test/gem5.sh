#!/bin/bash
. ../../gem5.sh
BB=32768
SB=2
D=032MiB
streamssd="--streamssd --streamssd_blockbytes $BB --streamssd_streamblocks $SB"
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} -d m5_${ARCH}_${D}_${BB}_${SB}_${DSUFFIX} ~/gem5/configs/example/se.py $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch $streamssd -c ./main -o "$DATA/$D $streambuffer_addr"
