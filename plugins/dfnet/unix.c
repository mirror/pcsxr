//
// DF Netplay Plugin
//
// Based on netSock 0.2 by linuzappz.
// The Plugin is free source code.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
extern int errno;

#include "dfnet.h"

int ExecCfg(const char *arg) {
	char cfg[512];
	struct stat buf;

	strcpy(cfg, "cfg/cfgDFNet");
	if (stat(cfg, &buf) != -1) {
		strcat(cfg, " ");
		strcat(cfg, arg);
		return system(cfg);
	}

	printf("cfgDFNet file not found!\n");
	return -1;
}

void SysMessage(const char *fmt, ...) {
	va_list list;
	char msg[512];
	char cmd[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	sprintf(cmd, "message %s\n", msg);
	ExecCfg(cmd);
}

long sockInit() {
	conf.PlayerNum = 0;
	tm.tv_sec = 0;
	tm.tv_usec = 0;

	return 0;
}

long sockShutdown() {
	return 0;
}

long sockOpen() {
	if (ExecCfg("open") == 0) return -1;

	LoadConf();

	return 0;
}

int sockPing() {
	char data[32];
	struct timeval tv, tvn;

	memset(data, 0, sizeof(data));

	gettimeofday(&tv, NULL);
	SEND(data, 32, PSE_NET_BLOCKING);
	RECV(data, 32, PSE_NET_BLOCKING);
	gettimeofday(&tvn, NULL);

	return (tvn.tv_sec - tv.tv_sec) * 1000 +
		   (tvn.tv_usec - tv.tv_usec) / 1000;
}

void CALLBACK NETconfigure() {
	ExecCfg("configure");
}

void CALLBACK NETabout() {
	ExecCfg("about");
}

pid_t cfgpid = 0;

void OnWaitDlg_Abort(int num) {
	WaitCancel = 1;
	cfgpid = 0;
}

void sockCreateWaitDlg() {
	signal(SIGUSR2, OnWaitDlg_Abort);
	if ((cfgpid = fork()) == 0) {
		execl("cfg/cfgDFNet", "cfgDFNet", "wait", NULL);
		exit(0);
	}
	usleep(100000);
}

void sockDlgUpdate() {
	usleep(100000);
}

void sockDestroyWaitDlg() {
	if (cfgpid > 0) {
		kill(cfgpid, SIGKILL);
		cfgpid = 0;
	}
}

long timeGetTime() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}
