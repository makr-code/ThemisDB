---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
Issue: "makr-code/ThemisDB#7145"
Milestone: "DOC-WEEKLY-2026-W44"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC Module Guide

## Audience and boundaries

Use this guide when adding a gRPC service to the runtime-loadable
`src/rpc_grpc` plugin. The module owns transport setup, server lifecycle,
credential construction, service registration, and helper observability. It
does not define business RPCs, process plugin discovery, or implement the
separate built-in API gRPC server.

## Integration checklist

1. **Build the plugin.** Configure `THEMIS_PLUGIN_RPC_GRPC=ON` and ensure CMake
   finds gRPC and Protobuf. The module is omitted if the flag is off or gRPC is
   unavailable.
2. **Implement a gRPC service.** Keep the service object alive until after the
   server has stopped. `GRPCServer` stores a raw service pointer and does not
   transfer ownership.
3. **Create and configure the server.** Obtain `IRPCServer` from
   `GRPCPlugin::createServer()`, fill `RPCServerConfig`, and check
   `initialize(config)`. Choose a concrete bind host and port explicitly;
   interface defaults are `0.0.0.0` and port `0`.
4. **Choose transport security.** Set `tls_enabled=true` and provide readable
   certificate, key, and CA paths. `auth_required=true` requires a client
   certificate; it is not a substitute for authorization in the service. Do
   not use the insecure listener for untrusted traffic.
5. **Register before start.** Call `registerService(service.get())` before
   `start()`. The method returns no status, so use logs and tests to confirm
   registration. A null pointer is ignored with an error message; duplicate
   pointer registration is a no-op.
6. **Check start and serve.** `start()` returns false if the server is already
   running, credential configuration fails, a null service is present, or
   `BuildAndStart()` fails. With no services it can still create an idle
   completion queue; an empty service set is warned about, not rejected.
7. **Stop before destruction.** Call `stop()` and allow gRPC shutdown to
   complete before destroying registered service instances. Shutdown has no
   module-configurable deadline.

## Configuration fields to avoid assuming

Although `RPCServerConfig` declares connection limits, thread-pool size,
timeouts, namespaces, rate limiting, and compression, the current gRPC
implementation does not apply those fields. Only the listener, TLS, client
certificate, and supported `extra_config` values described in
[`API.md`](API.md) affect this runtime.

`GRPCPlugin::initialize(config_json)` does not parse or apply JSON. Configure
the `GRPCServer` through `RPCServerConfig`; do not treat the plugin manifest's
JSON schema as proof of runtime configuration.

## Streaming helper

Use [`BidiStreamAdapter`](bidi_stream_adapter.h) only while the underlying
stream remains alive. It does not own the stream. Install the message handler
before `run()`. `write()` is synchronous and may block; the adapter does not
propagate `Stream::Write` failure. Call `finish(status)` to stop its read loop
and wake response writers; separately coordinate cancellation and worker
lifetimes.

## Validation path

- Run the existing lifecycle and contract-hardening tests after changing server
  setup.
- Run the stream adapter tests when changing read/write/finish behavior.
- Run integration/stress tests for concurrency or long-lived changes.
- For performance-sensitive changes, use the benchmark mapping in
  [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

The test targets are conditional on the gRPC build option. See
[`RESEARCH.md`](RESEARCH.md) for evidence boundaries and
[`EMBEDDING.md`](EMBEDDING.md) for host integration.
