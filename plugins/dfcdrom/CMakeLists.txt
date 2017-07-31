message("Configuring dfcdrom")

include(GlibCompileResourcesSupport)

option(USE_LIBCDIO "Use GNU libcdio for CD-ROM support." OFF)

if (USE_LIBCDIO)
  find_package(Cdio REQUIRED)
  include_directories(${CDIO_INCLUDE_DIR})
  add_definitions(-DUSE_LIBCDIO)
endif()

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

find_package(Threads REQUIRED)

#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(PLUGIN_SRCS
    cdr.c
    cdr-libcdio.c
    cdr-linux.c
    cdr-null.c
    cfg.c
    util.c
)

set(GUI_SRCS
    cdrcfg-0.1df/main.c
)


#resources
set(RESOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/cdrcfg-0.1df/dfcdrom.ui
)


compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE EMBED_C
                   PREFIX /org/pcsxr/dfcdrom
                   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cdrcfg-0.1df
                   RESOURCES ${RESOURCE_LIST})

add_custom_target(dfcdrom_resource DEPENDS ${RESOURCE_FILE})

include_directories(${CMAKE_SOURCE_DIR}/libpcsxcore)
add_library(DFCdrom MODULE ${PLUGIN_SRCS})
target_link_libraries(DFCdrom ${CDIO_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(cfgDFCdrom ${GUI_SRCS} ${RESOURCE_FILE})
add_dependencies(cfgDFCdrom dfcdrom_resource)
target_link_libraries(cfgDFCdrom ${GTK_LIBRARIES})

install(TARGETS DFCdrom LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
install(TARGETS cfgDFCdrom RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
