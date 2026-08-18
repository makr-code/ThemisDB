# ThemisDB Replication Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The replication module provides leader-follower and multi-writer replication behavior, WAL shipping, failover/promotion paths, conflict resolution, logical replication slots, and replication observability surfaces for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| replication_manager.cpp | replication orchestration, init, promote, runtime control |
| raft_v2.cpp | Raft v2 state and membership behavior |
| logical_replication.cpp | logical slot/filter and stream behavior |
| multi_tier_replication.cpp | tiered replication path management |
| conflict_resolution.cpp | HLC/LWW/CRDT conflict resolution behavior |
| event_stream.cpp | replication event subscription/dispatch behavior |
| replication_slot.cpp | slot lifecycle and persistence behavior |
| schema_cdc.cpp | schema-aware CDC bridge behavior |
| observability.cpp | lag/topology/health snapshots and metrics |
| policy.cpp | replication policy validation and assignment |

## Scope

In scope:
- replication lifecycle, failover, and leader promotion behaviors
- WAL/logical replication and conflict resolution surfaces
- replication observability, policy, and CDC integration paths

Out of scope:
- transport protocol ownership outside replication module contracts
- storage engine internals owned by storage subsystem
- authn/authz ownership outside module boundaries

## Runtime Behavior and Limits

- replication mode and topology drive consistency/performance behavior.
- promotion/failover paths are explicit and diagnosable.
- conflict resolution paths are deterministic per configured strategy.
- lag and health behavior remains observable via module metrics snapshots.

## Sourcecode Verification (Module: replication/readme)

- Verified files:
  - src/replication/replication_manager.cpp
  - src/replication/raft_v2.cpp
  - src/replication/logical_replication.cpp
  - src/replication/multi_tier_replication.cpp
  - src/replication/conflict_resolution.cpp
  - src/replication/event_stream.cpp
  - src/replication/replication_slot.cpp
  - src/replication/schema_cdc.cpp
  - src/replication/observability.cpp
  - src/replication/policy.cpp
- Verified behavior surfaces:
  - orchestration/failover, conflict resolution, CDC/logical replication, observability
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
## Configuration Keys

### Async Cross-Region WAL Shipping (Wave A Block 2)

Configuration for async WAL shipping to remote datacenters:

```
replication:
  wal_shipping:
    # Maximum acceptable WAL shipping lag in milliseconds
    # When the shipping lag exceeds this value, a lag alert is emitted
    # Default: 1000 (1 second)
    max_lag_ms: 1000

    # Number of histogram buckets for Prometheus lag metric
    # Default: 16
    histogram_buckets: 16

    # Remote datacenter endpoint (host:port or symbolic name)
    remote_dc_endpoint: "dc-eu-west:9876"

    # Local datacenter identifier
    local_dc_id: "dc-us-east"

    # Maximum number of segments that may queue before backpressure
    # Default: 4096
    max_queue_depth: 4096
```

### Lag Alert Manager Configuration

Configuration for replication lag monitoring and SLO thresholds:

```
replication:
  lag_alert:
    # Alert threshold (milliseconds): Initial warning-level notification
    # Default: 10000 (10 seconds)
    alert_threshold_ms: 10000

    # Critical threshold (milliseconds): Escalated error-level notification
    # Default: 30000 (30 seconds)
    critical_threshold_ms: 30000

    # Failover threshold (milliseconds): Consider automatic failover
    # Default: 60000 (60 seconds)
    failover_threshold_ms: 60000

    # Failover duration (milliseconds): How long critical lag must persist
    # before triggering automatic failover
    # Default: 300000 (5 minutes)
    failover_duration_ms: 300000
```

## Prometheus Metrics

### WAL Shipping Metrics

- `replication_wal_lag_ms` — Histogram of WAL shipping lag from enqueue to dispatch (ms)
  - Buckets: Geometric series from 1 ms to 10× max_lag_ms
  - Labels: `local_dc`, `remote_dc`
  
- `replication_wal_segments_enqueued_total` — Total WAL segments accepted (counter)
- `replication_wal_segments_shipped_total` — Total WAL segments dispatched (counter)
- `replication_wal_segments_dropped_total` — WAL segments dropped due to queue full (counter)
- `replication_wal_lag_alerts_total` — Times WAL lag exceeded configured limit (counter)
- `replication_wal_bytes_shipped_total` — Total bytes dispatched (counter)

### Lag Alert Manager Metrics

- `replication_lag_ms{replica_id}` — Current replication lag per replica (gauge)
- `lag_alert_triggered_total{replica_id}` — Total alert threshold crossings (counter)
- `lag_critical_triggered_total{replica_id}` — Total critical threshold crossings (counter)
- `failover_initiated_total{replica_id}` — Total failover initiations due to lag (counter)

## API Usage

### Async WAL Shipper

```cpp
#include "replication/async_wal_shipper.h"

// Configure
WalShippingConfig cfg;
cfg.remote_dc_endpoint = "dc-eu-west:9876";
cfg.local_dc_id        = "dc-us-east";
cfg.max_lag_ms         = 1000;

// Create shipper (starts background worker thread)
AsyncWalShipper shipper(cfg);

// Set up handlers
shipper.setAlertCallback([](uint64_t lag_ms) {
    std::cerr << "WAL lag alert: " << lag_ms << " ms\n";
});

shipper.setShipHandler([](const WalSegment& seg) -> bool {
    // Implement actual transport (gRPC, TCP, cloud storage, etc.)
    return transportToRemoteDC(seg);
});

// Enqueue segments for async shipping
WalSegment seg;
seg.sequence_number = 1;
seg.data            = wal_bytes;
seg.enqueue_time    = std::chrono::steady_clock::now();
seg.target_dc       = "dc-eu-west";

if (!shipper.enqueueSegment(std::move(seg))) {
    // Queue full; implement backpressure strategy
}

// Export metrics
std::cout << shipper.exportPrometheusMetrics();
```

### Lag Alert Manager

```cpp
#include "replication/lag_alert_manager.h"

// Create manager
LagAlertManager lag_monitor;

// Configure SLO thresholds
SLOThresholds thresholds;
thresholds.alert_threshold_ms = 5000;      // 5 seconds
thresholds.critical_threshold_ms = 15000;  // 15 seconds
lag_monitor.setThresholds(thresholds);

// Set up alert callback
lag_monitor.setAlertCallback([](const AlertEvent& evt) {
    std::cerr << "LAG ALERT: replica=" << evt.replica_id 
              << " lag=" << evt.lag_ms << "ms level=" << (int)evt.level << "\n";
});

// Periodically update replica lag
lag_monitor.updateReplicaLag("replica-1", 5500);
lag_monitor.updateReplicaLag("replica-2", 12000);

// Check and emit alerts
if (lag_monitor.checkAndAlertLagViolations()) {
    // At least one alert was fired
}

// Query status
auto in_critical = lag_monitor.replicasInCritical();
auto failover_eligible = lag_monitor.replicasEligibleForFailover();
```

## Testing

Comprehensive test suite for async WAL shipping and lag alerts:

`tests/replication/test_replication_async_wal_lag_alerts.cpp`

Test coverage includes:
- Basic enqueue and dispatch
- Lag alert threshold crossing
- Backpressure handling when queue full
- Transport handler failures and recovery
- Concurrent multi-threaded shipping
- Prometheus metrics export validation
- Single and multiple replica lag tracking
- Critical lag detection and failover eligibility
- Batch lag updates
- Callback exception handling
- End-to-end integration

See ROADMAP.md §3.1 for acceptance criteria and implementation evidence.
