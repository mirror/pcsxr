#!/bin/sh
xgettext --keyword=_ --keyword=N_ \
	../win32/gui/*.c ../gui/*.c ../gui/*.h ../libpcsxcore/*.c \
	../plugins/dfxvideo/*.c ../plugins/dfxvideo/gpucfg-0.1df/*.glade2 \
	../data/*.glade2 ../plugins/dfsound/*.c \
	../plugins/dfsound/spucfg-0.1df/*.glade2 \
	../plugins/dfcdrom/*.c ../plugins/dfcdrom/cdrcfg-0.1df/*.c \
	../plugins/dfcdrom/cdrcfg-0.1df/*.glade2 \
	../plugins/peopsxgl/*.c ../plugins/peopsxgl/gpucfg/*.c \
	../plugins/dfinput/*.c ../plugins/dfinput/*.glade2 \
	../plugins/dfnet/*.c ../plugins/dfnet/*.h ../plugins/dfnet/*.glade2 \
	../plugins/bladesio1/*.c ../plugins/bladesio1/*.glade2 \
	--output=pcsx.pot

msgmerge -U --backup=none pt_BR.po pcsx.pot
msgmerge -U --backup=none zh_CN.po pcsx.pot
msgmerge -U --backup=none zh_TW.po pcsx.pot
msgmerge -U --backup=none ru_RU.po pcsx.pot
msgmerge -U --backup=none it.po pcsx.pot
