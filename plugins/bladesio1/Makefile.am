bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libBladeSio1.la

libBladeSio1_la_SOURCES = cfg.c sio1.c sio1.h fifo.c fifo.h connection.c connection.h typedefs.h
libBladeSio1_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I../../include

bin_PROGRAMS = cfgBladeSio1
cfgBladeSio1_SOURCES = gui.c sio1.h
cfgBladeSio1_LDADD = $(GTK3_LIBS)

glade_DATA = sio1.ui
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
