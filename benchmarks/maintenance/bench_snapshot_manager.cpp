#include <benchmark/benchmark.h>
#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <random>

using namespace themis;
using namespace themis::transaction;

class SnapshotManagerBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        test_db_path_ = "./data/themis_snapshot_manager_bench";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(config);
        db_->open();
        
        auto txn_db = db_->getRawDB();
        changefeed_ = std::make_unique<Changefeed>(txn_db);
        snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
        
        // Pre-populate for benchmarks
        if (state.range(0) > 0) {
            populateTags(state.range(0));
        }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        snapshot_manager_.reset();
        changefeed_.reset();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    void populateTags(int64_t num_tags) {
        for (int64_t i = 0; i < num_tags; ++i) {
            std::string tag_name = "tag_" + std::to_string(i);
            std::string description = "Description for tag " + std::to_string(i);
            snapshot_manager_->createTag(tag_name, description, "benchmark");
        }
    }
    
    void recordEvents(int count) {
        for (int i = 0; i < count; ++i) {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "bench:key" + std::to_string(i);
            event.value = "value" + std::to_string(i);
            auto now = std::chrono::system_clock::now();
            event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            changefeed_->recordEvent(event);
        }
    }
    
protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_manager_;
};

// Benchmark: Create tag (target: <1ms)
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, CreateTag)(benchmark::State& state) {
    int counter = 0;
    for (auto _ : state) {
        std::string tag_name = "perf_tag_" + std::to_string(counter++);
        auto snapshot = snapshot_manager_->createTag(tag_name, "Performance test tag", "benchmark");
        benchmark::DoNotOptimize(snapshot);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Get tag (target: <0.5ms)
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, GetTag)(benchmark::State& state) {
    // Pre-create a tag
    snapshot_manager_->createTag("bench_tag", "Benchmark tag", "benchmark");
    
    for (auto _ : state) {
        auto snapshot = snapshot_manager_->getTag("bench_tag");
        benchmark::DoNotOptimize(snapshot);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: List tags with 10 tags
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTags10)(benchmark::State& state) {
    for (auto _ : state) {
        auto tags = snapshot_manager_->listTags();
        benchmark::DoNotOptimize(tags);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: List tags with 100 tags (target: <10ms)
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTags100)(benchmark::State& state) {
    for (auto _ : state) {
        auto tags = snapshot_manager_->listTags();
        benchmark::DoNotOptimize(tags);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: List tags with 1000 tags
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTags1000)(benchmark::State& state) {
    for (auto _ : state) {
        auto tags = snapshot_manager_->listTags();
        benchmark::DoNotOptimize(tags);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: List tags with limit
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTagsWithLimit)(benchmark::State& state) {
    for (auto _ : state) {
        auto tags = snapshot_manager_->listTags(50);
        benchmark::DoNotOptimize(tags);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: List tags sorted by name
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTagsSortedByName)(benchmark::State& state) {
    for (auto _ : state) {
        auto tags = snapshot_manager_->listTags(0, "name", true);
        benchmark::DoNotOptimize(tags);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: List tags sorted by sequence
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ListTagsSortedBySequence)(benchmark::State& state) {
    for (auto _ : state) {
        auto tags = snapshot_manager_->listTags(0, "sequence", true);
        benchmark::DoNotOptimize(tags);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Delete tag
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, DeleteTag)(benchmark::State& state) {
    int counter = 0;
    
    // Pre-create tags for deletion
    for (int i = 0; i < state.max_iterations; ++i) {
        snapshot_manager_->createTag("delete_tag_" + std::to_string(i), "To be deleted", "benchmark");
    }
    
    for (auto _ : state) {
        std::string tag_name = "delete_tag_" + std::to_string(counter++);
        bool deleted = snapshot_manager_->deleteTag(tag_name);
        benchmark::DoNotOptimize(deleted);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Tag exists check
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, TagExists)(benchmark::State& state) {
    snapshot_manager_->createTag("exists_tag", "Exists check", "benchmark");
    
    for (auto _ : state) {
        bool exists = snapshot_manager_->tagExists("exists_tag");
        benchmark::DoNotOptimize(exists);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Get statistics
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, GetStatistics)(benchmark::State& state) {
    for (auto _ : state) {
        auto stats = snapshot_manager_->getStats();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Get sequence for tag
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, GetSequenceForTag)(benchmark::State& state) {
    snapshot_manager_->createTag("seq_tag", "Sequence tag", "benchmark");
    
    for (auto _ : state) {
        auto seq = snapshot_manager_->getSequenceForTag("seq_tag");
        benchmark::DoNotOptimize(seq);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Concurrent tag creation
BENCHMARK_DEFINE_F(SnapshotManagerBenchmark, ConcurrentCreateTag)(benchmark::State& state) {
    std::atomic<int> counter{0};
    
    for (auto _ : state) {
        int id = counter.fetch_add(1);
        std::string tag_name = "concurrent_tag_" + std::to_string(id);
        auto snapshot = snapshot_manager_->createTag(tag_name, "Concurrent test", "benchmark");
        benchmark::DoNotOptimize(snapshot);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Register benchmarks with different dataset sizes
BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, CreateTag)
    ->Arg(0)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, GetTag)
    ->Arg(0)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, ListTags10)
    ->Arg(10)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, ListTags100)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(500);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, ListTags1000)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, ListTagsWithLimit)
    ->Arg(100)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, ListTagsSortedByName)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(500);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, ListTagsSortedBySequence)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(500);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, DeleteTag)
    ->Arg(0)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, TagExists)
    ->Arg(1)
    ->Unit(benchmark::kNanosecond)
    ->Iterations(10000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, GetStatistics)
    ->Arg(100)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, GetSequenceForTag)
    ->Arg(1)
    ->Unit(benchmark::kNanosecond)
    ->Iterations(10000);

BENCHMARK_REGISTER_F(SnapshotManagerBenchmark, ConcurrentCreateTag)
    ->Arg(0)
    ->Unit(benchmark::kMicrosecond)
    ->Threads(4)
    ->Iterations(1000);

BENCHMARK_MAIN();
