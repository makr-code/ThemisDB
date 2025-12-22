# MCP Transport Layer Implementation

## Overview

ThemisDB's MCP (Model Context Protocol) server supports three transport mechanisms for LLM integration:

1. **stdio** - Standard input/output for Claude Desktop (Linux/macOS)
2. **SSE (Server-Sent Events)** - HTTP-based unidirectional streaming
3. **WebSocket** - Bidirectional real-time communication

All transports implement the same JSON-RPC 2.0 protocol and provide identical MCP functionality.

## Transport Comparison

| Feature | stdio | SSE | WebSocket |
|---------|-------|-----|-----------|
| **Direction** | Bidirectional | Server→Client | Bidirectional |
| **Connection** | Process pipes | HTTP long-polling | TCP socket |
| **Reconnect** | Process restart | Automatic (SSE) | Manual |
| **Multiplexing** | Single client | Multiple clients | Multiple sessions |
| **Latency** | Very low | Low | Very low |
| **Complexity** | Simple | Moderate | Moderate |
| **Use Case** | Claude Desktop | Web dashboards | Real-time apps |
| **Platform** | POSIX only | Cross-platform | Cross-platform |

## 1. stdio Transport

### Description
Uses standard input/output streams for communication with Claude Desktop on POSIX systems (Linux/macOS).

### Configuration
```cpp
McpServer::Config config;
config.enable_stdio = true;
config.stdio_buffer_size = 4096;

McpServer server(io_context, config);
server.start();
```

### Claude Desktop Configuration
```json
{
  "mcpServers": {
    "themisdb": {
      "command": "/usr/local/bin/themis_server",
      "args": ["--mcp-stdio", "--db-path", "/var/lib/themisdb"]
    }
  }
}
```

### Characteristics
- **Protocol**: JSON-RPC 2.0 over stdin/stdout
- **Framing**: Newline-delimited JSON
- **Async I/O**: Uses `select()` for non-blocking reads
- **Limitations**: Single client, POSIX only, requires process lifecycle management

### Example Exchange
```bash
# Client (Claude) sends via stdin
{"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_entity","arguments":{"key":"user:123"}},"id":1}

# Server responds via stdout
{"jsonrpc":"2.0","result":{"status":"success","key":"user:123","value":{"name":"Alice"}},"id":1}
```

## 2. SSE (Server-Sent Events) Transport

### Description
HTTP-based unidirectional streaming from server to client. Clients initiate requests via HTTP POST and receive responses + notifications via SSE stream.

### Architecture
```
Client                          ThemisDB MCP Server
  |                                      |
  |---(1) HTTP POST /mcp/sse/call------>|  (Tool/Resource request)
  |<-----(2) HTTP 200 + SSE stream------|
  |                                      |
  |<-----(3) data: {...response...}-----|  (JSON-RPC response)
  |<-----(4) : keepalive-----------------|  (Every 30s)
  |<-----(5) data: {...notification.}---|  (Async events)
  |                                      |
  |---(6) HTTP POST /mcp/sse/disconnect->|  (Cleanup)
  |<-----(7) HTTP 200 OK-----------------|
```

### HTTP Endpoints

#### Subscribe to MCP via SSE
```http
GET /mcp/sse/subscribe?client_id=<uuid> HTTP/1.1
Host: localhost:8080
Accept: text/event-stream

HTTP/1.1 200 OK
Content-Type: text/event-stream
Cache-Control: no-cache
Connection: keep-alive

: connected

data: {"jsonrpc":"2.0","method":"initialized","params":{}}

: keepalive

data: {"jsonrpc":"2.0","result":{"..."},"id":1}
```

#### Send Request to MCP
```http
POST /mcp/sse/request HTTP/1.1
Host: localhost:8080
Content-Type: application/json

{
  "client_id": "550e8400-e29b-41d4-a716-446655440000",
  "request": {
    "jsonrpc": "2.0",
    "method": "tools/call",
    "params": {
      "name": "get_entity",
      "arguments": {"key": "user:123"}
    },
    "id": 1
  }
}
```

#### Disconnect
```http
POST /mcp/sse/disconnect HTTP/1.1
Host: localhost:8080
Content-Type: application/json

{
  "client_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

### Configuration
```cpp
McpServer::Config config;
config.enable_sse = true;
config.sse_keepalive_ms = 30000;  // Send keepalive every 30s

McpServer server(io_context, config);
server.start();

// Attach to HTTP server for endpoint handling
server.attachHttpServer(http_server);
```

### JavaScript Client Example
```javascript
const clientId = crypto.randomUUID();

// Subscribe to SSE stream
const eventSource = new EventSource(`/mcp/sse/subscribe?client_id=${clientId}`);

eventSource.onmessage = (event) => {
  const response = JSON.parse(event.data);
  console.log('Received:', response);
};

eventSource.onerror = (error) => {
  console.error('SSE error:', error);
  eventSource.close();
};

// Send request
async function callTool(name, args) {
  const response = await fetch('/mcp/sse/request', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({
      client_id: clientId,
      request: {
        jsonrpc: '2.0',
        method: 'tools/call',
        params: { name, arguments: args },
        id: Date.now()
      }
    })
  });
  
  return response.json();
}

// Usage
await callTool('get_entity', {key: 'user:123'});

// Cleanup
eventSource.close();
await fetch('/mcp/sse/disconnect', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({client_id: clientId})
});
```

### Characteristics
- **Protocol**: JSON-RPC 2.0 over SSE + HTTP POST
- **Framing**: SSE event stream format (`data:` prefix, double newline)
- **Keepalive**: Comment lines (`: keepalive\n\n`) every 30 seconds
- **Multiplexing**: Supports multiple clients via client_id
- **Reconnect**: Automatic via EventSource API
- **Buffering**: Server buffers events per client until consumed

## 3. WebSocket Transport

### Description
Full-duplex bidirectional communication over persistent WebSocket connection.

### Architecture
```
Client                          ThemisDB MCP Server
  |                                      |
  |--(1) HTTP Upgrade to WebSocket----->|
  |<----(2) 101 Switching Protocols-----|
  |                                      |
  |<===(3) WebSocket Connection Open====|
  |                                      |
  |---(4) {"method":"tools/call",...}--->|  (Request)
  |<-----(5) {"result":{...},"id":1}-----|  (Response)
  |                                      |
  |<-----(6) {"method":"ping",...}-------|  (Ping every 30s)
  |---(7) {"method":"pong",...}--------->|  (Pong response)
  |                                      |
  |<-----(8) {"method":"notification".}--|  (Async notification)
  |                                      |
  |--(9) Close frame--------------------->|
  |<---(10) Close frame ACK---------------|
```

### WebSocket Endpoint

#### Connect
```http
GET /mcp/ws HTTP/1.1
Host: localhost:8080
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13

HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

#### Message Exchange
```json
// Client → Server (Request)
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "put_entity",
    "arguments": {
      "key": "user:456",
      "value": {"name": "Bob", "email": "bob@example.com"}
    }
  },
  "id": 2
}

// Server → Client (Response)
{
  "jsonrpc": "2.0",
  "result": {
    "status": "success",
    "message": "Entity stored successfully",
    "key": "user:456"
  },
  "id": 2
}

// Server → Client (Notification)
{
  "jsonrpc": "2.0",
  "method": "notification",
  "params": {
    "type": "cdc_event",
    "key": "user:456",
    "operation": "PUT"
  }
}
```

### Configuration
```cpp
McpServer::Config config;
config.enable_websocket = true;
config.websocket_ping_interval_ms = 30000;  // Ping every 30s

McpServer server(io_context, config);
server.start();

// Attach to HTTP server for WebSocket upgrade handling
server.attachHttpServer(http_server);
```

### JavaScript Client Example
```javascript
const ws = new WebSocket('ws://localhost:8080/mcp/ws');

ws.onopen = () => {
  console.log('MCP WebSocket connected');
  
  // Call a tool
  ws.send(JSON.stringify({
    jsonrpc: '2.0',
    method: 'tools/call',
    params: {
      name: 'get_schema',
      arguments: {}
    },
    id: 1
  }));
};

ws.onmessage = (event) => {
  const message = JSON.parse(event.data);
  
  if (message.method === 'ping') {
    // Respond to ping
    ws.send(JSON.stringify({
      jsonrpc: '2.0',
      method: 'pong',
      params: {}
    }));
  } else if (message.result) {
    // Handle response
    console.log('Response:', message.result);
  } else if (message.method) {
    // Handle notification
    console.log('Notification:', message.params);
  }
};

ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

ws.onclose = () => {
  console.log('WebSocket closed');
};

// Cleanup
ws.close();
```

### Node.js Client Example
```javascript
const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:8080/mcp/ws');

ws.on('open', async () => {
  console.log('Connected to MCP server');
  
  // Helper function to call MCP tools
  const callTool = (name, args) => {
    return new Promise((resolve, reject) => {
      const id = Date.now();
      
      const handler = (data) => {
        const msg = JSON.parse(data);
        if (msg.id === id) {
          ws.off('message', handler);
          if (msg.error) reject(msg.error);
          else resolve(msg.result);
        }
      };
      
      ws.on('message', handler);
      
      ws.send(JSON.stringify({
        jsonrpc: '2.0',
        method: 'tools/call',
        params: { name, arguments: args },
        id
      }));
    });
  };
  
  // Use the helper
  try {
    const result = await callTool('get_entity', {key: 'user:123'});
    console.log('Entity:', result.value);
  } catch (error) {
    console.error('Error:', error);
  }
});

ws.on('message', (data) => {
  const msg = JSON.parse(data);
  
  if (msg.method === 'ping') {
    ws.send(JSON.stringify({
      jsonrpc: '2.0',
      method: 'pong',
      params: {}
    }));
  } else if (msg.method === 'notification') {
    console.log('Notification:', msg.params);
  }
});
```

### Characteristics
- **Protocol**: JSON-RPC 2.0 over WebSocket
- **Framing**: WebSocket text frames (one JSON message per frame)
- **Keepalive**: Ping/pong frames every 30 seconds
- **Multiplexing**: One session per WebSocket connection
- **Reconnect**: Manual (client must reconnect)
- **Message Queue**: Server queues messages per session until delivered

## Transport Selection Guide

### Use stdio when:
- Integrating with Claude Desktop
- Running on Linux/macOS
- Single-user desktop environment
- Minimal latency requirements
- Process-based security model

### Use SSE when:
- Building web dashboards or monitoring UIs
- Server-initiated notifications needed
- Clients are browsers without WebSocket support
- Firewall-friendly unidirectional streaming
- HTTP proxy compatibility required

### Use WebSocket when:
- Building interactive web applications
- Real-time bidirectional communication needed
- Long-lived connections with frequent exchanges
- Lower protocol overhead desired
- Modern browser or Node.js client

## Error Handling

All transports use JSON-RPC 2.0 error responses:

```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32600,
    "message": "Invalid request: missing 'key' parameter",
    "data": {
      "field": "key",
      "provided": null
    }
  },
  "id": 1
}
```

### Error Codes
- `-32700`: Parse error
- `-32600`: Invalid request
- `-32601`: Method not found
- `-32602`: Invalid params
- `-32603`: Internal error
- `-32000` to `-32099`: Server-defined errors

## Security Considerations

### stdio
- ✅ Process isolation (OS-level security)
- ✅ No network exposure
- ⚠️ Requires file system access control
- ⚠️ Vulnerable to local privilege escalation

### SSE
- ⚠️ No built-in authentication (add via HTTP auth)
- ⚠️ CSRF protection needed for POST endpoints
- ✅ TLS encryption supported (HTTPS)
- ✅ CORS configuration available
- ⚠️ Client_id must be cryptographically random (UUIDv4)

### WebSocket
- ⚠️ No built-in authentication (add via handshake)
- ✅ TLS encryption supported (WSS)
- ✅ Origin checking for CSRF protection
- ⚠️ Rate limiting recommended per session
- ✅ Automatic connection limits

### Recommendations
1. Use TLS (HTTPS/WSS) for production SSE/WebSocket
2. Implement authentication middleware for HTTP endpoints
3. Add rate limiting per client_id (SSE) or session (WebSocket)
4. Validate all JSON-RPC requests (parameter types, ranges)
5. Use parameterized queries to prevent Cypher/SQL injection
6. Sanitize user inputs in all MCP tool arguments

## Performance Tuning

### stdio
```cpp
config.stdio_buffer_size = 8192;  // Larger buffer for bulk operations
```

### SSE
```cpp
config.sse_keepalive_ms = 15000;  // More frequent keepalive for aggressive proxies
// Consider buffering limits per client to prevent memory exhaustion
```

### WebSocket
```cpp
config.websocket_ping_interval_ms = 60000;  // Less frequent ping for stable connections
// Adjust message queue size if handling high-frequency updates
```

## Monitoring & Debugging

### Logging
```cpp
// Enable MCP transport logging
spdlog::set_level(spdlog::level::debug);

// Log output examples:
// [info] MCP Server started successfully
// [debug] MCP SSE client added: 550e8400-e29b-41d4-a716-446655440000, total clients: 3
// [debug] MCP WebSocket message queued for session: ws_abc123
// [trace] MCP SSE keepalive sent to 3 clients
```

### Metrics
Track transport health:
- Active client/session count
- Messages sent/received
- Average latency per transport
- Error rates
- Connection lifetime statistics

## Future Enhancements

### Planned Features
- [ ] Windows stdio support via named pipes
- [ ] Authentication and authorization framework
- [ ] Rate limiting per client/session
- [ ] Message compression (gzip)
- [ ] Binary protocol option for performance
- [ ] Multiplexing multiple MCP servers over single transport
- [ ] Connection pooling for HTTP/2 Server Push

## References

- [Model Context Protocol Specification](https://modelcontextprotocol.io/)
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)
- [Server-Sent Events (SSE) Standard](https://html.spec.whatwg.org/multipage/server-sent-events.html)
- [WebSocket Protocol RFC 6455](https://datatracker.ietf.org/doc/html/rfc6455)
- [MCP Minimal Integration Guide](./MCP_MINIMAL_INTEGRATION.md)
- [MCP Protocol Support](./MCP_PROTOCOL_SUPPORT.md)
