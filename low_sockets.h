#define BIO_BUF_SIZE 16384
#define BIO_SOCKET_TIMEOUT 5

int ReadyToRead(SOCKET Socket, WORD timeout);
DWORD resolve(char* Host) ;
char* GetLocalInetAddr(void);
int sockets_bio(SOCKET first, SOCKET two);