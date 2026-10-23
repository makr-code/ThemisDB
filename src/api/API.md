---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Module Contract Notes

## Purpose

This document describes the public-facing contract of the `api` module: how transport logic, request admission, and service exposure are expected to behave at the boundary between external clients and internal ThemisDB services.

## Public contract surfaces

The public contracts are primarily defined in `include/api`:

- `graphql.h` — GraphQL parser, AST model, and runtime value representation
- `graphql_aql_resolver.h` — GraphQL/AQL resolver entry points
- `graphql_ws_handler.h` — GraphQL subscription transport adapter
- `grpc_server.h` — gRPC lifecycle and service registration API
- `ws_handler.h` — WebSocket CDC request validation and upgrade contract
- `otlp_exporter.h` — OTLP export and queue semantics
- `api_transport_contracts.h` — shared API transport contract definitions
- `api_transport_policy.h` — request admission policy and failure-class mapping

## Contract expectations

### GraphQL

The GraphQL layer accepts a document string and enforces maximum size, maximum nesting depth, field cardinality, and AST node count. Unsupported fragment syntax is rejected explicitly. The design is fail-closed and intends to keep large or malicious requests from consuming excessive runtime resources.

### gRPC

gRPC startup requires a valid host and port, an initialized configuration, and at least one bound service. The server also enforces sensible message-size limits and can optionally load configuration values from `config/networking/grpc.yaml` when present.

### WebSocket change stream

The WebSocket endpoint is a privileged CDC stream. It requires the advertised path, valid auth context, and the `cdc:subscribe` permission scope before upgrade. Optional filters such as `from_sequence` and `key_prefix` are parsed carefully and must not open the stream to ambiguous or invalid request state.

### Observability export

OTLP and tracing export paths are not allowed to silently block the main request path. Error and backpressure behavior is expected to be explicit and bounded so operators can distinguish missing telemetry from a live service failure.

## Failure semantics

The API layer defines explicit errors rather than hidden recovery:

- malformed GraphQL input is invalid syntax
- invalid port or missing setup is rejected before bind
- unauthorized stream access is denied
- unsupported capability or transport mismatch is mapped to the shared failure taxonomy
- exporter pressure is surfaced as warnings or dropped spans with explicit accounting

These semantics are intentional: clients and operators should be able to reason about the boundary without reading through the implementation internals.

## Compatibility guidance

The module's release contract has to remain stable across deployment profiles and transport combinations. Changes to public API behavior should preserve the logical semantics of the transport contract: explicit errors, bounded resource usage, and well-defined observability degradation under overload.
