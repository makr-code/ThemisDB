# server Module — Implementation Gap Analysis

**Status:** In Progress  
**Last Updated:** 2026-05-19  

---

## 📊 Gap Summary

This module's gap analysis is pending. Run the gap audit to populate this document:

```bash
python tools/gap_audit_pipeline_v2.py
```

---

## ✅ Recent Remediation (2026-05-19)

- **W1-S09 (2026-05-26) – `include/server/mcp_server.h`, `src/server/mcp_server.cpp`**
  - MCP transport lifecycle hardening: `StdioTransport`, `SseTransport`, and
    `WebSocketTransport` now use atomic `is_running_` flags and idempotent
    compare-exchange/exchange start-stop transitions instead of unsynchronized
    plain `bool` state writes.
  - Cross-thread access hardening: transport `send`, timer callbacks, and stdin
    read loops now gate lifecycle checks via atomic loads/stores, removing
    data-race-prone mixed-thread reads during shutdown and timer rescheduling.
  - Gap delta intent: reduce `data_race` findings in MCP transport lifecycle
    and async callback paths while preserving current transport semantics.

- **W1-S08 (2026-05-26) – `include/server/mcp_server.h`, `src/server/mcp_server.cpp`, `src/server/voice_api_handler.cpp`**
  - Concurrency hardening in MCP lifecycle: `McpServer::is_running_` and
    `initialized_` are now atomic state flags; `start()` now uses
    compare-exchange admission and `stop()` uses atomic exchange teardown to
    prevent concurrent start/stop races from observing stale lifecycle state.
  - Null-safety hardening in Voice API request dispatch: `handleRequest()` now
    fail-closes with `503 Service Unavailable` when `voice_assistant_` is not
    attached, preventing null-dereference paths across voice route handlers.
  - Gap delta intent: reduce `data_race` findings in `mcp_server` lifecycle
    paths and `null_dereference` findings in `voice_api_handler` request paths.

- **W1-S07 (2026-05-26) – `src/server/import_api_handler.cpp`, `include/importers/importer_interface.h`**
  - Import-job registry hardening: added snapshot-oriented helpers on
    `ImportJobRegistry` (`getJsonSnapshot`, `getRunningAndJsonSnapshot`,
    `getSourcePathSnapshot`, `allJsonSnapshots`) to centralize thread-safe
    access and reduce direct shared-handle retrieval in API handlers.
  - Handler updates: import job status/cancel/list/metrics/schema/relationship
    routes now consume registry snapshots instead of direct `get()/all()` handle
    access paths.
  - Gap delta intent: reduce import API `data_race` hotspots around registry
    access while preserving endpoint behavior and metrics output shape.

- **W1-S06 (2026-05-26) – `src/server/async_job_api_handler.cpp`, `include/server/async_job_api_handler.h`**
  - Concurrency-access encapsulation: added `AsyncJobRegistry` snapshot/cancel helpers
    (`getJsonSnapshot`, `allJsonSnapshots`, `requestCancel`) so route handlers no longer
    directly fetch mutable job pointers for list/status/cancel paths.
  - Handler hardening: `handleList`, `handleGetStatus`, and `handleCancel` now operate
    on registry-provided snapshots/results, reducing lock-hand-off surface between
    registry map access and per-record mutation.
  - Gap delta intent: reduce async-job `data_race`/iterator-handling false-positive
    hotspots and keep thread-safe access centralized in the registry boundary.

- **W1-S05 (2026-05-26) – `src/server/websocket_session.cpp`, `include/server/websocket_session.h`**
  - Executor-affinity hardening: public `send()` / `sendBinary()` now always
    marshal onto the WebSocket stream executor via `net::dispatch(...)` before
    touching Beast stream state. This removes cross-thread direct `async_write`
    and `set text()/binary()` calls from external threads (manager/poller paths).
  - Added private helpers (`sendOnExecutor`, `sendBinaryOnExecutor`,
    `startWriteLocked`, `closeInternalErrorOnExecutor`) so write-queue mutation
    and stream writes run on the same executor context.
  - `onWrite()` now reuses `startWriteLocked()` for the next frame, keeping the
    write-start logic single-source and reducing divergence risk.
  - Gap delta intent: reduce async thread-affinity / data-race findings in
    WebSocket write paths during the next server audit rescan.

- **W1-S04 (2026-05-26) – `src/server/websocket_session.cpp`**
  - Back-pressure close robustness: `send()` / `sendBinary()` now detect the
    `queue full + !writing_` state and explicitly dispatch a 1011 close on the
    stream executor. This closes immediately when no in-flight write exists,
    instead of relying solely on `onWrite()` (which never fires in that state).
  - Thread-safety: immediate-close path now updates `close_due_to_backpressure_`
    under `write_mutex_`, keeping flag mutations synchronized with `onWrite()`.
  - Gap delta intent: eliminate stale-session edge case after back-pressure
    saturation (`no_timeout`/lifecycle drift class in async close handling).

- **W1-S03 (2026-05-26) – `src/server/websocket_session.cpp`, `include/server/websocket_session.h`**
  - Data-race fix: `active_` changed from `bool` to `std::atomic<bool>`. All reads now use
    `load(acquire)` via `isActive()`; all writes use `store(release)` or `exchange(acq_rel)`.
    This eliminates the data race between the io_context read/write handlers and external callers
    such as `WebSocketManager::closeAll()` / `pollCDCEvents()`.
  - Thread-safe `close()`: now uses `atomic::exchange` so that concurrent calls are idempotent,
    and dispatches `doClose()` onto the stream's executor via `net::dispatch(executor, ...)`.
    Previously, a direct synchronous call to `ws.close()` from a non-io_context thread (e.g.
    the destructor thread) while an `async_read` was pending was undefined behaviour.
  - `send()` back-pressure path: the synchronous `ws.close()` call inside the `write_mutex_`
    lock (which could race with async I/O) is replaced by setting `close_due_to_backpressure_`
    so that `onWrite()` issues the 1011 close frame once the current write drains on the
    correct io_context thread.
  - Gap delta intent: resolve `data_race` (WebSocket active_ flag) and `no_thread_safety`
    (close from external thread) findings in next server rescan.

- **W1-S03 (2026-05-26) – `src/server/http3_session.cpp`**
  - GAP-019 annotation updated: `generateConnectionIdCallback` already uses `std::random_device`
    directly for cryptographic-quality QUIC connection ID generation. Comment updated from
    "GAP-019 open" to "GAP-019 fixed" to reflect the existing correct implementation.

- **W1-S02 (2026-05-26) – `src/server/http_server.cpp`**
  - Data-race hardening: introduced `std::atomic<uint32_t> hot_request_timeout_ms_` as an atomic shadow for `config_.request_timeout_ms`. Hot-reload path (request thread) now writes via `store(release)`; `Session::armReadTimer()` and `SslSession::armReadTimer()` now read via `load(acquire)`. Eliminates the data race between hot-reload and I/O-thread timer arming.
  - Iterator-invalidation / unbounded growth fix: `enforceAuditRateLimit` now performs amortised eviction of stale `audit_rate_buckets_` entries (entries older than 2 × window_ms) under the existing `audit_rate_mutex_`, triggered when the bucket map exceeds 128 entries. Prevents unbounded memory growth under adversarial request patterns.
  - No-timeout hardening: `Session::doWrite()` / `SslSession::doWrite()` now arm the same per-connection timeout guard used for reads, and `onWrite()` cancels it after completion. `SslSession::doShutdown()` now also arms/cancels the guard to avoid indefinite shutdown hangs on stalled peers.
  - Connection-admission hardening: `onAccept` now atomically reserves a connection slot (`compare_exchange_weak`) before session creation, preventing over-admission races around `max_connections` under concurrent accepts.
  - Gap delta intent: reduce `data_race` (Session/SslSession armReadTimer, hot-reload), `iterator_invalidation` (audit_rate_buckets_), and `no_timeout` (write/shutdown async paths) findings for `http_server.cpp` in next server rescan.

- **W1-S02b (2026-05-26) – `src/server/http2_session.cpp`, `include/server/http2_session.h`, `src/server/http_server.cpp`**
  - HTTP/2 admission parity: accept-path reserved connection slots are now transferred into `Http2Session` via `connection_slot_reserved` instead of being decremented before session start. This keeps accounting symmetric with HTTP/1.1/TLS sessions and removes a transient undercount window for HTTP/2 accepts.
  - `Http2Session` now participates in `active_connections_` ownership semantics: constructor increments only when no reservation was pre-held, destructor always decrements.
  - Timeout hardening: HTTP/2 handshake/read/write paths now arm/cancel per-connection timeout guards via `hot_request_timeout_ms_` (read and write timers), preventing indefinite hangs on stalled peers.
  - Lifecycle robustness: timeout handlers now capture `weak_ptr` (instead of strong `shared_ptr`) and timers are canceled during destructor teardown, preventing timeout-handler ownership from artificially extending session lifetime and delaying `active_connections_` release.
  - Structural cleanup: removed duplicated `response_buffers_` member declaration and redundant double-erase in stream-close callback to keep response-buffer ownership single-source and predictable.
  - Added focused protocol-level tests in `tests/test_http2_protocol.cpp` that verify reserved-slot transfer and unreserved-session ownership accounting semantics.
  - Gap delta intent: reduce `data_race`/admission-accounting drift around `max_connections` and `no_timeout` findings for HTTP/2 accept/I-O paths in next server rescan.

- **W1-S01 (2026-05-26) – `src/server/query_api_handler.cpp`**
  - Data-race hardening: thread-safe access for runtime-injected `IndexRecommender`, `StatisticsCollector`, and masking policy snapshots in request paths.
  - Timeout hardening: explicit timeout aborts added for traversal BFS and LET projection fallback prefix-scan paths (`timeout_ms`).
  - Null-safety hardening: fail-closed `503 Service Unavailable` guards for missing core query dependencies (`storage`, `secondary_index`) and missing enhanced-query `llm_store`.
  - Gap delta intent: reduce `data_race`, `no_timeout`, and `null_dereference` findings for `query_api_handler.cpp` in next server rescan.

- Removed token-value logging from:
  - `src/server/auth_middleware.cpp` (`AuthMiddleware::authorize`)
  - `src/server/http_server.cpp` (`HttpServer::handlePiiDeleteByUuid`)
- Removed Authorization header value logging and temporary `[AUTH-DBG]` stderr diagnostics from `src/server/http_server.cpp` (`HttpServer::requireAccess`).
- Removed detailed auth decision logging (`user_id` / `reason`) and token-validation diagnostics in `src/server/http_server.cpp` PII-delete authorization paths.
- Removed startup `validateToken` debug block in `HttpServer` constructor that ran on every server start and logged `user_id`/`reason` without operational value (GAP-011/CWE-532 clean-up).
- Added STUB/SIMULATION NOTE to HTTP/2 server-push `ResponseBuffer` raw `new` usage documenting the nghttp2 C API constraint and the planned migration to shared_ptr (see `FUTURE_ENHANCEMENTS.md §HTTP2 BufferManagement`).
- Security impact: avoids leaking bearer token fragments and internal validation state into logs (CWE-532 hardening).
- Related test coverage: `tests/test_auth_middleware.cpp`
  - `AuthMiddlewareGap013Test.DeniedReason_DoesNotEchoPresentedToken`
  - `AuthMiddlewareGap013Test.InsufficientScope_ReasonDoesNotEchoToken`
  - `AuthMiddlewareGap013Test.ValidateToken_ReasonDoesNotEchoToken`
  - `AuthMiddlewareGap013Test.ConcurrentDenyRequests_NoCrossContamination`

---

## 🚀 How to Use This Documentation

Once generated, this file will contain:

- **Gap Statistics:** Count of unimplemented paths, TODOs, STUBs, etc.
- **Critical Issues:** What needs to be fixed first
- **Implementation Roadmap:** Phases and priorities
- **Affected Files:** Which source files have gaps
- **GitHub Issues:** Links to related GitHub issues
- **Next Steps:** Action items for developers

---

## 📍 Location

This documentation is in the module directory for easy access:
```
src/server/MODULE_GAPS.md  ← You are here
```

Developers working on this module can reference this file directly.

---

## 🔄 How It's Updated

The documentation is automatically generated and updated by the gap audit pipeline:

```bash
# Full pipeline (scan + update headers + generate docs)
python tools/gap_audit_pipeline_v2.py

# Just generate module docs
python tools/module_doc_generator.py . ai_working ai_working/module_gaps
```

After each run, this file is updated with fresh analysis.

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v2  
**Auto-Generated:** Yes
