/// @file bench_tensor_update_worker.cpp
/// @brief Benchmark suite for update worker throughput and latency
/// @author ThemisDB Implementation Team
/// @date 2026-07-02
///
/// Measures update worker performance for:
/// - Patch path (small deltas)
/// - Partial refit path
/// - Full rebuild path
/// - Rank growth sensitivity

#include <benchmark/benchmark.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include <vector>
#include <random>

namespace themis {
namespace distributed_tensor {
namespace bench {

// Mock update work simulation
class UpdateWorkSimulator {
public:
    enum TaskType { PATCH, REFIT, REBUILD };

    void simulatePatch(int delta_size_bytes) {
        // Simulate patch application: O(delta_size)
        std::vector<uint8_t> buffer(delta_size_bytes);
        benchmark::DoNotOptimize(buffer.data());
    }

    void simulateRefit(int artifact_size_bytes, float refit_ratio) {
        // Simulate refit: O(artifact_size * refit_ratio)
        int refit_size = static_cast<int>(artifact_size_bytes * refit_ratio);
        std::vector<uint8_t> buffer(refit_size);
        benchmark::DoNotOptimize(buffer.data());
    }

    void simulateRebuild(int artifact_size_bytes) {
        // Simulate rebuild: O(artifact_size)
        std::vector<uint8_t> buffer(artifact_size_bytes);
        benchmark::DoNotOptimize(buffer.data());
    }

    void updateRankMetrics(int rank) {
        rank_ = rank;
        quality_score_ = 0.95 - (0.01 * (rank / 8));
    }

private:
    int rank_ = 16;
    double quality_score_ = 0.95;
};

// ============================================================================
// Update Worker Fixtures
// ============================================================================

class UpdateWorkerFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        simulator_ = std::make_unique<UpdateWorkSimulator>();

        manifest_.artifact_id = "test:tensor:worker";
        manifest_.rank_cap = 16;
        manifest_.current_state = ArtifactLifecycleState::ACTIVE;

        artifact_size_ = 10 * 1024 * 1024;  // 10MB
    }

    std::unique_ptr<UpdateWorkSimulator> simulator_;
    ArtifactManifest manifest_;
    int artifact_size_;
};

// ============================================================================
// Patch Path Performance
// ============================================================================

BENCHMARK_F(UpdateWorkerFixture, PatchPath_SmallDelta)(benchmark::State& state) {
    const int DELTA_SIZE = 1024;  // 1 KB

    for (auto _ : state) {
        simulator_->simulatePatch(DELTA_SIZE);
        manifest_.source_sequence_current++;
        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(DELTA_SIZE);
}

BENCHMARK_F(UpdateWorkerFixture, PatchPath_MediumDelta)(benchmark::State& state) {
    const int DELTA_SIZE = 100 * 1024;  // 100 KB

    for (auto _ : state) {
        simulator_->simulatePatch(DELTA_SIZE);
        manifest_.source_sequence_current++;
        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(DELTA_SIZE);
}

// ============================================================================
// Partial Refit Performance
// ============================================================================

BENCHMARK_F(UpdateWorkerFixture, RefitPath_10Percent)(benchmark::State& state) {
    for (auto _ : state) {
        simulator_->simulateRefit(artifact_size_, 0.1f);
        manifest_.source_sequence_current++;
        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(artifact_size_ / 10);
}

BENCHMARK_F(UpdateWorkerFixture, RefitPath_25Percent)(benchmark::State& state) {
    for (auto _ : state) {
        simulator_->simulateRefit(artifact_size_, 0.25f);
        manifest_.source_sequence_current++;
        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(artifact_size_ / 4);
}

BENCHMARK_F(UpdateWorkerFixture, RefitPath_50Percent)(benchmark::State& state) {
    for (auto _ : state) {
        simulator_->simulateRefit(artifact_size_, 0.5f);
        manifest_.source_sequence_current++;
        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(artifact_size_ / 2);
}

// ============================================================================
// Full Rebuild Performance
// ============================================================================

BENCHMARK_F(UpdateWorkerFixture, RebuildPath_Full)(benchmark::State& state) {
    for (auto _ : state) {
        simulator_->simulateRebuild(artifact_size_);
        manifest_.current_state = ArtifactLifecycleState::REBUILT;
        manifest_.source_sequence_current++;
        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(artifact_size_);
}

// ============================================================================
// Rank Growth Sensitivity
// ============================================================================

BENCHMARK_F(UpdateWorkerFixture, RankGrowthSensitivity)(benchmark::State& state) {
    const int RANK = state.range(0);

    for (auto _ : state) {
        simulator_->updateRankMetrics(RANK);
        simulator_->simulateRebuild(artifact_size_);
        manifest_.rank_cap = RANK;
        benchmark::DoNotOptimize(manifest_);
    }
    state.SetItemsProcessed(artifact_size_);
}
BENCHMARK_REGISTER_F(UpdateWorkerFixture, RankGrowthSensitivity)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64);

// ============================================================================
// Mixed Workload
// ============================================================================

BENCHMARK_F(UpdateWorkerFixture, MixedWorkload_PatchRefit)(benchmark::State& state) {
    for (auto _ : state) {
        // 70% patches, 30% refits
        if (state.iterations() % 10 < 7) {
            simulator_->simulatePatch(50 * 1024);
        } else {
            simulator_->simulateRefit(artifact_size_, 0.1f);
        }
        manifest_.source_sequence_current++;
        benchmark::DoNotOptimize(manifest_);
    }
}

} // namespace bench
} // namespace distributed_tensor
} // namespace themis
