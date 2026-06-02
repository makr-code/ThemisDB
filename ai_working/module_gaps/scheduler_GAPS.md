# scheduler Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: scheduler
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 129
- Actionable Findings (Critical + High): 47
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 12 |
| High | 35 |
| Medium | 82 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 46 |
| container | 19 |
| performance | 13 |
| concurrency | 10 |
| security | 8 |
| determinism | 6 |
| exception_safety | 6 |
| reliability | 6 |
| legacy_duplication | 5 |
| memory | 5 |
| observability | 4 |
| raii | 4 |
| platform | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/scheduler/task_scheduler.cpp | 65 | 10 | 22 | 33 | 0 |
| src/scheduler/event_trigger.cpp | 15 | 2 | 5 | 8 | 0 |
| src/scheduler/task_anomaly_detector.cpp | 14 | 0 | 0 | 14 | 0 |
| src/scheduler/task_audit_manager.cpp | 12 | 0 | 0 | 12 | 0 |
| src/scheduler/external_scheduler_adapter.cpp | 11 | 0 | 1 | 10 | 0 |
| src/scheduler/task_result_store.cpp | 6 | 0 | 2 | 4 | 0 |
| src/scheduler/distributed_task_coordinator.cpp | 4 | 0 | 3 | 1 | 0 |
| src/scheduler/hybrid_retention_manager.cpp | 1 | 0 | 1 | 0 | 0 |
| src/scheduler/task_audit_event.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/scheduler/task_scheduler.cpp
Total findings: 65

- Line 346: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double deadline_ms = static_cast<double>(task.sla_deadline->count());
- Line 1243: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task->avg_execution_time_ms =
- Line 1676: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
- Line 1676: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
- Line 1677: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const int base = static_cast<int>(t->priority);
- Line 1678: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (aging_thr > 0 && t->consecutive_skips >= aging_thr) {
- Line 1840: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task->avg_execution_time_ms =
- Line 2121: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task_json["trigger_type"] = static_cast<int>(task->trigger_type);
- Line 2123: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task_json["priority"] = static_cast<int>(task->priority);
- Line 2124: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task_json["trigger_logic"] = static_cast<int>(task->trigger_logic);
- Line 328: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param delay_ms           [in/out] Computed retry delay – may be clamped.
- Line 329: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param effective_max_retries [in/out] Max attempts – may be clamped.
- Line 535: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(25));
- Line 602: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: task_ptr->id = id;
- Line 605: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (task_ptr->trigger_type == ScheduledTask::TriggerType::CRON) {
- Line 607: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: updateCronExpression(id, task_ptr->cron_expression);
- Line 612: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: task_ptr->next_run = *next;
- Line 615: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (task_ptr->trigger_type == ScheduledTask::TriggerType::INTERVAL) {
- Line 617: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (task_ptr->next_run == std::chrono::system_clock::time_point{}) {
- Line 618: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: task_ptr->next_run = std::chrono::system_clock::now() + task_ptr->interval;
- Line 620: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (task_ptr->trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
- Line 644: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event.metadata["interval_ms"] = sanitized_task.interval.count();
- Line 1010: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = dependents.find(cur);
- Line 1084: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
- Line 1084: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
  Confidence: band=very_high; score=0.9
- Line 1103: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto dit = dependents.find(id);
- Line 1611: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(tasks_mutex_);
- Line 1676: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
- Line 1705: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(running_mutex_);
- Line 1918: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: failure_event.metadata["attempts_made"] = attempts_made;
- Line 2129: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cdc_json["event_types"] = nlohmann::json::array();
- Line 2241: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Load trigger configuration (with defaults for backward compatibility)
  Confidence: band=high; score=0.8
- Line 888: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 976: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<std::string>>& adj) const
  Confidence: band=high; score=0.74
- Line 979: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> in_degree;
  Confidence: band=high; score=0.74
- Line 990: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(id);
  Confidence: band=high; score=0.74
- Line 990: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(id);
  Confidence: band=high; score=0.74
- Line 996: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> dependents;
  Confidence: band=high; score=0.74
- Line 1014: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(dependent);
  Confidence: band=high; score=0.74
- Line 1047: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::shared_ptr<ScheduledTask>> task_map;
  Confidence: band=high; score=0.74
- Line 1062: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> adj;
  Confidence: band=high; score=0.74
- Line 1066: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[id].push_back(dep);
  Confidence: band=high; score=0.74
- Line 1066: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[id].push_back(dep);
  Confidence: band=high; score=0.74
- Line 1066: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[id].push_back(dep);
  Confidence: band=high; score=0.74
- Line 1076: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> dependents;
  Confidence: band=high; score=0.74
- Line 1078: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(id);
  Confidence: band=high; score=0.74
- Line 1078: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(id);
  Confidence: band=high; score=0.74
- Line 1084: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
  Confidence: band=high; score=0.74
- Line 1100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.condition_skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1138: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, nlohmann::json> dep_results;
  Confidence: band=high; score=0.74
- Line 1146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.condition_skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.condition_skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wave.push_back(id);
  Confidence: band=high; score=0.74
- Line 1190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &wave_results, i, &task_map]() {
  Confidence: band=high; score=0.74
- Line 1225: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: task_snapshot.push_back(*task);
  Confidence: band=high; score=0.74
- Line 1560: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*task);
  Confidence: band=high; score=0.74
- Line 1593: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;
  Confidence: band=high; score=0.74
- Line 1821: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2001: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json TaskScheduler::executeAqlQuery(const std::string& aql) {
  Confidence: band=high; score=0.74
- Line 2130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cdc_json["event_types"].push_back(type);
  Confidence: band=high; score=0.74
- Line 2202: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: af.close();
- Line 2252: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cdc_json = task_json["cdc_trigger"];
  Confidence: band=high; score=0.74
- Line 2359: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void TaskScheduler::validateAqlQuery(const std::string& aql) const {
  Confidence: band=high; score=0.74

### src/scheduler/event_trigger.cpp
Total findings: 15

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 236: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Poll for new events with long-polling
- Line 268: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(debounce_mutex_);
- Line 296: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::milliseconds(100),
- Line 305: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::seconds(1),
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raw_clauses.push_back(themis::utils::trim(condition.substr(pos)));
- Line 412: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raw_clauses.push_back(themis::utils::trim(condition.substr(pos, found - pos)));
- Line 428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(raw.substr(i, j - i + (j < n ? 1 : 0)));
  Confidence: band=high; score=0.74
- Line 429: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(raw.substr(i, j - i + (j < n ? 1 : 0)));
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(raw.substr(i, j - i));
- Line 448: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (k > 2) rhs += " ";
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (k > 2) rhs += " ";
- Line 453: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parsed_clauses_.push_back(std::move(pc));
  Confidence: band=high; score=0.74

### src/scheduler/task_anomaly_detector.cpp
Total findings: 14

- Line 88: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) metrics.description += ", ";
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) metrics.description += ", ";
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: intervals.push_back(std::chrono::duration<double>(interval).count());
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: intervals.push_back(std::chrono::duration<double>(interval).count());
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (double v : d) arr.push_back(v);
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ts_arr.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ts_arr.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ts_arr.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 465: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : arr) d.push_back(v.get<double>());
- Line 478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_times.push_back(
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_times.push_back(
  Confidence: band=high; score=0.74
- Line 479: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.execution_times.push_back(
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_results.push_back(b.get<bool>());
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.execution_results.push_back(b.get<bool>());

### src/scheduler/task_audit_manager.cpp
Total findings: 12

- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(event);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: case AuditQueryParams::SortBy::TIMESTAMP_ASC:
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: case AuditQueryParams::SortBy::TIMESTAMP_DESC:
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_uuids;
  Confidence: band=medium; score=0.66
- Line 311: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = j["timestamp"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(event);
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_uuids;
  Confidence: band=medium; score=0.66
- Line 468: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = j["timestamp"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 515: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(event.toJson(config_.enable_gdpr_mode));
  Confidence: band=high; score=0.74
- Line 546: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j.push_back(event.toJson(config_.enable_gdpr_mode));

### src/scheduler/external_scheduler_adapter.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2646 [scheduler] Integrate Kuber... (2026-03-12)
- Line 69: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '-';
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '-';
- Line 188: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 190: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': out += "\\'"; break;
- Line 191: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 192: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n"; break;
- Line 193: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r"; break;
- Line 194: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t"; break;
- Line 462: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "        headers={'Content-Type': 'application/json'},\n"

### src/scheduler/task_result_store.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 50: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Build a zero-padded 20-digit decimal timestamp so keys sort chronologically.
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: size_t to_delete = all_keys.size() - max_per_task_;
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.emplace_back(std::string(k), std::string(v));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(TaskExecutionResult::fromJson(j));
  Confidence: band=high; score=0.74

### src/scheduler/distributed_task_coordinator.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3364 [scheduler] Distributed cro... (2026-03-12) | #2568 [scheduler] Distrib
- Line 286: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: s.leadership_acquired = leadership_acquired_.load();
- Line 312: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: leadership_acquired_.fetch_add(1);
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(task);
  Confidence: band=high; score=0.74

### src/scheduler/hybrid_retention_manager.cpp
Total findings: 1

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/scheduler/task_audit_event.cpp
Total findings: 1

- Line 265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ecs["observer"]["type"] = "database";

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
