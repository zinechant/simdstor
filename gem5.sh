#!/bin/bash
export DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ -z "$OPT" ]; then
    OPT="opt"
fi
if [ -z "$ARCH" ]; then
    ARCH=RISCV
fi

DSUFFIX=""
if [ "0" == "$S" ] ; then
    scratchpad=""
    scratchpad_addr="0"
    DSUFFIX="${DSUFFIX}_noscratchpad"
else
    scratchpad="--scratchpad"
    scratchpad_addr="1099511627776"
    DSUFFIX="${DSUFFIX}_scratchpad"
fi

if [ "0" == "$B" ] ; then
    streambuffer=""
    streambuffer_addr="0"
    DSUFFIX="${DSUFFIX}_nostreambuffer"
else
    streambuffer="--streambuffer"
    streambuffer_addr="1100585369600"
    DSUFFIX="${DSUFFIX}_streambuffer"
fi

if [ "0" == "$P" ] ; then
    prefetch=""
    DSUFFIX="${DSUFFIX}_noprefetch"
else
    # Prefetcher=AMPMPrefetcher
    Prefetcher=DCPTPrefetcher
    # Prefetcher=SignaturePathPrefetcher
    prefetch="--l1d-hwp-type=$Prefetcher --l2-hwp-type=$Prefetcher"
    DSUFFIX="${DSUFFIX}_${Prefetcher}"
fi

DATA="$( cd "$DIR/data" && pwd )"
CLOCK="--cpu-clock=2GHz --sys-clock=2GHz"
CPUMEM="--mem-size=8192MB --cpu-type=TimingSimpleCPU"
CACHES="--caches --l2cache --l1d_size=32kB --l1i_size=32kB --l2_size=256kB --l1d_assoc=8 --l1i_assoc=8 --l2_assoc=16"
DEBUGFLAGS=""
#DEBUGFLAGS = --debug-flags=NoncoherentXBar,CoherentXBar,CacheTick,SimpleCPUTick,Exec
