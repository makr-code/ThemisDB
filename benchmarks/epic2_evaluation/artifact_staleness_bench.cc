/**
 * @file artifact_staleness_bench.cc
 * @brief Performance benchmarks for EPIC 2.6 Artifact Lifecycle staleness detection.
 *
 * Benchmarks measure:
 * - Staleness state computation latency
 * - Batch staleness detection throughput
 * - Filter and rebuild identification performance
 */

#include <benchmark/benchmark.h>

#include "artifact_lifecycle.h"

namespace themis {
namespace evaluation {
namespace {

// ---------------------------------------------------------------------------
// Benchmark Fixtures
// ---------------------------------------------------------------------------

class ArtifactStalenessFixture : public benchmark::Fixture {
 protected:
    ArtifactLifecycleManager manager_;

    LifecycleMetadata createMetadata(const std::string& id, std::uint32_t age_ms = 1000) {
        return LifecycleMetadata{
            .artifact_id = id,
            .state = LifecycleState::READY,
            .invalidation_reason = InvalidationReason::UNKNOWN,
            .source_seq_start = 1000,
            .source_seq_end = 2000,
            .delta_lag = 100,
            .artifact_age_ms = age_ms,
            .approximation_residual = 0.01,
            .residual_variance = 0.001,
            .max_permissible_rank = 100,
        };
    }
};

// ---------------------------------------------------------------------------
// Single Artifact Benchmarks
// ---------------------------------------------------------------------------

BENCHMARK_F(ArtifactStalenessFixture, BM_ComputeState_SingleArtifact_EmptyPolicy)
(benchmark::State& state) {
    auto metadata = createMetadata("test1");
    StalenessPolicy policy;  // Empty policy

    for (auto _ : state) {
        auto result = manager_.computeState(metadata, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_ComputeState_SingleArtifact_WithAgeThreshold)
(benchmark::State& state) {
    auto metadata = createMetadata("test1");
    StalenessPolicy policy;
    policy.withAgeThresholdMs(5000);

    for (auto _ : state) {
        auto result = manager_.computeState(metadata, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture,
             BM_ComputeState_SingleArtifact_WithMultipleThresholds)
(benchmark::State& state) {
    auto metadata = createMetadata("test1");
    StalenessPolicy policy;
    policy.withAgeThresholdMs(5000)
        .withDeltaLagThreshold(1000)
        .withResidualThreshold(0.05)
        .withRankCapThreshold(200);

    for (auto _ : state) {
        auto result = manager_.computeState(metadata, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_DiagnoseStalenessCause_SingleArtifact)
(benchmark::State& state) {
    auto metadata = createMetadata("test1", 3000);  // Age exceeds threshold
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);

    for (auto _ : state) {
        auto result = manager_.diagnoseStalenessCause(metadata, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_InvalidateArtifact)
(benchmark::State& state) {
    auto metadata = createMetadata("test1");

    for (auto _ : state) {
        auto result = ArtifactLifecycleManager::invalidate(
            metadata, InvalidationReason::INTEGRITY_CHECK_FAILED);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_BeginRebuild)
(benchmark::State& state) {
    auto metadata = createMetadata("test1");
    metadata.state = LifecycleState::INVALIDATED;

    for (auto _ : state) {
        auto result = ArtifactLifecycleManager::beginRebuild(metadata);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_CompleteRebuildSuccess)
(benchmark::State& state) {
    auto metadata = createMetadata("test1");
    metadata.state = LifecycleState::REBUILDING;

    for (auto _ : state) {
        auto result = ArtifactLifecycleManager::completeRebuildSuccess(
            metadata, 500, 50, 0.005);
        benchmark::DoNotOptimize(result);
    }
}

// ---------------------------------------------------------------------------
// Batch Operation Benchmarks
// ---------------------------------------------------------------------------

BENCHMARK_F(ArtifactStalenessFixture, BM_ComputeStatesBatch_100Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 100; ++i) {
        batch.push_back(createMetadata("artifact_" + std::to_string(i)));
    }

    StalenessPolicy policy;
    policy.withAgeThresholdMs(5000);

    for (auto _ : state) {
        auto result = manager_.computeStatesBatch(batch, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_ComputeStatesBatch_1000Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 1000; ++i) {
        batch.push_back(createMetadata("artifact_" + std::to_string(i)));
    }

    StalenessPolicy policy;
    policy.withAgeThresholdMs(5000);

    for (auto _ : state) {
        auto result = manager_.computeStatesBatch(batch, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_FilterUsableArtifacts_100Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 100; ++i) {
        auto meta = createMetadata("artifact_" + std::to_string(i));
        if (i % 3 == 0) {
            meta.state = LifecycleState::INVALIDATED;
        } else if (i % 3 == 1) {
            meta.state = LifecycleState::STALE;
        }
        batch.push_back(meta);
    }

    StalenessPolicy policy;

    for (auto _ : state) {
        auto result = manager_.filterUsableArtifacts(batch, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_FilterUsableArtifacts_1000Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 1000; ++i) {
        auto meta = createMetadata("artifact_" + std::to_string(i));
        if (i % 3 == 0) {
            meta.state = LifecycleState::INVALIDATED;
        } else if (i % 3 == 1) {
            meta.state = LifecycleState::STALE;
        }
        batch.push_back(meta);
    }

    StalenessPolicy policy;

    for (auto _ : state) {
        auto result = manager_.filterUsableArtifacts(batch, policy);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_IdentifyRebuildCandidates_100Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 100; ++i) {
        auto meta = createMetadata("artifact_" + std::to_string(i));
        if (i % 5 == 0) {
            meta.state = LifecycleState::INVALIDATED;
        } else if (i % 5 == 1) {
            meta.state = LifecycleState::FAILED;
        }
        batch.push_back(meta);
    }

    for (auto _ : state) {
        auto result = manager_.identifyRebuildCandidates(batch);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ArtifactStalenessFixture, BM_IdentifyRebuildCandidates_1000Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 1000; ++i) {
        auto meta = createMetadata("artifact_" + std::to_string(i));
        if (i % 5 == 0) {
            meta.state = LifecycleState::INVALIDATED;
        } else if (i % 5 == 1) {
            meta.state = LifecycleState::FAILED;
        }
        batch.push_back(meta);
    }

    for (auto _ : state) {
        auto result = manager_.identifyRebuildCandidates(batch);
        benchmark::DoNotOptimize(result);
    }
}

// ---------------------------------------------------------------------------
// Complex Scenario Benchmarks
// ---------------------------------------------------------------------------

BENCHMARK_F(ArtifactStalenessFixture,
             BM_StalenessDetectionWorkload_RealisticMix_100Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 100; ++i) {
        auto meta = createMetadata("artifact_" + std::to_string(i));
        // Realistic mix: 70% READY, 20% STALE, 5% INVALIDATED, 5% FAILED
        if (i % 20 < 14) {
            meta.state = LifecycleState::READY;
            meta.artifact_age_ms = 500 + (i % 1000);
        } else if (i % 20 < 18) {
            meta.state = LifecycleState::STALE;
        } else if (i % 20 == 18) {
            meta.state = LifecycleState::INVALIDATED;
        } else {
            meta.state = LifecycleState::FAILED;
        }
        batch.push_back(meta);
    }

    StalenessPolicy policy;
    policy.withAgeThresholdMs(5000)
        .withDeltaLagThreshold(2000)
        .withResidualThreshold(0.05);

    for (auto _ : state) {
        // Compute states
        auto states = manager_.computeStatesBatch(batch, policy);

        // Filter usable
        auto usable = manager_.filterUsableArtifacts(batch, policy);

        // Identify rebuild candidates
        auto rebuild = manager_.identifyRebuildCandidates(batch);

        benchmark::DoNotOptimize(states);
        benchmark::DoNotOptimize(usable);
        benchmark::DoNotOptimize(rebuild);
    }
}

BENCHMARK_F(ArtifactStalenessFixture,
             BM_StalenessDetectionWorkload_RealisticMix_1000Artifacts)
(benchmark::State& state) {
    std::vector<LifecycleMetadata> batch;
    for (int i = 0; i < 1000; ++i) {
        auto meta = createMetadata("artifact_" + std::to_string(i));
        // Realistic mix
        if (i % 20 < 14) {
            meta.state = LifecycleState::READY;
            meta.artifact_age_ms = 500 + (i % 1000);
        } else if (i % 20 < 18) {
            meta.state = LifecycleState::STALE;
        } else if (i % 20 == 18) {
            meta.state = LifecycleState::INVALIDATED;
        } else {
            meta.state = LifecycleState::FAILED;
        }
        batch.push_back(meta);
    }

    StalenessPolicy policy;
    policy.withAgeThresholdMs(5000)
        .withDeltaLagThreshold(2000)
        .withResidualThreshold(0.05);

    for (auto _ : state) {
        auto states = manager_.computeStatesBatch(batch, policy);
        auto usable = manager_.filterUsableArtifacts(batch, policy);
        auto rebuild = manager_.identifyRebuildCandidates(batch);

        benchmark::DoNotOptimize(states);
        benchmark::DoNotOptimize(usable);
        benchmark::DoNotOptimize(rebuild);
    }
}

}  // namespace
}  // namespace evaluation
}  // namespace themis

BENCHMARK_MAIN();
