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

# Network Module Research and Evaluation

## Purpose and evidence boundary

This page records how network-layer performance and resilience claims can be
evaluated. It is for maintainers deciding whether a transport or protocol-path
change is ready for broader testing. Runtime behavior is defined by the
network headers and implementation, especially
[`network_api_contract.h`](../../include/network/network_api_contract.h) and
[`ARCHITECTURE.md`](ARCHITECTURE.md); this page does not supersede them.

The module contains TCP wire-protocol handling, optional WebSocket and QUIC
surfaces, a UDP ingestion server, gRPC transport support, connection pooling,
routing, and overload controls. A measurement of one helper or protocol
operation is not an end-to-end measurement of those surfaces.

## Evaluation questions and methods

| Question | Existing evidence | Interpretation |
|---|---|---|
| Does frame parsing/dispatch stay within the declared hot-path budget? | [`bench_network_release_gates.cpp`](../../benchmarks/network/bench_network_release_gates.cpp), NRG-01 and NRG-06 | In-process frame-dispatch and serialization checks; the file declares p99 gates, not a hardware-independent result. |
| Do the auth, rate-limit, accept, and WebSocket helpers stay bounded? | The same benchmark, NRG-02 through NRG-05 | These isolate helper paths; NRG-05 is explicitly a mock accept check. |
| Are routing, breaker, connection-limit, queue, and session-guard paths measurable? | [`bench_network_routing_gates.cpp`](../../benchmarks/network/bench_network_routing_gates.cpp), NRG-07 through NRG-12 | Small in-memory/fast-path checks; not a full cluster or loaded listener test. |
| What does the proposed protocol-path work cost? | [`bench_network_protocol_path.cpp`](../../benchmarks/network/bench_network_protocol_path.cpp), NRG-P1 through NRG-P4 | These implementations are marked as stubs in the benchmark source. Their output is not evidence of real socket, TLS, QUIC, or gRPC performance. |
| Do failure and contract cases remain correct? | Focused suites listed in [`ROADMAP.md`](ROADMAP.md) and [`tests/network/CMakeLists.txt`](../../tests/network/CMakeLists.txt) | These provide regression evidence, not throughput or production-capacity evidence. |

Use the benchmark’s own source and labels to establish whether a scenario
executes real code or a modeled/stub path before interpreting results. In
particular, the protocol-path benchmark explicitly leaves hardware baselines
pending; do not describe its targets as achieved performance.

## Measurement controls

For a meaningful comparison:

1. Keep the executable, compiler, build flags, machine, and benchmark
   configuration fixed between baseline and candidate.
2. Record the exact benchmark case and its label, including whether it uses a
   mock or stub.
3. Separate helper-level measurements from socket-level tests and long-running
   load/soak tests.
4. Report distributions and errors, not only an average. The release-gate
   source specifies p99 thresholds for NRG-01..NRG-12.
5. Treat a result as provisional until reproduced on representative hardware
   and compared with a recorded baseline. The roadmap states that hardware
   baselines for NRG-P1..NRG-P4 remain pending.

## Trade-offs and limits

- Async I/O, worker-pool dispatch, batching, zero-copy paths, and compression
  have different costs and should not be collapsed into one throughput number.
- UDP intentionally has no delivery or ordering guarantee; an ACK is optional
  and does not change the transport into a reliable stream.
- QUIC/TLS handshake and connection migration costs cannot be established by
  an in-process benchmark that models cryptographic and I/O work.
- Test coverage for transport-failure combinations does not establish a
  long-duration capacity limit. The roadmap still tracks broader mesh and
  topology resilience evidence.

## Validation path

Use the focused network tests for the property being changed, then the
corresponding benchmark when one measures the real code path. Relevant suites
include `test_network_contract_hardening_focused.cpp`,
`test_network_transport_resilience_focused.cpp`,
`test_network_lifecycle_guardrails_focused.cpp`, and
`test_network_routing_hardening_focused.cpp` under `tests/network/`.
For operational claims, pair these with the integration soak test
`tests/integration/test_network_protocol_soak.cpp`; its TCP and WebSocket
scenarios are distinct from helper microbenchmarks.

See also [`MODELING.md`](MODELING.md),
[`IMPLEMENTATION.md`](IMPLEMENTATION.md), and the module
[`ROADMAP.md`](ROADMAP.md).
