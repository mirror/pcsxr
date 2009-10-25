AC_INIT(pcsx, 1.9)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(pcsx, 1.9)
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([include/config.h:include/config.h.in])

AC_PROG_CC
AC_PROG_RANLIB
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_STDC_HEADERS
AM_PROG_AS

AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=pcsx
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [gettext domain])

PKG_CHECK_MODULES(GLIB2, glib-2.0, [], AC_MSG_ERROR([*** glib2 not found!]))
PKG_CHECK_MODULES(GTK2, gtk+-2.0, [], AC_MSG_ERROR([*** libgtk2 not found!]))
PKG_CHECK_MODULES(GLADE2, libglade-2.0, [], AC_MSG_ERROR([*** libglade2 not found!]))

AM_PATH_SDL(1.2.0, :, AC_MSG_ERROR([*** SDL not found!]))

AC_SUBST(GLIB2_CFLAGS)
AC_SUBST(GLIB2_LIBS)
AC_SUBST(GTK2_CFLAGS)
AC_SUBST(GTK2_LIBS)
AC_SUBST(GLADE2_CFLAGS)
AC_SUBST(GLADE2_LIBS)

AC_CONFIG_FILES([Makefile data/Makefile doc/Makefile libpcsxcore/Makefile gui/Makefile plugins/dfinput/Makefile plugins/dfsound/Makefile plugins/dfxvideo/Makefile plugins/dfcdrom/Makefile pixmaps/Makefile po/Makefile.in])

dnl Check for -fno-dse option support
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fno-dse"
AC_CACHE_CHECK([for -fno-dse option support], ac_cv_c_no_dse_support,
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]], [[]])],[ac_cv_c_no_dse_support=yes],[ac_cv_c_no_dse_support=no]))
CFLAGS="$saved_CFLAGS"
if test "$ac_cv_c_no_dse_support" = "yes"
then
  CFLAGS="$CFLAGS -fno-dse"
fi

AM_CONDITIONAL(HAVE_OSS, false)

dnl Check for ALSA 0.9.x

AC_ARG_ENABLE(alsa, [  --enable-alsa           use ALSA sound output (default=no)],
[ BUILD_ALSA="$enableval" ],[ BUILD_ALSA="no" ])

if test "$BUILD_ALSA" = "yes"; then
	AC_CHECK_LIB(asound, snd_pcm_open, have_alsa=yes, have_alsa=no)
	AC_CHECK_HEADER(alsa/asoundlib.h, , have_alsa=no)
else
	AC_CHECK_HEADER(sys/soundcard.h, have_oss=yes, have_oss=no)
	if test "x$have_oss" = xno; then
		AC_MSG_WARN([sys/soundcard.h not found. Using NULL sound output])
	else
		AM_CONDITIONAL(HAVE_OSS, true)
	fi
fi

if test "x$have_alsa" = xyes; then
	ALSA_LIBS="-lasound"
else
	ALSA_LIBS=""
fi

AM_CONDITIONAL(HAVE_ALSA,test "x$have_alsa" = xyes)
AC_SUBST(ALSA_LIBS)

AC_CHECK_HEADER(zlib.h, have_zlib=yes, have_zlib=no)
if test "x$have_zlib" = xno; then
	AC_MSG_ERROR([unable to find libz headers])
fi
AC_CHECK_HEADER(bzlib.h, have_bzlib=yes, have_bzlib=no)
if test "x$have_bzlib" = xno; then
	AC_MSG_ERROR([unable to find libbz2 headers])
fi
AC_CHECK_HEADER(X11/extensions/Xv.h, have_xv=yes, have_xv=no)
if test "x$have_xv" = xno; then
	AC_MSG_ERROR([unable to find xv headers])
fi
AC_CHECK_HEADER(X11/extensions/XTest.h, have_xtest=yes, have_xtest=no)
if test "x$have_xtest" = xno; then
	AC_MSG_ERROR([unable to find xtest headers])
fi

AC_ARG_ENABLE(opengl, [  --enable-opengl         build OpenGL plugin (default=no)],
[ BUILD_OPENGL="$enableval" ],[ BUILD_OPENGL="no" ])

PEOPSXGL=""

if test "$BUILD_OPENGL" = "yes"; then
	AC_CHECK_HEADER(GL/gl.h, have_gl=yes, have_gl=no)
	if test "x$have_gl" = xno; then
		AC_MSG_ERROR([unable to find OpenGL headers])
	fi
	AC_CHECK_HEADER(GL/glx.h, have_glx=yes, have_glx=no)
	if test "x$have_glx" = xno; then
		AC_MSG_ERROR([unable to find GLX headers])
	fi
	AC_CHECK_HEADER(X11/extensions/xf86vmode.h, have_vmode=yes, have_vmode=no,
	[[#include <GL/gl.h>
	#include <GL/glx.h>
	]])
	if test "x$have_vmode" = xno; then
		AC_MSG_ERROR([unable to find xf86vmode headers])
	fi
	PEOPSXGL="plugins/peopsxgl"
	AC_SUBST(PEOPSXGL)
	AC_CONFIG_FILES([plugins/peopsxgl/Makefile])
fi

AM_CONDITIONAL(X86_NASM, false)

if expr x"$target_cpu" : 'xi.86' > /dev/null; then
	if expr x"$target_os" : 'x.*linux.*' > /dev/null; then
		AC_PATH_PROG([NASM],[nasm],[missing])
		if test "$NASM" = "missing"; then
			AC_MSG_WARN([unable to find nasm, needed to build dfx11video])
			AM_CONDITIONAL(X86_NASM, false)
		else
			AM_CONDITIONAL(X86_NASM, true)
		fi
	fi
fi

AM_CONDITIONAL(ARCH_X86, false)
AM_CONDITIONAL(ARCH_X86_64, false)
AM_CONDITIONAL(ARCH_PPC, false)

AC_ARG_ENABLE(dynarec, [  --enable-dynarec=...    force selection of dynamic recompiler platform (x86, x86_64, ppc) (default: autodetect)],
[ DYNAREC="$enableval" ],[ DYNAREC="auto" ])

if test "x$DYNAREC" = xauto; then
	DYNARECSEL="auto"
else if test "x$DYNAREC" = xx86; then
	DYNARECSEL="x86"
else if test "x$DYNAREC" = xx86_64; then
	DYNARECSEL="x86_64"
else if test "x$DYNAREC" = xppc; then
	DYNARECSEL="ppc"
else if test "x$DYNAREC" = xno; then
	DYNARECSEL="no"
else
	AC_MSG_WARN([Dynamic Recompiler "$DYNAREC" not found.  Autodetecting...])
	DYNARECSEL="auto"
fi
fi
fi
fi
fi

if test "x$DYNARECSEL" = xauto; then
	if expr x"$target_cpu" : 'xi.86' > /dev/null; then
		DYNARECSEL="x86"
	fi

	if expr x"$target_cpu" : 'xx86_64' > /dev/null; then
		DYNARECSEL="x86_64"
	fi

	if expr x"$target_cpu" : 'xpowerpc' > /dev/null; then
		DYNARECSEL="ppc"
	fi
fi

if test "x$DYNARECSEL" = xno; then
        AC_DEFINE([NOPSXREC], [1], [Define if we are compiling without dynamic recompiler.])
fi

if test "x$DYNARECSEL" = xx86; then
        AC_DEFINE([__i386__], [1], [Define if we are compiling for x86 architectures.])
        AM_CONDITIONAL(ARCH_X86, true)

	AC_MSG_RESULT([Dynamic Recompiler selected: x86])
fi

if test "x$DYNARECSEL" = xx86_64; then
        AC_DEFINE([__x86_64__], [1], [Define if we are compiling for x86_64 architectures.])
        AM_CONDITIONAL(ARCH_X86_64, true)
	dnl CFLAGS+=" -m64 "
	dnl AC_COMPILE_IFELSE(AC_LANG_PROGRAM,,AC_MSG_ERROR([Cannot compile with -m64])) 
	AC_MSG_RESULT([Dynamic Recompiler selected: x86_64])
fi

if test "x$DYNARECSEL" = xppc; then
        AC_DEFINE([__ppc__], [1], [Define if we are compiling for powerpc architectures.])
        AM_CONDITIONAL(ARCH_PPC, true)
	AC_MSG_RESULT([Dynamic Recompiler selected: ppc])
fi

AC_C_BIGENDIAN(AC_DEFINE([__BIGENDIAN__],[],[define on a big endian system]))

AC_DEFINE([__LINUX__], [1], [Define if building on a GNU/Linux system.])
AC_DEFINE([MAXPATHLEN], [4096], [Define to the maximum length of any path.])

AC_OUTPUT
