# ODIR=m5_${ARCH}_${D}${DSUFFIX}
P=0
S=0
B=1
. ../gem5.sh

ODIR=m5out
~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d $ODIR ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c './varsimd'
# ~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d $ODIR ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c './varsimd' -o '--gtest_filter=*.varstream '
# ~/gem5/build/$ARCH/gem5.${OPT} ${DEBUGFLAGS} --debug-file=log -d $ODIR ~/gem5/configs/example/se.py --param 'system.cpu[:].isa[:].sve_vl_se = 8' $CLOCK $CPUMEM $CACHES $scratchpad $streambuffer $prefetch  -c './varsimd' -o '--gtest_repeat=50 '
