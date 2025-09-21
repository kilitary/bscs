# BSCS Client for Windows32

This is a client implementation for the BSCS (Back Socks Firewall Fucker) server, designed to work with Windows32 using GCC compiler and GNU Make builder.

## Overview

The BSCS client connects to a BSCS server and acts as a bridge for network connections. When the server receives SOCKS proxy requests, it can forward connection requests to registered clients (like this one) to establish connections on behalf of the SOCKS clients.

## Architecture

- **bscs_client.c** - Main program entry point and command handling
- **client_core.c** - Core networking functions (connection, IP resolution, debugging)
- **client_protocol.c** - Protocol handling and bidirectional I/O
- **bscs_client.h** - Header file with all protocol definitions and function prototypes

## Protocol Support

The client implements the BSCS control protocol:

### Messages sent to server:
- `MSG_INFO` (0xA4) - Client information and capabilities
- `MSG_READY` (0xA5) - Ready to accept connections
- `MSG_PONG` (0xA9) - Response to server ping
- `MSG_SOCKET` (0xAA) - Socket connection established
- `MSG_ERROR` (0xA6) - Error notification

### Commands received from server:
- `CMD_PING` (0xE2) - Ping request (respond with MSG_PONG)
- `CMD_SYN` (0xE1) - Synchronization/connection request
- `CMD_BYE` (0xE3) - Disconnect command

## Building

### Requirements
- Windows 32-bit or 64-bit
- GCC compiler (MinGW recommended for Windows)
- GNU Make
- Winsock2 library (included with Windows)

### Build Commands

```bash
# Build release version
make

# Build debug version with verbose logging
make debug

# Clean build artifacts
make clean

# Install to parent directory
make install

# Show help
make help
```

## Usage

### Basic Usage
```bash
# Connect to local server on default port (83)
bscs_client.exe

# Connect to specific server
bscs_client.exe 192.168.1.100

# Connect to specific server and port
bscs_client.exe 192.168.1.100 83
```

### Command Line Arguments
1. **Server IP** (optional, default: 127.0.0.1) - IP address of BSCS server
2. **Server Port** (optional, default: 83) - Control port of BSCS server

### Example Session
```
C:\> bscs_client.exe 192.168.1.100
BSCS Client v1 starting...
CLIENT:<1234> Connecting to server 192.168.1.100:83
CLIENT:<1234> Connected to server successfully
CLIENT:<1234> Local IP: 192.168.1.50
CLIENT:<1234> MSG_INFO sent successfully
CLIENT:<1234> Sent initial MSG_READY
CLIENT:<1234> Starting command handling loop
CLIENT:<1234> Received command: 0xE2
CLIENT:<1234> Received CMD_PING, responding with MSG_PONG
CLIENT:<1234> Sent MSG_READY
```

## Network Flow

1. **Registration**: Client connects to server control port (83) and sends MSG_INFO
2. **Ready State**: Client sends MSG_READY to indicate availability
3. **Command Processing**: Client waits for and processes server commands:
   - **CMD_PING**: Responds with MSG_PONG to maintain connection
   - **CMD_SYN**: Creates connection to specified target and responds with MSG_SOCKET
   - **CMD_BYE**: Gracefully disconnects from server
4. **Connection Forwarding**: When CMD_SYN received, client establishes connection to target and handles bidirectional data transfer

## Configuration

Key configuration values in `bscs_client.h`:
- `CONTROLPORT`: Default server control port (83)
- `BACK_SOCKS_VERSION`: Protocol version (1)
- `BIO_BUF_SIZE`: Buffer size for data transfer (16384 bytes)
- `BIO_SOCKET_TIMEOUT`: Socket timeout for bidirectional I/O (5 seconds)

## Error Handling

The client includes comprehensive error handling:
- Network connection failures
- Protocol violations
- Memory allocation failures
- Socket errors with Windows error codes

Debug output shows detailed information about all operations when built in debug mode.

## Security Considerations

- No authentication mechanism - ensure server is trusted
- No encryption - data transmitted in plaintext
- Ensure proper firewall configuration
- Monitor for unauthorized usage

## Troubleshooting

### Common Issues

1. **Connection Failed**
   - Check server IP and port
   - Verify server is running
   - Check firewall settings

2. **Build Errors**
   - Ensure MinGW/GCC is properly installed
   - Verify Winsock2 libraries are available
   - Check Make is GNU Make

3. **Runtime Errors**
   - Run in debug mode for verbose logging: `make debug`
   - Check Windows event logs
   - Verify network connectivity

### Debug Mode
Build with `make debug` to enable verbose logging that shows:
- All network operations
- Protocol message exchanges
- Error details with Windows error codes
- Thread information

## License

This implementation follows the same licensing as the original BSCS project.