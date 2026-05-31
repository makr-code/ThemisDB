# maintenance Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: maintenance
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 31
- Actionable Findings (Critical + High): 15
- Affected Files: 3

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 14 |
| Medium | 16 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 13 |
| container | 11 |
| security | 4 |
| raii | 2 |
| reliability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/maintenance/database_maintenance_orchestrator.cpp | 29 | 1 | 12 | 16 | 0 |
| src/maintenance/maintenance_registry.cpp | 1 | 0 | 1 | 0 | 0 |
| src/maintenance/maintenance_schedule_store.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/maintenance/database_maintenance_orchestrator.cpp
Total findings: 29

- Line 1340: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = jobs_.begin(); it != jobs_.end(); ) {
- Line 154: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, entry] : loaded) {
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, entry] : schedules_) {
  Confidence: band=very_high; score=0.9
- Line 644: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, job] : jobs_) {
  Confidence: band=very_high; score=0.9
- Line 726: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: result[task_type_str] = handler->handlerName();
- Line 873: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::shared_ptr<IDistributedLock> acquired_dist_lock;
- Line 931: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: } dist_lock_guard{std::move(acquired_dist_lock), schedule_id};
- Line 946: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [jid, jobj] : jobs_) {
  Confidence: band=very_high; score=0.9
- Line 1299: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handler = it->second;
- Line 1304: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = handler->execute(job.id, task_type);
- Line 1304: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result = handler->execute(job.id, task_type);
- Line 1367: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("window_start_hour must be in [0, 23]");
- Line 1370: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("window_end_hour must be in [0, 23]");
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry);
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(job);
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(job);
- Line 664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.module_signals.push_back(std::move(sig));
  Confidence: band=high; score=0.74
- Line 665: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.module_signals.push_back(std::move(sig));
- Line 673: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.module_signals.push_back(std::move(sig));
- Line 718: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string>
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> result;
  Confidence: band=high; score=0.74
- Line 1390: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<MaintenanceTaskType, std::size_t> taskIndex;
  Confidence: band=high; score=0.74
- Line 1427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1427: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[prereq].push_back(dep.task_type);
  Confidence: band=high; score=0.74
- Line 1428: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents[prereq].push_back(dep.task_type);
- Line 1446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(t);
- Line 1457: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(cur);

### src/maintenance/maintenance_registry.cpp
Total findings: 1

- Line 147: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [mgr]() -> ModuleHealthSignal {

### src/maintenance/maintenance_schedule_store.cpp
Total findings: 1

- Line 71: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto scan_result = engine_->scanPrefix(

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
