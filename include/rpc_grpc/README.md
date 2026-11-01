---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# rpc_grpc Public Headers

## Header layout

The public include directory contains:

| Header | Purpose |
|---|---|
| [`circuit_breaker.h`](circuit_breaker.h) | Standalone, mutex-protected CLOSED/OPEN/HALF_OPEN circuit breaker. It is not automatically used by `GRPCServer`. |
| [`rpc_grpc_api_contract.h`](rpc_grpc_api_contract.h) | `RpcGrpcError`, `RpcServerState`, message-size defaults, and descriptive `RpcServiceDescriptor` metadata. |

The plugin implementation declarations and stream adapter are source-local:
[`grpc_plugin.h`](../../src/rpc_grpc/grpc_plugin.h) and
[`bidi_stream_adapter.h`](../../src/rpc_grpc/bidi_stream_adapter.h). This
directory contains no `.proto` files or generated service stubs. The runtime
implements [`IRPCServer`](../plugins/rpc_plugin_interface.h).

## Contract/runtime boundary

`GRPCServer::start()` consumes
`kDefaultMaxReceiveMessageBytes` and `kDefaultMaxSendMessageBytes` from
`rpc_grpc_api_contract.h` (100 MiB each). `RpcServiceDescriptor` and
`RpcServerState` are declared contract types; they are not consumed as runtime
registration or lifecycle state by `GRPCServer`.

The circuit breaker counts consecutive failures and uses total lifetime call
count for `min_calls_in_window`; it does not implement a time-based sliding
window. Callers must explicitly consult it and record call outcomes.

## Runtime configuration

The plugin uses `RPCServerConfig` from `plugins/rpc_plugin_interface.h`.
Defaults shown here are declared by that shared interface; they are not all
applied by the gRPC plugin.

| Field | Declared default | Runtime behavior |
|---|---|---|
| `host` | `"0.0.0.0"` | Used as the listener host; empty values are rejected. |
| `port` | `0` | Used as configured. Zero requests an OS-assigned port; `getDefaultPort()` returning `50051` does not replace this field. |
| `tls_enabled` | `false` | False selects insecure transport with a warning. |
| `tls_cert_path`, `tls_key_path`, `tls_ca_cert_path` | empty | Required and read when TLS is enabled. |
| `auth_required` | `true` | With TLS enabled, requires a client certificate; it is not application authorization. |
| `extra_config["keepalive_time_ms"]` | unset | Integer string passed to gRPC when valid; invalid values are ignored. |
| `extra_config["keepalive_timeout_ms"]` | unset | Integer string passed to gRPC when valid; invalid values are ignored. |
| `extra_config["admin_port"]` | unset | Valid ports 1–65535 bind a second listener using insecure credentials. Keep it isolated from untrusted networks. |

Other generic fields such as rate limits, connection limits, worker count,
timeouts, namespaces, and compression are not applied by this plugin.

## Lifecycle and observability

- Register externally owned `grpc::Service` objects before `start()`. Null,
  late, or duplicate pointer registration is rejected or treated as a no-op;
  `registerService()` returns `void`, so there is no programmatic error result.
- `reloadTls()` validates replacement material before stopping the listener,
  then drains and restarts it. It causes downtime; failed startup triggers a
  rollback attempt, and failed rollback leaves the server stopped.
- Metrics and access-log output are in-process hooks. `recordRPC()` is manual
  instrumentation; the plugin does not install an automatic interceptor or
  network scrape endpoint.
- `BidiStreamAdapter::write()` may block under backpressure. `finish()` wakes
  queue waiters but does not interrupt an already-blocked gRPC `Read()` or
  `Write()`. The caller owns stream lifetime and cancellation.

## Usage

This insecure loopback example is suitable only for local development:

```cpp
#include "rpc_grpc/grpc_plugin.h"
#include "plugins/rpc_plugin_interface.h"

using namespace themis::plugins::rpc::grpc_plugin;

GRPCPlugin plugin;
auto server = plugin.createServer();

RPCServerConfig config;
config.host = "127.0.0.1";
config.port = 0;
if (!server->initialize(config) || !server->start()) {
    return;
}
server->stop();
```

This only illustrates local lifecycle setup; production deployments should
configure TLS and register services.

## Installation

In-tree compilation needs both the repository `include/` and `src/` include
roots. The shared-library target is conditional on `THEMIS_PLUGIN_RPC_GRPC`
and gRPC being available; Protobuf is required after gRPC is found. The target
installs the plugin library and manifest under `lib/themis/plugins/rpc`.
`src/rpc_grpc/CMakeLists.txt` does not install these public headers.

See the [runtime API](../../src/rpc_grpc/API.md),
[architecture](ARCHITECTURE.md), [implementation notes](../../src/rpc_grpc/IMPLEMENTATION.md),
[tests](../../tests/rpc_grpc/README.md), and
[benchmarks](../../benchmarks/rpc_grpc/README.md).
