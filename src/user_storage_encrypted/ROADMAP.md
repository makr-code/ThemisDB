---
Author: ThemisDB Contributors
Created: 2026-08-08
Last Updated: 2026-11-07
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_Sources:
  - src/user_storage_encrypted/README.md
  - src/user_storage_encrypted/API.md
  - src/user_storage_encrypted/AUDIT.md
Related: issue #7465; proposal milestones v2.5.0 and v3.0.0 (unapproved)
---

# User Storage Encrypted Module Roadmap

Exact model backend not verifiable from final document text alone.

This is the single module roadmap for current behavior and proposed follow-up.
It is a source-evidenced planning document, not a security audit, release
commitment, test report, benchmark report, or production-readiness approval.
All work below is **proposal / open** unless stated as current source behavior.
No release dates are inferred.

## Current Status

- The root `ROADMAP.md` identifies the current project release as **v2.4.0-alpha**
  (root roadmap, updated 2026-11-06). This module roadmap proposes work for
  **v2.5.0** and **v3.0.0** only; neither target nor any work item is approved,
  scheduled, or represented as implemented.
- Current code provides a gocryptfs/FUSE backend, multi-level plugin storing
  user/group records as JSON files, KDF implementation, and separate rotation
  scheduler. See `src/user_storage_encrypted/{gocryptfs_backend.cpp,
  key_derivation_service.cpp,key_rotation_scheduler.cpp,multi_level_storage.cpp}`
  and `include/user_storage_encrypted/`.
- The KDF uses Argon2id when CMake finds libargon2 and PBKDF2-HMAC-SHA256 as
  fallback (`src/user_storage_encrypted/CMakeLists.txt`,
  `src/user_storage_encrypted/key_derivation_service.cpp`). The plugin creates
  the gocryptfs backend without injecting a KDF service; it parses rotation
  configuration but does not instantiate or wire the separate
  `KeyRotationScheduler` (`src/user_storage_encrypted/multi_level_storage.cpp`).
- Source does not establish transactional key rotation:
  `MultiLevelEncryptedStorage::rotateKey()` includes ignored unmount/rollback
  outcomes and does not roll back after a remount failure. `unmountAll()` ignores
  per-level results. Health checks cover backend presence/mount state rather
  than provider/scheduler health; `user_storage_container_size_bytes` is emitted
  but its gauge is not updated. Details and contract comparison are in
  `src/user_storage_encrypted/API.md` and `src/user_storage_encrypted/AUDIT.md`.
- The frozen contract in
  `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
  specifies guarantees that differ from demonstrated operational behavior and
  uses a separate namespace/error contract. Contract statements are not evidence
  that runtime paths provide those guarantees.
- Evidence limitations: Vault E2E cases unconditionally skip;
  `test_user_storage_encrypted_phase1_hardening.cpp` is not registered;
  high-cardinality stress uses stubs. Benchmark sources include placeholders or
  stubs, and `bench_user_storage_mount_latency.cpp` is not registered in the
  module benchmark CMake file. No measured baseline or enforceable performance
  gate is established. See the exact evidence map under
  [Known Issues & Limitations](#known-issues--limitations).

## In Progress

No module roadmap work is verified as in progress. Existing source and tests
describe current implementation, not approval or completion of any proposal
below. Maintainers must assign ownership and confirm scope before work is
treated as active.

## Planned Features

### Proposed v2.5.0 — near/mid-term hardening

All v2.5.0 items are **open proposals**, conditional on maintainer review and
release planning. The proposed order is behavioral decisions first, executable
validation next, then publication of evidence; it does not promise delivery in
v2.5.0.

1. **Wave 2.5-A — Contract and lifecycle failure semantics**
   - **Goal:** Resolve runtime/frozen-contract discrepancies and make mount,
     unmount, and rotation failure outcomes explicit before advertising stronger
     guarantees.
   - **Concrete tasks:**
     - [ ] Obtain owner decisions on the normative operational contract and lifecycle error semantics (Target: v2.5.0; proposal).
     - [ ] Specify partial-mount cleanup, per-level unmount error aggregation, and rotation failure/rollback behavior (Target: v2.5.0; proposal).
     - [ ] Implement only approved lifecycle decisions and add regression tests for each approved error path (Target: v2.5.0; proposal).
     - [ ] Align contract/API documentation with approved behavior in a separately scoped change (Target: v2.5.0; proposal).
   - **Dependencies:** Maintainer/API owner decisions on compatibility and
     error semantics; access to controlled gocryptfs/FUSE test environment for
     lifecycle cases. No implementation approach is preselected.
   - **Risks:** Fixes can alter externally observed error propagation; attempted
     cleanup may itself fail; false atomicity claims could risk data
     availability. Contract comments alone are not acceptance evidence.
   - **Proposed measurable KPIs (not current results):** 100% of injected
     mount/unmount/rotation failure cases return an observable failure or
     documented recovery state; no per-level unmount result is silently
     discarded; every frozen-contract guarantee is mapped to implementation
     evidence or explicitly revised/deferred by its owner.
   - **Validation:** **Unit:** deterministic failure injection for each
     lifecycle branch. **Integration:** encrypted mount/unmount and recovery
     scenarios with real gocryptfs/FUSE on a controlled host. **Benchmark:**
     capture before/after lifecycle latency distributions without claiming a
     target until a baseline is reviewed. **Security:** independent review of
     failure cleanup, key exposure, and recovery paths; this is not an audit
     claim. **Governance:** owner-approved contract decision, API/roadmap
     consistency review, and release-note impact assessment.
   - **Estimate:** 8–15 person-days, rough proposal; subject to maintainer
     estimation.

2. **Wave 2.5-B — Explicit KDF and rotation orchestration**
   - **Goal:** Make the selected derivation mode and rotation outcome
     reproducible and observable, without silently changing existing data
     compatibility.
   - **Concrete tasks:**
     - [ ] Decide whether plugin configuration injects a KDF service and which parameters it accepts (Target: v2.5.0; proposal).
     - [ ] Define persisted algorithm/parameter identity and existing-data compatibility and migration policy (Target: v2.5.0; proposal).
     - [ ] Define rotation callback success, retry, and persistence semantics; decide whether plugin scheduling is in scope (Target: v2.5.0; proposal).
     - [ ] Implement and test only the KDF/rotation decisions approved by maintainers (Target: v2.5.0; proposal).
   - **Dependencies:** Wave 2.5-A error/recovery semantics; key-provider owner
     input; a maintainer decision on backward compatibility and migration.
     Existing KDF modes are evidence, not a recommendation to switch modes.
   - **Risks:** Unversioned derivation changes can make existing data
     inaccessible; rotation can leave provider and container state inconsistent;
     callback/thread behavior may create deadlocks or duplicate attempts.
   - **Proposed measurable KPIs (not current results):** Each persisted
     derivation configuration identifies algorithm and parameters; tests prove
     reopening for every explicitly supported configuration; callback
     success/failure and retry state are asserted for all defined outcomes; no
     scheduler attempt is reported as successful solely because it ran.
   - **Validation:** **Unit:** KDF configuration, compatibility vectors, key
     buffer lifecycle, and scheduler failure/retry cases. **Integration:** create,
     reopen, and rotate encrypted data across supported provider/KDF
     configurations. **Benchmark:** measure selected KDF and rotation steps on
     declared hardware; report distributions and workload, not unsubstantiated
     thresholds. **Security:** review key lifecycle, parameter selection,
     provider transitions, and recovery/migration plan against cited guidance.
     **Governance:** approve compatibility/API changes and document upgrade
     behavior before release claims.
   - **Estimate:** 10–20 person-days, rough proposal; subject to maintainer
     estimation.

3. **Wave 2.5-C — Executable integration evidence and performance baseline**
   - **Goal:** Replace skipped/stub-only evidence for release-relevant paths with
     reproducible tests and measurements.
   - **Concrete tasks:**
     - [ ] Replace unconditional Vault E2E skips with a deliberately configured integration path or clearly scoped alternative (Target: v2.5.0; proposal).
     - [ ] Register and maintain hardening tests in CMake, and label stub coverage separately from real FUSE/provider coverage (Target: v2.5.0; proposal).
     - [ ] Register the mount-latency benchmark only if maintainers approve its workload as suitable (Target: v2.5.0; proposal).
     - [ ] Define a reproducible hardware/workload manifest and decide whether any measured threshold should be enforceable (Target: v2.5.0; proposal).
   - **Dependencies:** Waves 2.5-A/B for stable semantics; available CI or
     controlled host resources with gocryptfs/FUSE and any selected provider.
   - **Risks:** Host and filesystem variance can make results non-reproducible;
     secrets or external services can make CI unsafe; a stub or smoke pass can be
     misrepresented as production evidence.
   - **Proposed measurable KPIs (not current results):** Every claimed
     integration scenario is registered, executes its body, and reports
     pass/fail rather than unconditional skip; test inventory labels real versus
     stub coverage; at least one baseline report records environment, workload,
     sample count, and p50/p95/p99 for approved operations; any enforced gate has
     a reviewed numeric threshold and reproducible comparison process.
   - **Validation:** **Unit:** run registered module unit targets and retain
     results. **Integration:** run real lifecycle/provider scenarios in the
     controlled environment and record skips/failures transparently.
     **Benchmark:** capture baseline and repeatability before setting gates.
     **Security:** review test secrets, test isolation, and external-service
     configuration. **Governance:** verify CMake registration, evidence
     provenance, and that roadmap/readmes make no stronger claims than results.
   - **Estimate:** 6–12 person-days, rough proposal, excluding CI infrastructure
     provisioning and repeated hardware measurement; subject to maintainer
     estimation.

### Proposed v3.0.0 — strategic scope

These are strategic design proposals, not approved commitments. Scope depends
on v2.5 evidence and explicit owner decisions. They must not be read as a
promise to add backend, platform, or compliance support.

1. **Wave 3.0-A — Versioned operational contract and migration boundary**
   - **Goal:** Establish one maintained public behavior contract across
     operational APIs and any frozen/value contract, with explicit major-version
     compatibility and data migration rules.
   - **Concrete tasks:**
     - [ ] Decide whether the separate contract is retained, replaced, or versioned (Target: v3.0.0; proposal).
     - [ ] Reconcile namespaces, errors, lifecycle guarantees, threading, and key-handling obligations (Target: v3.0.0; proposal).
     - [ ] Define migration and deprecation policy for approved API/configuration changes and stored key metadata (Target: v3.0.0; proposal).
     - [ ] Add examples and migration tests for behavior approved at the v3 compatibility boundary (Target: v3.0.0; proposal).
   - **Dependencies:** Resolution and evidence from Waves 2.5-A/B; maintainer
     approval of the v3 compatibility boundary.
   - **Risks:** Contract/API migration may break source or stored-data
     compatibility; stronger guarantees may exceed what host FUSE/providers can
     supply.
   - **Proposed measurable KPIs (not current results):** Every public guarantee
     maps to a testable implementation path or is explicitly marked unsupported;
     migration tests cover every approved breaking configuration/data change;
     no unresolved normative-versus-operational discrepancy remains without an
     owner and disposition.
   - **Validation:** **Unit:** API/error and migration regression suite.
     **Integration:** upgrade/reopen/recovery tests from supported prior
     configurations. **Benchmark:** compare major-version behavior only for
     workloads with a v2 baseline. **Security:** review contract claims and
     migration/key-retention risks. **Governance:** approve v3 contract,
     deprecation, migration and release notes; no compliance claim by default.
   - **Estimate:** 10–20 person-days, rough proposal; subject to maintainer
     estimation.

2. **Wave 3.0-B — Operational health, observability, and supported-host policy**
   - **Goal:** Make module health and operational boundaries measurable and
     explicitly supported, based on deployment evidence rather than implied
     platform/security coverage.
   - **Concrete tasks:**
     - [ ] Decide provider/scheduler health semantics and actionable metrics, including whether and how to measure container size (Target: v3.0.0; proposal).
     - [ ] Document supported host/FUSE/gocryptfs combinations only from tested evidence (Target: v3.0.0; proposal).
     - [ ] Define owner/deployment responsibilities for permissions, backup, recovery, resource limits, logging, and incident response (Target: v3.0.0; proposal).
     - [ ] Implement only approved operational controls and health signals (Target: v3.0.0; proposal).
   - **Dependencies:** v2.5 executable integration/baseline evidence; v3.0-A
     contract decisions; deployment/security owners to define scope. No
     particular compliance regime or additional backend is assumed.
   - **Risks:** Metrics may be expensive or misleading; host-specific behavior
     may not generalize; a checklist can be confused with enforced control.
   - **Proposed measurable KPIs (not current results):** Every advertised health
     signal has a documented source and failure test; each supported platform
     entry links to an executed lifecycle validation record; each operational
     control is labeled as module-enforced, host-provided, or unverified.
   - **Validation:** **Unit:** metric/health state and failure mapping tests.
     **Integration:** lifecycle matrix on each host configuration proposed for
     support. **Benchmark:** measure observability overhead and record
     container-size methodology if adopted. **Security:** deployment threat and
     recovery review by owners, not a self-issued audit. **Governance:** owner
     sign-off for support matrix and control ownership; keep requirements and
     claims aligned.
   - **Estimate:** 12–25 person-days, rough proposal; subject to maintainer
     estimation and excluding unapproved platform/CI work.

## Implementation Phases

The phases below stage work from the proposed waves; they do not approve or
commit that work. All phases remain proposed/open, and the order is
**2.5-A → 2.5-B → 2.5-C → 3.0-A → 3.0-B** where dependencies apply.
Maintainers may reprioritize or reject proposals. Estimates remain independent
rough person-day ranges, not commitments or approved resourcing. No target
dates are assigned.

### Phase 1 — Design/API contract

- **Proposed wave mapping:** 2.5-A, 2.5-B; later contract boundary in 3.0-A.
- **Acceptance criteria (not passed; criteria only, not an additional task backlog):** Decisions have identified owners and approved dispositions; compatibility and migration behavior are explicit; unresolved guarantees are marked unsupported or deferred rather than implied.

### Phase 2 — Core implementation

- **Proposed wave mapping:** Approved implementation from 2.5-A/B; approved
  v3 contract/migration changes from 3.0-A.
- **Acceptance criteria (not passed; criteria only, not an additional task backlog):** Any resulting changes implement approved decisions, preserve the explicitly supported existing-data compatibility boundary, and map each advertised guarantee to an implementation path or an explicit limitation.

### Phase 3 — Error handling / edge cases

- **Proposed wave mapping:** 2.5-A/B, with migration and recovery edge cases
  from 3.0-A.
- **Acceptance criteria (not passed; criteria only, not an additional task backlog):** Injected failure cases produce an observable failure or documented recovery state; no per-level result or scheduler failure is silently represented as success; migration edge cases match the approved compatibility policy.

### Phase 4 — Tests

- **Proposed wave mapping:** Regression and integration tests from 2.5-A/B;
  executable evidence from 2.5-C; API/migration tests from 3.0-A.
- **Acceptance criteria (not passed; criteria only, not an additional task backlog):** Claimed scenarios are registered and execute their test bodies with pass/fail results; evidence identifies real versus stub coverage and transparently records skips, failures, and environment requirements.

### Phase 5 — Performance / hardening

- **Proposed wave mapping:** 2.5-C; operational health and supported-host
  evidence from 3.0-B.
- **Acceptance criteria (not passed; criteria only, not an additional task backlog):** Baseline reports include environment, workload, sample count, and p50/p95/p99; any enforceable threshold has a reviewed numeric value and repeatable comparison process. Each proposed support entry links to executed lifecycle evidence, and controls are labeled module-enforced, host-provided, or unverified.

### Phase 6 — Documentation / acceptance

- **Proposed wave mapping:** Documentation and governance validation across
  2.5-A/B/C and 3.0-A/B.
- **Acceptance criteria (not passed; criteria only, not an additional task backlog):** Documentation claims do not exceed approved behavior or recorded results; compatibility, support boundaries, and evidence provenance are reviewable; required owner sign-offs and acceptance decisions are recorded before any release or readiness claim.

## Production Readiness Checklist

All items are open checks, not claims that a gate has passed.

- [ ] Resolve operational API versus frozen-contract guarantees and record
      owner decisions. (Target: v2.5.0; proposal)
- [ ] Demonstrate explicit error propagation and recovery for mount, unmount,
      shutdown, and every rotation failure branch. (Target: v2.5.0; proposal)
- [ ] Establish supported KDF selection, parameter/version persistence,
      existing-data compatibility, and migration behavior. (Target: v2.5.0; proposal)
- [ ] Decide whether plugin KDF injection and scheduler/rotation wiring are
      supported features; test the selected behavior end to end. (Target: v2.5.0; proposal)
- [ ] Replace skipped E2E scenarios with executable, isolated integration
      coverage or accurately scope unsupported integrations. (Target: v2.5.0; proposal)
- [ ] Register and execute the intended hardening and lifecycle tests; retain
      reproducible evidence. Registration alone is not a pass. (Target: v2.5.0; proposal)
- [ ] Obtain representative benchmark measurements and review any threshold
      before making a performance or release-gate claim. (Target: v2.5.0; proposal)
- [ ] Validate provider/scheduler health and metric semantics, including the
      currently unupdated container-size gauge, if these signals are retained. (Target: v3.0.0; proposal)
- [ ] Review host permissions, FUSE/gocryptfs prerequisites, backup/recovery,
      secret handling, resource limits, and incident procedures with deployment
      owners; distinguish host controls from module-enforced controls. (Target: v3.0.0; proposal)
- [ ] Obtain security and maintainer review of approved implementation,
      compatibility, documentation, and release evidence. No security audit,
      compliance status, or production approval is claimed by this roadmap. (Target: v3.0.0; proposal)

## Known Issues & Limitations

Source-evidenced current limitations (not all independently re-tested for this
roadmap):

- `src/user_storage_encrypted/multi_level_storage.cpp`: plugin constructs
  `GocryptfsBackend` without KDF injection; parsed rotation configuration is not
  wired to `KeyRotationScheduler`; `unmountAll()` ignores per-level errors;
  `rotateKey()` is non-transactional in current source; health checks do not
  establish provider/scheduler health; content-size gauge is not updated.
- `src/user_storage_encrypted/key_derivation_service.cpp` and
  `src/user_storage_encrypted/CMakeLists.txt`: build-dependent Argon2id versus
  PBKDF2-HMAC-SHA256 selection exists. Compatibility across modes and existing
  data is not established by this roadmap.
- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h` versus
  `include/user_storage_encrypted/multi_level_storage.hpp` and
  `src/user_storage_encrypted/API.md`: separate frozen contract and operational
  API have differing namespaces/error surfaces and contract guarantees not
  demonstrated by implementation. Owner decision remains open.
- `tests/user_storage_encrypted/test_user_storage_encrypted_e2e_vault_integration_focused.cpp`
  calls `GTEST_SKIP()` in all eight scenarios. The CMake registration is in
  `tests/user_storage_encrypted/CMakeLists.txt`; the test source does not prove
  Vault integration. `test_user_storage_encrypted_phase1_hardening.cpp` exists
  but is not registered there. High-cardinality cases in
  `test_user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp`
  use stubs. See `tests/user_storage_encrypted/README.md`.
- `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`
  contains simulated/placeholder workloads;
  `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`
  is stub-backed. `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`
  is not registered by `benchmarks/user_storage_encrypted/CMakeLists.txt`.
  There is no measured baseline or enforceable gate evidenced by
  `benchmarks/user_storage_encrypted/README.md`.
- Host FUSE and gocryptfs availability constrain actual encrypted lifecycle
  validation. Platform claims should be limited to paths actually tested.

Open assumptions/questions for maintainers:

- Which API/contract artifact is normative for runtime behavior, and which
  existing guarantees must be implemented versus revised?
- Which KDF configurations and existing data formats must remain readable?
- Should rotation scheduling be plugin-owned or explicitly caller-owned?
- Which hosts, providers, and benchmark environments are in supported scope?
- What performance workload and evidence should justify any enforceable gate?
- Which operational/security controls belong in module code versus deployment
  policy?

## Breaking Changes

No breaking change is approved or scheduled. Potential v2.5/v3 changes requiring
explicit review include operational error propagation, API/error-contract
reconciliation, KDF configuration and persisted metadata, rotation callback
semantics, plugin configuration, and migration of any stored key/container
format. Do not change derivation or rotation behavior for existing data without
an approved compatibility and recovery plan. Any eventual breaking change needs
maintainer approval, versioning/migration notes, and release documentation.

## Canonical References and Technical Sources

Repository sources are the evidence for current module behavior. References to
other module docs explain detail and do not create a parallel roadmap:

- `src/user_storage_encrypted/README.md`
- `src/user_storage_encrypted/API.md`
- `src/user_storage_encrypted/AUDIT.md`
- `src/user_storage_encrypted/ARCHITECTURE.md`
- `src/user_storage_encrypted/PRODUCTION_REQUIREMENTS.md`
- `src/user_storage_encrypted/RESEARCH.md`
- `include/user_storage_encrypted/multi_level_storage.hpp`
- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
- `src/user_storage_encrypted/multi_level_storage.cpp`
- `src/user_storage_encrypted/gocryptfs_backend.cpp`
- `src/user_storage_encrypted/key_derivation_service.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `tests/user_storage_encrypted/CMakeLists.txt` and
  `tests/user_storage_encrypted/README.md`
- `benchmarks/user_storage_encrypted/CMakeLists.txt` and
  `benchmarks/user_storage_encrypted/README.md`
- Root `ROADMAP.md` for current aggregate release posture.

### Research findings and proposed-wave mapping

The external material below is standards, guidance, a research paper, or a
measurement tool—not ThemisDB design approval. The mapping is a proposal based
on the repository evidence above.

- **Key lifecycle — NIST SP 800-57 Part 1 Rev. 5 (key-management guidance):**
  covers cryptographic key management over key states and lifecycle, including
  protection, use, compromise response, archival, and destruction; it does not
  select this module's rotation interval or prove its key handling. **Evidence:**
  the module has provider key IDs, build-dependent KDF choices, and a separate
  scheduler; plugin rotation configuration is not wired to that scheduler, and
  transactional rotation is not established. **Gap → proposed wave:** agree
  key/version identity, compatibility, recovery, and failure semantics in
  **2.5-A**, then decide and test KDF/rotation orchestration in **2.5-B**.
- **AEAD and nonce integrity — RFC 5116 (AEAD interface/algorithm
  requirements) and NIST SP 800-38D (GCM-specific recommendation):** RFC 5116
  defines an authenticated-encryption interface and requirements; SP 800-38D
  specifies GCM, including IV construction/uniqueness constraints. These do not
  authorize inventing a module-specific cipher format. **Evidence:** encrypted
  levels are mounted through gocryptfs. **Gap → proposed wave:** document and
  validate the supported gocryptfs format/configuration and compatibility in
  **2.5-B/2.5-C**; gocryptfs owns filesystem encryption and nonce/format
  mechanics. Do not add a duplicate custom AEAD layer or claim independent
  nonce management for the module.
- **FUSE daemon trust and privilege boundary — Linux kernel FUSE
  documentation (implementation documentation):** FUSE routes filesystem
  operations through a userspace daemon; mount ownership and options such as
  `allow_other` affect who can access a mount. The daemon, kernel, host
  permissions, and key provider are therefore part of the trust boundary.
  **Evidence:** the backend depends on gocryptfs/FUSE and module health checks
  mount state, not the full host authorization boundary. **Gap → proposed
  wave:** set host/daemon privilege and supported-mount policy, and distinguish
  module-enforced from deployment controls in **3.0-B**, after executable
  lifecycle evidence in **2.5-C**.
- **Crash consistency and durable rename — Linux `fsync(2)` documentation
  (system-call semantics) and Pillai et al., USENIX OSDI 2014 (research
  paper):** syncing a file does not necessarily persist its containing
  directory entry; a rename can be atomic for namespace visibility without
  being durable across a crash unless the relevant file and directory
  persistence steps are handled. The paper demonstrates that crash-consistency
  behavior depends on filesystem and application operation sequences.
  **Evidence:** current user/group JSON writes use `std::ofstream` directly;
  source does not establish a write-temp/sync/rename/directory-sync protocol.
  **Gap → proposed wave:** decide durability and recovery semantics in
  **2.5-A**, then test crash/failure behavior on explicitly selected filesystems
  as part of **2.5-C**; any implementation protocol remains a design decision.
- **Backup, restore, and storage security — NIST SP 800-209 (storage
  infrastructure security guidance) and SP 800-88 Rev. 2 (media sanitization
  guidance):** these cover storage-system security and sanitization, not
  module-specific certification. Cryptographic erase is appropriate only
  where the applicable sanitization conditions are met and all relevant
  cryptographic keys/copies are made inaccessible; deleting a record or
  rotating a provider key alone does not demonstrate sanitization.
  **Evidence:** module source does not implement backup/restore or a verified
  sanitization process. **Gap → proposed wave:** define ownership, recovery
  tests, retained-key handling, and any sanitization boundary with deployment
  owners in **3.0-B**; **2.5-A** must first clarify rotation/recovery failures.
- **Compliance — GDPR Article 32 (legal requirement):** requires
  risk-appropriate technical and organizational measures, including measures
  relevant to confidentiality, integrity, availability/resilience, and
  restoration/testing; encryption is one example measure, not compliance by
  itself. **Evidence:** this module stores user/group JSON at configured
  security levels but does not establish a complete organizational or
  processing-control program. **Gap → proposed wave:** identify control
  ownership and deployment boundaries in **3.0-B**. No wave or encryption
  feature can establish GDPR compliance.
- **Workload benchmarking — fio project documentation (benchmark/workload
  tool, not a standard):** fio supports configured I/O workloads and latency
  percentile reporting; meaningful percentiles require declared workload,
  sample/measurement conditions, and environment. **Evidence:** existing
  module benchmarks include simulation/stub workloads; the mount-latency
  source is unregistered, so these do not constitute real encrypted-I/O
  baselines. **Gap → proposed wave:** define reproducible real workloads and
  report distributions (for example p50/p95/p99 with environment and sample
  context) in **2.5-C** before considering thresholds. No measured result or
  performance target is asserted here.

### Target-use-case assessment and boundaries

**Observed scope:** the API creates, reads, overwrites, lists, and deletes
per-user and per-group JSON files beneath `users/` and `groups/` for a selected
configured security level. User JSON fields include `user_id`, `username`,
`email`, `full_name`, `roles`, `classification`, and creation/update times;
group JSON fields include `group_id`, `name`, `description`, `member_ids`,
`classification`, and creation time. Configuration distinguishes an
unencrypted `offen` level from encrypted levels using gocryptfs and configured
providers. These observed shapes support assessing a file-backed user/group
registry or metadata store; they do not demonstrate general ThemisDB
data-at-rest encryption, authorization boundaries between levels, or
suitability for any formal classification regime. Level labels in
configuration are not certification.

**Proposed usage boundary:** consider this component only for the configured
user/group records and only with a trusted host, gocryptfs/FUSE daemon, mount
permissions, and key-provider path. Treat a mounted level as available to
authorized host processes, not as protected from a privileged host or
compromised daemon. Keep authorization, backup/recovery, audit, and
sanitization obligations explicit at deployment level; do not use `offen` as
an implicit encrypted fallback or treat a level name as access control.

**Core trade-offs:** separate file records and levels may simplify scoped
storage and provider selection, but many small-file operations, directory
enumeration, FUSE transitions, and provider access can add latency and
operational dependencies. Encryption at rest does not itself protect live
mounted data or establish crash durability, availability, restoreability, or
compliance. Stronger durability and recovery controls may add writes,
synchronization, retention, and operational complexity; their exact design
and acceptable cost remain proposals.

### External references

- **Standards:** RFC 9106, *Argon2 Memory-Hard Function for Password Hashing*,
  https://www.rfc-editor.org/rfc/rfc9106.html
- **Standards:** RFC 8018, *PKCS #5: Password-Based Cryptography Specification
  Version 2.1* (including PBKDF2),
  https://www.rfc-editor.org/rfc/rfc8018.html
- **Standards/guidance:** NIST SP 800-57 Part 1 Rev. 5,
  https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-57pt1r5.pdf
- **Standards:** RFC 5116, *An Interface and Algorithms for Authenticated
  Encryption*, https://www.rfc-editor.org/rfc/rfc5116.html
- **Standards/guidance:** NIST SP 800-38D, *Recommendation for Block Cipher
  Modes of Operation: Galois/Counter Mode (GCM) and GMAC*,
  https://csrc.nist.gov/pubs/sp/800/38/d/final
- **Implementation documentation:** Linux kernel FUSE documentation,
  https://docs.kernel.org/filesystems/fuse.html
- **System-call documentation:** Linux `fsync(2)`,
  https://man7.org/linux/man-pages/man2/fsync.2.html
- **Research paper:** Pillai et al., *All File Systems Are Not Created Equal:
  On the Complexity of Crafting Crash-Consistent Applications*, USENIX OSDI
  2014, https://www.usenix.org/conference/osdi14/technical-sessions/presentation/pillai
- **Standards/guidance:** NIST SP 800-209, *Security Guidelines for Storage
  Infrastructure*, https://csrc.nist.gov/pubs/sp/800/209/final
- **Standards/guidance:** NIST SP 800-88 Rev. 2, *Guidelines for Media
  Sanitization*, https://csrc.nist.gov/pubs/sp/800/88/r2/final
- **Legal text:** GDPR, Regulation (EU) 2016/679, Article 32,
  https://eur-lex.europa.eu/eli/reg/2016/679/oj
- **Benchmark tool documentation:** fio,
  https://fio.readthedocs.io/en/latest/fio_doc.html
- **Implementation reference, not a standard:** gocryptfs upstream project and
  documentation, https://github.com/rfjakob/gocryptfs
- **Implementation reference, not a standard:** libfuse documentation,
  https://libfuse.github.io/doxygen/

The proposed waves, KPIs, estimates, support decisions, usage boundaries, and
release mapping in this document are ThemisDB planning proposals; they are not
recommendations or requirements imposed by these external sources.
