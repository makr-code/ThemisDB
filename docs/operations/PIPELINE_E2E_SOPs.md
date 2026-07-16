[docs](../../docs/index.md) > [operations](../operations/README.md) > PIPELINE_E2E_SOPs  
**Version:** 1.0  
**Status:** stable  
**Issue:** #5420 (Phase 7 — SOP, Reviews & Documentation)  
**Last Updated:** 2026-07-16

# ThemisDB End-to-End Pipeline — Standard Operating Procedures

## TL;DR

This document defines role-specific SOPs for every stage of the ThemisDB End-to-End AI/ML Artifact Pipeline:  
**RAG → Package → Build → Deploy → Recover**  
Roles covered: Developer, Operator, Auditor, Data Owner.

---

## Table of Contents

1. [Pipeline Overview](#1-pipeline-overview)
2. [Roles and Responsibilities](#2-roles-and-responsibilities)
3. [SOP-P01 — Developer: RAG Pipeline Integration](#3-sop-p01--developer-rag-pipeline-integration)
4. [SOP-P02 — Developer: AdaLoRA Package Authoring](#4-sop-p02--developer-adalora-package-authoring)
5. [SOP-P03 — Operator: Model Build & Promotion](#5-sop-p03--operator-model-build--promotion)
6. [SOP-P04 — Operator: Deployment & Model Switch](#6-sop-p04--operator-deployment--model-switch)
7. [SOP-P05 — Operator: Recovery & Rollback](#7-sop-p05--operator-recovery--rollback)
8. [SOP-P06 — Auditor: Pipeline Audit & Evidence Collection](#8-sop-p06--auditor-pipeline-audit--evidence-collection)
9. [SOP-P07 — Data Owner: Data Ingestion & Policy Gate](#9-sop-p07--data-owner-data-ingestion--policy-gate)
10. [SOP-P08 — All Roles: Incident Escalation](#10-sop-p08--all-roles-incident-escalation)
11. [Governance Alignment](#11-governance-alignment)

---

## 1. Pipeline Overview

```
┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│   RAG    │──▶│ Package  │──▶│  Build   │──▶│  Deploy  │──▶│ Recover  │
│ Retrieval│   │ AdaLoRA  │   │ Validate │   │  Switch  │   │ Rollback │
└──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘
     │               │               │               │               │
 Data Owner      Developer       Operator        Operator        Operator
                                                              + Auditor
```

**Stage descriptions:**

| Stage | Purpose | Key Artifact | Owner |
|-------|---------|--------------|-------|
| RAG | Context retrieval for training/inference | Retrieved corpus | Data Owner + Dev |
| Package | AdaLoRA adapter packaging with provenance | `.adapkg` manifest | Developer |
| Build | Validation, integrity check, build pipeline | Signed model bundle | Operator |
| Deploy | Model switch and activation in production | Active model ref | Operator |
| Recover | Rollback, WAL replay, manifest restoration | Previous model ref | Operator + Auditor |

---

## 2. Roles and Responsibilities

| Role | Primary Responsibilities |
|------|------------------------|
| **Developer** | Implements RAG integrations, authors adapter packages, writes compatibility metadata |
| **Operator** | Runs builds, manages deployment gates, executes recovery procedures |
| **Auditor** | Verifies integrity evidence, signs off on compliance gates, collects audit trails |
| **Data Owner** | Approves data ingestion, enforces data-policy gates, validates lineage |

**Escalation path:**  
Data Owner / Developer → Operator → Auditor → Project Lead

---

## 3. SOP-P01 — Developer: RAG Pipeline Integration

**Trigger:** New retrieval source or corpus version is ready for integration.  
**Owner:** Developer  
**SLA:** < 5 business days from corpus availability to integration merge.

### Pre-conditions

- [ ] Corpus version is tagged in the data registry
- [ ] Data Owner approval obtained (see [SOP-P07](#9-sop-p07--data-owner-data-ingestion--policy-gate))
- [ ] Target retrieval configuration reviewed against `config/retrieval/`

### Steps

| # | Action | Who | Check |
|---|--------|-----|-------|
| 1 | Create feature branch from `develop` | Dev | `[ ]` |
| 2 | Update retrieval configuration (`config/retrieval/<corpus>.yaml`) | Dev | `[ ]` |
| 3 | Run local retrieval smoke test: `cmake --build --preset linux-release --target test_retrieval_smoke` | Dev | `[ ]` |
| 4 | Verify retrieved embedding dimensions match active adapter rank | Dev | `[ ]` |
| 5 | Commit with reference to corpus tag and Data Owner approval issue | Dev | `[ ]` |
| 6 | Open PR → `develop`; request Operator review for config changes | Dev | `[ ]` |
| 7 | Ensure CI passes `edition-develop-ci` | CI | `[ ]` |
| 8 | Merge on ≥ 1 Operator + ≥ 1 Developer approval | Dev/RM | `[ ]` |

### Error Handling

- Embedding dimension mismatch → block merge, raise compatibility issue, consult [SOP-P02](#4-sop-p02--developer-adalora-package-authoring)
- Retrieval latency > 200 ms p99 → performance review required before merge
- Data policy gate blocked → escalate to Data Owner

**Related:** `docs/EPIC1_ANN_FRONTDOOR.md` · `config/retrieval/` · [SOP-P07](#9-sop-p07--data-owner-data-ingestion--policy-gate)

---

## 4. SOP-P02 — Developer: AdaLoRA Package Authoring

**Trigger:** New adapter weights or base-model update requires a new package.  
**Owner:** Developer  
**SLA:** Package review within 3 business days.

### Pre-conditions

- [ ] Base model version confirmed and registered
- [ ] Adapter rank, quantization settings, and target task documented
- [ ] Compatibility matrix entry prepared (`config/lora/adalora_optimization_strategy.yaml`)

### Steps

| # | Action | Who | Check |
|---|--------|-----|-------|
| 1 | Author `<adapter-name>.adapkg` manifest (see schema `schemas/adapkg.schema.json`) | Dev | `[ ]` |
| 2 | Populate provenance fields: `base_model`, `training_corpus`, `training_job_id`, `rank` | Dev | `[ ]` |
| 3 | Run validator: `themisdb-cli validate adapter --manifest <adapter>.adapkg` | Dev | `[ ]` |
| 4 | Run compatibility check: `themisdb-cli compat check --adapter <adapter>.adapkg --base <model>` | Dev | `[ ]` |
| 5 | Add adapter to compatibility registry in `config/lora/` | Dev | `[ ]` |
| 6 | Open PR with completed `ADAPTER_REVIEW_TEMPLATE` (see `docs/reviews/PIPELINE_REVIEW_TEMPLATES.md`) | Dev | `[ ]` |
| 7 | Auditor verifies provenance fields and signs off | Auditor | `[ ]` |
| 8 | Merge on ≥ 1 Operator + ≥ 1 Auditor approval | Dev/RM | `[ ]` |

### Error Handling

- Validation failure on manifest → fix schema violations; re-run validator
- Compatibility check fails → do not promote; create compatibility issue; consult `docs/EPIC1_MODEL_SWITCH.md`
- Provenance fields incomplete → Auditor blocks merge until resolved

**Related:** `docs/EPIC1_LORA_ARTIFACTS.md` · `docs/adr/adr-e1-004-lora-package-distinction.md` · `config/lora/`

---

## 5. SOP-P03 — Operator: Model Build & Promotion

**Trigger:** Adapter package approved and ready for build pipeline.  
**Owner:** Operator  
**SLA:** Build completes within 4 hours of trigger; promotion decision within 1 business day.

### Pre-conditions

- [ ] Adapter package merged to `develop` (SOP-P02 complete)
- [ ] Build environment has sufficient GPU resources
- [ ] Previous build artifacts archived or confirmed clean

### Steps

| # | Action | Who | Check |
|---|--------|-----|-------|
| 1 | Trigger build pipeline: `themisdb-cli build trigger --adapter <adapter>.adapkg --env staging` | Operator | `[ ]` |
| 2 | Monitor build job: `themisdb-cli job status --job <job-id>` | Operator | `[ ]` |
| 3 | Verify build integrity: `themisdb-cli artifact verify --id <artifact-id>` | Operator | `[ ]` |
| 4 | Run integration tests against staging: `ctest --preset linux-release --label integration -j1` | Operator | `[ ]` |
| 5 | Review build report and attach to promotion PR | Operator | `[ ]` |
| 6 | Promote artifact to `artifacts/staging/` with signed manifest | Operator | `[ ]` |
| 7 | Notify Auditor for build evidence review | Operator | `[ ]` |

### Promotion Gate Checklist

- [ ] Build completed without warnings on integrity checks
- [ ] Integration tests pass (0 failures)
- [ ] Artifact hash matches manifest
- [ ] Auditor evidence sign-off received
- [ ] Performance regression check passed (< 5% latency increase)

### Error Handling

- Build timeout (> 4 h) → abort, investigate resource contention, re-trigger
- Integrity mismatch → block promotion, raise security incident
- Integration test failure → root cause in build log; consult developer before re-trigger

**Related:** `docs/production/RUNBOOKS.md#model-checkpoint-management` · `artifacts/`

---

## 6. SOP-P04 — Operator: Deployment & Model Switch

**Trigger:** Staged artifact passes all gates and deployment is authorized.  
**Owner:** Operator  
**SLA:** Deployment window ≤ 30 minutes; switch visible in metrics within 5 minutes.

### Pre-conditions

- [ ] Staging promotion complete (SOP-P03 complete)
- [ ] Deployment window communicated to stakeholders
- [ ] Rollback target identified (previous model reference)

### Steps

| # | Action | Who | Check |
|---|--------|-----|-------|
| 1 | Open maintenance window: `themisctl maintenance start --window 30m` | Operator | `[ ]` |
| 2 | Snapshot current model reference: `themisdb-cli snapshot create --tag pre-switch-$(date +%Y%m%d)` | Operator | `[ ]` |
| 3 | Initiate model switch: `themisdb-cli model switch --target <artifact-id> --strategy canary --canary-pct 10` | Operator | `[ ]` |
| 4 | Monitor canary metrics for 5 minutes: error rate, latency p99, throughput | Operator | `[ ]` |
| 5 | If canary healthy, promote to 100%: `themisdb-cli model switch --target <artifact-id> --strategy full` | Operator | `[ ]` |
| 6 | Verify health post-switch: `themisctl health --deep` | Operator | `[ ]` |
| 7 | Close maintenance window: `themisctl maintenance end` | Operator | `[ ]` |
| 8 | Audit log entry: record switch event in ops journal | Operator | `[ ]` |

### Canary Thresholds

| Metric | Abort Threshold |
|--------|----------------|
| Error rate | > 0.5% (vs. baseline) |
| Latency p99 | > 15% increase |
| Throughput | < 80% of baseline |

### Error Handling

- Canary threshold exceeded → abort switch: `themisdb-cli model switch --abort`; proceed to [SOP-P05](#7-sop-p05--operator-recovery--rollback)
- Health check fails post-full-promotion → immediate rollback per [SOP-P05](#7-sop-p05--operator-recovery--rollback)

**Related:** `docs/EPIC1_MODEL_SWITCH.md` · `docs/production/RUNBOOKS.md#lora-adapter-deployment`

---

## 7. SOP-P05 — Operator: Recovery & Rollback

**Trigger:** Deployment failure, canary abort, or production incident.  
**Owner:** Operator  
**SLA:** Rollback initiated within 15 minutes of incident detection; recovery confirmed within 1 hour.

### Pre-conditions

- [ ] Pre-switch snapshot exists (created in SOP-P04, step 2)
- [ ] Incident declared and severity assigned

### Steps

| # | Action | Who | Check |
|---|--------|-----|-------|
| 1 | Declare incident: `themisctl incident create --severity P1 --title "Model switch rollback"` | Operator | `[ ]` |
| 2 | Restore previous model reference: `themisdb-cli model restore --snapshot pre-switch-<date>` | Operator | `[ ]` |
| 3 | Verify rollback health: `themisctl health --deep` | Operator | `[ ]` |
| 4 | Confirm metrics return to baseline (observe for 5 min) | Operator | `[ ]` |
| 5 | Collect diagnostics: `themisdb-cli diagnostics collect --since 1h --output /tmp/incident-diag-$(date +%Y%m%d).tar.gz` | Operator | `[ ]` |
| 6 | Notify Auditor and Developer of rollback event | Operator | `[ ]` |
| 7 | File post-incident report within 24 h | Operator | `[ ]` |

### WAL Replay (Data-Loss Scenario)

If rollback reveals data loss due to partial WAL writes:

```bash
# Identify last consistent WAL position
themisdb-cli wal inspect --from <rollback-timestamp>

# Replay WAL to recovery point
themisdb-cli wal replay --from <last-consistent> --to <target>

# Validate record counts post-replay
themisdb-cli validate records --collection <collection>
```

### Error Handling

- Snapshot not found → locate most recent backup: `themisdb-cli backup list --type full --limit 5`
- WAL replay fails → stop replay, raise severity to P0, escalate to Project Lead
- Health check still failing after rollback → escalate to P0, engage full incident response per `docs/operations/incident-response/INCIDENT_RESPONSE_PLAYBOOK.md`

**Related:** `docs/production/RUNBOOKS/RESTORE_RUNBOOK.md` · `docs/production/RUNBOOKS/FAILOVER_RUNBOOK.md` · `docs/operations/disaster-recovery/DR_CHECKLISTS.md`

---

## 8. SOP-P06 — Auditor: Pipeline Audit & Evidence Collection

**Trigger:** Release gate, compliance review cycle, or post-incident audit.  
**Owner:** Auditor  
**SLA:** Initial evidence collection within 1 business day; full audit report within 5 business days.

### Pre-conditions

- [ ] Pipeline run ID or time range provided
- [ ] Access to audit trail storage confirmed
- [ ] Compliance framework applicable (ISO 27001, GDPR, etc.) identified

### Steps

| # | Action | Who | Check |
|---|--------|-----|-------|
| 1 | Collect pipeline audit trail: `themisdb-cli audit export --pipeline <run-id> --format json` | Auditor | `[ ]` |
| 2 | Verify adapter provenance chain: `themisdb-cli audit verify-provenance --adapter <adapter-id>` | Auditor | `[ ]` |
| 3 | Check artifact integrity signatures: `themisdb-cli artifact verify --all --since <date>` | Auditor | `[ ]` |
| 4 | Review policy gate decisions: `themisdb-cli policy audit --pipeline <run-id>` | Auditor | `[ ]` |
| 5 | Cross-reference build logs with manifest claims | Auditor | `[ ]` |
| 6 | Populate `PIPELINE_AUDIT_EVIDENCE_TEMPLATE` (see `docs/reviews/PIPELINE_REVIEW_TEMPLATES.md`) | Auditor | `[ ]` |
| 7 | Sign off evidence package with cryptographic signature | Auditor | `[ ]` |
| 8 | Store in audit evidence repository: `audit/pipeline-evidence/<run-id>/` | Auditor | `[ ]` |

### Compliance Checkpoints

| Control | Check | Evidence Type |
|---------|-------|---------------|
| Data lineage | Corpus-to-adapter traceability complete | Lineage graph export |
| Integrity | All artifacts have valid hash + signature | `artifact verify` output |
| Separation of duties | Dev ≠ Approver for production promotion | PR approval log |
| Change management | All switches logged in ops journal | Audit trail export |
| Recovery | Rollback capability tested within 90 days | DR test report |

**Related:** `docs/audit-framework/AUDIT_RUNBOOK.md` · `audit/docs/audit-reports/` · `docs/governance/`

---

## 9. SOP-P07 — Data Owner: Data Ingestion & Policy Gate

**Trigger:** New training corpus or retrieval dataset proposed for pipeline use.  
**Owner:** Data Owner  
**SLA:** Policy gate decision within 3 business days.

### Pre-conditions

- [ ] Dataset origin, license, and PII classification documented
- [ ] Data retention policy applicable to dataset identified
- [ ] GDPR / CCPA / HIPAA applicability assessed

### Steps

| # | Action | Who | Check |
|---|--------|-----|-------|
| 1 | Review dataset proposal (origin, license, classification) | Data Owner | `[ ]` |
| 2 | Run automated policy check: `themisdb-cli policy evaluate --dataset <dataset-id>` | Data Owner | `[ ]` |
| 3 | Verify PII scrubbing / anonymization applied where required | Data Owner | `[ ]` |
| 4 | Confirm data retention schedule matches policy | Data Owner | `[ ]` |
| 5 | Issue approval or rejection in GitHub issue (reference dataset ID) | Data Owner | `[ ]` |
| 6 | If approved: add dataset to approved registry in `config/data/approved_datasets.yaml` | Data Owner | `[ ]` |
| 7 | Notify Developer to proceed with SOP-P01 | Data Owner | `[ ]` |

### Policy Gate Outcomes

| Outcome | Next Action |
|---------|-------------|
| **Approved** | Developer proceeds with SOP-P01 |
| **Approved with conditions** | Developer addresses conditions; Data Owner re-reviews |
| **Rejected** | Dataset blocked; escalate if disputed to Project Lead |

**Related:** `src/governance/README.md` · `docs/de/compliance/` · `docs/operations/access-management/`

---

## 10. SOP-P08 — All Roles: Incident Escalation

**Trigger:** Any pipeline stage encounters a blocking issue.

### Severity Levels

| Level | Definition | Response Time | Escalation |
|-------|-----------|---------------|-----------|
| P0 | Data loss, security breach, complete outage | Immediate | Project Lead + Auditor |
| P1 | Production degraded, rollback required | ≤ 15 min | Operator + Auditor |
| P2 | Significant pipeline failure, SLA at risk | ≤ 2 h | Operator |
| P3 | Non-blocking issue, workaround available | ≤ 1 day | Developer |

### Escalation Path

```
Developer / Data Owner
        │
        ▼ (P1/P0 or no resolution in SLA)
    Operator
        │
        ▼ (P0 or security event)
    Auditor + Project Lead
```

### Communication Template

```
INCIDENT REPORT — [Severity] [Stage]
Date/Time: YYYY-MM-DD HH:MM UTC
Pipeline Run ID: <id>
Stage: RAG | Package | Build | Deploy | Recover
Description: <1–2 sentences>
Current Status: <active / mitigated / resolved>
Impact: <user-visible impact>
Next Action: <owner + action + ETA>
```

**Related:** `docs/operations/incident-response/INCIDENT_RESPONSE_PLAYBOOK.md` · `docs/SOP.md#sop-08--incident-response`

---

## 11. Governance Alignment

These SOPs are subordinate to and consistent with:

- [SOP.md](../../SOP.md) — Top-level project SOPs
- [GOVERNANCE.md](../../GOVERNANCE.md) — Decision authority
- [DOCUMENTATION_GOVERNANCE.md](../../DOCUMENTATION_GOVERNANCE.md) — Documentation ownership
- [docs/audit-framework/AUDIT_RUNBOOK.md](../audit-framework/AUDIT_RUNBOOK.md) — Audit procedures
- [docs/operations/incident-response/INCIDENT_RESPONSE_PLAYBOOK.md](incident-response/INCIDENT_RESPONSE_PLAYBOOK.md) — Incident response

**Review Cycle:** Quarterly or after each major pipeline incident.  
**Owner:** Operations Team + Lead Auditor  
**Approved by:** Project Lead  

---

**Document Classification:** Internal — Operational  
**Cross-reference:** Issue #5420 (Phase 7), `docs/PIPELINE_E2E_GUIDE.md`, `docs/reviews/PIPELINE_REVIEW_TEMPLATES.md`, `docs/PIPELINE_LESSONS_LEARNED.md`
