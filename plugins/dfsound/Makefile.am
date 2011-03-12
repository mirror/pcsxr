AM_CPPFLAGS = -I../../include -I../../libpcsxcore

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFSound.la

libDFSound_la_SOURCES = spu.c cfg.c dma.c freeze.c registers.c

libDFSound_la_CPPFLAGS = $(AM_CPPFLAGS)
libDFSound_la_LDFLAGS = -module -avoid-version 
libDFSound_la_LIBADD = -lpthread -lm

if SOUND_ALSA
libDFSound_la_SOURCES += alsa.c
libDFSound_la_CPPFLAGS += -DUSEALSA=1
libDFSound_la_LIBADD += $(ALSA_LIBS)
endif

if SOUND_OSS
libDFSound_la_SOURCES += oss.c
libDFSound_la_CPPFLAGS += -DUSEOSS=1
endif

if SOUND_PULSEAUDIO
libDFSound_la_SOURCES += pulseaudio.c
libDFSound_la_CPPFLAGS += -DUSEPULSEAUDIO=1 $(PULSEAUDIO_CFLAGS)
libDFSound_la_LIBADD += $(PULSEAUDIO_LIBS)
endif

if SOUND_SDL
libDFSound_la_SOURCES += sdl.c
libDFSound_la_CPPFLAGS += -DUSESDL=1 $(SDL_CFLAGS)
libDFSound_la_LIBADD += $(SDL_LIBS)
endif

if SOUND_NULL
libDFSound_la_SOURCES += nullsnd.c
libDFSound_la_CPPFLAGS += -DUSENULL=1
endif

bin_PROGRAMS = cfgDFSound
cfgDFSound_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) $(GLADE2_CFLAGS) $(AM_CPPFLAGS)
cfgDFSound_SOURCES = spucfg-0.1df/main.c
cfgDFSound_LDADD = $(GTK2_LIBS) $(GLADE2_LIBS)

glade_DATA = spucfg-0.1df/dfsound.glade2
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
