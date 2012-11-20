AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"${datadir}/pcsxr/\" \
	-DPIXMAPDIR=\"${datadir}/pixmaps/\" \
	-DLOCALE_DIR=\"${datadir}/locale/\" \
	$(GTK3_CFLAGS) \
	-I$(top_srcdir)/libpcsxcore -I$(top_srcdir)/include \
	-DPSEMU_DATA_DIR=\"${datadir}/psemu\" \
	-DDEF_PLUGIN_DIR=\"${libdir}/games/psemu\"

bin_PROGRAMS = pcsxr

pcsxr_SOURCES = \
	LnxMain.c	\
	Linux.h		\
	nopic.h		\
	Plugin.c	\
	Config.c	\
	GtkGui.c	\
	MemcardDlg.c	\
	MemcardDlg.h	\
	ConfDlg.c	\
	ConfDlg.h	\
	Cheat.c		\
	Cheat.h		\
	DebugMemory.c	\
	DebugMemory.h	\
	AboutDlg.c	\
	AboutDlg.h	\
	gdk-compat.h

pcsxr_LDADD = \
	../libpcsxcore/libpcsxcore.a \
	$(GTK3_LIBS) -lpthread -lz -lm -lXext -lXtst
