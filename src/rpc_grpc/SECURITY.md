# Security - RPC gRPC Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the rpc_grpc module focuses on fail-closed TLS/mTLS initialization, explicit credential reload behavior, deterministic lifecycle transitions, and observable request-path telemetry.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| insecure/invalid TLS startup fallback | fail-closed credential loading behavior |
| runtime credential drift or invalid reloads | explicit reload validation and failure signaling |
| untracked RPC request handling anomalies | per-method observability and access-log surfaces |
| unsafe lifecycle transitions | explicit lifecycle state checks and deterministic outcomes |

## Implemented Security Controls

- TLS/mTLS credential loading is validation-gated.
- insecure fallback on credential failure is not silent.
- lifecycle/startup failures are explicit and diagnosable.
- method-level metrics/logging preserve runtime observability.

## Security Follow-ups

- expand negative coverage for malformed cert/key/CA permutations.
- tighten diagnostic taxonomy for registration/stream path failures.
- deepen stress coverage for reload operations under active traffic.

## Sourcecode Verification (Module: rpc_grpc/security)

- Verified files:
  - src/rpc_grpc/grpc_plugin.cpp
  - src/rpc_grpc/grpc_plugin.h
  - src/rpc_grpc/bidi_stream_adapter.h
- Verified controls:
  - fail-closed TLS/mTLS handling
  - deterministic lifecycle and reload outcomes
  - explicit observability for RPC request handling