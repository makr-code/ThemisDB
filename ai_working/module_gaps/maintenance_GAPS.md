# maintenance Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: maintenance
- Generated: 2026-06-02 11:55:48
- Status: High-Priority Findings Present
- Total Findings: 18
- Actionable Findings (Critical + High): 8
- Affected Files: 2

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 8 |
| Medium | 10 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 9 |
| container | 4 |
| raii | 2 |
| security | 2 |
| reliability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/maintenance/database_maintenance_orchestrator.cpp | 17 | 0 | 7 | 10 | 0 |
| src/maintenance/maintenance_registry.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/maintenance/database_maintenance_orchestrator.cpp
Total findings: 17

- Line 724: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: result[task_type_str] = handler->handlerName();
- Line 871: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::shared_ptr<IDistributedLock> acquired_dist_lock;
- Line 929: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: } dist_lock_guard{std::move(acquired_dist_lock), schedule_id};
- Line 1302: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = handler->execute(job.id, task_type);
- Line 1302: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result = handler->execute(job.id, task_type);
- Line 1365: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("window_start_hour must be in [0, 23]");
- Line 1368: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("window_end_hour must be in [0, 23]");
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(job);
  Confidence: band=high; score=0.74
- Line 662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.module_signals.push_back(std::move(sig));
  Confidence: band=high; score=0.74
- Line 716: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string>
  Confidence: band=high; score=0.74
- Line 720: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> result;
  Confidence: band=high; score=0.74
- Line 1388: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<MaintenanceTaskType, std::size_t> taskIndex;
  Confidence: band=high; score=0.74
- Line 1425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents[prereq].push_back(dep.task_type);

### src/maintenance/maintenance_registry.cpp
Total findings: 1

- Line 145: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [mgr]() -> ModuleHealthSignal {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
