#include "StdAfx.h"
#include "globals.h"

WORD pri_socksport;
WORD socks_need = 1;

int SocksServer(WORD state,int port)
{
	static DWORD dwSocksThreadId = NULL;
	static HANDLE hSocksThread = NULL; 
	static int tmpPort;
	
	tmpPort = port;
	
	if( state == ENABLE ) {
		socks_need = 1;
		hSocksThread = CreateThread( NULL, 0, CoreSocks, (void*) tmpPort, 0,	&dwSocksThreadId);  
		if (hSocksThread == NULL) {
			deb( "xCreateThread for Socks failed (%d)\n", GetLastError() ); 
			return ERR;
		}
		return dwSocksThreadId;
	} else if( state == DISABLE ) 
	{
		socks_need = 0;
//		CloseHandle(hSocksThread);
		return SUCCESS;
	}
	
	return ERR;
}

DWORD WINAPI CoreSocks( LPVOID port )
{
	WSADATA wsaData;
	DWORD spId = NULL;
	HANDLE hsThread = NULL; 
	SOCKET s_socket;
	DWORD nonBlock = 1;
	int s_port;

	s_port = (int) port;
	
	if(s_port <= 0) 
	{
		deb("erroneous port for socks:%d\n",s_port);
		ExitThread(ERR);
	}
	
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	
	if (iResult != NO_ERROR) 
	{
		deb("Error at WSAStartup()\n");
		ExitThread(ERR);
	}
	
	s_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	
	if ( s_socket == INVALID_SOCKET ) 
	{
		deb( "Error at socket(): %ld\n", WSAGetLastError() );
		deb("socks:socket() failed. err:%d\n",WSAGetLastError());
		ExitThread(ERR);
	}
	
	sockaddr_in service;
	
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons( s_port );
	
	if ( bind( s_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) 
	{
		deb("socks port %d: bind() failed. err:%d\n",ntohs(service.sin_port),WSAGetLastError());
		closesocket(s_socket);
		ExitThread(ERR);
	}
	
	if ( listen( s_socket, SOMAXCONN) == SOCKET_ERROR ) 
	{
		deb( "Error listening on socket.\n");
		deb("socks listen() failed.\n");
		closesocket(s_socket);
		ExitThread(ERR);
	}
	
	SOCKET AcceptSocket;
	
	ioctlsocket( s_socket, FIONBIO, &nonBlock );
	//wsprintf(temp, "%s:%d", GetLocalInetAddr(), s_port);
	//udp_send_msg(SOCKS, temp, strlen(temp));
	
	while (1) 
	{  
		AcceptSocket = INVALID_SOCKET;
		while ( AcceptSocket == INVALID_SOCKET ) 
		{
			AcceptSocket = accept( s_socket, NULL, NULL );
			Sleep(100);
			if(!socks_need) 
				ExitThread(SUCCESS);
		}
		hsThread = CreateThread( NULL, 0, ProcessSocksClient, (void*) AcceptSocket, 0,&spId);
		if (spId == NULL) 
			deb( "xCreateThread for Socks failed: %s\n", FORMATERROR ); 
/*		else
			CloseHandle(hsThread);*/
	}
}

void SendErrorToClient(SOCKET cli_socket, int s_version, int ver, int rep)
{
	struct s5_reply s5rep;
	struct s4_reply s4rep;

	if(s_version == 0x05) 
	{
		s5rep.ver = ver;
		s5rep.rep = rep;
		s5rep.rsv = 0x00;
		s5rep.atyp = 0x00;
		//s5rep.local_addr.s_addr = inet_addr(GetLocalInetAddr());
		s5rep.local_port = htons(666);
		send(cli_socket,(char*)&s5rep,sizeof(s5rep),0);
		closesocket(cli_socket);
	}
	else
	{
		s4rep.vn = 0x00;
		s4rep.cd = 91;
		s4rep.d_port = 0x00;
		send(cli_socket,(char*)&s4rep,sizeof(s4rep),0);
		closesocket(cli_socket);
	}
}

DWORD WINAPI ProcessSocksClient( LPVOID s )
{
	char s_version;
	char methods[2];
	SOCKET cli_socket = (SOCKET) s;
	SOCKET rem_socket;
	struct sockaddr_in peer;
	struct sockaddr_in sin;
	int res,peer_size;
	DWORD nonBlock = 0;

	struct s5_packet sq5;
	struct s5_reply s5rep;
	struct s4_packet sq4;
	struct s4_reply s4rep;

	deb("processing socket=%d\n",cli_socket);
	
	ioctlsocket( cli_socket, FIONBIO, &nonBlock ); /* make it blocking */
	
	/* get socks version */
	if((res = recv(cli_socket,&s_version,1,0)) == SOCKET_ERROR) 
	{
		deb("could not get first version byte from client\n");
		closesocket(cli_socket);
		ExitThread(SUCCESS);
	}
	if(res == 0)
	{
		deb("connection has been reset by remote");
		closesocket(cli_socket);
		ExitThread(SUCCESS);
	}
	
	if(s_version == 0x05) 
	{
		if((res = recv(cli_socket,methods,2,0)) == SOCKET_ERROR) 
		{
			deb("could not get methods from client\n");
			closesocket(cli_socket);
			
			ExitThread(SUCCESS);
		}
		methods[0] = 0x05; /* socks 5 version */
		methods[1] = 0x00; /* no auth */
		if(send(cli_socket,methods,2,0) == SOCKET_ERROR) 
		{
			deb("could not sent auth reply to client\n");
			closesocket(cli_socket);
			ExitThread(SUCCESS);
		}
		if((res = recv(cli_socket,(char*) &sq5,sizeof(sq5),0)) == SOCKET_ERROR) 
		{
			deb("could not get sq5 packet from client error:[%s]\n",strerror(NULL));
			closesocket(cli_socket);
			ExitThread(SUCCESS);
		}
		if(sq5.ver != 0x05) 
		{
			deb("0x%x version mismatch after handshake!\n",sq5.ver);
			SendErrorToClient(cli_socket,s_version,0x05,0x05);
			ExitThread(SUCCESS);
		}
		if(sq5.cmd != 0x01) 
		{
			deb("method 0x%x not supported yet.\n",sq5.cmd);
			SendErrorToClient(cli_socket,s_version,0x05,0x07);
			ExitThread(SUCCESS);
		}
		if(sq5.atyp != 0x01) 
		{
			deb("atyp 0x%x other than 0x01\n",sq5.atyp);
			SendErrorToClient(cli_socket,s_version,0x05,0x08);
			ExitThread(SUCCESS);
		}
		
		rem_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if ( rem_socket == INVALID_SOCKET ) 
		{
			deb( "in processsocksclient: Error at socket(): %ld\n", WSAGetLastError() );
			SendErrorToClient(cli_socket,s_version,0x05,0x01);
			ExitThread(ERR);
		}
		
		sin.sin_family = AF_INET;
		sin.sin_addr = sq5.sin_addr;
		sin.sin_port = sq5.d_port;
		
		if(connect(rem_socket,(struct sockaddr*) &sin,sizeof(sin)) == SOCKET_ERROR) 
		{
			deb("connect error in procsockscl: %d\n",WSAGetLastError());
			SendErrorToClient(cli_socket,s_version,0x05,0x04);
			closesocket(rem_socket);
			ExitThread(SUCCESS);
		}
		/* success connect, send reply about this */
		
		s5rep.ver = 0x05;
		s5rep.rep = 0x00;
		s5rep.rsv = 0x00;
		s5rep.atyp = sq5.atyp;
		getsockname(rem_socket,(struct sockaddr*) &peer,&peer_size);
		s5rep.local_addr = peer.sin_addr;
		s5rep.local_port = peer.sin_port;
		
		if(send(cli_socket,(char*) &s5rep,sizeof(s5rep),0) == SOCKET_ERROR) 
		{
			deb("send error while sending reply %d\n",WSAGetLastError());
			closesocket(cli_socket);
			closesocket(rem_socket);		
			ExitThread(SUCCESS);
		}
		
	} 
	else if(s_version == 0x04) 
	{
		deb("socks 4 version\n");
		sq4.ver = 0x04;
		if((res = recv(cli_socket,(char*) &sq4.cd,sizeof(sq4)-sizeof(char),0)) == SOCKET_ERROR) 
		{
			deb("could not get sq4 packet from client error:[%d]\n",WSAGetLastError());		
			ExitThread(SUCCESS);
		}
		if(sq4.cd != 0x01) 
		{
			deb("method 0x%02x not supported yet.\n",sq4.cd);
			SendErrorToClient(cli_socket,s_version,0x04,91);
			ExitThread(SUCCESS);
		}
		rem_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if ( rem_socket == INVALID_SOCKET ) 
		{
			deb( "in processsocksclient: Error at socket(): %ld\n", WSAGetLastError() );
			SendErrorToClient(cli_socket,s_version,0x04,91);
			ExitThread(SUCCESS);
		}
		
		sin.sin_family = AF_INET;
		sin.sin_addr = sq4.sin_addr;
		sin.sin_port = sq4.d_port;
		
		if(connect(rem_socket,(struct sockaddr*) &sin,sizeof(sin)) == SOCKET_ERROR) 
		{
			deb("connect error in procsockscl: %d\n",WSAGetLastError());
			SendErrorToClient(cli_socket,s_version,0x04,91);
			closesocket(rem_socket);
			ExitThread(SUCCESS);
		}
		/* success connect, send reply about this */
		
		s4rep.vn = 0x00;
		s4rep.cd = 90;
		s4rep.d_port = sq4.d_port;
		s4rep.sin_addr = sq4.sin_addr;
		
		if(send(cli_socket,(char*) &s4rep,sizeof(s4rep),0) == SOCKET_ERROR) 
		{
			deb("send error while sending reply %d\n",WSAGetLastError());
			closesocket(cli_socket);
			closesocket(rem_socket);	
			ExitThread(SUCCESS);
		}
	} 
	else 
	{
		deb("0x%02x is unknown socks version for me.\n",s_version);
#ifdef _DEBUG
		hexdump((char*) &sq4, sizeof(sq4));
#endif
		closesocket(cli_socket);
		ExitThread(SUCCESS);
	}
	
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	/* remote <-> client */ 
	
	deb("remote <-> client started \n");
	peer_size = sizeof(peer);
	getpeername(cli_socket,(struct sockaddr*) &peer,&peer_size);
	deb("client : %s:%d\n",inet_ntoa(peer.sin_addr),ntohs(peer.sin_port));
	getpeername(rem_socket,(struct sockaddr*) &peer,&peer_size);
	deb("remote : %s:%d\n",inet_ntoa(peer.sin_addr),ntohs(peer.sin_port));
	getsockname(rem_socket,(struct sockaddr*) &peer,&peer_size);
	deb("local : %s:%d\n",inet_ntoa(peer.sin_addr),ntohs(peer.sin_port));
	
	
	int s_res;
	int a_res;
	int r_res;
	fd_set fds;
	char *buf;
	int size_buf = SOCKS_BUF_SIZE;
	struct timeval tm;

	memset(&tm, 0x0,sizeof(tm));
	tm.tv_sec = 5;
	buf = (char*) malloc(size_buf);

	if(buf == NULL) 
	{
		closesocket(cli_socket);
		closesocket(rem_socket);
		deb("malloc failed\n");
		ExitThread(ERR);
	}
	
	while(1) 
	{
		FD_ZERO(&fds);
		FD_SET(cli_socket, &fds);
		FD_SET(rem_socket, &fds);
		a_res = select(max(cli_socket,rem_socket), &fds, NULL, NULL, &tm);
		if(a_res == SOCKET_ERROR) 
		{
			closesocket(cli_socket);
			closesocket(rem_socket);	
			free(buf);
			ExitThread(SUCCESS);
		} 
		
		if(FD_ISSET(cli_socket,&fds)) 
		{
			s_res = recv(cli_socket,buf,size_buf-1,0);
			if(s_res == SOCKET_ERROR || s_res == 0) 
			{
				closesocket(cli_socket);
				closesocket(rem_socket);
				free(buf);
				ExitThread(SUCCESS);
			}
			r_res = send(rem_socket,buf,s_res,0);
			if(r_res == SOCKET_ERROR) 
			{
				closesocket(cli_socket);
				closesocket(rem_socket);
				free(buf);
				ExitThread(SUCCESS);
			}
		} else if(FD_ISSET(rem_socket,&fds)) 
		{
			r_res = recv(rem_socket,buf,size_buf-1,0);
			if(r_res == SOCKET_ERROR || r_res == 0) 
			{
				closesocket(cli_socket);
				closesocket(rem_socket);
				free(buf);
				ExitThread(SUCCESS);
			}
			s_res = send(cli_socket,buf,r_res,0);
			if(s_res == SOCKET_ERROR) 
			{
				closesocket(cli_socket);
				closesocket(rem_socket);
				free(buf);;
				ExitThread(SUCCESS);
			}
		}
		
	}
}