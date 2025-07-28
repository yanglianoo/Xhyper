# Makefile for GUEST_VM project
# Converted from CMakeLists.txt and build script, fixed for correct source file handling

# Cross-compiler prefix
CROSS_COMPILE = aarch64-linux-gnu-

# Compiler and linker tools
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
AR = ar

# Compiler flags
ASM_FLAGS = -march=armv8-a+nosimd+nofp -ffreestanding -Wextra -Wfatal-errors -Werror -O0 -g3 -D__ASSEMBLY__
C_FLAGS = -march=armv8-a+nosimd+nofp -ffreestanding -Wall -Wextra -Wfatal-errors -Werror -Wno-psabi -O0 -g3 -D__LITTLE_ENDIAN

# Include directories
INCLUDE_DIRS = -I.

# Source files
VM_SRCS = head.S main.c

# Object files (generate in build/, avoid duplicates)
VM_OBJS = $(addprefix $(BUILD_DIR)/, $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(VM_SRCS))))

# Static library
VM_LIB = libvm_libs.a

# Linker script
LSCRIPT = linker.ld

# Output files
EXEC = Guest_VM.elf
IMAGE = Guest_VM
OBJ = Guest_VM.o
MAP = VM.map

# Build directory
BUILD_DIR = build

# Default target
all: $(BUILD_DIR)/$(OBJ)
	@echo "Build completed successfully"

# Check for source files and linker script
$(foreach src,$(VM_SRCS),$(if $(wildcard $(src)),,$(error Source file $(src) not found)))
ifeq ($(wildcard $(LSCRIPT)),)
$(error Linker script $(LSCRIPT) not found)
endif

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile .S files (source in root, output in build/)
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@echo "AS $< -> $@"
	$(AS) $(ASM_FLAGS) $(INCLUDE_DIRS) -c $< -o $@ || { echo "Failed to compile $<"; exit 1; }

# Compile .c files (source in root, output in build/)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "CC $< -> $@"
	$(CC) $(C_FLAGS) $(INCLUDE_DIRS) -c $< -o $@ || { echo "Failed to compile $<"; exit 1; }

# Create static library
$(BUILD_DIR)/$(VM_LIB): $(VM_OBJS) | $(BUILD_DIR)
	@echo "Creating static library $@"
	$(AR) rcs $@ $(VM_OBJS) || { echo "Failed to create library $@"; exit 1; }

# Link to create Guest_VM.elf
$(BUILD_DIR)/$(EXEC): $(BUILD_DIR)/$(VM_LIB) $(LSCRIPT) | $(BUILD_DIR)
	@echo "Linking $@"
	$(LD) -pie -Map $(BUILD_DIR)/$(MAP) -T$(LSCRIPT) -L$(BUILD_DIR) -lvm_libs -o $@ || { echo "Link failed"; exit 1; }

# Convert ELF to binary
$(BUILD_DIR)/$(IMAGE): $(BUILD_DIR)/$(EXEC) | $(BUILD_DIR)
	@echo "Generating binary $@"
	$(OBJCOPY) -O binary -R .note -R .note.gnu.build-id -R .comment -S $< $@ || { echo "Objcopy failed"; exit 1; }

# Convert binary to relocatable object
$(BUILD_DIR)/$(OBJ): $(BUILD_DIR)/$(IMAGE) | $(BUILD_DIR)
	@echo "Generating relocatable object $@"
	$(LD) -r -b binary $< -o $@ || { echo "LD binary failed"; exit 1; }
	@cp $@ .

# Dependency generation
depend: $(VM_SRCS) | $(BUILD_DIR)
	@echo "Generating dependencies"
	$(CC) $(C_FLAGS) -M $(filter %.c,$(VM_SRCS)) > $(BUILD_DIR)/depend.d

# Include dependencies
-include $(BUILD_DIR)/depend.d

# Clean up
clean:
	@echo "Cleaning up"
	rm -rf $(BUILD_DIR) $(OBJ)

# Phony targets
.PHONY: all clean depend