#!/usr/bin/env bash
export DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
D=128MiB && spike -g --isa=RV64GCV --ic=64:8:64 --dc=64:8:64 --l2=256:16:64 $RISCV/riscv64-unknown-elf/bin/pk -s $DIR/main $DIR/$D f >$D.log 2>$D.hist