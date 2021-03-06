// PortScanner.cpp: 定義主控台應用程式的進入點。
//

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") //To link the winsock library  

int allisdigit(char* str) {
	int i;

	if (str) {
		for (i = 0; str[i]; i++) {
			if ((str[i] < '0') || (str[i] > '9')) {
				return 0;
			}
		}
		return 1;
	}

	return 0;
}

int allisipv4(char* str) {
	int i;
	int countdot = 0;
	int countnum = 0;

	if (str) {
		for (i = 0; str[i]; i++) {
			 if (str[i] == '.') {
				 if (countnum == 0) {
					 return 0; // only dot
				 }
				 countdot++;
				 countnum = 0;
			}else if ((str[i] < '0') || (str[i] > '9')) {
				return 0;
			}else{
				countnum++;
			}
		}
		if (countnum == 0) {
			return 0;
		}
		if (countdot != 3) {
			return 0;
		}
		return 1;
	}

	return 0;
}

int OpenPort(char* hostname, int port) {
	WSADATA firstsock;
	SOCKET s;
	struct hostent *host;
	int err, i, startport, endport;
	struct sockaddr_in sa; //this stores the destination address
	int resolved = 0;
	int success = 0;

	strcpy_s((char *)&sa, sizeof sa, "");
	sa.sin_family = AF_INET; //this line must be like this coz internet

								//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 0), &firstsock) == 0) {  //CHECKS FOR WINSOCK VERSION 2.0
		if (allisipv4(hostname)) {
			sa.sin_addr.s_addr = inet_addr(hostname); //get ip into s_addr
			resolved = 1;
		}else if ((host = gethostbyname(hostname)) != 0) {
			strncpy((char *)&sa.sin_addr, (char *)host->h_addr_list[0], sizeof sa.sin_addr);
			resolved = 1;
		}else {
			printf("[-] Error resolving hostname\n");
		}

		if (resolved == 1) {
			s = socket(AF_INET, SOCK_STREAM, 0); //make net a valid socket handle
			if (s >= 0) {

				struct timeval timeout;
				timeout.tv_sec = 0;
				timeout.tv_usec = 500;

				//setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
				//setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

				sa.sin_port = htons(port);
				//connect to the server with that socket
				err = connect(s, (struct sockaddr *)&sa, sizeof sa);

				setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
				setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

				if (err != SOCKET_ERROR) {
					shutdown(s, SD_BOTH);
					success = 1;
				}
				closesocket(s);   //closes the net socket 
			}
		}

		WSACleanup();
	}

	return success;
}

#pragma warning(disable: 4700)
int OpenPort2(char* hostname, int port)
{

	WSADATA wsa;
	int resolved = 0;
	struct hostent *host;
	int err = WSAStartup(MAKEWORD(2, 0), &wsa);
	if (err != 0)
	{
		return 1;
	}

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0); //make net a valid socket handle
	if (s == INVALID_SOCKET)
	{
		WSACleanup();
		return 1;
	}

	u_long enabled = 1;
	if (ioctlsocket(s, FIONBIO, &enabled) == SOCKET_ERROR)
	{
		closesocket(s);
		WSACleanup();
		return 1;
	}

	struct sockaddr_in sa = { 0 };
	sa.sin_family = AF_INET;
	if (allisipv4(hostname)) {
		sa.sin_addr.s_addr = inet_addr(hostname); //get ip into s_addr
		resolved = 1;
	}
	else if ((host = gethostbyname(hostname)) != 0) {
		strncpy((char *)&sa.sin_addr, (char *)host->h_addr_list[0], sizeof sa.sin_addr);
		resolved = 1;
	}
	else {
		printf("[-] Error resolving hostname\n");
	}
	sa.sin_port = htons(port);

	if (connect(s, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			closesocket(s);
			WSACleanup();
			return 1;
		}


		fd_set wfd, efd;

		FD_ZERO(&wfd);
		FD_SET(s, &wfd);

		FD_ZERO(&efd);
		FD_SET(s, &efd);

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 500;

		err = select(0, NULL, &wfd, &efd, &timeout);
		if (err == SOCKET_ERROR)
		{
			closesocket(s);
			WSACleanup();
			return 1;
		}

		if (err == 0)
		{
			// connect timeout
			closesocket(s);
			WSACleanup();
			return 0;
		}

		if (FD_ISSET(s, &efd))
		{
			int optlen;
			err = 0;
			optlen = sizeof err;
			getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &optlen);
			closesocket(s);
			WSACleanup();

			switch (err)
			{
			case WSAETIMEDOUT: // connect timeout
			case WSAECONNREFUSED: // port closed
				return 0;
			}

			return 1;
		}
	}

	closesocket(s);
	WSACleanup();
	return 0;
}

int main(int argc, char **argv)
{
	char host[1024]  = "";
	int port = 0;
	int i;
	int ret;
	for (i = 222; i < 223; i++) {
		sprintf_s(host, "192.168.137.%d", i);

		for (port = 1; port < 65536; port++) {
			ret = OpenPort(host, port);
			//ret = !OpenPort2(host, port);
			if (ret) {
				printf("===> %s:%d is open!!\n", host, port);
			}
			else {
				//printf("%s:%d is closed!!\n", host, port);
			}
		}
	}
	return(0);
}
