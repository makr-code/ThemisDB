# Exponential Backoff with Jitter for Transient Failure Retry

**Metadaten:**
- Source: AWS Builder's Library — "Avoiding Thundering Herd: Exponential Backoff and Jitter"
- URL: https://aws.amazon.com/builders-library/timeouts-retries-and-backoff-with-jitter/
- Tags: reliability, distributed-systems
- ThemisDB-Versionen: v1.8.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

When a distributed component (upstream broker, peer node, remote storage) experiences a transient fault and multiple callers retry simultaneously at fixed intervals, the resulting synchronised burst — the "thundering herd" — can prevent the upstream from recovering. The AWS Builder's Library documents the standard mitigation: exponential backoff (delay doubles on each attempt) combined with uniform random jitter (delay is randomised within the exponential window) so that retries are naturally spread over time.

ThemisDB's Chimera subsystem (`src/chimera/`) implements this pattern via a composable `RetryPolicy` class in `retry_policy.hpp` paired with a `CircuitBreaker` that transitions between CLOSED, OPEN, and HALF_OPEN states to avoid retrying when a remote is clearly unavailable for an extended period.

## 🎯 Core Principles

- **Exponential base delay**: Each successive retry waits `base_delay_ms * 2^attempt`, capped at a `max_delay_ms` ceiling to prevent indefinitely long waits.
- **Full jitter**: The actual delay is `uniform_random(0, capped_exponential_delay)`. Full jitter outperforms "equal jitter" and "decorrelated jitter" in terms of average retry spread under load, per the AWS analysis.
- **Transient predicate**: Retry is attempted only when an `is_transient(error_code)` predicate returns `true`; permanent errors (authentication failure, schema violation) are never retried.
- **Max attempt cap**: A hard ceiling on retry count (e.g., 5 attempts) prevents infinite retry loops that could mask bugs or delay crash-fast behaviour.
- **Circuit Breaker integration**: After consecutive failures exceed a threshold, the circuit breaker opens and calls fail-fast without executing the operation, allowing the upstream to recover. The HALF_OPEN state permits a single probe before re-closing.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/chimera/retry_policy.hpp` — `RetryPolicy` template class; configurable `base_delay`, `max_delay`, `max_attempts`, `is_transient` predicate, jitter RNG seeded from `std::random_device`.
- `src/chimera/circuit_breaker.hpp` — `CircuitBreaker<State>` FSM with CLOSED/OPEN/HALF_OPEN states; `failure_threshold`, `recovery_timeout_ms` parameters.
- `src/chimera/` integration layer — MQTT reconnection, peer-sync retries, and replication lag recovery all use `RetryPolicy` + `CircuitBreaker`.

### What Was Adopted?

- `RetryPolicy::execute(callable, context)` wraps any operation: catches exceptions, checks `is_transient`, computes jittered delay via `std::uniform_int_distribution`, sleeps, and re-invokes up to `max_attempts`.
- `CircuitBreaker::call(callable)` checks state before forwarding; transitions to OPEN after `failure_threshold` consecutive failures; resets to CLOSED after a successful HALF_OPEN probe.
- Delay formula: `delay = uniform(0, min(max_delay_ms, base_delay_ms * (1 << attempt)))` where `attempt` is 0-indexed.
- Log lines include `attempt`, `delay_ms`, and `error_code` at DEBUG level for observability.
- OpenTelemetry span attributes `retry.attempt` and `retry.delay_ms` are set on each retry iteration (see `opentelemetry_tracing.md`).

### Deviations & Rationale

- **Decorrelated jitter not used**: AWS recommends testing decorrelated jitter for certain workloads. ThemisDB uses full jitter because it is simpler and the AWS analysis shows it has equivalent or better performance for the target retry counts (≤5).
- **No adaptive backoff based on server hints**: HTTP `Retry-After` headers or broker-specific backoff hints are not currently parsed; static parameters are used. Tracking issue exists for v2.2.0.
- **Circuit breaker per-remote, not per-operation-type**: A single `CircuitBreaker` instance guards all operations to a given remote endpoint rather than per operation class. This is a deliberate simplification for the current topology.

## ⚠️ Trade-offs & Limitations

- **Latency tail increase**: Retries add latency to the failed request. With `max_attempts=5` and `base_delay=50 ms`, worst-case tail latency can exceed 3 seconds. Callers must account for this in their timeout budgets.
- **Resource consumption during open circuit**: Operations fail immediately when the circuit is OPEN, which is the desired behaviour. However, callers that do not handle `CircuitOpenException` gracefully may surface errors to end users during recovery windows.
- **Jitter requires PRNG per thread**: `std::mt19937` seeded from `std::random_device` is created per `RetryPolicy` instance. For very short-lived connections this incurs initialisation cost; a thread-local RNG pool would be more efficient.
- **Not suitable for non-idempotent operations**: Retrying a non-idempotent write can produce duplicate effects. `is_transient` predicates must be written conservatively for write paths.

## 🔬 Validation

- [x] Code reviewed against AWS Builder's Library article and jitter analysis
- [x] Unit tests in `tests/chimera/retry_policy_test.cpp` verify delay distribution and circuit state transitions
- [x] Integration tests simulate flapping upstream and verify convergence within `max_attempts`
- [x] Module README linked (`src/chimera/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [OpenTelemetry Tracing](opentelemetry_tracing.md)
- [Consistent Hash Ring](consistent_hash_ring.md)

---
**Last Updated:** 2026-04-06
