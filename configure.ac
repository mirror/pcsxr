AC_INIT([pcsxr], [1.9.95])

AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AM_INIT_AUTOMAKE(foreign)
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([include/config.h:include/config.h.in])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_STDC_HEADERS
AM_PROG_AS

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=pcsxr
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [gettext domain])

PKG_CHECK_MODULES(GLIB2, glib-2.0 >= 2.20, [], AC_MSG_ERROR([*** glib2 >= 2.20 not found!]))
PKG_CHECK_MODULES(GTK3, gtk+-3.0 , [], AC_MSG_ERROR([*** libgtk3 >= 3.0 not found!]))

PKG_CHECK_MODULES([SDL2], [sdl2 >= 2.0.0],
   [AC_DEFINE([HAVE_SDL2], [1], [Use SDL2])],
   [PKG_CHECK_MODULES([SDL], [sdl >= 1.2.12],
       [AC_DEFINE([HAVE_SDL], [1], [Use SDL])
    ], AC_MSG_ERROR([*** SDL >= 1.2.12 not found!]))
])

AC_SUBST(GLIB2_CFLAGS)
AC_SUBST(GLIB2_LIBS)
AC_SUBST(GTK3_CFLAGS)
AC_SUBST(GTK3_LIBS)

AC_CONFIG_FILES([Makefile
data/Makefile
doc/Makefile
libpcsxcore/Makefile
gui/Makefile
plugins/dfinput/Makefile
plugins/dfsound/Makefile
plugins/dfxvideo/Makefile
plugins/dfcdrom/Makefile
plugins/dfnet/Makefile
pixmaps/Makefile
po/Makefile.in])

AC_CHECK_LIB(dl, dlsym, [LIBS="$LIBS -ldl"], [])
AC_CHECK_LIB(socket, socket, [LIBS="$LIBS -lsocket"], [])
AC_CHECK_LIB(nsl, gethostbyname, [LIBS="$LIBS -lnsl"], [])
AC_CHECK_LIB(umem, umem_alloc, [LIBS="$LIBS -lumem"], [])

AM_CONDITIONAL(SOUND_OSS, false)
AM_CONDITIONAL(SOUND_SDL, false)
AM_CONDITIONAL(SOUND_OPENAL, false)
AM_CONDITIONAL(SOUND_NULL, false)

dnl Check for ALSA 1.x, OSS, OpenAL or PulseAudio

AC_ARG_ENABLE(sound, [  --enable-sound=...      force selection of sound backend (alsa/null/oss/pulseaudio/sdl/openal) (default: sdl)],
[ SOUND="$enableval" ],[ SOUND="sdl" ])

if test "x$SOUND" = xalsa; then
	PKG_CHECK_MODULES(ALSA, alsa >= 1.0.0, have_alsa=yes, have_alsa=no)
elif test "x$SOUND" = xpulseaudio; then
	PKG_CHECK_MODULES(PULSEAUDIO, libpulse >= 0.9.16, have_pulseaudio=yes)
elif test "x$SOUND" = xoss; then
	AC_CHECK_HEADER(sys/soundcard.h, have_oss=yes, have_oss=no)
	if test "x$have_oss" = xno; then
		AC_MSG_ERROR([sys/soundcard.h not found!])
	else
		AM_CONDITIONAL(SOUND_OSS, true)
	fi
elif test "x$SOUND" = xopenal; then
	PKG_CHECK_MODULES(OPENAL, openal >= 1.0.0, have_openal=yes)
elif test "x$SOUND" = xno; then
	AM_CONDITIONAL(SOUND_NULL, true)
elif test "x$SOUND" = xnull; then
	AM_CONDITIONAL(SOUND_NULL, true)
else
	AM_CONDITIONAL(SOUND_SDL, true)
fi

AM_CONDITIONAL(SOUND_ALSA, test "x$have_alsa" = xyes)
AC_SUBST(ALSA_LIBS)
AM_CONDITIONAL(SOUND_OPENAL, test "x$have_openal" = xyes)
AC_SUBST(OPENAL_LIBS)
AM_CONDITIONAL(SOUND_PULSEAUDIO, test "x$have_pulseaudio" = xyes)
AC_SUBST(PULSEAUDIO_CFLAGS)
AC_SUBST(PULSEAUDIO_LIBS)

AC_CHECK_HEADER(zlib.h, have_zlib=yes, have_zlib=no)
if test "x$have_zlib" = xno; then
	AC_MSG_ERROR([unable to find libz headers])
fi
AC_CHECK_HEADER(X11/extensions/Xv.h, have_xv=yes, have_xv=no)
if test "x$have_xv" = xno; then
	AC_MSG_ERROR([unable to find xv headers])
fi
AC_CHECK_HEADER(X11/extensions/XTest.h, have_xtest=yes, have_xtest=no)
if test "x$have_xtest" = xno; then
	AC_MSG_ERROR([unable to find xtest headers])
fi

AM_CONDITIONAL(USE_LIBCDIO, false)

AC_ARG_ENABLE(libcdio, [  --enable-libcdio        use GNU libcdio for CD-ROM support (default=no)],
[ BUILD_LIBCDIO="$enableval" ],[ BUILD_LIBCDIO="no" ])

if test "$BUILD_LIBCDIO" = "yes"; then
	PKG_CHECK_MODULES(LIBCDIO, libcdio, [], AC_MSG_ERROR([*** libcdio not found!]))
	AM_CONDITIONAL(USE_LIBCDIO, true)
fi

AC_SUBST(LIBCDIO_CFLAGS)
AC_SUBST(LIBCDIO_LIBS)

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

AC_ARG_ENABLE(sio1, [  --enable-sio1         build SIO1 plugin (default=yes)],
[ BUILD_SIO1="$enableval" ],[ BUILD_SIO1="yes" ])

BLADESIO1=""

if test "$BUILD_SIO1" = "yes"; then
	AC_DEFINE([ENABLE_SIO1API], [1], [Define if we want use sio interface.])
	BLADESIO1="plugins/bladesio1"
	AC_SUBST(BLADESIO1)
	AC_CONFIG_FILES([plugins/bladesio1/Makefile])
fi

#####apt-get install libavcodec-dev libavformat-dev libavutil-dev
AC_ARG_ENABLE(ccdda, [  --enable-ccdda         support for compressed CDDA formats via ffmpeg (default=no)],
[ BUILD_CCDDA="$enableval" ],[ BUILD_CCDDA="no" ])

if test "$BUILD_CCDDA" = "yes"; then
	AC_DEFINE([ENABLE_CCDDA], [1], [Defined when compressed cdda support wanted.])
	#AC_CHECK_HEADER([libavutil/mathematics.h],, AC_MSG_ERROR("No ffmpeg headers libavcodec/avcodec.h libavutil/mathematics.h"))
	#AC_CHECK_HEADER([libavcodec/avcodec.h],, AC_MSG_ERROR("No ffmpeg headers libavcodec/avcodec.h libavutil/mathematics.h"))
	AC_CHECK_HEADERS([libavcodec/avcodec.h libavutil/mathematics.h libavformat/avformat.h],,AC_MSG_ERROR("No ffmpeg headers libavcodec/avcodec.h libavformat/avformat.h libavutil/mathematics.h"))
	#AC_CHECK_LIB([avcodec], [main], [LIBS="$LIBS -l:libavcodec.so.52 "], AC_MSG_ERROR("No avcodec library"))
	#AC_CHECK_LIB(avutil, [main], [LIBS="$LIBS -l:libavutil.so.52 "], AC_MSG_ERROR("No avutil library"))
	#AC_CHECK_LIB(avformat, [main], [LIBS="$LIBS -l:libavformat.so.52 "], AC_MSG_ERROR("No avformat library"))
	AC_CHECK_LIB([avcodec], [main], [LIBS="$LIBS -lavcodec"], AC_MSG_ERROR("No avcodec library"))
	AC_CHECK_LIB([avutil], [main], [LIBS="$LIBS -lavutil"], AC_MSG_ERROR("No avutil library"))
	AC_CHECK_LIB([avformat], [main], [LIBS="$LIBS -lavformat"], AC_MSG_ERROR("No avformat library"))
	#AC_CHECK_LIB(avcodec  ,[main],AC_DEFINE([HAVE_LIBAVCODEC],   [1], [ ]),AC_MSG_ERROR([$errormsgl]))
	#AC_CHECK_LIB(swscale  ,[main],AC_DEFINE([HAVE_LIBSWSCALE],   [1], [ ]),AC_MSG_ERROR([$errormsgl]), [-lavutil])
	#AC_CHECK_LIB(avdevice ,[main],AC_DEFINE([HAVE_LIBAVDEVICE],  [1], [ ]),AC_MSG_ERROR([$errormsgl]), [-lavcodec -lavutil -lavformat])
fi

AM_CONDITIONAL(X86_NASM, false)

if expr x"$build_cpu" : 'xi.86' > /dev/null; then
	if expr x"$build_os" : 'x.*linux.*' > /dev/null; then
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

AC_ARG_ENABLE(dynarec, [  --enable-dynarec=...    force selection of dynamic recompiler platform (auto/no/x86/x86_64/ppc) (default: auto)],
[ DYNAREC="$enableval" ],[ DYNAREC="auto" ])

if test "x$DYNAREC" = xauto; then
	DYNARECSEL="auto"
elif test "x$DYNAREC" = xx86; then
	DYNARECSEL="x86"
elif test "x$DYNAREC" = xx86_64; then
	DYNARECSEL="x86_64"
elif test "x$DYNAREC" = xppc; then
	DYNARECSEL="ppc"
elif test "x$DYNAREC" = xno; then
	DYNARECSEL="no"
else
	AC_MSG_WARN([Dynamic Recompiler "$DYNAREC" not found.  Autodetecting...])
	DYNARECSEL="auto"
fi

if test "x$DYNARECSEL" = xauto; then
	if expr x"$build_cpu" : 'xi.86' > /dev/null; then
		DYNARECSEL="x86"
	elif expr x"$build_cpu" : 'xx86_64' > /dev/null; then
		DYNARECSEL="x86_64"
	elif expr x"$build_cpu" : 'xpowerpc' > /dev/null; then
		DYNARECSEL="ppc"
	fi
fi

if test "x$DYNARECSEL" = xno; then
		AC_DEFINE([NOPSXREC], [1], [Define if we are compiling without dynamic recompiler.])
elif test "x$DYNARECSEL" = xx86; then
	AC_DEFINE([__i386__], [1], [Define if we are compiling for x86 architectures.])
	AM_CONDITIONAL(ARCH_X86, true)

	AC_MSG_RESULT([Dynamic Recompiler selected: x86])
elif test "x$DYNARECSEL" = xx86_64; then
	AC_DEFINE([__x86_64__], [1], [Define if we are compiling for x86_64 architectures.])
	AM_CONDITIONAL(ARCH_X86_64, true)
	dnl CFLAGS+=" -m64 "
	dnl AC_COMPILE_IFELSE(AC_LANG_PROGRAM,,AC_MSG_ERROR([Cannot compile with -m64])) 
	AC_MSG_RESULT([Dynamic Recompiler selected: x86_64])
elif test "x$DYNARECSEL" = xppc; then
	AC_DEFINE([__ppc__], [1], [Define if we are compiling for powerpc architectures.])
	AM_CONDITIONAL(ARCH_PPC, true)
	AC_MSG_RESULT([Dynamic Recompiler selected: ppc])
fi

AC_C_BIGENDIAN(AC_DEFINE([__BIGENDIAN__],[],[define on a big endian system]))

AC_DEFINE([__LINUX__], [1], [Define if building on a GNU/Linux system.])
AC_DEFINE([MAXPATHLEN], [4096], [Define to the maximum length of any path.])

AC_OUTPUT
