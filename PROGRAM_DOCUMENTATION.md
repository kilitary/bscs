# BSCS - Back Socks Firewall Fucker v1
## Complete Program Documentation and Input/Output Analysis

### Overview
BSCS (Back Socks Firewall Fucker) is a Windows-based network tunneling application that implements a reverse SOCKS proxy system to bypass firewalls and NAT restrictions. The program enables clients behind restrictive networks to establish outbound connections that can then be used by external systems to access internal resources.

---

## Program Architecture

### Core Components

1. **Main Application (bscs.cpp)**
   - Entry point that initializes the control link
   - Runs in a monitoring loop waiting for shutdown

2. **Control Link System (mbscs.cpp)**
   - Manages the main control interface on port 83
   - Handles client registrations and connection management
   - Creates worker threads for each client connection

3. **SOCKS Proxy Core (socks_core.cpp)**
   - Implements SOCKS4 and SOCKS5 protocol handling
   - Manages proxy connections on configurable ports
   - Handles bidirectional data transfer

4. **Low-Level Socket Operations (low_sockets.cpp)**
   - Provides socket utility functions
   - Implements non-blocking I/O operations
   - Handles hostname resolution

5. **Support Modules**
   - debug.cpp: Debugging and error reporting
   - memory.cpp: Memory management wrappers
   - globals.cpp: Global state management

---

## Network Ports and Protocols

### Default Port Configuration
- **Control Port**: 83 (CONTROLPORT)
- **Default SOCKS Port**: 65530 (SOCKS_PORT)
- **Protocol**: TCP only
- **Address Binding**: INADDR_ANY (0.0.0.0)

### Communication Protocols

#### Control Link Protocol
The control link uses a custom binary protocol for communication between the server and clients:

**Message Types (Client to Server):**
- `MSG_INFO` (0xA4): Client information packet
- `MSG_READY` (0xA5): Client ready for connections
- `MSG_ERROR` (0xA6): Error notification
- `MSG_BUSY` (0xA7): Client busy status
- `MSG_RESET` (0xA8): Reset connection
- `MSG_PONG` (0xA9): Response to ping
- `MSG_SOCKET` (0xAA): Socket information

**Command Types (Server to Client):**
- `CMD_SYN` (0xE1): Synchronization request
- `CMD_PING` (0xE2): Ping request
- `CMD_BYE` (0xE3): Disconnect command

#### SOCKS Protocol Support
Supports both SOCKS4 and SOCKS5 protocols:

**SOCKS4 Packet Structure:**
```c
struct s4_packet {
    char ver;              // Version (0x04)
    char cd;               // Command code
    WORD d_port;          // Destination port
    struct in_addr sin_addr; // Destination IP
    char userid[10];       // User ID
};
```

**SOCKS5 Packet Structure:**
```c
struct s5_packet {
    char ver;              // Version (0x05)
    char cmd;              // Command code
    char rsv;              // Reserved
    char atyp;             // Address type
    struct in_addr sin_addr; // Destination IP
    WORD d_port;          // Destination port
};
```

---

## Program Flow and Execution

### 1. Initialization Phase
```
main() 
├── OpenControlLink()
│   ├── WSAStartup() - Initialize Winsock
│   ├── socket() - Create control socket
│   ├── bind() - Bind to CONTROLPORT (83)
│   └── listen() - Start listening for connections
└── Sleep loop waiting for shutdown
```

### 2. Client Connection Handling
```
OpenControlLink()
├── accept() - Accept client connections
├── CreateThread(ConnectRemoteMachine) - Handle each client
└── Loop back to accept more connections

ConnectRemoteMachine()
├── Process client messages
├── Handle SOCKS requests
├── Create pcr() threads for port forwarding
└── Manage bidirectional data flow
```

### 3. SOCKS Proxy Operation
```
SocksServer()
├── CreateThread(CoreSocks) - Start SOCKS listener
└── Return thread ID

CoreSocks()
├── socket() - Create SOCKS socket
├── bind() - Bind to SOCKS port
├── listen() - Listen for SOCKS clients
├── accept() - Accept SOCKS connections
└── CreateThread(ProcessSocksClient) - Handle each SOCKS client

ProcessSocksClient()
├── Parse SOCKS request
├── Establish remote connection
├── sockets_bio() - Bidirectional data transfer
└── Connection cleanup
```

---

## Input Specifications

### Command Line Arguments
The program currently accepts no command line arguments. All configuration is compile-time through header files.

### Network Inputs

#### Control Link Inputs
1. **Client Registration**
   - Source: Network clients connecting to port 83
   - Format: Binary protocol messages
   - Content: Client capabilities, network information

2. **Control Commands**
   - Source: Administrative connections
   - Format: Command packets (CMD_*)
   - Content: Synchronization, ping, disconnect commands

#### SOCKS Proxy Inputs
1. **SOCKS Connection Requests**
   - Source: SOCKS clients (browsers, applications)
   - Format: SOCKS4/SOCKS5 protocol packets
   - Content: Target hostname/IP, port, authentication

2. **Data Packets**
   - Source: Established SOCKS connections
   - Format: Raw TCP data streams
   - Content: HTTP, HTTPS, FTP, or any TCP-based protocol data

### Configuration Inputs (Compile-time)
From `configuration.h`:
- `CONTROLPORT`: Control interface port (default: 83)
- `SOCKS_PORT`: Default SOCKS proxy port (default: 65530)
- `BACK_SOCKS_VERSION`: Protocol version (default: 1)
- `SOCKS_BUF_SIZE`: Buffer size for data transfer (default: 32768)
- `CRM_BUF_SIZE`: Control message buffer size (default: 1024)

---

## Output Specifications

### Debug Output
When compiled in debug mode, the program generates extensive logging:

#### Debug Message Format
```
SERVER:<ThreadID> <message>
```

#### Key Debug Messages
1. **Startup Messages**
   ```
   cl: Back Socks Firewall Fucker v1 starting...
   ```

2. **Connection Events**
   ```
   cl: accepting control connections...
   cl: accepted connection <socket_handle>
   socks port <port>: bind() failed. err:<error_code>
   ```

3. **Client Information**
   ```
   client : <ip>:<port>
   remote : <ip>:<port>
   local : <ip>:<port>
   ```

4. **Error Conditions**
   ```
   cl: Error at WSAStartup()
   cl: Error at socket(): <error_code>
   malloc failed
   ```

### Network Outputs

#### Control Link Responses
1. **Status Acknowledgments**
   - Target: Connected clients
   - Format: Binary protocol responses
   - Content: Connection status, error codes

2. **Command Responses**
   - Target: Administrative connections
   - Format: Response packets
   - Content: Ping responses, status updates

#### SOCKS Proxy Responses
1. **SOCKS Reply Packets**
   - Target: SOCKS clients
   - Format: SOCKS4/SOCKS5 reply structures
   - Content: Connection status, bound addresses

2. **Data Streams**
   - Target: SOCKS clients and remote servers
   - Format: Raw TCP data
   - Content: Proxied application data

### Return Codes
- `SUCCESS` (0): Operation completed successfully
- `ERR` (-2): Generic error condition
- `SOCKET_ERROR`: Winsock error (typically -1)

---

## Error Handling and Edge Cases

### Common Error Scenarios

1. **Network Initialization Failures**
   - WSAStartup() failure
   - Socket creation errors
   - Port binding conflicts

2. **Connection Errors**
   - Client disconnections
   - Remote host unreachable
   - Connection timeouts

3. **Resource Exhaustion**
   - Memory allocation failures
   - Thread creation limits
   - Socket descriptor limits

4. **Protocol Violations**
   - Invalid SOCKS requests
   - Malformed control messages
   - Unsupported protocol versions

### Error Recovery Mechanisms
- Thread-level error isolation
- Socket cleanup on failures
- Graceful thread termination
- Resource deallocation

---

## Memory Management

### Buffer Sizes
- SOCKS data buffer: 32,768 bytes
- Control message buffer: 1,024 bytes
- BIO buffer: 16,384 bytes
- Debug string buffer: 32,768 bytes

### Memory Allocation
- Uses Windows Heap API through wrapper functions
- `halloc()`: Allocates memory with error checking
- `hfree()`: Deallocates memory with validation
- Thread-local storage for temporary data

---

## Threading Model

### Thread Types
1. **Main Thread**: Program initialization and monitoring
2. **Control Link Thread**: Handles control port connections
3. **Client Handler Threads**: One per connected client
4. **SOCKS Server Thread**: Manages SOCKS proxy port
5. **SOCKS Client Threads**: One per SOCKS connection
6. **Port Forwarder Threads**: Handle specific port forwarding

### Synchronization
- Global variables for state management
- Thread-safe socket operations
- Independent thread execution model

---

## Usage Scenarios

### Typical Deployment
1. **Server Setup**: Run BSCS on a server accessible from the internet
2. **Client Registration**: Internal clients connect to control port (83)
3. **SOCKS Configuration**: External clients configure SOCKS proxy settings
4. **Traffic Flow**: External traffic flows through registered internal clients

### Example Connection Flow
```
External Client -> SOCKS Proxy (port 65530) -> BSCS Server -> Internal Client -> Target Service
```

---

## Security Considerations

### Potential Vulnerabilities
1. No authentication mechanism for control connections
2. Open SOCKS proxy without access controls
3. Potential for abuse as an open relay
4. No encryption for data in transit

### Recommended Mitigations
- Deploy behind proper firewall rules
- Implement IP-based access controls
- Monitor connection logs for abuse
- Consider VPN or encrypted tunnel overlay

---

## Build and Deployment

### Build Requirements
- Windows development environment
- Visual Studio project files provided
- Winsock2 library dependencies
- Windows API headers

### Build Configuration
- Debug build: Full logging enabled
- Release build: Optimized with minimal logging
- Compiler optimizations available

### Runtime Requirements
- Windows operating system
- Winsock2 support
- Administrative privileges for low port binding
- Network connectivity

---

## Conclusion

BSCS provides a reverse proxy tunneling solution for bypassing network restrictions. The program implements a custom control protocol combined with standard SOCKS proxy functionality to enable flexible network traversal. While functional, the implementation lacks modern security features and should be deployed with appropriate network controls and monitoring.