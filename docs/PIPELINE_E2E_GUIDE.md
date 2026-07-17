[docs](../docs/index.md) > PIPELINE_E2E_GUIDE  
**Version:** 1.0  
**Status:** stable  
**Issue:** #5420 (Phase 7 — SOP, Reviews & Documentation)  
**Last Updated:** 2026-07-16

# ThemisDB End-to-End Pipeline Guide

## RAG → Package → Build → Deploy → Recover

---

## TL;DR

This guide documents the complete ThemisDB AI/ML artifact pipeline: from retrieval-augmented data ingestion through adapter packaging, model build, production deployment, and recovery. It is the canonical live reference for developers, operators, auditors, and data owners working with this pipeline.

---

## Table of Contents

1. [Pipeline Architecture](#1-pipeline-architecture)
2. [Stage 1: RAG — Retrieval-Augmented Data Ingestion](#2-stage-1-rag--retrieval-augmented-data-ingestion)
3. [Stage 2: Package — AdaLoRA Adapter Packaging](#3-stage-2-package--adalora-adapter-packaging)
4. [Stage 3: Build — Validation & Artifact Build](#4-stage-3-build--validation--artifact-build)
5. [Stage 4: Deploy — Model Switch & Activation](#5-stage-4-deploy--model-switch--activation)
6. [Stage 5: Recover — Rollback & Recovery](#6-stage-5-recover--rollback--recovery)
7. [Policy & Governance Integration](#7-policy--governance-integration)
8. [Observability & Monitoring](#8-observability--monitoring)
9. [Audit Trail & Compliance](#9-audit-trail--compliance)
10. [Quick Reference](#10-quick-reference)

---

## 1. Pipeline Architecture

### 1.1 Overview Diagram

```
  ┌─────────────────────────────────────────────────────────────────────┐
  │                  ThemisDB AI/ML Artifact Pipeline                   │
  │                                                                     │
  │  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐        │
  │  │  Stage 1 │   │  Stage 2 │   │  Stage 3 │   │  Stage 4 │        │
  │  │   RAG    │──▶│ Package  │──▶│  Build   │──▶│  Deploy  │──▶ 📦  │
  │  │Retrieval │   │ AdaLoRA  │   │Validate  │   │  Switch  │  prod  │
  │  └──────────┘   └──────────┘   └──────────┘   └──────────┘        │
  │       │               │               │               │            │
  │       │               │               │               ▼            │
  │  ┌──────────────────────────────────────────┐   ┌──────────┐       │
  │  │           Policy Engine Gates            │   │  Stage 5 │       │
  │  │  (evaluated at each stage transition)   │   │ Recover  │       │
  │  └──────────────────────────────────────────┘   │Rollback  │       │
  │                                                  └──────────┘       │
  │                                                                     │
  │  ┌──────────────────────────────────────────────────────────────┐  │
  │  │                    Audit Trail (continuous)                  │  │
  │  └──────────────────────────────────────────────────────────────┘  │
  └─────────────────────────────────────────────────────────────────────┘
```

### 1.2 Stage Summary

| Stage | Input | Output | Gating |
|-------|-------|--------|--------|
| 1 RAG | Dataset request | Approved corpus + embeddings | Data Owner policy gate |
| 2 Package | Adapter weights + base model | Signed `.adapkg` manifest | Developer + Auditor review |
| 3 Build | Approved `.adapkg` manifest | Validated artifact bundle | Integrity check + test gate |
| 4 Deploy | Validated artifact bundle | Active production model | Operator + Auditor approval |
| 5 Recover | Incident trigger | Restored model + evidence | Automatic + Operator confirm |

### 1.3 Key Configuration Locations

| Config | Location |
|--------|---------|
| Retrieval config | `config/retrieval/` |
| AdaLoRA strategy | `config/lora/adalora_optimization_strategy.yaml` |
| Compatibility matrix | `config/lora/` |
| Approved datasets | `config/data/approved_datasets.yaml` |
| Policy definitions | `src/governance/` |
| Artifact manifests | `artifacts/` |
| Audit evidence | `audit/pipeline-evidence/` |

---

## 2. Stage 1: RAG — Retrieval-Augmented Data Ingestion

### 2.1 Purpose

The RAG stage acquires, validates, and makes available training or retrieval corpora for use in adapter development and inference. Retrieval quality directly determines adapter task performance.

### 2.2 Data Flow

```
Data Source → [Policy Gate] → Data Registry → Retrieval Config → Embedding Index
                                                                        │
                                                                 Stage 2 (Package)
```

### 2.3 Key Components

| Component | Location | Purpose |
|-----------|---------|---------|
| ANN Front-Door | `src/retrieval/` | Top-K retrieval entry point |
| Embedding index | `artifacts/indexes/` | Pre-built vector index |
| Data registry | `config/data/` | Approved corpus metadata |
| Policy engine | `src/governance/policy_engine.cpp` | Data usage policy evaluation |

### 2.4 Retrieval Configuration

```yaml
# config/retrieval/<corpus-name>.yaml
corpus:
  id: "<corpus-id>"
  version: "<version>"
  embedding_model: "<model-ref>"
  embedding_dimension: 1024
  approved_by: "<data-owner-issue-ref>"

retrieval:
  top_k: 16
  similarity_metric: cosine
  min_score: 0.72
  index: artifacts/indexes/<corpus-id>-<version>.index
```

### 2.5 Validation Steps

```bash
# Validate corpus registration
themisdb-cli validate corpus --id <corpus-id> --version <version>

# Check embedding dimension alignment with active adapter
themisdb-cli compat check --corpus <corpus-id> --adapter <adapter-name>

# Run retrieval smoke test
cmake --build --preset linux-release --target test_retrieval_smoke
```

### 2.6 Policy Gate

Every new corpus must pass the Data Owner policy gate (SOP-P07) before use. The gate evaluates:
- PII presence and scrubbing completeness
- License compatibility with intended use
- Retention policy compliance
- GDPR / CCPA / HIPAA applicability

**Gate command:** `themisdb-cli policy evaluate --dataset <corpus-id>`

### 2.7 Related Documentation

- `docs/EPIC1_ANN_FRONTDOOR.md` — ANN retrieval architecture
- `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md` — Distributed retrieval
- SOP-P01 in `docs/operations/PIPELINE_E2E_SOPs.md`

---

## 3. Stage 2: Package — AdaLoRA Adapter Packaging

### 3.1 Purpose

The package stage creates a verified, signed adapter package (`.adapkg`) that bundles adapter weights with provenance metadata required for downstream build and audit workflows.

### 3.2 Manifest Schema

```json
{
  "schema_version": "1.0",
  "adapter_name": "<name>",
  "base_model": "<model-ref>",
  "rank": 16,
  "quantization": "int8",
  "training_corpus": "<corpus-id>@<version>",
  "training_job_id": "<job-id>",
  "created_at": "YYYY-MM-DDTHH:MM:SSZ",
  "hash": "<sha256>",
  "signature": "<ed25519-sig>",
  "compatibility": {
    "embedding_dimension": 1024,
    "min_base_model_version": "2.1.0"
  }
}
```

### 3.3 Packaging Workflow

```bash
# 1. Initialize manifest from training job
themisdb-cli adapter init \
  --job-id <training-job-id> \
  --output adapters/<adapter-name>.adapkg

# 2. Validate manifest completeness
themisdb-cli validate adapter --manifest adapters/<adapter-name>.adapkg

# 3. Compatibility check against target base model
themisdb-cli compat check \
  --adapter adapters/<adapter-name>.adapkg \
  --base <base-model-ref>

# 4. Sign manifest (requires Auditor key)
themisdb-cli adapter sign \
  --manifest adapters/<adapter-name>.adapkg \
  --key-id auditor-key

# 5. Register in compatibility matrix
themisdb-cli compat register \
  --adapter adapters/<adapter-name>.adapkg \
  --config config/lora/
```

### 3.4 Compatibility Matrix

The compatibility matrix (`config/lora/adalora_optimization_strategy.yaml`) records which adapter versions are compatible with which base model versions and embedding indices:

```yaml
adapters:
  <adapter-name>:
    compatible_base_models:
      - "<model-ref>@>=2.1.0"
    embedding_dimension: 1024
    quantization_support: ["fp16", "int8", "int4"]
    min_embedding_score: 0.72
```

### 3.5 Error Conditions

| Error | Cause | Resolution |
|-------|-------|-----------|
| `PROVENANCE_INCOMPLETE` | Missing required manifest field | Add field, re-validate |
| `COMPAT_MISMATCH` | Adapter rank incompatible with base model | Check embedding dimension, update compatibility entry |
| `SIGNATURE_INVALID` | Manifest tampered or key mismatch | Re-sign with correct Auditor key |
| `CORPUS_NOT_APPROVED` | Referenced corpus not in approved registry | Obtain Data Owner approval (SOP-P07) |

### 3.6 Related Documentation

- `docs/EPIC1_LORA_ARTIFACTS.md` — LoRA artifact design
- `docs/adr/adr-e1-004-lora-package-distinction.md` — Package/runtime distinction ADR
- `include/training/adalora_tt_bridge.h` — AdaLoRA TensorTrain bridge API
- SOP-P02 in `docs/operations/PIPELINE_E2E_SOPs.md`

---

## 4. Stage 3: Build — Validation & Artifact Build

### 4.1 Purpose

The build stage transforms an approved adapter package into a production-ready, integrity-checked artifact bundle that can be safely deployed to any supported environment.

### 4.2 Build Pipeline

```
.adapkg manifest
      │
      ▼
  Validate manifest
      │
      ▼
  Load adapter weights
      │
      ▼
  Run build (quantize, optimize, sign)
      │
      ▼
  Integration tests
      │
      ▼
  Performance regression check
      │
      ▼
  Artifact bundle (signed)
      │
      ▼
  Stage 4 (Deploy)
```

### 4.3 Build Commands

```bash
# Trigger build for adapter
themisdb-cli build trigger \
  --adapter adapters/<adapter-name>.adapkg \
  --env staging \
  --output artifacts/staging/

# Monitor build progress
themisdb-cli job status --job <job-id> --watch

# Verify artifact on completion
themisdb-cli artifact verify \
  --id <artifact-id> \
  --strict
```

### 4.4 Integration Test Suite

Run integration tests against the build artifact before promotion:

```bash
ctest --preset linux-release \
  --label integration \
  -j1 \
  --timeout 300 \
  --output-on-failure
```

Key test targets:

| Test | Purpose |
|------|---------|
| `test_adapter_load` | Verify adapter loads without error |
| `test_adapter_inference` | Verify output quality against reference |
| `test_model_compat` | Verify base model compatibility |
| `test_retrieval_pipeline` | End-to-end retrieval + adapter inference |

### 4.5 Promotion Gate

Before promoting to staging, verify:

```bash
# Integrity check
themisdb-cli artifact verify --id <artifact-id>

# Promote to staging
themisdb-cli artifact promote \
  --id <artifact-id> \
  --target staging \
  --sign
```

### 4.6 Related Documentation

- `docs/EPIC2_ARTIFACT_LIFECYCLE.md` — Artifact lifecycle design
- `docs/production/RUNBOOKS.md#model-checkpoint-management`
- SOP-P03 in `docs/operations/PIPELINE_E2E_SOPs.md`

---

## 5. Stage 4: Deploy — Model Switch & Activation

### 5.1 Purpose

The deploy stage performs a controlled model switch in production using canary deployment to limit blast radius. The previous model reference is snapshotted before the switch to enable rapid rollback.

### 5.2 Deployment Sequence

```bash
# 1. Open maintenance window
themisctl maintenance start --window 30m

# 2. Pre-switch snapshot
themisdb-cli snapshot create \
  --tag pre-switch-$(date +%Y%m%d-%H%M) \
  --include-schema

# 3. Canary deployment (10%)
themisdb-cli model switch \
  --target <artifact-id> \
  --strategy canary \
  --canary-pct 10

# 4. Monitor canary metrics (5 min)
themisdb-cli metrics watch \
  --duration 5m \
  --alert-on "error_rate>0.005,latency_p99_increase>0.15,throughput_decrease>0.2"

# 5. Promote to full traffic
themisdb-cli model switch \
  --target <artifact-id> \
  --strategy full

# 6. Post-switch health check
themisctl health --deep

# 7. Close maintenance window
themisctl maintenance end
```

### 5.3 Model Switch Architecture

```
         Traffic
            │
     ┌──────┴────────┐
     │  Load Balancer │
     └──────┬────────┘
            │ (canary split)
     ┌──────┴─────────────────┐
     │                        │
     ▼ (90%)                  ▼ (10%)
 Current Model           New Model (canary)
     │                        │
     └──────────┬─────────────┘
                │
         Metrics & Alert
```

The model switch is managed by the model switch workflow (`src/retrieval/include/model_switch.h`), which:
- Validates compatibility before switch initiation
- Manages traffic routing during canary phase
- Provides abort capability with automatic rollback trigger

### 5.4 Abort & Rollback Trigger

If canary metrics exceed thresholds:

```bash
# Abort canary switch
themisdb-cli model switch --abort

# Verify abort complete
themisdb-cli model status

# Proceed to recovery if needed (Stage 5)
```

Automatic abort fires when thresholds defined in `config/deployment/canary_thresholds.yaml` are exceeded for 3 consecutive check intervals.

### 5.5 Related Documentation

- `docs/EPIC1_MODEL_SWITCH.md` — Model switch workflow design
- `docs/production/RUNBOOKS.md#lora-adapter-deployment`
- SOP-P04 in `docs/operations/PIPELINE_E2E_SOPs.md`

---

## 6. Stage 5: Recover — Rollback & Recovery

### 6.1 Purpose

The recover stage restores the system to a known-good state after a deployment failure, data-loss event, or incident. It leverages pre-switch snapshots and WAL replay for minimal data loss.

### 6.2 Recovery Decision Tree

```
Incident Detected
       │
       ▼
Is pre-switch snapshot available?
       │
    Yes│                    No│
       ▼                       ▼
Restore snapshot          Locate latest backup
       │                       │
       ▼                       ▼
Verify health             Restore from backup
       │                       │
    Healthy?               WAL Replay required?
       │                       │
    Yes│No                  Yes│No
       │  └─▶WAL replay         │  └─▶Done
       │         │              │
       ▼         ▼              ▼
      Done    Verify        Done
```

### 6.3 Snapshot Restore

```bash
# List available snapshots
themisdb-cli snapshot list --limit 5

# Restore from pre-switch snapshot
themisdb-cli model restore \
  --snapshot pre-switch-<date>

# Verify restore
themisctl health --deep
themisdb-cli validate records --collection <primary-collection>
```

### 6.4 WAL Replay (Targeted Recovery)

For partial data recovery within a time window:

```bash
# Inspect WAL for recovery window
themisdb-cli wal inspect \
  --from <rollback-timestamp> \
  --verify-schema-compat

# Replay WAL to recovery point
themisdb-cli wal replay \
  --from <last-consistent> \
  --to <target-time> \
  --dry-run  # inspect impact first

# Apply replay
themisdb-cli wal replay \
  --from <last-consistent> \
  --to <target-time>
```

### 6.5 Disaster Recovery

For full system recovery (hardware failure, data-center incident):

Follow `docs/operations/disaster-recovery/DR_CHECKLISTS.md` and trigger the full DR drill procedure from `docs/operations/disaster-recovery/DR_TESTING.md`.

RTO target: ≤ 1 hour  
RPO target: ≤ 15 minutes

### 6.6 Post-Recovery Actions

After recovery is confirmed:

1. Collect diagnostics: `themisdb-cli diagnostics collect --since 2h --output /tmp/recovery-diag.tar.gz`
2. File incident report using REVIEW-T04 (`docs/reviews/PIPELINE_REVIEW_TEMPLATES.md`)
3. Update lessons learned in `docs/PIPELINE_LESSONS_LEARNED.md`
4. Notify Auditor for evidence collection (SOP-P06)

### 6.7 Related Documentation

- `docs/production/RUNBOOKS/RESTORE_RUNBOOK.md`
- `docs/production/RUNBOOKS/FAILOVER_RUNBOOK.md`
- `docs/operations/disaster-recovery/DR_CHECKLISTS.md`
- `docs/EPIC3_RECOVERY_STRATEGY.md`
- SOP-P05 in `docs/operations/PIPELINE_E2E_SOPs.md`

---

## 7. Policy & Governance Integration

### 7.1 Policy Gates

The policy engine evaluates gates at each stage transition:

| Transition | Gate Command | Failure Action |
|-----------|-------------|----------------|
| Data → RAG | `policy evaluate --dataset` | Block corpus use |
| RAG → Package | `policy evaluate --stage package` | Block adapter creation |
| Package → Build | `policy evaluate --stage build` | Block build trigger |
| Build → Deploy | `policy evaluate --stage deploy` | Block deployment |
| Recover | `policy evaluate --stage recover` | Log only (non-blocking) |

### 7.2 Policy Engine Version Synchronization

**Critical:** Always verify the running policy engine matches the latest committed policy version before gate evaluation.

```bash
# Check policy engine version alignment
themisdb-cli policy version check \
  --expected $(git log -1 --format="%H" -- src/governance/) \
  --running
```

If versions diverge by > 1 hour, **stop the pipeline** and update the policy engine before proceeding.

### 7.3 Compliance Framework Mapping

| Framework | Pipeline Stage | Key Control |
|-----------|---------------|-------------|
| GDPR | RAG (data ingestion) | PII scrubbing gate |
| ISO 27001 | Build, Deploy | Integrity and change management |
| SOC 2 | All stages | Audit trail completeness |
| HIPAA | RAG (if applicable) | PHI handling controls |

### 7.4 Related Documentation

- `src/governance/README.md`
- `docs/de/compliance/`
- `docs/governance/`

---

## 8. Observability & Monitoring

### 8.1 Key Metrics

| Metric | Prometheus Name | Alert Threshold |
|--------|----------------|----------------|
| Model inference latency p99 | `themis_model_latency_p99` | > 500 ms |
| Retrieval recall | `themis_retrieval_recall_at_10` | < 0.85 |
| Build artifact size | `themis_artifact_size_bytes` | > 10 GB |
| Policy gate failures | `themis_policy_gate_failures_total` | Any |
| Canary error rate delta | `themis_canary_error_rate_delta` | > 0.005 |
| Pipeline stage duration | `themis_pipeline_stage_duration_seconds` | > 2× baseline |

### 8.2 Dashboard Links

- Pipeline overview: `grafana/dashboards/pipeline-e2e-overview.json`
- Model performance: `grafana/dashboards/model-performance.json`
- Policy gates: `grafana/dashboards/policy-engine.json`

### 8.3 Alert Runbook

When a pipeline alert fires:

1. Check the relevant stage in this guide
2. Run the stage-specific diagnostic command
3. Assess severity (P0–P3) per SOP-P08
4. Follow the appropriate SOP from `docs/operations/PIPELINE_E2E_SOPs.md`

---

## 9. Audit Trail & Compliance

### 9.1 What is Automatically Audited

The following events are recorded in the audit trail automatically:

| Event | Audit Record |
|-------|-------------|
| Data policy gate evaluation | Dataset ID, policy version, outcome, timestamp |
| Adapter manifest signing | Adapter ID, signer key ID, hash, timestamp |
| Build trigger | Job ID, artifact ID, trigger user, timestamp |
| Artifact promotion | Artifact ID, target env, approver, timestamp |
| Model switch | Target artifact, strategy, canary metrics, outcome |
| Rollback / recovery | Trigger, snapshot used, WAL range, outcome |

### 9.2 Audit Trail Export

```bash
# Export pipeline audit trail for a run
themisdb-cli audit export \
  --pipeline <run-id> \
  --format json \
  --output audit/pipeline-evidence/<run-id>/trail.json

# Verify provenance chain
themisdb-cli audit verify-provenance \
  --adapter <adapter-id> \
  --output audit/pipeline-evidence/<run-id>/provenance.json
```

### 9.3 Evidence Storage Structure

```
audit/
└── pipeline-evidence/
    └── <run-id>/
        ├── trail.json             # Complete audit trail
        ├── provenance.json        # Adapter provenance chain
        ├── artifact-verify.json   # Integrity check results
        ├── policy-decisions.json  # Policy gate decisions
        └── review-t05.md          # Completed REVIEW-T05 template
```

### 9.4 Related Documentation

- `docs/audit-framework/AUDIT_RUNBOOK.md`
- `audit/docs/audit-reports/`
- SOP-P06 in `docs/operations/PIPELINE_E2E_SOPs.md`

---

## 10. Quick Reference

### Role-Specific Entry Points

| Role | First Document | Key SOP |
|------|---------------|---------|
| Developer | This guide §2 (RAG) + §3 (Package) | SOP-P01, SOP-P02 |
| Operator | This guide §4 (Deploy) + §5 (Recover) | SOP-P03, SOP-P04, SOP-P05 |
| Auditor | This guide §9 (Audit Trail) | SOP-P06 |
| Data Owner | This guide §2.6 (Policy Gate) | SOP-P07 |

### Emergency Contacts

| Situation | Action |
|-----------|--------|
| Production outage | Declare P0, follow SOP-P08, then SOP-P05 |
| Data loss suspected | Immediately declare P0, stop pipeline, engage Auditor |
| Security breach | Follow `SECURITY.md` + SOP-P08 |
| Policy gate blocked | Contact Data Owner for SOP-P07 |

### CLI Quick Reference

```bash
# Health check
themisctl health --deep

# Pipeline status
themisdb-cli pipeline status --all

# Current active model
themisdb-cli model status

# Last 5 audit events
themisdb-cli audit tail --limit 5

# Policy engine version check
themisdb-cli policy version check
```

---

## Document Map

```
docs/PIPELINE_E2E_GUIDE.md           ← you are here (live pipeline reference)
docs/operations/PIPELINE_E2E_SOPs.md ← role-based SOPs (step-by-step procedures)
docs/reviews/PIPELINE_REVIEW_TEMPLATES.md ← review templates for all pipeline gates
docs/PIPELINE_LESSONS_LEARNED.md     ← lessons learned + common failure patterns
```

**Related Architecture:**
- `docs/EPIC1_ANN_FRONTDOOR.md` — Retrieval
- `docs/EPIC1_LORA_ARTIFACTS.md` — Adapter packaging
- `docs/EPIC1_MODEL_SWITCH.md` — Model switch
- `docs/EPIC3_RECOVERY_STRATEGY.md` — Recovery strategy
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md` — Artifact lifecycle

---

**Document Classification:** Internal — Developer + Operator  
**Review Cycle:** After each major pipeline change; quarterly governance review  
**Maintained by:** Pipeline team (Developer + Operator leads)  
**Approved by:** Project Lead  
**Cross-reference:** Issue #5420 (Phase 7), `docs/operations/PIPELINE_E2E_SOPs.md`, `docs/reviews/PIPELINE_REVIEW_TEMPLATES.md`, `docs/PIPELINE_LESSONS_LEARNED.md`
