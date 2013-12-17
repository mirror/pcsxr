STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile \
	$(STRIP_FPIC) $(NASM) -f elf -d ELF -I${srcdir}/ $<

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS)\
	-I$(top_srcdir)/libpcsxcore \
	-I../../include

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFXVideo.la

libDFXVideo_la_SOURCES = gpu.c gpu.h cfg.c cfg.h draw.c draw.h fps.c fps.h key.c key.h menu.c menu.h prim.c prim.h soft.c soft.h zn.c swap.h externals.h hq2x.h hq3x.h interp.h
if X86_NASM
libDFXVideo_la_SOURCES += i386.asm macros.inc
AM_CPPFLAGS += -DUSE_NASM=1
endif
libDFXVideo_la_LDFLAGS = -module -avoid-version
libDFXVideo_la_LIBADD = -lX11 -lXv -lXext -lm

bin_PROGRAMS = cfgDFXVideo
cfgDFXVideo_SOURCES = gpucfg-0.1df/main.c
cfgDFXVideo_LDADD = $(GTK3_LIBS) -lXext

glade_DATA = gpucfg-0.1df/dfxvideo.ui
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
