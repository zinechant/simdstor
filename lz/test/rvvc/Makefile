CSRCS = ../../src/rvvc.c ../../src/sepd.c ../test.c
INCLUDE = -I ../../include
TARGET = test

include ../../../common.mk

$(TARGET): $(OBJS) $(CHDRS) $(CXXHDRS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LOADLIBES) $(LDLIBS)
