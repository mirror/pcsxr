AM_CPPFLAGS = -DLOCALE_DIR=\"${datadir}/locale/\" \
	-I$(top_srcdir)/include

noinst_LIBRARIES = libpcsxcore.a

libpcsxcore_a_SOURCES = \
	psxbios.c	\
	cdrom.c	\
	psxcounters.c	\
	psxdma.c	\
	disr3000a.c	\
	gpu.c	\
	gpu.h	\
	spu.c	\
	sio.c	\
	psxhw.c	\
	mdec.c	\
	psxmem.c	\
	misc.c	\
	plugins.c	\
	decode_xa.c	\
	r3000a.c	\
	psxinterpreter.c	\
	gte.c	\
	psxhle.c	\
	cdrom.h \
	coff.h \
	debug.c \
	debug.h \
	decode_xa.h \
	ecm.h \
	gte.h \
	mdec.h \
	misc.h \
	plugins.h \
	psemu_plugin_defs.h \
	psxbios.h \
	psxcommon.c \
	psxcommon.h \
	psxcounters.h \
	psxdma.h \
	psxhle.h \
	psxhw.h \
	psxmem.h \
	r3000a.h \
	sio.h \
	sjisfont.h \
	spu.h \
	system.h \
	cdriso.c \
	cdriso.h \
	cheat.c \
	cheat.h \
	socket.c \
	socket.h \
	ppf.c \
	ppf.h

if ARCH_X86_64
libpcsxcore_a_SOURCES += \
	ix86_64/iGte.h		\
	ix86_64/iR3000A-64.c	\
	ix86_64/ix86-64.c	\
	ix86_64/ix86-64.h	\
	ix86_64/ix86_cpudetect.c	\
	ix86_64/ix86_fpu.c	\
	ix86_64/ix86_3dnow.c	\
	ix86_64/ix86_mmx.c	\
	ix86_64/ix86_sse.c
else		
if ARCH_X86
libpcsxcore_a_SOURCES += \
	ix86/iGte.h	\
	ix86/iR3000A.c	\
	ix86/ix86.c	\
	ix86/ix86.h
endif
endif

if ARCH_PPC
libpcsxcore_a_SOURCES += \
	ppc/pGte.h	\
	ppc/pR3000A.c	\
	ppc/ppc.c	\
	ppc/ppc.h	\
	ppc/ppc_mnemonics.h	\
	ppc/reguse.c	\
	ppc/reguse.h
libpcsxcore_a_CCASFLAGS = -x assembler-with-cpp -mregnames -D__POWERPC__
endif
