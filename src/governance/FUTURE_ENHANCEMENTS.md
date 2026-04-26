> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Governance Module - Future Enhancements

This document covers planned enhancements to the Governance module beyond what is tracked in `ROADMAP.md`. It focuses on `policy_engine.cpp`, `policy_manager.cpp`, `policy_manager_versioned.cpp`, `compliance_reporter.cpp`, `policy_validator.cpp`, and related files. The features listed below build on the production-ready foundation already delivered (policy hot-reload, CCPA/CPRA support, OPA integration, automated data masking, and AI/ML model governance are all implemented); the remaining items extend the module toward full GDPR Article 17/20 automation, cross-border transfer control, and embedded OPA Wasm evaluation.

## Design Constraints

- `PolicyEngine` must remain the single authority for all access-control decisions at query time; no module may bypass it by querying storage directly.
- Policy changes must be atomic: a partial reload that leaves the engine in an inconsistent state must be rolled back and the previous policy set reinstated.
- Compliance rule evaluation must be deterministic and side-effect-free so that `policy_validator.cpp` can run the same evaluation in dry-run mode without affecting the audit trail.
- The audit trail written by `policy_coordinator.cpp` must be append-only and tamper-evident; no governance enhancement may introduce a code path that modifies or deletes existing audit entries.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `PolicyManager::reloadPolicies(source)` | File-watcher thread, admin REST API | Must be atomic; replaces `policy_manager_versioned.cpp` reload path |
| `PolicyEngine::simulateDecision(request)` | Policy dry-run CLI, `policy_validator.cpp` | Returns allow/deny + matching rule without writing audit entry |
| `ComplianceReporter::generateReport(regulation, range)` | Admin API `/governance/report`, scheduled job | Covers GDPR, HIPAA, CCPA; outputs JSON + PDF via pluggable renderer |
| `DataMasker::maskFields(doc, policy)` | Query execution layer, export pipeline | New interface; invoked after `PolicyEngine::checkQueryPermission()` |
| `OpaAdapter::evaluate(policy_bundle, input)` | `PolicyEngine` OPA integration path | Wraps OPA REST API or embedded Wasm engine |

## Planned Features

### CSV Export for Generic JSON Compliance Reports ✅ **Implemented in v1.8.0**
**Priority:** Medium
**Target Version:** v1.8.0

`compliance_reporting.cpp` line 1405 logs an explicit error: "CSV export not implemented for generic JSON reports". Several specific report types already implement `toCSV()` (e.g., `PolicySummaryReport`, `ComplianceStatusReport`, `RiskAssessmentReport`), but the generic `generateReport(format=CSV)` path falls through to an error log instead of delegating to the per-report `toCSV()` method.

**Status:** Implemented. See `include/governance/compliance_reporting.h`, `src/governance/compliance_reporting.cpp`, `tests/test_compliance_reporting.cpp`.

**Implementation Notes:**
- `[x]` In `ComplianceReporter::exportReport()`, when `format == ReportFormat::CSV`, delegates to `generateCSVFromJson()` instead of logging an error.
- `[x]` `IComplianceReport::toCSV()` pure-virtual method defined; all 6 report structs (`PolicySummaryReport`, `ComplianceStatusReport`, `AccessControlMatrix`, `RiskAssessmentReport`, `ChangeHistoryReport`, `CcpaReport`) implement it.
- `[x]` 12 `CsvExport_*` unit tests cover `CcpaReport`, `AccessControlMatrix`, `ChangeHistoryReport` column headers and delimiter escaping.

---



### PolicyManager Hot-Reload ✅ **Implemented in v1.8.0**
**Priority:** High
**Target Version:** v1.6.0

Enable `PolicyManager` to reload policies from disk or a remote config store without restarting the server. Currently a restart is required to pick up policy changes, which creates a compliance gap during the downtime window. The implementation in `policy_manager_versioned.cpp` already tracks policy versions; hot-reload builds on that foundation.

**Status:** Implemented. See `include/governance/policy_manager.h`, `src/governance/policy_manager.cpp`, `include/governance/policy_file_watcher.h`, `src/governance/policy_file_watcher.cpp`, `tests/test_policy_manager.cpp`, `tests/test_governance_policy_hot_reload.cpp`.

**Implementation Notes:**

- `[x]` `PolicyFileWatcher` class monitors policy files with polling + 500 ms debounce settling window before triggering reload.
- `[x]` `PolicyManager::reloadPolicies()` validates the new policy set via `PolicyValidator::validateRuleset()` before swapping; on validation failure, logs the error and retains the current set.
- `[x]` `PolicySet` struct added as the immutable snapshot type; `std::shared_ptr<const PolicySet>` with `shared_mutex` promotion provides the double-buffer: readers capture a snapshot before reload completes normally while the new set is promoted.
- `[x]` `governance_policy_reload_total` Prometheus counter emitted (labels: `result=success|failure`) in both `PolicyEngine::reloadIfChanged()` and `PolicyManager::reloadPolicies()`.
- `[x]` Audit entry with old and new policy version hashes written by `PolicyEngine::reloadIfChanged()`.

**Performance Targets:**

- `[x]` Policy reload latency ≤ 100 ms from change detection to new policy becoming active (reload itself is a YAML parse + shared_ptr swap).
- `[x]` Zero requests dropped or erroneously denied during the reload window (double-buffer keeps old set alive via ref-count).

---

### CCPA / CPRA Data Subject Rights Enforcement ✅ **Implemented in v1.6.0**

**Priority:** High
**Target Version:** v1.6.0

~~Add a CCPA/CPRA compliance rule set to `compliance_reporter.cpp` and `policy_engine.cpp`.~~

**Status:** Implemented. See `include/governance/ccpa_rules.h`, `src/governance/ccpa_rules.cpp`.

- `IComplianceRule` interface added; `RightToKnow`, `RightToDelete`, `OptOutOfSale`, and `DataPortability` rule evaluators implemented in `ccpa_rules.cpp`.
- `PolicyEngine` enforces opt-out at query time via `setCcpaOptOutSubjects()` / `X-User-Id` header lookup; `PolicyDecision::ccpa_opted_out` flag set; `export_allowed` forced to `false` for opted-out subjects.
- `ComplianceReporter::generateCcpaReport()` added, producing structured JSON/CSV with data categories, third-party disclosures, and per-evaluator compliance gaps.
- CCPA/HIPAA conflict detection integrated into `PolicyValidator::detectConflicts()` via `detectCcpaHipaaConflicts()`.

**Performance Targets:**

- CCPA opt-out flag lookup adds ≤ 0.5 ms to query-time policy evaluation p99.
- CCPA report generation for a 90-day window completes in ≤ 10 s for up to 1 M data subjects.

---

### Automated Data Masking in Query Results ✅ **Implemented in v1.7.0**

**Priority:** High
**Target Version:** v1.7.0

Implements `DataMasker` which post-processes query result documents to redact or tokenize sensitive fields based on configured governance rules. This is the last in-process defense before data leaves the query engine and is required for GDPR Article 25 (data protection by design).

**Status:** Implemented. See `include/governance/data_masker.h`, `src/governance/data_masker.cpp`.

- `DataMasker::maskFields(doc, policy)` and `maskFieldsArray(docs, policy)` apply field-level masking to JSON query result documents.
- Four masking strategies: `REDACT` (`"[REDACTED]"`), `TOKENIZE` (stable HMAC-SHA256 pseudonym; prefix `"tkn_"`), `TRUNCATE` (first N chars + `"..."`), `HASH` (SHA-256 hex; prefix `"sha_"`).
- `PolicyEngine::checkQueryPermission()` returns both a `PolicyDecision` and a `FieldMaskingPolicy`; query executor calls `DataMasker::maskFields()` before serialising results.
- Masking rules are loaded from the governance YAML `data_masking` section; hot-reloaded atomically alongside classification profiles.
- `governance_fields_masked_total` Prometheus counter emitted per masked field (label: `strategy`).
- TOKENIZE uses HMAC-SHA256 keyed on a per-collection secret; same input always produces same pseudonym (join-query support).
- Operator warned at load time when placeholder `collection_secret` is detected.

---

### OPA (Open Policy Agent) Integration ✅ **Implemented in v1.8.0**

**Priority:** Medium
**Target Version:** v1.8.0

~~Add an OPA adapter so that operators can write policies in Rego and evaluate them via OPA's REST API or an embedded Wasm bundle.~~

**Status:** Implemented. See `include/governance/opa_adapter.h`, `src/governance/opa_adapter.cpp`.

- `governance::OpaAdapter` implements `PolicyEngine::IPolicyEvaluator`; evaluates policy decisions by posting JSON input to `POST /v1/data/{policy_path}` of a local OPA sidecar.
- Fallback chain: if OPA returns a non-2xx response or times out (configurable, default 50 ms), native `PolicyEngine` rule evaluation is used and `governance_opa_fallback_total` Prometheus counter is emitted.
- OPA input contains the full request headers and route: `{"input": {"headers": {...}, "route": "/path"}}`.
- OPA response supports both a simple boolean result (`{"result": false}` = strict deny) and a full structured `PolicyDecision` (`{"result": {"allow": true, "classification": "offen", ...}}`).
- CCPA opt-out enforcement is always applied on top of the OPA decision (opt-out takes precedence).
- Policy bundles are loaded into OPA out-of-band (standard OPA bundle API); `PolicyEngine` only needs the OPA endpoint URL and the decision path via `OpaAdapter::Config`.
- Attach via `PolicyEngine::setOpaEvaluator(adapter.get())`; detach by passing `nullptr`.

---

### AI/ML Model Governance ✅ **Implemented in v2.0.0**

**Priority:** Low
**Target Version:** v2.0.0

Extend the Governance module to track training data lineage for models trained on ThemisDB collections, detect bias indicators in training datasets, and enforce data-use policies that restrict which collections may be used for model training.

**Status:** Implemented. See `include/governance/model_governance.h`, `src/governance/model_governance.cpp`.

- `ModelGovernancePolicy` implemented with `checkExportPermission()` evaluated when an export job carries `purpose=MODEL_TRAINING`; denies requests for "geheim"/"streng-geheim" data and for operator-restricted collections.
- Training data lineage recorded via `DataLineageTracker` as `MODEL_TRAINING` events (new enum value); captures export job ID, collection IDs, field selectors, timestamp, requesting user, and `adapter_id`.
- `BiasFieldStats` and `BiasAuditReport` structs added; `ComplianceReporter::generateBiasAuditReport()` computes demographic parity and representation statistics over configurable fields, assigning PASSED / FLAGGED / FAILED status and actionable recommendations.
- `governance_model_export_total` Prometheus counter (labels: `result=permitted|denied_classification|denied_restricted_collection`) emitted on every export permission decision.

**Performance Targets:**

- `ModelGovernancePolicy` evaluation at export time adds ≤ 2 ms to export job startup.
- Bias audit for a 1 M document training dataset completes in ≤ 5 minutes as a background job.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Each new rule evaluator (`CcpaRuleSet`, `DataMasker`, `OpaAdapter`) must have isolated unit tests with mocked policy sets and mocked OPA HTTP client; audit trail assertions on every decision |
| Integration | Full policy lifecycle | Hot-reload test: write new policy file, assert new rules active within 100 ms, assert zero denied requests during transition; run against live governance stack |
| Performance | Policy evaluation ≤ 5 ms p99 | Benchmark in `benchmarks/governance_bench.cpp` with 100 simultaneous query threads and a 500-rule policy set; run in CI on PRs touching `policy_engine.cpp` |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| Policy evaluation latency p99 | ~3 ms (estimate) | ≤ 5 ms with 500 rules | `benchmarks/governance_bench.cpp`, 100-thread load |
| Policy reload latency | ~5 s (restart) | ≤ 100 ms (hot-reload) | Timed in integration test with file-watcher |
| CCPA opt-out flag lookup | N/A | ≤ 0.5 ms added to query p99 | Micro-benchmark in `tests/governance/` |
| DataMasker per-document overhead | N/A | ≤ 1 ms per 50-field doc | Isolated benchmark in `tests/governance/bench_masker.cpp` |

## Security / Reliability

- Policy reload is atomic via double-buffer swap; a validation failure during reload retains the previous policy set and emits an alert-severity log entry so operators are not silently left with stale rules.
- The audit trail is append-only; `DataMasker`, `OpaAdapter`, and all new compliance rule evaluators write to the audit trail but never read from or modify it.
- OPA fallback is transparent to the caller but must always be logged; silent fallback with no observability signal is not acceptable.
- `DataMasker` TOKENIZE pseudonyms are keyed on a per-collection secret stored in the key management service, never in source code or configuration files.
- `ModelGovernancePolicy` must be evaluated before any training-purpose export begins; partial exports that start before policy evaluation must be rejected, not retroactively audited.

---

## Identified Gaps (from AI_ML_IMPACT_ASSESSMENT.md)

### Gap 6 — Governance Visibility into ML/AI Token-Cost Budgets (Target: Q4 2026)

**Source:** `AI_ML_IMPACT_ASSESSMENT.md §7, Gap 6 (Severity: High/S1)`
**Primary implementation:** `src/llm/FUTURE_ENHANCEMENTS.md §Gap 6` (`LLMTokenBudgetManager`).

**Problem:** `PolicyEngine` governs data access and model export but has no visibility
into ML/AI resource consumption (token spend, GPU utilization per tenant).  A tenant
that is permitted to run inference can exhaust shared GPU capacity without any
governance checkpoint, because token budgets are currently absent (see Gap 6 in the
LLM module).

**Governance Module Responsibility:**
- Extend `PolicyEngine` with a `checkTokenBudget(tenant_id, path_id, requested_tokens)`
  method that delegates to `LLMTokenBudgetManager` (when available).
- Add `TokenBudgetPolicy` as a new policy type in `model_governance.cpp`:
  ```
  token_budget:
    per_tenant_hourly: 50000
    per_path:
      agentic: 16384
      judge:   8192
  ```
- Emit `governance_token_budget_deny_total{tenant, path}` Prometheus counter on
  policy DENY so operations teams can detect budget abuse.
- Write `BUDGET_EXCEEDED` events to the existing append-only audit trail with
  `tenant_id`, `path_id`, `requested_tokens`, and `remaining_budget`.

**Inputs:** `PolicyEngine::checkTokenBudget()` call from `AsyncInferenceEngine`.
**Outputs:** PERMIT / DENY decision; audit log entry; Prometheus counter.
**Constraints:** Must not add synchronous network round-trips; local policy evaluation only.
**Errors:** Budget policy absent → PERMIT (backward-compatible); budget exceeded → DENY.
**Tests:** 3 unit tests — budget permit; budget deny; audit entry written on deny.
**Perf target:** `checkTokenBudget()` ≤ 100 µs (local memory lookup + atomic read).

## Scientific References

[1] European Parliament and Council, "General Data Protection Regulation (GDPR)," *Official Journal of the European Union*, L 119, May 2016. https://eur-lex.europa.eu/eli/reg/2016/679/oj

[2] National Institute of Standards and Technology, "Security and Privacy Controls for Information Systems and Organizations," NIST Special Publication 800-53 Rev. 5, Sep. 2020. https://doi.org/10.6028/NIST.SP.800-53r5

[3] R. S. Sandhu, E. J. Coyne, H. L. Feinstein, and C. E. Youman, "Role-Based Access Control Models," *IEEE Computer*, vol. 29, no. 2, pp. 38–47, Feb. 1996. https://doi.org/10.1109/2.485845

[4] T. Moses, Ed., "eXtensible Access Control Markup Language (XACML) Version 3.0," OASIS Standard, Jan. 2013. https://docs.oasis-open.org/xacml/3.0/xacml-3.0-core-spec-os-en.html

[5] Open Policy Agent contributors, "Open Policy Agent Documentation," 2024. https://www.openpolicyagent.org/docs/latest/

[6] P. A. Bonatti, S. Decker, A. Polleres, and V. Presutti, "Knowledge Graphs: New Directions for Knowledge Representation on the Semantic Web," *Dagstuhl Reports*, vol. 8, no. 9, pp. 29–111, 2019. https://doi.org/10.4230/DagRep.8.9.29

[7] A. Gebhardt, "GDPR Data Subject Rights Automation: Architecture Patterns for Compliance Engineering," *arXiv preprint*, arXiv:2105.02034, 2021.

[8] O. Tene and J. Polonetsky, "Big Data for All: Privacy and User Control in the Age of Analytics," *Northwestern Journal of Technology and Intellectual Property*, vol. 11, no. 5, pp. 239–273, 2013.

[9] California Legislature, "California Consumer Privacy Act (CCPA)," California Civil Code §1798.100 et seq., 2018. https://oag.ca.gov/privacy/ccpa

[10] Payment Card Industry Security Standards Council, "PCI DSS v4.0," Mar. 2022. https://www.pcisecuritystandards.org/document_library/

[11] AICPA, "SOC 2 — Trust Services Criteria," 2017. https://www.aicpa.org/resources/download/2017-trust-services-criteria

[12] V. C. Hu, D. Ferraiolo, R. Kuhn, A. Schnitzer, K. Sandlin, R. Miller, and K. Scarfone, "Guide to Attribute Based Access Control (ABAC) Definition and Considerations," NIST Special Publication 800-162, Jan. 2014. https://doi.org/10.6028/NIST.SP.800-162

[13] M. Mehrabi, A. Morstatter, N. Saxena, K. Lerman, and A. Galstyan, "A Survey on Bias and Fairness in Machine Learning," *ACM Computing Surveys*, vol. 54, no. 6, pp. 1–35, Jul. 2021. https://doi.org/10.1145/3457607
