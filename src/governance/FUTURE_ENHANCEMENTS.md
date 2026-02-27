# Governance Module - Future Enhancements

## Scope

This document covers planned enhancements to the Governance module beyond what is tracked in `ROADMAP.md`. It focuses on `policy_engine.cpp`, `policy_manager.cpp`, `policy_manager_versioned.cpp`, `compliance_reporter.cpp`, `policy_validator.cpp`, and related files. Features here describe the engineering work required to add policy hot-reload, CCPA/CPRA support, OPA integration, automated data masking, and AI/ML model governance to elevate the module from Beta to production-grade.

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

### Policy Hot-Reload Without Restart

**Priority:** High
**Target Version:** v1.6.0

Enable `PolicyManager` to reload policies from disk or a remote config store without restarting the server. Currently a restart is required to pick up policy changes, which creates a compliance gap during the downtime window. The implementation in `policy_manager_versioned.cpp` already tracks policy versions; hot-reload builds on that foundation.

**Implementation Notes:**

- Add a `PolicyFileWatcher` class that uses `inotify` (Linux) / `kqueue` (macOS) to detect changes to the policy directory; debounce events with a 500 ms settling window before triggering reload.
- `PolicyManager::reloadPolicies()` validates the new policy set via `PolicyValidator::validate()` before swapping; on validation failure, log the error and retain the current set.
- Use a `std::shared_ptr` double-buffer: requests in flight hold a reference to the old policy set and complete normally while the new set is atomically promoted via `std::atomic<std::shared_ptr<PolicySet>>::store(memory_order_release)`.
- Emit a `governance_policy_reload_total` Prometheus counter (labels: `result=success|failure`) and write an audit entry with the old and new policy version hashes.

**Performance Targets:**

- Policy reload latency ≤ 100 ms from file change detection to new policy becoming active.
- Zero requests dropped or erroneously denied during the reload window.

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
