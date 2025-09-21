#include "bscs_client.h"
#ifdef UNIX_BUILD
#include <stdint.h>
#endif

int sockets_bio(SOCKET first, SOCKET second)
{
    fd_set fds;
    int result, bytes_received, bytes_sent;
    char bio_buffer[BIO_BUF_SIZE];
    struct timeval timeout;

    client_debug("Starting bidirectional I/O between sockets %d and %d", (int)first, (int)second);

    while (g_running) {
        FD_ZERO(&fds);
        FD_SET(first, &fds);
        FD_SET(second, &fds);

        timeout.tv_sec = BIO_SOCKET_TIMEOUT;
        timeout.tv_usec = 0;

        result = select(0, &fds, NULL, NULL, &timeout);
        if (result == SOCKET_ERROR) {
            client_debug("select() failed: %d", WSAGetLastError());
            break;
        }

        if (result == 0) {
            // Timeout - continue to check g_running
            continue;
        }

        if (FD_ISSET(first, &fds)) {
            bytes_received = recv(first, bio_buffer, BIO_BUF_SIZE - 1, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    client_debug("Socket %d closed connection", (int)first);
                } else {
                    client_debug("recv() from socket %d failed: %d", (int)first, WSAGetLastError());
                }
                break;
            }

            bytes_sent = send(second, bio_buffer, bytes_received, 0);
            if (bytes_sent == SOCKET_ERROR) {
                client_debug("send() to socket %d failed: %d", (int)second, WSAGetLastError());
                break;
            }
            client_debug("Forwarded %d bytes from socket %d to socket %d", bytes_received, (int)first, (int)second);
        }

        if (FD_ISSET(second, &fds)) {
            bytes_received = recv(second, bio_buffer, BIO_BUF_SIZE - 1, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    client_debug("Socket %d closed connection", (int)second);
                } else {
                    client_debug("recv() from socket %d failed: %d", (int)second, WSAGetLastError());
                }
                break;
            }

            bytes_sent = send(first, bio_buffer, bytes_received, 0);
            if (bytes_sent == SOCKET_ERROR) {
                client_debug("send() to socket %d failed: %d", (int)first, WSAGetLastError());
                break;
            }
            client_debug("Forwarded %d bytes from socket %d to socket %d", bytes_received, (int)second, (int)first);
        }
    }

    client_debug("Bidirectional I/O finished");
    return SUCCESS;
}

int client_create_connection(const char* hostname, int port)
{
    SOCKET sock;
    struct sockaddr_in addr;
    DWORD resolved_addr;

    client_debug("Creating connection to %s:%d", hostname, port);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        client_debug("socket() failed: %d", WSAGetLastError());
        return INVALID_SOCKET;
    }

    resolved_addr = resolve_hostname(hostname);
    if (resolved_addr == 0) {
        client_debug("Failed to resolve hostname: %s", hostname);
        closesocket(sock);
        return INVALID_SOCKET;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = resolved_addr;

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        client_debug("connect() to %s:%d failed: %d", hostname, port, WSAGetLastError());
        closesocket(sock);
        return INVALID_SOCKET;
    }

    client_debug("Connected to %s:%d successfully", hostname, port);
    return sock;
}

THREAD_RETURN client_connection_handler(THREAD_PARAM param)
{
    struct socket_info* conn_info = (struct socket_info*)param;
    SOCKET target_sock;

    client_debug("Connection handler thread started for %s:%d", conn_info->hostname, conn_info->port);

    // Create connection to target
    target_sock = client_create_connection(conn_info->hostname, conn_info->port);
    if (target_sock == INVALID_SOCKET) {
        client_debug("Failed to connect to target %s:%d", conn_info->hostname, conn_info->port);
        free(conn_info);
#ifdef _WIN32
        return ERR;
#else
        return (void*)(intptr_t)ERR;
#endif
    }

    // Create connection back to server (this is a simplified approach)
    // In a real implementation, we'd need to coordinate with the main control connection
    client_debug("Target connection established, notifying server");

    // Perform bidirectional data transfer
    // Note: This is simplified - a full implementation would need to coordinate
    // the server connection properly through the control channel
    sockets_bio(target_sock, target_sock); // Placeholder

    closesocket(target_sock);
    free(conn_info);
    client_debug("Connection handler thread finished");
#ifdef _WIN32
    return SUCCESS;
#else
    return (void*)(intptr_t)SUCCESS;
#endif
}

int client_handle_syn_command(SOCKET control_sock, struct cmd_syn_data* syn_data)
{
    SOCKET target_sock;
    struct socket_info socket_info_msg;
    unsigned char msg_type;
    HANDLE thread_handle;
    struct socket_info* thread_data;

    client_debug("Handling CMD_SYN for %s:%d", syn_data->hostname, syn_data->port);

    // Try to connect to the target
    target_sock = client_create_connection(syn_data->hostname, syn_data->port);
    if (target_sock == INVALID_SOCKET) {
        // Send error response
        msg_type = MSG_ERROR;
        send(control_sock, (char*)&msg_type, 1, 0);
        unsigned char error_code = ERR_NO_ROUTE;
        send(control_sock, (char*)&error_code, 1, 0);
        client_debug("Failed to connect to target, sent MSG_ERROR");
        return ERR;
    }

    // Send MSG_SOCKET response
    msg_type = MSG_SOCKET;
    if (send(control_sock, (char*)&msg_type, 1, 0) != 1) {
        client_debug("Failed to send MSG_SOCKET type");
        closesocket(target_sock);
        return ERR;
    }

    // Prepare socket info
    memset(&socket_info_msg, 0, sizeof(socket_info_msg));
    socket_info_msg.version = BACK_SOCKS_VERSION;
    strncpy(socket_info_msg.hostname, syn_data->hostname, sizeof(socket_info_msg.hostname) - 1);
    socket_info_msg.port = syn_data->port;

    if (send(control_sock, (char*)&socket_info_msg, sizeof(socket_info_msg), 0) != sizeof(socket_info_msg)) {
        client_debug("Failed to send MSG_SOCKET data");
        closesocket(target_sock);
        return ERR;
    }

    client_debug("MSG_SOCKET sent successfully");

    // Create a thread to handle this connection
    thread_data = malloc(sizeof(struct socket_info));
    if (!thread_data) {
        client_debug("Failed to allocate memory for thread data");
        closesocket(target_sock);
        return ERR;
    }

    memcpy(thread_data, &socket_info_msg, sizeof(struct socket_info));

    thread_handle = CREATE_THREAD(client_connection_handler, thread_data);
    if (thread_handle == NULL) {
        client_debug("Failed to create connection handler thread");
        free(thread_data);
        closesocket(target_sock);
        return ERR;
    }

    CLOSE_THREAD_HANDLE(thread_handle);
    return SUCCESS;
}