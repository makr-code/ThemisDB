---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Updates Module Roadmap

## Current Status

This roadmap tracks source-visible work and evidence gaps. It is not a
production-readiness approval. The root maturity aggregate currently classifies
`updates` as **HARDENING** in [`../../ROADMAP.md`](../../ROADMAP.md); that
aggregate takes precedence over older module-local GA/readiness statements.

### Current source state (2026-11-01)

- Public interfaces, implementation sources, focused tests, and benchmark
  sources are present under `include/updates/`, `src/updates/`,
  `tests/updates/`, and `benchmarks/updates/`.
- `tests/updates/CMakeLists.txt` registers `test_updates_*.cpp` focused targets
  (with the diagnostics source excluded from the prefix glob; see test
  registration and module test sources for the actual target inventory).
- `benchmarks/updates/CMakeLists.txt` registers five benchmark sources when
  `THEMIS_BUILD_BENCHMARKS` is enabled. The cluster-stress benchmark is
  separately registered by `benchmarks/CMakeLists.txt`, with a conditional
  CTest smoke entry.
- The Windows Debug build of `module_updates_test_updates_production_focused`
  succeeded. Its seven SEC-01 regression tests pass directly; the full target
  runs 125 tests, with 124 passing and one unrelated ZSTD fallback expectation
  failing. Linux/release evidence and representative benchmark results remain
  open; test-source presence does not establish a release gate.

## Planned Features

Items below cover source hardening, evidence, and documentation; no release
readiness is implied. Target timing requires maintainer approval.

- [x] Record the configured focused test targets and CTest names from `tests/updates/CMakeLists.txt` (Target: 2026-11-01).
- [~] Close audit finding SEC-01: require a configured verifier, signed manifest, system-trusted certificate chain, and policy-allowed issuer; reject cache/config bypasses (Target: M1 · 2026-12-15).
- [ ] Complete the focused updates test build and record the CTest results (Target: next release-evidence cycle).
- [ ] Define reviewed benchmark baselines and acceptance thresholds in `PERFORMANCE_EXPECTATIONS.md` (Target: next release-evidence cycle).
- [x] Confirm `bench_updates_cluster_stress_q4` and its CTest smoke entry are registered by the repository-level benchmark CMake file (Target: 2026-11-01).
- [ ] Reconcile historical Phase/GA sign-offs with the root maturity aggregate (Target: next roadmap review).

## Implementation Phases

### Phase 1 — Design / API contract
- [ ] Confirm the reviewed public lifecycle, patch, migration, rollout, and diagnostics contracts against `include/updates/` and focused test sources (Target: next release-evidence cycle).
- [x] Define the SEC-01 trust contract: system trust-store chain plus exact normalized issuer-DN allowlist; unsigned overrides are unsupported (Target: 2026-11-01).

### Phase 2 — Core implementation
- [x] Confirm the cluster-stress benchmark target and CTest smoke entry are registered at repository level; avoid duplicate registration in the module CMake file (Target: 2026-11-01).
- [x] Enforce manifest hash, verifier, signature, signing certificate, certificate-chain, and issuer-policy checks; prevent signature-cache and `verify_signatures=false` bypasses (Target: 2026-11-01).

### Phase 3 — Error handling and edge cases
- [ ] Map failure and rollback cases to focused tests and record any uncovered public contract path (Target: next release-evidence cycle).
- [x] Reject missing trust material, self-signed/expired signers, and tampered manifest payloads without accepting cached success (Target: 2026-11-01).

### Phase 4 — Tests
- [x] Capture the configured updates test target and CTest inventory (Target: 2026-11-01).
- [~] Windows Debug SEC-01 regressions pass 7/7; investigate the unrelated ZSTD fallback test failure in the 125-test target and obtain Linux/release evidence (Target: v2.5.0 Wave 1).
- [ ] Build and execute the focused updates tests; record exact results (Target: v2.5.0 Wave 1).

### Phase 5 — Performance and hardening
- [ ] Establish reviewed benchmark baselines, thresholds, and a reproducible run manifest (Target: v2.5.0 Wave 3).

### Phase 6 — Documentation and acceptance
- [ ] Reconcile historical module evidence and readiness assertions with current root governance and verified run artifacts (Target: v2.5.0 Wave 3).
- [x] Synchronize the manifest verification contract across API, trust, operations, and developer-wiki documentation (Target: 2026-11-01).

## Research Assessment and Source-Grounded Gaps

The root roadmap classifies `updates` as **HARDENING**; that project-level
maturity status remains authoritative. This assessment is based on the current
contracts and source inventory, not on historical module sign-offs. The module
has 27 public headers, 25 implementation files, 11 focused test sources, and
six benchmark sources (five registered in the module CMake file; the cluster
stress target is registered at repository level).

| Area | Evidence in this repository | Gap and consequence |
|---|---|---|
| Performance and scale | State, canary, migration, coordinated-update and long-run benchmark sources are present under [`benchmarks/updates/`](../../benchmarks/updates/); [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) maps cases and counters. | No representative benchmark run or approved baseline is established. Source-commented rates and test thresholds are candidates, not release gates. |
| Security and resilience | [`manifest_database.h`](../../include/updates/manifest_database.h) defines `ManifestDatabase::verifyManifest()` requirements: configured verifier, signature, trusted certificate chain, allowed issuer, and matching payload hash. Seven SEC-01 regressions are in [`test_updates_production.cpp`](../../tests/updates/test_updates_production.cpp); they cover missing verifier/signature/certificate, untrusted or expired signing certificates, tampering, and the disabled-verification path. | Linux/release validation remains open. Update-signature checks do not establish metadata freshness, anti-replay/downgrade policy, signing-key rotation, artifact provenance, or an SBOM contract. |
| Recovery and availability | [`update_state_machine.h`](../../include/updates/update_state_machine.h) exposes in-flight journal detection; [`hot_reload_engine.h`](../../include/updates/hot_reload_engine.h) exposes file backups and rollback IDs; [`schema_migration.cpp`](schema_migration.cpp) implements an undo log. | In-flight detection is not automatic recovery, state checkpoints do not back up installed files, and schema rollback is a separate data concern from binary rollback. Recovery across these boundaries needs interruption/fault-injection evidence. |
| Rollout and usability | File/delta update, schema migration, canary/blue-green, cluster coordination, and tenant scheduling surfaces exist. [`canary_rollout.h`](../../include/updates/canary_rollout.h) selects membership per node; [`coordinated_update_manager.h`](../../include/updates/coordinated_update_manager.h) waits/signals through injected callbacks; [`blue_green_deployment.h`](../../include/updates/blue_green_deployment.h) tracks local slots. | The module does not supply cluster consensus, deployment-wide authorization, or a universal operator workflow. No representative tenant/node-scale evidence establishes usability or rollout limits; readiness, pause/abort policy, and target topology must be supplied or agreed with integrators. |
| Governance and maintainability | [`updates_api_contract.h`](../../include/updates/updates_api_contract.h) freezes its contract for v2.x; module diagnostics and focused test/benchmark registration are documented. | Full focused-suite results, release-profile evidence, and representative hardware baselines are missing. Historical readiness claims must not be used as current evidence. |

### Research basis and transfer limits (reviewed 2026-11-07)

- The [TUF specification](https://theupdateframework.github.io/specification/latest/index.html)
  treats update metadata trust, key compromise, and stale/replayed metadata as
  distinct from simply verifying an artifact signature. The
  [SLSA Build Provenance specification](https://slsa.dev/spec/v1.2/build-provenance)
  recommends verifiable source/build provenance bound to an artifact. These
  inform the v3.0.0 artifact-trust design; they do not imply current compliance.
- [Google SRE's canary guidance](https://sre.google/workbook/canarying-releases/)
  calls for representative exposure and metric-based comparison before rollout
  proceeds. Kubernetes' [rolling-update guidance](https://kubernetes.io/docs/tasks/run-application/update-deployment-rolling/)
  also makes rollout capacity and stalled-progress limits explicit. These inform
  the rollout validation criteria, not a requirement to adopt Kubernetes.
- [NIST SP 800-193](https://csrc.nist.gov/pubs/sp/800/193/final) frames
  resiliency as protect, detect, and recover. Its scope is platform firmware;
  this plan transfers the recovery principle, not a claim of database
  certification.
- Hu, Wang, and Zhou, “[Online Schema Evolution is (Almost) Free for Snapshot
  Databases](https://www.vldb.org/pvldb/vol16/p140-hu.pdf),” PVLDB 16 (2023),
  supports treating schema evolution and availability as a compatibility
  problem. The implementation choice still requires ThemisDB workload and
  storage-format validation. PostgreSQL's [upgrade guidance](https://www.postgresql.org/docs/current/upgrading.html)
  likewise distinguishes upgrade paths by compatibility and storage format; it
  is product-specific evidence, not a ThemisDB requirement.
- The [NTIA SBOM minimum-elements report](https://www.ntia.gov/report/2021/minimum-elements-software-bill-materials-sbom)
  informs the proposed release inventory work. Compliance scope, format, and
  retention policy require maintainer and deployment-owner decisions.
- [RFC 9019](https://www.rfc-editor.org/rfc/rfc9019) and
  [RFC 9124](https://www.rfc-editor.org/rfc/rfc9124) provide firmware-update
  architecture and manifest-model references. They are Informational IoT
  firmware RFCs; use them only as transferable design input, not database
  conformance criteria.

## Prioritized Release Plan

Effort estimates are engineering person-weeks, not calendar commitments; they
include implementation/design and focused validation but exclude external
deployment-owner waiting time. Current totals are 7–12 person-weeks for v2.5.0
and 9–17 person-weeks for v3.0.0, assuming sequential waves and no major
rework. Release windows are not set by this module roadmap. v2.5.0 work is
constrained to compatible v2.x behavior; v3.0.0 is where an approved
major-contract change may be proposed.

### v2.5.0 — Evidence closure and compatible hardening

Priority order: (P0) prove the existing trust contract in release configurations;
(P0) demonstrate safe recovery at file/state/schema boundaries; (P1) create
reviewed performance and operations gates.

#### Wave 1 — Trust contract and release evidence (P0, 2–3 person-weeks)

- **Goal:** prove that the implemented SEC-01 checks hold in supported release
  configurations and establish a reproducible test baseline without changing
  the v2 public contract.
- **Tasks:**
  - [ ] Build and run every registered `updates` focused test in Linux Debug and
    Release configurations; record commands, compiler, configuration, target
    names, and complete results (Target: v2.5.0 Wave 1).
  - [ ] Run the SEC-01 signer/manifest regression matrix in Linux and Release,
    including valid trusted signer, missing trust material, untrusted/expired
    signer, signature/hash mismatch, cached-success attempt, and disabled
    verification; investigate the existing unrelated ZSTD fallback expectation
    failure separately (Target: v2.5.0 Wave 1).
  - [ ] Reconcile the remaining module security documentation with the enforced
    mandatory-signature contract and update the test/evidence map from actual
    run outputs (Target: v2.5.0 Wave 1).
- **Dependencies:** Supported Linux and Release presets, OpenSSL and system trust
  configuration, and a test signer/certificate fixture.
- **Risks / trade-offs:** Certificate-chain behavior can vary with platform
  trust stores. Do not weaken verification to make a test pass; distinguish
  environment setup from a code defect.
- **Success metrics:** All registered focused targets pass in both named
  configurations; all invalid-signature/trust cases reject and the valid
  trusted-signer case succeeds; results include no unreported or skipped
  release-critical targets.
- **Validation:** Focused CTest suite, direct SEC-01 regression target, review
  trust/security/API docs against `ManifestDatabase::verifyManifest()`, and
  preserve test logs as release evidence.

#### Wave 2 — Interruption-safe recovery and rollback (P0, 3–5 person-weeks)

- **Goal:** define and verify a safe outcome for interruption between journal,
  file replacement, health check, rollback, and schema-migration boundaries.
- **Tasks:**
  - [ ] Specify restart behavior for each persisted in-flight state; distinguish
    detection from recovery and require an explicit safe reconciliation decision
    before another update proceeds (Target: v2.5.0 Wave 2).
  - [ ] Add deterministic fault-injection tests around file apply/restore,
    rollback metadata, migration commit/undo, and coordinated-update callback
    failure; verify that state-only checkpoints are never treated as file
    backups (Target: v2.5.0 Wave 2).
  - [ ] Fix only source-confirmed gaps in persistence ordering, rollback
    reporting, or recovery diagnostics, preserving v2.x public API compatibility
    (Target: v2.5.0 Wave 2).
  - [ ] Update runbook steps and recovery evidence to match tested restart
    behavior (Target: v2.5.0 Wave 2).
- **Dependencies:** Wave 1 test baseline; approved restart/recovery policy from
  storage and deployment owners.
- **Risks / trade-offs:** Durable recovery may require additional I/O or retained
  backups; it cannot make arbitrary schema/data changes reversible. Avoid
  claiming atomic cluster recovery from per-node callbacks.
- **Success metrics:** Every enumerated interruption point has a deterministic
  expected result; the complete fault matrix has zero silent-success outcomes
  and zero fixture divergence after a successful rollback; unresolved recovery
  states prevent rollout progression.
- **Validation:** Restart/subprocess and failure-injection tests; rollback and
  migration regression suites; inspect resulting files, schema markers, journal,
  diagnostics, and recovery runbook against each injected failure.

#### Wave 3 — Benchmarks, rollout gates, and operator evidence (P1, 2–4 person-weeks)

- **Goal:** convert source-presence and candidate thresholds into controlled,
  reviewed capacity and rollout acceptance criteria.
- **Tasks:**
  - [ ] Execute registered update benchmarks on named representative hardware;
    capture raw output, build/compiler, workload, data size, node/tenant count,
    repetitions, and host saturation (Target: v2.5.0 Wave 3).
  - [ ] Add or map representative end-to-end cases for artifact verification and
    transfer, canary decision/abort, migration, rollback, and coordinated
    workload; record p50/p95/p99 latency, throughput, error rate, memory, and
    bytes transferred (Target: v2.5.0 Wave 3).
  - [ ] Have module and release owners approve workload-specific thresholds and
    regression budgets in `PERFORMANCE_EXPECTATIONS.md`; do not promote
    source-commented candidate values to gates without measured evidence
    (Target: v2.5.0 Wave 3).
  - [ ] Reconcile module-local readiness and historical benchmark claims with
    current run artifacts and the root maturity aggregate (Target: v2.5.0 Wave 3).
- **Dependencies:** Waves 1–2 and access to representative benchmark hardware,
  workload, and deployment owners.
- **Risks / trade-offs:** Results from different hardware/workloads are not
  directly comparable; larger cohorts improve confidence but consume release
  time. Avoid fragile microbenchmark-only gates.
- **Success metrics:** Every registered benchmark is mapped to a workload and
  run record; each approved gate is reproducible with a named baseline and
  threshold. Proposed starting guardrail: no greater than 5% p95 regression
  against the same-host baseline, subject to owner approval and noise analysis
  before beta; otherwise use the reviewed per-workload budget.
- **Validation:** Repeat controlled benchmark runs, compare percentiles and
  error/saturation counters, review rollout stop/rollback criteria, and run
  documentation/governance consistency checks.

### v3.0.0 — Major-contract and architecture evolution

Priority order: (P0) approve the compatibility and trust architecture; (P1)
implement verifiable artifact provenance and staged schema compatibility; (P1)
close distributed rollout/recovery gaps for explicitly selected deployment
topologies.

#### Wave 4 — Major-version contracts and compatibility design (P0, 2–3 person-weeks)

- **Goal:** agree the v3 contract boundary before implementation; v2.x APIs stay
  frozen unless a separate compatibility policy is approved.
- **Tasks:**
  - [ ] Inventory public update APIs, duplicated verification-result semantics,
    persisted manifest/state formats, and direct consumers; classify each as
    stable, deprecated, or proposed v3 change (Target: v3.0.0 Wave 4).
  - [ ] Specify versioned manifest and migration contracts, including upgrade,
    downgrade, mixed-version operation, cancellation, failure, and ownership
    behavior; run the API change-impact review before proposing signatures
    (Target: v3.0.0 Wave 4).
  - [ ] Select supported fleet sizes, deployment topologies, artifact stores,
    trust/key-rotation model, schema compatibility window, and supported
    compliance/SBOM policy with maintainers and operators (Target: v3.0.0 Wave 4).
- **Dependencies:** v2.5.0 Waves 1–3 evidence and explicit product/maintainer
  decisions on supported deployments.
- **Risks / trade-offs:** A broader API can improve consistency but expands
  migration burden. Do not infer quorum, topology, or compliance needs from the
  transport-agnostic module API.
- **Success metrics:** Every public and persisted contract in scope has a
  versioning/migration decision and mapped compatibility test; design review
  records owners and unresolved questions before code is accepted.
- **Validation:** API compatibility inventory, documented architecture/design
  review, contract tests for current behavior, and migration guide review.

#### Wave 5 — Artifact provenance and online schema compatibility (P1, 3–6 person-weeks)

- **Goal:** add an approved, verifiable chain from release artifact identity to
  build evidence, and make schema change compatibility explicit across rollout
  and rollback.
- **Tasks:**
  - [ ] Based on Wave 4 decisions, design and implement versioned release
    metadata binding immutable artifact digests to trusted build provenance,
    metadata freshness/anti-replay policy, key rotation/revocation, and a
    machine-readable SBOM; preserve a documented v2-to-v3 migration path
    (Target: v3.0.0 Wave 5).
  - [ ] Design schema evolution as compatible stages (additive change,
    backfill/validation, reader/writer switch, delayed destructive cleanup) and
    define when downgrade is no longer safe (Target: v3.0.0 Wave 5).
  - [ ] Test malformed, tampered, stale/replayed, wrongly sourced, and
    wrong-identity artifact evidence; test old/new binary and schema combinations
    across apply, abort, and rollback (Target: v3.0.0 Wave 5).
- **Dependencies:** Approved v3 formats, signing identity/rotation ownership,
  SBOM format/retention policy, and Wave 4 compatibility decisions.
- **Risks / trade-offs:** Stronger provenance introduces release-pipeline and
  trust-root dependencies; additive migrations can lengthen the dual-format
  period and increase storage. TUF/SLSA/SBOM adoption must be scoped rather than
  claimed as compliance by the presence of signature checks.
- **Success metrics:** 100% of supported v3 artifacts bind to an immutable
  digest and verified provenance/SBOM; every negative trust case rejects before
  installation; every supported mixed-version/schema state is covered by the
  compatibility matrix, with rollback allowed only where explicitly safe.
- **Validation:** Security review, malformed/replay/provenance test matrix,
  schema-migration integration tests on recoverable copies, SBOM/provenance
  verification, and release artifact audit.

#### Wave 6 — Bounded multi-node rollout and recovery control (P1, 4–8 person-weeks)

- **Goal:** provide a testable, bounded rollout protocol for selected cluster
  topologies, with measurable pause/abort and recovery decisions.
- **Tasks:**
  - [ ] Define whether multi-node coordination remains a caller-provided control
    plane or gains a module-owned coordinator; specify quorum, readiness,
    version-skew, retry, timeout/cancellation, and isolated-node behavior
    without assuming that callback ordering is consensus (Target: v3.0.0 Wave 6).
  - [ ] Implement the approved protocol with bounded rollout stages and explicit
    health/latency/error stop criteria; expose enough sanitized progress and
    failure evidence for operators (Target: v3.0.0 Wave 6).
  - [ ] Add deterministic multi-node fault-injection and representative load
    tests for node loss, partition, failed migration, abort, and coordinated
    recovery at the Wave 4-approved fleet sizes (Target: v3.0.0 Wave 6).
- **Dependencies:** Waves 4–5, stable telemetry signals, and a named deployment
  topology/control-plane owner.
- **Risks / trade-offs:** Stronger safety gates can reduce availability or slow
  rollout; partition recovery and external control-plane failures need explicit
  ownership. Do not promise zero downtime outside validated topologies.
- **Success metrics:** No node advances past a failed readiness/quorum gate;
  every injected failure yields an explicit bounded stop, isolation, or approved
  rollback; all surviving nodes converge to an allowed version state within
  the approved recovery objective, with p95/p99 and saturation within reviewed
  Wave 3 budgets.
- **Validation:** Multi-node fault-injection/partition tests, mixed-version
  compatibility suite, representative end-to-end benchmark, security review of
  coordination/authentication boundaries, and operator recovery rehearsal.

## Risks, Trade-offs, and Open Assumptions

- v2.5.0 success depends first on reproducible evidence, not a claim that every
  missing control requires new code. Keep the existing v2 API stable; only fix
  gaps confirmed by tests or review.
- v3.0.0 provenance, freshness, key rotation, SBOM, cluster coordination, and
  schema-compatibility designs are proposals pending Wave 4 decisions. No
  product-specific compliance target, signing service, fleet size, or recovery
  objective is yet established by this module evidence.
- The proposed 5% p95 regression guardrail is a starting point for owner review,
  not an approved service-level objective or current benchmark result.
- Database migration rollback cannot be equated with restoring old binaries;
  destructive schema cleanup must wait until the rollback window is explicitly
  closed.
- Firmware update references (NIST SP 800-193, RFC 9019, and RFC 9124) are
  transferable design input only. Any standards mapping or compliance statement
  requires a separate scoped assessment.

## Production Readiness Checklist

- [ ] Current focused test results recorded for a named build configuration.
- [ ] SEC-01 signature-verification tests pass in Linux and release configurations.
- [ ] Representative benchmark results and approved thresholds recorded.
- [ ] Operational and deployment-specific controls verified by their owners.
- [ ] Root maturity aggregate and module-local readiness statements reconciled.

## Known Issues & Limitations

- Test and benchmark source presence does not establish passing runs.
- The cluster-stress target is registered at repository level, but no successful benchmark run is established here.
- Throughput values in the benchmark source are candidate expectations only; approved thresholds and representative-hardware results remain open.
- Deployment-wide authentication, authorization, topology, and recovery policy are outside the module's standalone C++ contract.
- Unsigned update manifests are rejected in every mode; deployments must configure the system trust store and allowed signing issuer. Linux/release validation is still pending.

## Compatibility and change control

Public API compatibility follows the repository versioning policy. A public
contract change should update the applicable header comments, `API.md`, focused
tests, and the relevant operational documentation. SEC-01 intentionally changes
runtime behavior: unsigned manifests and `verify_signatures=false` are rejected.
No C++ function signature changed.

## Breaking Changes

No C++ function signature changed. The SEC-01 runtime behavior change is
documented above; any future source-level breaking change must follow the
repository versioning policy and be documented in the affected header and API
contract.

## Historical status notes

Older sections in prior copies of this file described v1.1.0/v2.4.0 readiness,
UPDP gate passes, and future phase assignments. Their run evidence and approval
state could not be verified against the current CMake registration and current
release evidence during this documentation audit; those claims are not active
status.
