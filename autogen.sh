#!/bin/sh
# simple script to redo the autotooling

cp /usr/share/libtool/config/ltmain.sh .
autoreconf && autoheader && aclocal && autoconf && automake --add-missing --copy
cp /usr/share/gettext/config.rpath .
cp /usr/share/glib-2.0/gettext/mkinstalldirs .
rm -r autom4te.cache
