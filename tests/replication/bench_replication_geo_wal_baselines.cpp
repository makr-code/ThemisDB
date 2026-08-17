// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_replication_geo_wal_baselines.cpp
 * @brief p95/p99 latency benchmarks for geo-placement and WAL shipping
 *        (Wave A gate evidence — representative-hardware baseline).
 *
 * @note CTest labels: replication;benchmark;geo;wal;p95_p99_baseline
 *
 * ## Benchmark families
 *
 * ### BENCH-GEO — Geo-placement p95/p99 latency
 *
 * | ID          | Description                                                 |
 * |-------------|-------------------------------------------------------------|
 * | BENCH-GEO-01 | selectLeaderCandidate — 4-node topology, p95/p99 latency   |
 * | BENCH-GEO-02 | selectLeaderCandidate — 32-node topology, p95/p99 latency  |
 * | BENCH-GEO-03 | selectFailoverCandidate — 32-node topology, p95/p99        |
 * | BENCH-GEO-04 | validatePlacement — 32-node topology, p95/p99              |
 *
 * ### BENCH-WAL — WAL shipper enqueue p95/p99 latency
 *
 * | ID          | Description                                                 |
 * |-------------|-------------------------------------------------------------|
 * | BENCH-WAL-01 | enqueueSegment — 1 KB payload, p95/p99 latency             |
 * | BENCH-WAL-02 | enqueueSegment — 64 KB payload, p95/p99 latency            |
 * | BENCH-WAL-03 | enqueueSegment — 1 000-segment burst, throughput            |
 * | BENCH-WAL-04 | currentLagMs read — under concurrent enqueue load           |
 *
 * ### Acceptance thresholds (Wave A gate criteria)
 * - GEO selectLeaderCandidate p99 < 500 µs for 32-node topology
 * - GEO validatePlacement p99 < 500 µs for 32-node topology
 * - WAL enqueueSegment p99 < 200 µs for 1 KB / 64 KB payloads
 * - WAL 1000-segment burst throughput ≥ 50 000 segments/s
 *
 * ### Execution
 * This file is a standalone benchmark binary (no GTest dependency).
 * It outputs results to stdout in a human-readable table and exits with
 * code 0 on success or 1 if any p99 threshold is exceeded.
 *
 * CMake build target: bench_replication_geo_wal_baselines
 * CTest registration: ReplicationGeoWalBaselineBenchmarks
 */

#include "replication/async_wal_shipper.h"
#include "replication/geo_placement.h"
#include "replication/replication_manager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace themisdb::replication;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;
using Ns    = std::chrono::nanoseconds;

// ===========================================================================
// Measurement utilities
// ===========================================================================

namespace {

struct BenchResult {
    std::string name;
    uint64_t    p50_us;
    uint64_t    p95_us;
    uint64_t    p99_us;
    uint64_t    max_us;
    double      throughput_per_sec;  // 0 if not applicable
    bool        passed;              // meets threshold
};

/// Collect N latency samples, sort, and compute percentiles (in microseconds).
template <typename Fn>
BenchResult measureLatency(const std::string& name,
                           int iterations,
                           uint64_t p99_threshold_us,
                           Fn fn)
{
    std::vector<uint64_t> samples;
    samples.reserve(static_cast<size_t>(iterations));

    for (int i = 0; i < iterations; ++i) {
        const auto t0 = Clock::now();
        fn();
        const auto t1 = Clock::now();
        samples.push_back(static_cast<uint64_t>(
            std::chrono::duration_cast<Ns>(t1 - t0).count() / 1000u));
    }

    std::sort(samples.begin(), samples.end());
    const size_t n = samples.size();

    BenchResult r;
    r.name               = name;
    r.p50_us             = samples[n / 2];
    r.p95_us             = samples[static_cast<size_t>(n * 0.95)];
    r.p99_us             = samples[static_cast<size_t>(n * 0.99)];
    r.max_us             = samples.back();
    r.throughput_per_sec = 0.0;
    r.passed             = (r.p99_us <= p99_threshold_us);
    return r;
}

/// Measure throughput of a burst workload.
template <typename Fn>
BenchResult measureThroughput(const std::string& name,
                               int count,
                               double min_throughput_per_sec,
                               Fn fn)
{
    const auto t0 = Clock::now();
    for (int i = 0; i < count; ++i) fn(i);
    const auto t1 = Clock::now();

    const double elapsed_s = static_cast<double>(
        std::chrono::duration_cast<Ns>(t1 - t0).count()) / 1e9;
    const double tps = static_cast<double>(count) / elapsed_s;

    BenchResult r;
    r.name               = name;
    r.p50_us             = 0;
    r.p95_us             = 0;
    r.p99_us             = 0;
    r.max_us             = 0;
    r.throughput_per_sec = tps;
    r.passed             = (tps >= min_throughput_per_sec);
    return r;
}

void printResult(const BenchResult& r)
{
    if (r.throughput_per_sec > 0.0) {
        std::printf("  %-55s  throughput=%.0f/s  %s\n",
                    r.name.c_str(),
                    r.throughput_per_sec,
                    r.passed ? "PASS" : "FAIL");
    } else {
        std::printf("  %-55s  p50=%4lluµs  p95=%4lluµs  p99=%4lluµs  max=%6lluµs  %s\n",
                    r.name.c_str(),
                    (unsigned long long)r.p50_us,
                    (unsigned long long)r.p95_us,
                    (unsigned long long)r.p99_us,
                    (unsigned long long)r.max_us,
                    r.passed ? "PASS" : "FAIL");
    }
}

/// Build a topology of N replicas spread across num_dcs datacenters.
std::vector<ReplicaInfo> makeTopology(int n, int num_dcs)
{
    std::vector<ReplicaInfo> replicas;
    replicas.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        ReplicaInfo r;
        r.node_id               = "node-" + std::to_string(i);
        r.datacenter            = "dc-" + std::to_string(i % num_dcs);
        r.priority              = (i % 3) + 1;
        r.last_applied_sequence = static_cast<uint64_t>(i * 100);
        r.health_status         = HealthStatus::HEALTHY;
        r.is_voting_member      = true;
        r.role                  = ReplicationRole::FOLLOWER;
        r.last_heartbeat        = std::chrono::system_clock::now();
        replicas.push_back(r);
    }
    return replicas;
}

/// Build constraints that prefer the first DC and forbid none.
PlacementConstraints makeConstraints(int num_dcs)
{
    PlacementConstraints c;
    c.healthy_only = true;
    for (int i = 0; i < num_dcs; ++i)
        c.preferred_datacenters.push_back("dc-" + std::to_string(i));
    return c;
}

} // namespace

// ===========================================================================
// BENCH-GEO-01: selectLeaderCandidate — small topology (4 nodes)
// ===========================================================================

BenchResult benchGeo01()
{
    const auto replicas = makeTopology(4, 2);
    PlacementConstraints c = makeConstraints(2);
    GeoReplicaPlacementManager mgr;

    return measureLatency("BENCH-GEO-01: selectLeaderCandidate (4 nodes)", 2000, 500,
                          [&]() { (void)mgr.selectLeaderCandidate(replicas, c); });
}

// ===========================================================================
// BENCH-GEO-02: selectLeaderCandidate — large topology (32 nodes)
// ===========================================================================

BenchResult benchGeo02()
{
    const auto replicas = makeTopology(32, 4);
    PlacementConstraints c = makeConstraints(4);
    GeoReplicaPlacementManager mgr;

    return measureLatency("BENCH-GEO-02: selectLeaderCandidate (32 nodes)", 2000, 500,
                          [&]() { (void)mgr.selectLeaderCandidate(replicas, c); });
}

// ===========================================================================
// BENCH-GEO-03: selectFailoverCandidate — large topology (32 nodes)
// ===========================================================================

BenchResult benchGeo03()
{
    const auto replicas = makeTopology(32, 4);
    PlacementConstraints c = makeConstraints(4);
    const std::string failed_node = "node-0";
    GeoReplicaPlacementManager mgr;

    return measureLatency("BENCH-GEO-03: selectFailoverCandidate (32 nodes)", 2000, 500,
                          [&]() { (void)mgr.selectFailoverCandidate(replicas, failed_node, c); });
}

// ===========================================================================
// BENCH-GEO-04: validatePlacement — large topology (32 nodes)
// ===========================================================================

BenchResult benchGeo04()
{
    const auto replicas = makeTopology(32, 4);
    PlacementConstraints c = makeConstraints(4);
    c.min_copies_per_dc = 2;
    GeoReplicaPlacementManager mgr;

    return measureLatency("BENCH-GEO-04: validatePlacement (32 nodes)", 2000, 500,
                          [&]() { (void)mgr.validatePlacement(replicas, c); });
}

// ===========================================================================
// BENCH-WAL-01: enqueueSegment — 1 KB payload
// ===========================================================================

BenchResult benchWal01()
{
    std::atomic<int> shipped{0};
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "bench-dc:5432";
    cfg.local_dc_id        = "dc-local";
    cfg.max_lag_ms         = 60000;
    cfg.max_queue_depth    = 1024;
    cfg.transport_handler  = [&](const WalSegment&) { ++shipped; };

    AsyncWalShipper shipper(cfg);
    const std::string payload(1024, 'x');

    int seq = 0;
    auto result = measureLatency("BENCH-WAL-01: enqueueSegment (1 KB payload)", 2000, 200,
                                 [&]() {
                                     WalSegment s;
                                     s.sequence_number = static_cast<uint64_t>(++seq);
                                     s.data            = payload;
                                     s.enqueue_time    = Clock::now();
                                     s.target_dc       = "dc-remote";
                                     (void)shipper.enqueueSegment(std::move(s));
                                 });
    shipper.stop();
    return result;
}

// ===========================================================================
// BENCH-WAL-02: enqueueSegment — 64 KB payload
// ===========================================================================

BenchResult benchWal02()
{
    std::atomic<int> shipped{0};
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "bench-dc:5432";
    cfg.local_dc_id        = "dc-local";
    cfg.max_lag_ms         = 60000;
    cfg.max_queue_depth    = 256;
    cfg.transport_handler  = [&](const WalSegment&) { ++shipped; };

    AsyncWalShipper shipper(cfg);
    const std::string payload(64 * 1024, 'y');

    int seq = 0;
    auto result = measureLatency("BENCH-WAL-02: enqueueSegment (64 KB payload)", 500, 200,
                                 [&]() {
                                     WalSegment s;
                                     s.sequence_number = static_cast<uint64_t>(++seq);
                                     s.data            = payload;
                                     s.enqueue_time    = Clock::now();
                                     s.target_dc       = "dc-remote";
                                     (void)shipper.enqueueSegment(std::move(s));
                                 });
    shipper.stop();
    return result;
}

// ===========================================================================
// BENCH-WAL-03: enqueueSegment — 1 000-segment burst throughput
// ===========================================================================

BenchResult benchWal03()
{
    std::atomic<int> shipped{0};
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "bench-dc:5432";
    cfg.local_dc_id        = "dc-local";
    cfg.max_lag_ms         = 60000;
    cfg.max_queue_depth    = 2048;
    cfg.transport_handler  = [&](const WalSegment&) { ++shipped; };

    AsyncWalShipper shipper(cfg);
    const std::string payload(1024, 'z');

    auto result = measureThroughput(
        "BENCH-WAL-03: enqueueSegment burst (1000 × 1 KB)", 1000, 50000.0,
        [&](int i) {
            WalSegment s;
            s.sequence_number = static_cast<uint64_t>(i);
            s.data            = payload;
            s.enqueue_time    = Clock::now();
            s.target_dc       = "dc-remote";
            (void)shipper.enqueueSegment(std::move(s));
        });

    shipper.stop();
    return result;
}

// ===========================================================================
// BENCH-WAL-04: currentLagMs read under concurrent enqueue load
// ===========================================================================

BenchResult benchWal04()
{
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "bench-dc:5432";
    cfg.local_dc_id        = "dc-local";
    cfg.max_lag_ms         = 60000;
    cfg.max_queue_depth    = 1024;
    cfg.transport_handler  = [](const WalSegment&) {
        std::this_thread::sleep_for(1ms);
    };

    AsyncWalShipper shipper(cfg);

    // Background enqueue thread keeps the queue populated.
    std::atomic<bool> stop_enqueue{false};
    std::thread enqueuer([&]() {
        int seq = 0;
        while (!stop_enqueue.load()) {
            WalSegment s;
            s.sequence_number = static_cast<uint64_t>(++seq);
            s.data            = "bg";
            s.enqueue_time    = Clock::now();
            s.target_dc       = "dc-remote";
            (void)shipper.enqueueSegment(std::move(s));
        }
    });

    auto result = measureLatency(
        "BENCH-WAL-04: currentLagMs read under concurrent enqueue", 2000, 50,
        [&]() { (void)shipper.currentLagMs(); });

    stop_enqueue.store(true);
    enqueuer.join();
    shipper.stop();
    return result;
}

// ===========================================================================
// main — run all benchmarks and report pass/fail
// ===========================================================================

int main()
{
    std::printf("\n=== Replication Geo-Placement + WAL Shipping p95/p99 Baseline Benchmarks ===\n\n");
    std::printf("Acceptance thresholds:\n");
    std::printf("  GEO p99 < 500 µs  |  WAL enqueue p99 < 200 µs  |  WAL burst >= 50 000/s\n\n");

    std::vector<BenchResult> results;

    std::printf("--- GEO: Geographic Placement ---\n");
    results.push_back(benchGeo01()); printResult(results.back());
    results.push_back(benchGeo02()); printResult(results.back());
    results.push_back(benchGeo03()); printResult(results.back());
    results.push_back(benchGeo04()); printResult(results.back());

    std::printf("\n--- WAL: Async WAL Shipping ---\n");
    results.push_back(benchWal01()); printResult(results.back());
    results.push_back(benchWal02()); printResult(results.back());
    results.push_back(benchWal03()); printResult(results.back());
    results.push_back(benchWal04()); printResult(results.back());

    int failures = 0;
    for (const auto& r : results) {
        if (!r.passed) ++failures;
    }

    std::printf("\n=== %s: %d/%d benchmarks passed ===\n\n",
                failures == 0 ? "PASS" : "FAIL",
                static_cast<int>(results.size()) - failures,
                static_cast<int>(results.size()));

    return failures == 0 ? 0 : 1;
}
