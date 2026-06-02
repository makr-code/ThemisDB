# failover Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: failover
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 7
- Actionable Findings (Critical + High): 5
- Affected Files: 2

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 3 |
| Medium | 2 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 7 |
| distributed_consistency | 4 |
| concurrency | 3 |
| performance_patterns | 3 |
| exception_safety | 2 |
| performance | 2 |
| memory | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/failover/auto_failover_manager.cpp | 7 | 2 | 3 | 2 | 0 |
| src/failover/disaster_recovery_manager.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/failover/auto_failover_manager.cpp
Total findings: 7

- Line 114: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: static_cast<float>(config_.max_concurrent_failovers);
  Confidence: band=very_high; score=0.99
- Line 132: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: "/" + std::to_string(config_.max_concurrent_failovers);
  Confidence: band=very_high; score=0.99
- Line 47: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: monitoring_thread_ = std::thread(&AutoFailoverManager::monitoringLoop, this);
  Confidence: band=very_high; score=0.9
- Line 48: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: failover_thread_ = std::thread(&AutoFailoverManager::failoverLoop, this);
  Confidence: band=very_high; score=0.9
- Line 550: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint32_t attempt = 0; attempt < config_.max_recovery_attempts; ++attempt) {
  Confidence: band=very_high; score=0.9
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failing_nodes.push_back(node_id);
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failover_durations_.push_back(result.duration);
  Confidence: band=high; score=0.74

### src/failover/disaster_recovery_manager.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
