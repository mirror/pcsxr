STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile \
	$(STRIP_FPIC) $(NASM) -f elf -d ELF -I${srcdir}/ $<

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK2_CFLAGS) $(GLADE2_CFLAGS) \
	-I../../libpcsxcore \
	-I../../include

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

noinst_LTLIBRARIES = libcfg.la libglobals.la
libcfg_la_SOURCES = cfg.c cfg.h globals.h

libglobals_la_SOURCES = globals.c

lib_LTLIBRARIES = libGXVideo.la

libGXVideo_la_SOURCES = gpu.c gpu.h draw.c draw.h fps.c fps.h key.c key.h menu.c menu.h prim.c prim.h soft.c soft.h gpu_utils.h interp.h swap.h
if X86_NASM
libGXVideo_la_SOURCES += i386.asm macros.inc
AM_CPPFLAGS += -DUSE_NASM=1
endif
libGXVideo_la_LDFLAGS = -module -avoid-version
libGXVideo_la_LIBADD = libcfg.la libglobals.la -lX11 -lXv -lXext -lm

bin_PROGRAMS = cfgGXVideo
cfgGXVideo_SOURCES = gpucfg/main.c 
cfgGXVideo_LDADD = libcfg.la libglobals.la $(GTK2_LIBS) $(GLADE2_LIBS) -lXext 

glade_DATA = gpucfg/gxvideo.glade
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
