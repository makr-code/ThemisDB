// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_cdc_delivery_gates.cpp
 * @brief Phase 5 CDC delivery hot-path release-gate benchmarks.
 *
 * Provides reproducible latency and throughput measurements for the CDC
 * delivery, replay, and consumer-group hot paths identified in the CDC module
 * roadmap (Phase 5 — Performance and Hardening).  Results are used as release
 * gates; a regression beyond 10% vs. the baseline blocks promotion.
 *
 * ## Benchmark families
 *
 * ### CDG-01..03 — DeliveryTracker hot paths
 *   CDG-01  DeliveryTracker trackDelivery (10-event batch)
 *   CDG-02  DeliveryTracker acknowledge (single sequence)
 *   CDG-03  DeliveryTracker acknowledgeUpTo (cumulative ack, 100 events)
 *
 * ### CDG-04..05 — ConsumerGroupManager hot paths
 *   CDG-04  ConsumerGroupManager createGroup / deleteGroup round-trip
 *   CDG-05  ConsumerGroupManager fetchEventsAtLeastOnce (50 events, 1 consumer)
 *
 * ### CDG-06 — InMemoryReplayController hot path
 *   CDG-06  InMemoryReplayController::replayFromSequence + full drain (100 events)
 *
 * ## Hard release gates
 *
 * | Gate ID       | Benchmark | Threshold                |
 * |---------------|-----------|--------------------------|
 * | GATE-CDC-01   | CDG-01    | p99 ≤ 500 µs             |
 * | GATE-CDC-02   | CDG-02    | p99 ≤ 10 µs              |
 * | GATE-CDC-03   | CDG-03    | p99 ≤ 200 µs             |
 * | GATE-CDC-04   | CDG-04    | p99 ≤ 1 ms               |
 * | GATE-CDC-05   | CDG-05    | p99 ≤ 5 ms               |
 * | GATE-CDC-06   | CDG-06    | p99 ≤ 5 ms (100 events)  |
 *
 * All benchmarks:
 *   - Use kCdgCanonicalSeed = 42 for deterministic event data.
 *   - Run with Repetitions(kRepetitions) to capture variance.
 *   - I/O-bound registrations use UseRealTime().
 *
 * @see src/cdc/ROADMAP.md — Phase 5 items
 * @see include/cdc/cdc_delivery_contract.h — §2 delivery semantics, §6 group consistency
 * @see benchmarks/cdc/bench_cdc_pipeline.cpp — pipeline / fan-out benchmarks
 */

#include <benchmark/benchmark.h>

#include "cdc/changefeed.h"
#include "cdc/consumer_group.h"
#include "cdc/delivery_tracker.h"
#include "cdc/icdc_replay_controller.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::cdc;
namespace fs = std::filesystem;

// ============================================================================
// Constants — deterministic, release-pinned
// ============================================================================

/// Canonical PRNG seed for all CDG benchmarks.
static constexpr uint64_t kCdgCanonicalSeed = 42;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// Shared fixture — RocksDB-backed CDC components
// ============================================================================

class CdcDeliveryGateFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        db_path_ = fs::temp_directory_path()
                 / ("bench_cdc_dlvgates_" + std::to_string(
                        std::atomic_fetch_add(&instance_counter_, 1)));
        if (fs::exists(db_path_)) fs::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_.string();
        cfg.memtable_size_mb    = 64;
        cfg.block_cache_size_mb = 128;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();

        auto* raw = db_->getDB();

        Changefeed::RetentionPolicy ret;
        ret.enabled = false;
        changefeed_ = std::make_unique<Changefeed>(raw, nullptr, ret);
        manager_    = std::make_unique<ConsumerGroupManager>(raw, nullptr);
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        manager_.reset();
        changefeed_.reset();
        db_->close();
        db_.reset();
        if (fs::exists(db_path_)) fs::remove_all(db_path_);
    }

    void seedEvents(int n) {
        for (int i = 0; i < n; ++i) {
            Changefeed::ChangeEvent ev;
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = "k:" + std::to_string(i);
            ev.value        = R"({"v":)" + std::to_string(i) + "}";
            ev.timestamp_ms = static_cast<int64_t>(kCdgCanonicalSeed) * 1000000LL + i;
            changefeed_->recordEvent(ev);
        }
    }

    static std::vector<Changefeed::ChangeEvent> makeSyntheticEvents(
            int count, uint64_t base_seq = 1) {
        std::vector<Changefeed::ChangeEvent> evs;
        evs.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            Changefeed::ChangeEvent ev;
            ev.sequence     = base_seq + static_cast<uint64_t>(i);
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = "bk:" + std::to_string(i);
            ev.value        = "v";
            ev.timestamp_ms = static_cast<int64_t>(base_seq) * 1000LL + i;
            evs.push_back(ev);
        }
        return evs;
    }

protected:
    fs::path                              db_path_;
    std::unique_ptr<RocksDBWrapper>       db_;
    std::unique_ptr<Changefeed>           changefeed_;
    std::unique_ptr<ConsumerGroupManager> manager_;

private:
    static std::atomic<int> instance_counter_;
};

std::atomic<int> CdcDeliveryGateFixture::instance_counter_{0};

// ============================================================================
// CDG-01  GATE-CDC-01: DeliveryTracker trackDelivery (10-event batch)
//         Threshold: p99 ≤ 500 µs
// ============================================================================

BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG01_TrackDelivery10Events)(
        benchmark::State& state) {
    auto evs = makeSyntheticEvents(10);
    DeliveryTracker tracker;
    std::atomic<uint64_t> base{1000};

    for (auto _ : state) {
        // Give each iteration a unique sequence range to avoid pending-full rejection.
        uint64_t b = base.fetch_add(10, std::memory_order_relaxed);
        auto batch = makeSyntheticEvents(10, b);
        bool ok = tracker.trackDelivery("bench_consumer", batch);
        benchmark::DoNotOptimize(ok);
        // Acknowledge immediately to keep pending count from growing.
        tracker.acknowledgeUpTo("bench_consumer", b + 9);
    }
    state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK_REGISTER_F(CdcDeliveryGateFixture, CDG01_TrackDelivery10Events)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(500)
    ->UseRealTime();

// ============================================================================
// CDG-02  GATE-CDC-02: DeliveryTracker acknowledge (single sequence)
//         Threshold: p99 ≤ 10 µs
// ============================================================================

BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG02_AcknowledgeSingleEvent)(
        benchmark::State& state) {
    DeliveryTracker tracker;
    std::atomic<uint64_t> seq{1};

    for (auto _ : state) {
        uint64_t s = seq.fetch_add(1, std::memory_order_relaxed);
        auto ev_batch = makeSyntheticEvents(1, s);
        tracker.trackDelivery("bench_ack_consumer", ev_batch);

        state.PauseTiming();
        // Pause: set up only; measurement is the acknowledge call below.
        state.ResumeTiming();

        bool ok = tracker.acknowledge("bench_ack_consumer", s);
        benchmark::DoNotOptimize(ok);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CdcDeliveryGateFixture, CDG02_AcknowledgeSingleEvent)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(2000);

// ============================================================================
// CDG-03  GATE-CDC-03: DeliveryTracker acknowledgeUpTo (100 events)
//         Threshold: p99 ≤ 200 µs
// ============================================================================

BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG03_AcknowledgeUpTo100Events)(
        benchmark::State& state) {
    DeliveryTracker tracker;
    std::atomic<uint64_t> base{1};

    for (auto _ : state) {
        uint64_t b = base.fetch_add(100, std::memory_order_relaxed);
        auto batch = makeSyntheticEvents(100, b);
        tracker.trackDelivery("bench_upto_consumer", batch);

        std::size_t removed = tracker.acknowledgeUpTo("bench_upto_consumer", b + 99);
        benchmark::DoNotOptimize(removed);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK_REGISTER_F(CdcDeliveryGateFixture, CDG03_AcknowledgeUpTo100Events)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(500)
    ->UseRealTime();

// ============================================================================
// CDG-04  GATE-CDC-04: ConsumerGroupManager createGroup / deleteGroup
//         Threshold: p99 ≤ 1 ms
// ============================================================================

BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG04_CreateDeleteGroup)(
        benchmark::State& state) {
    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string gid = "cdg04_grp_" + std::to_string(id);

        ConsumerGroupConfig cfg;
        cfg.group_id       = gid;
        cfg.consumer_count = 2;
        manager_->createGroup(cfg);
        manager_->deleteGroup(gid);
        benchmark::DoNotOptimize(id);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CdcDeliveryGateFixture, CDG04_CreateDeleteGroup)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(1000)
    ->UseRealTime();

// ============================================================================
// CDG-05  GATE-CDC-05: fetchEventsAtLeastOnce (50 events, 1 consumer)
//         Threshold: p99 ≤ 5 ms
// ============================================================================

BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG05_FetchEventsAtLeastOnce50)(
        benchmark::State& state) {
    seedEvents(50);

    ConsumerGroupConfig cfg;
    cfg.group_id       = "cdg05_grp";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    for (auto _ : state) {
        auto events = manager_->fetchEventsAtLeastOnce(
            "cdg05_grp", "consumer_0", *changefeed_, 50);
        benchmark::DoNotOptimize(events);
    }
    state.SetItemsProcessed(state.iterations() * 50);
}
BENCHMARK_REGISTER_F(CdcDeliveryGateFixture, CDG05_FetchEventsAtLeastOnce50)
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(kRepetitions)
    ->Iterations(200)
    ->UseRealTime();

// ============================================================================
// CDG-06  GATE-CDC-06: InMemoryReplayController::replayFromSequence + full drain
//         Threshold: p99 ≤ 5 ms (100 events)
// ============================================================================

BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG06_ReplayFromSequenceDrain100)(
        benchmark::State& state) {
    seedEvents(100);

    InMemoryReplayController ctrl(changefeed_.get());

    for (auto _ : state) {
        auto session = ctrl.replayFromSequence(0);

        std::size_t total = 0;
        while (!session->done()) {
            auto batch = session->nextBatch();
            total += batch.size();
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK_REGISTER_F(CdcDeliveryGateFixture, CDG06_ReplayFromSequenceDrain100)
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(kRepetitions)
    ->Iterations(200)
    ->UseRealTime();

BENCHMARK_MAIN();
