# AI/ML Impact Assessment - Governance Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md -->

## Purpose

This document is the governance-owned impact assessment for AI/ML-relevant control paths implemented directly in `src/governance/`. It focuses on policy gating, training-export restrictions, bias/compliance reporting, data masking, lineage capture, and cross-border transfer controls.

## Scope

In scope:
- AI/ML training export governance in `model_governance.cpp`
- governance policy fallback and permission checks in `policy_engine.cpp`
- bias and compliance reporting in `compliance_reporter.cpp`
- masking and lineage controls in `data_masker.cpp` and `data_lineage.cpp`
- cross-border transfer decision logic in `cross_border_transfer.cpp`

Out of scope:
- full ML runtime behavior in `src/llm/`, `src/rag/`, `src/search/`, `src/analytics/`, or `src/training/`
- prompt filtering and model-output quality controls outside governance ownership
- deployment, CI, or external service configuration

## Governance-Owned AI/ML Touchpoints

| File | Governance role | Decision type |
|---|---|---|
| `src/governance/model_governance.cpp` | permit or deny training-data export based on classification and restricted collections | deterministic |
| `src/governance/policy_engine.cpp` | attach or fallback model-governance checks and masking policy to request decisions | deterministic |
| `src/governance/compliance_reporter.cpp` | generate compliance, risk, CCPA, coverage, and bias audit reports from policy state | deterministic |
| `src/governance/data_masker.cpp` | apply field-level masking strategies for governed output | deterministic |
| `src/governance/data_lineage.cpp` | record lineage events, export lineage JSON, and forward lineage audit entries | deterministic |
| `src/governance/cross_border_transfer.cpp` | determine whether transfer is allowed and which transfer mechanism applies | deterministic |

## Impact Categories

### 1. Training Data Export Control

- Owner:
  - `src/governance/model_governance.cpp`
- Verified impact:
  - blocks model-training exports for classifications `geheim` and `streng-geheim`
  - blocks exports for explicitly restricted collections
  - emits audit entries and Prometheus counters for permitted and denied decisions
  - records lineage for permitted model-training exports when a lineage tracker is configured

### 2. Policy-Driven Request Governance

- Owner:
  - `src/governance/policy_engine.cpp`
- Verified impact:
  - can delegate export permission checks to an injected `ModelGovernancePolicy`
  - keeps a fallback classification-based deny rule when no model-governance policy is configured
  - exposes masking policy together with query permission results
- Important limit:
  - this module gates governed behavior; it does not own downstream model execution.

### 3. Compliance and Bias Reporting

- Owner:
  - `src/governance/compliance_reporter.cpp`
- Verified impact:
  - analyzes policy coverage and overlapping rules
  - detects governance gaps such as missing encryption, missing audit, or overly permissive classified-data rules
  - generates framework-oriented compliance reports and access-control matrices
  - provides a dedicated bias audit report path for adapter/dataset combinations

### 4. Data Masking and Lineage

- Owners:
  - `src/governance/data_masker.cpp`
  - `src/governance/data_lineage.cpp`
- Verified impact:
  - masking supports redact, tokenize, truncate, and hash strategies
  - masking emits `governance_fields_masked_total` counters by strategy
  - lineage tracking records chronologically ordered events and forwards them to audit logging when configured
  - lineage supports upstream and downstream traversal per event chain

### 5. Cross-Border Transfer Governance

- Owner:
  - `src/governance/cross_border_transfer.cpp`
- Verified impact:
  - maps destination regions to transfer mechanisms such as adequacy decision, SCC, BCR, derogation, or prohibited
  - defaults unknown destinations to `PROHIBITED`
  - returns a structured transfer decision with `allowed`, mechanism, header value, and reason

## Primary Risks and Controls

| Risk | Governance control |
|---|---|
| model training on disallowed data | classification and restricted-collection gate in `ModelGovernancePolicy::checkExportPermission()` |
| permissive policy state on sensitive data | gap detection for encryption, audit, and export/cache combinations in `ComplianceReporter` |
| missing auditability of AI/ML data use | audit logger integration in model-governance and lineage paths |
| weak output governance for governed fields | strategy-based masking via `DataMasker` |
| untracked model-training provenance | lineage recording with `LineageEventType::MODEL_TRAINING` |
| unlawful transfer destination handling | deny-by-default transfer mechanism lookup |

## Failure and Fallback Semantics

| Path | Verified fallback or failure behavior |
|---|---|
| model export permission | deny when classification is blocked or collection is restricted |
| policy-engine export check | fallback classification deny rule if no `ModelGovernancePolicy` is configured |
| masking tokenization | empty secret or HMAC failure falls back to SHA-256-based tokenization path |
| cross-border transfer | unknown or empty destination resolves to prohibited decision |
| lineage/audit integration | lineage recording continues even when no audit logger is configured |

## Operating Limits

- This document does not inventory every AI/ML implementation across the repository; it covers only governance-owned impact surfaces.
- Bias and compliance reports are only as complete as the loaded policy/rule state and the supplied dataset statistics.
- Governance controls can deny, mask, classify, and record, but they do not replace the owning runtime module's quality or safety controls.

## Boundaries

| Topic | Owning document |
|---|---|
| governance module runtime architecture | `src/governance/ARCHITECTURE.md` |
| governance module security posture | `src/governance/SECURITY.md` |
| governance module audit state | `src/governance/AUDIT.md` |
| future governance roadmap | `src/governance/ROADMAP.md` |
| broader AI runtime behavior | module-local docs in `src/llm/`, `src/rag/`, `src/search/`, `src/training/` |

## Sourcecode Verification (Module: governance/ai-ml-impact-assessment)

- Verified files:
  - `src/governance/model_governance.cpp`
  - `src/governance/policy_engine.cpp`
  - `src/governance/compliance_reporter.cpp`
  - `src/governance/data_masker.cpp`
  - `src/governance/data_lineage.cpp`
  - `src/governance/cross_border_transfer.cpp`
- Verified behavior surfaces:
  - deterministic training-export gating and fallback rules
  - compliance, risk, and bias reporting surfaces
  - masking strategy application and lineage capture
  - deny-by-default cross-border transfer decisions
- Result:
  - the previous repo-wide AI/ML inventory has been replaced with a governance-owned impact assessment limited to module-local responsibilities
  - current claims are scoped to source-verifiable governance controls rather than cross-repository AI behavior
