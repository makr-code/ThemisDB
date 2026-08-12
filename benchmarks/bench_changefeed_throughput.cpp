// Benchmark: Changefeed Throughput
// Measures CDC event processing performance and subscriber scalability

#include "cdc/changefeed.h"
#include "cdc/cdc_metrics.h"
#include "storage/rocksdb_wrapper.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <random>
#include <sstream>

using namespace themis;

// ============================================================================
// Test Setup
// ============================================================================

class ChangefeedBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Create unique thread-safe test database path
        const auto unique_id = static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        std::ostringstream suffix;
        suffix << "bench_changefeed_tmp_t" << state.thread_index() << "_" << unique_id;
        const auto db_dir = std::filesystem::absolute(
            std::filesystem::path("data") / suffix.str());
        test_db_path_ = db_dir.string();
        
        // Clean up any existing test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create RocksDB wrapper with absolute paths and WAL directory
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.wal_dir = (db_dir / "wal").string();
        config.memtable_size_mb = 128;
        config.block_cache_size_mb = 256;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open RocksDB in benchmark");
        }
        
        // Create changefeed
        changefeed_ = std::make_unique<Changefeed>(db_->getRawDB(), nullptr);
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        changefeed_.reset();
        db_->close();
        db_.reset();
        
        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
};

// ============================================================================
// Benchmark: Event Recording Throughput
// ============================================================================

BENCHMARK_DEFINE_F(ChangefeedBenchmarkFixture, EventRecordingThroughput)(benchmark::State& state) {
    size_t event_count = 0;
    
    for (auto _ : state) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "user_" + std::to_string(event_count);
        event.value = "{\"name\":\"User" + std::to_string(event_count) + "\",\"active\":true}";
        
        auto recorded_event = changefeed_->recordEvent(event);
        benchmark::DoNotOptimize(recorded_event);
        
        event_count++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["events_per_sec"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(ChangefeedBenchmarkFixture, EventRecordingThroughput)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Event Polling (List Events)
// ============================================================================

BENCHMARK_DEFINE_F(ChangefeedBenchmarkFixture, EventPolling)(benchmark::State& state) {
    const int num_events = state.range(0);
    
    // Pre-populate with events
    for (int i = 0; i < num_events; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "item_" + std::to_string(i);
        event.value = "{\"data\":\"value" + std::to_string(i) + "\"}";
        changefeed_->recordEvent(event);
    }
    
    uint64_t last_sequence = 0;
    
    for (auto _ : state) {
        Changefeed::ListOptions options;
        options.from_sequence = last_sequence;
        options.limit = 100; // Poll 100 events at a time
        options.long_poll_ms = 0; // No blocking
        
        auto events = changefeed_->listEvents(options);
        benchmark::DoNotOptimize(events);
        
        if (!events.empty()) {
            last_sequence = events.back().sequence;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["total_events"] = num_events;
}

BENCHMARK_REGISTER_F(ChangefeedBenchmarkFixture, EventPolling)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Concurrent Subscribers
// ============================================================================

BENCHMARK_DEFINE_F(ChangefeedBenchmarkFixture, ConcurrentSubscribers)(benchmark::State& state) {
    const int num_subscribers = state.range(0);
    
    // Pre-populate with events
    const int total_events = 1000;
    for (int i = 0; i < total_events; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "data_" + std::to_string(i);
        event.value = "{\"value\":" + std::to_string(i) + "}";
        changefeed_->recordEvent(event);
    }
    
    std::atomic<int> total_reads{0};
    std::vector<std::thread> subscribers;
    std::atomic<bool> should_run{true};
    
    // Start subscribers
    for (int i = 0; i < num_subscribers; i++) {
        subscribers.emplace_back([&, i]() {
            uint64_t last_sequence = 0;
            
            while (should_run) {
                Changefeed::ListOptions options;
                options.from_sequence = last_sequence;
                options.limit = 10;
                options.long_poll_ms = 10; // 10ms poll interval
                
                auto events = changefeed_->listEvents(options);
                
                if (!events.empty()) {
                    total_reads += events.size();
                    last_sequence = events.back().sequence;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    
    // Measure time for all subscribers to catch up
    for (auto _ : state) {
        total_reads = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Wait for all subscribers to read all events
        while (total_reads < total_events * num_subscribers) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Timeout after 5 seconds
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed > 5) {
                break;
            }
        }
    }
    
    should_run = false;
    for (auto& t : subscribers) {
        t.join();
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["num_subscribers"] = num_subscribers;
    state.counters["total_events"] = total_events;
}

BENCHMARK_REGISTER_F(ChangefeedBenchmarkFixture, ConcurrentSubscribers)
    ->Arg(1)
    ->Arg(10)
    ->Arg(50)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(3);

// ============================================================================
// Benchmark: Event Type Mix
// ============================================================================

static void BM_EventTypeMix(benchmark::State& state) {
    // Setup
    std::string test_db_path = "./data/bench_changefeed_event_mix_tmp";
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.memtable_size_mb = 128;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    db->open();
    auto changefeed = std::make_unique<Changefeed>(db->getRawDB(), nullptr);
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> type_dist(0, 3);
    size_t event_count = 0;
    
    for (auto _ : state) {
        Changefeed::ChangeEvent event;
        
        // Random event type
        int type = type_dist(rng);
        switch (type) {
            case 0: event.type = Changefeed::ChangeEventType::EVENT_PUT; break;
            case 1: event.type = Changefeed::ChangeEventType::EVENT_DELETE; break;
            case 2: event.type = Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT; break;
            case 3: event.type = Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK; break;
        }
        
        event.key = "key_" + std::to_string(event_count);
        if (type == 0 || type == 2) {
            event.value = "{\"data\":\"value" + std::to_string(event_count) + "\"}";
        }
        
        auto recorded_event = changefeed->recordEvent(event);
        benchmark::DoNotOptimize(recorded_event);
        
        event_count++;
    }
    
    // Cleanup
    changefeed.reset();
    db->close();
    db.reset();
    
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["events_per_sec"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_EventTypeMix)
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Burst vs Steady State Writes
// ============================================================================

static void BM_BurstWrites(benchmark::State& state) {
    // Setup
    std::string test_db_path = "./data/bench_changefeed_burst_tmp";
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.memtable_size_mb = 256;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    db->open();
    auto changefeed = std::make_unique<Changefeed>(db->getRawDB(), nullptr);
    
    const int burst_size = state.range(0);
    size_t event_count = 0;
    
    for (auto _ : state) {
        // Record burst of events
        for (int i = 0; i < burst_size; i++) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "burst_" + std::to_string(event_count++);
            event.value = "{\"burst\":true}";
            
            changefeed->recordEvent(event);
        }
        
        // Small pause between bursts
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Cleanup
    changefeed.reset();
    db->close();
    db.reset();
    
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    state.SetItemsProcessed(state.iterations() * burst_size);
    state.counters["burst_size"] = burst_size;
    state.counters["events_per_sec"] = benchmark::Counter(
        state.iterations() * burst_size, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_BurstWrites)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Replication Lag Simulation
// ============================================================================

static void BM_ReplicationLag(benchmark::State& state) {
    // Setup
    std::string test_db_path = "./data/bench_changefeed_lag_tmp";
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.memtable_size_mb = 128;
    
    auto db = std::make_unique<RocksDBWrapper>(config);
    db->open();
    auto changefeed = std::make_unique<Changefeed>(db->getRawDB(), nullptr);
    
    // Pre-populate with events
    const int total_events = 10000;
    for (int i = 0; i < total_events; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "item_" + std::to_string(i);
        event.value = "{\"index\":" + std::to_string(i) + "}";
        changefeed->recordEvent(event);
    }
    
    // Measure catch-up speed
    for (auto _ : state) {
        uint64_t last_sequence = 0;
        int events_read = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        while (events_read < total_events) {
            Changefeed::ListOptions options;
            options.from_sequence = last_sequence;
            options.limit = 100;
            options.long_poll_ms = 0;
            
            auto events = changefeed->listEvents(options);
            
            if (events.empty()) {
                break;
            }
            
            events_read += events.size();
            last_sequence = events.back().sequence;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        state.counters["catch_up_time_ms"] = duration_ms;
        state.counters["events_per_sec"] = (total_events * 1000.0) / duration_ms;
    }
    
    // Cleanup
    changefeed.reset();
    db->close();
    db.reset();
    
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
    
    state.SetItemsProcessed(state.iterations() * total_events);
}

BENCHMARK(BM_ReplicationLag)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);

// ============================================================================
// Benchmark: Replication Lag with simulated WAN RTT
// Simulates cross-DC transport by adding half RTT before and after each poll.
// Reports lag p50/p95/p99 in milliseconds for R-8 style evaluation.
// ============================================================================

static void BM_ReplicationLagWAN(benchmark::State& state) {
    const int simulated_rtt_ms = static_cast<int>(state.range(0));
    const int batch_size = 100;

    std::string test_db_path = "./data/bench_changefeed_lag_wan_tmp";
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }

    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.memtable_size_mb = 128;

    auto db = std::make_unique<RocksDBWrapper>(config);
    if (!db->open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }
    auto changefeed = std::make_unique<Changefeed>(db->getRawDB(), nullptr);

    const int total_events = 10000;
    for (int i = 0; i < total_events; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "wan_item_" + std::to_string(i);
        event.value = "{\"index\":" + std::to_string(i) + "}";
        changefeed->recordEvent(event);
    }

    themis::cdc::LatencyHistogram lag_hist;

    for (auto _ : state) {
        uint64_t last_sequence = 0;
        int events_read = 0;

        while (events_read < total_events) {
            auto lag_start = std::chrono::steady_clock::now();

            // Simulate one-way network transfer to follower.
            std::this_thread::sleep_for(std::chrono::milliseconds(simulated_rtt_ms / 2));

            Changefeed::ListOptions options;
            options.from_sequence = last_sequence;
            options.limit = batch_size;
            options.long_poll_ms = 0;

            auto events = changefeed->listEvents(options);

            // Simulate one-way network transfer of ACK/visibility.
            std::this_thread::sleep_for(std::chrono::milliseconds(simulated_rtt_ms / 2));

            auto lag_end = std::chrono::steady_clock::now();
            auto lag_us = std::chrono::duration_cast<std::chrono::microseconds>(lag_end - lag_start).count();
            lag_hist.record(static_cast<uint64_t>(lag_us));

            if (events.empty()) {
                break;
            }

            events_read += static_cast<int>(events.size());
            last_sequence = events.back().sequence;
        }
    }

    state.SetItemsProcessed(state.iterations() * total_events);
    state.counters["simulated_rtt_ms"] = static_cast<double>(simulated_rtt_ms);
    state.counters["lag_p50_ms"] = static_cast<double>(lag_hist.p50()) / 1000.0;
    state.counters["lag_p95_ms"] = static_cast<double>(lag_hist.p95()) / 1000.0;
    state.counters["lag_p99_ms"] = static_cast<double>(lag_hist.p99()) / 1000.0;

    changefeed.reset();
    db->close();
    db.reset();
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
}

BENCHMARK(BM_ReplicationLagWAN)
    ->Arg(2)
    ->Arg(50)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);

// ============================================================================
// Benchmark: Event Recording Latency (p50 / p95 / p99)
// Uses ManualTime so each iteration time is the raw recordEvent() cost.
// Percentile counters are computed from a LatencyHistogram and reported
// as benchmark counters so they appear in every output format.
// ============================================================================

static void BM_RecordEventLatency(benchmark::State& state) {
    std::string test_db_path = "./data/bench_changefeed_record_latency_tmp";
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }

    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.memtable_size_mb = 128;

    auto db = std::make_unique<RocksDBWrapper>(config);
    if (!db->open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }
    auto changefeed = std::make_unique<Changefeed>(db->getRawDB(), nullptr);

    themis::cdc::LatencyHistogram hist;
    size_t event_count = 0;

    for (auto _ : state) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "latency_key_" + std::to_string(event_count++);
        event.value = "{\"v\":" + std::to_string(event_count) + "}";

        auto t0 = std::chrono::steady_clock::now();
        auto recorded = changefeed->recordEvent(event);
        auto t1 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(recorded);

        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        hist.record(static_cast<uint64_t>(elapsed_us));
        state.SetIterationTime(static_cast<double>(elapsed_us) * 1e-6);
    }

    state.counters["p50_us"] = static_cast<double>(hist.p50());
    state.counters["p95_us"] = static_cast<double>(hist.p95());
    state.counters["p99_us"] = static_cast<double>(hist.p99());
    state.counters["avg_us"] = hist.average();
    state.counters["events_per_sec"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);

    changefeed.reset();
    db->close();
    db.reset();
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
}

BENCHMARK(BM_RecordEventLatency)
    ->UseManualTime()
    ->Threads(1)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Event Polling Latency (p50 / p95 / p99)
// Pre-populates a fixed set of events then measures listEvents() latency.
// ============================================================================

static void BM_ListEventsLatency(benchmark::State& state) {
    const int num_events = static_cast<int>(state.range(0));

    std::string test_db_path = "./data/bench_changefeed_list_latency_tmp";
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }

    RocksDBWrapper::Config config;
    config.db_path = test_db_path;
    config.memtable_size_mb = 256;

    auto db = std::make_unique<RocksDBWrapper>(config);
    if (!db->open()) {
        state.SkipWithError("Failed to open RocksDB");
        return;
    }
    auto changefeed = std::make_unique<Changefeed>(db->getRawDB(), nullptr);

    for (int i = 0; i < num_events; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "poll_key_" + std::to_string(i);
        event.value = "{\"i\":" + std::to_string(i) + "}";
        changefeed->recordEvent(event);
    }

    themis::cdc::LatencyHistogram hist;
    uint64_t last_seq = 0;

    for (auto _ : state) {
        Changefeed::ListOptions opts;
        opts.from_sequence = last_seq;
        opts.limit = 100;
        opts.long_poll_ms = 0;

        auto t0 = std::chrono::steady_clock::now();
        auto events = changefeed->listEvents(opts);
        auto t1 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(events);

        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        hist.record(static_cast<uint64_t>(elapsed_us));
        state.SetIterationTime(static_cast<double>(elapsed_us) * 1e-6);

        if (!events.empty()) {
            last_seq = events.back().sequence;
        } else {
            last_seq = 0; // wrap around for next iteration
        }
    }

    state.counters["p50_us"] = static_cast<double>(hist.p50());
    state.counters["p95_us"] = static_cast<double>(hist.p95());
    state.counters["p99_us"] = static_cast<double>(hist.p99());
    state.counters["avg_us"] = hist.average();
    state.counters["preloaded_events"] = num_events;

    changefeed.reset();
    db->close();
    db.reset();
    if (std::filesystem::exists(test_db_path)) {
        std::filesystem::remove_all(test_db_path);
    }
}

BENCHMARK(BM_ListEventsLatency)
    ->UseManualTime()
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

// benchmark_main wird über CMake verlinkt
