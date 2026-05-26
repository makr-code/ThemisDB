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

- **W1-S02 (2026-05-26) – `src/server/http_server.cpp`**
  - Data-race hardening: introduced `std::atomic<uint32_t> hot_request_timeout_ms_` as an atomic shadow for `config_.request_timeout_ms`. Hot-reload path (request thread) now writes via `store(release)`; `Session::armReadTimer()` and `SslSession::armReadTimer()` now read via `load(acquire)`. Eliminates the data race between hot-reload and I/O-thread timer arming.
  - Iterator-invalidation / unbounded growth fix: `enforceAuditRateLimit` now performs amortised eviction of stale `audit_rate_buckets_` entries (entries older than 2 × window_ms) under the existing `audit_rate_mutex_`, triggered when the bucket map exceeds 128 entries. Prevents unbounded memory growth under adversarial request patterns.
  - No-timeout hardening: `Session::doWrite()` / `SslSession::doWrite()` now arm the same per-connection timeout guard used for reads, and `onWrite()` cancels it after completion. `SslSession::doShutdown()` now also arms/cancels the guard to avoid indefinite shutdown hangs on stalled peers.
  - Connection-admission hardening: `onAccept` now atomically reserves a connection slot (`compare_exchange_weak`) before session creation, preventing over-admission races around `max_connections` under concurrent accepts.
  - Gap delta intent: reduce `data_race` (Session/SslSession armReadTimer, hot-reload), `iterator_invalidation` (audit_rate_buckets_), and `no_timeout` (write/shutdown async paths) findings for `http_server.cpp` in next server rescan.

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
