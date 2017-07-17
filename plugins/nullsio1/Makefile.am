bindir = @libdir@/games/psemu/
libdir = @libdir@/games/psemu/

lib_LTLIBRARIES = libNullSio1.la

libNullSio1_la_SOURCES = sio1.c typedefs.h
libNullSio1_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = -DDATADIR=\"${datadir}/psemu/\" \
	-I$(top_srcdir)/libpcsxcore -I../../include
