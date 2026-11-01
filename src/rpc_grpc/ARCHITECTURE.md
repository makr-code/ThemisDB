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

# RPC gRPC Module Architecture

## Boundary and build flow

`src/rpc_grpc` is the runtime-loadable gRPC RPC plugin. It implements
`IRPCPlugin` and `IRPCServer` from
[`include/plugins/rpc_plugin_interface.h`](../../include/plugins/rpc_plugin_interface.h).
It is separate from the built-in API gRPC server in `src/api`.

When `THEMIS_PLUGIN_RPC_GRPC` is enabled and CMake finds gRPC, the module builds
the `themis_rpc_grpc` shared library and plugin manifest. Protobuf is required
after gRPC is found; reflection is linked only if its imported target exists.
The plugin runtime itself is implemented in `grpc_plugin.cpp` and declared in
`grpc_plugin.h`. `bidi_stream_adapter.h` is a source-local stream helper.

## Runtime flow

1. The host creates `GRPCPlugin` and obtains a `GRPCServer` through
   `createServer()`. `GRPCPlugin::initialize()` currently ignores its JSON
   argument; server settings are passed separately through `RPCServerConfig`.
2. `GRPCServer::initialize()` validates the host and, when TLS is enabled, the
   certificate, key, and CA file paths. It stores `host:port`; port zero is
   permitted for an OS-assigned port.
3. Before `start()`, the host registers externally owned `grpc::Service`
   pointers. The server rejects null/late registrations and treats duplicate
   pointers as a no-op; it imposes no count limit.
4. `start()` builds the listener, applies supported keepalive arguments and
   100 MiB send/receive limits, registers services, and marks the server running
   only after `BuildAndStart()` succeeds. With no services, it adds an idle
   completion queue.
5. `stop()` marks tracked health entries not-serving, invokes gRPC shutdown,
   drains the idle completion queue when present, and clears the running flag.

Lifecycle methods and registration are not synchronized as a group; serialize
them at the host boundary.

## Credentials and service plane

TLS mode loads the configured PEM files and creates server credentials.
`auth_required` controls whether a client certificate is required; it does not
provide application-level authorization. With TLS disabled, the implementation
uses insecure credentials and emits a warning. A valid `admin_port` adds a
second listener with insecure credentials even when the main listener uses TLS.

`reloadTls()` validates replacement credentials before stopping the listener,
then drains and restarts it. This causes downtime. If the replacement listener
fails to start, the prior configuration and credentials are used for a rollback
attempt; if that also fails, the server remains stopped.

## Streaming and observability

`BidiStreamAdapter` wraps a non-owning stream pointer. Its `run()` loop reads
messages and invokes an optional handler; `write()` synchronously drains a
bounded response queue. `finish()` releases queue waiters but does not interrupt
an already-blocked gRPC read/write. Callers remain responsible for cancellation
and stream lifetime.

`recordRPC()` updates method counters and aggregate server totals and invokes
the optional access-log sink. The module does not attach an automatic request
interceptor or expose an HTTP metrics endpoint. Health state is an in-memory
map, not a remote gRPC health service.

The public `CircuitBreaker` helper is separate from `GRPCServer` and must be
called explicitly. `RpcServiceDescriptor` is descriptive metadata not consumed
by `registerService()`.

## Tests and benchmarks

Focused tests under `tests/rpc_grpc` cover contract types, the circuit breaker,
plugin/server lifecycle, and a test-local stress simulation. Stream-adapter and
observability tests are also registered from `tests/CMakeLists.txt`; the
integration test requires gRPC. The module benchmarks are listed in
[`../../benchmarks/rpc_grpc/README.md`](../../benchmarks/rpc_grpc/README.md);
their source definitions are not evidence of measured production performance.

See the [API reference](API.md), [implementation notes](IMPLEMENTATION.md),
[module guide](MODULE_GUIDE.md), and [trust boundary](TRUST.md).
