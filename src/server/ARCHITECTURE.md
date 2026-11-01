---
Author: "ThemisDB Contributors"
Created: "2026-09-09"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Architecture - Server Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The server module composes gateway routing, middleware enforcement, endpoint dispatch, stream/session handling, and operational server surfaces into the client-facing runtime boundary of ThemisDB.

## Main Execution Planes

1. Request lifecycle plane
- HTTP server ingress, request parsing, middleware execution, and handler dispatch behavior

2. Endpoint plane
- administrative, data, governance, observability, and specialized API handler behavior

3. Session and protocol plane
- WebSocket, MQTT, PostgreSQL-wire, and gRPC service behavior owned by the server runtime

## Core Contracts

| Contract | Behavior |
|---|---|
| ingress contract | explicit request acceptance, parsing, and dispatch semantics |
| middleware contract | bounded auth, rate-limit, and overload-control behavior |
| endpoint contract | handler-local API behavior behind shared server lifecycle control |
| session contract | explicit protocol/session behavior for server-owned transports |

## Failure Semantics

- request rejection and overload behavior remain explicit through middleware and health signaling.
- endpoint failures remain bounded to handler and response paths rather than widening into transport control.
- protocol/session failures remain observable through server-owned runtime surfaces.

## Retry, Timeout, and Graceful-Shutdown Patterns (Phase 5, Q3 2026)

Phase 5 hardening delivered two runtime resilience patterns that are now part of the production architecture:

- **P5-S01 Wire-protocol retry** — exponential-backoff retry with configurable `max_retries`, `base_delay`, global budget cap, and optional jitter. Retry eligibility is gated at the transport layer (kTransient only; kFatal/kInvalidArg fail-fast). Per-request retry-count tracking is thread-safe with explicit reset semantics. Evidence: 16 deterministic WSR tests in `tests/server/test_server_phase5_hardening.cpp`.
- **P5-S02 HTTP timeout and graceful-shutdown drain** — per-request deadline enforcement (kTimedOut on deadline overrun); server lifecycle state machine (kRunning → kDraining → kStopped) with explicit in-flight drain before stop; idle-connection and keepalive-timeout recycling. Evidence: 12 deterministic HST tests in `tests/server/test_server_phase5_hardening.cpp`.

These patterns are covered by 8 benchmark release gates SVR-01..SVR-08 in `benchmarks/server/bench_server_hotpaths.cpp`.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| `query` | `include/query/aql_runner.h`, `include/query/aql_parser_service.h` | AQL query execution and parse dispatch |
| `transaction` | `include/transaction/transaction_manager.h`, `include/transaction/distributed_transaction_manager.h` | API-driven transaction lifecycle |
| `storage` | `include/storage/rocksdb_wrapper.h`, `include/storage/storage_engine.h` | Direct storage access for admin/data endpoints |
| `sharding` | `include/sharding/shard_router.h`, `include/sharding/adaptive_shard_router.h` | Request routing to correct shard |
| `llm` | `include/llm/lora_framework/lora_orchestrator.h` | LoRA fine-tuning API handler (`lora_api_handler.cpp`) |
| `rag` | `include/rag/` | RAG query and ingestion API endpoints |
| `search` | `include/search/` | FTS and search API dispatch |
| `index` | `include/index/` | Index management endpoints |
| `graph` | `include/graph/` | Graph traversal API endpoints |
| `security` | `include/security/` | AuthN/AuthZ enforcement via middleware |
| `auth` | `include/auth/` | Token validation and scope mapping (`auth_middleware.h`) |
| `cache` | `include/cache/` | Cache admin and data-cache API surfaces |
| `cdc` | `include/cdc/` | Change-feed and CDC streaming endpoints |
| `analytics` | `include/analytics/` | Analytics and reporting API endpoints |
| `content` | `include/content/` | Content API handler dispatch |
| `plugins` | `include/plugins/` | Plugin lifecycle and registration API |
| `maintenance` | `include/maintenance/` | Maintenance job scheduling and status API |
| `network` | `include/network/` | Underlying transport for WebSocket / MQTT / gRPC sessions |
| `timeseries` | `include/timeseries/` | Time-series query and ingestion endpoints |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `llm` | `src/llm/mcp_tool_bridge.cpp` → `include/server/mcp_server.h` | LLM-side MCP tool integration |

---

## Integration Points

### Critical Integration: Query Dispatch
**Files:** `include/server/api_gateway.h`, `src/server/api_gateway.cpp`, and the configured local handler.
**Contract:** `APIGateway::handleRequest()` performs optional authentication, configured rate limiting and load shedding, version-path normalization, route selection, then calls the local handler or configured shard path. Query parsing and execution belong to the selected handler and query modules; the gateway does not directly invoke `AqlRunner`.
**Failure Mode:** The gateway returns explicit responses for its own auth, admission, and routing failures. Handler-specific query errors and transport responses are determined by those downstream components.

### Critical Integration: Auth Middleware → Security / Auth
**Files:** `include/server/auth_middleware.h` ↔ `include/auth/` + `include/security/`
**Contract:** `AuthMiddleware::authorize(token, required_scope)` returns an `AuthResult`; callers must check `authorized` before using its identity context. The gateway and individual HTTP routes invoke auth according to their configuration and route logic; there is no module-wide `AuthMiddleware::process()` stage.
**Failure Mode:** The caller determines the response mapping for a denied result. Do not assume one status code for every route or that auth is enabled globally.

### LLM and MCP integration
**Files:** `src/server/lora_api_handler.cpp` → `include/llm/lora_framework/lora_orchestrator.h`; `src/llm/mcp_tool_bridge.cpp` → `include/server/mcp_server.h`
**Contract:** The LoRA handler integrates with the LLM orchestrator; the LLM-side MCP tool bridge depends on the server MCP interface. These are explicit module edges, not evidence that every call is asynchronous or retried.

### Protocol and session ownership
**Files:** `src/server/websocket_session.cpp`, `src/server/mqtt_session.cpp`, `src/server/postgres_session.cpp`, and `src/server/themis_core_grpc_service.cpp`
**Contract:** These components implement server-owned protocol/session behavior. Their I/O, synchronization, and teardown details are protocol-specific; consult the corresponding implementation and tests before relying on a shared lifecycle or thread-safety guarantee.

---

## Sourcecode Verification (Module: server/architecture)

- Verified files:
  - src/server/http_server.cpp
  - src/server/api_gateway.cpp
  - src/server/auth_middleware.cpp
  - src/server/rate_limiting_middleware.cpp
  - src/server/load_shedder.cpp
  - src/server/chunked_response_writer.cpp
  - src/server/websocket_session.cpp
  - src/server/mqtt_session.cpp
  - src/server/postgres_session.cpp
  - src/server/themis_core_grpc_service.cpp
- Verified architecture claims:
  - request lifecycle, endpoint, and session/protocol plane split
  - explicit middleware and overload boundaries
  - module-local ownership of client-facing server runtime behavior