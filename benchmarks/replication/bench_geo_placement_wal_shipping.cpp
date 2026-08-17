// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_geo_placement_wal_shipping.cpp
 * @brief Wave D Phase 1 release-gate benchmarks for geographic placement
 *        and cross-region WAL shipping.
 *
 * ## Benchmark families
 *
 * ### GEO-BENCH-01 — Leader election under placement constraints (3-DC topology)
 *   Gate: deterministic, ≤ 50 ms per election
 *   Workload: 100 elections in 3-DC cluster with placement constraints
 *
 * ### WAL-BENCH-01 — WAL segment shipping throughput
 *   Gate: ≥ 80 MB/s on simulated GbE link
 *   Workload: 10,000 × 1 KB segments enqueued and shipped
 *
 * ### WAL-BENCH-02 — Lag alert latency under backpressure
 *   Gate: Alert fires within 2× max_lag_ms window (default 2s)
 *   Workload: Segment delayed to exceed max_lag_ms, alert fires
 *
 * @see include/replication/geo_placement.h
 * @see include/replication/async_wal_shipper.h
 * @see src/replication/ROADMAP.md — §3.1 Wave D Phase 1
 */

#include <benchmark/benchmark.h>

#include "replication/async_wal_shipper.h"
#include "replication/geo_placement.h"
#include "replication/replication_manager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace bench {
namespace wave_d {

using namespace themisdb::replication;
using namespace std::chrono_literals;

// ===========================================================================
// Constants
// ===========================================================================

/// Canonical PRNG seed for all Wave D benchmarks.
static constexpr uint64_t kWaveDCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kWarmupIterations = 100;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

/// GbE link bandwidth capacity (theoretical).
static constexpr double kGbEBandwidthMBps = 125.0;  // 1 Gbps / 8 bits-per-byte

/// Acceptance threshold for WAL shipping: ≥80 MB/s.
static constexpr double kWalShippingThresholdMBps = 80.0;

// ===========================================================================
// Geo Placement Helpers
// ===========================================================================

namespace {

/// Build a healthy voting replica in the given datacenter.
ReplicaInfo makeReplica(
    const std::string& node_id,
    const std::string& datacenter,
    int32_t            priority = 1,
    uint64_t           seq      = 0,
    HealthStatus       health   = HealthStatus::HEALTHY)
{
    ReplicaInfo r;
    r.node_id              = node_id;
    r.datacenter           = datacenter;
    r.priority             = priority;
    r.last_applied_sequence = seq;
    r.health_status        = health;
    r.is_voting_member     = true;
    r.role                 = ReplicationRole::FOLLOWER;
    r.last_heartbeat       = std::chrono::system_clock::now();
    return r;
}

/// Build a standard 3-DC topology for testing.
std::vector<ReplicaInfo> make3DCTopology() {
    return {
        makeReplica("node-eu-1", "eu-west-1", 3, 1000),
        makeReplica("node-eu-2", "eu-west-1", 2, 950),
        makeReplica("node-us-1", "us-east-1", 2, 900),
        makeReplica("node-us-2", "us-east-1", 1, 850),
        makeReplica("node-ap-1", "ap-southeast-1", 1, 800),
    };
}

/// Build a WAL segment for shipping.
WalSegment makeSegment(uint64_t seq, size_t data_size = 1024,
                       const std::string& target_dc = "dc-eu")
{
    WalSegment s;
    s.sequence_number = seq;
    s.data            = std::string(data_size, 'X');  // Fill with data
    s.enqueue_time    = std::chrono::steady_clock::now();
    s.target_dc       = target_dc;
    return s;
}

} // anonymous namespace

// ===========================================================================
// GEO-BENCH-01: Leader Election Under Placement Constraints
// ===========================================================================

void GEO_BENCH_01_LeaderElectionWithConstraints(benchmark::State& state) {
    GeoReplicaPlacementManager placement_mgr;
    const auto                  replicas = make3DCTopology();

    PlacementConstraints constraints;
    constraints.preferred_datacenters = {"eu-west-1", "us-east-1"};
    constraints.require_voter         = true;
    constraints.healthy_only          = true;

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            placement_mgr.selectLeaderCandidate(replicas, constraints));
    }

    state.SetLabel("3-DC topology, preferred DC constraint");
}

BENCHMARK(GEO_BENCH_01_LeaderElectionWithConstraints)
    ->Iterations(100)
    ->Name("GEO-BENCH-01: Leader Election (3-DC)")
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// GEO-BENCH-02: Failover Candidate Selection with DC Constraint
// ===========================================================================

void GEO_BENCH_02_FailoverWithConstraint(benchmark::State& state) {
    GeoReplicaPlacementManager placement_mgr;
    const auto                  replicas = make3DCTopology();

    PlacementConstraints constraints;
    constraints.preferred_datacenters = {"us-east-1"};  // Failover to US region
    constraints.require_voter         = true;
    constraints.healthy_only          = true;

    for (auto _ : state) {
        benchmark::DoNotOptimize(
            placement_mgr.selectFailoverCandidate(replicas, constraints, "node-eu-1"));
    }

    state.SetLabel("Failover selection with DC preference");
}

BENCHMARK(GEO_BENCH_02_FailoverWithConstraint)
    ->Iterations(100)
    ->Name("GEO-BENCH-02: Failover (3-DC)")
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// GEO-BENCH-03: Placement Validation in 3-DC Topology
// ===========================================================================

void GEO_BENCH_03_PlacementValidation(benchmark::State& state) {
    GeoReplicaPlacementManager placement_mgr;
    const auto                  replicas = make3DCTopology();

    PlacementConstraints constraints;
    constraints.required_datacenters = {"eu-west-1", "us-east-1", "ap-southeast-1"};
    constraints.min_copies_per_dc    = 1;
    constraints.require_voter        = true;

    for (auto _ : state) {
        benchmark::DoNotOptimize(placement_mgr.validatePlacement(replicas, constraints));
    }

    state.SetLabel("Validation: 3 required DCs, min 1 copy each");
}

BENCHMARK(GEO_BENCH_03_PlacementValidation)
    ->Iterations(100)
    ->Name("GEO-BENCH-03: Placement Validation (3-DC)")
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// WAL-BENCH-01: WAL Segment Shipping Throughput
// ===========================================================================

void WAL_BENCH_01_ShippingThroughput(benchmark::State& state) {
    WalShippingConfig cfg;
    cfg.local_dc_id        = "dc-us-east";
    cfg.remote_dc_endpoint = "dc-eu-west:9876";
    cfg.max_lag_ms         = 1000;
    cfg.max_queue_depth    = 10000;

    AsyncWalShipper shipper(cfg);

    // Pre-allocate a 1 KB payload to avoid allocation overhead in the loop.
    const std::string payload(1024, 'X');

    // Track shipped bytes outside the benchmark loop for result reporting.
    uint64_t total_bytes_shipped = 0;

    for (auto _ : state) {
        WalSegment seg = makeSegment(state.iterations(), 1024);
        shipper.enqueueSegment(std::move(seg));
        total_bytes_shipped += 1024;
    }

    // Allow time for all segments to drain before stopping.
    std::this_thread::sleep_for(500ms);
    shipper.stop();

    // Calculate throughput
    const auto s = shipper.stats();
    if (s.segments_shipped > 0) {
        const double throughput_mbps = static_cast<double>(s.bytes_shipped) / (1024 * 1024);
        state.counters["throughput_MB/s"] = throughput_mbps;
        state.counters["segments_shipped"] = s.segments_shipped;

        // Check acceptance criteria
        if (throughput_mbps >= kWalShippingThresholdMBps) {
            state.SetLabel(
                std::string("PASS: ") + std::to_string(static_cast<int>(throughput_mbps)) +
                " MB/s >= " + std::to_string(static_cast<int>(kWalShippingThresholdMBps)) +
                " MB/s threshold");
        } else {
            state.SetLabel(
                std::string("FAIL: ") + std::to_string(static_cast<int>(throughput_mbps)) +
                " MB/s < " + std::to_string(static_cast<int>(kWalShippingThresholdMBps)) +
                " MB/s threshold");
        }
    }
}

BENCHMARK(WAL_BENCH_01_ShippingThroughput)
    ->Iterations(10000)
    ->Name("WAL-BENCH-01: Shipping Throughput (1 KB segs)")
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// WAL-BENCH-02: Lag Alert Latency Under Backpressure
// ===========================================================================

void WAL_BENCH_02_LagAlertLatency(benchmark::State& state) {
    WalShippingConfig cfg;
    cfg.local_dc_id        = "dc-us-east";
    cfg.remote_dc_endpoint = "dc-eu-west:9876";
    cfg.max_lag_ms         = 100;  // Very tight lag limit to trigger alerts
    cfg.max_queue_depth    = 1000;

    AsyncWalShipper shipper(cfg);

    std::atomic<uint64_t> alert_count{0};
    std::atomic<uint64_t> min_alert_lag_ms{UINT64_MAX};
    std::atomic<uint64_t> max_alert_lag_ms{0};

    shipper.setAlertCallback([&](uint64_t lag_ms) {
        ++alert_count;
        if (lag_ms < min_alert_lag_ms.load()) {
            min_alert_lag_ms.store(lag_ms);
        }
        if (lag_ms > max_alert_lag_ms.load()) {
            max_alert_lag_ms.store(lag_ms);
        }
    });

    for (auto _ : state) {
        // Create a segment with enqueue_time in the past to trigger lag alerts.
        WalSegment seg = makeSegment(state.iterations(), 512);
        seg.enqueue_time = std::chrono::steady_clock::now() - 500ms;
        shipper.enqueueSegment(std::move(seg));
    }

    // Allow time for all segments to dispatch and alerts to fire.
    std::this_thread::sleep_for(200ms);
    shipper.stop();

    state.counters["alerts_fired"] = alert_count.load();
    if (min_alert_lag_ms.load() != UINT64_MAX) {
        state.counters["min_alert_lag_ms"] = min_alert_lag_ms.load();
        state.counters["max_alert_lag_ms"] = max_alert_lag_ms.load();

        const uint64_t max_window_ms = cfg.max_lag_ms * 2;
        if (max_alert_lag_ms.load() <= max_window_ms) {
            state.SetLabel(
                "PASS: Max lag alert " + std::to_string(max_alert_lag_ms.load()) +
                " ms <= window " + std::to_string(max_window_ms) + " ms");
        } else {
            state.SetLabel(
                "FAIL: Max lag alert " + std::to_string(max_alert_lag_ms.load()) +
                " ms > window " + std::to_string(max_window_ms) + " ms");
        }
    }
}

BENCHMARK(WAL_BENCH_02_LagAlertLatency)
    ->Iterations(500)
    ->Name("WAL-BENCH-02: Lag Alert Latency (backpressure)")
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// WAL-BENCH-03: Stats Accuracy Under Load
// ===========================================================================

void WAL_BENCH_03_StatsAccuracy(benchmark::State& state) {
    WalShippingConfig cfg;
    cfg.local_dc_id        = "dc-us-east";
    cfg.remote_dc_endpoint = "dc-eu-west:9876";
    cfg.max_lag_ms         = 1000;
    cfg.max_queue_depth    = 10000;

    AsyncWalShipper shipper(cfg);

    const size_t segments_to_ship = 1000;
    const size_t payload_size     = 1024;

    for (auto _ : state) {
        shipper.enqueueSegment(makeSegment(state.iterations(), payload_size));
    }

    std::this_thread::sleep_for(200ms);
    shipper.stop();

    const auto s = shipper.stats();
    state.counters["segments_enqueued"] = s.segments_enqueued;
    state.counters["segments_shipped"]  = s.segments_shipped;
    state.counters["segments_dropped"]  = s.segments_dropped;
    state.counters["bytes_enqueued"]    = s.bytes_enqueued;
    state.counters["bytes_shipped"]     = s.bytes_shipped;

    state.SetLabel(
        "Enqueued: " + std::to_string(s.segments_enqueued) +
        " | Shipped: " + std::to_string(s.segments_shipped) +
        " | Dropped: " + std::to_string(s.segments_dropped));
}

BENCHMARK(WAL_BENCH_03_StatsAccuracy)
    ->Iterations(1000)
    ->Name("WAL-BENCH-03: Stats Accuracy (load test)")
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// WAL-BENCH-04: Prometheus Metrics Export Performance
// ===========================================================================

void WAL_BENCH_04_MetricsExport(benchmark::State& state) {
    WalShippingConfig cfg;
    cfg.local_dc_id        = "dc-us-east";
    cfg.remote_dc_endpoint = "dc-eu-west:9876";
    cfg.max_lag_ms         = 1000;
    cfg.max_queue_depth    = 10000;

    AsyncWalShipper shipper(cfg);

    // Prime the shipper with some data
    for (int i = 0; i < 100; ++i) {
        shipper.enqueueSegment(makeSegment(i, 512));
    }
    std::this_thread::sleep_for(100ms);

    for (auto _ : state) {
        benchmark::DoNotOptimize(shipper.exportPrometheusMetrics());
    }

    shipper.stop();

    state.SetLabel("Metrics export under normal load");
}

BENCHMARK(WAL_BENCH_04_MetricsExport)
    ->Iterations(1000)
    ->Name("WAL-BENCH-04: Prometheus Export")
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace wave_d
} // namespace bench
} // namespace themis
