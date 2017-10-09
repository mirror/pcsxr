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
	GtkResources.c	\
	GtkResources.h

# gresource.xml is dynamically generated in CMake. Mirror that here in a static way.
#define gresource_xml =
define gresource_xml
<?xml version="1.0" encoding="UTF-8"?>
<gresources>
  <gresource prefix="/org/pcsxr/gui">
    <file>pcsxr.ui</file>
    <file>pixmaps/pcsxr-about.png</file>
    <file>pixmaps/pcsxr-cd-open.png</file>
    <file>pixmaps/pcsxr-cheats-browse.png</file>
    <file>pixmaps/pcsxr-cheats-search.png</file>
    <file>pixmaps/pcsxr-cheats.png</file>
    <file>pixmaps/pcsxr-cpu.png</file>
    <file>pixmaps/pcsxr-exit.png</file>
    <file>pixmaps/pcsxr-icon.png</file>
    <file>pixmaps/pcsxr-iso-open.png</file>
    <file>pixmaps/pcsxr-iso-reopen.png</file>
    <file>pixmaps/pcsxr-load-state.png</file>
    <file>pixmaps/pcsxr-memcard.png</file>
    <file>pixmaps/pcsxr-memory-dump.png</file>
    <file>pixmaps/pcsxr-play.png</file>
    <file>pixmaps/pcsxr-plugin-cdr.png</file>
    <file>pixmaps/pcsxr-plugin-gpu.png</file>
    <file>pixmaps/pcsxr-plugin-netplay.png</file>
    <file>pixmaps/pcsxr-plugin-pad.png</file>
    <file>pixmaps/pcsxr-plugin-sio1.png</file>
    <file>pixmaps/pcsxr-plugin-spu.png</file>
    <file>pixmaps/pcsxr-reset.png</file>
    <file>pixmaps/pcsxr-run-bios.png</file>
    <file>pixmaps/pcsxr-run-exe.png</file>
    <file>pixmaps/pcsxr-save-state.png</file>
    <file>pixmaps/pcsxr-settings.png</file>
    <file>pixmaps/pcsxr-shutdown.png</file>
    <file>pixmaps/pcsxr.png</file>
  </gresource>
</gresources>
endef
# FIXME
export gresource_xml

pcsxr.gresource.xml:
#	$(file >$@,$(gresource_xml))
	@echo "$$gresource_xml" >$@
pcsxr_gresource: pcsxr.gresource.xml
	$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/data --generate-dependencies $(srcdir)/$<)
GtkResources.c: pcsxr.gresource.xml $(pcsxr_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/data --generate-source --c-name pcsxr $<
GtkResources.h: pcsxr.gresource.xml $(pcsxr_gresource)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/data --generate-header --c-name pcsxr $<

pcsxr_LDADD = \
	../libpcsxcore/libpcsxcore.a \
	$(GTK3_LIBS) -lpthread -lz -lm -lXext -lXtst

CLEANFILES = pcsxr.gresource.xml GtkResources.c GtkResources.h
