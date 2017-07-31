message("Configuring dfinput")

include(GlibCompileResourcesSupport)

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

set(USE_SDL2 0)
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

find_package(X11 REQUIRED)
find_package(Threads REQUIRED)

#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(PLUGIN_SRCS
    analog.c
    cfg.c
    pad.c
    sdljoy.c
    util.c
    xkb.c
)

set(GUI_SRCS
    cfg-gtk.c
)


#resources
set(RESOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/dfinput.ui
)


compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE EMBED_C
                   PREFIX /org/pcsxr/dfinput
                   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
                   RESOURCES ${RESOURCE_LIST})

add_custom_target(dfinput_resource DEPENDS ${RESOURCE_FILE})

include_directories(${CMAKE_SOURCE_DIR}/libpcsxcore ${X11_X11_INCLUDE_PATH})
add_library(DFInput MODULE ${PLUGIN_SRCS})
target_link_libraries(DFInput ${SDL_LIBRARY} ${SDL2_LIBRARY} ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


add_executable(cfgDFInput ${GUI_SRCS} ${RESOURCE_FILE})
add_dependencies(cfgDFInput dfinput_resource)
target_link_libraries(cfgDFInput ${GTK_LIBRARIES} ${SDL_LIBRARY} ${SDL2_LIBRARY} ${X11_LIBRARIES})

install(TARGETS DFInput LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
install(TARGETS cfgDFInput RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
