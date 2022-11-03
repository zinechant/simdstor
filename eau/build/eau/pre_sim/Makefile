TOP = tb_eau
srcs = ../../../src/*.sv ../../../test/${TOP}.sv
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
	vcs -full64 -sverilog +vpi -kdb -debug_acc+all -timescale=1ns/1ps \
		${dirs} ${srcs} \
		-top ${TOP}
	./simv -gui=verdi -verdi_opts "-sx"

.PHONY: clean
clean:
	rm -rf csrc DVE* simv* *.log *.key *.h *.vpd
