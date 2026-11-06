<!-- Status: research-backed proposal | source review: 2026-11-06; tests and benchmarks not run for this update -->
<!-- AI_Used: true; AI_Models: unknown; AI_Contributor: GitHub Copilot; Human_Review: false; Human_Author: ThemisDB Contributors -->
<!-- Exact model backend not verifiable from final document text alone. -->

# Config Module Roadmap

## Current Status

The module has implemented path resolution and caching, JSON/YAML parsing and
schema checks, platform file-change notifications, resolver metrics and an
in-memory path audit log, and an AES-256-GCM in-memory key/value store. Focused
test sources and resolver, migration-scanner, and release-gate benchmark
sources exist. These establish implementation breadth, not production
readiness: the contract audit lists unenforced limits, watcher tests do not
prove consumer reload/apply, and the benchmark targets have not been built or
measured in the evidence reviewed for this roadmap.

**Maturity assessment:** implemented but not release-validated. The most
important near-term gaps are enforced input bounds, an exact schema-support
contract, platform-level watcher evidence, and reproducible performance
baselines. The module supplies configuration primitives; it does not own
transactional application reloads, durable compliance auditing, deployment
rollouts, or external key management. Release and product status remain
governed by the root [`../../ROADMAP.md`](../../ROADMAP.md).

## In Progress

- [ ] Resolve each declared size, nesting, key-count, and validation-error bound
  by enforcing it or formally removing/de-scoping it, with negative and
  boundary tests (Target: v2.5.0; Wave 1).
- [ ] Publish and test the exact supported JSON Schema keyword/format subset;
  clarify behavior for unsupported keywords and sibling constraints (Target:
  v2.5.0; Wave 1).
- [ ] Establish delivered watcher callback behavior on supported operating
  systems without implying that notification reloads or applies configuration
  (Target: v2.5.0; Wave 2).
- [ ] Replace benchmark labels and placeholders with reproducible measurements
  and retained baselines before making performance claims (Target: v2.5.0;
  Wave 3).

## Planned Features

- [ ] Define a versioned, immutable configuration snapshot and atomic adoption
  contract for service consumers; do not make the config module responsible for
  deployment rollout (Target: v3.0.0; Wave 4).
- [ ] Design secret/key lifecycle and snapshot persistence so serialized
  configuration state cannot silently expose key material; require security
  review and a migration plan before changing the current format (Target:
  v3.0.0; Wave 5).
- [ ] Specify cross-process audit, compatibility, and scale requirements with
  consuming-service owners before adding interfaces or claiming compliance
  (Target: v3.0.0; Wave 6).

## Research Findings and Gap Analysis

The references below are technical standards or practitioner guidance, not
module-specific performance results or proof of compliance.

| Finding | Current code evidence | Roadmap implication |
|---|---|---|
| Security-focused configuration management treats approved baselines, change control, and monitoring as a lifecycle rather than just parsing a file [NIST SP 800-128](https://csrc.nist.gov/pubs/sp/800/128/upd1/final). | `ConfigAuditLog` is optional, bounded process memory and records successful path resolutions; it is not a durable change history. Watcher notification clears resolver cache but does not reload or apply settings. | Keep module-level notification and validation separate from consumer adoption; document the evidence boundary and define any audit integration with its owning service. |
| Schema validation requires predictable keyword evaluation and explicit format semantics [JSON Schema Draft 7 Validation](https://json-schema.org/draft-07/json-schema-validation). | `config_contract.h` describes a Draft 7 subset, while the implementation is partial; local references only are supported, formats are regex-based, and `enum`/`const` can return before sibling validation. | Publish a tested subset contract; do not claim Draft 7 conformance until parity tests establish it. |
| Secret management guidance calls for controlled provisioning, access, auditing, and rotation, rather than treating an encrypted value store as a complete secret-management service [OWASP Secrets Management Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Secrets_Management_Cheat_Sheet.html). | `ConfigEncryptedStore` uses AES-256-GCM in process memory, but `serialize()` includes the key bytes; it is not an external KMS or ciphertext-only backup. | Preserve explicit snapshot warnings in v2.5.0; design any key-provider/persistence boundary as a reviewed v3.0.0 migration, not an unannounced compatibility fallback. |
| Canary evaluation and rollback reduce the blast radius of production changes [Google SRE Workbook: Canarying Releases](https://sre.google/workbook/canarying-releases/). | The module has no transactional reload, staged rollout, last-known-good adoption, or rollback API; consumer services own those actions. | v3.0.0 may define a snapshot/adoption contract with consumers, but rollout policy and deployment orchestration remain outside this module. |
| Performance conclusions require workload and run evidence; a benchmark name or threshold label alone is not a measurement. | Three benchmark sources are registered, but the release-gate source measures watcher constants and has a metrics placeholder; no current baseline or benchmark pass was verified. | Treat existing numbers as candidate targets only. Record profile, hardware, sample distribution, and baseline before setting release gates. |

### Source-grounded observations

- `include/config/config_contract.h` declares file, schema, path, key/value,
  top-level-key, nesting, and error-count limits. The current gap audit found
  several are not enforced; policy constants alone are not protective bounds.
- Validator behavior is a subset, not established Draft 7 conformance.
- Watcher callbacks notify and clear resolver state; consumer reload, validation,
  and adoption remain caller responsibilities.
- The encrypted-store snapshot contains the current key. Key rotation blocks
  concurrent operations while re-encrypting under an exclusive lock.
- Benchmark sources cover useful resolver and scan paths, but watcher and
  metrics cases do not measure their labeled behavior, and no benchmark
  compilation/result is verified.

See [`MODULE_GAPS.md`](MODULE_GAPS.md),
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md),
[`ARCHITECTURE.md`](ARCHITECTURE.md), and [`SECURITY.md`](SECURITY.md) for the
source-review details and current evidence limits.

## Version Priorities

### v2.5.0 — enforce and measure the existing module boundary

Priority is P0 for safety-contract discrepancies and P1 for verification and
baseline work. Keep changes compatible with the v1.x public contract unless a
breaking change is approved and versioned under the root
[`../../VERSIONING.md`](../../VERSIONING.md) policy.

- [ ] Enforce or formally retire the declared resource bounds; add boundary
  tests (Target: v2.5.0).
- [ ] Specify supported schema keywords, combinations, formats, and
  unsupported behavior; test them as a contract (Target: v2.5.0).
- [ ] Verify watcher delivery, debounce, stop/start, and error behavior on
  supported platforms; keep reload/apply owned by consumers (Target: v2.5.0).
- [ ] Build and run focused tests and benchmarks, correct misleading benchmark
  cases, and retain representative baselines (Target: v2.5.0).
- [ ] Keep key-bearing snapshot handling and the non-durable audit boundary
  explicit in API and operator-facing docs (Target: v2.5.0).

### v3.0.0 — evolve configuration into a verifiable adoption contract

Strategic work is conditional on confirmed consumer requirements and a reviewed
design. It may change public API or persisted formats and therefore requires
explicit compatibility/migration decisions before implementation.

- [ ] Specify immutable, versioned snapshots and atomic consumer adoption,
  including validation failure, concurrent readers, last-known-good policy, and
  rollback (Target: v3.0.0).
- [ ] Decide whether persisted secret-bearing values need an external
  key-provider boundary; define key rotation, snapshot migration, access control,
  and failure behavior without promising a particular KMS integration
  prematurely (Target: v3.0.0).
- [ ] Define versioned schema migration and deprecation behavior for real
  consumers (Target: v3.0.0).
- [ ] Integrate with owner-provided durable audit and staged rollout systems only
  where consumers require them; do not duplicate deployment or compliance
  systems inside `config` (Target: v3.0.0).
- [ ] Re-run representative-scale and concurrency workloads against the v2.5.0
  baseline and publish compatibility and operations evidence (Target: v3.0.0).

## Wave Plan

Effort estimates are engineering person-weeks, not calendar commitments. They
assume one engineer familiar with the module and focused review; platform CI
availability, consumer integration, and security review can change the total.
KPIs are proposed acceptance gates, not current results.

### v2.5.0 waves

#### Wave 1 — Contract and input-boundary closure (P0; 2–3 person-weeks)

- **Goal:** make declared input and schema behavior precise and enforced.
- **Tasks:**
  - [ ] Map every policy constant to implementation and test evidence (Target:
    v2.5.0).
  - [ ] Enforce or formally retire unsupported limits and test just-below, at,
    and above each retained boundary (Target: v2.5.0).
  - [ ] Define supported schema keywords, combinations, formats, local `$ref`,
    unsupported-keyword behavior, and sibling evaluation (Target: v2.5.0).
  - [ ] Align public contract comments with the verified behavior (Target:
    v2.5.0).
- **Dependencies:** none beyond the current contract and source/test inventory.
- **Risk / trade-off:** enforcement may reject configurations previously
  accepted or consume less input; provide actionable errors and document
  migration impact rather than silently relaxing bounds.
- **Success metrics:** every retained safety bound has a rejecting boundary test;
  every removed bound has an explicit contract decision; every supported schema
  keyword has valid and invalid coverage; no unsupported keyword is described
  as enforced.
- **Validation:** focused resolver and validator suites plus hardening suite;
  fuzz/property tests for nested/oversized/malformed input where existing test
  facilities permit; check exception and diagnostic stability.

#### Wave 2 — Watcher and secret-handling assurance (P1; 2–4 person-weeks)

- **Goal:** verify the actual notification and secret-state boundaries without
  expanding module ownership.
- **Tasks:**
  - [ ] Test event delivery for create/change/rename/delete and atomic-file
    replacement (Target: v2.5.0).
  - [ ] Verify debounce, repeated start/stop, shutdown, and callback failures
    per supported platform (Target: v2.5.0).
  - [ ] Document that watcher callbacks do not reload or apply settings
    (Target: v2.5.0).
  - [ ] Review serialization callers and ensure snapshots are treated as key
    material (Target: v2.5.0).
- **Dependencies:** Wave 1's error/bounds contract; supported OS CI runners and
  consumer-owner review for reload expectations.
- **Risk / trade-off:** filesystem event semantics vary by OS and can coalesce;
  tests must assert supported guarantees, not assume every write yields one
  event. Changing snapshot format is out of v2.5.0 scope without migration
  approval.
- **Success metrics:** callback-delivery and shutdown assertions pass on every
  supported platform lane; no test or doc calls the in-memory audit durable or
  the serialized snapshot ciphertext-only.
- **Validation:** watcher and store focused/hardening suites on Linux, Windows,
  and macOS where supported; repeat event bursts and shutdown under load; review
  secret-bearing serialization and logging paths.

#### Wave 3 — Reproducible performance and release evidence (P1; 1–2 person-weeks)

- **Goal:** replace source labels with measured, repeatable evidence.
- **Tasks:**
  - [ ] Build the registered resolver, migration-scanner, and release-gate
    targets (Target: v2.5.0).
  - [ ] Fix or remove watcher-constant and metrics-placeholder cases (Target:
    v2.5.0).
  - [ ] Measure cache hit/miss, schema validation, encrypted-store get/set,
    watcher delivery, and 1K/10K scan workloads (Target: v2.5.0).
  - [ ] Record baseline, compiler/build profile, CPU/OS, dataset, repetitions,
    and distribution (Target: v2.5.0).
- **Dependencies:** Wave 1 and 2 semantics; benchmark and module dependencies
  available in CI or a documented representative environment.
- **Risk / trade-off:** filesystem, compiler, and hardware variance make a single
  timing brittle; gate only repeatable workloads and keep hardware-sensitive
  thresholds separate from correctness gates.
- **Success metrics:** all maintained benchmark cases build and run; no
  placeholder/constant-only case is reported as a performance gate; each
  candidate threshold is supported by retained repeated-run data and an
  agreed variance rule. Do not infer p99 from a mean-only benchmark.
- **Validation:** benchmark smoke run in CI; repeated release-profile runs on
  identified hardware; compare against the retained baseline and archive raw
  results before declaring any target passed.

### v3.0.0 waves

#### Wave 4 — Versioned snapshot and adoption design (P0; 3–5 person-weeks)

- **Goal:** define atomic, observable configuration adoption for services that
  need it while preserving ownership boundaries.
- **Tasks:**
  - [ ] Inventory real consumers and reload requirements (Target: v3.0.0).
  - [ ] Design immutable versioned snapshots, validation-before-publication,
    atomic swap, and concurrent-reader behavior (Target: v3.0.0).
  - [ ] Define bounded last-known-good and rollback policy (Target: v3.0.0).
  - [ ] Define API and migration contract before implementation (Target:
    v3.0.0).
- **Dependencies:** v2.5.0 contract closure and benchmark baseline; named
  consuming-service owners; architecture and API compatibility review.
- **Risk / trade-off:** cross-service rollout semantics can over-couple the
  module or imply stale configuration is always safe; make fail-closed behavior
  and per-consumer policy explicit.
- **Success metrics:** each participating consumer has an agreed reload/apply
  contract; concurrency tests observe no partial snapshot; invalid candidates
  never replace the active snapshot; rollback policy has a bounded, tested
  decision rule.
- **Validation:** API contract review; concurrent read/reload stress tests;
  fault injection for parse, validation, publication, and consumer-apply
  failures; end-to-end test in at least one explicitly participating service.

#### Wave 5 — Secret lifecycle and persisted-format migration (P0; 3–6 person-weeks)

- **Goal:** prevent key-bearing serialized state from being mistaken for secure
  ciphertext backup and establish an approved key lifecycle.
- **Tasks:**
  - [ ] Document data classification and current snapshot consumers (Target:
    v3.0.0).
  - [ ] Choose between prohibiting persistence and a reviewed versioned
    envelope/key-provider design (Target: v3.0.0).
  - [ ] Specify key identifiers, rotation, recovery, authorization, error
    behavior, and migration from existing snapshots (Target: v3.0.0).
  - [ ] Complete threat modeling and security review before implementation
    (Target: v3.0.0).
- **Dependencies:** security-owner approval, inventory of snapshot users, and
  Wave 4 state/versioning design where snapshots include configuration state.
- **Risk / trade-off:** external key services add availability and operational
  dependencies; a breaking format change can strand existing snapshots. Retain
  a fail-closed recovery path and explicit migration tooling/notes if a format
  change is approved.
- **Success metrics:** no persisted production snapshot contains raw key bytes;
  legacy snapshot migration (or explicit rejection) is deterministic and tested;
  rotation and interrupted recovery leave either the old or new complete state,
  never a partial state.
- **Validation:** security review and threat model; tests for tampering, wrong or
  unavailable keys, interrupted rotation, migration, access denial, and recovery;
  scan logs and test artifacts for secret disclosure.

#### Wave 6 — Operations, governance, and scale acceptance (P1; 2–4 person-weeks)

- **Goal:** demonstrate operational behavior and compliance integration at
  representative scale without claiming that the module itself provides
  compliance certification.
- **Tasks:**
  - [ ] Agree audit/event ownership with consuming services (Target: v3.0.0).
  - [ ] Define redaction, attribution, retention, and export boundaries
    (Target: v3.0.0).
  - [ ] Benchmark concurrent readers and reload/adoption paths (Target:
    v3.0.0).
  - [ ] Document failure response, safe rollback, compatibility/deprecation,
    and supported-platform matrix (Target: v3.0.0).
- **Dependencies:** Waves 4–5 decisions and participating service/security/
  operations owners.
- **Risk / trade-off:** durable audit, rollout, and policy engines may already
  exist elsewhere; duplicate systems increase cost and create inconsistent
  records. Prefer documented adapters to new in-module control planes.
- **Success metrics:** every required operational event has one named owner and
  tested path; no secret values appear in logs; scale results meet thresholds
  agreed from Wave 3 baselines; runbook recovery and compatibility checks pass
  for each supported release profile.
- **Validation:** service-level integration and recovery drills; repeated
  concurrency benchmarks on identified hardware; operational/security review;
  confirm root release gates and human sign-off separately.

## Implementation Phases

### Phase 1 — Design / API contract

- [ ] Approve the retained input bounds and exact schema subset in Wave 1.
- [ ] Obtain consumer and security-owner decisions for Waves 4 and 5 before
  proposing public interfaces or persisted-format changes.

### Phase 2 — Core implementation

- [ ] Implement only contract-approved v2.5.0 limits and schema behavior in
  `src/config/` and `include/config/` (Target: v2.5.0).
- [ ] Implement v3.0.0 snapshot/key-lifecycle changes only after API, migration,
  and threat-model approval (Target: v3.0.0).

### Phase 3 — Error handling and edge cases

- [ ] Cover oversized, deeply nested, malformed, unsupported-schema, watcher
  shutdown, key failure, and interrupted-rotation paths (Target: v2.5.0/v3.0.0
  by owning wave).

### Phase 4 — Tests

- [ ] Run focused tests for each changed concern and platform watcher tests;
  distinguish source/registration evidence from executed passes (Target:
  v2.5.0).
- [ ] Add concurrent snapshot, migration, and recovery tests for approved
  v3.0.0 behavior (Target: v3.0.0).

### Phase 5 — Performance / hardening

- [ ] Establish repeated benchmark baselines and remove misleading release-gate
  placeholders (Target: v2.5.0).
- [ ] Re-run representative concurrency, recovery, and security tests against
  the agreed v3.0.0 acceptance criteria (Target: v3.0.0).

### Phase 6 — Documentation and acceptance

- [ ] Synchronize API, security, operations, test, benchmark, and migration
  evidence with each delivered wave; obtain human review and release approval
  separately (Target: v2.5.0/v3.0.0).

## Production Readiness Checklist

- [ ] All retained resource bounds are enforced and boundary-tested.
- [ ] The supported schema subset and unknown-keyword behavior are explicit and
  covered by tests.
- [ ] Watcher guarantees are tested per supported OS; consumers own reload/apply.
- [ ] Encrypted-store snapshots and audit records have accurate data-boundary
  documentation.
- [ ] Focused tests and benchmark targets build and pass in a recorded profile.
- [ ] Baselines, statistical acceptance rules, and security/recovery evidence
  are retained for any release gate.
- [ ] v3.0.0 API and persistence changes have approved compatibility and
  migration notes.
- [ ] Human review, release readiness, and project-level sign-off are confirmed
  by their canonical governance owners.

## Known Issues & Limitations

- Several constants in `config_contract.h` are policy only and are not runtime
  limits; see [`MODULE_GAPS.md`](MODULE_GAPS.md).
- The validator implements a partial schema vocabulary; full Draft 7 or
  RFC-level format conformance is not established.
- Watcher notifications do not prove reload, validation, or application of new
  settings.
- The path audit is in-memory and not operator-attributed compliance evidence.
- `ConfigEncryptedStore::serialize()` includes key bytes; protect it as secret
  material. This component is not an external KMS.
- Benchmark targets and labels do not establish measured performance. Builds,
  test passes, representative hardware, and baselines remain unverified in the
  reviewed evidence.
- The module evidence Wiki was generated on 2026-11-02 and reports 1,450
  removed and 26 changed sources; treat its synthesis as stale until refreshed
  against current module sources.

## Open Assumptions & Knowledge Gaps

- The exact consuming services and their reload, rollout, and snapshot-persistence
  requirements have not been inventoried for this roadmap; Waves 4–6 require
  service-owner confirmation.
- Supported operating-system/version lanes for the watcher were not confirmed
  from executed CI evidence; Wave 2 must establish the matrix before its KPI can
  be closed.
- No benchmark run profile, representative hardware, raw result set, or stable
  baseline was verified; performance thresholds remain proposals until Wave 3.
- No compliance framework, audit-retention requirement, or in-module KMS
  requirement was verified. Do not equate this roadmap with a compliance claim
  or add an external service dependency without owner approval.
- Person-week estimates exclude cross-service implementation, waiting on CI
  runners, security review, and release governance; re-estimate after the
  dependency owners are identified.
- The Developer LLM Wiki freshness signal is not current enough to establish
  synchronized module context; refresh its generated status after this source
  roadmap is accepted.

## Breaking Changes

No breaking change is committed by this roadmap. v2.5.0 should preserve the
current public API where feasible; any incompatible behavior requires explicit
versioning and migration analysis. The contract header describes its policy as
stable within v1.x even though the root product is on the v2.4.0-alpha line; that
module/product version relationship needs owner clarification before relying on
the header's version label. Versioned snapshots, a changed encrypted
store format, or revised public adoption APIs are v3.0.0 candidates only after
consumer/security approval, compatibility review, and migration tests. Follow
the root [`../../VERSIONING.md`](../../VERSIONING.md) and
[`../../CHANGELOG.md`](../../CHANGELOG.md); do not infer a compatibility promise
from this proposal.

## Source References

- Implementation and public contract: `src/config/*.cpp`,
  [`../../include/config/config_contract.h`](../../include/config/config_contract.h),
  and [`../../include/config/`](../../include/config/README.md).
- Tests and registrations: [`../../tests/config/README.md`](../../tests/config/README.md),
  `../../tests/config/test_config_*.cpp`, and root `../../tests/CMakeLists.txt`.
- Benchmarks and registration: `../../benchmarks/config/*.cpp` and
  `../../benchmarks/CMakeLists.txt`.
- Current source audits: [`MODULE_GAPS.md`](MODULE_GAPS.md) and
  [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
