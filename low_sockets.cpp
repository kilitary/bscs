#include "stdafx.h"

int ReadyToRead(SOCKET Socket,WORD timeout)
{
	fd_set          fds;
	struct timeval  tm;
	int dwResult;
	
	memset(&tm, 0x0,sizeof(tm));
	tm.tv_sec = timeout;
	FD_ZERO(&fds);
	FD_SET(Socket, &fds);
	
	if ( (dwResult = select(Socket + 1, &fds, NULL, NULL, &tm)) > 0) 	{
		if(FD_ISSET(Socket,&fds))
			return SUCCESS;
	}

	if(dwResult == SOCKET_ERROR)
		return SOCKET_ERROR;
	
	return ERR;
}

DWORD resolve(char* Host) 
{
	DWORD addr;
	struct hostent *he;

	addr = inet_addr(Host);

	if(addr != INADDR_NONE) {
		return addr;
	}

	he = gethostbyname((char FAR*) Host);
	
	if(he == NULL) {
		deb("resolve error: %d",WSAGetLastError());
		return NULL;
	}

	addr = ((struct in_addr *) (he->h_addr_list[0]))->s_addr;

	return addr;
}


char* GetLocalInetAddr(void)
{
	static char name[255];
	struct hostent *hostinfo;
	WSADATA wsaData;

	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

	if (iResult != NO_ERROR) {
		deb("localaddr: Error at WSAStartup()\n");
	}

	if( gethostname ( (char FAR*)name, 255) == 0) {
        if((hostinfo = gethostbyname(name)) != NULL) {
            return (inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list));
        }
		deb("localaddr: gethostbyname failed, %s",FORMATERROR );
	}
	deb("localaddr: gethostname failed, %s",FORMATERROR );

	return NULL;
}

int sockets_bio(SOCKET first, SOCKET two)
{
	FD_SET fds;
	int a_res, s_res, r_res;
	char bio_buffer[BIO_BUF_SIZE];
	int size_bio_buffer = BIO_BUF_SIZE;
	struct timeval tm;

	deb("socket_bio: first %x two %x", first, two);

	memset(&tm, 0x0,sizeof(tm));
	tm.tv_sec = BIO_SOCKET_TIMEOUT;

	while(1) {
		FD_ZERO(&fds);
		FD_SET(first, &fds);
		FD_SET(two, &fds);

		a_res = select(max(first, two), &fds, NULL, NULL, &tm);
		if(a_res == SOCKET_ERROR) {
			closesocket(first);
			closesocket(two);	
			ExitThread(SUCCESS);
		} 
		
		if(FD_ISSET(first,&fds)) {
			s_res = recv(first, bio_buffer, size_bio_buffer - 1,0);
			if(s_res == SOCKET_ERROR || s_res == 0) {
				closesocket(first);
				closesocket(two);
				ExitThread(SUCCESS);
			}
			r_res = send(two, bio_buffer, s_res, 0);
			if(r_res == SOCKET_ERROR) {
				closesocket(first);
				closesocket(two);
				ExitThread(SUCCESS);
			}
		} else if(FD_ISSET(two,&fds)) {
			r_res = recv(two, bio_buffer, size_bio_buffer - 1,0);
			if(r_res == SOCKET_ERROR || r_res == 0) {
				closesocket(first);
				closesocket(two);
				ExitThread(SUCCESS);
			}
			s_res = send(first, bio_buffer, r_res, 0);
			if(s_res == SOCKET_ERROR) {
				closesocket(first);
				closesocket(two);
				ExitThread(SUCCESS);
			}
		}
	}
}