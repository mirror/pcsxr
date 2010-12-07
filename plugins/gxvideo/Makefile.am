STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile \
	$(STRIP_FPIC) $(NASM) -f elf -d ELF -I${srcdir}/ $<

INCLUDES = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) $(GLADE2_CFLAGS) -I/usr/X11R6/include \
	-I../../libpcsxcore \
	-I../../include

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

noinst_LIBRARIES = libcfg.a libglobals.a
libcfg_a_SOURCES = cfg.c
libcfg_a_CFLAGS = -fPIC

libglobals_a_SOURCES = globals.c
libglobals_a_CFLAGS = -fPIC

lib_LTLIBRARIES = libGXVideo.la

libGXVideo_la_SOURCES = gpu.c draw.c fps.c key.c menu.c prim.c soft.c
if X86_NASM
libGXVideo_la_SOURCES += i386.asm
INCLUDES += -DUSE_NASM=1
endif
libGXVideo_la_LDFLAGS = -module -avoid-version
libGXVideo_la_LDFLAGS += -L/usr/X11R6/lib64 -L/usr/X11R6/lib \
	-lX11 -lXv -lXext -lm libcfg.a libglobals.a

bin_PROGRAMS = cfgGXVideo
cfgGXVideo_SOURCES = gpucfg/main.c 
cfgGXVideo_LDADD = $(GTK2_LIBS) $(GLADE2_LIBS) -L/usr/X11R6/lib -lXext libcfg.a libglobals.a

glade_DATA = gpucfg/gxvideo.glade
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
