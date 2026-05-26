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

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v2  
**Auto-Generated:** Yes
