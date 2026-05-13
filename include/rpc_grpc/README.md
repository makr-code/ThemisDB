> **Build (Linux):** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

# rpc_grpc Module Headers

<!-- Status: current | validated: 2026-05-13 | Primary: src/rpc_grpc/ | Secondary: docs/de/rpc_grpc/ -->
<!-- Links: ../../src/rpc_grpc/README.md · ../../src/rpc_grpc/ROADMAP.md · ../../src/rpc_grpc/FUTURE_ENHANCEMENTS.md -->

This include folder currently carries module-facing API documentation. The
runtime headers are source-local in `src/rpc_grpc/` and are typically included
as `rpc_grpc/...` because in-tree builds expose `src/` on the include path.

## Public Entry Points

| Header / Entry Point | Purpose |
|---|---|
| `rpc_grpc/grpc_plugin.h` | Declares `GRPCServer` (`IRPCServer`) and `GRPCPlugin` (`IRPCPlugin`) |
| `rpc_grpc/bidi_stream_adapter.h` | Header-only helper for bidirectional stream handlers |
| `plugins/rpc_plugin_interface.h` | Stable cross-module contract (`RPCServerConfig`, `IRPCServer`, `IRPCPlugin`) |
| `createPlugin()` / `destroyPlugin()` | C ABI plugin lifecycle exports for dynamic loading |

## Public API Surface

### `GRPCPlugin`

- `initialize(config_json)`, `shutdown()`, `createServer()`
- Metadata: `getName()`, `getVersion()`, `getProtocol()`, `getDefaultPort()`,
  `getProtocolDescription()`, `getCapabilities()`

### `GRPCServer`

- Lifecycle: `initialize(config)`, `start()`, `stop()`, `isRunning()`
- Service and stats: `registerService(void*)`, `getAddress()`, `getStats()`, `resetStats()`
- TLS/admin extensions: `reloadTls(cert,key,ca)`, `getAdminAddress()`
- Observability: `setServiceHealth()`, `isServiceHealthy()`, `recordRPC()`,
  `getMetricsText()`, `setAccessLogSink()`, `logAccess()`

### `BidiStreamAdapter<Req, Resp>`

- Handler registration: `onMessage(handler)`
- Stream loop: `run()`
- Outbound flow control: `write(response)` with bounded queue backpressure
- Finalization/introspection: `finish(status)`, `isFinished()`, `queueDepth()`, `finishStatus()`

## Runtime Configuration Keys

`GRPCServer` reads these `RPCServerConfig` fields and protocol-specific keys:

| Key | Type | Default | Behavior |
|---|---|---|---|
| `host` | string | `0.0.0.0` | Bind address for all listeners |
| `port` | uint16 | `0` | Main listen port (`GRPCPlugin::getDefaultPort()` is `50051`) |
| `tls_enabled` | bool | `false` | Enables TLS credential setup |
| `tls_cert_path` / `tls_key_path` / `tls_ca_cert_path` | string | empty | PEM file inputs for TLS/mTLS credential creation |
| `auth_required` | bool | `true` | mTLS mode (`true`) vs server-only TLS (`false`) |
| `extra_config["keepalive_time_ms"]` | integer-as-string | unset | gRPC keepalive interval argument |
| `extra_config["keepalive_timeout_ms"]` | integer-as-string | unset | gRPC keepalive timeout argument |
| `extra_config["admin_port"]` | integer-as-string | unset | Optional extra insecure admin listener |

## Runtime Behavior, Error Cases, and Limits

- TLS startup is fail-closed: invalid cert/key/CA input aborts startup and does
  not silently downgrade to insecure credentials.
- If TLS is disabled, the module logs an explicit security warning and runs
  insecure transport.
- Message-size limit is 100 MiB for both receive and send.
- `reloadTls()` updates credentials for future connections only; existing
  sessions continue with already negotiated parameters.
- `recordRPC()`-based metrics are manual instrumentation hooks; metrics are empty
  until at least one RPC is recorded.
- Access logs are sink-driven JSON lines; no sink means no log emission.

## Usage

```cpp
#include "rpc_grpc/grpc_plugin.h"

using namespace themis::plugins::rpc::grpc_plugin;

GRPCPlugin plugin;
auto server = plugin.createServer();

RPCServerConfig cfg;
cfg.host = "127.0.0.1";
cfg.port = plugin.getDefaultPort();
server->initialize(cfg);
server->start();
server->stop();
```

## Installation

For in-tree targets, make sure both include roots are available:

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}
    ${THEMISDB_SOURCE_DIR}/src
)
```

## Troubleshooting

- **Plugin loads but server start fails:** verify config values and TLS paths,
  and inspect stderr for fail-closed credential errors.
- **`getAdminAddress()` is empty:** set `extra_config["admin_port"]` to a valid
  numeric port in the range 1..65535 before `start()`.
- **`write()` blocks in `BidiStreamAdapter`:** consumer is applying backpressure;
  increase queue depth or drain responses faster.

## See Also

- [`../../src/rpc_grpc/README.md`](../../src/rpc_grpc/README.md) — implementation overview
- [`../../src/rpc_grpc/ARCHITECTURE.md`](../../src/rpc_grpc/ARCHITECTURE.md) — architecture and flow
- [`../../src/rpc_grpc/ROADMAP.md`](../../src/rpc_grpc/ROADMAP.md) — phased delivery status
- [`../../src/rpc_grpc/FUTURE_ENHANCEMENTS.md`](../../src/rpc_grpc/FUTURE_ENHANCEMENTS.md) — planned feature work
- [`../../src/rpc_grpc/SECURITY.md`](../../src/rpc_grpc/SECURITY.md) — security model
- [`../../docs/de/rpc_grpc/README.md`](../../docs/de/rpc_grpc/README.md) — German secondary overview
