cmake_minimum_required(VERSION 3.5.1)
project(X-Hyper)

enable_language(C ASM)
message("-- Building X Hypervisor --")

set(CROSS_COMPILE 		aarch64-linux-gnu-)
set(CMAKE_C_COMPILER 	"${CROSS_COMPILE}gcc")
set(CMAKE_ASM_COMPILER 	"${CROSS_COMPILE}gcc")
set(CMAKE_LINKER 		"${CROSS_COMPILE}ld")
set(CMAKE_ASM_FLAGS 	"-march=armv8-a+nosimd+nofp")
set(CMAKE_C_FLAGS   	"-march=armv8-a+nosimd+nofp")

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ffreestanding -Wextra -Wfatal-errors -Werror -O0 -g3 -D__ASSEMBLY__")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} -ffreestanding -Wall -Wextra -Wfatal-errors -Werror -Wno-psabi -O0 -g3 -D__LITTLE_ENDIAN")

include_directories("./hypervisor/include")
add_subdirectory(./hypervisor/src/lds)

set(X_HYPER_SRCS
	./hypervisor/src/head.S
	./hypervisor/src/vector.S
	./hypervisor/src/pl011.c
	./hypervisor/src/utils.c
	./hypervisor/src/spinlock.c
	./hypervisor/src/printf.c
	./hypervisor/src/xmalloc.c
	./hypervisor/src/kalloc.c
	./hypervisor/src/vmm.c
	./hypervisor/src/guest.c
	./hypervisor/src/vcpu.c
	./hypervisor/src/vm.c
	./hypervisor/src/el1_sync.c
	./hypervisor/src/vpsci.c
	./hypervisor/src/main.c

	./test/stage2_translation_test.c
)

set(CMAKE_C_FLAGS "-Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-override-init ${CMAKE_C_FLAGS}")

message("-- Compiling X-Hyper_libs --")
add_library(x_hyper_libs STATIC ${X_HYPER_SRCS})

message("-- link X-Hyper_libs --")
set(LINK_PATH ${PROJECT_BINARY_DIR})
set(LSCRIPT ${PROJECT_BINARY_DIR}/hypervisor/src/lds/CMakeFiles/ld.dir/linker.ld.S.o)
set(Guest_VM_Image ${PROJECT_BINARY_DIR}/../guest/Guest_VM.o)

if(EXISTS ${Guest_VM_Image})
	set(X_HYPER_LINK "${CMAKE_LINKER} -pie -Map X_Hyper.map -T${LSCRIPT} -L${LINK_PATH} \
					  -lx_hyper_libs -o X-Hyper.elf ${Guest_VM_Image}")
else()
	set(X_HYPER_LINK "${CMAKE_LINKER} -pie -Map X_Hyper.map -T${LSCRIPT} -L${LINK_PATH} \
					  -lx_hyper_libs -o X-Hyper.elf")
endif()

set(X_HYPER_IMAGE "${CROSS_COMPILE}objcopy -O binary -R .note -R .note.gnu.build-id \
				   -R .comment -S X-Hyper.elf X-Hyper")
add_executable(X-Hyper.elf ./hypervisor/src/main.c)
target_link_libraries(X-Hyper.elf ld)
set(CMAKE_C_LINK_EXECUTABLE ${X_HYPER_LINK}\n${X_HYPER_IMAGE})

