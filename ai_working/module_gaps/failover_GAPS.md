# failover Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: failover
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 27
- Actionable Findings (Critical + High): 23
- Affected Files: 2

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 4 |
| High | 19 |
| Medium | 4 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 8 |
| container | 7 |
| distributed_consistency | 4 |
| concurrency | 3 |
| exception_safety | 2 |
| performance | 2 |
| memory | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/failover/auto_failover_manager.cpp | 24 | 3 | 17 | 4 | 0 |
| src/failover/disaster_recovery_manager.cpp | 3 | 1 | 2 | 0 | 0 |

## Full Scanner Findings

### src/failover/auto_failover_manager.cpp
Total findings: 24

- Line 113: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: static_cast<float>(config_.max_concurrent_failovers);
  Confidence: band=very_high; score=0.99
- Line 131: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: "/" + std::to_string(config_.max_concurrent_failovers);
  Confidence: band=very_high; score=0.99
- Line 520: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto epoch_token = fencing_manager_->bumpEpoch(
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 46: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitoring_thread_ = std::thread(&AutoFailoverManager::monitoringLoop, this);
  Confidence: band=very_high; score=0.9
- Line 47: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: failover_thread_ = std::thread(&AutoFailoverManager::failoverLoop, this);
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [node_id, failures] : consecutive_failures_) {
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(monitor_mutex_);
- Line 215: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [node_id, is_healthy] : cluster) {
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& node_id : failing_nodes) {
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(failover_mutex_);
- Line 270: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: failover_cv_.wait_for(lock, std::chrono::seconds(1), [this] {
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: failover_cv_.wait_for(lock, std::chrono::seconds(1), [this] {
- Line 308: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(5));  // Brief delay before recovery attempt
- Line 415: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 502: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: spdlog::info("Updating metadata: {} -> {}", old_leader_id, new_leader_id);
- Line 549: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint32_t attempt = 0; attempt < config_.max_recovery_attempts; ++attempt) {
  Confidence: band=very_high; score=0.9
- Line 551: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> stats_lock(stats_mutex_);
- Line 591: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failing_nodes.push_back(node_id);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failing_nodes.push_back(node_id);
- Line 633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failover_durations_.push_back(result.duration);
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failover_durations_.push_back(result.duration);

### src/failover/disaster_recovery_manager.cpp
Total findings: 3

- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto token = fencing_mgr_->bumpEpoch("disaster-recovery: " + plan.plan_id);
- Line 300: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 342: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(
  Confidence: band=very_high; score=0.9

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
