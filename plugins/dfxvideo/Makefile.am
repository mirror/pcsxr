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
cfgDFXVideo_SOURCES = gpucfg-0.1df/main.c GtkResources.c GtkResources.h
cfgDFXVideo_LDADD = $(GTK3_LIBS) -lXext

# gresource.xml is dynamically generated in CMake. Mirror that here in a static way.
#define gresource_xml =
define gresource_xml
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/pcsxr/dfxvideo">
    <file>dfxvideo.ui</file>
  </gresource>
</gresources>
endef
# FIXME
export gresource_xml

dfxvideo.gresource.xml:
#	$(file >$@,$(gresource_xml))
	@echo "$$gresource_xml" >$@
dfxvideo_gresource: dfxvideo.gresource.xml
	$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/gpucfg-0.1df --generate-dependencies $(srcdir)/$<)
GtkResources.c: dfxvideo.gresource.xml $(dfxvideo_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/gpucfg-0.1df --generate-source --c-name dfxvideo $<
GtkResources.h: dfxvideo.gresource.xml $(dfxvideo_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/gpucfg-0.1df --generate-header --c-name dfxvideo $<


#glade_DATA = gpucfg-0.1df/dfxvideo.ui
#gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)

CLEANFILES = dfxvideo.gresource.xml GtkResources.c GtkResources.h
