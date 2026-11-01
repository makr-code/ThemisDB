---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Server Embedding and Internal Dependencies

## Purpose

This document describes how the `server` module sits inside ThemisDB's larger runtime graph. The server is the public boundary and the integration hub for request dispatch, middleware enforcement, and protocol-driven access to storage and service capabilities.

## Upstream dependencies

The server module consumes multiple internal runtime systems, including:

| Dependency area | Representative sources | Role |
|---|---|---|
| Auth and identity | `auth/`, `security/` | Validate tokens, scopes, and credentials |
| Query / execution | `query/` | Execute AQL or query operations on behalf of endpoint handlers |
| Storage / durability | `storage/` | Data persistence and recovery path behind API operations |
| Sharding / routing | `sharding/` | Request placement and distributed dispatch |
| LLM / model | `llm/` | Model serving and prompt or LoRA operations |
| Network / transport | `network/` | Session and socket primitives for gRPC, WebSocket, MQTT |
| Observability | `observability/` | Status, metrics, and error reporting |

The module therefore acts as an orchestrator of contracts, not as the sole owner of runtime semantics.

## Critical edges

### Query dispatch

`APIGateway::handleRequest()` decides whether a request is local, shard-routed, or scatter-gather. The code preserves a normalized path and then invokes the downstream route or handler. That integration boundary keeps routing logic in the server layer while leaving execution logic in query-centric modules.

### Authentication and authorization

`AuthMiddleware` is the gatekeeper for tokens and provider-backed identity checks. It combines multiple auth methods while preserving a role/scope mapping model that can be enforced before the business handler executes. This is a key trust boundary in the system.

### Load and backpressure

`RateLimitingMiddleware` and `LoadShedder` generate explicit rejection decisions before the workload reaches deeper services. This gives the server module a non-trivial backpressure role: it can protect the rest of the cluster from overload collapse.

### Protocol lifecycle

The HTTP, WebSocket, MQTT, and gRPC session code paths keep transport state in the server module. The connection remains the server's concern even when the actual business logic sits elsewhere.

## Downstream exposure

The server layer is the outward-facing boundary that enables:

- administrative endpoints,
- data queries and entity operations,
- vector/graph/timeseries APIs,
- LLM, model, and prompt services,
- health/error/monitoring surfaces.

This means the server is not just a transport shell; it is the runtime contract described by APIs to clients and operators.

## Operational embedding model

A production request is best conceptualized as:

1. client connects to server transport,
2. route / version gating is determined,
3. auth and quota checks run,
4. shard or local routing is decided,
5. endpoint executes against other modules,
6. result or failure is emitted back to the client.

This is a conceptual flow, not a guarantee that every listener uses one
shared gateway pipeline. `HttpServer::routeRequest()` and
`APIGateway::handleRequest()` are distinct paths; verify the concrete route
and configuration before assuming a common auth, admission, or dispatch order.

This embedding model explains why the module's quality is operationally important: if it fails early or fails closed, the rest of ThemisDB remains safer and more diagnosable.

## Guardrails

- Keep middleware decisions explicit and not hidden in downstream code.
- Preserve a narrow server interface surface; avoid moving deep business logic into the gateway.
- Keep protocol and session state independent from core storage semantics.
- Prefer deterministic failures (401, 403, 429, 503) over silent fallback or speculative retries.

## Summary

The `server` module is the integration fabric of ThemisDB. It is not the database's core; it is the boundary responsible for safe, observable, and bounded access to it.
