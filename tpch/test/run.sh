#!/bin/bash
if [ -z ${M+x} ]; then
    M=release
fi

if [ -z ${Q} ]; then
    Q=q02_part
fi

set -x
ARM=2 make clean && ARM=2 make $M
set +x

rm -f *.bin

Q=$Q D=0 bash gem5.sh
Q=$Q D=1 bash gem5.sh
Q=$Q D=2 bash gem5.sh
Q=$Q D=3 bash gem5.sh

echo

set -x
diff p_mfgr_0.bin p_mfgr_1.bin
diff p_mfgr_0.bin p_mfgr_2.bin
diff p_mfgr_0.bin p_mfgr_3.bin

diff p_partkey_0.bin p_partkey_1.bin
diff p_partkey_0.bin p_partkey_2.bin
diff p_partkey_0.bin p_partkey_3.bin
set +x

rm -rf $Q
mkdir $Q
mv *.bin $Q
