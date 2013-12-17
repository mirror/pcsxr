bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFCdrom.la

libDFCdrom_la_SOURCES = \
	cdr.c \
	cdr.h \
	cfg.c \
	cdr-linux.c \
	cdr-null.c \
	cdr-libcdio.c \
	util.c

libDFCdrom_la_CFLAGS =
libDFCdrom_la_LDFLAGS = -module -avoid-version 
libDFCdrom_la_LIBADD = -lpthread

if USE_LIBCDIO
libDFCdrom_la_CFLAGS += -DUSE_LIBCDIO=1 $(LIBCDIO_CFLAGS)
libDFCdrom_la_LIBADD += $(LIBCDIO_LIBS)
endif

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I../../include

bin_PROGRAMS = cfgDFCdrom
cfgDFCdrom_SOURCES = cdrcfg-0.1df/main.c
cfgDFCdrom_LDADD = $(GTK3_LIBS)

glade_DATA = cdrcfg-0.1df/dfcdrom.ui
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
