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

# ThemisDB RPC gRPC Module

<!-- Status: review | source-verified: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · RESEARCH.md · MODELING.md · EMBEDDING.md · IMPLEMENTATION.md · API.md · MODULE_GUIDE.md · RUNBOOK.md · OPERATIONS.md · TRUST.md -->

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

- when TLS is enabled, missing or unreadable TLS files reject initialization
  and credential setup failures reject startup; when TLS is disabled, the
  implementation explicitly selects insecure credentials.
- TLS reload drains and restarts the listener with validated replacement
  credentials; this causes downtime and attempts to restore the prior
  configuration if the new listener fails to start.
- server lifecycle transitions (init/start/stop) are deterministic.
- service registration rejects null, late, and duplicate pointers but has no
  service-count cap. Rejections are reported through diagnostics because the
  interface returns void; stream adapter paths expose explicit outcomes.
- observability helpers provide method-level counters and an optional log sink;
  they do not install an HTTP metrics endpoint or automatic request interceptor.

## Build and installation

The shared-library target is added only when `THEMIS_PLUGIN_RPC_GRPC` is
enabled and CMake finds gRPC; Protobuf is required after that. Build target:
`themis_rpc_grpc`. The module installs the plugin library and generated
`plugin.json` under `lib/themis/plugins/rpc`. Its CMake file does not install
the public headers.

## Usage and validation

The plugin creates a server through `GRPCPlugin::createServer()`. Configure the
returned `IRPCServer` with `RPCServerConfig`, register service objects before
starting, and check both `initialize()` and `start()` results. See
[`MODULE_GUIDE.md`](MODULE_GUIDE.md) for ownership, TLS, and lifecycle
requirements.

Focused tests and benchmark source boundaries are documented in
[`../../tests/rpc_grpc/README.md`](../../tests/rpc_grpc/README.md) and
[`../../benchmarks/rpc_grpc/README.md`](../../benchmarks/rpc_grpc/README.md).

## Developer Documentation

- [Architecture](ARCHITECTURE.md) — boundaries, runtime flow, and failure domains
- [Research](RESEARCH.md) — evaluation evidence and validation limits
- [Modeling](MODELING.md) — lifecycle, listener, stream, and observability models
- [Embedding](EMBEDDING.md) — integrating the plugin into a host process
- [Implementation](IMPLEMENTATION.md) — source-level runtime behavior
- [API](API.md) — interfaces, configuration, and edge cases
- [Module Guide](MODULE_GUIDE.md) — service integration checklist
- [Runbook](RUNBOOK.md) — source-verified failure triage and recovery
- [Operations](OPERATIONS.md) — readiness, monitoring, and recovery constraints
- [Trust](TRUST.md) — transport trust boundaries and security controls
- [Security](SECURITY.md) — module threat summary
- [Operator Runbook (superseded)](OPERATOR_RUNBOOK.md) — legacy page retained as a redirect

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