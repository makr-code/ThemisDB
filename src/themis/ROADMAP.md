---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-11-07"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

# Themis Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | source-reviewed: 2026-11-07 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The source tree contains implementations for build identity, edition/license
gating, module loading and verification, dependency resolution, and wire-server
operation. This inventory does not establish module-level production
readiness; root `ROADMAP.md` remains authoritative for aggregate release
readiness and blockers.

> **Evidence boundary (2026-11-07):** Source and CMake registration were checked;
> no build, test, soak, or benchmark execution was performed for this update.
> The high-cardinality stress and core soak sources use stub implementations;
> the dedicated-gates benchmark measures stubs, and the core benchmark's wire
> case defaults to synthetic dispatch. These sources are not production-path
> performance or resilience evidence. No representative-hardware baseline or
> run artifact was checked. Older status labels and target dates below remain
> unresolved where they require execution evidence or sign-off.

## In Progress

- [?] verify loader/signature policy and trust-edge behavior on supported platforms (Target: v2.5.0; execution evidence pending)
- [?] validate diagnostics consistency across license, verification, and wire runtime stages (Target: v2.5.0; runtime signal evidence pending)
- [?] establish production-path benchmark guardrails for themis core hot paths (Target: v2.5.0; representative baseline pending)

## Planned Features

### Short-term (3-6 months)
- [?] validate deterministic dependency and trust outcomes against production resolver/verifier paths (Target: v2.5.0) — `test_themis_highcardinality_stress.cpp` exercises stubs, not these paths
- [?] exercise wire-session and loader contention through production implementations (Target: v2.5.0) — current high-cardinality fixtures are stubs
- [?] confirm operator signals for load/verify/gating incidents are emitted and actionable (Target: v2.5.0) — runbook exists; live signal coverage was not checked

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for loader/gating/server-sensitive paths (Target: v2.5.0)
- [?] measure module lifecycle and wire runtime using production paths (Target: v2.5.0) — `bench_themis_dedicated_gates.cpp` uses in-process stubs
- [?] validate sustained-load reliability through production implementations (Target: v2.5.0) — `test_themis_core_soak.cpp` uses stubs

## Research and Evidence-Based Release Plan

### Planning scope and research basis

The repository currently identifies itself as `2.4.0` in [`../../VERSION`](../../VERSION). The v2.5.0 and v3.0.0 entries below are proposed module targets, not release commitments; root `ROADMAP.md`, `VERSIONING.md`, `RELEASE_STRATEGY.md`, and maintainer approval remain authoritative. v2.5.0 work preserves the current major-line contracts; any incompatible public or wire contract change belongs behind v3.0.0 migration planning.

The plan applies research methods to this module; the cited work does not establish that ThemisDB already meets a performance or reliability threshold.

| Reference | Finding applied to this plan |
|---|---|
| Cooper et al., “Benchmarking Cloud Serving Systems with YCSB,” SoCC 2010, [ACM DOI 10.1145/1807128.1807152](https://doi.org/10.1145/1807128.1807152) | Use explicit, repeatable workload profiles and disclose workload/environment when comparing throughput and scale; adapt workloads to loader, resolver, gate, and wire paths rather than borrowing YCSB numbers. |
| Basiri et al., “Chaos Engineering,” *IEEE Software* 33(3), 2016, [IEEE DOI 10.1109/MS.2016.60](https://doi.org/10.1109/MS.2016.60) | State a steady-state hypothesis, inject bounded faults in a representative setup, and verify that the observed result matches the hypothesis before claiming resilience. |
| NIST SP 800-218, *Secure Software Development Framework* (SSDF) v1.1, [publication](https://csrc.nist.gov/pubs/sp/800/218/final) | Treat security validation, reviewable evidence, and release provenance as lifecycle controls rather than a one-time source-code check. |
| Google SRE, [Monitoring Distributed Systems](https://sre.google/sre-book/monitoring-distributed-systems/) | Pair latency with traffic, errors, and saturation; distinguish successful-request latency from error latency and report tail behavior rather than averages alone. |

### Current maturity and code-to-research gap

**Assessment:** implementation is established across runtime identity and licensing/edition gates, module loading and verification, dependency resolution, and wire service. Contract/integration tests and performance source files exist, but demonstrated production readiness is **not established**: this review did not execute tests or benchmarks, and the strongest named stress/soak fixtures and dedicated-gate benchmarks do not call production implementations.

| Focus | In-tree evidence | Gap and implication |
|---|---|---|
| Performance and scaling | `benchmarks/themis/bench_themis_core.cpp` measures build-info, hash verification plus direct OS library open/close, license-signature validation, and feature-gate paths. `module_dependency_resolver.cpp` uses deterministic Kahn sorting. | The core wire benchmark defaults to a synthetic switch and its optional UDP loopback is not the TCP/session request path. `bench_themis_dedicated_gates.cpp` benchmarks stub functions with hard-coded nanosecond gates. The sorted ready-vector operations in the resolver merit a workload benchmark before any optimization claim. |
| Application usability | `src/themis/MODULE_GUIDE.md`, `API.md`, `OPERATIONS.md`, and `RUNBOOK.md` describe ownership, interfaces, and operation. | The module has separate loader surfaces under `include/themis/` and `include/themis/base/`; callers and deployment teams need an explicit scope/ownership map and validated failure guidance. |
| Security and resilience | `module_signature_verifier.cpp`, `module_security.cpp`, `module_hash_verifier.cpp`, and `module_dependency_resolver.cpp` implement trust and lifecycle controls; integration tests cover license/gating and hash/build-info behavior. | `PRODUCTION_REQUIREMENTS.md` says build/deployment policy must be verified. High-cardinality and soak tests substitute stubs for loader, verifier, resolver, and TCP paths; production fail-closed, platform, fault, and recovery evidence is incomplete. |
| Governance and maintainability | Module-local API, security, operations, research, and roadmap docs exist; release governance is rooted in `../../ROADMAP.md` and `../../VERSIONING.md`. | Source presence is not acceptance evidence. The curated Developer LLM Wiki has no themis module page; its status snapshot was generated 2026-11-02 and reports 26 changed/1,450 removed sources, so it is context only and needs a separate refresh. |
| Architecture and operations | The architecture separates identity/gating, secure module lifecycle, and wire runtime; test targets are registered from `tests/themis/CMakeLists.txt`. | `src/themis/wire_protocol_server.cpp` directly includes query, index, timeseries, and process headers. Need execution evidence on supported OS/build policies, actual network sessions, resource limits, and operator signals. Target hardware, service-level objectives, and key-management ownership are not established in the inspected sources. |

### v2.5.0 — backward-compatible hardening

#### Wave 1 — Evidence and benchmark validity (P0; estimate: 2–3 engineer-weeks)

- **Goal:** make every release performance claim trace to a measured production path.
- **Tasks:** map each `THMP-*`/`THMG-*` case to its implementation; replace or explicitly mark stub-based `TH-BM-01..04` as non-gating; add real loader, resolver, and full wire-session profiles with cold/warm, churn, success/error, and cardinality variants; emit a run manifest with commit, build mode, OS, CPU, configuration, workload, and raw latency samples.
- **Dependencies:** none; agree a supported-platform and representative-hardware matrix with maintainers.
- **Risk/trade-off:** production-path benchmarks cost more to run and may need dedicated hardware; excluding a synthetic case from gates can reveal that prior numbers were not comparable.
- **Success metrics:** every release-gating case either executes the named production path or is explicitly non-gating; zero unmapped manifest cases; three repeat runs on the same host/configuration are archived with p50/p95/p99, throughput, errors, and environment.
- **Validation:** build/run the mapped CMake benchmark targets in release mode; compare same-host runs and document variance before choosing numeric p99 thresholds. Use YCSB’s explicit workload/environment discipline, not its database thresholds.

#### Wave 2 — Secure lifecycle and failure contracts (P0; estimate: 3–5 engineer-weeks)

- **Goal:** demonstrate deterministic fail-closed behavior for module activation and license/edition gates.
- **Tasks:** verify production configuration and signature policy per supported OS/build; define expected outcomes for missing, malformed, tampered, unsigned, untrusted/revoked, and valid signatures; exercise missing/cyclic/version-incompatible dependencies and loader failures; ensure denied modules never reach activation and diagnostics map to the runbook.
- **Dependencies:** Wave 1 test/benchmark manifest; security owner confirms trust roots, revocation behavior, and supported-platform policy.
- **Risk/trade-off:** strict policy can reject deployments with incomplete trust configuration; cross-platform trust stores do not have identical semantics.
- **Success metrics:** 100% of the agreed positive/negative policy matrix has an automated test and expected result on each supported platform; zero unauthorized activations or silent permits in that matrix; every denial has a stable error category and operator action.
- **Validation:** focused unit and integration tests plus malformed-input and platform-specific verifier tests; security review of policy configuration and test evidence against NIST SSDF. Do not equate hash match with signer authorization.

#### Wave 3 — Real workload scale and operability (P1; estimate: 3–5 engineer-weeks)

- **Goal:** establish production-path capacity, tail-latency, and recovery evidence for loader, resolver, and wire service.
- **Tasks:** exercise resolver graphs at 100/1,000/10,000 modules and wire service at 10,000 concurrent sessions where supported; include churn, malformed requests, slow/disconnected clients, and bounded resource exhaustion; add controlled fault/soak runs; expose or validate latency, request traffic, error, and saturation signals and link alerts to the runbook.
- **Dependencies:** Waves 1–2; representative hardware and approved soak duration.
- **Risk/trade-off:** long stress runs increase CI cost and hardware variance; higher connection caps trade memory/file descriptors for capacity.
- **Success metrics:** zero unexpected stalls or unauthorized success in the agreed workload; p95/p99 and error latency are recorded; no more than 10% regression against the validated Wave 1 baseline; p99 stays within an SLO threshold agreed and frozen before release (no threshold is inferred from source names).
- **Validation:** registered focused/integration tests, production-path benchmark, sustained soak, and bounded fault-injection experiment with documented hypothesis/outcome, matching the Chaos Engineering method and SRE signals.

### v3.0.0 — strategic contract and lifecycle evolution

#### Wave 4 — Versioned module and wire compatibility (P0; estimate: 4–6 engineer-weeks)

- **Goal:** define machine-verifiable compatibility and migration behavior before any major-line contract break.
- **Tasks:** specify and version module manifest/ABI and supported wire protocol versions; make version parsing strict (reject malformed or ambiguous values); define compatibility negotiation, deprecation, and N-to-N-1 migration/rollback rules; map public headers and actual loader entry points to the contract; move wire-to-query/index/timeseries/process dispatch behind explicit service adapters, preserving the existing callback behavior and removing direct compile-time coupling.
- **Dependencies:** v2.5.0 Waves 1–2; API owners and downstream module inventory.
- **Risk/trade-off:** stronger validation may expose previously accepted malformed manifests; explicit negotiation can add startup work and maintenance burden.
- **Success metrics:** every loadable module declares the required identity/compatibility fields; 100% of malformed, unsupported, or incompatible fixtures fail deterministically before activation; supported migration paths pass N-1/N compatibility tests; wire request behavior remains unchanged with feature-module implementations linked only through the documented adapters.
- **Validation:** contract tests for current, previous, unsupported, missing, and malformed versions; cross-platform loader tests; wire integration/link tests for adapter boundaries; publish migration notes and API/wire compatibility matrix before changing the contract.

#### Wave 5 — Verifiable release provenance and trust lifecycle (P1; estimate: 4–6 engineer-weeks)

- **Goal:** bind module identity, dependencies, authorization, and release origin to auditable trust evidence.
- **Tasks:** design a signed manifest that binds module digest, version/ABI, dependencies, edition/feature policy, and build provenance; define key rotation, revocation, offline verification, and recovery ownership; produce and retain SBOM/provenance evidence for release artifacts.
- **Dependencies:** Wave 4 contract; security/governance owners decide signing authority, retention, and revocation policy.
- **Risk/trade-off:** key custody and revocation introduce operational dependencies; fail-closed behavior can reduce availability during trust-service outages, so offline verification and recovery must be explicit.
- **Success metrics:** all release modules have verifiable signed manifests and provenance records; tampered, revoked, or policy-incompatible artifacts are rejected in tests on every supported platform; key rotation and rollback drills complete with an auditable result.
- **Validation:** signature/manifest tamper tests, key-rotation/revocation and offline tests, SBOM/provenance review, and release artifact audit aligned to NIST SSDF.

#### Wave 6 — Measured lifecycle scaling and release acceptance (P1; estimate: 3–5 engineer-weeks)

- **Goal:** scale module resolution and prove the complete lifecycle without weakening deterministic behavior.
- **Tasks:** profile real dependency graphs and loader churn; optimize the resolver’s sorted-ready-set operations only if measured data confirms a bottleneck; define stable ordering and resource bounds; run upgrade, rollback, and recovery across a representative module set; finalize operator dashboards/runbooks and release evidence.
- **Dependencies:** Waves 3–5; stable manifest contract and representative hardware.
- **Risk/trade-off:** data-structure optimization can alter deterministic ordering or favor large graphs at the expense of small ones; defer changes unsupported by profiling.
- **Success metrics:** deterministic topological order and identical allow/deny decisions across repeated runs; 10,000-node graph results and memory use recorded; p95/p99 meet the approved SLO with no more than 10% regression on the small-graph baseline; upgrade/rollback and recovery tests pass.
- **Validation:** production resolver/load benchmarks at 100/1,000/10,000 nodes, property/regression tests for cycles and version constraints, cross-platform lifecycle integration, soak and recovery drill, and human release sign-off.

### Cross-wave risks, KPIs, and unresolved assumptions

- **Primary trade-off:** realistic integration evidence is slower and hardware-sensitive, but synthetic throughput cannot validate loader, socket, trust-store, or operator behavior. Keep microbenchmarks for diagnosis; do not use them as substitutes for release gates.
- **KPI dashboard:** release-gate mapping completeness; test-matrix pass rate; unexpected activation count (target 0); p50/p95/p99 and error latency; throughput; error rate; resource saturation; resolver time/memory by graph size; soak stalls; successful key-rotation/rollback drills. Numeric service SLOs and hardware limits must be set from representative deployment data before the relevant release gate.
- **Open assumptions:** supported OS/build-mode matrix; 10,000-session and 10,000-module target workload relevance; signing authority, key revocation, and offline behavior; telemetry/retention ownership; public ABI/wire compatibility requirements and downstream module inventory. No hardware runs, tests, or release artifacts were inspected in this update.
- **Wiki gap:** `ai_context/developer_llm_wiki/` has no curated themis module page. The 2026-11-02 wiki snapshot reports a non-zero source delta; refresh/index work is a separate follow-up. Module-local sources and root SOT take precedence for this plan.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze themis core runtime/gating/lifecycle contracts for current major line (Target: Q3 2026)
- [x] define explicit error taxonomy for license/load/verify incident classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [?] verify hardening for loader/verifier and wire-server internals (Target: Q4 2026) — the cited high-cardinality test uses stubs; production-path evidence is pending
- [?] validate platform-specific load behavior against bounded runtime contracts (Target: Q4 2026) — the cited THSTR-01 fixture does not load a shared library

### Phase 3: Error Handling and Edge Cases
- [?] verify fail-safe behavior for signature, dependency, and runtime gate faults (Target: Q4 2026) — policy is documented; platform/configuration execution evidence is pending
- [?] verify diagnostics across license, lifecycle, and wire incident classes (Target: Q4 2026) — runbook mappings exist; live signal coverage is unverified

### Phase 4: Tests
- [?] expand production-path regressions for loader/verify and wire-session edge scenarios (Target: Q4 2026) — current stress/soak fixtures are stubs
- [?] validate deterministic stress behavior against core runtime concurrency (Target: Q4 2026) — existing stub fixtures do not establish runtime concurrency behavior

### Phase 5: Performance and Hardening
- [?] lock benchmark-backed release gates for themis core hot paths (Target: Q4 2026; no run artifact was checked)
- [?] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026; no run artifact was checked)

### Phase 6: Documentation and Acceptance
- [x] core themis module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core themis surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [?] remaining hardening tasks closed for loader/gating/wire edge paths — test source exists, but passing execution evidence was not checked
- [?] release benchmark stabilization complete — benchmark sources are registered, but no baseline or run result was checked

## Known Issues and Limitations

- runtime behavior depends on platform loader constraints and trust material configuration.
- selected verification and wire-session edge scenarios need continued hardening.
- benchmark depth should continue expanding for wider core runtime workloads.

## Breaking Changes

No breaking change is planned for v2.5.0. The v3.0.0 proposal includes versioned module/API/wire contract evolution; any incompatible change requires explicit approval, migration notes, and a changelog entry under the root versioning policy.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `themis`
- [?] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — stress fixtures are stub-only; production-path execution and acceptance were not checked
- [?] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — soak source uses stubs; production-path registration and run results were not checked
- [?] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — runbook exists; current signal availability was not verified

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [?] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — require path-specific test and configuration evidence
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [?] Operator-critical paths have validated diagnostics, alerts, and runbooks (Target: Q1 2027) — runbook exists; live signal and alert coverage are unverified
