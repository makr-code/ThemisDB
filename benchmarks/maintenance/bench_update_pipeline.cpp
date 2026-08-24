/// @file bench_update_pipeline.cpp
/// @brief Performance benchmarks for the Updates module pipeline.
///
/// Covers the following process lines:
///   - UpdateStateMachine::transition()   – state machine throughput
///   - UpdateStateMachine::currentState() – state read latency
///   - ReleaseManifest::toJson() / fromJson() – manifest serialisation
///   - DeltaUpdateEngine::generatePatch() / applyPatch() – binary diff cost
///
/// Performance targets (src/updates/ROADMAP.md):
///   - State transition:    < 1 µs
///   - currentState():      < 100 ns
///   - Manifest JSON round-trip (5 files): < 200 µs
///   - generatePatch (1 MB): < 500 ms
///   - applyPatch (1 MB patch): < 100 ms

#include <benchmark/benchmark.h>
#include "updates/update_state_machine.h"
#include "updates/release_manifest.h"
#include "updates/delta_update_engine.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::updates;

// ============================================================================
// UpdateStateMachine – state transition throughput
// ============================================================================

static void BM_UpdateStateMachine_Transition(benchmark::State& state) {
    UpdateStateMachine sm;

    for (auto _ : state) {
        // Full happy-path cycle: IDLE → DOWNLOADING → VERIFYING → APPLYING → IDLE
        sm.transition(UpdateState::DOWNLOADING, "1.5.0", "bench");
        sm.transition(UpdateState::VERIFYING,   "1.5.0", "bench");
        sm.transition(UpdateState::APPLYING,    "1.5.0", "bench");
        sm.transition(UpdateState::IDLE,        "1.5.0", "bench");
        benchmark::DoNotOptimize(sm.currentState());
    }

    // Report 4 transitions per iteration
    state.SetItemsProcessed(state.iterations() * 4);
}

BENCHMARK(BM_UpdateStateMachine_Transition)->Unit(benchmark::kNanosecond);

// ============================================================================
// UpdateStateMachine – currentState() read cost
// ============================================================================

static void BM_UpdateStateMachine_CurrentState(benchmark::State& state) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "1.5.0", "bench");

    for (auto _ : state) {
        auto s = sm.currentState();
        benchmark::DoNotOptimize(s);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_UpdateStateMachine_CurrentState)->Unit(benchmark::kNanosecond);

// ============================================================================
// UpdateStateMachine – rollback path
// ============================================================================

static void BM_UpdateStateMachine_RollbackPath(benchmark::State& state) {
    for (auto _ : state) {
        UpdateStateMachine sm;
        sm.transition(UpdateState::DOWNLOADING, "1.5.0", "bench");
        sm.transition(UpdateState::VERIFYING,   "1.5.0", "bench");
        sm.transition(UpdateState::ROLLING_BACK, "1.5.0", "bench");
        sm.transition(UpdateState::IDLE,         "1.5.0", "bench");
        benchmark::DoNotOptimize(sm.currentState());
    }

    state.SetItemsProcessed(state.iterations() * 4);
}

BENCHMARK(BM_UpdateStateMachine_RollbackPath)->Unit(benchmark::kNanosecond);

// ============================================================================
// ReleaseManifest JSON serialisation round-trip
// ============================================================================

static void BM_ReleaseManifest_JsonRoundTrip(benchmark::State& state) {
    const int num_files = static_cast<int>(state.range(0));

    ReleaseManifest manifest;
    manifest.version          = "1.5.0";
    manifest.release_date     = std::chrono::system_clock::now();
    manifest.min_upgrade_from = "1.4.0";
    manifest.release_notes    = "Performance benchmark release.";

    for (int i = 0; i < num_files; ++i) {
        ReleaseFile f;
        f.path        = "bin/component_" + std::to_string(i);
        f.type        = "executable";
        f.sha256_hash = "deadbeef" + std::to_string(i);
        f.size_bytes  = 1024 * 1024;
        f.platform    = "linux";
        f.architecture = "x64";
        manifest.files.push_back(f);
    }

    for (auto _ : state) {
        auto json_obj = manifest.toJson();
        auto parsed   = ReleaseManifest::fromJson(json_obj);
        benchmark::DoNotOptimize(parsed);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("files=" + std::to_string(num_files));
}

BENCHMARK(BM_ReleaseManifest_JsonRoundTrip)
    ->Arg(1)
    ->Arg(5)
    ->Arg(20)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// DeltaUpdateEngine – generatePatch / applyPatch (small files, no I/O)
// ============================================================================

class DeltaEngineBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        work_dir_ = "./data/bench_delta_engine_tmp";
        if (std::filesystem::exists(work_dir_)) {
            std::filesystem::remove_all(work_dir_);
        }
        std::filesystem::create_directories(work_dir_);

        // Create a base file (1 KB of repeating bytes)
        base_path_  = work_dir_ + "/base.bin";
        patch_path_ = work_dir_ + "/patch.bin";
        target_path_= work_dir_ + "/target.bin";
        new_path_   = work_dir_ + "/new.bin";

        std::string base_data(1024, 'A');
        std::ofstream(base_path_, std::ios::binary) << base_data;

        // Target differs in the last 16 bytes
        std::string target_data = base_data;
        target_data.replace(1008, 16, std::string(16, 'Z'));
        std::ofstream(target_path_, std::ios::binary) << target_data;

        engine_ = std::make_unique<DeltaUpdateEngine>(work_dir_, work_dir_);
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        engine_.reset();
        if (std::filesystem::exists(work_dir_)) {
            std::filesystem::remove_all(work_dir_);
        }
    }

protected:
    std::string                          work_dir_;
    std::string                          base_path_;
    std::string                          patch_path_;
    std::string                          target_path_;
    std::string                          new_path_;
    std::unique_ptr<DeltaUpdateEngine>   engine_;
};

BENCHMARK_DEFINE_F(DeltaEngineBenchFixture, GeneratePatch)(benchmark::State& state) {
    for (auto _ : state) {
        bool ok = engine_->generatePatch(base_path_, target_path_, patch_path_);
        benchmark::DoNotOptimize(ok);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(DeltaEngineBenchFixture, GeneratePatch)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(200);

BENCHMARK_DEFINE_F(DeltaEngineBenchFixture, ApplyPatch)(benchmark::State& state) {
    // Pre-generate the patch once
    engine_->generatePatch(base_path_, target_path_, patch_path_);

    for (auto _ : state) {
        bool ok = engine_->applyPatch(base_path_, patch_path_, new_path_);
        benchmark::DoNotOptimize(ok);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(DeltaEngineBenchFixture, ApplyPatch)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(200);

BENCHMARK_MAIN();
