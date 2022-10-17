ifeq ($(ARM),1)
	CC = clang
	CXX = clang++
	ARCH = --gcc-toolchain=$(HOME)/arm --sysroot=$(HOME)/arm/aarch64-none-linux-gnu/libc --target=aarch64-linux-gnueabi -march=armv8.2-a+nofp
else ifeq ($(ARM),2)
	CC = clang
	CXX = clang++
	ARCH = --gcc-toolchain=$(HOME)/arm --sysroot=$(HOME)/arm/aarch64-none-linux-gnu/libc --target=aarch64-linux-gnueabi -march=armv8.2-a+sve
else ifeq ($(ARM),0)
	CC = clang
	CXX = clang++
	ARCH ?= -march=core2
else ifeq ($(RVV),0)
	CC = gcc
	CXX= g++
	ARCH ?= -march=core2
else ifeq ($(RVV),1) # scalar implementation compiled with riscv
	CC = clang
	CXX= clang++
	ARCH = --gcc-toolchain=$(RISCV) --target=riscv64 -march=rv64imv1p0 -mabi=lp64
else ifeq ($(RVV),2)
	CC = $(RISCV)/bin/riscv64-unknown-elf-gcc
	CXX= $(RISCV)/bin/riscv64-unknown-elf-g++
	CPPFLAGS += -D RVV
	ARCH = -march=rv64imv -mabi=lp64
else
	CC = clang
	CXX= clang++
	CPPFLAGS += -D RVV
	ARCH = --gcc-toolchain=$(RISCV) --target=riscv64 -march=rv64imv1p0 -mabi=lp64
endif

ifneq ($(ARM),)
ifneq ($(ARM),0)
M5LIBD=${HOME}/gem5/util/m5/build/arm64/out
ifneq ($(wildcard ${M5LIBD}/.*),)
	INCLUDE += -I ${HOME}/gem5/include
	LDFLAGS += -fuse-ld=lld -L ${M5LIBD}
	LDLIBS += -lm5
endif
endif
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
LDFLAGS += $(ARCH) -static

ifeq ($(CXX),clang++)
	CPPFLAGS += -fno-vectorize -fno-slp-vectorize
endif

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
