/// @file bench_tensor_commit_overhead.cpp
/// @brief Benchmark suite for commit-path overhead with tensor delta logging
/// @author ThemisDB Implementation Team
/// @date 2026-07-02
///
/// Measures write-path overhead introduced by:
/// - Baseline RocksDB transaction
/// - Transaction + tensor delta logging
/// - Transaction + delta logging + manifest invalidation

#include <benchmark/benchmark.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include <random>
#include <chrono>
#include <vector>

namespace themis {
namespace distributed_tensor {
namespace bench {

// ============================================================================
// Mock RocksDB-like Storage
// ============================================================================

class MockRocksDBStore {
public:
    struct Transaction {
        std::vector<std::pair<std::string, std::string>> writes;
    };

    bool commit(const Transaction& tx) {
        for (const auto& [key, value] : tx.writes) {
            data_[key] = value;
        }
        return true;
    }

private:
    std::map<std::string, std::string> data_;
};

// ============================================================================
// Mock Delta Logger
// ============================================================================

class MockDeltaLogger {
public:
    bool logDelta(const std::string& artifact_id, const std::string& delta_data) {
        deltas_[artifact_id].push_back(delta_data);
        return true;
    }

private:
    std::map<std::string, std::vector<std::string>> deltas_;
};

// ============================================================================
// Commit Overhead Fixtures
// ============================================================================

class CommitOverheadFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        store_ = std::make_unique<MockRocksDBStore>();
        logger_ = std::make_unique<MockDeltaLogger>();

        // Initialize manifest
        manifest_.artifact_id = "test:tensor:commit";
        manifest_.artifact_class = ArtifactClass::DERIVED;
        manifest_.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
        manifest_.current_state = ArtifactLifecycleState::ACTIVE;
        manifest_.created_at_unix_sec = 1000;
        manifest_.last_verified_unix_sec = 1000;
        manifest_.staleness_threshold_sec = 3600;

        // Initialize test data
        test_key_ = "test:row:1";
        test_value_ = std::string(1024, 'A');  // 1KB value
        delta_data_ = std::string(512, 'B');   // 512B delta
    }

    std::unique_ptr<MockRocksDBStore> store_;
    std::unique_ptr<MockDeltaLogger> logger_;
    ArtifactManifest manifest_;
    std::string test_key_;
    std::string test_value_;
    std::string delta_data_;
};

// ============================================================================
// Baseline: RocksDB Transaction Only
// ============================================================================

BENCHMARK_F(CommitOverheadFixture, BaselineRocksDBTransaction)(benchmark::State& state) {
    // Measure baseline RocksDB transaction performance
    for (auto _ : state) {
        MockRocksDBStore::Transaction tx;
        tx.writes.push_back({test_key_, test_value_});

        store_->commit(tx);
        benchmark::DoNotOptimize(manifest_);
    }
}

// ============================================================================
// Transaction + Delta Logging
// ============================================================================

BENCHMARK_F(CommitOverheadFixture, TransactionWithDeltaLogging)(benchmark::State& state) {
    // Measure overhead of delta logging on write path
    for (auto _ : state) {
        MockRocksDBStore::Transaction tx;
        tx.writes.push_back({test_key_, test_value_});

        // Commit and log delta atomically
        store_->commit(tx);
        logger_->logDelta(manifest_.artifact_id, delta_data_);
        manifest_.source_sequence_current++;

        benchmark::DoNotOptimize(manifest_);
    }
}

// ============================================================================
// Transaction + Delta Logging + Manifest Invalidation
// ============================================================================

BENCHMARK_F(CommitOverheadFixture, TransactionWithDeltaAndManifestInvalidation)(
    benchmark::State& state) {
    // Measure overhead of delta logging + manifest state change
    for (auto _ : state) {
        MockRocksDBStore::Transaction tx;
        tx.writes.push_back({test_key_, test_value_});

        // Commit, log delta, and update manifest atomically
        store_->commit(tx);
        logger_->logDelta(manifest_.artifact_id, delta_data_);
        manifest_.source_sequence_current++;

        // Simulate manifest invalidation (state change)
        manifest_.current_state = ArtifactLifecycleState::STALE;
        manifest_.last_verified_unix_sec -= 1000;

        benchmark::DoNotOptimize(manifest_);
    }
}

// ============================================================================
// Batch Commit Tests
// ============================================================================

BENCHMARK_F(CommitOverheadFixture, BatchCommitBaseline)(benchmark::State& state) {
    // Measure baseline batch performance
    const int BATCH_SIZE = state.range(0);

    for (auto _ : state) {
        MockRocksDBStore::Transaction tx;
        for (int i = 0; i < BATCH_SIZE; ++i) {
            tx.writes.push_back({test_key_ + std::to_string(i), test_value_});
        }

        store_->commit(tx);
        benchmark::DoNotOptimize(manifest_);
    }
}
BENCHMARK_REGISTER_F(CommitOverheadFixture, BatchCommitBaseline)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100);

BENCHMARK_F(CommitOverheadFixture, BatchCommitWithDeltaLogging)(benchmark::State& state) {
    // Measure batch commit with delta logging
    const int BATCH_SIZE = state.range(0);

    for (auto _ : state) {
        MockRocksDBStore::Transaction tx;
        for (int i = 0; i < BATCH_SIZE; ++i) {
            tx.writes.push_back({test_key_ + std::to_string(i), test_value_});
        }

        store_->commit(tx);

        for (int i = 0; i < BATCH_SIZE; ++i) {
            logger_->logDelta(manifest_.artifact_id, delta_data_);
        }
        manifest_.source_sequence_current += BATCH_SIZE;

        benchmark::DoNotOptimize(manifest_);
    }
}
BENCHMARK_REGISTER_F(CommitOverheadFixture, BatchCommitWithDeltaLogging)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100);

// ============================================================================
// Variable Payload Size Tests
// ============================================================================

class PayloadSizeFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        store_ = std::make_unique<MockRocksDBStore>();
        logger_ = std::make_unique<MockDeltaLogger>();

        manifest_.artifact_id = "test:tensor:payload";
        manifest_.source_sequence_current = 0;
    }

    std::unique_ptr<MockRocksDBStore> store_;
    std::unique_ptr<MockDeltaLogger> logger_;
    ArtifactManifest manifest_;
};

BENCHMARK_F(PayloadSizeFixture, DeltaLoggingPayloadSweep)(benchmark::State& state) {
    // Measure delta logging overhead for various payload sizes
    const int PAYLOAD_SIZE = state.range(0);
    std::string delta_data(PAYLOAD_SIZE, 'X');
    std::string test_key = "test:key";
    std::string test_value(PAYLOAD_SIZE, 'Y');

    for (auto _ : state) {
        MockRocksDBStore::Transaction tx;
        tx.writes.push_back({test_key, test_value});

        store_->commit(tx);
        logger_->logDelta(manifest_.artifact_id, delta_data);
        manifest_.source_sequence_current++;

        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(PAYLOAD_SIZE);
}
BENCHMARK_REGISTER_F(PayloadSizeFixture, DeltaLoggingPayloadSweep)
    ->Arg(256)      // 256 bytes
    ->Arg(1024)     // 1 KB
    ->Arg(10240)    // 10 KB
    ->Arg(102400)   // 100 KB
    ->Arg(1048576); // 1 MB

} // namespace bench
} // namespace distributed_tensor
} // namespace themis
