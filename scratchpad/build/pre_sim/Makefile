all: opt sim
debug: dbg verdi

opt:
	vcs -full64 -sverilog -timescale=1ns/1ps \
		+incdir+../../src ../../src/*.sv \
		+incdir+../../test ../../test/ ../../test/*.sv
dbg:
	vcs -full64 -sverilog -timescale=1ns/1ps -debug_acc+all -kdb\
		+incdir+../../src ../../src/*.sv \
		+incdir+../../test ../../test/ ../../test/*.sv \
		+incdir+/usr/synopsys/saed14/SAED14nm_EDK_SRAM_v_05072020/lib/sram/verilog/ \
		/usr/synopsys/saed14/SAED14nm_EDK_SRAM_v_05072020/lib/sram/verilog/saed14sram.v

verdi:
	./simv  -gui=verdi -verdi_opts "-sx"

sim:
	./simv

clean:
	rm -rf csrc DVE* simv* *.log *.key *.h *.vpd
