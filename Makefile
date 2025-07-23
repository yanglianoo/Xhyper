# Makefile for X-Hyper project

# Cross-compiler prefix
CROSS_COMPILE = aarch64-linux-gnu-

# Compilers and linker
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# Compiler and assembler flags
ASM_FLAGS = -march=armv8-a+nosimd+nofp -ffreestanding -Wextra -Wfatal-errors -Werror -O0 -g3 -D__ASSEMBLY__
CFLAGS = -march=armv8-a+nosimd+nofp -ffreestanding -Wall -Wextra -Wfatal-errors -Werror -Wno-psabi -O0 -g3 -D__LITTLE_ENDIAN -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-override-init

# Directories
INCLUDE_DIR = ./hypervisor/include
SRC_DIR = ./hypervisor/src
LDS_DIR = ./hypervisor/src/lds
BUILD_DIR = build

# Source files
X_HYPER_SRCS = \
	$(SRC_DIR)/head.S \
	$(SRC_DIR)/pl011.c \
	$(SRC_DIR)/utils.c \
	$(SRC_DIR)/printf.c \
	$(SRC_DIR)/spinlock.c \
	$(SRC_DIR)/xmalloc.c \
	$(SRC_DIR)/kalloc.c \
	$(SRC_DIR)/main.c

# Object files
X_HYPER_OBJS = $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(X_HYPER_SRCS:.S=.o))
X_HYPER_OBJS := $(X_HYPER_OBJS:.c=.o)

# Linker script
LSCRIPT = $(LDS_DIR)/linker.ld.S
LSCRIPT_OBJ = $(BUILD_DIR)/linker.ld.S.o
LSCRIPT_PREPROCESSED = $(BUILD_DIR)/linker.ld

# Output files
OUTPUT = $(BUILD_DIR)/X-Hyper.elf
MAP = $(BUILD_DIR)/X-Hyper.map
BINARY = $(BUILD_DIR)/X-Hyper

# Create build directory
$(shell mkdir -p $(BUILD_DIR))

# Default target
all: $(BINARY)

# Rule to build the final binary
$(BINARY): $(OUTPUT)
	@echo "-- Generating X-Hyper binary --"
	$(OBJCOPY) -O binary -R .note -R .note.gnu.build-id -R .comment -S $< $@

# Rule to link the executable
$(OUTPUT): $(X_HYPER_OBJS) $(LSCRIPT_PREPROCESSED)
	@echo "-- Linking X-Hyper_libs --"
	$(LD) -pie -Map $(MAP) -T$(LSCRIPT_PREPROCESSED) -L$(BUILD_DIR) $(X_HYPER_OBJS) -o $@

# Rule to preprocess linker script
$(LSCRIPT_PREPROCESSED): $(LSCRIPT)
	@echo "-- Compiling linker.ld.S --"
	$(AS) -P -E -D__ASSEMBLY__ -I$(INCLUDE_DIR) $< -o $@

# Rule for assembling .S files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@echo "-- Compiling $< --"
	$(AS) $(ASM_FLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Rule for compiling .c files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "-- Compiling $< --"
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Clean target
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all clean