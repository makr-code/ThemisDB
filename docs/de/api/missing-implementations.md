[docs](../../index.md) > [de](../index.md) > [api](./index.md) > [missing-implementations](./missing-implementations.md)

# API Module – Missing Implementations Report

**Generated:** 2026-03-22  
**Validated against:** commit `47727816d` (branch `copilot/update-docs-and-sourcecode`)  
**Primary source:** `src/api/`, `include/api/`

---

## Executive Summary

The API module is **production-ready** for HTTP/REST, GraphQL, WebSocket, OTLP tracing, rate
limiting, geo-index hooks, and versioned routing as of v1.8.0. No falsely-claimed `[x]` roadmap
items were found during this review cycle.

Fifteen open `[ ]` items were identified across `src/api/FUTURE_ENHANCEMENTS.md` and
`include/api/FUTURE_ENHANCEMENTS.md`. These fall into four categories:

1. **gRPC stub wiring** (6 items) — highest impact; blocks all gRPC clients beyond document CRUD
2. **gRPC server correctness** (3 items) — mutex contention and missing deadline in `grpc_server.cpp`
3. **GraphQL completeness** (4 items) — fragments, `Parser::error()` removal, introspection meta-types, parallel field resolution
4. **Middleware improvements** (5 items) — rate-limiter eviction, audit-logger threading, cache invalidation, OTLP connection reuse, URL-decode in WebSocket handler

---

## Findings

### FINDING-API-001: gRPC Phase 4 — ✅ Abgeschlossen (v1.9.0)

| Feld | Wert |
|------|------|
| **Schweregrad** | Hoch |
| **Status** | ✅ Vollständig abgeschlossen (v1.9.0) |
| **Claim-Quelle** | `src/api/ROADMAP.md` Phase 4 |
| **Feature** | GrpcApiServer::start() gibt Mutex vor BuildAndStart() frei; stop() nutzt 30s Deadline |
| **Feature** | ThemisDBGrpcServiceFactory — Header-only Fluent-Builder in `include/api/themisdb_grpc_service_factory.h` |
| **Feature** | `aqlEscape()` sanitisiert AQL-Strings für HybridSearch + FullTextSearch |
| **Tests** | 8 neue Tests in `tests/test_themisdb_grpc_service.cpp` |

---

### FINDING-API-001: gRPC ExecuteAQL Stub

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | Open |
| **Claim source** | `src/api/ROADMAP.md` — Phase 3 `[x]` gRPC surface with proto definitions |
| **Expected** | `ExecuteAQL` RPC executes AQL queries and returns results |
| **Observed** | `themisdb_grpc_service.cpp:~line 302` returns `UNIMPLEMENTED` with message "AQL execution requires an AQLEngine; wire one in via ThemisDBGrpcServiceFactory" |
| **Evidence paths** | `src/api/themisdb_grpc_service.cpp`, `include/api/themisdb_grpc_service.h` |
| **Fix** | Implement `ThemisDBGrpcServiceFactory` that injects `AQLEngine*` into `ServiceImpl`; delegate to `engine_->execute(req->query(), ...)` |
| **Issue title suggestion** | `[api] Wire gRPC ExecuteAQL stub via ThemisDBGrpcServiceFactory` |
| **Label suggestions** | `enhancement`, `api`, `grpc` |

---

### FINDING-API-002: gRPC StreamAQL Stub

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | Open |
| **Claim source** | `src/api/ROADMAP.md` — Phase 3 `[x]` gRPC surface |
| **Expected** | Server-side streaming RPC `StreamAQL(AQLQueryRequest) returns (stream AQLRow)` |
| **Observed** | `themisdb_grpc_service.cpp:~line 337` — streaming loop exists as commented code; returns `UNIMPLEMENTED` |
| **Evidence paths** | `src/api/themisdb_grpc_service.cpp` |
| **Fix** | Uncomment and wire after `ThemisDBGrpcServiceFactory` injection in FINDING-API-001 |
| **Issue title suggestion** | `[api] Wire gRPC StreamAQL server-side streaming stub` |
| **Label suggestions** | `enhancement`, `api`, `grpc` |

---

### FINDING-API-003: gRPC VectorSearch / FilteredVectorSearch / HybridSearch / FullTextSearch Stubs

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | Open |
| **Claim source** | `src/api/ROADMAP.md` — Phase 3 `[x]` gRPC surface |
| **Expected** | Vector, filtered-vector, hybrid, and full-text search RPCs return results |
| **Observed** | All four handlers return `UNIMPLEMENTED` in `themisdb_grpc_service.cpp:~line 354–393` |
| **Evidence paths** | `src/api/themisdb_grpc_service.cpp` |
| **Fix** | Add `VectorIndex*` and `FullTextIndex*` injection points to `ServiceImpl` (parallel to `AQLEngine*`); delegate to respective index APIs |
| **Issue title suggestion** | `[api] Wire gRPC VectorSearch / HybridSearch / FullTextSearch stubs` |
| **Label suggestions** | `enhancement`, `api`, `grpc` |

---

### FINDING-API-004: gRPC BatchWrite No Atomicity

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — gRPC API Surface section |
| **Expected** | All writes in a `BatchWrite` RPC succeed or fail atomically |
| **Observed** | `BatchWrite` loop calls `db_->put(key, body)` individually; if any put fails, the response still returns `success = true` with a `upserted_count` less than requested — no error code |
| **Evidence paths** | `src/api/themisdb_grpc_service.cpp` |
| **Fix** | Wrap writes in `RocksDBWrapper::WriteBatchWrapper`; set `success = false` and include error details if `upserted_count != req->upserts_size()` |
| **Issue title suggestion** | `[api] Fix gRPC BatchWrite partial-failure silent success` |
| **Label suggestions** | `bug`, `api`, `grpc` |

---

### FINDING-API-005: GrpcApiServer::start() Holds Mutex During BuildAndStart()

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `include/api/FUTURE_ENHANCEMENTS.md` — Design Constraints |
| **Expected** | `start()` releases mutex before blocking I/O |
| **Observed** | `grpc_server.cpp:start()` calls `builder.BuildAndStart()` inside `std::lock_guard<std::mutex> lock(mutex_)`, blocking `stop()` and `isRunning()` for the entire port-bind and TLS-handshake duration |
| **Evidence paths** | `src/api/grpc_server.cpp` |
| **Fix** | Extract `ServerBuilder` setup before acquiring lock; acquire lock only to store `server_` and set `running_ = true` |
| **Issue title suggestion** | `[api] Fix GrpcApiServer::start() holding mutex across BuildAndStart()` |
| **Label suggestions** | `bug`, `api`, `grpc`, `concurrency` |

---

### FINDING-API-006: GrpcApiServer::stop() No Shutdown Deadline

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `include/api/FUTURE_ENHANCEMENTS.md` — Design Constraints |
| **Expected** | `stop()` completes within a bounded time window |
| **Observed** | `grpc_server.cpp:stop()` calls `server_->Shutdown()` with no deadline — can block indefinitely if in-flight RPCs do not terminate |
| **Evidence paths** | `src/api/grpc_server.cpp` |
| **Fix** | Use `server_->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(30))` and release mutex before calling `Shutdown()` |
| **Issue title suggestion** | `[api] Add 30-second deadline to GrpcApiServer::stop() Shutdown()` |
| **Label suggestions** | `bug`, `api`, `grpc`, `reliability` |

---

### FINDING-API-007: GraphQL Parser Missing Fragments, Directives, Inline Fragments

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `include/api/graphql.h` (documented as "Not yet supported") |
| **Expected** | Full GraphQL spec compliance including fragments, directives, and inline fragments |
| **Observed** | `graphql.h` explicitly documents "Not yet supported: Fragments, Directives, Inline fragments"; Apollo and Relay-style query composition fail at parse time |
| **Evidence paths** | `include/api/graphql.h`, `src/api/graphql.cpp` |
| **Fix** | Implement `parseFragmentDefinition()` and `parseInlineFragment()` in `graphql.cpp` |
| **Issue title suggestion** | `[api] Implement GraphQL fragment and directive support in Parser` |
| **Label suggestions** | `enhancement`, `api`, `graphql` |

---

### FINDING-API-008: GraphQL Schema::introspect() Missing Meta-Types

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — GraphQL Schema Completion section |
| **Expected** | `Schema::introspect()` supports `__typename`, `__Field`, `__InputValue`, `__EnumValue`, `__Directive` per GraphQL June 2018 spec |
| **Observed** | Only `__schema` and `__type` are handled; introspection-based tooling (code generators, schema diffing tools) does not work fully |
| **Evidence paths** | `src/api/graphql.cpp` |
| **Fix** | Add missing meta-type resolvers to `Schema::introspect()` |
| **Issue title suggestion** | `[api] Add missing GraphQL introspection meta-types (__typename, __Field, etc.)` |
| **Label suggestions** | `enhancement`, `api`, `graphql` |

---

### FINDING-API-009: RateLimiter Stale Bucket Accumulation

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — Rate Limiter section |
| **Expected** | `RateLimiter::buckets_` map does not grow unbounded in long-running deployments |
| **Observed** | Every unique key creates a `Bucket` entry that is never evicted; over weeks, thousands of stale tenant-ID / IP-address buckets accumulate |
| **Evidence paths** | `include/api/rate_limiter.h` — `RateLimiter::allow()` |
| **Fix** | Add TTL-based eviction: remove buckets whose `last_refill > 2 × window` and `tokens >= capacity` |
| **Issue title suggestion** | `[api] Add TTL-based stale bucket eviction to RateLimiter` |
| **Label suggestions** | `enhancement`, `api`, `performance` |

---

### FINDING-API-010: AuditLogger Callbacks Invoked Under Mutex

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — Audit Logger section |
| **Expected** | `AuditLogger::log()` does not hold mutex during handler callbacks |
| **Observed** | `audit_logger.h::log()` holds `std::lock_guard<std::mutex>` for the entire body including the handler dispatch loop; file-writing or network-sending handlers stall all concurrent API threads |
| **Evidence paths** | `include/api/audit_logger.h` — `AuditLogger::log()` |
| **Fix** | Copy handlers vector under lock (O(n) pointer copies), release lock, invoke handlers outside critical section |
| **Issue title suggestion** | `[api] Fix AuditLogger blocking handler dispatch under mutex` |
| **Label suggestions** | `bug`, `api`, `concurrency` |

---

### FINDING-API-011: ResponseCache::invalidatePattern() Ignores Pattern Argument

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — GraphQL Response Cache section |
| **Expected** | `invalidatePattern(collection)` evicts only cache entries referencing the given collection |
| **Observed** | `graphql_cache.h:290` ignores the `pattern` argument and calls `cache_.clear()`, invalidating all cached responses |
| **Evidence paths** | `include/api/graphql_cache.h` — `ResponseCache::invalidatePattern()` |
| **Fix** | Tag `CachedResponse` with read-collection set at insertion time; evict only entries whose tag set contains `pattern` |
| **Issue title suggestion** | `[api] Implement selective collection-based cache invalidation in ResponseCache` |
| **Label suggestions** | `enhancement`, `api`, `graphql`, `performance` |

---

### FINDING-API-012: OtlpExporter New CURL Handle Per Flush Batch

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — OTLP Exporter section |
| **Expected** | `OtlpExporter` reuses a persistent `CURL*` handle across flush batches |
| **Observed** | `otlp_exporter.cpp::flushBatch()` calls `curl_easy_init()` / `curl_easy_cleanup()` on every flush, opening a new TCP connection each time |
| **Evidence paths** | `src/api/otlp_exporter.cpp`, `include/api/otlp_exporter.h` |
| **Fix** | Create one persistent `CURL*` in `start()` with `CURLOPT_FORBID_REUSE=0L` and `CURLOPT_TCP_KEEPALIVE=1L`; reuse across `flushBatch()` calls |
| **Issue title suggestion** | `[api] Use persistent CURL handle in OtlpExporter::flushBatch()` |
| **Label suggestions** | `enhancement`, `api`, `performance` |

---

### FINDING-API-013: WsChangeHandler URL-Decoding of Query Parameters

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — WebSocket Streaming section |
| **Expected** | `WsChangeHandler::validate()` URL-decodes `from_sequence` and `key_prefix` query-string parameters |
| **Observed** | `ws_handler.cpp` uses `std::string::find` for ad-hoc query-string parsing; percent-encoded values (e.g., `key_prefix=orders%3A`) are never decoded and cause incorrect filter values |
| **Evidence paths** | `src/api/ws_handler.cpp` |
| **Fix** | Apply `boost::urls` or a small `url_decode()` utility before extracting parameter values |
| **Issue title suggestion** | `[api] Fix WsChangeHandler URL-decoding for query-string parameters` |
| **Label suggestions** | `bug`, `api`, `websocket` |

---

### FINDING-API-014: IGRPCBridge Has No Concrete Implementation

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | Open |
| **Claim source** | `include/api/FUTURE_ENHANCEMENTS.md` — gRPC Bridge Interface section |
| **Expected** | A concrete `GrpcBridgeImpl` class registers services and routes dispatches |
| **Observed** | `grpc_bridge.h` defines pure-virtual `IGRPCBridge`; no concrete implementation exists in the codebase |
| **Evidence paths** | `include/api/grpc_bridge.h` |
| **Fix** | Implement `GrpcBridgeImpl` in `src/api/grpc_bridge.cpp` with `std::unordered_map<std::string, ServiceDescriptor>` guarded by `std::shared_mutex` |
| **Issue title suggestion** | `[api] Implement GrpcBridgeImpl concrete class for IGRPCBridge interface` |
| **Label suggestions** | `enhancement`, `api`, `grpc` |

---

### FINDING-API-015: QueryAllowList Disabled by Default in Production

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | Open |
| **Claim source** | `src/api/FUTURE_ENHANCEMENTS.md` — Security/Reliability section |
| **Expected** | `QueryAllowList` enforcement is active in production builds; startup warning if disabled |
| **Observed** | `persisted_queries.h::QueryAllowList` has `enabled_ = false` in default constructor; no startup warning emitted when running with `NDEBUG` |
| **Evidence paths** | `include/api/persisted_queries.h` |
| **Fix** | Add startup check that emits `THEMIS_WARN` if `QueryAllowList` is disabled in a production build (detected via `NDEBUG`); document activation path in operations runbook |
| **Issue title suggestion** | `[api] Add startup warning when QueryAllowList is disabled in production` |
| **Label suggestions** | `security`, `api` |

---

## Summary Table

| ID | Severity | Category | Status |
|----|----------|----------|--------|
| FINDING-API-001 | High | gRPC Stub | Open |
| FINDING-API-002 | High | gRPC Stub | Open |
| FINDING-API-003 | High | gRPC Stub | Open |
| FINDING-API-004 | High | gRPC Correctness | Open |
| FINDING-API-005 | Medium | gRPC Concurrency | Open |
| FINDING-API-006 | Medium | gRPC Reliability | Open |
| FINDING-API-007 | Medium | GraphQL Completeness | Open |
| FINDING-API-008 | Medium | GraphQL Completeness | Open |
| FINDING-API-009 | Medium | Rate Limiter | Open |
| FINDING-API-010 | Medium | Audit Logger | Open |
| FINDING-API-011 | Low | GraphQL Cache | Open |
| FINDING-API-012 | Low | OTLP Exporter | Open |
| FINDING-API-013 | Medium | WebSocket | Open |
| FINDING-API-014 | Low | gRPC Bridge | Open |
| FINDING-API-015 | Medium | Security | Open |
