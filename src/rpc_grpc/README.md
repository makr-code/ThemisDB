# ThemisDB RPC gRPC Module

<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The rpc_grpc module provides the gRPC-backed RPC plugin runtime for ThemisDB, including server lifecycle management, TLS/mTLS credential handling, service registration, bidirectional stream adapter utilities, and module-local observability hooks.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| grpc_plugin.cpp | gRPC server lifecycle, credentials, metrics/access-log hooks |
| grpc_plugin.h | public plugin/server declarations and entry contracts |
| bidi_stream_adapter.h | typed helper for bidirectional stream handler integration |
| CMakeLists.txt | build integration for gRPC/protobuf dependencies |

## Scope

In scope:
- gRPC RPC plugin lifecycle and runtime server management
- TLS/mTLS configuration and reload behavior
- service registration and streaming helper support
- module-local RPC metrics/access logging surfaces

Out of scope:
- higher-level business RPC semantics outside plugin contracts
- non-gRPC transport backend ownership
- global auth policy ownership outside module boundaries

## Runtime Behavior and Limits

- startup behavior is explicit and fail-closed for invalid TLS material.
- server lifecycle transitions (init/start/stop) are deterministic.
- service registration and stream adapter paths expose explicit outcomes.
- observability hooks provide method-level RPC counters/log surfaces.

## Sourcecode Verification (Module: rpc_grpc/readme)

- Verified files:
  - src/rpc_grpc/grpc_plugin.cpp
  - src/rpc_grpc/grpc_plugin.h
  - src/rpc_grpc/bidi_stream_adapter.h
  - src/rpc_grpc/CMakeLists.txt
- Verified behavior surfaces:
  - server lifecycle, credential management, registration, stream helper, observability
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md