/*
 * Cdrom for Psemu Pro like Emulators
 *
 * By: linuzappz <linuzappz@hotmail.com>
 *
 */

#if defined (__linux__)
#include "cdr-linux.c"
#else
#include "cdr-null.c"
#endif
