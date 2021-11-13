make clean && make release && riscv64-unknown-elf-objdump -d -l ./test > ./test.s && \
spike -g --isa=RV64IMC $RISCV/riscv64-unknown-elf/bin/pk -s ./test ~/Downloads/silesia/silesia.tar 2> silesia.hist && \
python3 ../../../python3/get_insts.py --objdump ./test.s --hist ./silesia.hist --profile sepc
