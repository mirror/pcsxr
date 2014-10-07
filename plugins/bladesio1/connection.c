/***************************************************************************
 *   Copyright (C) 2013 by Blade_Arma <edgbla@yandex.ru>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined _WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "typedefs.h"
#include "psemu_plugin_defs.h"

#include "sio1.h"
#include "connection.h"

/***************************************************************************/

static int serversock = -1;
static int clientsock = -1;
static struct sockaddr_in address;
static struct hostent *hostinfo;

/***************************************************************************/

int connectionOpen() {
#if defined _WINDOWS
	WSADATA wsaData;
	if(WSAStartup(0x202, &wsaData))
		fprintf(stderr, "[SIO1] ERROR: WSAStartup()\n");
#endif

	switch(settings.player) {
		case PLAYER_MASTER: {
			int reuse_addr = 1;
			int one = 1;

			serversock = socket(AF_INET, SOCK_STREAM, 0);
			if(serversock == -1) {
				fprintf(stderr, "[SIO1] ERROR: server socket()\n");
				return -1;
			}
			
			setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_addr, sizeof(reuse_addr));
			setsockopt(serversock, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));

			memset(&address, 0, sizeof(address));
			address.sin_family		= AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port		= settings.port;

			if(bind(serversock,(struct sockaddr*)&address,sizeof(address)) == -1) {
				fprintf(stderr, "[SIO1] ERROR: server bind()\n");
				return -1;
			}

			if(listen(serversock, 1) != 0) {
				fprintf(stderr, "[SIO1] ERROR: server listen()\n");
				return -1;
			}

			clientsock = -1;
			while(clientsock < 0)
				clientsock = accept(serversock, NULL, NULL);
		}
		break;
		case PLAYER_SLAVE: {
			int one = 1;

			memset(&address, 0, sizeof(address));
			hostinfo = gethostbyname(settings.ip);
			address.sin_family = AF_INET;
			address.sin_addr   = *((struct in_addr*)hostinfo->h_addr);
			address.sin_port   = settings.port;

			clientsock = socket(AF_INET, SOCK_STREAM, 0);
			if(clientsock == -1) {
				fprintf(stderr, "[SIO1] ERROR: client socket()\n");
				return -1;
			}

			setsockopt(clientsock, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));

			if(connect(clientsock,(struct sockaddr*)&address,sizeof(address)) != 0) {
				fprintf(stderr, "[SIO1] ERROR: client connect(%s)\n", settings.ip);
				return -1;
			}
		}
		break;
	}

	return 0;
}

void connectionClose() {
	if(clientsock >= 0) {
		//close(clientsock);
		clientsock = -1;
	}

	if(serversock >= 0) {
		//close(serversock);
		serversock = -1;
	}

#if defined _WINDOWS
	WSACleanup();
#endif
}

/***************************************************************************/

int connectionSend(u8 *pdata, s32 size) {
	int bytes = 0;

	if(clientsock >= 0)
		if((bytes = (int)send(clientsock, (const char*)pdata, size, 0)) < 0)
			return 0;

	return bytes;
}

int connectionRecv(u8 *pdata, s32 size) {
	int bytes = 0;

	if(clientsock >= 0)
		if((bytes = (int)recv(clientsock, (char*)pdata, size, 0)) < 0)
			return 0;

	return bytes;
}

/***************************************************************************/
