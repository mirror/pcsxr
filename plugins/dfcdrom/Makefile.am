bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFCdrom.la

libDFCdrom_la_SOURCES = cdr.c cfg.c
libDFCdrom_la_LDFLAGS = -module -avoid-version -lpthread

INCLUDES = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) \
	-DREVISION=1 -DBUILD=0 \
	-I../../libpcsxcore

bin_PROGRAMS = cfgDFCdrom
cfgDFCdrom_SOURCES = cfg-gtk2.c main.c support.c interface.c callbacks.c
cfgDFCdrom_LDADD = $(GTK2_LIBS)
