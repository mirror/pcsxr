bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libBladeSio1.la

libBladeSio1_la_SOURCES = cfg.c sio1.c sio1.h fifo.c fifo.h connection.c connection.h typedefs.h
libBladeSio1_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I../../include

bin_PROGRAMS = cfgBladeSio1
cfgBladeSio1_SOURCES = gui.c sio1.h GtkResources.c GtkResources.h
cfgBladeSio1_LDADD = $(GTK3_LIBS)

# gresource.xml is dynamically generated in CMake. Mirror that here in a static way.
#define gresource_xml =
define gresource_xml
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/pcsxr/bladesio1">
    <file>sio1.ui</file>
  </gresource>
</gresources>
endef
# FIXME
export gresource_xml

bladesio1.gresource.xml:
#	$(file >$@,$(gresource_xml))
	@echo "$$gresource_xml" >$@
bladesio1_gresource: bladesio1.gresource.xml
	$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/$<)
GtkResources.c: bladesio1.gresource.xml $(bladesio1_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source --c-name bladesio1 $<
GtkResources.h: bladesio1.gresource.xml $(bladesio1_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-header --c-name bladesio1 $<


#glade_DATA = sio1.ui
#gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)

CLEANFILES = bladesio1.gresource.xml GtkResources.c GtkResources.h
