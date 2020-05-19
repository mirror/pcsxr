message(STATUS "* Configuring UI")

include(GlibCompileResourcesSupport)

#deps
find_package(GLib REQUIRED)
if(NOT GLib_FOUND)
    message(FATAL_ERROR "GLIB2 library not found")
endif(NOT GLib_FOUND)
include_directories(${GLib_INCLUDE_DIRS})

find_package(GTK3 3.14.0 REQUIRED)
if(NOT GTK3_FOUND)
    message(FATAL_ERROR "GTK3 library not found")
endif(NOT GTK3_FOUND)
include_directories(${GTK3_INCLUDE_DIRS})
set(GTK_LIBRARIES ${GTK3_LIBRARY} ${GDK3_LIBRARY} ${GDKPixbuf_LIBRARY} ${Pango_LIBRARY} ${Cairo_LIBRARY} ${GObject_LIBRARY} ${GLib_LIBRARY} ${GIO_LIBRARY})

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_library(DL_LIB dl REQUIRED)
find_library(RT_LIB rt REQUIRED)

find_package(X11 REQUIRED)
include_directories(${X11_XTest_INCLUDE_PATH})

if (${DYNAREC} STREQUAL "no")
    message(STATUS "User selected to not build dynarec")
    add_definitions(-DNOPSXREC)
endif()

#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(SRCS
    AboutDlg.c
    Cheat.c
    ConfDlg.c
    Config.c
    DebugMemory.c
    GtkGui.c
    LnxMain.c
    MemcardDlg.c
    Plugin.c
    resources.c
)


#resources
set(RESOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pcsxr.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-icon.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-cd-open.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-plugin-cdr.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-plugin-gpu.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-iso-open.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-iso-reopen.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-memcard.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-plugin-pad.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-play.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-plugin-sio1.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-run-bios.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-save-state.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-load-state.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-settings.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-plugin-netplay.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-cpu.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-shutdown.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-reset.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-run-exe.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-plugin-spu.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-cheats.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-cheats-search.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-cheats-browse.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-exit.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-about.png
    ${CMAKE_CURRENT_SOURCE_DIR}/data/pixmaps/pcsxr-memory-dump.png
)


compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE EMBED_C
                   PREFIX /org/pcsxr/gui
                   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data
                   RESOURCES ${RESOURCE_LIST})

add_custom_target(resource DEPENDS ${RESOURCE_FILE})
add_executable(pcsxr ${SRCS} ${RESOURCE_FILE} )
add_dependencies(pcsxr resource)
target_link_libraries(pcsxr pcsxcore ${GTK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES} ${RT_LIB} ${DL_LIB} ${X11_XTest_LIB})

install(TARGETS pcsxr RUNTIME DESTINATION bin)
install(FILES data/pcsxr.desktop DESTINATION share/applications)
install(FILES data/pixmaps/pcsxr-icon.png DESTINATION share/icons/hicolor/48x48/apps)

add_subdirectory("po")
