# HTTP Graceful Shutdown Hardening

**Module:** `server` — `http_shutdown_manager.h` / `http_shutdown_manager.cpp`  
**Delivery:** Block C (P5-S02)  
**Status:** Production-Ready (v1.0.0)

---

## Overview

`HttpShutdownManager` replaces the inline ad-hoc drain loop in
`HttpServer::stop()` with a well-defined, bounded, multi-phase shutdown
sequence.  Each phase is observable, logged, and has an explicit timeout.

---

## Shutdown Phases

```
IDLE → DRAINING → FORCE_CLOSE → TEARDOWN → DONE
```

| Phase | Enum | Description |
|---|---|---|
| IDLE | `kIdle` | Not yet started. |
| DRAINING | `kDraining` | Acceptor closed; polling in-flight counter until drain deadline or all requests finish. |
| FORCE_CLOSE | `kForceClose` | Drain deadline elapsed; `force_close_sessions` callback invoked; waits up to `force_close_timeout_ms`. |
| TEARDOWN | `kTeardown` | All requests resolved; subsystems torn down. |
| DONE | `kDone` | Shutdown complete. `run()` returns. |

---

## Key Properties

- **Bounded:** every phase has an explicit timeout — no phase can block forever.
- **Thread-safe:** `phase_` is `std::atomic<ShutdownPhase>`.
- **Observable:** `phase()`, `isDone()`, `forcedCount()`, `drainElapsedUs()`.
- **Zero allocations after construction.**

---

## Constructor Parameters

| Parameter | Description |
|---|---|
| `drain_timeout_ms` | Max wait in DRAINING phase (ms). 0 = skip drain entirely. |
| `force_close_timeout_ms` | Budget for FORCE_CLOSE phase (ms). |
| `query_in_flight` | Callable returning current in-flight count. Must not be null. |
| `force_close_sessions` | Optional callback to force-cancel active sessions. |

---

## Integration in `HttpServer::stop()`

```cpp
themis::server::HttpShutdownManager shutdown_mgr(
    config_.graceful_shutdown_timeout_ms,
    HttpShutdownManager::kDefaultForceCloseTimeoutMs,
    [this]() -> uint64_t {
        return active_requests_.load(std::memory_order_acquire);
    });
shutdown_mgr.run();
if (shutdown_mgr.forcedCount() > 0) {
    THEMIS_WARN("Forced: {} request(s) still in flight", shutdown_mgr.forcedCount());
}
```

---

## Edge Cases

| Scenario | Behaviour |
|---|---|
| `drain_timeout_ms = 0` | Skips DRAINING immediately; proceeds to FORCE_CLOSE. |
| All requests complete before drain deadline | DRAINING exits early; `forcedCount = 0`. |
| Requests survive force-close | `forcedCount > 0`; TEARDOWN proceeds anyway. |
| `force_close_sessions` is null | FORCE_CLOSE still waits for `force_close_timeout_ms` and records remaining count. |

---

## Gate Criteria (Block C / P5-S02)

- `HttpServer::stop()` uses phased shutdown via `HttpShutdownManager`.
- All 16 `HSH-01..HSH-16` integration tests pass.
- No regression in existing HTTP server test suite.
