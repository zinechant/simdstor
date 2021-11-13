make clean && make release && riscv64-unknown-elf-objdump -d -l ./main > ./main.s

VLEN=0128 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./main > ./$VLEN.out 2> ./$VLEN.hist &
VLEN=0256 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./main > ./$VLEN.out 2> ./$VLEN.hist &
VLEN=0512 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./main > ./$VLEN.out 2> ./$VLEN.hist &
VLEN=1024 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./main > ./$VLEN.out 2> ./$VLEN.hist &
VLEN=2048 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./main > ./$VLEN.out 2> ./$VLEN.hist &
VLEN=4096 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./main > ./$VLEN.out 2> ./$VLEN.hist &

wait

python3 $RISCV/simd_compress/python3/get_insts.py --hist ./0128.hist --objdump ./main.s --profile hdec && echo
python3 $RISCV/simd_compress/python3/get_insts.py --hist ./0256.hist --objdump ./main.s --profile hdec && echo
python3 $RISCV/simd_compress/python3/get_insts.py --hist ./0512.hist --objdump ./main.s --profile hdec && echo
python3 $RISCV/simd_compress/python3/get_insts.py --hist ./1024.hist --objdump ./main.s --profile hdec && echo
python3 $RISCV/simd_compress/python3/get_insts.py --hist ./2048.hist --objdump ./main.s --profile hdec && echo
python3 $RISCV/simd_compress/python3/get_insts.py --hist ./4096.hist --objdump ./main.s --profile hdec && echo

make clean && RVV=1 make release && riscv64-unknown-elf-objdump -d -l ./main > ./main.s && \
spike -g --isa=RV64IM $RISCV/riscv64-unknown-elf/bin/pk -s ./main > ./scalar.out 2> ./scalar.hist && \
python3 $RISCV/simd_compress/python3/get_insts.py --hist ./scalar.hist --objdump ./main.s --profile hdec
