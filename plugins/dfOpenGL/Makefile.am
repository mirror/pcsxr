INCLUDES = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
        -DLOCALE_DIR=\"${datadir}/locale/\" \
        -DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) $(GLADE2_CFLAGS) -I/usr/X11R6/include \
	-g -O0 -Wall -fPIC

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libdfOpenGL.la

libdfOpenGL_la_SOURCES = GPU.c primitive_drawing.c fps.c gllog.c cfg.c
libdfOpenGL_la_LDFLAGS = -module -avoid-version -L/usr/lib \
	-L/usr/X11R6/lib64 -L/usr/X11R6/lib -lX11 -lXxf86vm -lGL -lm

bin_PROGRAMS = cfgDFOpenGL
cfgDFOpenGL_SOURCES = gpucfg-0.1df/main.c
cfgDFOpenGL_LDADD = $(GTK2_LIBS) $(GLADE2_LIBS)

glade_DATA = gpucfg-0.1df/dfopengl.glade2
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
