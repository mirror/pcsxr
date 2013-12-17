bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFNet.la

libDFNet_la_SOURCES = dfnet.c dfnet.h unix.c cfg.c
libDFNet_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I../../include

bin_PROGRAMS = cfgDFNet
cfgDFNet_SOURCES = gui.c
cfgDFNet_LDADD = $(GTK3_LIBS)

glade_DATA = dfnet.ui
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
