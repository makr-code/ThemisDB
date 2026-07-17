[docs](../docs/index.md) > PIPELINE_LESSONS_LEARNED  
**Version:** 1.0  
**Status:** living document — updated after each incident or quarterly review  
**Issue:** #5420 (Phase 7 — SOP, Reviews & Documentation)  
**Last Updated:** 2026-07-16

# ThemisDB Pipeline — Lessons Learned & Common Failure Patterns

## Purpose

This document captures lessons learned across all phases of the ThemisDB End-to-End AI/ML Artifact Pipeline (RAG → Package → Build → Deploy → Recover), organized by pipeline stage. It serves as the institutional memory for the pipeline team.

**How to use this document:**

- After every P0/P1 incident, add a new entry to the relevant stage section.
- After every quarterly governance review, review entries for actionable patterns.
- Cross-reference with `docs/reviews/PIPELINE_REVIEW_TEMPLATES.md` (REVIEW-T04/T07).

---

## Table of Contents

1. [Summary Table](#1-summary-table)
2. [RAG Stage Lessons](#2-rag-stage-lessons)
3. [Package Stage Lessons](#3-package-stage-lessons)
4. [Build Stage Lessons](#4-build-stage-lessons)
5. [Deploy Stage Lessons](#5-deploy-stage-lessons)
6. [Recover Stage Lessons](#6-recover-stage-lessons)
7. [Cross-Stage Lessons](#7-cross-stage-lessons)
8. [Common Failure Patterns](#8-common-failure-patterns)
9. [Improvement Backlog](#9-improvement-backlog)

---

## 1. Summary Table

| ID | Stage | Severity | Date | Root Cause | Status |
|----|-------|----------|------|-----------|--------|
| LL-001 | Package | Medium | 2026 Q1 | Missing provenance field not caught until audit | Resolved |
| LL-002 | Build | High | 2026 Q1 | Artifact integrity check bypassed under timeout | Resolved |
| LL-003 | Deploy | High | 2026 Q2 | Canary abort threshold too loose for latency regression | Resolved |
| LL-004 | Recover | Medium | 2026 Q2 | Snapshot pre-dating dependency changes caused replay gap | Resolved |
| LL-005 | RAG | Low | 2026 Q2 | Embedding dimension drift undetected across corpus versions | Mitigated |
| LL-006 | Cross-stage | High | 2026 Q3 | Policy gate evaluated against stale policy version | Resolved |

---

## 2. RAG Stage Lessons

### LL-005 — Embedding Dimension Drift Across Corpus Versions

**Date:** 2026 Q2  
**Severity:** Low (caught pre-production)  
**Impact:** RAG retrieval returned semantically misaligned results for 3% of queries  

**What happened:**  
A corpus update changed the embedding model from 768-dim to 1024-dim without updating the retrieval configuration. The mismatch was not detected until integration tests.

**Root cause:**  
No automated dimension compatibility check between corpus version and active adapter rank.

**What went well:**  
Integration tests in staging caught the issue before production deployment.

**Resolution:**  
Added dimension compatibility assertion to SOP-P01 step 4 and `themisdb-cli compat check`.

**Action taken:**  
- Added `embedding_dimension` field to corpus registry metadata
- `themisdb-cli validate adapter` now verifies dimension alignment

**Lessons:**
1. Always assert embedding dimensions explicitly in the retrieval config. Do not rely on implicit compatibility.
2. Corpus updates must trigger re-validation of adapter compatibility, not just corpus format checks.
3. Dimension mismatches surface late in integration if not checked early in the RAG config.

---

## 3. Package Stage Lessons

### LL-001 — Missing Provenance Field Not Caught Until Audit

**Date:** 2026 Q1  
**Severity:** Medium (compliance risk)  
**Impact:** Package merged without `training_corpus` field; discovered during quarterly audit  

**What happened:**  
A developer omitted the `training_corpus` field in the adapter manifest. The PR review focused on functional correctness and the missing field went unnoticed. The gap was only identified during the quarterly audit evidence review.

**Root cause:**  
No automated schema validation in the PR CI pipeline for `.adapkg` manifests.

**What went well:**  
The quarterly audit process caught the gap within the same quarter.

**Resolution:**  
- Added `themisdb-cli validate adapter` to CI pipeline as a mandatory step
- Added schema validation to the adapter PR template (REVIEW-T01)

**Lessons:**
1. Provenance fields must be validated automatically in CI, not relied upon in human review.
2. Audit templates should have mandatory fields surfaced as required checkboxes, not narrative descriptions.
3. The cost of a missed field at merge is much higher than the cost of a validation gate at PR open.

---

## 4. Build Stage Lessons

### LL-002 — Artifact Integrity Check Bypassed Under Timeout

**Date:** 2026 Q1  
**Severity:** High (security risk)  
**Impact:** A build artifact reached staging without a valid integrity signature  

**What happened:**  
During a high-load build period, `themisdb-cli artifact verify` timed out due to resource contention. The operator, under time pressure, proceeded to staging promotion assuming the artifact was correct.

**Root cause:**  
No hard gate enforced the integrity check result. The operator had discretion to override. Build pipeline timeout was too short for large model artifacts (> 8 GB).

**What went well:**  
The staging integrity check (applied again at deploy gate) caught the missing signature before production.

**Resolution:**  
- Integrity check is now a hard gate in CI/CD pipeline with no human override path
- Build pipeline timeout increased from 30 min to 90 min for large artifacts
- Alert fires to operator when build approaches 70% of timeout budget

**Lessons:**
1. Integrity checks must be hard gates, not soft recommendations. Human override should require explicit Auditor co-approval.
2. Resource-based timeouts on security checks are a systemic risk pattern. Monitor timeout approach time.
3. The staging re-check that caught the issue is valuable defense-in-depth. Keep it.

---

## 5. Deploy Stage Lessons

### LL-003 — Canary Threshold Too Loose for Latency Regression

**Date:** 2026 Q2  
**Severity:** High (user-visible degradation for 12 minutes)  
**Impact:** Canary promoted to 100% while p99 latency was 22% above baseline  

**What happened:**  
The canary abort threshold for latency p99 was set at 30% above baseline. A new adapter caused a 22% latency increase, which was under the threshold. The canary was promoted to 100% and users experienced significant degradation before the operator noticed in the metrics dashboard.

**Root cause:**  
Threshold calibrated against historical CPU-heavy workloads, not the new GPU-accelerated inference path. No automatic abort was triggered.

**What went well:**  
Operator monitoring was active. Alert fired after 8 minutes of full promotion.

**Resolution:**  
- Default p99 latency abort threshold tightened to 15% above baseline (see SOP-P04)
- Added automatic abort if threshold exceeded for > 3 consecutive minutes
- GPU inference path added as a separate threshold tier

**Lessons:**
1. Canary thresholds require workload-aware calibration. A single global threshold is insufficient when workload profiles differ significantly.
2. Automatic abort on sustained threshold breach is safer than relying on operator reaction time.
3. Observe for at least 5 minutes at each canary percentage before promotion. Latency issues may not manifest in the first 60 seconds.

---

## 6. Recover Stage Lessons

### LL-004 — Snapshot Pre-Dating Dependency Changes Caused Replay Gap

**Date:** 2026 Q2  
**Severity:** Medium (manual recovery required, < 10 records affected)  
**Impact:** WAL replay to pre-switch snapshot skipped records written after a dependency schema change  

**What happened:**  
The pre-switch snapshot was taken 4 hours before deployment (standard procedure). In that window, a schema migration added a new column. The WAL replay stopped at the migration boundary because the snapshot schema did not match the WAL schema for records after the migration.

**Root cause:**  
Snapshot timing did not account for schema migrations in the deployment window. Migration was applied independently of the model switch pipeline.

**What went well:**  
WAL inspector correctly detected the schema mismatch and stopped replay rather than silently dropping records.

**Resolution:**  
- SOP-P04 updated: snapshot must be taken < 15 minutes before deployment, after all migrations are applied
- Schema migration and model switch deployment windows are now coordinated
- Added `themisdb-cli wal verify --schema-compat` step before replay

**Lessons:**
1. Snapshot timing is not just about time—it must reflect the latest schema state.
2. Schema migrations and model switches must be coordinated, not executed independently in overlapping windows.
3. Silent record loss is worse than an audible replay failure. Fail loudly.

---

## 7. Cross-Stage Lessons

### LL-006 — Policy Gate Evaluated Against Stale Policy Version

**Date:** 2026 Q3  
**Severity:** High (compliance gap)  
**Impact:** Production deployment proceeded under a policy that had been superseded by a new regulatory requirement  

**What happened:**  
A policy update reflecting a new GDPR sub-processor requirement was committed to `develop` but not yet deployed to the policy engine. The deployment pipeline evaluated the policy gate against the running policy engine version, which was 3 days behind the committed update.

**Root cause:**  
No synchronization check between committed policy version and running policy engine version before gate evaluation.

**What went well:**  
The quarterly audit identified the gap; no data was incorrectly processed.

**Resolution:**  
- Added policy version sync check to SOP-P03 pre-deployment checklist
- `themisdb-cli policy evaluate` now reports running version vs. latest committed version
- Policy engine updates now have a maximum deployment lag of 1 hour

**Lessons:**
1. Policy gates are only as current as the running policy engine. Never assume the engine reflects the latest committed policies.
2. Version drift between committed policy and running engine is a compliance risk, not just a technical debt.
3. Gate evaluations must assert engine version freshness, not just pass/fail result.

---

## 8. Common Failure Patterns

The following patterns appear repeatedly across pipeline stages and incidents. Treat these as pre-flight checks when reviewing any pipeline change.

### FP-1: Silent Validation Bypass Under Time Pressure

**Pattern:** An operator or developer skips or overrides a validation gate when under time pressure. The issue surfaces later at higher cost.

**Indicators:** "We'll fix it in the next deployment." / "The check is flaky, we can skip it this once."

**Prevention:**
- Hard gates with no human-only override (require Auditor co-approval)
- On-call pressure documented in incident reports, not reflected in validation bypasses
- Retro item required for any bypass event

---

### FP-2: Schema Drift Between Pipeline Stages

**Pattern:** A structural change (embedding dimension, manifest field, WAL schema) is made in one stage without updating dependent stages.

**Indicators:** Unexpected format errors in integration tests; field-not-found errors in artifact verification.

**Prevention:**
- Cross-stage compatibility checks as part of every schema change PR
- Version fields in all manifests; explicit compatibility matrix entries
- Integration smoke tests run across stage boundaries, not just within stages

---

### FP-3: Outdated Configuration as Root Cause

**Pattern:** A configuration file (canary thresholds, policy engine, compatibility matrix) is outdated relative to the current workload profile or compliance state.

**Indicators:** Thresholds that "have always been these values"; policy versions not matching commit history.

**Prevention:**
- Configuration files treated as code: reviewed quarterly against current workload and compliance state
- Automated alerts for configuration age approaching review threshold
- Version metadata in all configuration files

---

### FP-4: Provenance Gap in Adapter Chain

**Pattern:** The connection between training data, training job, and adapter artifact is incomplete or missing, creating a compliance audit gap.

**Indicators:** Manifest missing `training_corpus` or `training_job_id`; corpus ID not in approved dataset registry.

**Prevention:**
- Schema-validated manifests in CI (hard fail on missing provenance fields)
- Data Owner approval issue linked in every adapter PR
- Quarterly audit checks provenance completeness for all production artifacts

---

### FP-5: Recovery Lag Due to Missing Pre-Switch Snapshot

**Pattern:** Rollback is delayed because the pre-switch snapshot was taken too early (before schema migrations or config changes), making it incompatible with current state.

**Indicators:** WAL replay fails at schema boundary; snapshot timestamp predates critical change.

**Prevention:**
- Snapshot taken < 15 minutes before deployment (per updated SOP-P04)
- Schema migration and model switch windows explicitly coordinated
- Snapshot compatibility verified before deployment proceeds

---

## 9. Improvement Backlog

Items identified through lessons learned that are not yet implemented. Review at quarterly governance meeting.

| ID | Source | Proposed Improvement | Owner | Priority | Target |
|----|--------|---------------------|-------|----------|--------|
| IMP-001 | LL-005 | Automated embedding dimension validation in corpus registry | Developer | Medium | Q4 2026 |
| IMP-002 | LL-002 | Adaptive timeout for integrity checks based on artifact size | Operator | High | Q4 2026 |
| IMP-003 | FP-3 | Configuration age alert (> 90 days without review) | Operator | Medium | Q4 2026 |
| IMP-004 | FP-2 | Cross-stage integration smoke test suite | Developer | High | Q4 2026 |
| IMP-005 | LL-006 | Policy engine version drift alert (> 1 h) | Operator | High | Q4 2026 |

---

**Document Classification:** Internal — Operational  
**Maintainer:** Pipeline team + Lead Auditor  
**Update trigger:** After each P0/P1 incident (mandatory) or quarterly governance review  
**Cross-reference:** Issue #5420 (Phase 7), `docs/reviews/PIPELINE_REVIEW_TEMPLATES.md`, `docs/operations/PIPELINE_E2E_SOPs.md`
