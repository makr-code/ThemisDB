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

# RPC gRPC Public Headers — Architecture

## Boundary

`include/rpc_grpc/` contains two headers:

- [`rpc_grpc_api_contract.h`](rpc_grpc_api_contract.h) defines contract enums,
  shared defaults, and `RpcServiceDescriptor`.
- [`circuit_breaker.h`](circuit_breaker.h) defines a standalone,
  mutex-protected circuit breaker.

Neither header defines gRPC service implementations or generated Protocol
Buffers stubs. The runtime plugin is implemented in
[`../../src/rpc_grpc/`](../../src/rpc_grpc/); it implements `IRPCServer` from
[`../plugins/rpc_plugin_interface.h`](../plugins/rpc_plugin_interface.h).
The built-in API gRPC server in `src/api` is a separate component.

## Runtime relationship

The plugin's `GRPCServer::start()` consumes the 100 MiB send/receive limits from
`rpc_grpc_api_contract.h`. The other contract types are not a replacement for
the `IRPCServer` interface, and their presence does not imply that every
declared invariant is enforced by the runtime. In particular,
`RpcServiceDescriptor` is not consumed by `GRPCServer::registerService()`.

The `CircuitBreaker` is not automatically attached to the plugin or server.
Callers must explicitly use `allowRequest()` and `recordResult()`. Its minimum
call guard uses lifetime call count, not a time-based sliding window.

## Build and verification

The plugin target is conditional on `THEMIS_PLUGIN_RPC_GRPC` and an available
gRPC package; Protobuf is required after gRPC is found. The public headers are
also covered by focused tests registered under `tests/rpc_grpc`.

See the [module README](../../src/rpc_grpc/README.md),
[API reference](../../src/rpc_grpc/API.md), and
[build/test guide](../../tests/rpc_grpc/README.md).
