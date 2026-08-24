# maintenance Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: maintenance
- Generated: 2026-06-04 08:50:22
- Status: High-Priority Findings Present
- Total Findings: 14
- Actionable Findings (Critical + High): 9
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 9 |
| Medium | 3 |
| Low | 2 |

## Category Summary

| Category | Count |
|---|---:|
| map_vs_unordered_map | 3 |
| uninitialized_access | 3 |
| db_connection_leak | 2 |
| module_doc_linkset_drift | 2 |
| null_dereference | 2 |
| delete_without_nullptr | 1 |
| explicit_delete | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| maintenance/database_maintenance_orchestrator.cpp | 11 | 0 | 8 | 3 | 0 |
| maintenance/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| maintenance/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| maintenance/maintenance_registry.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### maintenance/database_maintenance_orchestrator.cpp
Total findings: 11

- Line 446: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string name = it->second.name;



    // Remove from durable store first; if the durable remove fails, abort the

    // delete so the schedule does not resurrect itself on the next restart.

    if (schedule_store_) {

        auto persist_result = schedule_store_->remove(id);

        if (!persist_result.has_value()) {
- Line 446: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // delete so the schedule does not resurrect itself on the next restart.
- Line 724: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: result[task_type_str] = handler->handlerName();
- Line 871: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::shared_ptr<IDistributedLock> acquired_dist_lock;
- Line 929: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: } dist_lock_guard{std::move(acquired_dist_lock), schedule_id};
- Line 1302: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto result = handler->execute(job.id, task_type);
- Line 1365: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("window_start_hour must be in [0, 23]");
- Line 1368: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("window_end_hour must be in [0, 23]");
- Line 716: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string>
- Line 720: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> result;
- Line 1388: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<MaintenanceTaskType, std::size_t> taskIndex;

### maintenance/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### maintenance/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### maintenance/maintenance_registry.cpp
Total findings: 1

- Line 145: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [mgr]() -> ModuleHealthSignal {

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
