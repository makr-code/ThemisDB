> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB gRPC RPC Plugin

**Version:** 0.2.0
**Status:** 🟢 Production-Ready (v0.2.0)
**Last Updated:** 2026-04-15
**Module Path:** `src/rpc_grpc/`
**Namespace:** `themis::plugins::rpc::grpc_plugin`
**Default Port:** 50051

---

## Module Purpose

The gRPC plugin provides a high-performance RPC transport for ThemisDB using the gRPC
framework with HTTP/2 multiplexing, Protocol Buffers serialisation, mutual TLS (mTLS),
and bidirectional streaming. It implements two interfaces:

- **`GRPCServer`** — implements `IRPCServer`; manages the gRPC `grpc::Server` lifecycle.
- **`GRPCPlugin`** — implements `IRPCPlugin` + `IThemisPlugin`; factory for `GRPCServer`
  instances and plugin entry point.

The plugin uses a **fail-closed TLS configuration**: if TLS is enabled and certificate
loading fails, the server refuses to start rather than falling back to insecure transport.

---

## Component Table

| File | Class / Role |
|------|-------------|
| `grpc_plugin.h` | `GRPCServer` + `GRPCPlugin` declarations; v0.2.0 keepalive, multi-port, TLS reload extensions |
| `grpc_plugin.cpp` | Full implementation: server lifecycle, mTLS, keepalive tuning, multi-port binding, TLS hot-reload |
| `bidi_stream_adapter.h` | Header-only `BidiStreamAdapter<Req,Resp>` bidirectional streaming helper |
| `CMakeLists.txt` | Build configuration; links gRPC++ and protobuf |

---

## Quick-Start Example

```cpp
#include "grpc_plugin.h"
using namespace themis::plugins::rpc::grpc_plugin;

// 1. Instantiate the plugin
GRPCPlugin plugin;
plugin.initialize(nullptr);  // no extra config needed

// 2. Create a server
auto server = plugin.createServer();

// 3. Configure
RPCServerConfig config;
config.host = "0.0.0.0";
config.port = 50051;
config.tls_enabled = true;
config.tls_cert_path = "/certs/server.crt";
config.tls_key_path  = "/certs/server.key";
config.tls_ca_cert_path = "/certs/ca.crt";
config.auth_required = true;  // enables mTLS client cert verification

server->initialize(config);

// 4. Register a protobuf service implementation
server->registerService(static_cast<grpc::Service*>(&my_service_impl));

// 5. Start
server->start();  // non-blocking; gRPC runs its own thread pool

// 6. Stats
auto stats = server->getStats();
// stats.total_requests, stats.active_connections, etc.

// 7. Stop
server->stop();
```

---

## TLS Modes

| Mode | `tls_enabled` | `auth_required` | Behaviour |
|------|--------------|-----------------|-----------|
| Insecure | `false` | — | `InsecureServerCredentials()` — development only |
| TLS | `true` | `false` | Server certificate only; no client cert |
| mTLS | `true` | `true` | `GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY` |

---

## Plugin Registration

```cpp
// C export for dynamic loading
extern "C" {
  IThemisPlugin* createPlugin();   // returns new GRPCPlugin()
  void destroyPlugin(IThemisPlugin*);
}
```

---

## See Also

- `ARCHITECTURE.md` — component diagram, TLS configuration flow
- `SECURITY.md` — fail-closed TLS design, threat model
- `ROADMAP.md` — implementation phases and feature backlog

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/rpc_grpc/README.md`](../../include/rpc_grpc/README.md) for the public API.
