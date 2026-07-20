# Wire Protocol Retry Policy

**Module:** `network` — `wire_retry_policy.h` / `wire_retry_policy.cpp`  
**Delivery:** Block C (P5-S01)  
**Status:** Production-Ready (v1.0.0)

---

## Overview

The wire protocol retry subsystem provides a configurable, jitter-aware
exponential-backoff retry policy used throughout the ThemisDB wire protocol
server.  It replaces the ad-hoc linear-backoff retry loops that previously
existed inline in `wire_protocol_server.cpp`.

---

## Components

### `WireRetryPolicy`

A value-type (cheap to copy) struct holding all retry parameters:

| Field | Default | Description |
|---|---|---|
| `max_attempts` | 3 | Maximum retry attempts after the first try. |
| `base_delay_ms` | 100 | Base delay for the first retry (ms). |
| `max_delay_ms` | 30 000 | Hard cap on computed delay (ms). |
| `multiplier` | 2.0 | Exponential multiplier applied per attempt. |
| `enable_jitter` | true | If true, apply full-jitter (uniform in [0, delay]). |

Three named factory methods cover the common cases:

| Factory | Use-case | max_attempts | base_delay_ms | jitter |
|---|---|---|---|---|
| `forBindListen()` | Server bind/listen start-up | 3 | 100 | off |
| `forConnectionIO()` | Per-connection transient I/O errors | 5 | 50 | on |
| `forTesting()` | Unit tests (no real sleeping) | 3 | 0 | off |

### `RetryContext`

Mutable state tracking a single retry sequence.  Owned per call-site; not
thread-safe.  Key methods:

- `canRetry()` — returns true when more attempts remain.
- `nextDelay(WireErrorClass)` — advances the counter and computes the next
  sleep delay.  Returns `std::nullopt` on exhaustion or permanent errors.
- `reset()` — restarts the sequence against the same policy.

### `classifyBoostError()`

Maps a `boost::system::error_code` to `WireErrorClass`:

- **kTransient** — `EAGAIN`, `EINTR`, `ETIMEDOUT`, `ENOBUFS`, `ENOMEM`,
  `connection_reset`, `eof`, `timed_out`, `try_again`, `would_block`
- **kPermanent** — `EINVAL`, `EADDRINUSE`, `EACCES`, `connection_refused`,
  `operation_aborted`
- **kUnknown** — anything not explicitly classified (treated as transient)

### `retryWithPolicy()`

Generic retry executor: invokes a `bool()` callable in a loop driven by
`WireRetryPolicy`.  Swallows exceptions from the callable (treated as
transient).  Accepts an optional `on_fail` callback for logging.

---

## Delay Formula

```
computed = min(base_delay_ms * multiplier^attempt, max_delay_ms)
if enable_jitter: delay = uniform_random(0, computed)
else:             delay = computed
```

---

## Integration Points

### Bind/Listen start-up

`wire_protocol_server.cpp` — `start()` now delegates to `retryWithPolicy`
with `WireRetryPolicy::forBindListen()`:

```cpp
const auto policy = WireRetryPolicy::forBindListen();
const bool ok = retryWithPolicy(policy,
    [&]() -> bool { /* open/bind/listen */ },
    [&](uint32_t attempt, int64_t delay_ms) {
        // log failure
    });
```

---

## Gate Criteria (Block C / P5-S01)

- Bind/listen failures use policy-driven exponential backoff.
- All 16 `WPR-01..WPR-16` integration tests pass.
- No regression in existing wire-protocol test suite.
