cmake_minimum_required(VERSION 3.1)

# Avoid having empty buildtype
set(CMAKE_BUILD_TYPE_INIT "Release")

# Project name
project(neogeo_libretro)
set(PROJECT_NAME neogeo_libretro)

# Under Windows we want static libraries
if (WIN32)
    set(BUILD_STATIC 1)
endif (WIN32)

# Check if the compiler can handle modern C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add the current directory to includes
#set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

################################################################
#                       Compiler options                       #
# Uncomment one line suitable for your system or make your own #
################################################################

# Set to "-march=native" on non x86 platforms
set(MACHINE_OPTIONS "-march=x86-64 -mtune=generic")

# Base options
set(BASE_OPTIONS "-Ofast -fomit-frame-pointer -ffast-math")

# Set CFLAGS
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${MACHINE_OPTIONS} ${BASE_OPTIONS}")

# Linker options
if (CMAKE_BUILD_TYPE MATCHES Release)
    set(LINK_OPTIONS -s)
endif()

# Library path
#set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L. ")

################################################################
#                      3rd party libraries                     #
#                                                              #
################################################################

# Define the m68k sources
set ( M68K_C_SRCS
	${CMAKE_SOURCE_DIR}/src/3rdparty/musashi/m68kcpu.c
#	${CMAKE_SOURCE_DIR}/src/3rdparty/musashi/m68kdasm.c
	${CMAKE_SOURCE_DIR}/src/3rdparty/musashi/m68kopac.c
    ${CMAKE_SOURCE_DIR}/src/3rdparty/musashi/m68kopdm.c
    ${CMAKE_SOURCE_DIR}/src/3rdparty/musashi/m68kopnz.c
    ${CMAKE_SOURCE_DIR}/src/3rdparty/musashi/m68kops.c
)

set ( M68K_H_SRCS
	${CMAKE_SOURCE_DIR}/src/3rdparty/musashi/m68kconf.h
)

add_library(m68k OBJECT ${M68K_C_SRCS} ${M68K_H_SRCS})

# Define the z80 sources
set ( Z80_C_SRCS
    ${CMAKE_SOURCE_DIR}/src/3rdparty/z80/z80daisy.c
    ${CMAKE_SOURCE_DIR}/src/3rdparty/z80/z80.c
)

add_library(z80 OBJECT ${Z80_C_SRCS})

# Define the ym2610 sources
set ( YM2610_C_SRCS
    ${CMAKE_SOURCE_DIR}/src/3rdparty/ym/ym2610.c
	${CMAKE_SOURCE_DIR}/src/3rdparty/ym/ym_delta_t.c
	${CMAKE_SOURCE_DIR}/src/3rdparty/ym/ym_ssg.c
)

add_library(ym2610 OBJECT ${YM2610_C_SRCS})

# Define the miniz sources
set ( MINIZ_C_SRCS
    ${CMAKE_SOURCE_DIR}/src/3rdparty/miniz/miniz.c
)

add_library(miniz OBJECT ${MINIZ_C_SRCS})

#define PD4990a sources
set ( PD4990A_C_SRCS
	${CMAKE_SOURCE_DIR}/src/3rdparty/pd4990a/pd4990a.c )

add_library(pd4990a OBJECT ${PD4990A_C_SRCS})

################################################################
#                        Libretro core                         #
#                                                              #
################################################################

# Define the C sources
set ( C_SRCS
	${CMAKE_SOURCE_DIR}/src/aux_inputs.c
	${CMAKE_SOURCE_DIR}/src/cartridge.c
	${CMAKE_SOURCE_DIR}/src/common_tools.c
	${CMAKE_SOURCE_DIR}/src/joypads.c
    ${CMAKE_SOURCE_DIR}/src/libretro.c
    ${CMAKE_SOURCE_DIR}/src/libretro_core.c
	${CMAKE_SOURCE_DIR}/src/memory_backup_ram.c
	${CMAKE_SOURCE_DIR}/src/memory_input_output.c
	${CMAKE_SOURCE_DIR}/src/memory_palettes_ram.c
	${CMAKE_SOURCE_DIR}/src/memory_work_ram.c
	${CMAKE_SOURCE_DIR}/src/mvs_dips.c
    ${CMAKE_SOURCE_DIR}/src/m68k_interface.c
    ${CMAKE_SOURCE_DIR}/src/neogeo.c
	${CMAKE_SOURCE_DIR}/src/sound.c
    ${CMAKE_SOURCE_DIR}/src/timer.c
	${CMAKE_SOURCE_DIR}/src/timers_group.c
    ${CMAKE_SOURCE_DIR}/src/video.c
    ${CMAKE_SOURCE_DIR}/src/z80intf.c
)

# Define the H sources
set ( H_SRCS
	${CMAKE_SOURCE_DIR}/src/aux_inputs.h
	${CMAKE_SOURCE_DIR}/src/cartridge.h
	${CMAKE_SOURCE_DIR}/src/common_tools.h
	${CMAKE_SOURCE_DIR}/src/endian.h
	${CMAKE_SOURCE_DIR}/src/joypads.h
    ${CMAKE_SOURCE_DIR}/src/libretro.h
    ${CMAKE_SOURCE_DIR}/src/libretro_core.h
	${CMAKE_SOURCE_DIR}/src/log.h
	${CMAKE_SOURCE_DIR}/src/memory_backup_ram.h
	${CMAKE_SOURCE_DIR}/src/memory_input_output.h
	${CMAKE_SOURCE_DIR}/src/memory_mapping.h
	${CMAKE_SOURCE_DIR}/src/memory_palettes_ram.h
	${CMAKE_SOURCE_DIR}/src/memory_region.h
	${CMAKE_SOURCE_DIR}/src/memory_work_ram.h
	${CMAKE_SOURCE_DIR}/src/mvs_dips.h
    ${CMAKE_SOURCE_DIR}/src/neogeo.h
	${CMAKE_SOURCE_DIR}/src/rom_region.h
	${CMAKE_SOURCE_DIR}/src/sound.h
    ${CMAKE_SOURCE_DIR}/src/timer.h
	${CMAKE_SOURCE_DIR}/src/timers_group.h
    ${CMAKE_SOURCE_DIR}/src/video.h
)

add_library(${PROJECT_NAME} SHARED ${C_SRCS} ${H_SRCS} $<TARGET_OBJECTS:m68k> $<TARGET_OBJECTS:z80> $<TARGET_OBJECTS:ym2610> $<TARGET_OBJECTS:miniz> $<TARGET_OBJECTS:pd4990a>)

target_link_libraries(${PROJECT_NAME} ${LINK_OPTIONS})

message("")
message("Configuration Summary")
message("---------------------")
message("CMAKE_BUILD_TYPE:        ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_FLAGS_RELEASE:   ${CMAKE_C_FLAGS_RELEASE}")
message("LINK_OPTIONS:            ${LINK_OPTIONS}")
message("")
