bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFInput.la

libDFInput_la_SOURCES = cfg.c pad.c pad.h sdljoy.c xkb.c analog.c util.c util.h
libDFInput_la_LDFLAGS = -module -avoid-version 
libDFInput_la_LIBADD = -lpthread -lX11 $(SDL_LIBS)

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) $(GLADE2_CFLAGS) \
	-I../../include -I../../libpcsxcore $(SDL_CFLAGS)

bin_PROGRAMS = cfgDFInput
cfgDFInput_SOURCES = cfg-gtk2.c pad.h
cfgDFInput_LDADD = $(GTK2_LIBS) $(GLADE2_LIBS) $(SDL_LIBS) -lX11

glade_DATA = dfinput.ui
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
