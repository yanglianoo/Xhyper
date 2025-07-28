message("-- Compiling linder.ld.S --")
set(CMAKE_ASM_FLAGS "-P -E -D__ASSEMBLY__")
add_library(ld STATIC linker.ld.S)

