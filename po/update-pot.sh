#!/bin/sh
xgettext --keyword=_ --keyword=N_ ../win32/gui/*.c ../gui/*.c ../gui/*.h ../libpcsxcore/*.c ../plugins/dfxvideo/gpucfg-0.1df/*.glade2 ../data/*.glade2 ../plugins/dfsound/spucfg-0.1df/*.glade2 ../plugins/dfcdrom/*.c ../plugins/dfinput/*.c ../plugins/dfinput/*.glade2 --output=pcsx.pot
