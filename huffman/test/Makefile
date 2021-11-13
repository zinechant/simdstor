CSRCS += $(shell ls ../src/*.c 2> /dev/null) $(shell ls ./*.c 2> /dev/null)
CHDRS += $(shell ls ../include/*.h 2> /dev/null) $(shell ls ./*.h 2> /dev/null)
CXXSRCS = $(shell ls ../src/*.cc 2> /dev/null) $(shell ls ./*.cc 2> /dev/null)
CXXHDRS = $(shell ls ../include/*.hh 2> /dev/null) $(shell ls ./*.hh 2> /dev/null)

INCLUDE += -I ../include

TARGET = main
LDLIBS = -lm

include ../../common.mk

$(TARGET): $(OBJS) $(CHDRS) $(CXXHDRS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LOADLIBES) $(LDLIBS)
