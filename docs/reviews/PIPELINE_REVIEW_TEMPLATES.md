[docs](../../docs/index.md) > [reviews](../reviews/README.md) > PIPELINE_REVIEW_TEMPLATES  
**Version:** 1.0  
**Status:** stable  
**Issue:** #5420 (Phase 7 — SOP, Reviews & Documentation)  
**Last Updated:** 2026-07-16

# ThemisDB Pipeline Review Templates

## Purpose

This document provides standardized review templates for all pipeline governance checkpoints in the ThemisDB End-to-End AI/ML Artifact Pipeline (RAG → Package → Build → Deploy → Recover). Templates ensure consistent, auditable reviews across all roles.

---

## Table of Contents

1. [Template Index](#1-template-index)
2. [REVIEW-T01 — Adapter Package Review](#2-review-t01--adapter-package-review)
3. [REVIEW-T02 — Build Promotion Review](#3-review-t02--build-promotion-review)
4. [REVIEW-T03 — Deployment Approval Review](#4-review-t03--deployment-approval-review)
5. [REVIEW-T04 — Post-Incident Pipeline Review](#5-review-t04--post-incident-pipeline-review)
6. [REVIEW-T05 — Audit Evidence Review](#6-review-t05--audit-evidence-review)
7. [REVIEW-T06 — Data Policy Gate Review](#7-review-t06--data-policy-gate-review)
8. [REVIEW-T07 — Quarterly Governance Review](#8-review-t07--quarterly-governance-review)

---

## 1. Template Index

| Template | Stage | Triggered By | Required Approvers |
|----------|-------|-------------|-------------------|
| REVIEW-T01 | Package | Developer opens adapter PR | Operator + Auditor |
| REVIEW-T02 | Build | Build pipeline completes | Operator (2nd eyes) |
| REVIEW-T03 | Deploy | Staging gates pass | Operator + Auditor |
| REVIEW-T04 | Recover | Incident resolved | Operator + Auditor |
| REVIEW-T05 | Audit | Compliance cycle | Lead Auditor |
| REVIEW-T06 | Data | Dataset proposed | Data Owner |
| REVIEW-T07 | Governance | Quarterly cadence | Project Lead + all roles |

---

## 2. REVIEW-T01 — Adapter Package Review

**Purpose:** Verify completeness, integrity, and compliance of a new AdaLoRA adapter package before merge.

```markdown
## Adapter Package Review — REVIEW-T01

**PR / Issue:** #<id>
**Adapter Name:** <adapter-name>
**Base Model:** <base-model-ref>
**Training Job ID:** <job-id>
**Reviewer:** @<github-handle>
**Date:** YYYY-MM-DD

---

### 1. Manifest Completeness

- [ ] `base_model` field present and matches registered model
- [ ] `rank` field matches training configuration
- [ ] `quantization` setting documented
- [ ] `training_corpus` references approved dataset (see `config/data/approved_datasets.yaml`)
- [ ] `training_job_id` traceable in job history
- [ ] `compatibility_matrix` entry added to `config/lora/`

### 2. Integrity Checks

- [ ] `themisdb-cli validate adapter` passed (attach output)
- [ ] `themisdb-cli compat check` passed (attach output)
- [ ] Artifact hash computed and recorded in manifest

### 3. Provenance & Data Lineage

- [ ] Data Owner approval confirmed (link to issue/comment)
- [ ] PII / sensitive data scrubbing verified
- [ ] License compatibility confirmed

### 4. Auditor Sign-off

- [ ] Provenance chain reviewed and accepted
- [ ] Manifest signature applied
- [ ] Evidence stored at `audit/pipeline-evidence/adapkg-<name>/`

### 5. Notes / Findings

<free-text: any concerns, deferred items, or conditions>

### Decision

- [ ] **Approved** — merge as-is
- [ ] **Approved with conditions** — list conditions above
- [ ] **Rejected** — see notes

**Approver Signatures:**
- Operator: @<handle> (YYYY-MM-DD)
- Auditor: @<handle> (YYYY-MM-DD)
```

---

## 3. REVIEW-T02 — Build Promotion Review

**Purpose:** Confirm build output integrity and readiness for staging deployment.

```markdown
## Build Promotion Review — REVIEW-T02

**Build Job ID:** <job-id>
**Adapter / Model:** <name>
**Target Environment:** staging
**Reviewer:** @<github-handle>
**Date:** YYYY-MM-DD

---

### 1. Build Outcome

- [ ] Build completed without errors
- [ ] No integrity warnings in build log
- [ ] Artifact size within expected range (± 10% vs. previous)

### 2. Test Results

- [ ] Integration tests passed (0 failures): attach `ctest` output
- [ ] Performance regression check passed (< 5% latency increase)
- [ ] GPU compatibility verified (if applicable)

### 3. Artifact Verification

- [ ] `themisdb-cli artifact verify` passed
- [ ] Artifact hash matches manifest claim
- [ ] Artifact promoted to `artifacts/staging/` with signed manifest

### 4. Build Report

Attach build report or link: <url or path>

### 5. Notes / Findings

<free-text>

### Decision

- [ ] **Promote to staging** — proceed to REVIEW-T03
- [ ] **Hold** — identified issue, see notes
- [ ] **Reject build** — re-trigger after fix

**Approver Signature:**
- Operator: @<handle> (YYYY-MM-DD)
```

---

## 4. REVIEW-T04 — Deployment Approval Review

**Purpose:** Formal authorization before production deployment / model switch.

```markdown
## Deployment Approval Review — REVIEW-T03

**Artifact ID:** <artifact-id>
**Target Environment:** production
**Deployment Window:** YYYY-MM-DD HH:MM – HH:MM UTC
**Rollback Snapshot:** pre-switch-<date>
**Reviewer:** @<github-handle>
**Date:** YYYY-MM-DD

---

### 1. Pre-Deployment Gates

- [ ] REVIEW-T02 approved (build promotion)
- [ ] Staging soak period met (minimum 24 h)
- [ ] Deployment window communicated to stakeholders
- [ ] On-call operator identified and confirmed
- [ ] Rollback snapshot confirmed accessible

### 2. Canary Plan

- Initial canary percentage: <10% / 20% / other>
- Observation window: <5 min / 10 min>
- Abort thresholds documented (see SOP-P04)

### 3. Compliance Gate

- [ ] Auditor sign-off on evidence package received
- [ ] Change management record created
- [ ] Policy engine gate passed: `themisdb-cli policy evaluate --stage deploy`

### 4. Stakeholder Notification

- [ ] Data Owner notified
- [ ] Developer team notified
- [ ] Status page / maintenance notice updated (if user-visible)

### 5. Notes / Conditions

<free-text>

### Decision

- [ ] **Approved for deployment** — proceed with SOP-P04
- [ ] **Deferred** — reason + new target date
- [ ] **Rejected** — see notes

**Approver Signatures:**
- Operator: @<handle> (YYYY-MM-DD)
- Auditor: @<handle> (YYYY-MM-DD)
```

---

## 5. REVIEW-T05 — Post-Incident Pipeline Review

**Purpose:** Structured retrospective after any P0 or P1 incident involving the pipeline.

```markdown
## Post-Incident Pipeline Review — REVIEW-T04

**Incident ID:** <id>
**Severity:** P0 / P1
**Pipeline Stage Affected:** RAG | Package | Build | Deploy | Recover
**Date of Incident:** YYYY-MM-DD
**Date of Review:** YYYY-MM-DD
**Review Lead:** @<handle>
**Attendees:** @<list>

---

### 1. Incident Timeline

| Time (UTC) | Event | Actor |
|------------|-------|-------|
| HH:MM | <event> | <role> |
| HH:MM | Incident detected | <role> |
| HH:MM | Rollback initiated | Operator |
| HH:MM | Recovery confirmed | Operator |

### 2. Root Cause Analysis

**Immediate cause:**
<1–2 sentences>

**Contributing factors:**
- <factor 1>
- <factor 2>

**Root cause:**
<technical root cause>

### 3. Impact Assessment

| Dimension | Impact |
|-----------|--------|
| User-visible downtime | <duration> |
| Data at risk | <yes/no, scope if yes> |
| SLO breach | <yes/no> |
| Compliance implication | <yes/no, describe if yes> |

### 4. What Went Well

- <item>
- <item>

### 5. What Could Be Improved

- <item> — Owner: @<handle> — Target: YYYY-MM-DD
- <item> — Owner: @<handle> — Target: YYYY-MM-DD

### 6. Action Items

| # | Action | Owner | Target Date | Status |
|---|--------|-------|-------------|--------|
| 1 | <action> | @<handle> | YYYY-MM-DD | `[ ]` |
| 2 | <action> | @<handle> | YYYY-MM-DD | `[ ]` |

### 7. Lessons Learned

Add findings to `docs/PIPELINE_LESSONS_LEARNED.md` — section matching the affected stage.

### Sign-off

- Operator: @<handle> (YYYY-MM-DD)
- Auditor: @<handle> (YYYY-MM-DD)
- Project Lead: @<handle> (YYYY-MM-DD)
```

---

## 6. REVIEW-T06 — Audit Evidence Review

**Purpose:** Formal acceptance of audit evidence package for a pipeline run.

```markdown
## Audit Evidence Review — REVIEW-T05

**Pipeline Run ID:** <run-id>
**Review Period:** YYYY-MM-DD to YYYY-MM-DD
**Lead Auditor:** @<handle>
**Date:** YYYY-MM-DD

---

### 1. Evidence Completeness

- [ ] Pipeline audit trail exported and stored
- [ ] Adapter provenance chain verified
- [ ] All artifact integrity signatures confirmed valid
- [ ] Policy gate decisions documented
- [ ] Build logs archived at `audit/pipeline-evidence/<run-id>/`

### 2. Compliance Controls

| Control | Status | Evidence Reference |
|---------|--------|--------------------|
| Data lineage | `[ ]` Pass / Fail | <link> |
| Integrity (hash + sig) | `[ ]` Pass / Fail | <link> |
| Separation of duties | `[ ]` Pass / Fail | <link> |
| Change management | `[ ]` Pass / Fail | <link> |
| Recovery capability | `[ ]` Pass / Fail | <link> |

### 3. Findings

| Finding ID | Severity | Description | Remediation |
|------------|----------|-------------|-------------|
| F-001 | High / Med / Low | <description> | <action + owner> |

### 4. Conclusion

- [ ] **Accepted** — evidence complete, no material findings
- [ ] **Accepted with findings** — findings logged, remediation tracked
- [ ] **Rejected** — material gaps; re-collect before compliance sign-off

**Lead Auditor Signature:** @<handle> (YYYY-MM-DD)
```

---

## 7. REVIEW-T07 — Data Policy Gate Review

**Purpose:** Data Owner review and approval / rejection of a dataset for pipeline use.

```markdown
## Data Policy Gate Review — REVIEW-T06

**Dataset ID:** <id>
**Dataset Origin:** <source>
**Proposed Use:** training | retrieval | evaluation
**Data Owner:** @<handle>
**Date:** YYYY-MM-DD

---

### 1. Dataset Classification

- [ ] PII presence assessed: None / Present (scrubbed) / Present (blocked)
- [ ] License: <license type>
- [ ] Regulatory scope: GDPR / CCPA / HIPAA / None / Other: <specify>
- [ ] Retention period: <duration>

### 2. Policy Check

- [ ] `themisdb-cli policy evaluate --dataset <id>` passed (attach output)
- [ ] PII scrubbing confirmed (if applicable)
- [ ] Data retention schedule aligns with policy

### 3. Lineage Documentation

- [ ] Origin documented in dataset registry
- [ ] Training/corpus chain traceable to approved sources
- [ ] No re-identification risk identified

### 4. Decision

- [ ] **Approved** — add to `config/data/approved_datasets.yaml`
- [ ] **Approved with conditions:** <list conditions>
- [ ] **Rejected:** <reason>

**Data Owner Signature:** @<handle> (YYYY-MM-DD)
```

---

## 8. REVIEW-T08 — Quarterly Governance Review

**Purpose:** Assess overall pipeline health, SOP compliance, and identify improvement areas.

```markdown
## Quarterly Governance Review — REVIEW-T07

**Quarter:** Q<n> <YYYY>
**Review Lead:** @<handle>
**Participants:** @<list>
**Date:** YYYY-MM-DD

---

### 1. Pipeline Health Metrics (Last Quarter)

| Metric | Target | Actual | Status |
|--------|--------|--------|--------|
| Deployment success rate | ≥ 98% | <n>% | `[ ]` |
| Mean time to rollback (MTTR) | ≤ 15 min | <n> min | `[ ]` |
| SLA breach count | 0 | <n> | `[ ]` |
| Audit evidence gaps | 0 | <n> | `[ ]` |
| P0/P1 incidents | 0 / ≤ 2 | <n> | `[ ]` |

### 2. SOP Compliance

| SOP | Last Executed | Compliant | Notes |
|-----|---------------|-----------|-------|
| SOP-P01 | YYYY-MM-DD | `[ ]` | |
| SOP-P02 | YYYY-MM-DD | `[ ]` | |
| SOP-P03 | YYYY-MM-DD | `[ ]` | |
| SOP-P04 | YYYY-MM-DD | `[ ]` | |
| SOP-P05 | YYYY-MM-DD | `[ ]` | |
| SOP-P06 | YYYY-MM-DD | `[ ]` | |
| SOP-P07 | YYYY-MM-DD | `[ ]` | |

### 3. Open Action Items from Previous Quarter

| # | Action | Owner | Status |
|---|--------|-------|--------|
| 1 | <item> | @<handle> | `[ ]` Pending / `[x]` Done |

### 4. SOP Changes Proposed This Quarter

- <proposed change + rationale>

### 5. Lessons Learned Updates

- New entries added to `docs/PIPELINE_LESSONS_LEARNED.md`: <count>

### 6. Approval for Next Quarter

- [ ] SOPs approved as-is for next quarter
- [ ] SOPs updated (changes merged to `develop`)

**Sign-off:**
- Project Lead: @<handle> (YYYY-MM-DD)
- Lead Operator: @<handle> (YYYY-MM-DD)
- Lead Auditor: @<handle> (YYYY-MM-DD)
- Data Owner representative: @<handle> (YYYY-MM-DD)
```

---

**Document Classification:** Internal — Governance  
**Review Cycle:** Quarterly  
**Cross-reference:** Issue #5420 (Phase 7), `docs/operations/PIPELINE_E2E_SOPs.md`, `docs/PIPELINE_LESSONS_LEARNED.md`, `docs/PIPELINE_E2E_GUIDE.md`
