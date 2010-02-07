INCLUDES = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) $(GLADE2_CFLAGS) \
	-I../../include

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFSound.la

libDFSound_la_SOURCES = spu.c cfg.c dma.c freeze.c psemu.c registers.c \
	alsa.c oss.c pulseaudio.c pulseaudiosimple.c nullsnd.c

libDFSound_la_CFLAGS =
libDFSound_la_LDFLAGS = -module -avoid-version -lpthread -lm

if HAVE_ALSA
libDFSound_la_CFLAGS += -DUSEALSA=1
libDFSound_la_LDFLAGS += $(ALSA_LIBS)
endif

if HAVE_OSS
libDFSound_la_CFLAGS += -DUSEOSS=1
endif

if HAVE_PULSEAUDIOSIMPLE
libDFSound_la_CFLAGS += $(PULSEAUDIO_CFLAGS) -DUSEPULSEAUDIOSIMPLE=1
libDFSound_la_LDFLAGS += $(PULSEAUDIO_LIBS)
endif

if HAVE_PULSEAUDIO
libDFSound_la_CFLAGS += $(PULSEAUDIO_CFLAGS) -DUSEPULSEAUDIO=1
libDFSound_la_LDFLAGS += $(PULSEAUDIO_LIBS)
endif

bin_PROGRAMS = cfgDFSound
cfgDFSound_SOURCES = spucfg-0.1df/main.c
cfgDFSound_LDADD = $(GTK2_LIBS) $(GLADE2_LIBS)

glade_DATA = spucfg-0.1df/dfsound.glade2
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
