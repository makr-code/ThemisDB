/// @file bench_tensor_snapshot_rebuild.cpp
/// @brief Benchmark suite for snapshot extraction and rebuild latency
/// @author ThemisDB Implementation Team
/// @date 2026-07-02
///
/// Measures snapshot and rebuild performance:
/// - Snapshot extraction cost
/// - Rebuild latency by graph size
/// - Artifact publish/swap latency

#include <benchmark/benchmark.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include <vector>
#include <cstring>

namespace themis {
namespace distributed_tensor {
namespace bench {

// Mock snapshot operations
class SnapshotOperations {
public:
    struct Snapshot {
        std::vector<uint8_t> data;
        uint32_t checksum = 0;
    };

    Snapshot extractSnapshot(const std::vector<uint8_t>& artifact_data) {
        Snapshot snapshot;
        snapshot.data = artifact_data;
        snapshot.checksum = computeChecksum(artifact_data);
        return snapshot;
    }

    std::vector<uint8_t> rebuildFromSnapshot(const Snapshot& snapshot) {
        return snapshot.data;
    }

    std::vector<uint8_t> rebuildFromSnapshotWithDelta(
        const Snapshot& snapshot,
        const std::vector<uint8_t>& delta) {
        std::vector<uint8_t> result = snapshot.data;
        result.insert(result.end(), delta.begin(), delta.end());
        return result;
    }

    uint32_t computeChecksum(const std::vector<uint8_t>& data) {
        uint32_t crc = 0;
        for (const auto& byte : data) {
            crc = ((crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0)) ^ byte;
        }
        return crc;
    }

    bool verifySnapshot(const Snapshot& snapshot) {
        return computeChecksum(snapshot.data) == snapshot.checksum;
    }

    void simulateArtifactSwap(Snapshot& old_snapshot, const Snapshot& new_snapshot) {
        old_snapshot = new_snapshot;
    }
};

// ============================================================================
// Snapshot Rebuild Fixtures
// ============================================================================

class SnapshotRebuildFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        operations_ = std::make_unique<SnapshotOperations>();

        manifest_.artifact_id = "test:tensor:snapshot";
        manifest_.current_state = ArtifactLifecycleState::ACTIVE;

        // Create test artifact of 10MB
        artifact_data_.resize(10 * 1024 * 1024, 0xAB);

        // Create delta
        delta_data_.resize(1 * 1024 * 1024, 0xCD);

        // Extract baseline snapshot
        baseline_snapshot_ = operations_->extractSnapshot(artifact_data_);
    }

    std::unique_ptr<SnapshotOperations> operations_;
    ArtifactManifest manifest_;
    std::vector<uint8_t> artifact_data_;
    std::vector<uint8_t> delta_data_;
    SnapshotOperations::Snapshot baseline_snapshot_;
};

// ============================================================================
// Snapshot Extraction Cost
// ============================================================================

BENCHMARK_F(SnapshotRebuildFixture, SnapshotExtraction_10MB)(benchmark::State& state) {
    for (auto _ : state) {
        auto snapshot = operations_->extractSnapshot(artifact_data_);
        benchmark::DoNotOptimize(snapshot);
    }
    state.SetItemsProcessed(artifact_data_.size());
}

BENCHMARK_F(SnapshotRebuildFixture, SnapshotExtraction_VariableSize)(
    benchmark::State& state) {
    const int SIZE_MB = state.range(0);
    std::vector<uint8_t> test_data(SIZE_MB * 1024 * 1024, 0xAB);

    for (auto _ : state) {
        auto snapshot = operations_->extractSnapshot(test_data);
        benchmark::DoNotOptimize(snapshot);
    }
    state.SetItemsProcessed(test_data.size());
}
BENCHMARK_REGISTER_F(SnapshotRebuildFixture, SnapshotExtraction_VariableSize)
    ->Arg(1)   // 1 MB
    ->Arg(10)  // 10 MB
    ->Arg(50)  // 50 MB
    ->Arg(100) // 100 MB
    ->Arg(500) // 500 MB
    ->Arg(1024); // 1 GB

// ============================================================================
// Rebuild Latency by Graph Size
// ============================================================================

BENCHMARK_F(SnapshotRebuildFixture, RebuildLatency_FromSnapshot)(
    benchmark::State& state) {
    for (auto _ : state) {
        auto rebuilt = operations_->rebuildFromSnapshot(baseline_snapshot_);
        benchmark::DoNotOptimize(rebuilt);
    }
    state.SetItemsProcessed(baseline_snapshot_.data.size());
}

BENCHMARK_F(SnapshotRebuildFixture, RebuildLatency_WithDelta)(benchmark::State& state) {
    for (auto _ : state) {
        auto rebuilt = operations_->rebuildFromSnapshotWithDelta(baseline_snapshot_, delta_data_);
        benchmark::DoNotOptimize(rebuilt);
    }
    state.SetItemsProcessed(baseline_snapshot_.data.size() + delta_data_.size());
}

// ============================================================================
// Rebuild by Graph Size Scaling
// ============================================================================

BENCHMARK_F(SnapshotRebuildFixture, RebuildScaling_GraphSizeSweep)(
    benchmark::State& state) {
    const int GRAPH_SIZE_K = state.range(0);
    int artifact_size_bytes = GRAPH_SIZE_K * 1024;

    std::vector<uint8_t> scaled_artifact(artifact_size_bytes, 0xAB);
    auto snapshot = operations_->extractSnapshot(scaled_artifact);

    for (auto _ : state) {
        auto rebuilt = operations_->rebuildFromSnapshot(snapshot);
        benchmark::DoNotOptimize(rebuilt);
    }
    state.SetItemsProcessed(artifact_size_bytes);
}
BENCHMARK_REGISTER_F(SnapshotRebuildFixture, RebuildScaling_GraphSizeSweep)
    ->Arg(1)      // 1K
    ->Arg(10)     // 10K
    ->Arg(100)    // 100K
    ->Arg(1000)   // 1M
    ->Arg(10000)  // 10M
    ->Arg(100000) // 100M
    ->Arg(1000000); // 1G

// ============================================================================
// Artifact Publish/Swap Latency
// ============================================================================

BENCHMARK_F(SnapshotRebuildFixture, ArtifactPublishSwap_Latency)(benchmark::State& state) {
    auto old_snapshot = baseline_snapshot_;

    for (auto _ : state) {
        // Create new snapshot (simulates rebuild)
        std::vector<uint8_t> new_data = artifact_data_;
        new_data[0] = 0xFF;  // Minor modification
        auto new_snapshot = operations_->extractSnapshot(new_data);

        // Swap (atomic visibility)
        operations_->simulateArtifactSwap(old_snapshot, new_snapshot);
        benchmark::DoNotOptimize(old_snapshot);
    }
}

// ============================================================================
// Snapshot Verification
// ============================================================================

BENCHMARK_F(SnapshotRebuildFixture, SnapshotVerification)(benchmark::State& state) {
    for (auto _ : state) {
        bool is_valid = operations_->verifySnapshot(baseline_snapshot_);
        benchmark::DoNotOptimize(is_valid);
    }
}

// ============================================================================
// End-to-End Rebuild Workflow
// ============================================================================

BENCHMARK_F(SnapshotRebuildFixture, EndToEnd_SnapshotRebuildPublish)(
    benchmark::State& state) {
    for (auto _ : state) {
        // Extract
        auto snapshot = operations_->extractSnapshot(artifact_data_);

        // Apply delta
        auto rebuilt = operations_->rebuildFromSnapshotWithDelta(snapshot, delta_data_);

        // Verify
        uint32_t checksum = operations_->computeChecksum(rebuilt);

        // Publish (simulate)
        auto new_snapshot = SnapshotOperations::Snapshot{rebuilt, checksum};

        benchmark::DoNotOptimize(new_snapshot);
    }
    state.SetItemsProcessed(artifact_data_.size() + delta_data_.size());
}

} // namespace bench
} // namespace distributed_tensor
} // namespace themis
