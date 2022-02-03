ifeq ($(RVV),0)
	CC = gcc
	CXX= g++
	ARCH ?= -march=core2
else ifeq ($(RVV),1) # scalar implementation compiled with riscv
	CC = $(RISCV)/bin/clang
	CXX= $(RISCV)/bin/clang++
	ARCH = --gcc-toolchain=$(RISCV) --target=riscv64 -menable-experimental-extensions -march=rv64imv0p10 -mabi=lp64
else ifeq ($(RVV),2)
	CC = riscv64-unknown-elf-gcc
	CXX= riscv64-unknown-elf-g++
	CPPFLAGS += -D RVV
	ARCH = -march=rv64imv -mabi=lp64
else
	CC = $(RISCV)/bin/clang
	CXX= $(RISCV)/bin/clang++
	CPPFLAGS += -D RVV
	ARCH = --gcc-toolchain=$(RISCV) --target=riscv64 -menable-experimental-extensions -march=rv64imv0p10 -mabi=lp64
endif

DIR:=$(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

CSRCS += $(shell ls $(DIR)/src/*.c 2> /dev/null)
CHDRS += $(shell ls $(DIR)/include/*.h 2> /dev/null)
CXXSRCS += $(shell ls $(DIR)/src/*.cc 2> /dev/null)
CXXHDRS += $(shell ls $(DIR)/include/*.hh 2> /dev/null)

INCLUDE += -I $(DIR)/include

OBJS = $(CSRCS:.c=.o) $(CXXSRCS:.cc=.o)

CPPFLAGS += -static -g $(OPT) -Wall -Wextra -fPIC $(INCLUDE) $(ARCH)
CXXFLAGS += -std=c++17
CFLAGS += -std=c11
LDFLAGS += $(ARCH)

all : OPT ?= -O2
all : $(TARGET)

debug : OPT ?= -O0
debug : CPPFLAGS+= -D DEBUG
debug : $(TARGET)

release : OPT ?= -O3
release : CPPFLAGS+= -D NDEBUG
release : $(TARGET)

.phony: clean
clean:
	rm -rf $(TARGET) $(OBJS)
