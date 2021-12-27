#!/bin/bash
export DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DATA="$( cd "$DIR/../../data" && pwd )"

D=001MiB
set -x
spike -g --isa=RV64GCV --ic=64:8:64 --dc=64:8:64 --l2=256:16:64 $RISCV/riscv64-unknown-elf/bin/pk -s $DIR/main $DATA/$D $DATA/gf_mul.data $DATA/gf_inv.data f >$D.log 2>$D.hist
set +x
