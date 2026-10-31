---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Network Control and Routing Models

## Purpose and scope

This document describes the explicit control models implemented by
`src/network/`: retry delay, circuit breaking, topology selection, and bounded
UDP duplicate tracking. It is for developers changing those decisions. These
are deterministic software policies over configured state and observations;
the module does not train a statistical network model.

## Retry delay

`RetryPolicy` in
[`wire_protocol_server.h`](../../include/network/wire_protocol_server.h)
classifies `ECONNRESET`, `ETIMEDOUT`, `EAGAIN`, and `EWOULDBLOCK` as transient.
For zero-based retry attempt `n`, its documented delay is:

```text
min(base_delay_ms × 2^n + U(0, jitter_ms), max_delay_ms)
```

`computeDelay()` returns a duration and does not sleep. The caller owns retry
execution and must stop at its retry budget. This policy does not make an
operation safe to replay; callers still need idempotency or an explicit
delivery outcome.

## Circuit breaker

`AdaptiveCircuitBreaker` in
[`adaptive_circuit_breaker.h`](../../include/network/adaptive_circuit_breaker.h)
uses three states:

- **CLOSED:** requests are allowed; consecutive failures can trip the breaker.
- **OPEN:** requests are rejected until `open_timeout` elapses.
- **HALF_OPEN:** probe requests test recovery; a failure reopens the breaker,
  while the configured consecutive-success threshold closes it.

Optional adaptive thresholds adjust the effective trip threshold after
repeated trips or sustained recovery. Optional per-error-class thresholds
track configured error labels independently and can trip the same breaker.
Unknown error classes still contribute to global accounting. A state-change
callback is invoked while the breaker mutex is held, so keep it short and do
not call back into code that may acquire that lock.

The breaker is a local admission decision, not a health probe or proof that a
remote dependency is healthy. Operators should inspect its state and rejection
counters together with the underlying connection errors.

## Topology routing

`GeoTopologyRouter` consumes healthy-shard information from
`sharding::ShardTopology`; it does not own sockets, establish connections, or
perform health checks. Its policies are:

- `PREFER_LOCAL`: rank by locality (same region, zone, then datacenter);
  optionally fall back across regions.
- `LOWEST_LATENCY`: select using configured per-region one-way latency hints;
  without hints it falls back to local preference.
- `ROUND_ROBIN`: distribute across healthy shards without locality scoring.

An empty endpoint is a normal failure result when no eligible healthy shard
exists or cross-region fallback is disabled. A latency hint is configuration
input, not a measured live RTT. The caller must handle an empty result and
obtain a connection through the connection-pool layer.

## UDP duplicate and rate controls

`UDPServer` keys rate and sequence-number state by source IP. Its configured
per-IP packet rate uses a one-second window. Duplicate detection retains a
bounded FIFO set of recent sequence numbers per source; once the configured
window is full, older values are evicted. Therefore deduplication is bounded
and does not prevent a replay after eviction. This UDP path remains
best-effort: packet loss and reordering are possible, and duplicate tracking
does not provide durable exactly-once processing.

## Evaluation

The focused breaker and routing regression suites are named in
[`ROADMAP.md`](ROADMAP.md) and registered by
[`tests/network/CMakeLists.txt`](../../tests/network/CMakeLists.txt).
Performance targets are listed in
[`bench_network_routing_gates.cpp`](../../benchmarks/network/bench_network_routing_gates.cpp);
they are gates for isolated paths, not measured deployment capacity. See
[`RESEARCH.md`](RESEARCH.md) for the limits of benchmark evidence.
