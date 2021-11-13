make clean && make release && riscv64-unknown-elf-objdump -d -l ./test > ./test.s

VLEN=0128 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./test ~/Downloads/silesia/silesia.tar > silesia.$VLEN.out 2> silesia.$VLEN.hist &
VLEN=0256 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./test ~/Downloads/silesia/silesia.tar > silesia.$VLEN.out 2> silesia.$VLEN.hist &
VLEN=0512 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./test ~/Downloads/silesia/silesia.tar > silesia.$VLEN.out 2> silesia.$VLEN.hist &
VLEN=1024 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./test ~/Downloads/silesia/silesia.tar > silesia.$VLEN.out 2> silesia.$VLEN.hist &
VLEN=2048 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./test ~/Downloads/silesia/silesia.tar > silesia.$VLEN.out 2> silesia.$VLEN.hist &
VLEN=4096 && spike --varch=vlen:$VLEN,elen:64,slen:$VLEN -g --isa=RV64IMV $RISCV/riscv64-unknown-elf/bin/pk -s ./test ~/Downloads/silesia/silesia.tar > silesia.$VLEN.out 2> silesia.$VLEN.hist &

wait

python3 ../../../python3/get_insts.py --hist ./silesia.0128.hist --objdump ./test.s --profile rvvc && echo
python3 ../../../python3/get_insts.py --hist ./silesia.0256.hist --objdump ./test.s --profile rvvc && echo
python3 ../../../python3/get_insts.py --hist ./silesia.0512.hist --objdump ./test.s --profile rvvc && echo
python3 ../../../python3/get_insts.py --hist ./silesia.1024.hist --objdump ./test.s --profile rvvc && echo
python3 ../../../python3/get_insts.py --hist ./silesia.2048.hist --objdump ./test.s --profile rvvc && echo
python3 ../../../python3/get_insts.py --hist ./silesia.4096.hist --objdump ./test.s --profile rvvc && echo
