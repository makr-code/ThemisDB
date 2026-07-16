### Context

This issue implements the roadmap item 'CSV Export for Generic JSON Compliance Reports' for the governance domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: CSV Export for Generic JSON Compliance Reports

### Goal

Deliver the scoped changes for CSV Export for Generic JSON Compliance Reports in src/governance/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### CSV Export for Generic JSON Compliance Reports
**Priority:** Medium
**Target Version:** v1.8.0

`compliance_reporting.cpp` line 1405 logs an explicit error: "CSV export not implemented for generic JSON reports". Several specific report types already implement `toCSV()` (e.g., `PolicySummaryReport`, `ComplianceStatusReport`, `RiskAssessmentReport`), but the generic `generateReport(format=CSV)` path falls through to an error log instead of delegating to the per-report `toCSV()` method.

**Implementation Notes:**
- `[ ]` In `ComplianceReporter::generateReport()` at line 1404, when `format == ReportFormat::CSV`, dispatch to the concrete report type's `toCSV()` method via virtual dispatch instead of logging an error.
- `[ ]` Define a `IComplianceReport::toCSV()` pure-virtual method in the report base class so all report types are required to implement it.
- `[ ]` Add unit tests for CSV output of `CcpaReport`, `AccessControlMatrix`, and `ChangeHistoryReport` verifying column headers and delimiter escaping.

---



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

### Acceptance Criteria

- [ ] In `ComplianceReporter::generateReport()` at line 1404, when `format == ReportFormat::CSV`, dispatch to the concrete report type's `toCSV()` method via virtual dispatch instead of logging an error.
- [ ] Define a `IComplianceReport::toCSV()` pure-virtual method in the report base class so all report types are required to implement it.
- [ ] Add unit tests for CSV output of `CcpaReport`, `AccessControlMatrix`, and `ChangeHistoryReport` verifying column headers and delimiter escaping.
- [ ] Add a `PolicyFileWatcher` class that uses `inotify` (Linux) / `kqueue` (macOS) to detect changes to the policy directory; debounce events with a 500 ms settling window before triggering reload.
- [ ] `PolicyManager::reloadPolicies()` validates the new policy set via `PolicyValidator::validate()` before swapping; on validation failure, log the error and retain the current set.
- [ ] Use a `std::shared_ptr` double-buffer: requests in flight hold a reference to the old policy set and complete normally while the new set is atomically promoted via `std::atomic<std::shared_ptr<PolicySet>>::store(memory_order_release)`.
- [ ] Emit a `governance_policy_reload_total` Prometheus counter (labels: `result=success|failure`) and write an audit entry with the old and new policy version hashes.
- [ ] Policy reload latency ≤ 100 ms from file change detection to new policy becoming active.
- [ ] Zero requests dropped or erroneously denied during the reload window.

### Relationships

- Roadmap row: #173 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/governance/FUTURE_ENHANCEMENTS.md#csv-export-for-generic-json-compliance-reports
- Source key: roadmap:173:governance:v1.8.0:csv-export-for-generic-json-compliance-reports

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:173:governance:v1.8.0:csv-export-for-generic-json-compliance-reports -->
<!-- roadmap-ref: row=173;module=governance;target=v1.8.0 -->
<!-- roadmap-detail: src/governance/FUTURE_ENHANCEMENTS.md#csv-export-for-generic-json-compliance-reports -->
