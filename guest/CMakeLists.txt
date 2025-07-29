cmake_minimum_required(VERSION 3.5.1)
project(GUEST_VM)

enable_language(C ASM)
message("-- Building Guest VM --")

set(CROSS_COMPILE 		aarch64-linux-gnu-)
set(CMAKE_C_COMPILER 	"${CROSS_COMPILE}gcc")
set(CMAKE_ASM_COMPILER 	"${CROSS_COMPILE}gcc")
set(CMAKE_LINKER 		"${CROSS_COMPILE}ld")
set(CMAKE_ASM_FLAGS 	"-march=armv8-a+nosimd+nofp")
set(CMAKE_C_FLAGS   	"-march=armv8-a+nosimd+nofp")

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ffreestanding -Wextra -Wfatal-errors -Werror -O0 -g3 -D__ASSEMBLY__")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} -ffreestanding -Wall -Wextra -Wfatal-errors -Werror -Wno-psabi -O0 -g3 -D__LITTLE_ENDIAN")

include_directories("./include")

set(VM_SRCS
	./head.S
	./pl011.c
	./printf.c
	./libs.c
	./main.c
)

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ffreestanding -Wextra -Wfatal-errors -Werror -O0 -g3 -D__ASSEMBLY__")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} -ffreestanding -Wall -Wextra -Wfatal-errors -Werror -Wno-psabi -O0 -g3 -D__LITTLE_ENDIAN")

message("-- Compiling VM_libs --")
add_library(vm_libs STATIC ${VM_SRCS})

set(LINK_PATH ${PROJECT_BINARY_DIR})
set(LSCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

set(Guest_VM_LINK "${CMAKE_LINKER} -pie -Map VM.map -T${LSCRIPT} -L${LINK_PATH} \
		     -lvm_libs -o Guest_VM.elf")
set(Guest_VM_IMAGE "${CROSS_COMPILE}objcopy -O binary -R .note -R .note.gnu.build-id \
				   -R .comment -S Guest_VM.elf Guest_VM")
add_executable(Guest_VM.elf ./main.c)
set(CMAKE_C_LINK_EXECUTABLE ${Guest_VM_LINK}\n${Guest_VM_IMAGE})

