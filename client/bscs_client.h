#ifndef BSCS_CLIENT_H
#define BSCS_CLIENT_H

// Cross-platform compatibility
#ifdef _WIN32
    // Windows compatibility for GCC/MinGW
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    // Windows-specific types and functions
    #define THREAD_RETURN DWORD WINAPI
    #define THREAD_PARAM LPVOID
    #define CREATE_THREAD(func, param) CreateThread(NULL, 0, func, param, 0, NULL)
    #define CLOSE_THREAD_HANDLE(handle) CloseHandle(handle)
    #define GET_THREAD_ID() GetCurrentThreadId()
    #define SLEEP_MS(ms) Sleep(ms)
    
#else
    // Unix/Linux compatibility
    #ifndef UNIX_BUILD
    #define UNIX_BUILD
    #endif
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <errno.h>
    #include <sys/select.h>
    #include <sys/time.h>
    
    // Windows compatibility layer for Unix
    typedef int SOCKET;
    typedef unsigned long DWORD;
    typedef void* HANDLE;
    typedef void* LPVOID;
    typedef unsigned short WORD;
    
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define NO_ERROR 0
    #define closesocket close
    #define WSAGetLastError() errno
    #define WSAStartup(ver, data) 0
    #define WSACleanup() (void)0
    #ifndef INADDR_NONE
    #define INADDR_NONE ((unsigned long) -1)
    #endif
    
    #define THREAD_RETURN void*
    #define THREAD_PARAM void*
    #define CREATE_THREAD(func, param) ({ \
        pthread_t thread; \
        pthread_create(&thread, NULL, func, param) == 0 ? (HANDLE)thread : NULL; \
    })
    #define CLOSE_THREAD_HANDLE(handle) pthread_detach((pthread_t)handle)
    #define GET_THREAD_ID() ((unsigned int)pthread_self())
    #define SLEEP_MS(ms) usleep((ms) * 1000)
    
    // Dummy WSADATA for compatibility
    struct _WSADATA { int dummy; };
    typedef struct _WSADATA WSADATA;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Configuration
#define CONTROLPORT 83
#define BACK_SOCKS_VERSION 1
#define CLIENT_BUF_SIZE 1024
#define BIO_BUF_SIZE 16384
#define BIO_SOCKET_TIMEOUT 5

// Return codes
#define SUCCESS 0
#define ERR -2
#define ENABLE 1
#define DISABLE 0

// Protocol definitions from server
enum { MSG_INFO = 0xA4, MSG_READY, MSG_ERROR, MSG_BUSY, MSG_RESET, MSG_PONG, MSG_SOCKET };
enum { CMD_SYN = 0xE1, CMD_PING, CMD_BYE };
enum { ERR_SYN_TIMEOUT = 0x04, ERR_NO_ROUTE, ERR_RESOURCES, ERR_AGAIN };

#pragma pack(1)
struct msg_info
{
    unsigned char version;
    struct sockaddr_in host;
    int unused1;
    int unused2;
    int unused3;
    int unused4;
};
#pragma pack()

struct cmd_syn_data
{
    unsigned char version;
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
    unsigned char version;
    char hostname[64];
    int port;
    int unused1;
    int unused2;
};

// Function prototypes
int client_connect_to_server(const char* server_ip, int server_port);
int client_send_info(SOCKET sock);
int client_handle_commands(SOCKET control_sock);
int client_handle_syn_command(SOCKET control_sock, struct cmd_syn_data* syn_data);
int client_create_connection(const char* hostname, int port);
int sockets_bio(SOCKET first, SOCKET second);
void client_debug(const char* format, ...);
char* get_local_ip(void);
int resolve_hostname(const char* hostname);
THREAD_RETURN client_connection_handler(THREAD_PARAM param);

// Global variables
extern int g_running;
extern int g_debug_enabled;

#endif // BSCS_CLIENT_H