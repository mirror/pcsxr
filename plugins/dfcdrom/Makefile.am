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
cfgDFCdrom_SOURCES = cdrcfg-0.1df/main.c GtkResources.c GtkResources.h
cfgDFCdrom_LDADD = $(GTK3_LIBS)

# gresource.xml is dynamically generated in CMake. Mirror that here in a static way.
#define gresource_xml =
define gresource_xml
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/pcsxr/dfcdrom">
    <file>dfcdrom.ui</file>
  </gresource>
</gresources>
endef
# FIXME
export gresource_xml

dfcdrom.gresource.xml:
#	$(file >$@,$(gresource_xml))
	@echo "$$gresource_xml" >$@
dfcdrom_gresource: dfcdrom.gresource.xml
	$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/cdrcfg-0.1df --generate-dependencies $(srcdir)/$<)
GtkResources.c: dfcdrom.gresource.xml $(dfcdrom_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/cdrcfg-0.1df --generate-source --c-name dfcdrom $<
GtkResources.h: dfcdrom.gresource.xml $(dfcdrom_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/cdrcfg-0.1df --generate-header --c-name dfcdrom $<


#glade_DATA = cdrcfg-0.1df/dfcdrom.ui
#gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)

CLEANFILES = dfcdrom.gresource.xml GtkResources.c GtkResources.h
