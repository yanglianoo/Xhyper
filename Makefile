# Makefile for X-Hyper project
# Generated from CMakeLists.txt for ARMv8-A hypervisor build
# All intermediate and output files are placed in the build/ directory

# Cross-compiler prefix
CROSS_COMPILE = aarch64-linux-gnu-

# Compilers and tools
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# Compiler and assembler flags
ASMFLAGS = -march=armv8-a+nosimd+nofp -ffreestanding -Wextra -Wfatal-errors -Werror -O0 -g3 -D__ASSEMBLY__
CFLAGS = -march=armv8-a+nosimd+nofp -ffreestanding -Wall -Wextra -Wfatal-errors -Werror -Wno-psabi -O0 -g3 -D__LITTLE_ENDIAN -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-override-init

# Include directories
INCLUDE_DIRS = -I./hypervisor/include

# Source files
X_HYPER_SRCS = \
	hypervisor/src/head.S \
	hypervisor/src/vector.S \
	hypervisor/src/pl011.c \
	hypervisor/src/utils.c \
	hypervisor/src/spinlock.c \
	hypervisor/src/printf.c \
	hypervisor/src/xmalloc.c \
	hypervisor/src/kalloc.c \
	hypervisor/src/guest.c \
	hypervisor/src/el1_sync.c \
	hypervisor/src/vcpu.c \
	hypervisor/src/vm.c \
	hypervisor/src/vmm.c \
	hypervisor/src/main.c \
	hypervisor/src/vpsci.c \
	test/stage2_translation_test.c

# Object files (placed in build/)
X_HYPER_OBJS = $(patsubst %.c,build/%.o,$(X_HYPER_SRCS))
X_HYPER_OBJS := $(patsubst %.S,build/%.o,$(X_HYPER_OBJS))

# Linker script and guest VM image
LSCRIPT_SRC = hypervisor/src/lds/linker.ld.S
LSCRIPT = build/linker.ld
GUEST_VM_IMAGE = ./guest/Guest_VM.o

# Output files (placed in build/)
LIB = build/libx_hyper_libs.a
ELF = build/X-Hyper.elf
BINARY = build/X-Hyper
MAP = build/X-Hyper.map

# Default target
all: $(BINARY)
	@echo "-- X-Hyper build complete --"

# Create build directory
$(X_HYPER_OBJS) $(LIB) $(LSCRIPT) $(ELF) $(BINARY): | build_dirs
build_dirs:
	@mkdir -p build/hypervisor/src build/test

# Static library
$(LIB): $(X_HYPER_OBJS)
	@echo "-- Compiling X-Hyper_libs --"
	@ar rcs $@ $^

# Object file rules
build/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

build/%.o: %.S
	$(AS) $(ASMFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Preprocess linker script
$(LSCRIPT): $(LSCRIPT_SRC)
	@echo "-- Preprocessing linker.ld.S to build/linker.ld --"
	$(CC) -E -P $(LSCRIPT_SRC) -o $@ $(INCLUDE_DIRS)

# Linker command
ifeq ($(wildcard $(GUEST_VM_IMAGE)),)
$(ELF): $(LIB) $(LSCRIPT)
	@echo "-- Linking X-Hyper_libs without Guest_VM.o --"
	$(LD) -pie -Map $(MAP) -T$(LSCRIPT) -Lbuild -lx_hyper_libs -o $@
else
$(ELF): $(LIB) $(LSCRIPT) $(GUEST_VM_IMAGE)
	@echo "-- Linking X-Hyper_libs with Guest_VM.o --"
	$(LD) -pie -Map $(MAP) -T$(LSCRIPT) -Lbuild -lx_hyper_libs $(GUEST_VM_IMAGE) -o $@
endif

# Binary image generation
$(BINARY): $(ELF)
	@echo "-- Generating X-Hyper binary --"
	$(OBJCOPY) -O binary -R .note -R .note.gnu.build-id -R .comment -S $< $@

# Clean target
clean:
	rm -rf build

# Phony targets
.PHONY: all clean build_dirs