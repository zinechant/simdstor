CXXSRCS = $(shell ls ./*.cc 2> /dev/null)
CXXHDRS = $(shell ls ./*.hh 2> /dev/null)

TARGET = test

include ../common.mk

$(TARGET): $(OBJS) $(CHDRS) $(CXXHDRS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LOADLIBES) $(LDLIBS)
