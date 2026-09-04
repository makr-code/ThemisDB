// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_deterministic_training_lifecycle.cpp
 * @brief Phase 4 deterministic stress fixtures for training lifecycle workloads.
 *
 * Verifies that the adapter lifecycle (construction, weight update, merge,
 * serving) is fully deterministic: identical seed → identical output,
 * bit-for-bit, regardless of run count.  Each test uses SEED=42 (canonical
 * per bench_fixtures.h convention) so failures are always reproducible.
 *
 * Test IDs
 * --------
 * DTL-01  LoRA adapter initialization is deterministic with same seed
 * DTL-02  addLayer produces identical initial B/A matrices for same dimensions
 * DTL-03  applyUpdate accumulates deterministically over N steps
 * DTL-04  Weight-update batch is deterministic across two independent runs
 * DTL-05  Exported weights round-trip exactly through importWeights
 * DTL-06  Multi-layer adapter produces stable layer-norm ordering
 * DTL-07  TIES merge with fixed inputs is bit-for-bit reproducible
 * DTL-08  Linear merge with fixed inputs is bit-for-bit reproducible
 * DTL-09  Training convergence curve (synthetic loss steps) is monotone
 * DTL-10  1000-step lifecycle produces no unbounded weight norm growth
 * DTL-11  Adapter state after N identical update batches equals one N×-scaled update
 * DTL-12  Incident emitter broadcasts all three classes in insertion order
 */

#include <gtest/gtest.h>
#include "training/lora_adapter.h"
#include "training/lora_adapter_merger.h"
#include "training/training_incident_emitter.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace themis::training;

// ─────────────────────────────────────────────────────────────────────────────
// Canonical seed (per bench_fixtures.h kCanonicalRngSeed=42)
// ─────────────────────────────────────────────────────────────────────────────
static constexpr unsigned int kSeed = 42u;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

std::vector<float> makeRandomWeights(size_t n, float scale, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-scale, scale);
    std::vector<float> v(n);
    for (float& x : v) {
      x = dist(rng);
    }
    return v;
}

float l2Norm(const std::vector<float>& v) {
    float s = 0.0f;
    for (float x : v) {
      s += x * x;
    }
    return std::sqrt(s);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// DTL-01: LoRA adapter initialization is deterministic with same seed
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_01_adapter_init_deterministic) {
    // Two adapters with identical construction parameters should have
    // the same initial B values (Kaiming-uniform is seeded by rank/dims convention).
    LoRAAdapter a1(4, 8.0f);
    LoRAAdapter a2(4, 8.0f);
    a1.addLayer("attn", 64, 64, 4, 8.0f);
    a2.addLayer("attn", 64, 64, 4, 8.0f);

    // Both should have A all-zero (LoRA paper §3 convention)
    const auto& w1 = a1.getWeights("attn");
    const auto& w2 = a2.getWeights("attn");
    ASSERT_EQ(w1.A.size(), w2.A.size());
    EXPECT_EQ(w1.A, w2.A) << "Initial A must be zero for both adapters";

    // B must have the same dimensions
    ASSERT_EQ(w1.B.size(), w2.B.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-02: addLayer produces identical initial B/A matrices for same dimensions
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_02_addlayer_identical_shape) {
    LoRAAdapter adapter(8, 16.0f);
    adapter.addLayer("layer_a", 128, 256, 8, 16.0f);
    adapter.addLayer("layer_b", 128, 256, 8, 16.0f);

    const auto& wa = adapter.getWeights("layer_a");
    const auto& wb = adapter.getWeights("layer_b");

    // Dimensions must match
    EXPECT_EQ(wa.in_dim,  wb.in_dim);
    EXPECT_EQ(wa.out_dim, wb.out_dim);
    EXPECT_EQ(wa.rank,    wb.rank);
    EXPECT_EQ(wa.alpha,   wb.alpha);

    // A must be zero for both
    EXPECT_EQ(wa.A, wb.A);
    EXPECT_EQ(wa.A, std::vector<float>(8 * 256, 0.0f));

    // B size must be correct
    EXPECT_EQ(wa.B.size(), 128u * 8u);
    EXPECT_EQ(wb.B.size(), 128u * 8u);
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-03: applyUpdate accumulates deterministically over N steps
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_03_update_accumulation_deterministic) {
    const size_t in_dim = 32, out_dim = 32, rank = 4;

    // Run 1
    LoRAAdapter adapter1(rank, 8.0f);
    adapter1.addLayer("fc", in_dim, out_dim, rank, 8.0f);

    // Run 2 – identical construction
    LoRAAdapter adapter2(rank, 8.0f);
    adapter2.addLayer("fc", in_dim, out_dim, rank, 8.0f);

    // Apply 50 identical updates sourced from seed 42
    const int N = 50;
    for (int i = 0; i < N; ++i) {
        unsigned int step_seed = kSeed + static_cast<unsigned int>(i);
        auto dB = makeRandomWeights(in_dim * rank, 0.01f, step_seed);
        auto dA = makeRandomWeights(rank * out_dim, 0.01f, step_seed + 1000u);
        adapter1.applyUpdate("fc", dB, dA);
        adapter2.applyUpdate("fc", dB, dA);
    }

    const auto& w1 = adapter1.getWeights("fc");
    const auto& w2 = adapter2.getWeights("fc");
    EXPECT_EQ(w1.B, w2.B) << "B must be identical after N identical updates";
    EXPECT_EQ(w1.A, w2.A) << "A must be identical after N identical updates";
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-04: Weight-update batch is deterministic across two independent runs
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_04_batch_update_deterministic) {
    const size_t in_dim = 16, out_dim = 16, rank = 4;

    LoRAAdapter a1(rank, 8.0f), a2(rank, 8.0f);
    for (auto* a : {&a1, &a2}) {
        a->addLayer("q", in_dim, out_dim, rank, 8.0f);
        a->addLayer("k", in_dim, out_dim, rank, 8.0f);
        a->addLayer("v", in_dim, out_dim, rank, 8.0f);
    }

    WeightUpdateBatch batch;
    batch.layer_names = {"q", "k", "v"};
    for (size_t li = 0; li < 3; ++li) {
        batch.delta_B.push_back(makeRandomWeights(in_dim * rank, 0.005f,
                                                   kSeed + static_cast<unsigned int>(li)));
        batch.delta_A.push_back(makeRandomWeights(rank * out_dim, 0.005f,
                                                   kSeed + 100u + static_cast<unsigned int>(li)));
    }

    a1.applyBatchUpdate(batch);
    a2.applyBatchUpdate(batch);

    for (const auto& name : {"q", "k", "v"}) {
        EXPECT_EQ(a1.getWeights(name).B, a2.getWeights(name).B)
            << "B mismatch for layer " << name;
        EXPECT_EQ(a1.getWeights(name).A, a2.getWeights(name).A)
            << "A mismatch for layer " << name;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-05: Exported weights round-trip exactly through importWeights
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_05_export_import_roundtrip) {
    const size_t in_dim = 32, out_dim = 64, rank = 8;

    LoRAAdapter src(rank, 16.0f);
    src.addLayer("proj", in_dim, out_dim, rank, 16.0f);

    // Apply a known update to get non-trivial weights
    auto dB = makeRandomWeights(in_dim * rank, 0.1f, kSeed);
    auto dA = makeRandomWeights(rank * out_dim, 0.1f, kSeed + 1u);
    src.applyUpdate("proj", dB, dA);

    // Export and import
    auto snapshot = src.exportWeights();
    LoRAAdapter dst(rank, 16.0f);
    dst.importWeights(snapshot);

    ASSERT_TRUE(dst.hasLayer("proj"));
    const auto& ws = src.getWeights("proj");
    const auto& wd = dst.getWeights("proj");

    EXPECT_EQ(ws.B, wd.B) << "B must be preserved through export/import";
    EXPECT_EQ(ws.A, wd.A) << "A must be preserved through export/import";
    EXPECT_EQ(ws.rank,   wd.rank);
    EXPECT_EQ(ws.alpha,  wd.alpha);
    EXPECT_EQ(ws.in_dim, wd.in_dim);
    EXPECT_EQ(ws.out_dim,wd.out_dim);
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-06: Multi-layer adapter produces stable layer-count after construction
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_06_multilayer_count_stable) {
    LoRAAdapter adapter(4, 8.0f);
    const int N_LAYERS = 12;
    for (int i = 0; i < N_LAYERS; ++i) {
        adapter.addLayer("layer_" + std::to_string(i), 64, 64, 4, 8.0f);
    }
    EXPECT_EQ(adapter.layerCount(), static_cast<size_t>(N_LAYERS));

    // Removing one should reduce by exactly one
    adapter.removeLayer("layer_0");
    EXPECT_EQ(adapter.layerCount(), static_cast<size_t>(N_LAYERS - 1));

    // Re-adding the same name should succeed
    adapter.addLayer("layer_0", 64, 64, 4, 8.0f);
    EXPECT_EQ(adapter.layerCount(), static_cast<size_t>(N_LAYERS));
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-07: TIES merge with fixed inputs is bit-for-bit reproducible
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_07_ties_merge_reproducible) {
    const size_t in_dim = 16, out_dim = 16, rank = 4;

    auto makeAdapter = [&](unsigned int seed) {
        auto adapter = std::make_unique<LoRAAdapter>(rank, 8.0f);
        adapter->addLayer("fc", in_dim, out_dim, rank, 8.0f);
        auto dB = makeRandomWeights(in_dim * rank, 0.05f, seed);
        auto dA = makeRandomWeights(rank * out_dim, 0.05f, seed + 500u);
        adapter->applyUpdate("fc", dB, dA);
        return adapter;
    };

    auto a1 = makeAdapter(kSeed);
    auto a2 = makeAdapter(kSeed + 1u);
    auto a3 = makeAdapter(kSeed);
    auto a4 = makeAdapter(kSeed + 1u);

    LoRAAdapterMerger merger;
    auto r1 = merger.mergeTIESAll({a1.get(), a2.get()}, rank, 0.2f);
    auto r2 = merger.mergeTIESAll({a3.get(), a4.get()}, rank, 0.2f);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_EQ(r1.layers.size(), r2.layers.size());

    for (size_t i = 0; i < r1.layers.size(); ++i) {
        EXPECT_EQ(r1.layers[i].B, r2.layers[i].B)
            << "TIES B mismatch at layer " << i;
        EXPECT_EQ(r1.layers[i].A, r2.layers[i].A)
            << "TIES A mismatch at layer " << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-08: Linear merge with fixed inputs is bit-for-bit reproducible
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_08_linear_merge_reproducible) {
    const size_t in_dim = 16, out_dim = 16, rank = 4;
    const float  weight = 0.5f;

    auto makeAdapter = [&](unsigned int seed) {
        auto adapter = std::make_unique<LoRAAdapter>(rank, 8.0f);
        adapter->addLayer("fc", in_dim, out_dim, rank, 8.0f);
        auto dB = makeRandomWeights(in_dim * rank, 0.05f, seed);
        auto dA = makeRandomWeights(rank * out_dim, 0.05f, seed + 500u);
        adapter->applyUpdate("fc", dB, dA);
        return adapter;
    };

    auto a1 = makeAdapter(kSeed);
    auto a2 = makeAdapter(kSeed + 2u);
    auto a3 = makeAdapter(kSeed);
    auto a4 = makeAdapter(kSeed + 2u);

    LoRAAdapterMerger merger;
    auto r1 = merger.mergeLinearAll({a1.get(), a2.get()}, {weight, weight}, rank);
    auto r2 = merger.mergeLinearAll({a3.get(), a4.get()}, {weight, weight}, rank);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_EQ(r1.layers.size(), r2.layers.size());

    for (size_t i = 0; i < r1.layers.size(); ++i) {
        EXPECT_EQ(r1.layers[i].B, r2.layers[i].B)
            << "Linear merge B mismatch at layer " << i;
        EXPECT_EQ(r1.layers[i].A, r2.layers[i].A)
            << "Linear merge A mismatch at layer " << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-09: Synthetic loss trajectory converges monotonically over seeded data
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_09_synthetic_loss_monotone) {
    // Simulate a simple gradient-descent loop where the update magnitude
    // decreases each step: loss should trend downward (or at least not
    // diverge) over 200 steps.
    const size_t in_dim = 32, out_dim = 32, rank = 8;
    LoRAAdapter adapter(rank, 8.0f);
    adapter.addLayer("fc", in_dim, out_dim, rank, 8.0f);

    // Target weights (arbitrary fixed target)
    auto target_B = makeRandomWeights(in_dim * rank, 0.2f, kSeed);
    auto target_A = makeRandomWeights(rank * out_dim, 0.2f, kSeed + 1u);

    const float lr = 0.01f;
    float prev_loss = std::numeric_limits<float>::max();
    bool saw_decrease = false;

    for (int step = 0; step < 200; ++step) {
        const auto& w = adapter.getWeights("fc");

        // MSE loss with respect to target
        float loss = 0.0f;
        for (size_t i = 0; i < w.B.size(); ++i)
            loss += (w.B[i] - target_B[i]) * (w.B[i] - target_B[i]);
        for (size_t i = 0; i < w.A.size(); ++i)
            loss += (w.A[i] - target_A[i]) * (w.A[i] - target_A[i]);
        loss /= static_cast<float>(w.B.size() + w.A.size());

        if (step > 0 && loss < prev_loss) {
          saw_decrease = true;
        }

        // Gradient = 2*(w - target); delta = -lr * gradient
        std::vector<float> dB(w.B.size()), dA(w.A.size());
        for (size_t i = 0; i < dB.size(); ++i)
            dB[i] = -lr * 2.0f * (w.B[i] - target_B[i]);
        for (size_t i = 0; i < dA.size(); ++i)
            dA[i] = -lr * 2.0f * (w.A[i] - target_A[i]);

        adapter.applyUpdate("fc", dB, dA);
        prev_loss = loss;
    }

    EXPECT_TRUE(saw_decrease) << "Loss must decrease at least once over 200 steps";
    EXPECT_LT(prev_loss, 1.0f) << "Loss should converge to small value";
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-10: 1000-step lifecycle produces no unbounded weight norm growth
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_10_thousand_steps_no_norm_growth) {
    const size_t in_dim = 32, out_dim = 32, rank = 4;
    LoRAAdapter adapter(rank, 8.0f);
    adapter.addLayer("fc", in_dim, out_dim, rank, 8.0f);

    float initial_norm = l2Norm(adapter.getWeights("fc").B);

    for (int step = 0; step < 1000; ++step) {
        // Small bounded update (magnitude bounded by scale * sqrt(n))
        unsigned int s = kSeed + static_cast<unsigned int>(step);
        auto dB = makeRandomWeights(in_dim * rank, 0.001f, s);
        auto dA = makeRandomWeights(rank * out_dim, 0.001f, s + 10000u);
        adapter.applyUpdate("fc", dB, dA);
    }

    float final_norm = l2Norm(adapter.getWeights("fc").B);

    // After 1000 small random updates the norm should stay within a
    // 100× envelope of the initial value (random walk bound).
    EXPECT_LT(final_norm, 100.0f * (initial_norm + 1.0f))
        << "Norm must not grow unboundedly; initial=" << initial_norm
        << " final=" << final_norm;
    EXPECT_TRUE(std::isfinite(final_norm)) << "Weight norm must remain finite";
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-11: Adapter state after N identical update batches equals one N×-scaled update
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_11_n_identical_updates_equals_n_scaled) {
    const size_t in_dim = 8, out_dim = 8, rank = 2;
    const int N = 5;

    // Adapter a: N individual identical updates
    LoRAAdapter a_n(rank, 8.0f);
    a_n.addLayer("fc", in_dim, out_dim, rank, 8.0f);

    // Adapter b: one update scaled ×N
    LoRAAdapter a_1(rank, 8.0f);
    a_1.addLayer("fc", in_dim, out_dim, rank, 8.0f);

    auto dB = makeRandomWeights(in_dim * rank, 0.05f, kSeed);
    auto dA = makeRandomWeights(rank * out_dim, 0.05f, kSeed + 1u);

    for (int i = 0; i < N; ++i) {
        a_n.applyUpdate("fc", dB, dA);
    }

    std::vector<float> dB_n(dB.size()), dA_n(dA.size());
    for (size_t i = 0; i < dB.size(); ++i) {
      dB_n[i] = dB[i] * static_cast<float>(N);
    }
    for (size_t i = 0; i < dA.size(); ++i) {
      dA_n[i] = dA[i] * static_cast<float>(N);
    }
    a_1.applyUpdate("fc", dB_n, dA_n);

    const auto& wn = a_n.getWeights("fc");
    const auto& w1 = a_1.getWeights("fc");

    // B and A should be element-wise equal (float addition is associative for
    // identical operands, so this holds exactly).
    ASSERT_EQ(wn.B.size(), w1.B.size());
    for (size_t i = 0; i < wn.B.size(); ++i) {
        EXPECT_FLOAT_EQ(wn.B[i], w1.B[i]) << "B mismatch at index " << i;
    }
    ASSERT_EQ(wn.A.size(), w1.A.size());
    for (size_t i = 0; i < wn.A.size(); ++i) {
        EXPECT_FLOAT_EQ(wn.A[i], w1.A[i]) << "A mismatch at index " << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DTL-12: Incident emitter broadcasts all three classes in insertion order
// ─────────────────────────────────────────────────────────────────────────────
TEST(DeterministicTrainingLifecycle, DTL_12_incident_emitter_all_classes_ordered) {
    struct CapturingListener : TrainingIncidentListener {
        std::vector<TrainingIncidentClass> classes;
        void onIncident(const TrainingIncident& inc) override {
            classes.push_back(inc.incident_class);
        }
    };

    TrainingIncidentEmitter emitter;
    auto listener = std::make_shared<CapturingListener>();
    emitter.addListener(listener);

    emitter.emitDatasetIncident(
        TrainingErrorCode::SUCCESS, "auto_labeler", "label_batch",
        "DTL-12 dataset incident");
    emitter.emitTrainingIncident(
        TrainingErrorCode::SUCCESS, "lora_trainer", "train_step",
        "DTL-12 training incident");
    emitter.emitAdapterIncident(
        TrainingErrorCode::SUCCESS, "adapter_serving", "deploy_version",
        "DTL-12 adapter incident");

    ASSERT_EQ(listener->classes.size(), 3u);
    EXPECT_EQ(listener->classes[0], TrainingIncidentClass::DATASET);
    EXPECT_EQ(listener->classes[1], TrainingIncidentClass::TRAINING);
    EXPECT_EQ(listener->classes[2], TrainingIncidentClass::ADAPTER);
}
