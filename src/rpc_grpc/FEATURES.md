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

# rpc_grpc Features

The module builds an optional, runtime-loadable gRPC implementation of the
shared `IRPCPlugin`/`IRPCServer` interfaces.

## Implemented surfaces

- Server lifecycle: initialize, start, graceful stop, and address/stat access.
- TLS or mTLS credential construction and disruptive TLS reload with rollback
  attempt.
- Pre-start registration of externally owned `grpc::Service` pointers.
- A source-local bidirectional stream adapter with synchronous writes and a
  bounded response queue.
- Explicit health-state, method-metric, and access-log hooks.
- A separate public circuit-breaker helper in `include/rpc_grpc/circuit_breaker.h`.

## Runtime limitations

- Generic `RPCServerConfig` fields for connection limits, timeouts, worker
  counts, rate limits, namespaces, and compression are not applied by this
  plugin.
- Metrics are updated through explicit calls; no automatic request interceptor
  or network scrape endpoint is installed.
- The stream adapter does not own its stream or interrupt an in-flight gRPC
  read/write when `finish()` is called.
- The circuit breaker is not connected automatically to `GRPCServer`.

See [API](API.md), [architecture](ARCHITECTURE.md), and
[performance expectations](PERFORMANCE_EXPECTATIONS.md) for verified behavior
and evidence boundaries.
