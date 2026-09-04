/// @file bench_cdc_pipeline.cpp
/// @brief Performance benchmarks for the CDC pipeline beyond raw changefeed
///        event recording (which is covered by bench_changefeed_throughput).
///
/// Covers:
///   - ConsumerGroupManager::createGroup / deleteGroup latency
///   - ConsumerGroupManager::fetchEventsAtLeastOnce throughput
///   - ConsumerGroupManager::acknowledgeEvents throughput
///   - Partition-fan-out: N consumers fetching in parallel
///
/// Performance targets (src/cdc/ROADMAP.md):
///   - createGroup:                   < 1 ms
///   - fetchEventsAtLeastOnce (100 events, 1 consumer): < 5 ms
///   - acknowledgeEvents (100 events): < 2 ms

#include <benchmark/benchmark.h>
#include "cdc/consumer_group.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <string>
#include <vector>
#include <atomic>

using namespace themis;
using namespace themis::cdc;

// ============================================================================
// Fixture
// ============================================================================

class CdcPipelineBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        db_path_ = "./data/bench_cdc_pipeline_tmp";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 64;
        cfg.block_cache_size_mb = 128;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        db_->open();

        auto* raw_db = db_->getDB();

        Changefeed::RetentionPolicy ret;
        ret.enabled = false;
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, ret);
        manager_    = std::make_unique<ConsumerGroupManager>(raw_db, nullptr);
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        manager_.reset();
        changefeed_.reset();
        db_->close();
        db_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    void seedEvents(int n) {
        for (int i = 0; i < n; ++i) {
            Changefeed::ChangeEvent ev;
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = "key:" + std::to_string(i);
            ev.value        = R"({"v":)" + std::to_string(i) + "}";
            ev.timestamp_ms = 1700000000000LL + i;
            changefeed_->recordEvent(ev);
        }
    }

protected:
    std::string                          db_path_;
    std::unique_ptr<RocksDBWrapper>      db_;
    std::unique_ptr<Changefeed>          changefeed_;
    std::unique_ptr<ConsumerGroupManager> manager_;
};

// ============================================================================
// createGroup / deleteGroup latency
// ============================================================================

BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, CreateDeleteGroup)(benchmark::State& state) {
    std::atomic<int> counter{0};
    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string gid = "bench_grp_" + std::to_string(id);

        ConsumerGroupConfig cfg;
        cfg.group_id       = gid;
        cfg.consumer_count = 2;
        manager_->createGroup(cfg);
        manager_->deleteGroup(gid);
        benchmark::DoNotOptimize(id);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(CdcPipelineBenchFixture, CreateDeleteGroup)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(2000);

// ============================================================================
// fetchEventsAtLeastOnce – throughput vs. batch size
// ============================================================================

BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, FetchEventsAtLeastOnce)(benchmark::State& state) {
    const int batch = static_cast<int>(state.range(0));

    seedEvents(batch);

    ConsumerGroupConfig cfg;
    cfg.group_id       = "bench_fetch_grp";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    for (auto _ : state) {
        auto events = manager_->fetchEventsAtLeastOnce(
            "bench_fetch_grp", "consumer_0", *changefeed_, batch);
        benchmark::DoNotOptimize(events);
    }

    state.SetItemsProcessed(state.iterations() * batch);
}

BENCHMARK_REGISTER_F(CdcPipelineBenchFixture, FetchEventsAtLeastOnce)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(200);

// ============================================================================
// acknowledgeEvents throughput
// ============================================================================

BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, AcknowledgeEvents)(benchmark::State& state) {
    const int batch = static_cast<int>(state.range(0));

    seedEvents(batch);

    ConsumerGroupConfig cfg;
    cfg.group_id       = "bench_ack_grp";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    auto events = manager_->fetchEventsAtLeastOnce(
        "bench_ack_grp", "consumer_0", *changefeed_, batch);

    uint64_t max_seq = 0;
    for (const auto& e : events) {
        if (e.sequence > max_seq) {
          max_seq = e.sequence;
        }
    }

    for (auto _ : state) {
        manager_->acknowledgeEvents("bench_ack_grp", "consumer_0", max_seq);
        benchmark::DoNotOptimize(max_seq);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(events.size()));
}

BENCHMARK_REGISTER_F(CdcPipelineBenchFixture, AcknowledgeEvents)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(500);

// ============================================================================
// Concurrent fetch – N consumers in parallel (fan-out)
// ============================================================================

BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, ConcurrentFetch)(benchmark::State& state) {
    const int consumers = static_cast<int>(state.range(0));
    const int batch     = 50;

    seedEvents(batch * consumers);

    ConsumerGroupConfig cfg;
    cfg.group_id       = "bench_concurrent_grp";
    cfg.consumer_count = static_cast<uint32_t>(consumers);
    manager_->createGroup(cfg);

    for (auto _ : state) {
        int total = 0;
        for (int c = 0; c < consumers; ++c) {
            auto events = manager_->fetchEventsAtLeastOnce(
                "bench_concurrent_grp",
                "consumer_" + std::to_string(c),
                *changefeed_,
                batch);
            total += static_cast<int>(events.size());
        }
        benchmark::DoNotOptimize(total);
    }

    state.SetItemsProcessed(state.iterations() * consumers * batch);
}

BENCHMARK_REGISTER_F(CdcPipelineBenchFixture, ConcurrentFetch)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Unit(benchmark::kMillisecond)
    ->Threads(1)
    ->Iterations(100);

BENCHMARK_MAIN();
