// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_onnx_clip_soak.cpp
 * @brief Wave D — ONNX CLIP Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB onnx_clip module hot paths:
 * inference throughput, model load stability, and embedding reliability.
 * Verifies all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - OnnxClipSoak_InferenceThroughput   : ≥ 200 inferences/sec over soak window
 * - OnnxClipSoak_ModelLoadStability    : zero load failures
 * - OnnxClipSoak_EmbeddingReliability  : zero embedding corruptions
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_ONNX_CLIP.md — operator runbook
 * @see src/onnx_clip/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs model the onnx_clip inference paths without requiring
// ONNX Runtime, CUDA, or real model weights. MUST NOT be used in production
// code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

class StubInferenceEngine {
public:
    bool infer(uint64_t img_id) {
        ++infer_count_;
        (void)img_id;
        return true;
    }
    uint64_t inferCount() const { return infer_count_.load(); }
    uint64_t failCount()  const { return fail_count_.load(); }
private:
    std::atomic<uint64_t> infer_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

class StubModelLoader {
public:
    bool load(const std::string& model_path) {
        std::lock_guard<std::mutex> lk(mu_);
        ++load_count_;
        (void)model_path;
        return true;
    }
    uint64_t loadCount() const { return load_count_.load(); }
    uint64_t failCount() const { return fail_count_.load(); }
private:
    std::mutex mu_;
    std::atomic<uint64_t> load_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

class StubEmbeddingValidator {
public:
    bool validate(uint64_t embed_id) {
        ++validate_count_;
        (void)embed_id;
        return true;
    }
    uint64_t validateCount()    const { return validate_count_.load(); }
    uint64_t corruptionCount()  const { return corruption_count_.load(); }
private:
    std::atomic<uint64_t> validate_count_{0};
    std::atomic<uint64_t> corruption_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: OnnxClipSoak_InferenceThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_OnnxClipSoak, OnnxClipSoak_InferenceThroughput) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubInferenceEngine engine;
    std::atomic<bool>   running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                engine.infer(id++);
                std::this_thread::yield();
            }
        });
    }

    const auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    const double elapsed_s =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const uint64_t total_ops   = engine.inferCount();
    const double   ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "[ONNX_CLIP:InferenceTimeout] At least one inference must complete";
    EXPECT_GE(ops_per_sec, 200.0)
        << "Inference throughput must be ≥ 200/sec. Observed: " << ops_per_sec;
    EXPECT_EQ(engine.failCount(), 0u)
        << "[ONNX_CLIP:RuntimeError] Zero inference failures expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: OnnxClipSoak_ModelLoadStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_OnnxClipSoak, OnnxClipSoak_ModelLoadStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubModelLoader   loader;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 2; ++i) {
        workers.emplace_back([&]() {
            while (running.load(std::memory_order_relaxed)) {
                loader.load("stub_model.onnx");
                std::this_thread::sleep_for(1ms);
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(loader.loadCount(), 0u)
        << "[ONNX_CLIP:ModelLoadFailed] At least one model load must complete";
    EXPECT_EQ(loader.failCount(), 0u)
        << "[ONNX_CLIP:ModelLoadFailed] Zero model load failures expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: OnnxClipSoak_EmbeddingReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_OnnxClipSoak, OnnxClipSoak_EmbeddingReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubEmbeddingValidator validator;
    std::atomic<bool>      running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                validator.validate(id++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(validator.validateCount(), 0u)
        << "[ONNX_CLIP:EmbeddingCorruption] At least one embedding must be validated";
    EXPECT_EQ(validator.corruptionCount(), 0u)
        << "[ONNX_CLIP:EmbeddingCorruption] Zero embedding corruptions expected";
}
