---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
Source_Level: "L1"
SOT_Domain: "api-contract"
Issue: "makr-code/ThemisDB#7145"
Milestone: "DOC-WEEKLY-2026-W44"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC API Reference

## Purpose

This page documents the source-level integration surface for the runtime
loadable gRPC RPC plugin. The primary declarations are
[`grpc_plugin.h`](grpc_plugin.h) and the shared
[`rpc_plugin_interface.h`](../../include/plugins/rpc_plugin_interface.h).
The built-in API gRPC server is a separate component.

[`rpc_grpc_api_contract.h`](../../include/rpc_grpc/rpc_grpc_api_contract.h)
declares a separate `themis::rpc_grpc` contract namespace; it is not the
`IRPCServer` interface implemented by this plugin. Use it as a contract
reference only where its types and behavior are actually connected to a
runtime call path.

## Plugin and server interfaces

| Symbol | Behavior |
|---|---|
| `GRPCPlugin::getName()` / `getVersion()` | Returns `"grpc"` / `"2.0.0"`. |
| `GRPCPlugin::getProtocol()` / `getDefaultPort()` | Returns `RPCProtocol::GRPC` / `50051`. |
| `GRPCPlugin::getCapabilities()` | Reports streaming, batching, and thread-safe capabilities; does not apply generic limits or authorize calls. |
| `GRPCPlugin::initialize(const char*)` | Sets the initialized flag and returns true; currently ignores the JSON argument. |
| `GRPCPlugin::createServer()` | Creates a new `GRPCServer` behind `std::unique_ptr<IRPCServer>`. |
| `createPlugin()` / `destroyPlugin()` | C-linkage plugin factory/destructor defined by `grpc_plugin.cpp`. |
| `GRPCServer::initialize(config)` / `start()` / `stop()` | Configures and controls one gRPC server instance; check `initialize` and `start` results. |
| `registerService(void*)` | Expects a `grpc::Service*`; registration is pre-start and the server does not own the service. |
| `reloadTls(cert, key, ca)` | Returns false on validation/build error. A true result only confirms the new credential object was stored; active-listener replacement is not implemented. |
| `getMetricsText()` / `setAccessLogSink()` | Provides an in-process metrics snapshot and optional callback, not an HTTP endpoint. |

`registerService` has a `void` return type. Null pointers and calls after
start are rejected by diagnostic messages, not an error result. Registering
the identical service pointer twice is a no-op.

The roadmap describes bounded service registration, but the current
`GRPCServer::registerService` implementation has no service-count cap. Treat
that planned/recorded bound as unresolved documentation drift, not an API
guarantee.

## Configuration

The shared `RPCServerConfig` declaration has the following defaults. The
“Applied here” column distinguishes declared API fields from values actually
read in `grpc_plugin.cpp`.

| Field | Declared default | Applied by this plugin |
|---|---:|---|
| `host` | `"0.0.0.0"` | Yes; empty host is rejected. |
| `port` | `0` | Yes; zero is accepted for an OS-assigned port. `getDefaultPort()` does not override it automatically. |
| `tls_enabled` | `false` | Yes; false selects insecure gRPC credentials with a warning. |
| `tls_cert_path`, `tls_key_path`, `tls_ca_cert_path` | empty | Yes when TLS is enabled; all three must be readable. |
| `auth_required` | `true` | Yes; controls client-certificate requirement when TLS is enabled, not general authorization. |
| `max_connections`, `thread_pool_size` | `1000`, `8` | No. |
| `connection_timeout_ms`, `request_timeout_ms` | `30000`, `60000` | No. |
| `namespace_default` | `"default"` | No. |
| `rate_limit_enabled`, `rate_limit_requests_per_sec`, `rate_limit_burst_size` | `false`, `1000`, `100` | No. |
| `compression_enabled` | `false` | No. |
| `extra_config` | empty map | Yes for `keepalive_time_ms`, `keepalive_timeout_ms`, and `admin_port`. |

Keepalive values are strings parsed as integers; invalid strings are ignored.
`admin_port` must parse to an integer in `1..65535`. If accepted, the second
listener uses insecure credentials on the configured host.

## Message size and contract notes

`GRPCServer::start` sets both gRPC send and receive message limits to 100 MiB.
The separately published `rpc_grpc_api_contract.h` declares a 4 MiB receive
constant. This mismatch is a contract/documentation issue: callers must not
infer the live server limit from that constant until the sources are reconciled.

## Stream adapter

[`BidiStreamAdapter`](bidi_stream_adapter.h) is a template over request,
response, and stream types. It accepts a non-owning stream pointer and optional
queue depth (default 100); a null stream throws `std::invalid_argument`.
`onMessage` accepts a `std::function<void(Req&&)>`, `run` reads until stream
EOF/finish, `write` performs synchronous stream writes, and `finish` stores a
gRPC status and releases queue waiters. `write` returns false after finish,
but does not report a failed `Stream::Write`; callers retain cancellation and
lifetime responsibility.

## Observability contract

`recordRPC(method, success, duration_ms)` updates counters and emits an access
log only if a sink is configured. The generated metrics text includes
`grpc_server_requests_total`, `grpc_server_errors_total`,
`grpc_server_latency_ms_total`, and `grpc_server_active_connections`. The
method-level latency value is cumulative milliseconds, not a histogram or
percentile. The source does not expose a network scrape endpoint.

See [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for lifecycle details and
[`TRUST.md`](TRUST.md) for the security boundary.
