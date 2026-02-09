# ThemisDB Wire Protocol Documentation

**Version:** 1.0  
**Status:** Production Ready

## Overview

The ThemisDB Wire Protocol is a high-performance binary protocol for client-server communication. It provides efficient data serialization, TLS/mTLS security, and support for all ThemisDB operations including CRUD, transactions, queries, and real-time streaming.

## Key Features

- **Binary Protocol:** Efficient binary serialization using Protocol Buffers
- **TLS/mTLS Support:** Secure communication with optional mutual authentication
- **Connection Pooling:** Client-side connection pooling for reduced latency
- **Keep-Alive:** Persistent connections with automatic reconnection
- **Multiplexing:** Multiple concurrent operations over a single connection
- **Streaming:** Bidirectional streaming for large result sets and real-time data

## Architecture

### Protocol Stack

```
┌─────────────────────────────────────┐
│   Application Layer (AQL, CRUD)    │
├─────────────────────────────────────┤
│   Wire Protocol (Message Framing)  │
├─────────────────────────────────────┤
│   TLS/mTLS (Optional)              │
├─────────────────────────────────────┤
│   TCP (Connection Pooling)          │
└─────────────────────────────────────┘
```

### Message Format

All wire protocol messages follow this structure:

```
┌──────────────┬──────────────┬─────────────────────┐
│ Magic (2B)   │ Length (4B)  │ Payload (Variable)  │
└──────────────┴──────────────┴─────────────────────┘
```

- **Magic Bytes (2 bytes):** Protocol identifier `0xDB01` (ThemisDB v1)
- **Length (4 bytes):** Big-endian uint32, length of payload in bytes
- **Payload (variable):** Protocol Buffers encoded message

### Opcodes

The wire protocol supports the following operation codes:

| Opcode | Name | Description |
|--------|------|-------------|
| 0x01 | HANDSHAKE | Initial connection handshake and authentication |
| 0x02 | CREATE | Create a new document/entity |
| 0x03 | READ | Read document(s) by ID |
| 0x04 | UPDATE | Update existing document |
| 0x05 | DELETE | Delete document(s) |
| 0x06 | QUERY | Execute AQL query |
| 0x07 | BEGIN_TX | Begin transaction |
| 0x08 | COMMIT_TX | Commit transaction |
| 0x09 | ROLLBACK_TX | Rollback transaction |
| 0x0A | SUBSCRIBE | Subscribe to real-time updates |
| 0x0B | UNSUBSCRIBE | Unsubscribe from updates |
| 0x0C | PING | Keep-alive ping |
| 0x0D | PONG | Keep-alive response |
| 0xFF | ERROR | Error response |

## Server Setup

### Basic Configuration

Configure the wire protocol server in your `themis.conf`:

```ini
[wire_protocol]
enabled = true
bind_address = 0.0.0.0
port = 8766
max_connections = 1000
idle_timeout = 300
```

### TLS Configuration

To enable TLS for secure communication:

```ini
[wire_protocol]
enabled = true
bind_address = 0.0.0.0
port = 8766

# TLS Configuration
enable_tls = true
tls_cert_path = /path/to/server-cert.pem
tls_key_path = /path/to/server-key.pem
tls_min_version = TLSv1.2  # or TLSv1.3
```

### Mutual TLS (mTLS) Configuration

For mutual authentication where clients must also present certificates:

```ini
[wire_protocol]
enabled = true
bind_address = 0.0.0.0
port = 8766

# mTLS Configuration
enable_tls = true
enable_mtls = true
tls_cert_path = /path/to/server-cert.pem
tls_key_path = /path/to/server-key.pem
tls_ca_cert_path = /path/to/ca-cert.pem
tls_require_client_cert = true
```

### Certificate Generation

Generate self-signed certificates for testing:

```bash
# Generate CA certificate
openssl req -x509 -newkey rsa:4096 -keyout ca-key.pem -out ca-cert.pem -days 365 -nodes

# Generate server certificate
openssl req -newkey rsa:4096 -keyout server-key.pem -out server-req.pem -nodes
openssl x509 -req -in server-req.pem -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out server-cert.pem -days 365

# Generate client certificate (for mTLS)
openssl req -newkey rsa:4096 -keyout client-key.pem -out client-req.pem -nodes
openssl x509 -req -in client-req.pem -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out client-cert.pem -days 365
```

## Client Connection

### Connection Pool Configuration

The Wire Protocol Connection Pool provides client-side connection pooling:

```cpp
#include <network/wire_protocol_connection_pool.h>

using namespace themis::network;

// Configure connection pool
WireProtocolConnectionPool::Config config;
config.min_connections_per_target = 2;
config.max_connections_per_target = 20;
config.idle_timeout = std::chrono::seconds(60);
config.connect_timeout = std::chrono::seconds(5);

// Create pool
WireProtocolConnectionPool pool(config);

// Acquire connection
auto conn = pool.acquireConnection("localhost:8766");

// Use connection for I/O via the underlying socket
auto& socket = conn.socket();
// socket.write_some(boost::asio::buffer(request_bytes));
// socket.read_some(boost::asio::buffer(response_buffer));

// Or access the wrapper directly for SSL-specific operations
if (conn.socketWrapper().is_ssl()) {
    // SSL-specific logic
}

// Connection automatically returned to pool when conn goes out of scope
```

### TLS Client Configuration

To connect with TLS:

```cpp
WireProtocolConnectionPool::Config config;
config.enable_ssl = true;
config.ssl_ca_cert_path = "/path/to/ca-cert.pem";  // Optional, uses system CA by default

WireProtocolConnectionPool pool(config);
auto conn = pool.acquireConnection("localhost:8766");
```

### mTLS Client Configuration

To connect with mutual TLS:

```cpp
WireProtocolConnectionPool::Config config;
config.enable_ssl = true;
config.enable_mtls = true;
config.ssl_cert_path = "/path/to/client-cert.pem";
config.ssl_key_path = "/path/to/client-key.pem";
config.ssl_ca_cert_path = "/path/to/ca-cert.pem";

WireProtocolConnectionPool pool(config);
auto conn = pool.acquireConnection("localhost:8766");
```

## Authentication

### Handshake Protocol

After establishing a connection, clients must perform a handshake:

1. **Client → Server:** Send HANDSHAKE message with protocol version and authentication credentials
2. **Server → Client:** Respond with HANDSHAKE_ACK and session token
3. **Client:** Include session token in subsequent requests

### Authentication Methods

The wire protocol supports multiple authentication methods:

- **Username/Password:** Basic authentication with bcrypt-hashed passwords
- **API Key:** Bearer token authentication
- **JWT:** JSON Web Token authentication
- **Certificate-based:** mTLS certificate authentication (CN as username)

### Example Handshake Message

```protobuf
message HandshakeRequest {
  string protocol_version = 1;  // "1.0"
  string client_name = 2;       // "ThemisDB Client/1.0"
  AuthMethod auth_method = 3;   // USERNAME_PASSWORD, API_KEY, JWT, CERTIFICATE
  string username = 4;          // For USERNAME_PASSWORD
  string password = 5;          // For USERNAME_PASSWORD
  string api_key = 6;           // For API_KEY
  string jwt_token = 7;         // For JWT
}

message HandshakeResponse {
  bool success = 1;
  string session_token = 2;
  string error_message = 3;
  ServerInfo server_info = 4;
}
```

## Message Examples

### CREATE Operation

Request:
```protobuf
message CreateRequest {
  string collection = 1;
  bytes document = 2;           // JSON-encoded document
  string session_token = 3;
}
```

Response:
```protobuf
message CreateResponse {
  bool success = 1;
  string document_id = 2;
  string error_message = 3;
}
```

### QUERY Operation

Request:
```protobuf
message QueryRequest {
  string aql_query = 1;
  map<string, bytes> bind_vars = 2;  // Query parameters
  string session_token = 3;
  int32 batch_size = 4;              // For streaming results
}
```

Response (streaming):
```protobuf
message QueryResponse {
  bool success = 1;
  repeated bytes results = 2;        // Batch of result documents
  bool has_more = 3;                 // More results available
  string cursor_id = 4;              // For fetching next batch
  string error_message = 5;
  QueryStats stats = 6;
}
```

## Performance Considerations

### Connection Pooling Benefits

- **Reduced Latency:** Avoid TCP 3-way handshake overhead (~1-2ms per connection)
- **TLS Handshake Reuse:** Avoid expensive TLS negotiation (~10-50ms)
- **Memory Efficiency:** Reuse connection buffers and state
- **Scalability:** Handle thousands of concurrent operations with fewer connections

### Recommended Settings

For typical workloads:
- `min_connections_per_target`: 2-5
- `max_connections_per_target`: 10-50 (based on concurrency)
- `idle_timeout`: 30-300 seconds
- `keepalive_interval`: 15-60 seconds

For high-throughput workloads:
- `min_connections_per_target`: 10
- `max_connections_per_target`: 100+
- `idle_timeout`: 300+ seconds
- `keepalive_interval`: 30 seconds

## Security Best Practices

### TLS Configuration

1. **Use TLS 1.2 or higher:** Disable SSLv3, TLSv1.0, TLSv1.1
2. **Strong Cipher Suites:** Use ECDHE with AES-GCM or ChaCha20-Poly1305
3. **Certificate Validation:** Always verify server certificates in production
4. **Certificate Rotation:** Implement automated certificate renewal

### mTLS Configuration

1. **Client Authentication:** Use mTLS for service-to-service communication
2. **Certificate Management:** Use a proper PKI for certificate issuance
3. **Revocation:** Implement CRL or OCSP for certificate revocation
4. **Short-lived Certificates:** Use certificates with shorter validity periods

### Network Security

1. **Firewall Rules:** Restrict wire protocol port to trusted networks
2. **Rate Limiting:** Implement connection and request rate limits
3. **Monitoring:** Log authentication failures and suspicious patterns
4. **Encryption:** Always use TLS in production environments

## Troubleshooting

### Connection Failures

**Problem:** Cannot connect to server

**Solutions:**
1. Check server is running: `netstat -an | grep 8766`
2. Verify firewall rules allow connection
3. Check bind address (0.0.0.0 vs 127.0.0.1)
4. Review server logs for errors

### TLS Handshake Failures

**Problem:** SSL/TLS handshake fails

**Solutions:**
1. Verify certificate paths are correct
2. Check certificate validity: `openssl x509 -in cert.pem -text -noout`
3. Ensure CA certificate is trusted
4. Verify hostname matches certificate CN/SAN
5. Check TLS version compatibility

### Authentication Failures

**Problem:** Handshake rejected by server

**Solutions:**
1. Verify credentials are correct
2. Check authentication method is enabled on server
3. For mTLS, ensure client certificate is signed by trusted CA
4. Review server authentication logs

### Performance Issues

**Problem:** Slow connection acquisition

**Solutions:**
1. Increase `max_connections_per_target`
2. Enable connection warmup: `enable_warmup = true`
3. Adjust `acquire_timeout` if needed
4. Monitor connection pool statistics

## Protocol Buffers Definition

The complete Protocol Buffers definition is available at:
- `proto/themis_wire_v1.proto`

## See Also

- [Performance Tips](knowledge-base/PERFORMANCE_TIPS.md) - Connection pooling optimization
- [TLS Implementation](../TLS_IMPLEMENTATION_VERIFICATION.md) - TLS setup verification
- [Security Guide](../SECURITY.md) - Security best practices
- [API Documentation](de/apis/README.md) - Higher-level API reference

## References

- Protocol Buffers: https://protobuf.dev/
- TLS 1.3 RFC: https://tools.ietf.org/html/rfc8446
- Boost.Asio SSL: https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/ssl__stream.html
