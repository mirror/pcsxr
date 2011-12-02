//
// DF Netplay Plugin
//
// Based on netSock 0.2 by linuzappz.
// The Plugin is free source code.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dfnet.h"

#define CFG_FILENAME "dfnet.cfg"

void AboutDlgProc();
void ConfDlgProc();
void ReadConfig();

void NETabout() {
	AboutDlgProc();
}

long NETconfigure() {
	ConfDlgProc();
	
	return 0;
}

void SaveConf() {
	FILE *f;

	f = fopen(CFG_FILENAME, "w");
	if (f == NULL) return;
	fwrite(&conf, 1, sizeof(conf), f);
	fclose(f);
}

void LoadConf() {
	ReadConfig();
}
