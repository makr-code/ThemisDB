# ThemisDB Audit Hub and Canonical Map

**Author:** ThemisDB Contributors
**Created:** 2026-09-13
**Last Updated:** 2026-11-09
**Status:** active
**Repository Metadata:** `VERSION=2.4.0-alpha`
**Canonical Rule:** `/audit/**` is the audit source of truth; `/docs/**` is downstream publication/legacy mirror unless explicitly marked otherwise.
**Source-verified status:** The production audit logger exists in source, but design-level Wave-C pass claims remain provisional until validated against the real production sink and persistence path.

<!--
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Exact model backend not verifiable from final document text alone.
-->

> **Baseline rule (2026-09-07):** Root audit documents without direct date reference in filename (`AUDIT.md`, `README.md`, `WAVE_C_AUDIT_EVIDENCE.md`) are synchronized to the latest consolidated baseline report and source-verified implementation status.

---

## Scope Lock

- **Authoritative audit stack:** `audit/`
- **Downstream docs mirror/publication:** `docs/`
- **Historical/archive:** `audit/docs/audit-reports/v1.4.1/`, `audit/docs/audit-framework/evidence/v1.4.1/`, `audit/docs/ARCHIVED/`
- **Non-canonical working evidence:** `ai_working/**` (draft/evidence only; never release/security source of truth)

---

## Current Audit Set (Authoritative)

| Document | Purpose |
|---|---|
| `FULL_DEEP_DIVE_AUDIT_2026-10-31.md` | Full deep-dive audit of source and docs (73 modules, 79 calibrated findings, doc-vs-code drift matrix) |
| `BENCHMARK_MODULE_COVERAGE.json` | Source/CMake/CTest and evidence-status inventory for all 73 source modules |
| `ROADMAP.md` | Audit-derived production-readiness roadmap (WS1–WS11, milestones M0–M4, traceability matrix); non-normative for release status |
| `THEMISDB_AUDIT_MATURITY_SECURITY_MONETARY_REPORT_2026-08-31.md` | Consolidated source-verified audit, maturity, security, and monetary update |
| `AUDIT.md` | Central security/compliance/release audit summary |
| `MATURITY_REPORT_2026-08.md` | Monthly maturity and gate posture |
| `IMPLEMENTATION_AUDIT_2026-09-30.md` | Current implementation sync report |
| `MARKER_LOCATIONS_2026-11-09.md` | Current commit-pinned raw marker inventory (`src/`, reproducible Git scan) |
| `MARKER_GAP_CLASSIFICATION_2026-11-09.md` | Current source-triage checkpoint; candidate-by-candidate review remains open |
| `ACTIONABLE_TODOS_2026-10-01.md` | Historical TODO reality-check; refresh before using its counts |
| `MARKER_LOCATIONS_2026-10-01.md` | Historical raw marker snapshot |
| `MARKER_GAP_CLASSIFICATION_2026-10-01.md` | Historical marker classification and non-TODO split |
| `MARKER_LOCATIONS_2026-09-30.md` | Historical raw marker snapshot |
| `MARKER_GAP_CLASSIFICATION_2026-09-30.md` | Historical marker classification status |
| `IMPLEMENTATION_AUDIT_2026-09-14.md` | Prior implementation sync report |
| `IMPLEMENTATION_AUDIT_2026-09-13.md` | Prior implementation sync report |
| `IMPLEMENTATION_AUDIT_2026-08-26.md` | Earlier implementation sync report |
| `IMPLEMENTATION_AUDIT_2026-08-12.md` | Historical implementation sync report |
| `IMPLEMENTATION_AUDIT_CORRECTED_2026-08-08.md` | Historical deep-dive delta report |
| `IMPLEMENTATION_AUDIT_2026-08-08.md` | Historical raw delta report |
| `IMPLEMENTATION_AUDIT_2026-08-07.md` | Historical base snapshot |
| `BSI_C5_2026_THEMISDB_AUDIT.md` | BSI-C5-2026 delta audit and actions |

---

## Inventory Matrix (`/audit` ↔ `/docs`)

| Topic | Canonical (`/audit`) | Downstream (`/docs`) | Status |
|---|---|---|---|
| Root audit navigation | `../AUDIT.md` → `audit/AUDIT.md` | n/a | ✅ synced |
| EU AI Act compliance set | `docs/compliance/EU_AI_ACT_*.md` (inside `audit/`) | `../docs/compliance/` currently does not mirror these files | 🟡 divergence tracked |
| Audit framework runbook/templates | `docs/audit-framework/*` (inside `audit/`) | `../docs/audit-framework/*` | 🟡 mirrored with overlap; `/audit` authoritative |
| Legacy `Audit` topic docs | `docs/Audit/*` (inside `audit/`) | `../docs/Audit/*` (inventory/gap-analysis docs) | ✅ scope-separated (no same-file duplicates) |
| Versioned audit bundles | `docs/audit-reports/v1.4.1/*` (inside `audit/`) | `../docs/audit-reports/v1.4.1/*` | 🟡 historical mirror; archival-only framing required |

---

## Quick Navigation

### Compliance & Governance (canonical in `/audit`)
- `docs/compliance/EU_AI_ACT_COMPLIANCE.md`
- `docs/compliance/EU_AI_ACT_RISK_MAPPING.md`
- `docs/compliance/EU_AI_ACT_EVIDENCE_BUNDLE.md`
- `docs/audit-framework/AUDIT_GOVERNANCE_STRUCTURE.md`
- `../docs/de/compliance/compliance_full_checklist.md`

### Release & Security Evidence
- `FULL_DEEP_DIVE_AUDIT_2026-10-31.md`
- `ROADMAP.md`
- `THEMISDB_AUDIT_MATURITY_SECURITY_MONETARY_REPORT_2026-08-31.md`
- `AUDIT.md`
- `MATURITY_REPORT_2026-08.md`
- `IMPLEMENTATION_AUDIT_2026-09-30.md`
- `ACTIONABLE_TODOS_2026-10-01.md`
- `MARKER_LOCATIONS_2026-11-09.md`
- `MARKER_GAP_CLASSIFICATION_2026-11-09.md`
- `MARKER_LOCATIONS_2026-10-01.md`
- `MARKER_GAP_CLASSIFICATION_2026-10-01.md`
- `MARKER_LOCATIONS_2026-09-30.md`
- `MARKER_GAP_CLASSIFICATION_2026-09-30.md`
- `evidence/waves/manifests/security_wave_c_integrated_evidence_chain_2026-10-01.md`
- `IMPLEMENTATION_AUDIT_2026-09-14.md`
- `IMPLEMENTATION_AUDIT_2026-09-13.md`
- `IMPLEMENTATION_AUDIT_2026-08-26.md`
- `IMPLEMENTATION_AUDIT_CORRECTED_2026-08-08.md`
- `../docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
- `../security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
- `../docs/governance/GA_PROMOTION_SIGN_OFF.md`

### Historical / Archival Paths
- `docs/audit-reports/q2-2026-quality-wave-1/AUDIT.md`
- `docs/audit-reports/v1.4.1/` (archival package)
- `docs/audit-framework/evidence/v1.4.1/` (archival evidence bundle)
- `docs/ARCHIVED/`

---

## Current Status Snapshot

- **2026-10-31 deep-dive verdict:** ThemisDB 2.4.0-alpha is **not production-ready** (unweighted ≈ 2.4/5; 2 Critical, 22 High). Critical: fail-open recovery in `WALStorage` (cross-shard transaction and replication WAL) and failover/DR handlers reporting success without action. See `FULL_DEEP_DIVE_AUDIT_2026-10-31.md` and the remediation plan in `ROADMAP.md`.
- The real production audit implementation is present in source: `include/utils/audit_logger.h` and `src/utils/audit_logger.cpp`.
- The current source-backed status is: implemented and substantively present, but not fully end-to-end GA-/production-certified without a live run against the actual sink and persistence path.
- The Wave-C proof file `tests/audit/test_audit_wavec_integrity_export_focused.cpp` is a mock-based design validation harness, not a direct proof of the real production backend.
- For current implementation drift handling, use `IMPLEMENTATION_AUDIT_2026-09-30.md` first.
- The dated marker artifacts `MARKER_LOCATIONS_2026-08-31.md` and `MARKER_GAP_CLASSIFICATION_2026-08-31.md` are historical snapshots; use `ACTIONABLE_TODOS_2026-10-01.md`, `MARKER_LOCATIONS_2026-10-01.md`, `MARKER_GAP_CLASSIFICATION_2026-10-01.md`, and `IMPLEMENTATION_AUDIT_2026-09-30.md` for current-state assessment.

### Benchmark density and release-claim evidence (2026-11-09)

The [machine-readable benchmark module matrix](BENCHMARK_MODULE_COVERAGE.json) was generated from `develop` at `c21f7bba0cb44d58e5ad49f64781a76703b3ee56`. It enumerates 73 `src/` modules, finds 390 named C++ benchmark-source candidates, maps 338 to module directories, and lists the other 52 sources that do not map to a `src/` module path separately. The historical 339-file total in the 2026-10-31 audit is retained as a dated figure, not reused as a current coverage measure. These counts establish source presence only; they do not prove execution, representativeness, or performance.

The matrix records target/CTest registration, build profile, CI triggers, expected result paths, missing workload metadata, and the latest benchmark CI attempt. The static registration scan maps 388 candidate sources to benchmark targets and CTest registrations; it also reports one unregistered source and eight CTest entries without a statically mapped source target. Of the 73 modules, 26 are marked high-risk by the audit hotspot list or `release_critical` CTest labels; 21 still lack workload-specific release-claim records. Their `claim_inventory_review` rows now link available source docs, partial declared-target mappings, benchmark candidates, and an accountable issue, but do not claim a complete claim-to-workload crosswalk. Three modules remain `gap`; 70 are `blocked` because the latest weekly attempt did not execute workloads; none is marked `covered` or `not applicable`. CMake conditionals are not evaluated by the source scanner, so registrations that cannot be mapped statically remain unverified. The weekly performance workflow retains artifacts for 14 days; its latest run [42548800031](https://github.com/makr-code/ThemisDB/actions/runs/42548800031) failed during CMake configuration because RocksDB was unavailable. Benchmark build/run and report jobs were skipped, so no benchmark result artifact or PASS evidence exists. The PR CTest workflow checks registration structure and does not execute workloads. No module is marked `not applicable` without a specific reviewed rationale.

Claim disposition follows the existing owners rather than duplicating their implementation:

- Query `LIMIT 10` at 100K/10M rows remains a **gap** under [#7488](https://github.com/makr-code/ThemisDB/issues/7488): merged PR #7804 reports synthetic pull-source measurements but no retained raw result from a storage scan.
- `src/performance/` regression methodology remains a **gap** under [#7534](https://github.com/makr-code/ThemisDB/issues/7534); noise policy and maintainer-approved thresholds are not recorded here.
- Wave A representative-hardware results remain **blocked** under [#7810](https://github.com/makr-code/ThemisDB/issues/7810); merged PR #8012 refreshed manifests but states that required measurements are pending and the referenced baseline artifact is absent. GPU capability/fallback evidence remains with [#7492](https://github.com/makr-code/ThemisDB/issues/7492).
- The unsupported 1,024-request LLM statement is **withdrawn** by merged PR #7805 under [#7490](https://github.com/makr-code/ThemisDB/issues/7490); no hardware benchmark is claimed for it.

For every high-risk module, `claim_inventory_review` in the matrix now lists the available performance/roadmap source documents, any declared target-to-benchmark mappings from `benchmarks/benchmark_target_mapping.json`, candidate benchmark sources, an owner issue, and a concrete evidence follow-up. Existing module issues are linked only where their acceptance criteria require benchmark or metric evidence; otherwise #7828 owns portfolio triage until a workload-specific issue is verified. The target-mapping file is explicitly treated as a partial index, not a complete claim inventory or proof of execution, so unresolved modules remain `gap`.

Regenerate on the intended source branch with:

```bash
python3 tools/ci/audit_benchmark_coverage.py \
  --module-matrix audit/BENCHMARK_MODULE_COVERAGE.json
```

Provide `--latest-run-url`, `--latest-run-sha`, `--latest-run-result`, and `--latest-run-note` to record a CI attempt. A run counts as benchmark evidence only when it succeeds, retains machine-readable results, and documents workload metadata; missing values stay `gap`/`blocked`. The release-claim details, evidence references, scanner exclusions, and exact source revision are in the JSON matrix.

### Source-Verified Reality Check (2026-09-07)

The audit implementation in source is real and present, but a number of high-level audit claims in `/audit` need careful interpretation:

- `include/utils/audit_logger.h` and `src/utils/audit_logger.cpp` implement the production audit logger with hash chaining, queue-size guarding, log rotation, fsync support, PKI-signature metadata, encryption fallback, and SIEM forwarding.
- `tests/audit/test_audit_wavec_integrity_export_focused.cpp` is a focused production-backed regression harness; it validates Wave-C audit behavior against the real `themis::utils::AuditLogger` persistence path rather than an in-memory mock logger.
- Current source-backed state: the audit subsystem is implemented and operational at the core-logger layer, but the strongest Wave-C certification claims should be treated as design-level validation until an end-to-end run against the real production sink and storage path is executed.

### Compliance Snapshot

| Framework | Status | Primary Evidence |
|---|---|---|
| ISO 27001:2022 | ✅ 95% | `../docs/de/compliance/compliance_full_checklist.md` |
| BSI C5 (2026 delta) | 🟡 92% | `BSI_C5_2026_THEMISDB_AUDIT.md` |
| GDPR / DSGVO | ✅ 98% | `../docs/de/compliance/compliance_dpia.md` |
| EU AI Act | 🟡 65% | `docs/compliance/EU_AI_ACT_COMPLIANCE.md` |
| NIS2 | ✅ 94% | `../docs/de/compliance/compliance_bcp_drp.md` |
| SOC 2 Type II | ✅ 90% | `../docs/de/compliance/compliance_full_checklist.md` |

> Percentages are checklist-based self-assessments, not third-party certifications or attestations
> (see drift item 11 in `FULL_DEEP_DIVE_AUDIT_2026-10-31.md`).

---

## Provenance

- Root release-readiness status: `../ROADMAP.md`
- Root release trace: `../CHANGELOG.md`
- Final governance gate: `../docs/governance/GA_PROMOTION_SIGN_OFF.md`
- Module-level implementation truth: `../src/*/ROADMAP.md`, `../src/*/AUDIT.md`
