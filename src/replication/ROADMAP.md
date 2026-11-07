---
Author: ThemisDB Contributors
Created: 2026-05-31
Last Updated: 2026-11-07
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - AUDIT.md
  - README.md
  - ../../include/replication/replication_manager.h
  - ../../include/replication/raft_v2.h
  - ../../include/replication/logical_replication.h
  - ../../include/replication/replication_slot.h
  - ../../include/replication/async_wal_shipper.h
  - ../../src/replication/
  - ../../tests/replication/
  - ../../benchmarks/replication/
  - ../../research/stand_der_technik/2026_q1_landscape.md
  - ../../research/implementation_influence/by_module.md
Related_Issue: "#7446"
Related_Milestone: "DOC-WEEKLY-2026-W45"
---

Exact model backend not verifiable from final document text alone.

# Replication Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

The repository contains replication APIs and implementation components, but the reviewed
inventory does not establish a fully integrated, end-to-end production replication
topology. The audit counts 18 public headers and 13 implementation `.cpp` files and
records unresolved wiring/configuration questions. The `AsyncWalShipper` default handler
is a no-op; lag evaluation requires caller action and does not establish automatic
failover. Current tests and benchmarks include mocks, in-process stubs, or simulated
paths, so they are not by themselves evidence of durable network transport, production
multi-writer correctness, or release performance.

The plan therefore treats **v2.5.0** as integration, durability, and release-evidence
closure, and **v3.0.0** as a separate strategic consistency, scale, and operations
program. Neither release is declared production-ready by this roadmap. Peer
authentication/TLS, RPO/RTO, supported topology, and runtime integration remain open
assumptions requiring explicit decisions and evidence.

## Planned Features

Planning estimates below are rough engineering effort in person-weeks, not approved
commitments. No new absolute numeric performance targets are introduced; existing targets
remain provisional until reproduced against a documented baseline and approved by an
owner. Each wave is a gate proposal, not a claim that its tasks are complete.

### v2.5.0 — integration and release-evidence closure

#### Wave 2.5-A — runtime integration and configuration contract

- **Priority:** P0 — release prerequisite.
- **Goal:** Establish which replication surfaces are supported, wired, configurable, and
  reachable through an active build/runtime path.
- **Tasks:**
  - [ ] Trace `kafka_change_stream.h` and `multi_master_replication.h` to implementation,
    build registration, and runtime call sites; document supported, unsupported, or
    blocked status for each (Target: source/build/runtime integration map).
  - [ ] Define and verify the supported C++ configuration mapping for
    `ReplicationConfig`, `WalShippingConfig`, and `SLOThresholds`; distinguish it from
    the separate server-side sharding WAL shipper config (Target: tested configuration
    contract and user-facing examples).
  - [ ] Confirm active CMake configure paths include intended replication tests and
    benchmark registrations; document the exact configure/test commands (Target:
    reproducible build/test registration evidence).
- **Dependencies:** Repository/build-owner access; decisions on supported interfaces and
  configuration ownership.
- **Risks:** Header declarations may lack an implementation/runtime consumer; similarly
  named sharding configuration may be mistaken for replication-module wiring.
- **Effort:** 2–4 person-weeks.
- **Success metrics:** Both flagged headers have an explicit disposition; every supported
  C++ config field has a traced parser/default path and a passing mapping test; active
  configure paths and registered test targets are recorded without unresolved inclusion
  ambiguity.
- **Validation strategy:** Trace declarations through `.cpp`, CMake, and runtime
  consumers; add/execute focused contract and config tests in the implementation task;
  verify the configured target list. Documentation-only inventory is not runtime proof.

#### Wave 2.5-B — durable CDC/WAL replay, backpressure, and fault closure

- **Priority:** P0 — release prerequisite.
- **Goal:** Demonstrate end-to-end delivery and recovery behavior at real storage and
  transport boundaries, including bounded queues and CDC/slot progress.
- **Tasks:**
  - [ ] Replace or explicitly gate the default no-op WAL handler for supported runtime
    usage; define durable acknowledgement, retry/replay, ordering, and duplicate handling
    contracts (Target: documented contract and integrated transport path).
  - [ ] Exercise queue saturation, ship failure/restart, WAL replay, and CDC/slot
    checkpoint recovery with production-path collaborators rather than only mocks
    (Target: repeatable fault/recovery evidence).
  - [ ] Establish whether lag alert handling is connected to an operator action or
    failover controller; document the caller-owned boundary and fail-closed behavior
    until automatic behavior is actually wired (Target: explicit and tested behavior).
- **Dependencies:** Wave 2.5-A config/runtime map; selected durable storage and transport
  implementation; agreed acknowledgement and recovery semantics.
- **Risks:** Mock and in-process results may not represent durable transport; uncertain
  failover coupling could cause unsafe assumptions about data loss or promotion.
- **Effort:** 4–8 person-weeks, excluding external transport/storage implementation work.
- **Success metrics:** Recovery tests demonstrate the declared acknowledgement and replay
  contract across injected transport/process faults, with zero acknowledged events lost;
  queue bounds and lag/checkpoint behavior are asserted on the integrated path; no
  supported path silently treats a no-op handler as successful remote delivery.
- **Validation strategy:** Extend and run
  `test_replication_async_wal_lag_alerts.cpp`,
  `test_replication_geo_placement_wal_shipping_focused.cpp`,
  `test_replication_chaos_failover_focused.cpp`, and
  `test_replication_fail_closed_behavior.cpp` against wired collaborators; select the
  relevant slot/CDC tests after tracing their current coverage. Retain mock-only tests
  as unit tests, not integration evidence. Record test configuration and fault histories.

#### Wave 2.5-C — baseline, security, and release gates

- **Priority:** P1 — required release acceptance.
- **Goal:** Produce trustworthy performance and security acceptance evidence for the
  v2.5.0 supported topology.
- **Tasks:**
  - [ ] Define the topology, peer trust/authentication and TLS expectations, threat
    boundaries, and operational owners (Target: approved scope; no implicit defaults).
  - [ ] Capture repeatable end-to-end throughput, latency, lag, recovery, and resource
    baselines on documented hardware/topology; compare regressions baseline-relatively
    and obtain owner approval before setting numeric gates (Target: reproducible
    baseline report).
  - [ ] Promote only tests exercising the agreed runtime path into release gates and
    document exclusions (Target: traceable release-criteria matrix).
- **Dependencies:** Waves 2.5-A and 2.5-B; security, release, and operations owners;
  representative test environment.
- **Risks:** Unspecified TLS/auth and topology scope; simulated benchmarks could be
  misread as transport performance. The header's 80 MB/s GbE WAL target is provisional:
  the reviewed benchmark is queue/no-op or simulated and the audit says it was not
  executed.
- **Effort:** 3–6 person-weeks, plus environment and review availability.
- **Success metrics:** Every accepted release gate maps to a supported topology and
  reproducible test; security assumptions have named decisions; end-to-end performance
  results report throughput, p95/p99 latency, lag, recovery time, and resource use with
  baseline, environment, and measurement method. Measure the existing ≥80 MB/s GbE WAL
  criterion as provisional until an end-to-end transport run and owner approval confirm
  whether it is an appropriate release threshold.
- **Validation strategy:** Run gated tests and end-to-end benchmarks on the declared
  setup; review peer-auth/TLS configuration and fault behavior; preserve reports and
  CI results as auditable release evidence.

### v3.0.0 — consistency, scale, and operational maturity

#### Wave 3.0-A — consistency model and API compatibility

- **Priority:** P0 — prerequisite for strategic behavior changes.
- **Goal:** Make replication consistency, conflict, membership, and migration guarantees
  explicit before expanding multi-writer support.
- **Tasks:**
  - [ ] Specify supported consistency modes, ordering/visibility guarantees, conflict
    semantics, and failure behavior for `ReplicationManager`, Raft-v2, logical
    replication, and any multi-master surface (Target: reviewed behavioral contract).
  - [ ] Define compatibility policy for public API/config changes and migration behavior
    between v2.5.0 and v3.0.0 (Target: versioned compatibility and migration plan).
  - [ ] Map CRDT use cases to declared merge invariants and identify cases requiring
    coordination; do not infer product guarantees from the CRDT literature (Target:
    per-mode invariant matrix).
- **Dependencies:** v2.5.0 interface/config inventory; product decisions on consistency
  modes and supported API changes.
- **Risks:** Ambiguous semantics can make convergence claims misleading; compatibility
  changes can affect persisted state and clients.
- **Effort:** 3–5 person-weeks.
- **Success metrics:** Every supported mode has explicit invariants, failure semantics,
  and compatibility status; each public breaking change has a migration decision and
  testable acceptance criteria.
- **Validation strategy:** Contract review plus deterministic histories/model-based
  tests for ordering, convergence, membership changes, and restart/replay; compare
  observed histories to the declared invariants.

#### Wave 3.0-B — scalable multi-region/multi-writer with demonstrated invariants

- **Priority:** P1 — contingent on 3.0-A contracts.
- **Goal:** Establish evidence-based scale and correctness boundaries for supported
  multi-region and multi-writer workloads.
- **Tasks:**
  - [ ] Define candidate topologies, failure domains, placement constraints, and
    workload envelopes; identify explicitly unsupported arrangements (Target: reviewed
    topology matrix).
  - [ ] Replace simulated-only multi-DC benchmark evidence with an integrated path using
    real transport and durable WAL/CDC endpoints (Target: reproducible end-to-end
    workload).
  - [ ] Run partition, delay, duplicate/reorder, node-loss, and recovery histories and
    check them against Wave 3.0-A invariants (Target: published correctness and
    baseline-relative scale report).
- **Dependencies:** Waves 2.5-B/2.5-C and 3.0-A; real transport/storage path; test
  environment representative of selected topology.
- **Risks:** Network and failure-domain effects can invalidate in-process results;
  broader multi-writer support may require coordination and trade-offs not yet selected.
- **Effort:** 6–12 person-weeks, excluding infrastructure provisioning.
- **Success metrics:** Each claimed topology has a reproducible workload and fault
  history; all designated histories pass with zero violations of the declared invariants;
  performance and resource results are reported relative to a documented baseline, with
  no unsupported numeric commitment.
- **Validation strategy:** Integrated load/fault tests, Jepsen-style history checking,
  recovery/replay verification, and repeated benchmark runs on documented environments.
  Simulated/no-op runs remain labeled as simulations.

#### Wave 3.0-C — security, operations, and governance

- **Priority:** P1 — required before claiming operational maturity.
- **Goal:** Make supported replication deployable and operable with explicit security,
  recovery, ownership, and lifecycle guarantees.
- **Tasks:**
  - [ ] Decide and test peer authentication/TLS, credential rotation, authorization, and
    failure behavior for every supported transport (Target: reviewed threat model and
    security acceptance tests).
  - [ ] Set topology-specific RPO/RTO objectives with owners and verify them under
    measured failure/recovery conditions (Target: approved objectives and evidence;
    no values assumed in this roadmap).
  - [ ] Define monitoring/alert ownership, runbooks, upgrade/rollback, backup/reseed,
    and deprecation/migration governance (Target: operational acceptance package).
- **Dependencies:** Wave 3.0-B supported topology and measured recovery evidence;
  security and operations owner decisions.
- **Risks:** Unapproved objectives or incomplete credential lifecycle can leave gaps
  despite passing functional tests; objectives may differ by topology.
- **Effort:** 4–8 person-weeks, plus security and operations review.
- **Success metrics:** Every supported topology has approved RPO/RTO and security
  criteria, tested runbooks, monitored failure signals, and a named operational owner;
  all release-blocking checks have recorded outcomes.
- **Validation strategy:** Security review and negative tests; execute recovery,
  rotation, upgrade/rollback, and runbook exercises in a representative environment;
  retain evidence and close or explicitly accept each risk.

## Implementation Phases

These phases are a delivery sequence for the release waves above, not assertions of
completed implementation.

### Phase 1: Design / API Contract
- [ ] Complete Wave 2.5-A interface, configuration, and topology inventory.
- [ ] Specify the behavioral and compatibility contracts required by Wave 3.0-A.

### Phase 2: Core Implementation
- [ ] Wire supported config, transport, WAL/CDC, and runtime paths; preserve explicit
  unsupported status for unimplemented surfaces.

### Phase 3: Error Handling and Edge Cases
- [ ] Define and verify replay, retry, backpressure, lag, slot, promotion, and
  fail-closed behavior at the supported runtime boundary.

### Phase 4: Tests
- [ ] Separate unit/mock evidence from integrated durability, chaos, and history-check
  evidence; run the latter against declared topologies.

### Phase 5: Performance and Hardening
- [ ] Establish reproducible baseline-relative performance and security/reliability
  gates; approve any absolute target with its owner.

### Phase 6: Documentation and Acceptance
- [ ] Publish versioned compatibility, configuration, security, operations, and
  topology acceptance evidence; obtain explicit release-owner review.

## Production Readiness Checklist

No end-to-end production readiness is established by this roadmap update. The following
remain release gates:

- [ ] Supported runtime topology and active build/config integration are demonstrated.
- [ ] Durable transport, replay, backpressure, CDC/slot recovery, and failover boundaries
  are validated using production-path collaborators.
- [ ] Peer authentication/TLS, threat boundary, and credential lifecycle are decided and
  tested.
- [ ] Topology-specific RPO/RTO and operational ownership are approved and exercised.
- [ ] Release gates use reproducible end-to-end evidence; mock, stub, and no-op results
  are labeled and excluded from product-performance claims.
- [ ] Compatibility/migration review and release-owner acceptance are recorded.

### Historical completion claims — unverified

The prior roadmap recorded checked completion claims for geographic placement, async
WAL shipping/lag alerts, focused contract and chaos tests, benchmark gates, Doxygen/API
work, Wave A closure, and a Batch 4 gap-closure session. These are retained only as
historical claims: this update did not revalidate their dates, implementation, CI
registration, or results. In particular, mock-based RCH tests, in-process stress stubs,
and simulated/no-op benchmarks are not end-to-end evidence. The reported 80 MB/s GbE
WAL target remains provisional pending a real transport benchmark and owner approval.

## Known Issues & Limitations

- `AUDIT.md` inventories 18 public headers and 13 implementation `.cpp` files; it leaves
  wiring/integration of `kafka_change_stream.h` and `multi_master_replication.h`
  unresolved.
- Mapping of the replication C++ configuration types to YAML is not established.
- `AsyncWalShipper` has a bounded queue and injected ship handler, but its default
  handler is a no-op. `LagAlertManager` requires caller update/check; automatic failover
  is not established.
- `test_replication_contract_hardening_focused.cpp` (RCH) uses mock CRDT/WAL/change
  stream helpers; `test_replication_highcardinality_stress.cpp` uses in-process stubs.
- `bench_replication_multi_dc_multi_writer.cpp` exercises in-memory/no-op or simulated
  writes; `bench_replication_release_gates.cpp` uses local helpers/mocks for CRDT,
  serialization, WAL apply, offsets, and lag checks. Neither establishes end-to-end
  transport performance. The audit reports benchmarks were not executed.
- The extent to which module-local test CMake registration is included in every active
  configure path remains uncertain; root test registrations exist.
- Peer authentication/TLS, RPO/RTO, supported topology, runtime integration, and
  operational ownership remain decisions/evidence gaps, not assumed capabilities.

## Breaking Changes

No v2.5.0 or v3.0.0 breaking contract is approved by this roadmap. The v3.0.0 plan
requires an explicit compatibility decision; any breaking API, configuration, persisted
state, or wire-contract change must have migration/rollback notes and release/change
documentation before acceptance.

## Research-informed status and references

These sources inform the gap analysis and validation approach; none establishes ThemisDB
behavior, product performance, or release readiness.

| Research finding | Current code/evidence comparison | Roadmap implication |
|---|---|---|
| Raft requires log-safety and safe membership-transition invariants (Ongaro & Ousterhout, 2014). | `raft_v2.h` declares joint consensus; the API/source inventory does not by itself prove end-to-end crash/partition safety. | Test reconfiguration and leader-loss histories against the declared invariants (Waves 2.5-B, 3.0-A/B). |
| Geo-replication trades coordination/latency against availability and consistency (Corbett et al., OSDI 2012; DeCandia et al., SOSP 2007). | The manager declares sync, semi-sync, and async modes; supported topology and acknowledgement behavior remain to be verified at the integrated runtime boundary. | State per-mode guarantees and measure client latency under region RTT and failure conditions (Waves 2.5-A/C, 3.0-A/B). |
| CRDT convergence relies on datatype-specific algebraic conditions (Shapiro et al., SSS 2011). | The contract lists CRDT properties, but RCH focused tests use mock helpers rather than the production resolver path. | Validate each supported type under reordered, duplicated, and delayed updates; do not generalize merge safety to arbitrary documents (Wave 3.0-A/B). |
| Logical replication requires explicit slot, apply-order, replay, and retention behavior (PostgreSQL logical-replication documentation). | ThemisDB exposes logical changes and slot progress; restart/replay and WAL-retention behavior are not established by the module inventory. | Fault-test consumer restart, slot recovery, gaps/duplicates, and retained-WAL growth (Wave 2.5-B). |
| Serving-system comparisons need representative workloads and tail-latency measurement (Cooper et al., YCSB, SoCC 2010; Dean & Barroso, “The Tail at Scale,” 2013). | Existing benchmarks include no-op/simulated paths; they do not show end-to-end multi-region throughput or tail latency. | Record workload, topology, hardware, throughput, p95/p99, lag, and resource use against reproducible baselines (Wave 2.5-C, 3.0-B). |
| Fault injection plus operation-history checking can reveal safety violations that throughput tests miss (Jepsen methodology). | Focused chaos tests exist, but no history-checking result is claimed here. | Check recorded operations against the versioned consistency contract across faults and recovery (Wave 3.0-B). |
| Identity-centered authorization is a security control independent of network location (NIST SP 800-207). | TLS-related config fields and an injected transport exist, but enforcement and authorization of administrative operations are not established. | Decide, implement, and negatively test peer identity, transport protection, credential lifecycle, and role authorization (Wave 2.5-C, 3.0-C). |

### References

- Ongaro & Ousterhout, 2014, [In Search of an Understandable Consensus Algorithm
  (Raft)](https://raft.github.io/raft.pdf).
- Corbett et al., OSDI 2012, [Spanner: Google’s Globally-Distributed Database](https://research.google.com/archive/spanner.html).
- DeCandia et al., SOSP 2007, [Dynamo: Amazon’s Highly Available Key-value Store](https://web.stanford.edu/class/cs345d-01/rl/dynamo.pdf).
- Shapiro et al., SSS 2011, [Conflict-Free Replicated Data Types](https://link.springer.com/content/pdf/10.1007/978-3-642-24550-3_29); also catalogued in
  [`research/stand_der_technik/2026_q1_landscape.md`](../../research/stand_der_technik/2026_q1_landscape.md)
  and [`research/implementation_influence/by_module.md`](../../research/implementation_influence/by_module.md).
- PostgreSQL, [current logical replication documentation](https://www.postgresql.org/docs/current/logical-replication.html).
- Cooper et al., SoCC 2010, [Benchmarking Cloud Serving Systems with YCSB](https://doi.org/10.1145/1807128.1807152).
- Dean & Barroso, 2013, [The Tail at Scale](https://doi.org/10.1145/2408776.2408794).
- [Jepsen distributed-systems testing methodology](https://github.com/jepsen-io/jepsen).
- NIST, [SP 800-207: Zero Trust Architecture](https://csrc.nist.gov/pubs/sp/800/207/final).

**Source review basis:** [`AUDIT.md`](AUDIT.md), [`README.md`](README.md), the public
headers and runtime sources listed in the metadata, and the named replication tests and
benchmarks. This is a plan/status documentation update; no tests, builds, or benchmarks
were run.
