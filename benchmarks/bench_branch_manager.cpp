/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_branch_manager.cpp                           ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:21:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     293                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9c9ead9b4f  2026-04-09  Implement feature X to enhance user experience and optimi... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <benchmark/benchmark.h>
#include "transaction/branch_manager.h"
#include "transaction/snapshot_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace themis {
namespace transaction {

class BranchManagerBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Create unique test database path
        test_db_path_ = "./data/themis_branch_manager_bench";
        
        // Remove if exists
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create directory
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize database
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        db_ = std::make_unique<RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("failed to open benchmark RocksDB instance");
        }
        
        // Initialize changefeed
        changefeed_ = std::make_unique<Changefeed>(db_->getRawDB());
        
        // Initialize snapshot manager
        snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
        
        // Initialize branch manager
        branch_manager_ = std::make_unique<BranchManager>(*db_, *changefeed_, *snapshot_manager_);
        
        // Record some events
        for (int i = 0; i < 1000; i++) {
            std::string key = "bench_key_" + std::to_string(i);
            std::string value = "bench_value_" + std::to_string(i);
            Changefeed::ChangeEvent ev;
            ev.type = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key = key;
            ev.value = value;
            changefeed_->recordEvent(std::move(ev));
        }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        // Reset in correct order
        branch_manager_.reset();
        snapshot_manager_.reset();
        changefeed_.reset();
        db_.reset();
        
        // Remove test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_manager_;
    std::unique_ptr<BranchManager> branch_manager_;
};

// Benchmark: Create branch
BENCHMARK_F(BranchManagerBenchmark, CreateBranch)(benchmark::State& state) {
    int branch_count = 0;
    for (auto _ : state) {
        std::string branch_name = "feature/bench_" + std::to_string(branch_count++);
        auto branch = branch_manager_->createBranch(
            branch_name,
            "main",
            "Benchmark branch",
            "bench_user"
        );
        benchmark::DoNotOptimize(branch);
    }
}

// Benchmark: Get branch
BENCHMARK_F(BranchManagerBenchmark, GetBranch)(benchmark::State& state) {
    // Create branches
    for (int i = 0; i < 100; i++) {
        std::string branch_name = "feature/bench_" + std::to_string(i);
        branch_manager_->createBranch(branch_name, "main", "Benchmark branch");
    }
    
    int idx = 0;
    for (auto _ : state) {
        std::string branch_name = "feature/bench_" + std::to_string(idx % 100);
        auto branch = branch_manager_->getBranch(branch_name);
        benchmark::DoNotOptimize(branch);
        idx++;
    }
}

// Benchmark: List branches
BENCHMARK_F(BranchManagerBenchmark, ListBranches)(benchmark::State& state) {
    int num_branches = state.range(0);
    
    // Create branches
    for (int i = 0; i < num_branches; i++) {
        std::string branch_name = "feature/bench_" + std::to_string(i);
        branch_manager_->createBranch(branch_name, "main", "Benchmark branch");
    }
    
    for (auto _ : state) {
        auto branches = branch_manager_->listBranches();
        benchmark::DoNotOptimize(branches);
    }
}

BENCHMARK_REGISTER_F(BranchManagerBenchmark, ListBranches)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500);

// Benchmark: Switch branch
BENCHMARK_F(BranchManagerBenchmark, SwitchBranch)(benchmark::State& state) {
    // Create branches
    for (int i = 0; i < 10; i++) {
        std::string branch_name = "feature/bench_" + std::to_string(i);
        branch_manager_->createBranch(branch_name, "main", "Benchmark branch");
    }
    
    int idx = 0;
    for (auto _ : state) {
        std::string branch_name = "feature/bench_" + std::to_string(idx % 10);
        bool success = branch_manager_->switchBranch(branch_name);
        benchmark::DoNotOptimize(success);
        idx++;
    }
}

// Benchmark: Delete branch
BENCHMARK_F(BranchManagerBenchmark, DeleteBranch)(benchmark::State& state) {
    state.PauseTiming();
    int branch_count = 0;
    
    for (auto _ : state) {
        // Create branch
        std::string branch_name = "feature/bench_" + std::to_string(branch_count++);
        branch_manager_->createBranch(branch_name, "main", "Benchmark branch");
        
        state.ResumeTiming();
        // Delete branch
        bool success = branch_manager_->deleteBranch(branch_name, true);
        benchmark::DoNotOptimize(success);
        state.PauseTiming();
    }
}

// Benchmark: Check branch exists
BENCHMARK_F(BranchManagerBenchmark, BranchExists)(benchmark::State& state) {
    // Create branches
    for (int i = 0; i < 100; i++) {
        std::string branch_name = "feature/bench_" + std::to_string(i);
        branch_manager_->createBranch(branch_name, "main", "Benchmark branch");
    }
    
    int idx = 0;
    for (auto _ : state) {
        std::string branch_name = "feature/bench_" + std::to_string(idx % 100);
        bool exists = branch_manager_->branchExists(branch_name);
        benchmark::DoNotOptimize(exists);
        idx++;
    }
}

// Benchmark: Get stats
BENCHMARK_F(BranchManagerBenchmark, GetStats)(benchmark::State& state) {
    int num_branches = state.range(0);
    
    // Create branches
    for (int i = 0; i < num_branches; i++) {
        std::string branch_name = "feature/bench_" + std::to_string(i);
        branch_manager_->createBranch(branch_name, "main", "Benchmark branch");
    }
    
    for (auto _ : state) {
        auto stats = branch_manager_->getStats();
        benchmark::DoNotOptimize(stats);
    }
}

BENCHMARK_REGISTER_F(BranchManagerBenchmark, GetStats)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500);

// Benchmark: Branch name validation
BENCHMARK_DEFINE_F(BranchManagerBenchmark, ValidateBranchName)(benchmark::State& state) {
    std::vector<std::string> names = {
        "main",
        "feature/test",
        "bugfix/issue-123",
        "release_v1.0",
        "invalid@name",
        "HEAD"
    };
    
    int idx = 0;
    for (auto _ : state) {
        std::string name = names[idx % names.size()];
        bool valid = BranchManager::isValidBranchName(name);
        benchmark::DoNotOptimize(valid);
        idx++;
    }
}

BENCHMARK_REGISTER_F(BranchManagerBenchmark, ValidateBranchName);

// Benchmark: JSON serialization
BENCHMARK_F(BranchManagerBenchmark, JsonSerialization)(benchmark::State& state) {
    auto branch = branch_manager_->createBranch(
        "feature/json_test",
        "main",
        "JSON test branch"
    );
    
    for (auto _ : state) {
        auto json_obj = branch->toJson();
        auto deserialized = BranchManager::Branch::fromJson(json_obj);
        benchmark::DoNotOptimize(deserialized);
    }
}

// Benchmark: Merge branches (fast-forward)
BENCHMARK_F(BranchManagerBenchmark, MergeBranchesFastForward)(benchmark::State& state) {
    state.PauseTiming();
    int merge_count = 0;
    
    for (auto _ : state) {
        // Create source and target branches
        std::string source_name = "source_" + std::to_string(merge_count);
        std::string target_name = "target_" + std::to_string(merge_count);
        merge_count++;
        
        branch_manager_->createBranch(source_name, "main", "Source");
        
        BranchManager::CreateBranchOptions options;
        options.from_sequence = 1;
        branch_manager_->createBranch(target_name, "main", "Target", "system", options);
        
        state.ResumeTiming();
        // Perform merge
        auto result = branch_manager_->mergeBranches(source_name, target_name);
        benchmark::DoNotOptimize(result);
        state.PauseTiming();
    }
}

} // namespace transaction
} // namespace themis

BENCHMARK_MAIN();
