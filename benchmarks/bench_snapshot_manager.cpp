/**
 * @file bench_snapshot_manager.cpp
 * @brief Performance benchmarks for Named Snapshots (SnapshotManager)
 * 
 * This benchmark suite measures the performance characteristics of the
 * SnapshotManager implementation for various operations and scenarios.
 * 
 * Benchmark Categories:
 * - Basic Operations: Create, Get, List, Delete
 * - Scalability: Performance with varying snapshot counts
 * - Concurrency: Thread-safe operations under load
 * - Validation: Input validation overhead
 * 
 * Dependencies:
 *   - Google Benchmark (required): Install via vcpkg or system package manager
 *     vcpkg: vcpkg install benchmark
 *     apt: sudo apt-get install libbenchmark-dev
 *     brew: brew install google-benchmark
 * 
 * Build:
 *   cmake -B build -DTHEMIS_BUILD_BENCHMARKS=ON
 *   cmake --build build --target bench_snapshot_manager
 * 
 * Run:
 *   ./build/benchmarks/bench_snapshot_manager
 * 
 * Note: Benchmarks are optional. If Google Benchmark is not found,
 *       the build will continue without benchmarks.
 */

#include <benchmark/benchmark.h>
#include "transaction/snapshot_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include "utils/logger.h"
#include <filesystem>
#include <random>
#include <thread>

using namespace themis;

// Test fixture for benchmarks
class SnapshotBenchmarkFixture : public benchmark::Fixture {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_mgr_;
    std::string test_db_path_;

    void SetUp(const ::benchmark::State& state) override {
        // Create unique database for this benchmark run
        test_db_path_ = "/tmp/bench_snapshot_" + 
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = false;  // Faster for benchmarks
        config.enable_statistics = false;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        db_->open();
        
        // Initialize Changefeed
        changefeed_ = std::make_unique<Changefeed>(db_->getTransactionDB(), nullptr);
        
        // Initialize SnapshotManager
        snapshot_mgr_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
    }

    void TearDown(const ::benchmark::State& state) override {
        snapshot_mgr_.reset();
        changefeed_.reset();
        db_->close();
        db_.reset();
        
        // Clean up test database
        std::filesystem::remove_all(test_db_path_);
    }

    // Helper: Create changefeed events
    void createChangefeedEvents(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "bench:key_" + std::to_string(i);
            event.value = "value_" + std::to_string(i);
            event.timestamp_ms = 0;
            changefeed_->recordEvent(event);
        }
    }

    // Helper: Generate random tag name
    std::string generateTagName(size_t index) {
        return "benchmark_tag_" + std::to_string(index);
    }
};

// ============================================================================
// Basic Operations Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, CreateTag)(benchmark::State& state) {
    createChangefeedEvents(10);
    size_t index = 0;
    
    for (auto _ : state) {
        std::string tag_name = generateTagName(index++);
        auto status = snapshot_mgr_->createTag(tag_name, "Benchmark snapshot", "bench");
        benchmark::DoNotOptimize(status);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, CreateTag)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, GetTag)(benchmark::State& state) {
    // Pre-create tags
    createChangefeedEvents(10);
    for (int i = 0; i < 100; ++i) {
        snapshot_mgr_->createTag(generateTagName(i), "Benchmark", "bench");
    }
    
    size_t index = 0;
    for (auto _ : state) {
        std::string tag_name = generateTagName(index++ % 100);
        auto snapshot = snapshot_mgr_->getTag(tag_name);
        benchmark::DoNotOptimize(snapshot);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, GetTag)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, DeleteTag)(benchmark::State& state) {
    createChangefeedEvents(10);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::string tag_name = generateTagName(state.iterations());
        snapshot_mgr_->createTag(tag_name, "Benchmark", "bench");
        state.ResumeTiming();
        
        auto status = snapshot_mgr_->deleteTag(tag_name);
        benchmark::DoNotOptimize(status);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, DeleteTag)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, ListTags)(benchmark::State& state) {
    // Pre-create varying numbers of tags
    createChangefeedEvents(10);
    size_t tag_count = state.range(0);
    for (size_t i = 0; i < tag_count; ++i) {
        snapshot_mgr_->createTag(generateTagName(i), "Benchmark", "bench");
    }
    
    for (auto _ : state) {
        auto tags = snapshot_mgr_->listTags();
        benchmark::DoNotOptimize(tags);
    }
    
    state.SetItemsProcessed(state.iterations() * tag_count);
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, ListTags)
    ->Unit(benchmark::kMillisecond)
    ->Args({10})      // 10 tags
    ->Args({100})     // 100 tags
    ->Args({1000})    // 1000 tags
    ->Args({5000});   // 5000 tags

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, GetStats)(benchmark::State& state) {
    // Pre-create varying numbers of tags
    createChangefeedEvents(10);
    size_t tag_count = state.range(0);
    for (size_t i = 0; i < tag_count; ++i) {
        snapshot_mgr_->createTag(generateTagName(i), "Benchmark", "bench");
    }
    
    for (auto _ : state) {
        auto stats = snapshot_mgr_->getStats();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations() * tag_count);
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, GetStats)
    ->Unit(benchmark::kMillisecond)
    ->Args({10})
    ->Args({100})
    ->Args({1000})
    ->Args({5000});

// ============================================================================
// Validation Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, ValidateTagName)(benchmark::State& state) {
    std::vector<std::string> test_names = {
        "valid_tag_name",
        "another-valid-tag",
        "tag_with_123_numbers",
        "UPPERCASE_TAG",
        "MixedCase_Tag-123"
    };
    
    size_t index = 0;
    for (auto _ : state) {
        auto status = SnapshotManager::validateTagName(test_names[index++ % test_names.size()]);
        benchmark::DoNotOptimize(status);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, ValidateTagName)
    ->Unit(benchmark::kNanosecond)
    ->Iterations(100000);

// ============================================================================
// Concurrency Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, ConcurrentCreate)(benchmark::State& state) {
    createChangefeedEvents(10);
    std::atomic<size_t> counter{0};
    
    for (auto _ : state) {
        std::string tag_name = generateTagName(counter.fetch_add(1));
        auto status = snapshot_mgr_->createTag(tag_name, "Concurrent test", "bench");
        benchmark::DoNotOptimize(status);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, ConcurrentCreate)
    ->Unit(benchmark::kMicrosecond)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Iterations(1000);

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, ConcurrentRead)(benchmark::State& state) {
    // Pre-create tags
    createChangefeedEvents(10);
    for (int i = 0; i < 100; ++i) {
        snapshot_mgr_->createTag(generateTagName(i), "Benchmark", "bench");
    }
    
    std::atomic<size_t> counter{0};
    for (auto _ : state) {
        std::string tag_name = generateTagName(counter.fetch_add(1) % 100);
        auto snapshot = snapshot_mgr_->getTag(tag_name);
        benchmark::DoNotOptimize(snapshot);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, ConcurrentRead)
    ->Unit(benchmark::kMicrosecond)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Iterations(10000);

// ============================================================================
// End-to-End Scenario Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, FullWorkflow)(benchmark::State& state) {
    createChangefeedEvents(10);
    size_t index = 0;
    
    for (auto _ : state) {
        std::string tag_name = generateTagName(index++);
        
        // Create
        auto create_status = snapshot_mgr_->createTag(tag_name, "Workflow test", "bench");
        benchmark::DoNotOptimize(create_status);
        
        // Get
        auto snapshot = snapshot_mgr_->getTag(tag_name);
        benchmark::DoNotOptimize(snapshot);
        
        // Check existence
        bool exists = snapshot_mgr_->tagExists(tag_name);
        benchmark::DoNotOptimize(exists);
        
        // Delete
        auto delete_status = snapshot_mgr_->deleteTag(tag_name);
        benchmark::DoNotOptimize(delete_status);
    }
    
    state.SetItemsProcessed(state.iterations() * 4); // 4 operations per iteration
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, FullWorkflow)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

// ============================================================================
// Memory and Storage Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(SnapshotBenchmarkFixture, StorageOverhead)(benchmark::State& state) {
    createChangefeedEvents(10);
    size_t tag_count = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Create tags with varying description lengths
        for (size_t i = 0; i < tag_count; ++i) {
            std::string desc(state.range(1), 'x');  // Description length
            snapshot_mgr_->createTag(generateTagName(i), desc, "bench");
        }
        
        state.ResumeTiming();
        
        // Measure listing performance with storage overhead
        auto tags = snapshot_mgr_->listTags();
        benchmark::DoNotOptimize(tags);
        
        state.PauseTiming();
        // Cleanup
        for (size_t i = 0; i < tag_count; ++i) {
            snapshot_mgr_->deleteTag(generateTagName(i));
        }
        state.ResumeTiming();
    }
}
BENCHMARK_REGISTER_F(SnapshotBenchmarkFixture, StorageOverhead)
    ->Unit(benchmark::kMillisecond)
    ->Args({100, 50})     // 100 tags, 50 char descriptions
    ->Args({100, 200})    // 100 tags, 200 char descriptions
    ->Args({100, 1000})   // 100 tags, 1000 char descriptions
    ->Iterations(10);

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    // Initialize logger for benchmarks
    Logger::instance().initialize();
    Logger::instance().setLevel(LogLevel::ERROR);  // Minimize logging overhead
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    return 0;
}
