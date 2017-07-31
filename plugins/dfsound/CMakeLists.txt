message("Configuring dfsound")

include(GlibCompileResourcesSupport)

set(SND_BACKEND "sdl" CACHE STRING "Sound backend to use.")
set_property(CACHE SND_BACKEND PROPERTY STRINGS oss alsa pulse sdl openal null)

#deps

find_package(GLib REQUIRED)
if(NOT GLib_FOUND)
    message(FATAL_ERROR "GLIB2 library not found")
endif(NOT GLib_FOUND)
include_directories(${GLib_INCLUDE_DIRS})

find_package(GTK3 REQUIRED)
if(NOT GTK3_FOUND)
    message(FATAL_ERROR "GTK3 library not found")
endif(NOT GTK3_FOUND)
include_directories(${GTK3_INCLUDE_DIRS})
set(GTK_LIBRARIES ${GTK3_LIBRARY} ${GDK3_LIBRARY} ${GDKPixbuf_LIBRARY} ${Pango_LIBRARY} ${Cairo_LIBRARY} ${GObject_LIBRARY} ${GLib_LIBRARY} ${GIO_LIBRARY})


if (${SND_BACKEND} STREQUAL "oss")
  find_package(OSS REQUIRED)
  include_directories(${OSS_INCLUDE_DIR})
  add_definitions(-DUSEOSS=1)

elseif (${SND_BACKEND} STREQUAL "alsa")
  find_package(ALSA REQUIRED)
  include_directories(${ALSA_INCLUDE_DIRS})
  add_definitions(-DUSEALSA=1)

elseif (${SND_BACKEND} STREQUAL "pulse")
  find_package(PulseAudio REQUIRED)
  include_directories(${PULSEAUDIO_INCLUDE_DIR})
  add_definitions(-DUSEPULSEAUDIO=1)

elseif (${SND_BACKEND} STREQUAL "sdl")
  find_package(SDL2)
  if(NOT SDL2_FOUND)
    find_package(SDL)
      if(NOT SDL_FOUND)
          message(FATAL_ERROR "SDL or SDL2 library not found")
      else(NOT SDL_FOUND)
          message("Using SDL")
          include_directories(${SDL_INCLUDE_DIR})
      endif(NOT SDL_FOUND)
  else(NOT SDL2_FOUND)
      set(USE_SDL2 1)
      message("Using SDL2")
      include_directories(${SDL2_INCLUDE_DIR})
  endif(NOT SDL2_FOUND)
  add_definitions(-DUSESDL=1)

elseif (${SND_BACKEND} STREQUAL "openal")
  find_package(OpenAL REQUIRED)
  include_directories(${OPENAL_INCLUDE_DIR})
  add_definitions(-DUSEOPENAL=1)
elseif (${SND_BACKEND} STREQUAL "null")
  add_definitions(-DUSENULL=1)

endif()

find_package(Threads REQUIRED)

#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(PLUGIN_SRCS
    adsr.c
    cfg.c
    dma.c
    externals.c
    freeze.c
    registers.c
    reverb.c
    spu.c
    xa.c
)

set(GUI_SRCS
    spucfg-0.1df/main.c
)


#resources
set(RESOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/spucfg-0.1df/dfsound.ui
)


compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE EMBED_C
                   PREFIX /org/pcsxr/dfsound
                   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spucfg-0.1df
                   RESOURCES ${RESOURCE_LIST})

add_custom_target(dfsound_resource DEPENDS ${RESOURCE_FILE})

include_directories(${CMAKE_SOURCE_DIR}/libpcsxcore)

if (${SND_BACKEND} STREQUAL "oss")
    add_library(DFSound MODULE ${PLUGIN_SRCS} oss.c)

elseif (${SND_BACKEND} STREQUAL "alsa")
    add_library(DFSound MODULE ${PLUGIN_SRCS} alsa.c)
    target_link_libraries(DFSound ${ALSA_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

elseif (${SND_BACKEND} STREQUAL "pulse")
    add_library(DFSound MODULE ${PLUGIN_SRCS} pulseaudio.c)
    target_link_libraries(DFSound ${PULSEAUDIO_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

elseif (${SND_BACKEND} STREQUAL "sdl")
    add_library(DFSound MODULE ${PLUGIN_SRCS} sdl.c)
    target_link_libraries(DFSound ${SDL_LIBRARY} ${SDL2_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

elseif (${SND_BACKEND} STREQUAL "openal")
    add_library(DFSound MODULE ${PLUGIN_SRCS} openal.c)
    target_link_libraries(DFSound ${OPENAL_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

elseif (${SND_BACKEND} STREQUAL "null")
    add_library(DFSound MODULE ${PLUGIN_SRCS} nullsnd.c)

endif()


add_executable(cfgDFSound ${GUI_SRCS} ${RESOURCE_FILE})
add_dependencies(cfgDFSound dfsound_resource)
target_link_libraries(cfgDFSound ${GTK_LIBRARIES})

install(TARGETS DFSound LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
install(TARGETS cfgDFSound RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
