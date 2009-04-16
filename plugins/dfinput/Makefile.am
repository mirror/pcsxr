bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFInput.la

libDFInput_la_SOURCES = pad.c padjoy.h
libDFInput_la_LDFLAGS = -module -avoid-version -lpthread -lX11 \
	-L/usr/X11R6/lib64 -L/usr/X11R6/lib

INCLUDES = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) $(GLADE2_CFLAGS) -I/usr/X11R6/include \
	-DVERSION=0 -DREVISION=1 -DBUILD=0

bin_PROGRAMS = cfgDFInput
cfgDFInput_SOURCES = cfg.c padjoy.h
cfgDFInput_LDADD = $(GTK2_LIBS) $(GLADE2_LIBS)

glade_DATA = dfinput.glade2
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
