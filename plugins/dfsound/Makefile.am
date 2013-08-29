AM_CPPFLAGS = -I../../include

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFSound.la

libDFSound_la_SOURCES = adsr.c adsr.h spu.c spu.h cfg.c cfg.h dma.c dma.h gauss_i.h freeze.c reverb.c reverb.h registers.c registers.h externals.c externals.h dsoundoss.h psemuxa.h regs.h xa.c xa.h stdafx.h

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
libDFSound_la_CPPFLAGS += -DUSESDL=1 $(SDL2_CFLAGS) $(SDL_CFLAGS)
libDFSound_la_LIBADD += $(SDL2_LIBS) $(SDL_LIBS)
endif

if SOUND_OPENAL
libDFSound_la_SOURCES += openal.c
libDFSound_la_CPPFLAGS += -DUSEOPENAL=1
libDFSound_la_LIBADD += $(OPENAL_LIBS)
endif

if SOUND_NULL
libDFSound_la_SOURCES += nullsnd.c
libDFSound_la_CPPFLAGS += -DUSENULL=1
endif

bin_PROGRAMS = cfgDFSound
cfgDFSound_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) $(AM_CPPFLAGS)
cfgDFSound_SOURCES = spucfg-0.1df/main.c
cfgDFSound_LDADD = $(GTK3_LIBS)

glade_DATA = spucfg-0.1df/dfsound.ui
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
