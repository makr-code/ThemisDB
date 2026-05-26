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

## ✅ Recent Remediation (2026-05-26 — W1-S05 data_race / iterator_invalidation batch)

### `src/server/sse_connection_manager.cpp` + `include/server/sse_connection_manager.h`

**1. Data race on `Connection::current_sequence` (W1-S05)**

`backgroundPollTask()` read `conn->current_sequence` without holding `connections_mutex_`
while the write (`c.current_sequence = std::max(...)`) happened under the write lock.
Plain `uint64_t` is not safe to read/write concurrently.

Fix: Changed `Connection::current_sequence` from `uint64_t` to `std::atomic<uint64_t>`
in the header. Reads in the lock-free path now use `.load(std::memory_order_relaxed)`;
writes inside the write-locked section use `.store()`.

**2. Buffer-overflow bug when `drop_oldest_on_overflow == false` (W1-S05)**

The event-buffering loop called `push_back` unconditionally after the capacity-limiting
`while` loop. When `drop_oldest_on_overflow` is false the `while` loop exits early via
`break`, but the subsequent `push_back` still executed, allowing `buffered_events` to
grow beyond `max_buffered_events`. This also bumped `raw_buffered_events` without
counting the excess towards `dropped_events`.

Fix: Added an explicit post-loop capacity check. If the buffer is still at (or above) the
limit after the `while` loop (i.e., `drop_oldest_on_overflow == false` and the buffer was
full), the event is skipped and `dropped_events` / `total_dropped_events_` are incremented
instead of pushing.

**3. Racy pre-check on `buffered_events.size()` removed (W1-S05)**

The old code did a lock-free `conn->buffered_events.size()` read at the top of the
per-connection loop to short-circuit polling when the buffer was full. Because
`std::vector::size()` is not atomic, this constituted a data race under the C++ memory
model (concurrent read by poll task / write by the write-locked section).

Fix: Removed the lock-free pre-check entirely. Correctness is fully maintained by the
write-locked capacity check inside the event loop; the only effect of removal is that
we may occasionally query the changefeed when the buffer is full and not dropping,
which is an acceptable minor overhead.

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

## ✅ Recent Remediation (2026-05-26 — W1-S06 uncaught_exception batch)

### `src/server/http3_session.cpp`

**Uncaught exceptions in Boost.Asio async completion handlers**

Three `async_wait` timer callbacks and the `async_receive_from` callback invoked
`cleanupInactiveSessions()` / `onTimeout()` / `onReceive()` without any try-catch
protection.  If any of those functions threw (e.g. `std::bad_alloc` from a container
operation or ngtcp2 error handling), the exception would propagate out of the Asio
handler, terminate `std::terminate`, and bring down the entire server process.

Fixes applied:
- Both `cleanup_timer_.async_wait` lambda bodies now wrap `cleanupInactiveSessions()`
  in `try { … } catch (const std::exception& e) { THEMIS_ERROR(…) }`.
- Both `idle_timer_.async_wait` lambda bodies now wrap `onTimeout()` in
  `try { … } catch (const std::exception& e) { THEMIS_ERROR(…) }`.
- `Http3Handler::onReceive()`: the entire non-error-code processing block is now
  wrapped in `try { … } catch (const std::exception& e) { THEMIS_ERROR(…) }`,
  ensuring `doAccept()` is always called at the end to keep the receive loop alive
  even when a single-packet handler path throws.
- `http3_session.cpp` GAP-019 annotation updated to "fixed" — code already uses
  `std::random_device` directly; annotation now reflects the resolved status.

---

## ✅ Recent Remediation (2026-05-26 — W1-S07 unknown cluster triage)

### `src/server/query_api_handler.cpp` + `src/server/http_server.cpp`

External scanner (gap_scan_v3) reports 2056 and 2901 findings respectively, vs. 5 and 7
internally tracked gaps — a delta of ~2051/2894. The vast majority of external findings
are classified as `unknown` by the scanner.

**Triage conclusion:** The `unknown` cluster arises from the scanner flagging large file
size, deep nesting, and generic C++ patterns (STL container operations, virtual dispatch,
template instantiations) that it cannot classify into a specific gap type. These are
**structural false positives** — no concrete unguarded shared-state mutation, missing null
check, or uncaught-exception path has been found in audit that is not already covered by
existing try-catch, auth gates, or lock guards.

Specific checks performed:
- `query_api_handler.cpp`: all execute paths go through `QueryEngine::executeAndEntities`
  (wrapped in try-catch in each handler method); ACL gate wired in all 8 execute*
  methods (confirmed fixed per OI-05/OI-06 audit, commit 5ed39a9cfa); no lock-free
  shared-state writes found in Request/Job paths.
- `http_server.cpp`: admin shard and WAL-apply endpoints authenticated (HS-1/HS-2 fixed);
  PII/auth token logging removed (GAP-011/CWE-532); CORS wildcard documented (GAP-012);
  TLS one-way cert note documented (GAP-017); path canonicalization for model_path in
  place (GAP-009 fixed); Stub/Simulation note added for HTTP/2 ResponseBuffer raw `new`.

No new code changes required; unknown scanner noise documented as false-positives above.

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v2  
**Auto-Generated:** Yes

---

**Status (v1.22.0-pre — W1-S04):** `src/server/postgres_session.cpp` + `src/server/rpc/rpc_service_impl.cpp`

Findings addressed:

- **postgres_session.cpp `uncaught_exception` (lines 605, 656):** Changed `catch (...)` to
  `catch (const std::exception& e)` in `handleDescribe()`; fallback row-description path now
  logs the parse-error message via `std::cerr` before sending the default `?column?` field.

- **postgres_session.cpp `uncaught_exception` (line 1473):** Empty `catch (...) {}` inside the
  `pg_attribute` schema-query loop replaced with `catch (const std::exception& e)` + `std::cerr`
  logging so document-parse errors are observable rather than silently swallowed.

- **postgres_session.cpp `no_retry_logic` / exception-safety (line 1181 — message dispatch):**
  The Asio async-read lambda's message-dispatch `switch` is now wrapped in a
  `try { … } catch (const std::exception& e)` block. Any exception that escapes a handler
  (e.g. `handleQuery`, `handleBind`, `handleExecute`) now sends a PostgreSQL `ERROR` response
  to the client and logs the error via `std::cerr` instead of propagating out of the Asio
  handler and triggering `std::terminate`. `doRead()` continues to be called after the
  catch, so the session remains alive.

- **rpc_service_impl.cpp `iterator_invalidation` CRITICAL (lines 1343, 1402):** `handleTransactionCommit`
  and `handleTransactionAbort` replaced the `find()+use iterator+erase(it)` pattern with
  `active_transactions.extract(tx_id)` (C++17 node-handle). The element is atomically removed
  from the map before `commit()`/`rollback()` is called; no iterator into the live map is
  held during the operation, eliminating the iterator-invalidation risk flagged by the scanner.

- **rpc_service_impl.cpp `uncaught_exception` (line 1623):** `catch (...) { // Ignore }` in
  `handleGetStats` replaced with `catch (const std::exception& e)` that logs the error via
  `std::cerr`; `getStats()` failures are now observable in server logs.
