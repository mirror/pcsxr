bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFCdrom.la

libDFCdrom_la_SOURCES = cdr.c cfg.c
libDFCdrom_la_LDFLAGS = -module -avoid-version -lpthread

INCLUDES = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) \
	-I../../libpcsxcore -I../../include

bin_PROGRAMS = cfgDFCdrom
cfgDFCdrom_SOURCES = cdrcfg-0.1df/cfg-gtk2.c cdrcfg-0.1df/main.c \
	cdrcfg-0.1df/support.c cdrcfg-0.1df/interface.c \
	cdrcfg-0.1df/callbacks.c
cfgDFCdrom_LDADD = $(GTK2_LIBS)
