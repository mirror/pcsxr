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

char *PSEgetLibName(void) {
	return _(LibName);
}

unsigned long PSEgetLibType(void) {
	return PSE_LT_CDR;
}

unsigned long PSEgetLibVersion(void) {
	return 1 << 16;
}
