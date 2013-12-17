AM_CPPFLAGS = -DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	-DDATADIR=\"${datadir}/psemu/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I../../include

bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libpeopsxgl.la

libpeopsxgl_la_SOURCES = cfg.c cfg.h draw.c draw.h fps.c fps.h gl_ext.h \
	gpu.c gpu.h key.c key.h menu.c menu.h prim.c prim.h soft.c soft.h \
	texture.c texture.h gte_accuracy.c gte_accuracy.h
libpeopsxgl_la_LDFLAGS = -module -avoid-version
libpeopsxgl_la_LIBADD = -lX11 -lXxf86vm -lGL -lm

bin_PROGRAMS = cfgpeopsxgl
cfgpeopsxgl_SOURCES = gpucfg/main.c
cfgpeopsxgl_LDADD = $(GTK3_LIBS)

glade_DATA = gpucfg/peopsxgl.ui
gladedir = $(datadir)/psemu/
EXTRA_DIST = $(glade_DATA)
