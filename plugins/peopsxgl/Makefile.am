AM_CPPFLAGS = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I../../include

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libpeopsxgl.la

libpeopsxgl_la_SOURCES = \
	cfg.c cfg.h \
	draw.c draw.h \
	fps.c fps.h \
	gl_ext.h \
	gpu.c gpu.h \
	key.c key.h \
	menu.c menu.h \
	prim.c prim.h \
	soft.c soft.h \
	texture.c texture.h \
	gte_accuracy.c gte_accuracy.h \
	pgxp_gpu.c pgxp_gpu.h
libpeopsxgl_la_LDFLAGS = -module -avoid-version
libpeopsxgl_la_LIBADD = -lX11 -lXxf86vm -lGL -lm

bin_PROGRAMS = cfgpeopsxgl
cfgpeopsxgl_SOURCES = gpucfg/main.c GtkResources.c GtkResources.h
cfgpeopsxgl_LDADD = $(GTK3_LIBS)

# gresource.xml is dynamically generated in CMake. Mirror that here in a static way.
#define gresource_xml =
define gresource_xml
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/pcsxr/peopsxgl">
    <file>peopsxgl.ui</file>
  </gresource>
</gresources>
endef
# FIXME
export gresource_xml

peopsxgl.gresource.xml:
#	$(file >$@,$(gresource_xml))
	@echo "$$gresource_xml" >$@
peopsxgl_gresource: peopsxgl.gresource.xml
	$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/gpucfg --generate-dependencies $(srcdir)/$<)
GtkResources.c: peopsxgl.gresource.xml $(peopsxgl_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/gpucfg --generate-source --c-name peopsxgl $<
GtkResources.h: peopsxgl.gresource.xml $(peopsxgl_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/gpucfg --generate-header --c-name peopsxgl $<


#glade_DATA = gpucfg/peopsxgl.ui
#gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)

CLEANFILES = peopsxgl.gresource.xml GtkResources.c GtkResources.h
