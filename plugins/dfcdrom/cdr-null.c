/*
 * Cdrom for Psemu Pro like Emulators
 *
 * By: linuzappz <linuzappz@hotmail.com>
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

char *LibName = N_("CDR NULL Plugin");

char *PSEgetLibName(void) {
	return _(LibName);
}

unsigned long PSEgetLibType(void) {
	return PSE_LT_CDR;
}

unsigned long PSEgetLibVersion(void) {
	return 1 << 16;
}

long CDRinit(void) {
	return 0;
}

long CDRshutdown(void) {
	return 0;
}

long CDRopen(void) {
	return 0;
}

long CDRclose(void) {
	return 0;
}

long CDRgetTN(unsigned char *buffer) {
	buffer[0] = 1;
	buffer[1] = 1;
	return 0;
}

long CDRgetTD(unsigned char track, unsigned char *buffer) {
	memset(buffer + 1, 0, 3);
	return 0;
}

long CDRreadTrack(unsigned char *time) {
	return -1;
}

unsigned char *CDRgetBuffer(void) {
	return NULL;
}

long CDRplay(unsigned char *sector) {
	return 0;
}

long CDRstop(void) {
	return 0;
}

long CDRconfigure() {
	return 0;
}

void CDRabout() {
}
