---
Author: "ThemisDB Contributors"
Created: "unknown (pre-existing document; creation date not recorded)"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.
>
> Statuses below describe tracked work; benchmark gates and contract tests do
> not establish live transport performance or production deployment readiness.
>
> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Network Module Roadmap

## Current Status
Transport and protocol implementation surfaces include TCP wire protocol, WebSocket, UDP, optional QUIC/HTTP3, and gRPC. The public contract and focused tests cover frame/session guards, bounded lifecycle behavior, retries, and routing. However, optional build flags and runtime wiring vary by transport, most resilience tests use deterministic stand-ins, and the protocol-path benchmarks are stubs; these do not establish live-network capacity or production tail latency. Current maturity is therefore **feature-rich, contract-tested, but not yet performance- or deployment-validated across transports**.

## In Progress
- [~] Network hardening wave for protocol safety, transport resilience, and predictable latency behavior (Target: Q3 2026)
  - [x] Complete remaining failure-injection coverage for multi-transport edge cases (Target: Q3 2026) — NMT-01..NMT-08 in tests/network/test_network_hardening_phase2_focused.cpp
  - [x] Tighten auth/rate-limit/session guard behavior under sustained adversarial traffic (Target: Q3 2026) — NAG-01..NAG-08 in tests/network/test_network_hardening_phase2_focused.cpp
- [x] approved next implementation block: mixed-transport failure injection and adversarial guard hardening delivered (Q3 2026)

## Planned Features

### v2.5.0 — backward-compatible evidence and operations
- [ ] Complete Waves 1–3 below: establish live-transport performance baselines, validate enabled transports under faults, and close operator-visible telemetry gaps (Target: v2.5.0)
- [ ] Preserve the frozen v1.x wire/API contract; add only backward-compatible APIs or behavior, with `release_critical` green on `develop` before promotion (Target: v2.5.0)

### v3.0.0 — strategic protocol and control-plane evolution
- [ ] Complete Waves 4–6 below: specify explicit wire capability/version negotiation, harden service-mesh control-plane security, and validate bounded behavior at multi-node scale (Target: v3.0.0)
- [ ] Any intentional wire/API incompatibility requires an approved migration plan, compatibility matrix, and release-note coverage before implementation (Target: v3.0.0)

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity — 3.5 Network** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

- [~] **HTTP/3 QUIC production enablement**: the optional QUIC/HTTP3 component and focused readiness tests exist; production-default deployment and live packet-loss/0-RTT validation are not established (Target: Q3 2026)
  - Acceptance: QUIC connection migration test passes at 1% simulated packet loss; 0-RTT handshake
    succeeds on reconnect within 50 ms; HTTP/3 throughput ≥ HTTP/2 baseline; `release_critical` green
  - Evidence: NQP-01..NQP-06 in tests/network/test_network_lifecycle_guardrails_focused.cpp
- [x] **Zero-copy socket I/O**: `ZeroCopyFrameBuilder::writeToWithSendfile()` wires sendfile(2)/splice(2)
  for large payload sends on TCP paths (Linux / macOS / FreeBSD); falls back to writev() on platforms
  without support or for small payloads (< 64 KiB threshold) (Target: Q4 2026)
  - Inputs: payload_fd, payload_offset, sendfile_threshold (default 64 KiB)
  - Acceptance: sendfile path active for payloads ≥ 64 KiB on Linux; writev fallback for small
    payloads and non-Linux; no regression for small payloads; `release_critical` green
  - Evidence: include/network/wire_protocol_zero_copy.h writeToWithSendfile(); src/network/wire_protocol_zero_copy.cpp

## Implementation Phases

### Phase 1: Protocol and Session Safety
- [x] Freeze network module API contract — connection lifecycle, frame validation, auth/session guards, transport fallback, rate-limiting, error taxonomy (include/network/network_api_contract.h) (Target: Q3 2026)
- [x] Define explicit NetworkErrorCode taxonomy (FRAME_INVALID, AUTH_REQUIRED, SESSION_EXPIRED, TRANSPORT_CLOSED, RATE_LIMITED, BACKPRESSURE_EXCEEDED, QUORUM_DEGRADED, …) (Target: Q3 2026)
- [x] Re-validate auth/session checks and frame/input validation across all major opcode/transport paths (Target: Q3 2026)
- [x] Extend deterministic regression packs for malformed frame and rate-limit abuse scenarios (Target: Q3 2026)

### Phase 2: Multi-Transport Resilience
- [x] Strengthen failure handling across TCP, WebSocket, UDP, QUIC/HTTP3, and gRPC paths (Target: Q4 2026) — NTR-01..NTR-08 in tests/network/test_network_transport_resilience_focused.cpp
- [x] Validate transport fallback/retry behavior under network degradation (Target: Q4 2026) — NTR-01..NTR-02 (retry policy exhaustion + bounded backoff), NTR-05..NTR-06 (gRPC fallback sequence)
- [x] Expand transport-level regression coverage for mixed TCP/WS/UDP/QUIC deployments (Target: Q4 2026) — NTR-08 (mixed multi-transport failure injection)

### Phase 3: Routing and Topology Hardening
- [x] Expand routing correctness checks under changing health/latency/topology signals (Target: Q4 2026) — NRH-01..NRH-08 in tests/network/test_network_routing_hardening_focused.cpp
- [x] Harden load-balancing state transitions and recovery behavior under churn (Target: Q4 2026) — NRH-03..NRH-05 (LB state machine correctness, churn tolerance)
- [x] Strengthen distributed routing and failover diagnostics for operator triage (Target: Q4 2026) — NRH-06..NRH-07 (failover preference order, region-down routing)

### Phase 4: Performance and Operational Hardening
- [x] Contract-hardening focused tests NCH-01..NCH-16 covering frame validation, auth/session, rate-limit, and connection lifecycle invariants (tests/network/test_network_contract_hardening_focused.cpp) (Target: Q1 2027)
- [x] Harden connection lifecycle guardrails (limits, backpressure, timeout interplay) under peak load (Target: Q4 2026) — NLG-01..NLG-08 in tests/network/test_network_lifecycle_guardrails_focused.cpp
- [~] HTTP/3 QUIC production readiness: NQP-01..NQP-06 provide focused contract/readiness coverage; live transport and representative performance evidence remain outstanding (Target: Q3 2026)
- [~] Measure protocol throughput and tail-latency envelopes across representative production mixes (Target: Q1 2027) — representative hardware baselines remain pending
- [~] Measure compression/batching/zero-copy overhead under high concurrency (Target: Q1 2027)

### Phase 5: Performance and Hardening
- [x] Define regression thresholds for in-process network helper gates: NRG-01..NRG-06 in benchmarks/network/bench_network_release_gates.cpp (TCP dispatch p99≤200µs, auth check p99≤100µs, rate-limit p99≤50µs, WS dispatch p99≤300µs, mock accept p99≤1ms, serialize p99≤100µs) (Target: Q3 2026)
- [x] Extended routing/CB/lifecycle benchmark gates: NRG-07..NRG-12 in benchmarks/network/bench_network_routing_gates.cpp (topology select, LB forward, CB shouldAllow, connection limit, queue gate, session guard) (Target: Q4 2026)
- [ ] Replace or clearly separate stub-only NRG-P1..NRG-P4 cases from promotion gates by adding real transport-path measurements (Target: v2.5.0)

### Phase 6: Documentation and Release Acceptance
- [ ] Keep roadmap status, test/benchmark evidence, and transport enablement claims aligned with current source; do not treat helper/stub results as production evidence (Target: ongoing)
- [ ] Keep completed work in the changelog and obtain human release sign-off against the applicable v2.5.0 or v3.0.0 wave gates (Target: each release)

## Production Readiness Checklist
- [x] Tracking in progress
- [x] Contract header frozen: include/network/network_api_contract.h (Phase 1)
- [x] Contract-hardening tests: tests/network/test_network_contract_hardening_focused.cpp (Phase 4, NCH-01..NCH-16)
- [x] Phase 2 hardening tests: tests/network/test_network_hardening_phase2_focused.cpp (NMT-01..NMT-08 multi-transport failure injection, NAG-01..NAG-08 adversarial auth/rate-limit guard hardening)
- [x] Phase 2 transport resilience tests: tests/network/test_network_transport_resilience_focused.cpp (NTR-01..NTR-08 retry policy, backpressure, gRPC fallback, pool drain, mixed injection)
- [x] Phase 3 routing hardening tests: tests/network/test_network_routing_hardening_focused.cpp (NRH-01..NRH-08 topology routing, LB state machine, circuit breaker)
- [x] Phase 4 lifecycle guardrail tests: tests/network/test_network_lifecycle_guardrails_focused.cpp (NLG-01..NLG-08 lifecycle, NQP-01..NQP-06 QUIC readiness)
- [x] Release-gate benchmarks: benchmarks/network/bench_network_release_gates.cpp (Phase 5, NRG-01..NRG-06)
- [x] Extended routing/CB/lifecycle benchmarks: benchmarks/network/bench_network_routing_gates.cpp (Phase 5, NRG-07..NRG-12)
- [x] Benchmark CMakeLists registered: benchmarks/network/CMakeLists.txt
- [x] AdaptiveCircuitBreaker per-error-class thresholds: include/network/adaptive_circuit_breaker.h + src/network/adaptive_circuit_breaker.cpp; NCB-PEC-01..08 in tests/network/test_network_cb_per_error_class_focused.cpp
- [x] Zero-copy sendfile/splice for large payloads: ZeroCopyFrameBuilder::writeToWithSendfile() in include/network/wire_protocol_zero_copy.h + src/network/wire_protocol_zero_copy.cpp
- [~] Optional HTTP/3 QUIC component and focused readiness tests exist; production deployment validation and live performance evidence remain outstanding (THEMIS_ENABLE_HTTP3)
- [x] New public headers: multipath_tcp.h, bbr_congestion_control.h, network_observability.h (implementations in src/network/)
- [x] EnvoyXdsClient xDS REST polling lifecycle documented: src/network/ARCHITECTURE.md §8
- Nachweise: network focused tests, transport integration tests, protocol security regressions, performance suites
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some cross-transport fault combinations still need broader evidence under long-duration stress.
- Certain route/mesh/topology scenarios require additional benchmark and resilience validation.
- Operator-facing diagnostics for complex multi-transport incidents continue to be refined.

## Research-Based Release Plan — v2.5.0 and v3.0.0

### Research and evidence basis

IETF QUIC standards define a secure multiplexed transport with flow control, migration, TLS integration, loss recovery, and congestion control; HTTP/3 maps HTTP semantics onto QUIC rather than making every QUIC transport an HTTP/3 implementation ([RFC 9000](https://www.rfc-editor.org/rfc/rfc9000), [RFC 9001](https://www.rfc-editor.org/rfc/rfc9001), [RFC 9002](https://www.rfc-editor.org/rfc/rfc9002), [RFC 9114](https://www.rfc-editor.org/rfc/rfc9114)). They motivate testing loss, migration, flow-control, handshake, and congestion behavior on real enabled paths rather than inferring transport readiness from helper tests. In particular, QUIC 0-RTT early data has replay considerations; mutating operations must not be accepted unless the application provides replay-safe semantics.

TailBench's peer-reviewed latency-critical workload methodology demonstrates why service performance needs realistic workload arrivals and tail-percentile reporting, not only isolated operation timing ([Kasture and Sanchez, IISWC 2016](https://dspace.mit.edu/handle/1721.1/112803)). For this module, keep open-loop/offered-load behavior, achieved throughput, p50/p95/p99 latency, errors, CPU, and memory in each live-load result. Fix and report machine, compiler, flags, transport configuration, workload, and run length; compare against a recorded same-hardware baseline.

### Code-to-evidence gap analysis

| Area | Source-grounded current state | Gap and consequence |
|---|---|---|
| Contracts and safety | `include/network/network_api_contract.h` specifies frame/auth/session checks, limits, lifecycle, errors, and fail-closed behavior; focused contract suites exist under `tests/network/`. | Contract tests establish local invariants, not independent client interoperability or behavior over a live listener under load. |
| Transports and use | TCP wire protocol, WebSocket, UDP, optional QUIC, and gRPC surfaces exist; feature enablement varies by build. `include/network/quic_transport.h` describes a QUIC binary wire transport, distinct from HTTP/3 semantics in RFC 9114. | Feature presence and configuration tests do not prove the transport is enabled, interoperable, or operationally supported in a target deployment. |
| Application scenarios | `src/network/ARCHITECTURE.md` identifies native client and node-to-node traffic; WebSocket and UDP provide distinct realtime/datagram paths. | There is no documented representative client/workload matrix tying application scenarios to supported transports, delivery semantics, deployment settings, and fallback behavior. |
| Performance and scale | NRG-01..NRG-12 cover in-process helper gates. `bench_network_protocol_path.cpp` explicitly labels NRG-P1..NRG-P4 as stubs with no live sockets. | No representative live-transport throughput/tail-latency baseline or demonstrated capacity envelope is currently established. |
| Resilience and security | NTR/NLG/NQP suites cover deterministic retry, backpressure, lifecycle, and QUIC readiness conditions; `src/network/SECURITY.md` records configuration-dependent controls and limitations. | The transport-resilience suite has no live sockets, and modeled QUIC migration/0-RTT cases do not prove packet-loss recovery, TLS behavior, or replay safety on a deployed listener. |
| Governance and operations | `network_api_contract.h` is frozen for the active v1.x line; `NetworkAuditLog` is bounded/in-memory with callback export, and `docs/operability/RUNBOOK_NETWORK_TRANSPORT.md` exists. | Evidence does not show durable audit delivery across every transport, fleet-level cardinality/resource bounds, or a network-specific compliance certification. No certification claim is made here. |
| Control plane | `src/network/ARCHITECTURE.md` §8 documents Envoy xDS REST polling over plain TCP, callback execution while a mutex is held, and the absence of exponential backoff and ACK/NACK. | Authenticated control-plane transport, bounded retry behavior, callback lifecycle, and applied-update evidence need explicit design and tests before broad service-mesh use. |

### v2.5.0 waves — backward-compatible delivery

| Wave | Priority / effort | Goal and concrete tasks | Dependencies | Risks / trade-offs | Success measures | Validation |
|---|---|---|---|---|---|---|
| **1 — Live-path baselines** | P0; M (3–5 engineer-weeks) | - [ ] Add real listener/client benchmark cases for each target-build transport; label NRG helper gates helper-only; record offered/achieved load, p50/p95/p99, errors, CPU/memory, and environment (Target: v2.5.0). | Agree on representative hardware, supported build matrix, and reproducible workload before recording baselines. | Real network runs are more variable and costly than in-process microbenchmarks; an unrepresentative client or host can bias results. | Publish reproducible baselines for every in-scope transport; no >10% regression in p99 or achieved throughput versus the same-build baseline unless a reviewed trade-off is recorded. Do not claim an absolute capacity target until these baselines exist. | Unit/contract regressions plus live loopback and representative-host load runs; repeat each fixed case at least five times, publish distributions and failures, and use an open-loop arrival model or disclose its absence. |
| **2 — Transport failure and 0-RTT safety** | P0; L (6–10 engineer-weeks) | - [ ] Exercise enabled TCP/WS/UDP/QUIC/gRPC paths with socket fault injection, bounded queue/timeout behavior, TLS failure cases, QUIC migration/loss, and reconnect/fallback; reject or deduplicate replayable mutations before enabling their 0-RTT processing (Target: v2.5.0). | Wave 1 transport/build inventory; stable test certificates and a fault-injection environment. | Fault simulation may not represent WAN loss; disabling early data can increase reconnect latency. | Every enabled transport passes its documented failure matrix; no silent data loss; queue/connection/memory bounds remain within configured limits; no mutating operation is replayed through 0-RTT. Measure 0-RTT latency against the full-handshake baseline rather than treating the existing modeled 50 ms gate as achieved. | Focused `tests/network/` contract tests, live integration tests, packet-loss/reconnect scenarios, security review of early-data policy, and bounded soak with captured error/recovery results. |
| **3 — Observability and release operations** | P1; M (3–5 engineer-weeks) | - [ ] Define transport-neutral connection, error, retry, backpressure, queue, and fallback metrics; bound label cardinality and audit-export failure behavior; connect alerts/runbooks to live-path signals (Target: v2.5.0). | Wave 1 metric definitions; Wave 2 failure taxonomy and transport instrumentation. | High-cardinality labels or synchronous export can add latency, memory growth, or dropped events. | For each enabled front door, demonstrate its agreed counters/histograms and operator diagnosis path; prove metric label cardinality and local buffering stay bounded under the Wave 2 soak; `release_critical` remains green on `develop`. | Metric schema review, bounded-cardinality stress, exporter outage/failure injection, runbook exercise, security/governance review, and full `release_critical` gate. |

### v3.0.0 waves — major-version evolution

| Wave | Priority / effort | Goal and concrete tasks | Dependencies | Risks / trade-offs | Success measures | Validation |
|---|---|---|---|---|---|---|
| **4 — Wire capability and version negotiation** | P0; L (6–10 engineer-weeks) | - [ ] Specify version/capability negotiation, unsupported-version errors, and a client/server compatibility matrix; decide and document any framing/API break before code changes (Target: v3.0.0). | v2.5.0 contract and client inventory; maintainer approval of the v3 compatibility and migration policy. | Negotiation can increase handshake complexity and create downgrade or split-version failure modes. | Every supported client/server pair has a deterministic negotiated result; unknown or unsafe versions fail closed; migration and rollback instructions are reviewed before release. | Cross-version client/server integration matrix, malformed/downgrade negotiation tests, protocol fuzzing, API/architecture review, and release-note/migration review. |
| **5 — Secure service-mesh control plane** | P1; L (6–10 engineer-weeks) | - [ ] Design authenticated/encrypted xDS transport, bounded backoff/timeouts, callback invocation outside internal locks, and observable update outcomes; reject invalid updates (Target: v3.0.0). | Wave 3 observability; selected xDS protocol/client requirements and credential lifecycle decisions. | More robust control-plane semantics add state and dependency complexity; certificate rotation or control-plane outage can stall updates. | Control-plane credentials are verified; invalid/stale updates are rejected and diagnosed; stop/reconfigure cannot deadlock; reconnect work and in-memory state remain bounded under control-plane outage. | Local TLS/xDS integration tests, invalid credential and malformed-response tests, outage/rotation/restart fault injection, thread-safety stress, and security review. |
| **6 — Multi-node scale and operational sign-off** | P1; L (6–10 engineer-weeks) | - [ ] Validate routing, QoS, connection pools, failover, and overload on representative multi-node topologies; publish capacity/recovery envelopes, deployment profiles, and rollback/runbook steps (Target: v3.0.0). | Waves 1–5; stable representative cluster and workload; refreshed `release_critical` evidence. | Results may not transfer across hardware, network paths, or tenant mixes; stronger isolation can reduce peak throughput. | At the published offered-load envelope, p99/error/recovery and resource bounds meet thresholds set from Wave 1 service objectives; no unbounded connection, queue, metric, or retry growth; release gates and operator sign-off pass. | Repeatable multi-node load/soak, topology/failure injection, recovery and rollback drill, benchmark review on representative hardware, and security/governance sign-off. |

### Assumptions, conflicts, and decision gates

- Effort estimates are engineering-week ranges for planning, not committed schedules; they assume an available representative multi-node test environment and transport-specific maintainers.
- Absolute throughput and latency SLOs remain open until Wave 1 records representative workloads and baselines. Existing NRG thresholds are helper-level regression gates, not service SLOs.
- Root `ROADMAP.md` §3.5 currently marks HTTP/3 QUIC production enablement complete, while this module roadmap and in-scope evidence describe optional QUIC surfaces and modeled readiness tests without live deployment/performance validation. Documentation precedence makes root governance authoritative; reconcile that status and obtain maintainer sign-off before treating QUIC as a release-ready production capability.
- The developer wiki health snapshot (`ai_context/developer_llm_wiki/WIKI_STATUS.json`) was generated 2026-11-02 and reports a 1,450-source removal delta. It is context only; the current code, tests, and canonical module/root roadmaps above take precedence.
- No module-specific compliance certification, fleet capacity result, or universal transport availability is assumed. Release/edition support must be verified against actual build options, deployment configuration, and `release_critical` evidence.

## Breaking Changes
- Network public APIs and protocol evolution in active major lines remain additive-first.
- Any behavior change requiring client migration must be versioned and documented in changelog/migration notes.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `network`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — delivered 2026-09-16: high-cardinality stress in tests/network/test_network_highcardinality_stress.cpp; runbook in docs/operability/RUNBOOK_NETWORK_TRANSPORT.md; D1 trace span cross-links documented
- [x] **Wire-Protocol Session-State Strand Safety**: Replace shared `payload_buffer_` / `header_buffer_` members with per-dispatch copies or a `net::strand` to eliminate I/O-thread / worker-thread race under pipelining (`wire_protocol_server.cpp`). (Target: Q1 2027) — fixed 2026-09-16: per-dispatch capture-by-value + RAII scope-swap pattern in `dispatchToWorkerPool()`; THREAD SAFETY comments added at all changed sites
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — delivered 2026-09-16: tests/integration/test_network_protocol_soak.cpp (TCP dispatch throughput, WS frame RTT, connection lifecycle stability)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — delivered 2026-09-16: docs/operability/RUNBOOK_NETWORK_TRANSPORT.md (6 scenarios: TCP/WS/QUIC connection failure, frame validation errors, rate-limit/backpressure, circuit breaker, WS auth, gRPC fallback)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — confirmed: wire-protocol strand-safety fix eliminates race; all auth/frame/rate-limit paths remain fail-closed per NCH-01..NCH-16
- [~] Establish p95/p99 baselines on representative hardware (Target: Q1 2027) — NRG-P1..NRG-P4 are stub benchmarks; hardware baselines pending
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — docs/operability/RUNBOOK_NETWORK_TRANSPORT.md delivered 2026-09-16

---

## Wave D Closure Batch (2026-09-16)

All four Wave D open items for the `network` module were closed in this batch:

| Item | Type | Evidence |
|------|------|----------|
| Wire-protocol strand safety fix | `[I]` → `[x]` | `dispatchToWorkerPool()` in `src/network/wire_protocol_server.cpp` — per-dispatch RAII scope-swap; THREAD SAFETY comments added |
| High-cardinality stress coverage | `[ ]` → `[x]` | `tests/network/test_network_highcardinality_stress.cpp` — HighConcurrencyConnectionStress (500 conns), MixedFrameTypeHighCardinality (500 frames), CircuitBreakerUnderHighErrorRate |
| Long-duration soak test | `[ ]` → `[x]` | `tests/integration/test_network_protocol_soak.cpp` — TCP dispatch throughput (≥ 1000 ops/sec), WS frame RTT (p99 ≤ 200 µs), connection lifecycle stability |
| Operator runbook | `[ ]` → `[x]` | `docs/operability/RUNBOOK_NETWORK_TRANSPORT.md` — 6 scenarios + D1 trace span cross-links |

Mid-term planned features advanced to `[~]`:
- NRG-P1..NRG-P4 protocol-path benchmarks created (`benchmarks/network/bench_network_protocol_path.cpp`); hardware baselines pending
- Resilience stress coverage expanded; full mesh/topology benchmarks pending
- Observability runbook and D1 span targets documented; instrumentation pending Phase 2A

**Historical closure evidence date:** 2026-09-16; this is not a later hardware-baseline or production sign-off.
