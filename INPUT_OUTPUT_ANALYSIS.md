# BSCS Input/Output Technical Analysis

## Detailed Input/Output Specification

### Program Execution Flow

```
STARTUP SEQUENCE:
main() → OpenControlLink() → Multi-threaded Operation

INPUT: None (no command line arguments)
OUTPUT: Process execution, network services on ports 83 and 65530
```

---

## Network I/O Analysis

### Control Port (83) - Input/Output Behavior

#### Inputs Accepted:
1. **TCP Connection Requests**
   ```
   Input: TCP SYN packets to port 83
   Processing: accept() system call
   Result: New client socket created
   ```

2. **Control Protocol Messages**
   ```
   Message Structure:
   - Header: 1 byte message type
   - Payload: Variable length data
   
   Supported Input Messages:
   MSG_INFO (0xA4):
     Input: Client network information
     Format: struct msg_info (contains IP, version, etc.)
     Processing: Store client capabilities
     Output: Acknowledgment or error response
   
   MSG_READY (0xA5):
     Input: Client ready signal
     Processing: Mark client as available
     Output: May trigger new connection assignments
   
   MSG_ERROR (0xA6):
     Input: Error notification from client
     Processing: Log error, possibly cleanup resources
     Output: Error logged to debug output
   
   MSG_BUSY (0xA7):
     Input: Client busy notification
     Processing: Mark client as unavailable
     Output: Updated client status
   
   MSG_PONG (0xA9):
     Input: Response to ping
     Processing: Update client alive status
     Output: Reset timeout counters
   ```

#### Outputs Generated:
1. **Command Messages to Clients**
   ```
   CMD_SYN (0xE1):
     Output: Synchronization request with target info
     Format: struct cmd_syn_data
     Content: hostname[64], port, sockaddr_in structures
   
   CMD_PING (0xE2):
     Output: Keep-alive ping
     Processing: Periodic health check
   
   CMD_BYE (0xE3):
     Output: Disconnect command
     Processing: Graceful client termination
   ```

2. **Debug Output (Console/OutputDebugString)**
   ```
   Format: "SERVER:<ThreadID> <message>"
   
   Examples:
   "SERVER:<1234> cl: Back Socks Firewall Fucker v1 starting..."
   "SERVER:<1234> cl: accepted connection 0x48C"
   "SERVER:<5678> pcr: enter, pcrData: 0x12345678"
   ```

---

### SOCKS Port (65530) - Input/Output Behavior

#### SOCKS4 Protocol Handling:

**Input Packet Structure:**
```c
struct s4_packet {
    ver:      0x04          // SOCKS version
    cd:       0x01          // CONNECT command
    d_port:   2 bytes       // Destination port (network order)
    sin_addr: 4 bytes       // Destination IP
    userid:   variable      // Null-terminated user ID
}
```

**Processing:**
1. Validate version (must be 0x04)
2. Extract destination IP and port
3. Attempt connection to target
4. Generate response packet

**Output Response:**
```c
struct s4_reply {
    vn:       0x00          // Version of reply (0x00)
    cd:       status        // Command code (0x5A = granted)
    d_port:   2 bytes       // Port (usually same as request)
    sin_addr: 4 bytes       // IP (usually same as request)
}

Status Codes:
0x5A = Request granted
0x5B = Request rejected or failed
0x5C = Request failed (client not running identd)
0x5D = Request failed (identd could not confirm user)
```

#### SOCKS5 Protocol Handling:

**Authentication Negotiation Input:**
```
Byte 0: Version (0x05)
Byte 1: Number of methods
Byte 2+: Method list (0x00 = no auth)
```

**Authentication Response Output:**
```
Byte 0: Version (0x05)
Byte 1: Selected method (0x00 = no auth, 0xFF = no acceptable methods)
```

**Connection Request Input:**
```c
struct s5_packet {
    ver:      0x05          // SOCKS version
    cmd:      0x01          // CONNECT command
    rsv:      0x00          // Reserved
    atyp:     type          // Address type
    addr:     variable      // Address (IP or domain)
    port:     2 bytes       // Port (network order)
}

Address Types:
0x01 = IPv4 address (4 bytes)
0x03 = Domain name (1 byte length + name)
0x04 = IPv6 address (16 bytes)
```

**Connection Response Output:**
```c
struct s5_reply {
    ver:        0x05        // SOCKS version
    rep:        status      // Reply code
    rsv:        0x00        // Reserved
    atyp:       0x01        // Address type (IPv4)
    local_addr: 4 bytes     // Bound IP address
    local_port: 2 bytes     // Bound port
}

Reply Codes:
0x00 = Succeeded
0x01 = General SOCKS server failure
0x02 = Connection not allowed by ruleset
0x03 = Network unreachable
0x04 = Host unreachable
0x05 = Connection refused
0x06 = TTL expired
0x07 = Command not supported
0x08 = Address type not supported
```

---

## Data Transfer Operations

### Bidirectional Socket I/O (sockets_bio function)

**Input Sources:**
1. **SOCKS Client Data**
   ```
   Source: Applications using SOCKS proxy
   Format: Raw TCP data streams
   Buffer Size: 16,384 bytes (BIO_BUF_SIZE)
   Processing: Forwarded to remote connection
   ```

2. **Remote Server Data**
   ```
   Source: Target servers being accessed
   Format: Raw TCP data streams
   Buffer Size: 16,384 bytes
   Processing: Forwarded to SOCKS client
   ```

**Processing Logic:**
```c
while(1) {
    select() on both sockets with 5-second timeout
    
    if (data available on client socket):
        recv() from client
        send() to remote server
    
    if (data available on remote socket):
        recv() from remote server  
        send() to client
    
    if (error or connection closed):
        cleanup and exit thread
}
```

**Output Behavior:**
- Transparent data forwarding
- Connection termination on any socket error
- Thread cleanup on completion

---

## Memory I/O Operations

### Allocation Patterns:
```c
// Control message buffer allocation
char *buffer = (char*) halloc(CRM_BUF_SIZE);  // 1024 bytes

// SOCKS data buffer allocation  
char *buf = (char*) malloc(SOCKS_BUF_SIZE);   // 32768 bytes

// BIO transfer buffer
char bio_buffer[BIO_BUF_SIZE];                // 16384 bytes (stack)
```

### Memory I/O Results:
- **Success**: Returns valid pointer, memory ready for use
- **Failure**: Returns NULL, logs error via `deb()` function
- **Cleanup**: Automatic on thread termination

---

## File System I/O

### Debug Output File Operations:
```c
// Console output
printf("%s\n", stringout);

// Windows debug output
OutputDebugString(stringout);
```

**Output Format:**
```
SERVER:<ThreadID> <timestamp_implied> <message>
```

**Log Categories:**
- Startup/shutdown events
- Connection establishment/termination
- Error conditions
- Protocol violations
- Resource allocation failures

---

## Error I/O Patterns

### Windows Error Handling:
```c
char* fmterr(void) {
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, GetLastError(), 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf, 0, NULL
    );
    return formatted_error_string;
}
```

### Socket Error Responses:
```c
// Input: Invalid socket operations
// Processing: WSAGetLastError() 
// Output: Numeric error code + descriptive text

Common Error Outputs:
"Error at WSAStartup()"
"Error at socket(): <error_code>"
"bind() failed. err:<error_code>"
"Connection refused"
"Host unreachable"
```

---

## Threading I/O Coordination

### Thread Creation I/O:
```c
// Input: Thread function pointer, parameter data
hsThread = CreateThread(NULL, 0, ThreadFunction, param, 0, &threadId);

// Output Success: Valid thread handle + thread ID
// Output Failure: NULL handle, error logged
```

### Inter-thread Communication:
- **Global Variables**: `ControlLinkEnabled`, `RunControlLink`
- **Socket Handles**: Passed between threads
- **Shared Memory**: Client data structures

---

## Performance Characteristics

### Buffer Sizes and Throughput:
- **SOCKS Buffer**: 32KB → ~250 Mbps theoretical max per connection
- **BIO Buffer**: 16KB → Optimized for low-latency forwarding
- **Control Buffer**: 1KB → Sufficient for protocol messages

### Connection Limits:
- **Theoretical**: Limited by Windows socket handles (~2048)
- **Practical**: Limited by thread creation overhead
- **Memory**: ~50KB per active connection

---

## Input Validation and Sanitization

### SOCKS Request Validation:
```c
// Version field validation
if (packet.ver != 0x04 && packet.ver != 0x05) {
    // Send error response
    SendErrorToClient(socket, version, error_code);
    return ERROR;
}

// Port validation  
if (ntohs(packet.d_port) == 0) {
    // Invalid port
    return ERROR;
}

// Address validation
if (packet.sin_addr.s_addr == 0) {
    // Invalid address
    return ERROR;  
}
```

### Control Message Validation:
```c
// Message type validation
switch(message_type) {
    case MSG_INFO:
    case MSG_READY: 
    case MSG_ERROR:
    case MSG_BUSY:
    case MSG_PONG:
        // Valid message types
        break;
    default:
        // Invalid message, ignore or log
        return ERROR;
}
```

---

## Output Status Codes Summary

### Function Return Values:
- `SUCCESS` (0): Operation completed successfully
- `ERR` (-2): Generic error condition  
- `SOCKET_ERROR` (-1): Winsock error
- `NULL`: Memory allocation failure or invalid handle

### Network Status Outputs:
- **SOCKS Success**: 0x5A (SOCKS4) or 0x00 (SOCKS5)
- **Connection Refused**: 0x5B (SOCKS4) or 0x05 (SOCKS5)
- **Network Unreachable**: 0x03 (SOCKS5 only)
- **Host Unreachable**: 0x04 (SOCKS5 only)

This technical analysis provides the complete input/output specification for the BSCS program, covering all data flows, processing behaviors, and result patterns.