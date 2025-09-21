#include "bscs_client.h"

int client_handle_commands(SOCKET control_sock)
{
    unsigned char command;
    int result;
    struct cmd_syn_data syn_data;
    unsigned char response;

    client_debug("Starting command handling loop");

    while (g_running) {
        // Receive command from server
        result = recv(control_sock, (char*)&command, 1, 0);
        if (result <= 0) {
            if (result == 0) {
                client_debug("Server closed connection");
            } else {
                client_debug("recv() failed: %d", WSAGetLastError());
            }
            break;
        }

        client_debug("Received command: 0x%02x", command);

        switch (command) {
            case CMD_PING:
                client_debug("Received CMD_PING, responding with MSG_PONG");
                response = MSG_PONG;
                if (send(control_sock, (char*)&response, 1, 0) != 1) {
                    client_debug("Failed to send MSG_PONG");
                    return ERR;
                }
                break;

            case CMD_SYN:
                client_debug("Received CMD_SYN, reading synchronization data");
                
                // Receive syn_data structure
                result = recv(control_sock, (char*)&syn_data, sizeof(syn_data), 0);
                if (result != sizeof(syn_data)) {
                    client_debug("Failed to receive CMD_SYN data: received %d, expected %d", 
                               result, (int)sizeof(syn_data));
                    return ERR;
                }

                client_debug("CMD_SYN: target=%s:%d, bind=%s:%d", 
                           syn_data.hostname, syn_data.port,
                           syn_data.bhostname, syn_data.bport);

                // Handle the synchronization request
                if (client_handle_syn_command(control_sock, &syn_data) != SUCCESS) {
                    client_debug("Failed to handle CMD_SYN");
                    return ERR;
                }
                break;

            case CMD_BYE:
                client_debug("Received CMD_BYE, shutting down");
                g_running = 0;
                return SUCCESS;

            default:
                client_debug("Unknown command: 0x%02x", command);
                // Send error response
                response = MSG_ERROR;
                send(control_sock, (char*)&response, 1, 0);
                unsigned char error = ERR_AGAIN;
                send(control_sock, (char*)&error, 1, 0);
                break;
        }

        // Send MSG_READY after processing command
        response = MSG_READY;
        if (send(control_sock, (char*)&response, 1, 0) != 1) {
            client_debug("Failed to send MSG_READY");
        } else {
            client_debug("Sent MSG_READY");
        }
    }

    client_debug("Command handling loop finished");
    return SUCCESS;
}

int main(int argc, char* argv[])
{
    const char* server_ip = "127.0.0.1";
    int server_port = CONTROLPORT;
    SOCKET control_sock;

    printf("BSCS Client v%d starting...\n", BACK_SOCKS_VERSION);
    
    // Check for help
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("Usage: %s [server_ip] [server_port]\n", argv[0]);
        printf("\nArguments:\n");
        printf("  server_ip   - IP address of BSCS server (default: 127.0.0.1)\n");
        printf("  server_port - Control port of BSCS server (default: %d)\n", CONTROLPORT);
        printf("\nExample:\n");
        printf("  %s 192.168.1.100 83\n", argv[0]);
        printf("\nThe client will connect to the BSCS server and register as available\n");
        printf("for handling connection forwarding requests.\n");
        return SUCCESS;
    }
    
    // Parse command line arguments
    if (argc >= 2) {
        server_ip = argv[1];
    }
    if (argc >= 3) {
        server_port = atoi(argv[2]);
        if (server_port <= 0 || server_port > 65535) {
            printf("Invalid port number: %s\n", argv[2]);
            return ERR;
        }
    }

    client_debug("Connecting to BSCS server at %s:%d", server_ip, server_port);

    // Connect to server
    control_sock = client_connect_to_server(server_ip, server_port);
    if (control_sock == INVALID_SOCKET) {
        printf("Failed to connect to server\n");
        return ERR;
    }

    // Send client information
    if (client_send_info(control_sock) != SUCCESS) {
        printf("Failed to send client information\n");
        closesocket(control_sock);
        WSACleanup();
        return ERR;
    }

    // Send MSG_READY to indicate we're ready for commands
    unsigned char ready_msg = MSG_READY;
    if (send(control_sock, (char*)&ready_msg, 1, 0) != 1) {
        client_debug("Failed to send initial MSG_READY");
        closesocket(control_sock);
        WSACleanup();
        return ERR;
    }
    client_debug("Sent initial MSG_READY");

    // Handle commands from server
    client_handle_commands(control_sock);

    // Cleanup
    client_debug("Shutting down client");
    closesocket(control_sock);
    WSACleanup();
    
    printf("BSCS Client shut down\n");
    return SUCCESS;
}