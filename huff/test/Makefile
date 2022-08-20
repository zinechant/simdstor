CXXSRCS = $(shell ls ../src/*.cc 2> /dev/null) $(shell ls ./*.cc 2> /dev/null)
CXXHDRS = $(shell ls ../include/*.hh 2> /dev/null) $(shell ls ./*.hh 2> /dev/null)

INCLUDE += -I ../include

TARGET = test
LDLIBS = -lm

include ../../common.mk

$(TARGET): $(OBJS) $(CHDRS) $(CXXHDRS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LOADLIBES) $(LDLIBS)
