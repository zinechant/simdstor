CSRCS +=  $(shell ls ./*.c 2> /dev/null)
CHDRS +=  $(shell ls ./*.h 2> /dev/null)
CXXSRCS = $(shell ls ./*.cc 2> /dev/null)
CXXHDRS = $(shell ls ./*.hh 2> /dev/null)

INCLUDE += -I ../util/googletest/build/include
LDFLAGS += -L ../util/googletest/build/lib
LDLIBS += -lgtest -lgtest_main

ifeq ($(ARM),1)
	TARGET = varsimd
else ifeq ($(ARM),2)
	TARGET = varsimd
else
	TARGET = varsimd
endif

include ../common.mk

varsimd: $(OBJS) $(CHDRS) $(CXXHDRS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LOADLIBES) $(LDLIBS)

varsimd.objdump: varsimd
	llvm-objdump --arch-name=aarch64 --mattr=+v8.2a,+sve -S $< > $@
