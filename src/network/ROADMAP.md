> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Network Module Roadmap

## Current Status
Production-grade transport and protocol layer with TCP wire protocol, WebSocket, UDP paths, QUIC/HTTP3, and gRPC components in active use.

## In Progress
- [~] Network hardening wave for protocol safety, transport resilience, and predictable latency behavior (Target: Q3 2026)
  - [ ] Complete remaining failure-injection coverage for multi-transport edge cases (Target: Q3 2026)
  - [ ] Tighten auth/rate-limit/session guard behavior under sustained adversarial traffic (Target: Q3 2026)
- [~] approved next implementation block: schedule mixed-transport failure injection and lifecycle/backpressure/timeout validation after Graph + LLM kickoff evidence is secured (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Expand transport-level regression coverage for mixed TCP/WS/UDP/QUIC deployments (Target: Q4 2026)
- [ ] Strengthen distributed routing and failover diagnostics for operator triage (Target: Q4 2026)
- [ ] Harden connection lifecycle guardrails (limits, backpressure, timeout interplay) under peak load (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Improve protocol-path performance consistency with benchmark-backed promotion gates (Target: Q1 2027)
- [ ] Expand resilience validation for mesh/topology-aware routing under partial failures (Target: Q1 2027)
- [ ] Advance transport observability and security telemetry fidelity across all network front doors (Target: Q1 2027)

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity — 3.5 Network** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

- [ ] **HTTP/3 QUIC production enablement**: promote the existing QUIC/HTTP3 implementation from
  experimental to production-default for external API endpoints; validate connection migration and
  0-RTT resumption under realistic packet-loss profiles (Target: Q3 2026)
  - Acceptance: QUIC connection migration test passes at 1% simulated packet loss; 0-RTT handshake
    succeeds on reconnect within 50 ms; HTTP/3 throughput ≥ HTTP/2 baseline; `release_critical` green
- [ ] **Zero-copy socket I/O**: wire `io_uring` (Linux ≥ 5.12) or `sendfile`/`splice` for large
  payload sends on TCP and QUIC paths; fall back to standard `send()` on platforms without support (Target: Q4 2026)
  - Inputs: payload size threshold (default 64 KB), platform capability probe at startup
  - Acceptance: throughput for large payloads (≥ 1 MB) increases ≥ 20% vs. copy-send baseline;
    CPU utilization decreases ≥ 15% under sustained large-payload load; no regression for small
    payloads; `release_critical` green

## Implementation Phases

### Phase 1: Protocol and Session Safety
- [ ] Re-validate auth/session checks and frame/input validation across all major opcode/transport paths (Target: Q3 2026)
- [ ] Extend deterministic regression packs for malformed frame and rate-limit abuse scenarios (Target: Q3 2026)

### Phase 2: Multi-Transport Resilience
- [ ] Strengthen failure handling across TCP, WebSocket, UDP, QUIC/HTTP3, and gRPC paths (Target: Q4 2026)
- [ ] Validate transport fallback/retry behavior under network degradation (Target: Q4 2026)
- [ ] Use the follow-on hardening block to prioritize transport fault injection before broader routing diagnostics work (Target: Q4 2026)

### Phase 3: Routing and Topology Hardening
- [ ] Expand routing correctness checks under changing health/latency/topology signals (Target: Q4 2026)
- [ ] Harden load-balancing state transitions and recovery behavior under churn (Target: Q4 2026)

### Phase 4: Performance and Operational Hardening
- [ ] Re-baseline protocol throughput and tail-latency envelopes across representative production mixes (Target: Q1 2027)
- [ ] Keep compression/batching/zero-copy overhead bounded under high concurrency (Target: Q1 2027)

### Phase 5: Documentation and Release Readiness
- [ ] Keep network docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist
- Status: Tracking in progress
- Nachweise: network focused tests, transport integration tests, protocol security regressions, performance suites
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some cross-transport fault combinations still need broader evidence under long-duration stress.
- Certain route/mesh/topology scenarios require additional benchmark and resilience validation.
- Operator-facing diagnostics for complex multi-transport incidents continue to be refined.

## Breaking Changes
- Network public APIs and protocol evolution in active major lines remain additive-first.
- Any behavior change requiring client migration must be versioned and documented in changelog/migration notes.
