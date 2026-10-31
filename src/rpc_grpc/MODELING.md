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

# RPC gRPC Runtime Model

## Purpose

This page models the implemented state and request flow in `src/rpc_grpc`.
It is for developers changing lifecycle, credentials, streaming, or observability
behavior. It describes the implementation, not a proposal for a separate RPC
business-service model.

## Server lifecycle

`GRPCServer` stores a running flag and a gRPC server object; the current source
does not use the four-state `RpcServerState` enum from
[`rpc_grpc_api_contract.h`](../../include/rpc_grpc/rpc_grpc_api_contract.h).
The effective flow is:

```text
construct -> initialize(config) -> start() -> running
                                  -> stop() -> not running
```

- `initialize` rejects an empty host. If TLS is enabled, it also requires
  non-empty, readable certificate, key, and CA files. It builds the address
  from the configured host and port; port `0` is permitted for an
  OS-assigned port.
- `start` rejects a second start, prepares credentials, adds listeners and
  services, then sets `running` only when `BuildAndStart()` returns a server.
- `stop` is a no-op with a warning when no server is running. Otherwise it
  marks tracked service-health entries not-serving, calls gRPC shutdown, drains
  an idle completion queue if one exists, and clears `running`.

Call lifecycle and registration methods serially from the plugin host.
`registerService` has no internal lock, accepts a `grpc::Service*` through a
`void*`, and must be called before `start`.

## Credentials and listener model

The main listener uses TLS credentials when `RPCServerConfig::tls_enabled` is
true. The `auth_required` flag controls whether the TLS credential requests and
requires a client certificate; it is not a general application authorization
policy. When TLS is false, the source explicitly returns
`grpc::InsecureServerCredentials()` and emits a warning. A configured
`admin_port` adds a second listener using insecure credentials, regardless of
the main listener's TLS setting.

The optional admin listener is not an HTTP endpoint. The implementation
registers configured gRPC services on the server builder; it does not register
an HTTP `/health` or `/metrics` handler in this module.

## Request, stream, and observability flows

- Services are registered before start. A null pointer is rejected with a
  diagnostic; re-registering the same pointer is treated as an idempotent
  no-op. The server does not own the service objects.
- `BidiStreamAdapter` reads until gRPC `Read` returns false or `finish` sets
  its finished flag. It passes moved request objects to the optional handler.
  `write` queues a response and immediately drains the queue through
  synchronous `Write` calls.
- `recordRPC` updates method-level counters and server totals, then asks the
  configured access-log sink to receive a JSON record. No automatic interceptor
  connection is present in `grpc_plugin.cpp`.
- Health state is an in-memory map. `isServiceHealthy` returns true for a
  service name that has not been recorded; this is not a remote health-check
  service.

## Separate circuit-breaker helper

[`circuit_breaker.h`](../../include/rpc_grpc/circuit_breaker.h) provides a
thread-safe CLOSED/OPEN/HALF_OPEN helper. It is not invoked by
`GRPCServer` in `grpc_plugin.cpp`; consumers must explicitly call
`allowRequest()` and `recordResult()`. Its implementation counts consecutive
failures and compares total lifetime calls with `min_calls_in_window`; it does
not maintain a time-based sliding window.

## Contract and implementation boundary

The API contract header is useful for intended invariants, but it must not be
mistaken for proof that each invariant is wired into the runtime. For example,
the contract header and `GRPCServer::start` now share 100 MiB send and receive
limits. The runtime consumes these constants directly; update the shared
defaults and boundary test together if the limit changes.

`registerService` rejects null pointers, late registration, and duplicate
pointers, but does not enforce a service-count limit. This distinction is
reflected in the roadmap; the high-cardinality stress fixture uses a separate
local registry and is not evidence of a plugin registration cap.

See [`ARCHITECTURE.md`](ARCHITECTURE.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md),
and [`TRUST.md`](TRUST.md) for related boundaries.
