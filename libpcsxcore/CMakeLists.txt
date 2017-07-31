message("Configuring core")

set(DYNAREC "auto" CACHE STRING "Build dynarec for arch.")
set_property(CACHE DYNAREC PROPERTY STRINGS auto x86_64 x86 ppc no)

option(ENABLE_CCDDA "Enables compressed CDDA support." OFF)
option(USE_LIBARCHIVE "Enables compressed data-tracks support." OFF)

if (ENABLE_CCDDA)
  find_package(FFMPEG REQUIRED)
  include_directories(${FFMPEG_INCLUDE_DIRS})
  add_definitions(-DENABLE_CCDDA)
endif()

if (USE_LIBARCHIVE)
  find_package(LibArchive REQUIRED)
  include_directories(${LibArchive_INCLUDE_DIRS})
  add_definitions(-DHAVE_LIBARCHIVE)
endif()

# Architecture detection and arch specific settings
message(${CMAKE_SYSTEM_PROCESSOR})
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^powerpc")
    set(_ARCH_PPC 1)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^i.86")
    set(_ARCH_32 1)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^x86_64")
    set(_ARCH_64 1)
else()
    message("Unsupported arch. Will not build dynarec")
    add_definitions(-DNOPSXREC)
endif()


if (${DYNAREC} STREQUAL "auto")
    if (_ARCH_PPC)
        set(DYNAREC_PPC 1)
        message("Using PPC Dynarec")
    elseif(_ARCH_64)
        set(DYNAREC_64 1)
        message("Using x86_64 Dynarec")
    elseif(_ARCH_32)
        set(DYNAREC_32 1)
        message("Using x86 Dynarec")
    endif()
elseif (${DYNAREC} STREQUAL "ppc")
#if anyone ever fixes ppc dynarec
#    set(DYNAREC_PPC 1)
#    message("Using PPC Dynarec")
    message("PPC Dynarec is broken, sorry.")
    add_definitions(-DNOPSXREC)
elseif (${DYNAREC} STREQUAL "x86_64")
    set(DYNAREC_64 1)
    message("Using x86_64 Dynarec")
elseif (${DYNAREC} STREQUAL "x86")
    set(DYNAREC_32 1)
    message("Using x86 Dynarec")
elseif (${DYNAREC} STREQUAL "no")
    message("Will not build dynarec")
    add_definitions(-DNOPSXREC)
endif()


set(SRCS  psxbios.c
          cdrom.c
          psxcounters.c
          psxdma.c
          disr3000a.c
          gpu.c
          spu.c
          sio.c
          psxhw.c
          mdec.c
          psxmem.c
          misc.c
          plugins.c
          decode_xa.c
          r3000a.c
          psxinterpreter.c
          gte.c
          psxhle.c
          debug.c
          psxcommon.c
          cdriso.c
          cheat.c
          socket.c
          ppf.c)

set(LIBS "-lm")

if(DYNAREC_64)
    file(GLOB_RECURSE DYNAREC_SRC  ix86_64/*.c)
elseif(DYNAREC_32)
    file(GLOB_RECURSE DYNAREC_SRC  ix86/*.c)
elseif(DYNAREC_PPC)
    enable_language(ASM-ATT)
    SET(CMAKE_ASM-ATT_SOURCE_FILE_EXTENSIONS nasm;nas;asm;s)
    file(GLOB_RECURSE DYNAREC_SRC  ppc/*.c)
    set(DYNAREC_SRC ${DYNAREC_SRC} ppc/pasm.s)
endif()

set(SRCS ${SRCS} ${DYNAREC_SRC})

add_library(pcsxcore STATIC ${SRCS})
target_link_libraries(pcsxcore ${FFMPEG_LIBRARIES} ${LibArchive_LIBRARIES} ${LIBS})
