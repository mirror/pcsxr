message("Configuring dfxvideo")

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


find_package(X11 REQUIRED)

#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(PLUGIN_SRCS
    cfg.c
    draw.c
    fps.c
    gpu.c
    key.c
    menu.c
    prim.c
    soft.c
    zn.c
)

set(GUI_SRCS
    gpucfg-0.1df/main.c
)


#resources
set(RESOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/gpucfg-0.1df/dfxvideo.ui
)


compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE EMBED_C
                   PREFIX /org/pcsxr/dfxvideo
                   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gpucfg-0.1df
                   RESOURCES ${RESOURCE_LIST})

add_custom_target(dfxvideo_resource DEPENDS ${RESOURCE_FILE})

include_directories(${CMAKE_SOURCE_DIR}/libpcsxcore ${X11_X11_INCLUDE_PATH} ${X11_Xv_INCLUDE_PATH} ${X11_XShm_INCLUDE_PATH})
add_library(DFXVideo MODULE ${PLUGIN_SRCS})
target_link_libraries(DFXVideo ${X11_LIBRARIES} ${X11_Xv_LIB} ${X11_Xext_LIB} -lm)


add_executable(cfgDFXVideo ${GUI_SRCS} ${RESOURCE_FILE})
add_dependencies(cfgDFXVideo dfxvideo_resource)
target_link_libraries(cfgDFXVideo ${GTK_LIBRARIES})

install(TARGETS DFXVideo LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
install(TARGETS cfgDFXVideo RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
