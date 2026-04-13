<!-- Status: current | validated: 2026-06-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# API Module - Future Enhancements

## Scope

This document covers implementation-specific future enhancements for the API module (`src/api/` and `include/api/`), which exposes ThemisDB over HTTP (stub in `src/api/http_server.cpp`; live implementation in `src/server/http_server.cpp`), GraphQL (`graphql.cpp` + `graphql_ws_handler.cpp`), WebSocket CDC streaming (`ws_handler.cpp`), gRPC (`grpc_server.cpp` + `themisdb_grpc_service.cpp`), geospatial index hooks (`geo_index_hooks.cpp`), request tracing (`tracing_middleware.cpp`), and OTLP span export (`otlp_exporter.cpp`). Supporting header-only components in `include/api/` — `rate_limiter.h`, `audit_logger.h`, `graphql_cache.h`, `persisted_queries.h`, `websocket_handler.h`, `grpc_bridge.h` — are equally in scope. Enhancements to AQL execution, storage engines, or authentication internals are out of scope; only the API surface, transport layer, and middleware pipeline are covered here.

## Design Constraints

- `[x]` REST endpoint signatures introduced in v1.x must remain backward-compatible; new capabilities are added via versioned prefixes (`/v2/`) or opt-in headers, not breaking changes to existing `/v1/` routes. (**Enforced** — `RouteVersionRouter` redirects unversioned paths 301 to `/v1/`; all new functionality is under `/v2/`.)
- `[x]` The GraphQL parser in `graphql.cpp` uses `QueryLimits::defaults()` for depth/complexity guards; any new field resolver must enforce those limits to prevent query amplification. (**Enforced** — `QueryLimits` passed to `Parser::parse()` in all call sites; `QueryLimits::production()` disables introspection.)
- `[x]` TLS is mandatory for all production transports; new WebSocket and gRPC transports must share the same TLS context as the existing HTTP listener. (**Enforced** — `GrpcApiServer` uses `grpc::SslServerCredentials` from the same PEM paths; WebSocket upgrades go through the Beast TLS acceptor.)
- `[x]` Auth middleware (`src/auth/`) is a hard dependency; no new transport may bypass JWT/JWKS validation enforced by `jwt_validator.cpp`. (**Enforced** — `WsChangeHandler::validate()` and gRPC interceptor both call `AuthMiddleware::authorize` before any data is exchanged.)
- `[x]` `GrpcApiServer::start()` must not hold `mutex_` across blocking operations. (**Fixed v1.9.0** — mutex released before `BuildAndStart()`; lock re-acquired afterwards.)
- `[x]` `GrpcApiServer::stop()` must specify a shutdown deadline. (**Fixed v1.9.0** — 30-second hard deadline passed to `server_->Shutdown()`.)
- `[x]` GraphQL `$variable` references in field arguments must be resolved at execution time. (**Fixed v2.0.0** — `Value::VariableRef` type + `Executor::resolveValue()` + default-value merge in `executeOperation()`; see `include/api/graphql.h`, `src/api/graphql.cpp`.)

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `graphql::Parser::parse()` | GraphQL HTTP handler | `QueryLimits` must be configurable per-tenant |
| `geo_index_hooks` | REST geo query endpoints | Hook registration must be idempotent for hot-reload |
| `auth::JWTValidator` | All HTTP/WS/gRPC handlers | Must propagate tenant ID into request context |
| `cdc::Changefeed` | Planned WebSocket change-stream endpoint | Requires `Changefeed::subscribe()` returning an async event iterator |
| `aql::LLMAQLHandler` | AQL execution endpoint | Streaming result set needed for `/v2/query/stream` |
| `IGRPCBridge` (`include/api/grpc_bridge.h`) | gRPC bridge consumers | Interface remains extension point; runtime wiring is factory-driven via `ThemisDBGrpcServiceFactory` |

## Planned Features

### GraphQL Schema Completion and Subscription Support
**Priority:** High
**Target Version:** v1.7.0

`graphql.cpp` implements a full parser and query executor but lacks mutation resolvers, schema introspection (`__schema`, `__type`), and subscription over WebSocket. Complete the schema to cover documents, graph edges, vector search, and geospatial queries; add `subscription` operation support backed by `cdc::Changefeed`.

**Implementation Notes:**
- `[x]` Add `SchemaRegistry` class to `graphql.cpp`; auto-build from registered `TypeDefinition` objects at server start.
- `[x]` Implement `__schema` and `__type` introspection resolvers; required by all major GraphQL clients (Apollo, Relay).
- `[x]` Subscription transport: use Boost.Beast WebSocket upgrades; create `graphql_ws_handler.cpp` implementing the `graphql-transport-ws` protocol (not the legacy `subscriptions-transport-ws`).
- `[x]` Wire `cdc::Changefeed::subscribe(filter)` as the event source for `subscription { onChange(collection: "...") { ... } }`. Implemented: `Changefeed::subscribe(SubscriptionFilter, SubscriptionCallback)` + `SubscriptionHandle` RAII type in `changefeed.h/cpp`; wired in `GraphQLWsHandler::handleSubscribe()` via `extractOnChangeCollection()`.
- `[x]` Enforce `QueryLimits::maxSubscriptions` per connection to prevent fan-out DoS.
- `[ ]` In `graphql.h`, the `Parser` class explicitly documents "Not yet supported: Fragments, Directives, Inline fragments." Implement `parseFragmentDefinition()` and `parseInlineFragment()` in `graphql.cpp` — without fragment support, clients using Apollo's automatic persisted query fragments or any relay-style fragment composition will fail at parse time.
- `[ ]` `graphql.h::Parser::error()` is documented as **deprecated** ("Deprecated: Use `Result<T>` return types instead of `error()` method") but the method still exists in the class definition. Remove it after migrating all call sites in `graphql.cpp` to return `themis::Result<T>` with structured `ParseError` objects to eliminate the dual error-reporting path.
- `[ ]` `Schema::introspect()` in `graphql.cpp` only handles `__schema` and `__type` fields. The GraphQL June 2018 spec also requires `__typename` on every composite type, `__Field`, `__InputValue`, `__EnumValue`, and `__Directive` meta-types. Add these to `Schema::introspect()` so introspection-based tooling (code generators, schema diffing tools) works fully.
- `[ ]` `Executor::executeSelections()` in `graphql.cpp` resolves fields serially in a range-for loop. For independent sibling fields that each invoke storage I/O, this means sequential round-trips. Add parallel field resolution via `std::async` or a small task graph; guard behind a `QueryLimits::parallel_fields_enabled` flag to allow gradual rollout.

**Performance Targets:**
- GraphQL parse + validate + execute for a 10-field document query in < 2 ms (p99) under 500 concurrent HTTP/2 connections.
- Subscription event delivery latency < 50 ms from `Changefeed` event emission to WebSocket frame sent.

**API Sketch:**
```graphql
# New subscription type (graphql.cpp SchemaRegistry)
type Subscription {
  onChange(collection: String!, filter: ChangeFilter): ChangeEvent!
}

type ChangeEvent {
  sequence: Int!
  type: ChangeType!
  key: String!
  document: JSON
  timestampMs: Int!
}
```

---

### WebSocket Real-Time Change Streaming Endpoint
**Priority:** High
**Target Version:** v1.7.0

Add a dedicated WebSocket endpoint `/v2/changes` that multiplexes multiple `cdc::Changefeed` subscriptions over a single connection. This is distinct from the GraphQL subscription path and targets clients that need raw change events without the GraphQL envelope.

**Implementation Notes:**
- `[x]` Create `ws_handler.cpp` (`src/api/ws_handler.cpp`); register route `WS /v2/changes` in `src/server/http_server.cpp`.
- `[x]` Frame format: newline-delimited JSON, each frame matching `Changefeed::ChangeEvent::toJson()` output. (`WebSocketSession::pollCDCEvents` emits JSON via `ev.toJson()` / `buildEventFrame`; legacy path uses `cdc_message["type"]="cdc_event"`)
- `[x]` Client subscribes/unsubscribes by sending `{"action":"subscribe","collection":"orders","filter":{"type":"PUT"}}` control frames. (`WebSocketSession::processMessage` handles `type="subscribe"/"unsubscribe"` for `/v2/changes`; `CdcWebSocketHandler::handleFrame` handles `action="subscribe"/"unsubscribe"/"ack"` for `/v2/cdc/stream`)
- `[x]` Implement per-connection back-pressure: if the outbound frame queue exceeds 1,000 entries, close with `1011 Internal Error` and log tenant/connection ID. (`WebSocketSession::kMaxQueueDepth = 1000`)
- `[x]` Reuse `auth::JWTValidator` middleware already wired for HTTP; extract Bearer token from the WebSocket upgrade `Authorization` header. (`WsChangeHandler::validate()` requires `cdc:subscribe` scope)
- `[ ]` `WsChangeHandler::validate()` in `ws_handler.cpp` parses query-string parameters (`from_sequence`, `key_prefix`) with ad-hoc string search using `std::string::find`. URL-encoded characters (e.g., `key_prefix=orders%3A`) are never decoded, so clients that percent-encode the query string will receive incorrect filter values. Replace with a proper URL-decoding step (e.g., using `boost::urls` or a small `url_decode()` utility) before extracting parameter values.

**Performance Targets:**
- ≥ 10,000 concurrent WebSocket connections on a single node with < 50 MB additional RSS.
- Frame delivery latency p99 < 30 ms under 5,000 events/sec aggregate throughput.

---

### Versioned API Routing and `/v2/` Prefix
**Priority:** High
**Target Version:** v1.8.0

Current REST routes use unversioned paths (e.g., `/documents/{id}`). Introduce a `/v1/` prefix retroactively (with redirect from unversioned) and implement `/v2/` routes that support bulk operations, streaming query results, and async job tracking.

**Implementation Notes:**
- `[x]` Add `RouteVersionRouter` middleware in `include/server/route_version_router.h`; unversioned paths redirect 301 to `/v1/`; wired in `src/server/http_server.cpp`.
- `[x]` `/v1/` routes: exact current behaviour; unversioned paths redirect 301 to `/v1/` via `RouteVersionRouter::getRedirectTarget()`.
- `[x]` `/v2/documents` — bulk insert endpoint accepting `application/x-ndjson` body (newline-delimited JSON documents, up to 10,000 per request); implemented in `EntityApiHandler::handleBulkNdjson()`.
- `[x]` `/v2/query/stream` — SSE endpoint implemented via `QueryApiHandler::handleQueryStreamSse()`; registered as `Route::QueryStreamSseGet`.
- `[x]` `/v2/jobs/{id}` — async job status for long-running queries; store job state in `cache::AdaptiveQueryCache` with TTL = 1 hour.

**Performance Targets:**
- Bulk insert of 10,000 256-byte documents in < 500 ms end-to-end (network excluded).
- SSE streaming first-byte latency < 5 ms after query planning completes.

---

### gRPC API Surface — Wire Stub Implementations
**Priority:** High
**Target Version:** v2.0.0

`themisdb_grpc_service.cpp` is now factory-wired for core operations and query execution. Remaining enhancement scope is focused on advanced search parity and hardening of batch semantics.

**Implementation Notes:**
- `[x]` Create `src/api/grpc_server.cpp`; gRPC C++ server using `grpc::ServerBuilder` (synchronous dispatch model, consistent with the rest of the codebase).
- `[x]` Reuse existing service-layer infrastructure via `GrpcApiServer::registerService()`; no business logic duplication — service implementations are registered externally.
- `[x]` TLS: `grpc::SslServerCredentials` using the same PEM cert/key pair as the Beast HTTP listener; fail-closed on cert load failure.
- `[x]` Expose gRPC reflection service in debug builds only to prevent schema leakage in production.
- `[x]` **`ExecuteAQL` factory wiring**: `ThemisDBGrpcServiceFactory` now injects `AQLEngine*` and delegates execution through service implementations.
- `[x]` **`StreamAQL` server-streaming path**: streaming AQL execution is wired when the query engine is present; service keeps `UNIMPLEMENTED` fail-fast semantics when the dependency is absent.
- `[ ]` **Advanced search RPC parity**: `VectorSearch`, `FilteredVectorSearch`, `HybridSearch`, and `FullTextSearch` still require complete backend feature wiring across all deployment profiles; current behavior is dependency/feature-gated and may return `UNIMPLEMENTED` where optional engines are missing.
- `[ ]` **Hard-coded document version** in `CreateDocument` and `UpdateDocument` (`themisdb_grpc_service.cpp`): both handlers unconditionally set `resp->set_version(1)`, regardless of whether the document already existed. Add a real version counter sourced from the storage layer (e.g., a RocksDB sequence number or a dedicated version key) so optimistic-concurrency clients can detect conflicting updates.
- `[ ]` **`BatchWrite` silent partial failures** (`themisdb_grpc_service.cpp`): the loop over `req->upserts()` increments `upserted` only when `db_->put(key, body)` returns true, but the final response always sets `resp->set_success(true)`. If some puts fail (e.g., storage full), the caller receives a success response with a `upserted_count` less than the number of requested writes, with no error code. Change to: if `upserted_count != req->upserts_size()`, set `success = false` and include error details.
- `[ ]` **`BatchWrite`/`BatchRead` lack input bounds checks**: no validation of the number of documents in `req->upserts()` or keys in `req->keys()`. A single request can contain arbitrarily many items, leading to unbounded memory allocation. Add a hard upper limit (e.g., 10,000 items) with a `RESOURCE_EXHAUSTED` gRPC status code on violation.
- `[x]` **`GrpcApiServer::start()` mutex scope hardening**: blocking startup work is performed outside the critical section; lock is used only for state commit.
- `[x]` **`GrpcApiServer::stop()` bounded shutdown**: shutdown deadline is set and lock hold duration minimized to avoid state-query contention.
- `[ ]` **`GrpcServerConfig::max_message_size_bytes` configurability**: default remains compile-time; expose a runtime config key (e.g., `grpc.max_message_size_mb`) for operator tuning.

**Performance Targets:**
- gRPC unary `GetDocument` < 1 ms added latency vs equivalent REST call (same process).
- gRPC streaming `ExecuteQuery` sustains ≥ 100,000 rows/sec on localhost.

---

### Request Tracing and Correlation IDs
**Priority:** Medium
**Target Version:** v1.7.0

All inbound requests must carry or receive a `X-Correlation-ID` header that propagates through the entire call stack (API → AQL → storage → cache) and appears in all log lines and error responses.

**Implementation Notes:**
- `[x]` Add `TracingMiddleware` in `src/api/tracing_middleware.cpp`; generate UUID v4 if `X-Correlation-ID` absent; inject into thread-local `RequestContext`. (**Implemented** — `TracingMiddleware::processRequest()` uses `boost::uuids::random_generator` per thread.)
- `[x]` Forward `RequestContext::correlationId` to `include/utils/logger.h` log macros via a structured field (`correlation_id`). (**Implemented** — `utils::Logger::setTraceContext(corr_id)` called in `processRequest()`.)
- `[x]` Echo back `X-Correlation-ID` in all responses including errors and SSE streams (implemented in `HttpServer::applyGovernanceHeaders()`).
- `[x]` Export span data to OpenTelemetry collector via OTLP HTTP exporter (configurable endpoint in `config/networking/`). Implemented in `include/api/otlp_exporter.h` + `src/api/otlp_exporter.cpp` (async queue + libcurl POST, OTLP JSON format); `TracingMiddleware` extended with `finishSpan()` and optional `OtlpExporter*`; configuration in `config/networking/otlp.yaml`.
- `[x]` Decision: retain proprietary `X-Correlation-ID` as the primary correlation header; the OTLP exporter uses the correlation-ID value as the OTLP `traceId`. A future W3C `traceparent` bridge can be added when SDK interoperability is required.

**Performance Targets:**
- Middleware overhead < 10 µs per request (UUID generation + thread-local write).
- Zero correlation ID collision probability for ≥ 1 billion requests (UUID v4 guarantee).

---

### OTLP Exporter Performance and Reliability
**Priority:** Medium
**Target Version:** v2.1.0

`otlp_exporter.cpp` implements an async queue + background-thread OTLP/HTTP exporter using libcurl. Two structural inefficiencies limit throughput and reliability at production scale.

**Implementation Notes:**
- `[ ]` **New `CURL*` handle per flush batch** (`otlp_exporter.cpp::flushBatch()`): every call to `flushBatch()` opens a new TCP connection via `curl_easy_init()` and cleans up with `curl_easy_cleanup()` after the POST. Under the default flush interval (5 s) with 64-span batches this is infrequent, but if the batch interval is reduced or the collector is remote, connection setup becomes the dominant latency. Replace with a persistent `CURL*` handle created once in `start()` and reused across batches (set `CURLOPT_FORBID_REUSE=0L` and `CURLOPT_TCP_KEEPALIVE=1L`).
- `[ ]` **`queue_` uses `std::vector` with `erase(begin, begin+n)` dequeue** (`otlp_exporter.h` + `otlp_exporter.cpp::flushLoop()`): the internal span queue is a `std::vector<SpanData>` and the dequeue path calls `queue_.erase(queue_.begin(), queue_.begin() + take_offset)`, which is O(n) because it shifts all remaining elements. Replace with `std::deque<SpanData>` or a fixed-size ring buffer to get O(1) pop-front at the cost of a trivial container change.
- `[x]` **No retry on transient HTTP errors**: `flushBatch()` now retries up to `max_export_retries` times (default 3) with exponential back-off (`retry_initial_delay_ms` doubles each attempt: 100 ms → 200 ms → 400 ms) for retriable HTTP status codes (429, 503) and transient curl transport errors, before dropping the batch and incrementing `dropped_count_`. Non-retriable HTTP errors (e.g. 400, 404, 500) still drop immediately. Both new fields are exposed in `OtlpExporterConfig` with defaults `max_export_retries = 3` and `retry_initial_delay_ms = 100`.
- `[ ]` **`droppedSpanCount` metric not exposed via Prometheus**: `OtlpExporter::droppedSpanCount()` and `exportedSpanCount()` exist but are not wired to the Prometheus `/metrics` endpoint. Register `otlp_spans_exported_total` and `otlp_spans_dropped_total` counters in the Prometheus registry at `OtlpExporter::start()` time.

**Performance Targets:**
- Span enqueue (hot path) < 500 ns per call (single lock acquire + vector push_back or deque push_back).
- Flush of 64 spans to a local OTLP collector < 5 ms end-to-end (reusing a persistent connection).

---

### Rate Limiter — Stale Bucket Eviction and Nested Lock Contention
**Priority:** Medium
**Target Version:** v2.0.0

`include/api/rate_limiter.h` implements a token-bucket rate limiter. Two structural issues limit correctness and scalability in long-running deployments.

**Implementation Notes:**
- `[ ]` **`buckets_` map grows unbounded** (`rate_limiter.h::RateLimiter::allow()`): every unique key passed to `allow()` creates a `Bucket` entry that is never removed. In production, keys are typically tenant IDs or IP addresses; a deployment running for weeks will accumulate thousands of stale buckets. Add a TTL-based eviction pass: in `allow()` (or a dedicated background sweep), remove buckets whose `last_refill` is older than `2 × window` and whose `tokens >= capacity` (fully recharged means no active traffic).
- `[ ]` **`OperationRateLimiter::allow()` holds outer mutex while calling inner `RateLimiter::allow()`** (`rate_limiter.h`): `OperationRateLimiter::allow()` takes `mutex_` with a `std::lock_guard`, then calls `it->second->allow(key, cost)`, which in turn takes `RateLimiter::mutex_`. This is a two-mutex lock chain on every allowed request. Under high concurrency (e.g., 5,000 GraphQL requests/sec), this creates a mutex bottleneck on the outer lock. Replace the outer `std::mutex` with `std::shared_mutex` (shared lock for `allow()`/`remaining()`; exclusive lock only for `setLimit()`).
- `[ ]` **`RateLimiter::allow()` calls `steady_clock::now()` inside the lock** (`rate_limiter.h::Bucket::consume()`): `Bucket::refill()` calls `std::chrono::steady_clock::now()` while the outer `mutex_` is held. A clock syscall under a mutex adds unnecessary critical-section time. Compute `now` before acquiring the lock and pass it to `consume()`.

**Performance Targets:**
- `RateLimiter::allow()` throughput ≥ 1,000,000 calls/sec single-thread (vs. ~200,000 with current nested locks).
- Stale bucket count bounded to ≤ 2× the number of active clients at any time.

---

### GraphQL Response Cache — Pattern-Based Invalidation
**Priority:** Medium
**Target Version:** v2.0.0
**Status:** ✅ Implemented

`include/api/graphql_cache.h::ResponseCache::invalidatePattern()` previously contained a `TODO: Implement pattern-based invalidation` comment. The implementation now performs selective eviction.

**Implementation Notes:**
- `[x]` **`ResponseCache::invalidatePattern()` always clears entire cache** (`graphql_cache.h:290`): the method now iterates the cache and evicts only entries whose `collections` tag set contains the given pattern. `CachedResponse` has been extended with a `std::unordered_set<std::string> collections` field. The generic `Cache<T>` template gained an `eraseIf(pred)` method for O(n) selective eviction.

**Performance Targets:**
- Targeted invalidation of a single collection evicts ≤ 10% of cached entries when 10 distinct collections are active. ✅ Verified by `GraphQLCache.InvalidatePatternPerformanceTarget` test.

---

### Audit Logger — Non-Blocking Handler Dispatch
**Priority:** Medium
**Target Version:** v2.0.0

`include/api/audit_logger.h::AuditLogger::log()` holds `mutex_` for the entire duration of calling all registered handlers. Handlers may write to disk, push to a network audit sink, or run regex matching — all while the mutex is held.

**Implementation Notes:**
- `[x]` **`AuditLogger::log()` holds `mutex_` during handler callbacks** (`audit_logger.h::log()`): a `std::lock_guard<std::mutex> lock(mutex_)` is held for the entire body of `log()`, including the inner `for (const auto& handler : handlers_) { handler(entry); }` loop. File-writing or network-sending handlers will stall every concurrent API thread that tries to emit an audit entry. Decouple: copy the handlers vector under the lock (O(n) pointer copies), release the lock, then invoke the handlers outside the critical section. The buffer append (also inside the lock) is already fast and should remain protected. (**Fixed** — `log()` now copies `handlers_` under a scoped lock, releases the lock, then invokes each handler; buffer append and stats update remain protected by a second scoped lock.)
- `[x]` **In-memory audit buffer is not persistent** (`audit_logger.h`): `buffer_` (a circular in-memory vector) is lost on process restart. Add an optional file-backed `AuditLogHandler` that appends newline-delimited JSON audit entries to a configurable path, and register it by default when `config/audit.yaml` specifies `persistence: file`. (**Implemented** — `FileAuditLogHandler` class added to `audit_logger.h`; `AuditLogger::addFileHandler(path)` convenience method registers a JSONL-appending handler; `config/audit.yaml` now contains a `persistence:` section with `backend: none|file` and `file_path` settings.)

---

### GraphQL WebSocket Handler — CDC Callback Lifetime Safety
**Priority:** High
**Target Version:** v1.8.0

`graphql_ws_handler.cpp::handleSubscribe()` captures a raw `GraphQLWsHandler*` (`self`) pointer inside the CDC callback lambda that is passed to `Changefeed::subscribe()`. The `SubscriptionHandle` RAII type should cancel the subscription on destruction, but the safety of this interaction depends on CDC correctly serialising the callback teardown before the handle destructor returns.

**Implementation Notes:**
- `[x]` **Raw `self` pointer captured in CDC callback** (`graphql_ws_handler.cpp::handleSubscribe()`): the lambda `[self, sub_id](const themis::Changefeed::ChangeEvent& ev) { ... std::lock_guard<std::mutex> lk(self->mutex_); self->pending_frames_.push_back(frame); }` is invoked by the CDC system on its own thread. If the CDC implementation allows callbacks to fire after `SubscriptionHandle` destruction (even briefly), this is a use-after-free. Add a `std::shared_ptr<std::atomic<bool>>` "alive" flag shared between the handler and the lambda; the lambda checks it before dereferencing `self`, and the flag is set to false in `GraphQLWsHandler::reset()` before subscriptions are cleared. (**Implemented** — `alive_` member added to header; constructor initialises to `true`; `reset()` stores `false` with `memory_order_release` before `subscriptions_.clear()`; lambda captures `alive` by value and loads with `memory_order_acquire` before dereferencing `self`.)
- `[x]` **Missing step-2 in `handleSubscribe()` comment sequence** (`graphql_ws_handler.cpp`): the comment block labels "step 1" (reject duplicate IDs + enforce max_subscriptions) and "step 3" (parse payload), with no "step 2". This indicates a planned intermediate validation step (likely query variable type-checking against the schema) was omitted. Add schema-level argument type validation: verify that `variables` provided in the payload match the declared `VariableDefinition` types in the parsed operation before registering the subscription. (**Implemented** — `validateVariables(const graphql::Operation&, const nlohmann::json&)` private static helper validates required/non-null presence, null-value legality, list vs. scalar shape, and built-in scalar type matching (String/ID/Int/Float/Boolean). Called in step 2 of `handleSubscribe()`, after parse/operation-type validation and before subscription registration.)

**Performance Targets:**
- Zero use-after-free races under 10,000 concurrent subscription setup/teardown cycles.

---

### gRPC Bridge Interface — Concrete Implementation
**Priority:** Low
**Target Version:** v2.1.0

`include/api/grpc_bridge.h` defines a pure-virtual `IGRPCBridge` interface and supporting plain-data structs (`ServiceDescriptor`, `GRPCRequest`, `GRPCMetadata`) for registering and routing gRPC services. No concrete implementation is registered anywhere in the codebase.

**Implementation Notes:**
- `[ ]` **`IGRPCBridge` has no concrete implementation** (`grpc_bridge.h`): the interface exposes `registerService()`, `route()`, `getMetadata()`, and `listServices()` pure-virtual methods. Implement `GrpcBridgeImpl` in a dedicated API bridge implementation file (planned) that holds a `std::unordered_map<std::string, ServiceDescriptor>` guarded by `std::shared_mutex` and delegates routing to `GrpcApiServer::registerService()`.
- `[ ]` **`IGRPCBridge` has no integration tests**: add a dedicated gRPC bridge test target exercising service registration, duplicate-name rejection, and metadata lookup.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Test `graphql::Parser` new resolvers with `QueryLimits` boundary cases; mock `Changefeed` for subscription tests |
| Integration | All `/v1/` routes ≥ 95% | `tests/test_api_integration.cpp`; add WebSocket client tests for `/v2/changes` |
| Performance | Regression ≤ 5% on existing endpoints | Benchmark with `wrk` at 500 concurrent connections; alert on p99 regression |
| gRPC stub coverage | Advanced search and stream RPCs have integration tests | `tests/test_themisdb_grpc_service.cpp`; use `grpc::testing::MockServerWriter` for `StreamAQL` |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| GraphQL parse+execute (10-field query) | ~5 ms (estimate) | < 2 ms p99 | `tests/test_graphql_variables.cpp` + dedicated benchmark task |
| WebSocket concurrent connections | 0 (not implemented) | ≥ 10,000 | Load test with `k6` |
| Bulk insert 10K docs via `/v2/documents` | N/A | < 500 ms | `benchmarks/bench_api_endpoints.cpp` |
| Correlation ID middleware overhead | N/A | < 10 µs/req | microbenchmark in `benchmarks/bench_api_endpoints.cpp` |
| OTLP span flush (64 spans, persistent conn) | N/A | < 5 ms | planned OTLP microbenchmark target |
| `RateLimiter::allow()` throughput | ~200K calls/sec (est.) | ≥ 1M calls/sec | microbenchmark after shared_mutex migration |

## Security / Reliability

- `[x]` All WebSocket upgrade requests must be validated by `auth::JWTValidator` before the upgrade handshake completes; reject with HTTP 401 before protocol switch. (`WsChangeHandler::validate()` checks Bearer token / JWT using `AuthMiddleware::authorize` with `cdc:subscribe` scope before any handshake)
- `[x]` GraphQL `__schema` introspection disabled via `QueryLimits::allow_introspection = false`; `QueryLimits::production()` factory sets this to `false`; enforced in `Parser::parseField()`. Expose a config flag in `config/networking/` when a configuration layer is added.
- `[x]` Rate limiting middleware (`RateLimitingMiddleware`) is applied to all `/v2/` routes via `HttpServer::checkRateLimit()`; `/v2/documents` has a tighter per-endpoint override (50% of default capacity) to prevent bulk-insert abuse.
- `[ ]` **`QueryAllowList` disabled by default** (`include/api/persisted_queries.h::QueryAllowList`): `enabled_ = false` in the default constructor. In production deployments, the allow-list should be enforced to prevent ad-hoc query injection. Document the activation path (`QueryAllowList::instance().setEnabled(true)`) in the operations runbook and add a startup check that logs a `THEMIS_WARN` if the allow-list is disabled in a production build (detected via `NDEBUG`).
- `[ ]` **`BatchWrite` in gRPC service has no atomicity guarantee** (`themisdb_grpc_service.cpp`): individual document writes in `BatchWrite` are not wrapped in a `RocksDB::WriteBatch`. A server crash mid-loop leaves a partially applied batch with no way for the client to distinguish committed from uncommitted entries. Use `RocksDBWrapper::WriteBatchWrapper` (already in the codebase, used by `GeoIndexHooks::onEntityPutAtomic`) to make `BatchWrite` atomic.

---

## Scientific References

[1] Hartig, O., & Pérez, J. (2018). **Semantics and Complexity of GraphQL**. *Proceedings of the 2018 World Wide Web Conference (WWW)*, 1155–1164. https://doi.org/10.1145/3178876.3186014

[2] Fette, I., & Melnikov, A. (2011). **The WebSocket Protocol**. RFC 6455. IETF. https://doi.org/10.17487/RFC6455

[3] Grigorik, I. (2013). **High Performance Browser Networking**, Chapter 4: Transport Layer Security. O'Reilly Media. https://hpbn.co/transport-layer-security-tls/

[4] Suresh, V., & Nielsen, F. (2020). **gRPC: A Framework for High-Performance Client-Server Applications**. *IEEE Software*, 37(5), 26–32. https://doi.org/10.1109/MS.2020.2993646

[5] Montesi, F., & Weber, J. (2016). **Circuit Breakers, Discovery, and API Gateways in Microservices**. *arXiv preprint*. https://arxiv.org/abs/1609.05830

[6] Fielding, R. T. (2000). **Architectural Styles and the Design of Network-based Software Architectures** (Doctoral dissertation, University of California, Irvine). https://ics.uci.edu/~fielding/pubs/dissertation/top.htm

[7] Leitner, P., & Cito, J. (2016). **Patterns in the Chaos — A Study of Performance Variation and Predictability in Public IaaS Clouds**. *ACM Transactions on Internet Technology*, 16(3), 1–23. https://doi.org/10.1145/2885497

[8] Belshe, M., Peon, R., & Thomson, M. (2015). **Hypertext Transfer Protocol Version 2 (HTTP/2)**. RFC 7540. IETF. https://doi.org/10.17487/RFC7540

[9] Hunt, P., Konar, M., Junqueira, F. P., & Reed, B. (2010). **ZooKeeper: Wait-free Coordination for Internet-scale Systems**. *Proceedings of the 2010 USENIX Annual Technical Conference (ATC)*, 145–158. (Relevance: atomic write-batch semantics and distributed coordination patterns used in gRPC `BatchWrite` atomicity design.) https://www.usenix.org/conference/usenix-atc-10/zookeeper-wait-free-coordination-internet-scale-systems

[10] Mell, P., & Grance, T. (2011). **The NIST Definition of Cloud Computing**. NIST Special Publication 800-145. https://doi.org/10.6028/NIST.SP.800-145 (Relevance: multi-tenant API isolation requirements for per-tenant rate limiting and namespace routing.)
