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

### CCPA / CPRA Data Subject Rights Enforcement
**Priority:** High
**Target Version:** v1.6.0

Add a CCPA/CPRA compliance rule set to `compliance_reporter.cpp` and `policy_engine.cpp`. CCPA requires honoring opt-out of sale, right-to-know, and right-to-delete requests. The existing GDPR rules in `policy_engine.cpp` cover right-to-erasure but not California-specific opt-out-of-sale semantics.

**Implementation Notes:**
- Add `CcpaRuleSet` in a new `ccpa_rules.cpp`; implement `RightToKnow`, `RightToDelete`, `OptOutOfSale`, and `DataPortability` rule evaluators conforming to the existing `IComplianceRule` interface.
- `OptOutOfSale` rule checks a per-subject preference flag stored in the data classification metadata; `PolicyEngine` consults this flag on every read query for opted-out subjects.
- Add `ComplianceReporter::generateCcpaReport()` that produces a structured JSON summary of data categories, third-party disclosures, and opt-out counts for a configurable date range.
- Integrate CCPA rule set into `policy_validator.cpp` so that policy conflicts between CCPA and HIPAA rules are detected at load time.

**Performance Targets:**
- CCPA opt-out flag lookup adds ≤ 0.5 ms to query-time policy evaluation p99.
- CCPA report generation for a 90-day window completes in ≤ 10 s for up to 1 M data subjects.

---

### Automated Data Masking in Query Results
**Priority:** High
**Target Version:** v1.7.0

Implement `DataMasker` which post-processes query result documents to redact or tokenize sensitive fields based on the requester's policy grants. This is the last in-process defense before data leaves the query engine and is required for GDPR Article 25 (data protection by design).

**Implementation Notes:**
- Add `data_masker.cpp` with strategies: `REDACT` (replace value with `"[REDACTED]"`), `TOKENIZE` (replace with a stable pseudonym via HMAC-SHA256 keyed on a per-collection secret), `TRUNCATE` (keep first N characters), and `HASH` (SHA-256 hex digest).
- `PolicyEngine::checkQueryPermission()` returns a `MaskingPolicy` alongside allow/deny; the query executor applies `DataMasker::maskFields(doc, masking_policy)` before serializing the result.
- Masking is applied transparently to AQL `RETURN` projections and REST API responses; the caller cannot observe whether a field was masked vs absent unless granted the `VIEW_MASKED_FIELDS` privilege.
- Add `governance_fields_masked_total` Prometheus counter (label: `strategy`) to track masking activity.

**Performance Targets:**
- `DataMasker::maskFields()` overhead ≤ 1 ms per document for documents with ≤ 50 fields.
- TOKENIZE strategy must produce the same pseudonym for the same input within the same collection to support join queries over pseudonymized identifiers.

---

### OPA (Open Policy Agent) Integration
**Priority:** Medium
**Target Version:** v1.8.0

Add an OPA adapter so that operators can write policies in Rego and evaluate them via OPA's REST API or an embedded Wasm bundle. This enables GitOps policy-as-code workflows where policies are versioned in a repository and deployed via CI/CD without touching ThemisDB configuration files.

**Implementation Notes:**
- Add `opa_adapter.cpp` implementing `IPolicyEvaluator`; evaluates policy decisions by posting JSON input to `POST /v1/data/{policy_path}` of a local OPA sidecar.
- Add a fallback chain: if OPA returns a non-2xx response or times out (configurable, default 50 ms), fall back to the native `PolicyEngine` rule evaluation and emit `governance_opa_fallback_total` counter.
- Policy bundles are loaded into OPA out-of-band (standard OPA bundle API); `PolicyManager` only needs the OPA endpoint URL and the decision path.
- The existing `policy_manager_versioned.cpp` version history is preserved; OPA bundle version is recorded alongside the native policy version in the audit trail.

**Performance Targets:**
- OPA evaluation round-trip ≤ 5 ms p99 when OPA sidecar is co-located on the same host.
- Fallback to native evaluation must complete within the same 5 ms budget.

---

### AI/ML Model Governance
**Priority:** Low
**Target Version:** v2.0.0

Extend the Governance module to track training data lineage for models trained on ThemisDB collections, detect bias indicators in training datasets, and enforce data-use policies that restrict which collections may be used for model training.

**Implementation Notes:**
- Add `ModelGovernancePolicy` rule type to `policy_engine.cpp`; evaluated when the Exporters module calls `PolicyEngine::checkExportPermission()` with a `purpose=MODEL_TRAINING` context.
- Record training data lineage in a new `model_lineage` collection: export job ID, collection IDs, field selectors, timestamp, and requesting user; linked to the resulting LoRA adapter metadata via `adapter_id`.
- Add `BiasAuditReport` to `compliance_reporter.cpp` that computes demographic parity and representation statistics over configurable fields in the training dataset.
- Bias audit runs as a background job triggered by the Exporters module post-export; results are stored in the audit trail and surfaced via `GET /governance/model/{adapter_id}/bias-report`.

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
