TOP = tb_trimux
srcs = ../../../src/trimux.sv ../../../test/${TOP}.sv
dirs = +incdir+../src

all: sim

.PHONY: sim
sim: ${srcs}
	vcs -full64 -sverilog +vpi -timescale=1ns/1ps \
		${dirs} ${srcs} \
		-top ${TOP}
	./simv | tee simv.log

.PHONY: debug
debug: ${srcs}
	vcs -full64 -sverilog +vpi -debug_acc+all -timescale=1ns/1ps \
		${dirs} ${srcs} \
		-top ${TOP}
	./simv -gui -dve_opt "-script signal_${TOP}.tcl"

.PHONY: clean
clean:
	rm -rf csrc DVE* simv* *.log *.key *.h *.vpd
