# scheduler Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: scheduler
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 240
- Actionable Findings (Critical + High): 133
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 28 |
| High | 105 |
| Medium | 107 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 56 |
| container | 52 |
| reliability | 45 |
| concurrency | 20 |
| security | 15 |
| performance | 13 |
| memory | 12 |
| determinism | 6 |
| exception_safety | 6 |
| legacy_duplication | 5 |
| raii | 5 |
| observability | 4 |
| platform | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/scheduler/task_scheduler.cpp | 134 | 24 | 61 | 49 | 0 |
| src/scheduler/event_trigger.cpp | 25 | 3 | 13 | 9 | 0 |
| src/scheduler/task_anomaly_detector.cpp | 23 | 1 | 6 | 16 | 0 |
| src/scheduler/external_scheduler_adapter.cpp | 17 | 0 | 7 | 10 | 0 |
| src/scheduler/task_audit_manager.cpp | 16 | 0 | 0 | 16 | 0 |
| src/scheduler/distributed_task_coordinator.cpp | 12 | 0 | 10 | 2 | 0 |
| src/scheduler/task_result_store.cpp | 7 | 0 | 2 | 5 | 0 |
| src/scheduler/hybrid_retention_manager.cpp | 4 | 0 | 4 | 0 | 0 |
| src/scheduler/task_audit_event.cpp | 2 | 0 | 2 | 0 | 0 |

## Full Scanner Findings

### src/scheduler/task_scheduler.cpp
Total findings: 134

- Line 348: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double deadline_ms = static_cast<double>(task.sla_deadline->count());
- Line 545: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 661: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tasks_.find(task_id);
- Line 765: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second->avg_execution_time_ms = old_stats.avg_execution_time_ms;
- Line 1013: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = dependents.find(cur);
- Line 1058: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task_map[id] = it->second;
- Line 1106: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator dit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto dit = dependents.find(id);
- Line 1119: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator dit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto dit = dependents.find(id);
- Line 1153: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator dit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto dit = dependents.find(id);
- Line 1245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task->avg_execution_time_ms =
- Line 1352: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator dit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto dit = dependents.find(wr.id);
- Line 1638: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (task->consecutive_skips >= config_.aging_threshold) {
- Line 1678: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
- Line 1678: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
- Line 1679: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const int base = static_cast<int>(t->priority);
- Line 1680: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (aging_thr > 0 && t->consecutive_skips >= aging_thr) {
- Line 1708: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator existing may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto existing = running_task_threads_.find(task->id);
- Line 1715: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: running_task_threads_[task->id] = std::move(task_thread);
- Line 1842: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task->avg_execution_time_ms =
- Line 2123: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task_json["trigger_type"] = static_cast<int>(task->trigger_type);
- Line 2125: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task_json["priority"] = static_cast<int>(task->priority);
- Line 2126: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: task_json["trigger_logic"] = static_cast<int>(task->trigger_logic);
- Line 2870: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double sla_ms = static_cast<double>(task.sla_deadline->count());
- Line 2915: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_failure_alert_ids_.find(task_id);
- Line 303: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * Otherwise fall back to a policy derived from the legacy max_retries field.
  Confidence: band=high; score=0.8
- Line 309: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy fallback: exponential backoff with max_retries
  Confidence: band=high; score=0.8
- Line 330: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param delay_ms           [in/out] Computed retry delay – may be clamped.
- Line 331: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param effective_max_retries [in/out] Max attempts – may be clamped.
- Line 433: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TaskScheduler: query_engine cannot be null");
- Line 537: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(25));
  Confidence: band=very_high; score=0.9
- Line 537: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(25));
- Line 571: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 589: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CDC event triggers require a Changefeed instance");
- Line 604: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: task_ptr->id = id;
- Line 604: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: task_ptr->id = id;
- Line 607: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (task_ptr->trigger_type == ScheduledTask::TriggerType::CRON) {
- Line 609: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: updateCronExpression(id, task_ptr->cron_expression);
- Line 614: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: task_ptr->next_run = *next;
- Line 617: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (task_ptr->trigger_type == ScheduledTask::TriggerType::INTERVAL) {
- Line 619: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (task_ptr->next_run == std::chrono::system_clock::time_point{}) {
- Line 620: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: task_ptr->next_run = std::chrono::system_clock::now() + task_ptr->interval;
- Line 622: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: } else if (task_ptr->trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
- Line 627: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: tasks_[id] = task_ptr;
- Line 645: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event.metadata["cron_expression"] = sanitized_task.cron_expression;
- Line 646: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: event.metadata["interval_ms"] = sanitized_task.interval.count();
- Line 1012: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = dependents.find(cur);
- Line 1024: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 1038: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unauthorized: Missing required permission 'task:execute'");
- Line 1055: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 1086: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
- Line 1086: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
  Confidence: band=very_high; score=0.9
- Line 1105: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto dit = dependents.find(id);
- Line 1375: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 1562: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, task] : tasks_) {
  Confidence: band=very_high; score=0.9
- Line 1613: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(tasks_mutex_);
- Line 1678: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
- Line 1678: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto effectivePriority = [aging_thr](const std::shared_ptr<ScheduledTask>& t) -> int {
- Line 1687: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [&effectivePriority](const std::shared_ptr<ScheduledTask>& a,
- Line 1693: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& task : tasks_to_execute) {
  Confidence: band=very_high; score=0.9
- Line 1707: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(running_mutex_);
- Line 1920: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: failure_event.metadata["attempts_made"] = attempts_made;
- Line 2025: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("AQL query failed: " + result.error().message());
- Line 2037: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Function not found: " + name);
- Line 2130: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: cdc_json["key_prefix"] = task->cdc_trigger.key_prefix;
- Line 2131: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cdc_json["event_types"] = nlohmann::json::array();
- Line 2243: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Load trigger configuration (with defaults for backward compatibility)
  Confidence: band=high; score=0.8
- Line 2269: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Restore retry policy (legacy max_retries if no retry_policy block)
  Confidence: band=high; score=0.8
- Line 2363: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQL query cannot be empty");
- Line 2397: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQL query validation failed: " + validation_result.error_message);
- Line 2403: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQL query exceeds maximum length of " +
- Line 2437: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Task timeout exceeds maximum allowed: " +
- Line 2443: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Task timeout is too short. Minimum: " +
- Line 2447: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Validate max_retries (legacy field and retry_policy.max_retries)
  Confidence: band=high; score=0.8
- Line 2525: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Query nesting level exceeds maximum allowed: " +
- Line 2548: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Query contains too many FOR loops. Maximum: " +
- Line 2609: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Cron expression cannot be empty");
- Line 2614: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid cron expression: " + validation.error_message);
- Line 2629: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Failed to parse cron expression: " + expression);
- Line 2647: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CDC trigger key_prefix cannot be empty");
- Line 2651: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CDC trigger must specify at least one event type");
- Line 2657: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid CDC event type: " + std::to_string(type));
- Line 2670: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: config.key_prefix = task->cdc_trigger.key_prefix;
- Line 2673: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int type_int : task->cdc_trigger.event_types) {
  Confidence: band=very_high; score=0.9
- Line 2691: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: task_ptr = it->second;
- Line 2718: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: {"cdc_key_prefix", task->cdc_trigger.key_prefix}
- Line 890: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 978: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<std::string>>& adj) const
  Confidence: band=high; score=0.74
- Line 981: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> in_degree;
  Confidence: band=high; score=0.74
- Line 992: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(id);
  Confidence: band=high; score=0.74
- Line 992: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(id);
  Confidence: band=high; score=0.74
- Line 993: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(id);
- Line 998: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> dependents;
  Confidence: band=high; score=0.74
- Line 1001: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents[dep].push_back(id);
- Line 1011: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(cur);
- Line 1016: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ready.push_back(dependent);
  Confidence: band=high; score=0.74
- Line 1017: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ready.push_back(dependent);
- Line 1049: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::shared_ptr<ScheduledTask>> task_map;
  Confidence: band=high; score=0.74
- Line 1064: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> adj;
  Confidence: band=high; score=0.74
- Line 1068: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[id].push_back(dep);
  Confidence: band=high; score=0.74
- Line 1068: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[id].push_back(dep);
  Confidence: band=high; score=0.74
- Line 1068: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adj[id].push_back(dep);
  Confidence: band=high; score=0.74
- Line 1069: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adj[id].push_back(dep);
- Line 1078: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> dependents;
  Confidence: band=high; score=0.74
- Line 1080: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(id);
  Confidence: band=high; score=0.74
- Line 1080: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dependents[dep].push_back(id);
  Confidence: band=high; score=0.74
- Line 1081: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dependents[dep].push_back(id);
- Line 1086: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
  Confidence: band=high; score=0.74
- Line 1102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.condition_skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.condition_skipped.push_back(id);
- Line 1115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.skipped.push_back(id);
- Line 1140: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, nlohmann::json> dep_results;
  Confidence: band=high; score=0.74
- Line 1148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.condition_skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.condition_skipped.push_back(id);
  Confidence: band=high; score=0.74
- Line 1149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.condition_skipped.push_back(id);
- Line 1161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wave.push_back(id);
  Confidence: band=high; score=0.74
- Line 1162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wave.push_back(id);
- Line 1172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.skipped.push_back(id);
- Line 1192: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &wave_results, i, &task_map]() {
  Confidence: band=high; score=0.74
- Line 1227: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: task_snapshot.push_back(*task);
  Confidence: band=high; score=0.74
- Line 1448: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: task_snapshot.push_back(*task);
- Line 1562: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*task);
  Confidence: band=high; score=0.74
- Line 1563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*task);
- Line 1595: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: params.sort_by = scheduler::AuditQueryParams::SortBy::TIMESTAMP_DESC;
  Confidence: band=high; score=0.74
- Line 1665: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tasks_to_execute.push_back(task);
- Line 1823: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2003: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json TaskScheduler::executeAqlQuery(const std::string& aql) {
  Confidence: band=high; score=0.74
- Line 2132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cdc_json["event_types"].push_back(type);
  Confidence: band=high; score=0.74
- Line 2133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cdc_json["event_types"].push_back(type);
- Line 2185: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 2204: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: af.close();
- Line 2254: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cdc_json = task_json["cdc_trigger"];
  Confidence: band=high; score=0.74
- Line 2361: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void TaskScheduler::validateAqlQuery(const std::string& aql) const {
  Confidence: band=high; score=0.74

### src/scheduler/event_trigger.cpp
Total findings: 25

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 513: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = triggers_.find(id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 97: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EventTrigger: changefeed cannot be null");
- Line 101: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EventTrigger: invalid config - " +
- Line 106: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EventTrigger: callback cannot be null");
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EventTrigger: invalid config - " +
- Line 238: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Poll for new events with long-polling
- Line 270: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(debounce_mutex_);
- Line 298: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lock, std::chrono::milliseconds(100),
  Confidence: band=very_high; score=0.9
- Line 298: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::milliseconds(100),
- Line 307: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::seconds(1),
- Line 450: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t k = 2; k < tokens.size(); ++k) {
  Confidence: band=very_high; score=0.9
- Line 479: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EventTriggerManager: changefeed cannot be null");
- Line 545: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, trigger] : triggers_) {
  Confidence: band=very_high; score=0.9
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raw_clauses.push_back(themis::utils::trim(condition.substr(pos)));
- Line 414: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raw_clauses.push_back(themis::utils::trim(condition.substr(pos, found - pos)));
- Line 430: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(raw.substr(i, j - i + (j < n ? 1 : 0)));
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(raw.substr(i, j - i + (j < n ? 1 : 0)));
- Line 436: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(raw.substr(i, j - i));
- Line 450: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (k > 2) rhs += " ";
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (k > 2) rhs += " ";
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parsed_clauses_.push_back(std::move(pc));
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parsed_clauses_.push_back(std::move(pc));

### src/scheduler/task_anomaly_detector.cpp
Total findings: 23

- Line 319: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return it->second.total_executions >= config_.min_samples;
- Line 334: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 339: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 344: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 349: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 398: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& tp : stats.execution_times) {
  Confidence: band=very_high; score=0.9
- Line 406: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (bool b : stats.execution_results) res_arr.push_back(b);
  Confidence: band=very_high; score=0.9
- Line 90: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) metrics.description += ", ";
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) metrics.description += ", ";
- Line 236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: intervals.push_back(std::chrono::duration<double>(interval).count());
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: intervals.push_back(std::chrono::duration<double>(interval).count());
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (double v : d) arr.push_back(v);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (double v : d) arr.push_back(v);
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ts_arr.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
  Confidence: band=high; score=0.74
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ts_arr.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
  Confidence: band=high; score=0.74
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ts_arr.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (bool b : stats.execution_results) res_arr.push_back(b);
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : arr) d.push_back(v.get<double>());
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_times.push_back(
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_times.push_back(
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.execution_times.push_back(
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_results.push_back(b.get<bool>());
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.execution_results.push_back(b.get<bool>());

### src/scheduler/external_scheduler_adapter.cpp
Total findings: 17

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2646 [scheduler] Integrate Kubernetes CronJob and Airflow external sched... (2026-03-12T05:53
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 216: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 323: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto require = [&](const nlohmann::json& obj, const std::string& key) -> const nlohmann::json& {
- Line 325: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 341: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: task.id           = metadata["name"].get<std::string>();
- Line 379: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 71: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '-';
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '-';
- Line 190: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 192: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': out += "\\'"; break;
- Line 193: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 194: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n"; break;
- Line 195: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r"; break;
- Line 196: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t"; break;
- Line 464: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "        headers={'Content-Type': 'application/json'},\n"

### src/scheduler/task_audit_manager.cpp
Total findings: 16

- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(event);
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(event);
- Line 228: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: case AuditQueryParams::SortBy::TIMESTAMP_ASC:
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: case AuditQueryParams::SortBy::TIMESTAMP_DESC:
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_uuids;
  Confidence: band=medium; score=0.66
- Line 313: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = j["timestamp"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(event));
- Line 375: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(event);
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(event);
- Line 440: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_uuids;
  Confidence: band=medium; score=0.66
- Line 470: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ts_ms = j["timestamp"].get<int64_t>();
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(event));
- Line 517: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 547: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(event.toJson(config_.enable_gdpr_mode));
  Confidence: band=high; score=0.74
- Line 548: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j.push_back(event.toJson(config_.enable_gdpr_mode));

### src/scheduler/distributed_task_coordinator.cpp
Total findings: 12

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3364 [scheduler] Distributed cron leader election â€“ one runner per clu... (2026-03-12T07:07
- Line 29: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedTaskCoordinator: scheduler cannot be null");
- Line 32: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedTaskCoordinator: coordinator cannot be null");
- Line 48: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedTaskCoordinator: scheduler cannot be null");
- Line 51: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedTaskCoordinator: coordinator cannot be null");
- Line 97: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: coordinator_->setLeaderElectedCallback(nullptr);
- Line 97: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: coordinator_->setLeaderElectedCallback(nullptr);
- Line 263: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, task] : task_registry_) {
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: s.leadership_acquired = leadership_acquired_.load();
- Line 314: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: leadership_acquired_.fetch_add(1);
- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(task);
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(task);

### src/scheduler/task_result_store.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 52: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Build a zero-padded 20-digit decimal timestamp so keys sort chronologically.
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: size_t to_delete = all_keys.size() - max_per_task_;
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.emplace_back(std::string(k), std::string(v));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(TaskExecutionResult::fromJson(j));
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(TaskExecutionResult::fromJson(j));

### src/scheduler/hybrid_retention_manager.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 35: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridRetentionManager: query_engine cannot be null");
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridRetentionManager: tsstore cannot be null");
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridRetentionManager: scheduler cannot be null");

### src/scheduler/task_audit_event.cpp
Total findings: 2

- Line 187: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metadata"] = metadata;
- Line 267: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ecs["observer"]["type"] = "database";

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
