message("Configuring nullsio1")


#deps


#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(PLUGIN_SRCS
    sio1.c
)


include_directories(${CMAKE_SOURCE_DIR}/libpcsxcore)
add_library(NullSio1 MODULE ${PLUGIN_SRCS})



install(TARGETS NullSio1 LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
