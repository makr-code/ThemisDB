> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: ../../include/rpc_grpc/README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB gRPC RPC Plugin

**Version:** 0.3.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-05-13
**Module Path:** `src/rpc_grpc/`
**Namespace:** `themis::plugins::rpc::grpc_plugin`
**Default Port:** `50051`

---

## Module Purpose

The `rpc_grpc` module provides the gRPC-based `IRPCPlugin` backend for ThemisDB.
It implements transport lifecycle management (`GRPCServer`), TLS/mTLS credential
setup with fail-closed behavior, optional admin-port binding, per-method metrics,
structured access logging, and the `BidiStreamAdapter` helper for bidirectional
streaming handlers.

## Main Components

| File | Role |
|---|---|
| `grpc_plugin.h` | Public declarations for `GRPCServer` and `GRPCPlugin` |
| `grpc_plugin.cpp` | Server lifecycle, credential setup, reload, metrics, access-log implementation |
| `bidi_stream_adapter.h` | Header-only typed helper for bidirectional stream handlers |
| `CMakeLists.txt` | Module build integration with gRPC/protobuf toolchain |

## Public API & Entry Points

- Public API overview: [`../../include/rpc_grpc/README.md`](../../include/rpc_grpc/README.md)
- Primary entry header: [`grpc_plugin.h`](./grpc_plugin.h)
- Streaming helper header: [`bidi_stream_adapter.h`](./bidi_stream_adapter.h)
- Dynamic plugin exports in `grpc_plugin.cpp`:
  - `extern "C" themis::plugins::IThemisPlugin* createPlugin()`
  - `extern "C" void destroyPlugin(themis::plugins::IThemisPlugin*)`

## Configuration Options

`GRPCServer::initialize(const RPCServerConfig&)` consumes `RPCServerConfig` from
[`include/plugins/rpc_plugin_interface.h`](../../include/plugins/rpc_plugin_interface.h).
The following keys are directly used by this module:

| Key | Type | Default | Runtime behavior |
|---|---|---|---|
| `host` | string | `0.0.0.0` | Bind address for `server_address_` |
| `port` | uint16 | `0` | Main listen port; plugin default is `50051` |
| `tls_enabled` | bool | `false` | Selects TLS credentials vs insecure credentials |
| `tls_cert_path` | string | empty | Server certificate PEM path when TLS is enabled |
| `tls_key_path` | string | empty | Server private key PEM path when TLS is enabled |
| `tls_ca_cert_path` | string | empty | CA PEM path for TLS trust and mTLS verification |
| `auth_required` | bool | `true` | Enables mTLS client-certificate requirement when TLS is enabled |
| `extra_config["keepalive_time_ms"]` | integer-as-string | unset | Sets `GRPC_ARG_KEEPALIVE_TIME_MS` when parseable |
| `extra_config["keepalive_timeout_ms"]` | integer-as-string | unset | Sets `GRPC_ARG_KEEPALIVE_TIMEOUT_MS` when parseable |
| `extra_config["admin_port"]` | integer-as-string | unset | Adds secondary insecure listener on `<host>:<admin_port>` when 1..65535 |

## Runtime Behavior, Error Cases, and Limits

- `start()` is non-blocking and returns `false` if called while already running.
- If TLS is enabled and certificate files cannot be loaded, startup fails
  (fail-closed): the server does **not** fall back to insecure mode.
- If TLS is disabled, the module logs a security warning and uses
  `grpc::InsecureServerCredentials()`.
- `registerService(nullptr)` is rejected with a log message and no registration.
- If no services are registered, the module creates an idle completion queue so
  `BuildAndStart()` can still succeed.
- Message-size limits are set to 100 MiB receive and 100 MiB send.
- `reloadTls(cert,key,ca)` works only when server is running with TLS enabled;
  invalid files keep old credentials active and return `false`.
- `setServiceHealth()` / `isServiceHealthy()` track health state in-process; they
  do not expose the standard `grpc.health.v1.Health` service endpoint yet.
- `recordRPC()` updates in-memory per-method counters and emits access-log JSON
  through the configured sink.

## Usage Snippets

### Basic server lifecycle with TLS/mTLS

```cpp
#include "rpc_grpc/grpc_plugin.h"

using namespace themis::plugins::rpc::grpc_plugin;

GRPCPlugin plugin;
plugin.initialize(nullptr);

auto server = plugin.createServer();

RPCServerConfig cfg;
cfg.host = "0.0.0.0";
cfg.port = 50051;
cfg.tls_enabled = true;
cfg.tls_cert_path = "/certs/server.crt";
cfg.tls_key_path = "/certs/server.key";
cfg.tls_ca_cert_path = "/certs/ca.crt";
cfg.auth_required = true; // mTLS
cfg.extra_config["keepalive_time_ms"] = "30000";

server->initialize(cfg);
server->registerService(static_cast<void*>(my_grpc_service));

if (server->start()) {
    auto stats = server->getStats();
    server->stop();
}
```

### Observability hooks

```cpp
auto* grpc_server = dynamic_cast<GRPCServer*>(server.get());
if (grpc_server != nullptr) {
    grpc_server->setAccessLogSink([](const std::string& line) {
        std::cerr << line << '\n';
    });

    grpc_server->recordRPC("/pkg.Service/Method", true, 7);
    std::string prom = grpc_server->getMetricsText();
}
```

## Installation

This module is built with ThemisDB. In-tree consumers that include
`rpc_grpc/grpc_plugin.h` should expose both repository include roots:

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}
    ${THEMISDB_SOURCE_DIR}/src
)
```

## Troubleshooting

- **`start()` returns `false` with TLS enabled:** verify certificate/key/CA files
  exist and are readable; TLS startup is fail-closed on load/parse errors.
- **No metrics output from `getMetricsText()`:** call `recordRPC()` first; export
  is empty while no method counters exist.
- **`reloadTls()` returns `false`:** ensure the server is already running and was
  initialized with `tls_enabled=true`.
- **Unexpected insecure admin listener warning:** remove `extra_config["admin_port"]`
  or terminate insecure admin traffic externally.

## See Also

- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — module architecture and data flow
- [`SECURITY.md`](./SECURITY.md) — threat model and security controls
- [`ROADMAP.md`](./ROADMAP.md) — implementation phases and delivery status
- [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — planned enhancements
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) — benchmark targets
- [`../../include/rpc_grpc/README.md`](../../include/rpc_grpc/README.md) — public API and include-surface guide
- [`../../docs/de/rpc_grpc/README.md`](../../docs/de/rpc_grpc/README.md) — secondary module overview (DE)
- [`../../plugins/rpc/README.md`](../../plugins/rpc/README.md) — RPC backend integration overview
