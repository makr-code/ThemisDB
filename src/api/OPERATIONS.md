---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Module Operations Guide

## Operational role

The `api` module provides protocol adapters and transport components used by ThemisDB's server
runtime. It does not own the primary HTTP listener, general route dispatch, storage, or query
execution engine.

## Key runtime responsibilities

- GraphQL request validation, parsing, and AST enforcement
- gRPC server startup, service registration, and message-size control
- WebSocket change-stream validation and authorization
- tracing context propagation and OTLP handling
- fail-closed behavior for malformed or unsupported inputs

## Operational checks

The following are checks for a deployment that uses these API components; they are not a claim
that every signal is emitted by the API module itself.

Assess the integrated service using its configured runtime signals and verify that:

- GraphQL documents validate quickly and do not exceed configured limits
- the gRPC listener starts on the configured address without invalid state
- WebSocket subscription requests are correctly allowed or denied
- exporter backlog does not saturate the request path
- request-level failures remain explicit and explainable

## Monitoring expectations

Where the host runtime exposes these signals, operators should verify:

- request admission and auth rejection counts
- malformed payload counts
- protocol startup failures
- queue pressure or exporter drop counts
- service-latency and throughput for GraphQL/gRPC/WebSocket paths

## Normal operating patterns

- Keep the GraphQL parser limits aligned with expected production workloads.
- Keep gRPC message size and service registration configuration explicit.
- Treat OTLP export as an operationally important but optional dependency.
- Treat WebSocket CDC authorization as a privileged path and audit it thoroughly.

## Failure and rollback policy

The correct recovery pattern for transport-layer issues is to restore a safe runtime configuration and re-enable a known-good service profile. Do not add permissive fallbacks in the transport layer to hide malformed traffic. When the failure is config-based, a small rollback is usually preferable to broad application changes.

## Escalation triggers

Escalate to module owners if:

- repeated auth failures appear on the CDC stream
- gRPC startup or service binding fails across multiple hosts
- request validation blocks valid traffic in a durable way
- exporter health affects observability or drifts outside expected bounds

## Operational summary

This module is most stable when it remains strict, explicit, and predictable. The correct operational posture is to treat protocol misuse as a boundary failure, not a hidden runtime condition.
