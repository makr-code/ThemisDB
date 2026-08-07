# Architecture - RPC gRPC Module

<!-- Status: current | validated: 2026-08-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The rpc_grpc module encapsulates gRPC server lifecycle management, TLS/mTLS credential provisioning, service registration, bidirectional stream adapter integration, and per-method observability into a bounded RPC transport plugin subsystem.

## Main Execution Planes

1. Server lifecycle plane
- initialize/start/stop behavior
- listener and runtime argument handling

2. Credentials and service plane
- TLS/mTLS material load/reload behavior
- service registration and dispatch preparation

3. Streaming and observability plane
- bidi stream adapter support behavior
- method metrics and structured access logging surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| lifecycle contract | deterministic init/start/stop/reload semantics |
| credentials contract | fail-closed TLS/mTLS handling with explicit outcomes |
| service contract | explicit registration and runtime service activation behavior |
| observability contract | per-method RPC metric/log emission behavior |

## Failure Semantics

- invalid TLS material fails startup/reload explicitly.
- duplicate/invalid lifecycle transitions return explicit failure outcomes.
- service registration errors are surfaced deterministically.
- stream and observability path failures remain non-silent.

## Sourcecode Verification (Module: rpc_grpc/architecture)

- Verified files:
  - src/rpc_grpc/grpc_plugin.cpp
  - src/rpc_grpc/grpc_plugin.h
  - src/rpc_grpc/bidi_stream_adapter.h
- Verified architecture claims:
  - lifecycle + credentials/service + streaming/observability plane split
  - explicit failure boundaries for lifecycle, credentials, and registration
  - module-local ownership of gRPC transport plugin behavior