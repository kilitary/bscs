#include "bscs_client.h"
#include <stdarg.h>
#ifdef UNIX_BUILD
#include <stdint.h>
#endif

// Global variables
int g_running = 1;
int g_debug_enabled = 1;

void client_debug(const char* format, ...)
{
    if (!g_debug_enabled) return;
    
    va_list args;
    char buffer[1024];
    char output[1200];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    snprintf(output, sizeof(output), "CLIENT:<%X> %s", GET_THREAD_ID(), buffer);
    printf("%s\n", output);
    fflush(stdout);
}

char* get_local_ip(void)
{
    static char name[255];
    struct hostent *hostinfo;
    
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR) {
        client_debug("Error at WSAStartup()");
        return NULL;
    }
#endif

    if (gethostname(name, 255) == 0) {
        if ((hostinfo = gethostbyname(name)) != NULL) {
            return inet_ntoa(*(struct in_addr *)*hostinfo->h_addr_list);
        }
        client_debug("gethostbyname failed");
    }
    client_debug("gethostname failed");
    
    return NULL;
}

int resolve_hostname(const char* hostname)
{
    DWORD addr;
    struct hostent *he;

    addr = inet_addr(hostname);
    if (addr != INADDR_NONE) {
        return addr;
    }

    he = gethostbyname(hostname);
    if (he == NULL) {
        client_debug("resolve error: %d", WSAGetLastError());
        return 0;
    }

    addr = ((struct in_addr *)(he->h_addr_list[0]))->s_addr;
    return addr;
}

int client_connect_to_server(const char* server_ip, int server_port)
{
    SOCKET sock;
    struct sockaddr_in server_addr;
    
    client_debug("Connecting to server %s:%d", server_ip, server_port);
    
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR) {
        client_debug("WSAStartup failed: %d", iResult);
        return INVALID_SOCKET;
    }
#endif
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        client_debug("socket() failed: %d", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        server_addr.sin_addr.s_addr = resolve_hostname(server_ip);
        if (server_addr.sin_addr.s_addr == 0) {
            client_debug("Failed to resolve hostname: %s", server_ip);
            closesocket(sock);
            WSACleanup();
            return INVALID_SOCKET;
        }
    }
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        client_debug("connect() failed: %d", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return INVALID_SOCKET;
    }
    
    client_debug("Connected to server successfully");
    return sock;
}

int client_send_info(SOCKET sock)
{
    struct msg_info info;
    char* local_ip;
    unsigned char msg_type = MSG_INFO;
    
    client_debug("Sending MSG_INFO to server");
    
    // Send message type first
    if (send(sock, (char*)&msg_type, 1, 0) != 1) {
        client_debug("Failed to send MSG_INFO type: %d", WSAGetLastError());
        return ERR;
    }
    
    // Prepare info structure
    memset(&info, 0, sizeof(info));
    info.version = BACK_SOCKS_VERSION;
    
    // Get local IP
    local_ip = get_local_ip();
    if (local_ip) {
        info.host.sin_family = AF_INET;
        info.host.sin_addr.s_addr = inet_addr(local_ip);
        info.host.sin_port = 0; // Client doesn't listen on specific port
        client_debug("Local IP: %s", local_ip);
    } else {
        client_debug("Failed to get local IP, using localhost");
        info.host.sin_family = AF_INET;
        info.host.sin_addr.s_addr = inet_addr("127.0.0.1");
        info.host.sin_port = 0;
    }
    
    // Send info structure
    if (send(sock, (char*)&info, sizeof(info), 0) != sizeof(info)) {
        client_debug("Failed to send MSG_INFO data: %d", WSAGetLastError());
        return ERR;
    }
    
    client_debug("MSG_INFO sent successfully");
    return SUCCESS;
}