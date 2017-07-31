message("Configuring bladesio1")

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


#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(PLUGIN_SRCS
    cfg.c
    sio1.c
    fifo.c
    connection.c
)

set(GUI_SRCS
    gui.c
)


#resources
set(RESOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/sio1.ui
)


compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE EMBED_C
                   PREFIX /org/pcsxr/bladesio1
                   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
                   RESOURCES ${RESOURCE_LIST})

add_custom_target(bladesio1_resource DEPENDS ${RESOURCE_FILE})

include_directories(${CMAKE_SOURCE_DIR}/libpcsxcore)
add_library(BladeSio1 MODULE ${PLUGIN_SRCS})


add_executable(cfgBladeSio1 ${GUI_SRCS} ${RESOURCE_FILE})
add_dependencies(cfgBladeSio1 bladesio1_resource)
target_link_libraries(cfgBladeSio1 ${GTK_LIBRARIES})

install(TARGETS BladeSio1 LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
install(TARGETS cfgBladeSio1 RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
