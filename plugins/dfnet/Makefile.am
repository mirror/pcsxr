bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFNet.la

libDFNet_la_SOURCES = dfnet.c dfnet.h unix.c cfg.c
libDFNet_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I../../include

bin_PROGRAMS = cfgDFNet
cfgDFNet_SOURCES = gui.c GtkResources.c GtkResources.h
cfgDFNet_LDADD = $(GTK3_LIBS)

# gresource.xml is dynamically generated in CMake. Mirror that here in a static way.
#define gresource_xml =
define gresource_xml
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/pcsxr/dfnet">
    <file>dfnet.ui</file>
  </gresource>
</gresources>
endef
# FIXME
export gresource_xml

dfnet.gresource.xml:
#	$(file >$@,$(gresource_xml))
	@echo "$$gresource_xml" >$@
dfnet_gresource: dfnet.gresource.xml
	$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/$<)
GtkResources.c: dfnet.gresource.xml $(dfnet_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source --c-name dfnet $<
GtkResources.h: dfnet.gresource.xml $(dfnet_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-header --c-name dfnet $<


#glade_DATA = dfnet.ui
#gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)

CLEANFILES = dfnet.gresource.xml GtkResources.c GtkResources.h
