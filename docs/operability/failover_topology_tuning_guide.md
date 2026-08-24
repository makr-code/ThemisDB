# Failover Topology Tuning Guide

**Module:** failover  
**Wave:** D (Operability Hardening)  
**Last Updated:** 2026-08-24  
**Scope:** AutoFailoverManager configuration tuning for different cluster topologies

---

## 1. Overview

This guide provides recommended `AutoFailoverConfig` and `DisasterRecoveryConfig`
parameter settings for 3-node, 5-node, and 9-node cluster topologies. All values
are derived from the Wave B hardening work and the release-gate benchmarks
(FRG-01..FRG-06, FWB-01..FWB-08).

---

## 2. Cluster Topology Comparison

| Parameter | 3-Node Cluster | 5-Node Cluster | 9-Node Cluster |
|---|---|---|---|
| Quorum size | 2 (floor(3/2)+1) | 3 | 5 |
| Max tolerated failures | 1 node | 2 nodes | 4 nodes |
| Election timeout budget | 5 s | 5 s | 8 s (more election rounds) |
| Heartbeat interval | 500 ms | 500 ms | 500 ms |
| Health-check interval | 500 ms | 500 ms | 750 ms |
| Adaptive interval | Recommended | Recommended | Required |
| GC grace count | 3 | 3 | 5 |
| GC grace window | 1 s | 1 s | 2 s |
| Quorum timeout | 10 s | 15 s | 30 s |
| Max concurrent failovers | 1 | 2 | 4 |
| Consecutive failures before action | 3 | 3 | 5 |

---

## 3. 3-Node Cluster Configuration

### Characteristics
- Minimum recommended cluster size
- Single-node failure tolerance
- Lowest latency, smallest quorum requirement

### Recommended `AutoFailoverConfig`

```cpp
AutoFailoverConfig cfg;
// Timing
cfg.health_check_interval             = std::chrono::milliseconds(500);
cfg.health_check_call_timeout_ms      = std::chrono::milliseconds(5000);
cfg.failover_timeout                  = std::chrono::milliseconds(15000);  // 15 s
cfg.leader_election_timeout           = std::chrono::milliseconds(5000);   // 5 s
cfg.recovery_retry_interval           = std::chrono::milliseconds(3000);

// Thresholds
cfg.consecutive_failures_before_action = 3;
cfg.max_concurrent_failovers           = 1;
cfg.queue_pressure_threshold           = 0.75f;

// Quorum
cfg.quorum_timeout_ms                 = std::chrono::milliseconds(10000);  // 10 s

// Adaptive health-check
cfg.adaptive_check_interval           = true;
cfg.adaptive_check_samples            = 10;
cfg.adaptive_check_interval_min       = std::chrono::milliseconds(200);
cfg.adaptive_check_interval_max       = std::chrono::milliseconds(2000);

// GC grace
cfg.gc_grace_failure_count            = 3;
cfg.gc_grace_window                   = std::chrono::milliseconds(1000);
cfg.gc_grace_period                   = std::chrono::milliseconds(2000);

// Behavior
cfg.enable_automatic_failover         = true;
cfg.enable_split_brain_prevention     = true;
cfg.enable_automatic_recovery         = true;
cfg.max_recovery_attempts             = 3;
```

### 3-Node Notes
- With only 1 tolerated failure, quorum is lost quickly on the second failure.
  Plan for rapid DR execution if two nodes fail simultaneously.
- `max_concurrent_failovers=1` prevents cascading: if two nodes flap simultaneously,
  only the first failover is processed; the second queues.
- Consider a hot spare with `enable_spare_activation=true` for 3-node setups where
  replacing a failed node quickly is critical.

---

## 4. 5-Node Cluster Configuration

### Characteristics
- Standard production cluster size
- Two-node failure tolerance
- Balanced election speed vs. resilience

### Recommended `AutoFailoverConfig`

```cpp
AutoFailoverConfig cfg;
// Timing
cfg.health_check_interval             = std::chrono::milliseconds(500);
cfg.health_check_call_timeout_ms      = std::chrono::milliseconds(5000);
cfg.failover_timeout                  = std::chrono::milliseconds(20000);  // 20 s
cfg.leader_election_timeout           = std::chrono::milliseconds(5000);
cfg.recovery_retry_interval           = std::chrono::milliseconds(5000);

// Thresholds
cfg.consecutive_failures_before_action = 3;
cfg.max_concurrent_failovers           = 2;
cfg.queue_pressure_threshold           = 0.75f;

// Quorum
cfg.quorum_timeout_ms                 = std::chrono::milliseconds(15000);  // 15 s

// Adaptive health-check
cfg.adaptive_check_interval           = true;
cfg.adaptive_check_samples            = 20;
cfg.adaptive_check_interval_min       = std::chrono::milliseconds(100);
cfg.adaptive_check_interval_max       = std::chrono::milliseconds(3000);

// GC grace
cfg.gc_grace_failure_count            = 3;
cfg.gc_grace_window                   = std::chrono::milliseconds(1000);
cfg.gc_grace_period                   = std::chrono::milliseconds(2000);

// Behavior
cfg.enable_automatic_failover         = true;
cfg.enable_split_brain_prevention     = true;
cfg.enable_automatic_recovery         = true;
cfg.max_recovery_attempts             = 5;
cfg.deterministic_tie_breaking        = true;
```

### 5-Node Notes
- `deterministic_tie_breaking=true` is critical at 5 nodes because split-votes
  (2 vs. 2 with 1 abstention) are more likely than in 3-node clusters.
- `max_concurrent_failovers=2` allows handling two simultaneous node failures
  without losing quorum, as long as the 3 remaining nodes stay healthy.
- Increase `max_recovery_attempts` to 5 to account for longer re-sync time on a
  larger cluster.

---

## 5. 9-Node Cluster Configuration

### Characteristics
- High-availability / hyperscaler topology
- Four-node failure tolerance
- More election rounds → higher election latency

### Recommended `AutoFailoverConfig`

```cpp
AutoFailoverConfig cfg;
// Timing
cfg.health_check_interval             = std::chrono::milliseconds(750);    // Slightly slower
cfg.health_check_call_timeout_ms      = std::chrono::milliseconds(5000);
cfg.failover_timeout                  = std::chrono::milliseconds(30000);  // 30 s
cfg.leader_election_timeout           = std::chrono::milliseconds(8000);   // More rounds
cfg.recovery_retry_interval           = std::chrono::milliseconds(8000);

// Thresholds
cfg.consecutive_failures_before_action = 5;     // Fewer false positives at scale
cfg.max_concurrent_failovers           = 4;
cfg.queue_pressure_threshold           = 0.50f;  // Alert earlier on queue pressure

// Quorum
cfg.quorum_timeout_ms                 = std::chrono::milliseconds(30000);  // 30 s (5-node quorum)

// Adaptive health-check (required at 9 nodes — prevents health-check storms)
cfg.adaptive_check_interval           = true;
cfg.adaptive_check_samples            = 30;
cfg.adaptive_check_interval_min       = std::chrono::milliseconds(200);
cfg.adaptive_check_interval_max       = std::chrono::milliseconds(5000);

// GC grace (looser window for larger JVM/GC pause variance)
cfg.gc_grace_failure_count            = 5;
cfg.gc_grace_window                   = std::chrono::milliseconds(2000);
cfg.gc_grace_period                   = std::chrono::milliseconds(4000);

// Behavior
cfg.enable_automatic_failover         = true;
cfg.enable_split_brain_prevention     = true;
cfg.enable_automatic_recovery         = true;
cfg.max_recovery_attempts             = 8;
cfg.deterministic_tie_breaking        = true;
cfg.max_heartbeats_per_second         = 5;       // Coalescing to reduce network traffic
```

### 9-Node Notes
- **Adaptive health-check is required** at 9 nodes. With 9 health-check calls per
  monitoring cycle, a fixed 500ms interval can create health-check storms under load.
  Adaptive interval automatically backs off under latency spikes.
- **Heartbeat coalescing** (`max_heartbeats_per_second=5`) is strongly recommended.
  At 9 nodes, 9 heartbeats/500ms = 18/s per node; coalescing reduces this to 5/s.
- Increase `consecutive_failures_before_action` to 5 to tolerate more transient
  network jitter in larger clusters without triggering unnecessary failovers.
- `queue_pressure_threshold=0.50f` alerts operators earlier because with 4 concurrent
  failovers possible, the queue can fill quickly in a cascading failure scenario.

---

## 6. Health-Check Frequency vs. Detection Latency Trade-off

| Check Interval | Detection Latency (3 failures × interval) | False Positive Rate | Network Cost |
|---|---|---|---|
| 100 ms | 300 ms | High | High |
| 200 ms | 600 ms | Medium | Medium |
| 500 ms | 1.5 s | Low | Low |
| 1000 ms | 3.0 s | Very Low | Very Low |

**Recommendation:** Use 500ms as the base interval with adaptive enabled.  
The adaptive algorithm will automatically increase to 1-2s during high-latency
periods and decrease to 200ms when the cluster is healthy and responsive.

---

## 7. Quorum Threshold Tuning Matrix

| Cluster Size | Quorum Size | `quorum_timeout_ms` | Notes |
|---|---|---|---|
| 3 | 2 | 10 s | Tight — lose quorum on single failure |
| 5 | 3 | 15 s | Standard production |
| 7 | 4 | 20 s | Good resilience |
| 9 | 5 | 30 s | High resilience; longer election rounds |
| 11 | 6 | 45 s | Very high resilience; consider geo-distribution |

---

## 8. Adaptive Interval Tuning

The adaptive interval algorithm computes p95 latency from the last N health-check
calls and sets the interval to `p95 * 2`, clamped to `[min, max]`.

### Parameter guidance

| Parameter | Conservative | Balanced (Default) | Aggressive |
|---|---|---|---|
| `adaptive_check_samples` | 30 | 20 | 10 |
| `adaptive_check_interval_min` | 300 ms | 100 ms | 50 ms |
| `adaptive_check_interval_max` | 5000 ms | 3000 ms | 1000 ms |

**Conservative:** Slower to respond to changes; fewer false positives;  
**Aggressive:** Faster detection; more CPU/network overhead.

---

## 9. Related Resources

- `include/failover/auto_failover_manager.h` — Full config struct with inline docs
- `include/failover/failover_api_contract.h` — Timing constants and quorum formula
- `benchmarks/failover/bench_failover_wave_b_gates.cpp` — FWB-01..08 performance gates
- `docs/operability/failover_runbook_split_brain.md` — Split-brain response
- `docs/operability/failover_runbook_manual_recovery.md` — Manual recovery procedure
- `src/failover/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` — Wave A implementation evidence
