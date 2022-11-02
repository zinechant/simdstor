all: opt sim
debug: dbg verdi

opt:
	vcs -full64 -sverilog -timescale=1ns/1ps \
		+incdir+../../src ../../src/streambuffer.sv \
		+incdir+../../test ../../test/tb_streambuffer.sv
dbg:
	vcs -full64 -sverilog -timescale=1ns/1ps -debug_acc+all -kdb\
		+incdir+../../src ../../src/streambuffer.sv \
		+incdir+../../test ../../test/tb_streambuffer.sv


verdi:
	./simv  -gui=verdi -verdi_opts "-sx"

sim:
	./simv

clean:
	rm -rf csrc DVE* simv* *.log *.key *.h *.vpd
