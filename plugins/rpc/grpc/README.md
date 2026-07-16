# gRPC Plugin for ThemisDB

**Version:** 1.0.0  
**Release:** v1.3.0  
**Protocol:** gRPC (Google Remote Procedure Call)

---

## Overview

This plugin provides gRPC server functionality for ThemisDB, enabling high-performance RPC communication with:

- **HTTP/2 Multiplexing** - Multiple requests over single connection
- **Protocol Buffers** - Efficient binary serialization
- **Native mTLS Support** - Mutual TLS authentication
- **Bidirectional Streaming** - Client, server, and bidirectional streams
- **Multi-Language** - Clients in C++, Python, Go, Java, JavaScript, etc.

---

## Features

- ✅ Full gRPC server implementation
- ✅ TLS/mTLS configuration
- ✅ Service registration
- ✅ Connection management
- ✅ Statistics tracking
- ✅ Graceful shutdown
- ✅ Plugin manifest (plugin.json)

---

## Building

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt install libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc
```

**macOS:**
```bash
brew install grpc protobuf
```

**vcpkg:**
```bash
vcpkg install grpc protobuf
```

### Build with CMake

```bash
cd /path/to/ThemisDB
mkdir build && cd build

# Enable gRPC plugin
cmake .. -DTHEMIS_BUILD_ENTERPRISE_PLUGINS=ON \
         -DTHEMIS_PLUGIN_RPC_GRPC=ON

cmake --build . --target themis_rpc_grpc
```

### Output

```
build/plugins/rpc/
├── themis_rpc_grpc.so       # Plugin shared library
└── plugin.json              # Plugin manifest
```

---

## Configuration

### Server Configuration

```yaml
# config.yaml
rpc:
  servers:
    grpc:
      enabled: true
      host: "0.0.0.0"
      port: 50051
      
      # TLS Configuration
      tls:
        enabled: true
        cert_path: "./certs/server.crt"
        key_path: "./certs/server.key"
        ca_cert_path: "./certs/ca.crt"
      
      # Connection settings
      max_connections: 1000
      auth_required: true  # Enable mutual TLS
```

### Loading the Plugin

The plugin is auto-loaded by the ThemisDB Plugin Manager if `auto_load: true` in `plugin.json`.

**Manual Loading:**

```cpp
#include "plugins/plugin_manager.h"
#include "plugins/rpc_plugin_interface.h"

auto& pm = themis::plugins::PluginManager::instance();

// Load gRPC plugin
auto* plugin = pm.loadPlugin("grpc");
auto* rpc_plugin = dynamic_cast<themis::plugins::rpc::IRPCPlugin*>(plugin);

// Create and configure server
auto server = rpc_plugin->createServer();

themis::plugins::rpc::RPCServerConfig config;
config.host = "0.0.0.0";
config.port = 50051;
config.tls_enabled = true;
config.tls_cert_path = "./certs/server.crt";
config.tls_key_path = "./certs/server.key";
config.tls_ca_cert_path = "./certs/ca.crt";
config.auth_required = true;  // mTLS

server->initialize(config);

// Register gRPC services
// auto* service = new MyGRPCService();
// server->registerService(service);

server->start();
```

---

## TLS/mTLS Configuration

### Server-Side TLS Only

```cpp
RPCServerConfig config;
config.tls_enabled = true;
config.tls_cert_path = "./server.crt";
config.tls_key_path = "./server.key";
config.auth_required = false;  // No client cert required
```

### Mutual TLS (mTLS)

```cpp
RPCServerConfig config;
config.tls_enabled = true;
config.tls_cert_path = "./server.crt";
config.tls_key_path = "./server.key";
config.tls_ca_cert_path = "./ca.crt";  // CA for verifying clients
config.auth_required = true;           // Require client cert
```

### Insecure (Development Only)

```cpp
RPCServerConfig config;
config.tls_enabled = false;
// WARNING: No encryption, use only for local development
```

---

## Client Examples

### C++ Client

```cpp
#include <grpcpp/grpcpp.h>

// For mTLS
grpc::SslCredentialsOptions ssl_opts;
ssl_opts.pem_cert_chain = readFile("client.crt");
ssl_opts.pem_private_key = readFile("client.key");
ssl_opts.pem_root_certs = readFile("ca.crt");

auto channel = grpc::CreateChannel(
    "localhost:50051",
    grpc::SslCredentials(ssl_opts)
);

// Use the channel with your service stub
```

### Python Client

```python
import grpc

# For mTLS
with open('client.crt', 'rb') as f:
    client_cert = f.read()
with open('client.key', 'rb') as f:
    client_key = f.read()
with open('ca.crt', 'rb') as f:
    ca_cert = f.read()

credentials = grpc.ssl_channel_credentials(
    root_certificates=ca_cert,
    private_key=client_key,
    certificate_chain=client_cert
)

channel = grpc.secure_channel('localhost:50051', credentials)
```

---

## Performance

### Expected Performance

| Metric | HTTP/REST | gRPC | Improvement |
|--------|-----------|------|-------------|
| Latency (p50) | 1.5 ms | 0.3 ms | **5x faster** |
| Latency (p99) | 5 ms | 1 ms | **5x faster** |
| Throughput | 1,000 ops/s | 10,000 ops/s | **10x higher** |
| Bandwidth | ~1.5 KB/req | ~1.1 KB/req | **27% less** |

### Optimizations

- HTTP/2 connection reuse
- Binary Protocol Buffers (vs JSON)
- Header compression (HPACK)
- Multiplexed streams
- Zero-copy operations

---

## Monitoring

### Server Statistics

```cpp
auto stats = server->getStats();

std::cout << "Total Requests: " << stats.total_requests << std::endl;
std::cout << "Active Connections: " << stats.active_connections << std::endl;
std::cout << "Avg Latency: " << stats.avg_latency_ms << " ms" << std::endl;
```

### Metrics (OpenTelemetry)

The plugin tracks:
- `rpc_requests_total{protocol="grpc"}`
- `rpc_request_duration_seconds{protocol="grpc"}`
- `rpc_active_connections{protocol="grpc"}`
- `rpc_bytes_sent/received_total{protocol="grpc"}`

---

## Troubleshooting

### Plugin Not Loading

**Symptom:**
```
Warning: Failed to load plugin: grpc
```

**Solutions:**
1. Check that `themis_rpc_grpc.so` exists in `plugins/rpc/` directory
2. Verify gRPC libraries are installed: `ldconfig -p | grep grpc`
3. Check file permissions
4. Review plugin manifest `plugin.json`

### TLS Handshake Errors

**Symptom:**
```
gRPC error: SSL_ERROR_SSL
```

**Solutions:**
1. Verify certificate paths are correct
2. Check certificate validity: `openssl x509 -in server.crt -text -noout`
3. Ensure CA certificate matches client/server certs
4. For mTLS, verify client cert is signed by CA

### Connection Refused

**Symptom:**
```
Connection refused: localhost:50051
```

**Solutions:**
1. Verify server is running: `netstat -tlnp | grep 50051`
2. Check firewall rules
3. Verify host/port configuration
4. Review server logs for startup errors

---

## Development

### Adding New Services

1. Define service in `.proto` file
2. Generate gRPC code: `protoc --grpc_out=. --cpp_out=. service.proto`
3. Implement service class
4. Register with server:

```cpp
class MyService final : public MyService::Service {
    grpc::Status MyMethod(grpc::ServerContext* context,
                          const Request* request,
                          Response* response) override {
        // Implementation
        return grpc::Status::OK;
    }
};

auto* service = new MyService();
server->registerService(service);
```

---

## References

- [gRPC C++ Documentation](https://grpc.io/docs/languages/cpp/)
- [Protocol Buffers Guide](https://protobuf.dev/)
- [RPC Plugin Architecture](../../../docs/plugins/RPC_PLUGIN_ARCHITECTURE.md)
- [mTLS Inter-Shard Design](../../../docs/plugins/RPC_MTLS_INTER_SHARD.md)

---

**ThemisDB v1.3.0** - gRPC Plugin
