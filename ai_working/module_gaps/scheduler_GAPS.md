# scheduler Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: scheduler
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 57
- Actionable Findings (Critical + High): 2
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 2 |
| Medium | 55 |
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
| src/scheduler/task_scheduler.cpp | 31 | 0 | 2 | 29 | 0 |
| src/scheduler/task_audit_manager.cpp | 9 | 0 | 0 | 9 | 0 |
| src/scheduler/task_anomaly_detector.cpp | 8 | 0 | 0 | 8 | 0 |
| src/scheduler/event_trigger.cpp | 3 | 0 | 0 | 3 | 0 |
| src/scheduler/task_result_store.cpp | 3 | 0 | 0 | 3 | 0 |
| src/scheduler/external_scheduler_adapter.cpp | 2 | 0 | 0 | 2 | 0 |
| src/scheduler/distributed_task_coordinator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/scheduler/hybrid_retention_manager.cpp | 0 | 0 | 0 | 0 | 0 |
| src/scheduler/task_audit_event.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/scheduler/task_scheduler.cpp
Total findings: 31

- Line 1084: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: std::set<std::string> failed_or_skipped;    // Tasks we should NOT execute (dep failure)
  Confidence: band=very_high; score=0.9
- Line 2241: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Load trigger configuration (with defaults for backward compatibility)
  Confidence: band=high; score=0.8
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
- Line 2001: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: nlohmann::json TaskScheduler::executeAqlQuery(const std::string& aql) {
  Confidence: band=high; score=0.74
- Line 2130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cdc_json["event_types"].push_back(type);
  Confidence: band=high; score=0.74
- Line 2252: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cdc_json = task_json["cdc_trigger"];
  Confidence: band=high; score=0.74
- Line 2359: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void TaskScheduler::validateAqlQuery(const std::string& aql) const {
  Confidence: band=high; score=0.74

### src/scheduler/task_audit_manager.cpp
Total findings: 9

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
- Line 545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(event.toJson(config_.enable_gdpr_mode));
  Confidence: band=high; score=0.74

### src/scheduler/task_anomaly_detector.cpp
Total findings: 8

- Line 88: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) metrics.description += ", ";
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: intervals.push_back(std::chrono::duration<double>(interval).count());
  Confidence: band=high; score=0.74
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
- Line 478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_times.push_back(
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_times.push_back(
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.execution_results.push_back(b.get<bool>());
  Confidence: band=high; score=0.74

### src/scheduler/event_trigger.cpp
Total findings: 3

- Line 428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(raw.substr(i, j - i + (j < n ? 1 : 0)));
  Confidence: band=high; score=0.74
- Line 448: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (k > 2) rhs += " ";
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parsed_clauses_.push_back(std::move(pc));
  Confidence: band=high; score=0.74

### src/scheduler/task_result_store.cpp
Total findings: 3

- Line 50: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Build a zero-padded 20-digit decimal timestamp so keys sort chronologically.
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.emplace_back(std::string(k), std::string(v));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(TaskExecutionResult::fromJson(j));
  Confidence: band=high; score=0.74

### src/scheduler/external_scheduler_adapter.cpp
Total findings: 2

- Line 69: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '-';
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74

### src/scheduler/distributed_task_coordinator.cpp
Total findings: 1

- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(task);
  Confidence: band=high; score=0.74

### src/scheduler/hybrid_retention_manager.cpp
Total findings: 0


### src/scheduler/task_audit_event.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
