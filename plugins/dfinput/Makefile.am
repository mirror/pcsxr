bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libDFInput.la

libDFInput_la_SOURCES = cfg.c pad.c pad.h sdljoy.c xkb.c analog.c util.c util.h
libDFInput_la_LDFLAGS = -module -avoid-version 
libDFInput_la_LIBADD = -lpthread -lX11 $(SDL2_LIBS) $(SDL_LIBS)

AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I../../include -I$(top_srcdir)/libpcsxcore $(SDL2_CFLAGS) $(SDL_CFLAGS)

bin_PROGRAMS = cfgDFInput
cfgDFInput_SOURCES = cfg-gtk.c pad.h GtkResources.c GtkResources.h
cfgDFInput_LDADD = $(GTK3_LIBS) $(SDL2_LIBS) $(SDL_LIBS) -lX11

# gresource.xml is dynamically generated in CMake. Mirror that here in a static way.
#define gresource_xml =
define gresource_xml
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/pcsxr/dfinput">
    <file>dfinput.ui</file>
  </gresource>
</gresources>
endef
# FIXME
export gresource_xml

dfinput.gresource.xml:
#	$(file >$@,$(gresource_xml))
	@echo "$$gresource_xml" >$@
dfinput_gresource: dfinput.gresource.xml
	$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/$<)
GtkResources.c: dfinput.gresource.xml $(dfinput_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source --c-name dfinput $<
GtkResources.h: dfinput.gresource.xml $(dfinput_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-header --c-name dfinput $<


#glade_DATA = dfinput.ui
#gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)

CLEANFILES = dfinput.gresource.xml GtkResources.c GtkResources.h
