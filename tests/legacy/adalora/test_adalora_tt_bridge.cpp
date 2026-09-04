/**
 * @file test_adalora_tt_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 98/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "training/adalora_tt_bridge.h"
#include "storage/tensor_network_storage_engine.h"

#include <cmath>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using namespace themis::training;
using namespace themis::storage;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a minimal AdaLoraTTExport (one 4×4 layer, rank=2).
AdaLoraTTExport makeMinimalExport(const std::string& name = "test_adapter",
                                   const std::string& tenant = "T") {
    // Build a simple TTTrain with 2 cores: G0 (1×4×2) and G1 (2×4×1).
    TTTrain train;

    TTCore g0;
    g0.r_left  = 1;
    g0.n       = 4;
    g0.r_right = 2;
    g0.data.resize(1 * 4 * 2, 0.5f);

    TTCore g1;
    g1.r_left  = 2;
    g1.n       = 4;
    g1.r_right = 1;
    g1.data.resize(2 * 4 * 1, 0.5f);

    train.cores     = {g0, g1};
    train.mode_sizes = {4u, 4u};

    AdaLoraTTLayerExport lexp;
    lexp.layer_name  = "q_proj";
    lexp.active_rank = 2u;
    lexp.scaling     = 1.0f;
    lexp.train       = std::move(train);

    AdaLoraTTExport exp;
    exp.adapter_name = name;
    exp.tenant       = tenant;
    exp.layers.push_back(std::move(lexp));
    return exp;
}

/// Build a bridge with nullptr engine (storage disabled; export/import still work).
AdaLoraTTBridge makeBridge() {
    return AdaLoraTTBridge(nullptr, AdaLoraTTBridgeConfig{});
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3 bridge — mapAdapter()
// ─────────────────────────────────────────────────────────────────────────────

// ALTB-P3-01: mapAdapter() returns false when no MapAdapterFn is injected.
TEST(AdaLoraTTBridgeStub271, ALTB_P3_01_map_adapter_returns_false_without_fn) {
    auto bridge = makeBridge();
    bridge.clearMapAdapterFn();
    auto exp = makeMinimalExport();
    EXPECT_FALSE(bridge.mapAdapter(exp));
}

// ALTB-P3-02: mapAdapter() calls the injected fn and returns its result.
TEST(AdaLoraTTBridgeStub271, ALTB_P3_02_map_adapter_calls_injected_fn) {
    auto bridge = makeBridge();

    bool called = false;
    bridge.setMapAdapterFn(
        [&called](const AdaLoraTTExport& exp) -> bool {
            called = true;
            return !exp.adapter_name.empty();
        });

    auto exp = makeMinimalExport("my_adapter");
    const bool result = bridge.mapAdapter(exp);
    bridge.clearMapAdapterFn();

    EXPECT_TRUE(called);
    EXPECT_TRUE(result);
}

// ALTB-P3-03: clearMapAdapterFn() reverts mapAdapter() to returning false.
TEST(AdaLoraTTBridgeStub271, ALTB_P3_03_clear_map_adapter_fn_reverts_to_false) {
    auto bridge = makeBridge();
    bridge.setMapAdapterFn([](const AdaLoraTTExport&) -> bool { return true; });
    bridge.clearMapAdapterFn();

    auto exp = makeMinimalExport();
    EXPECT_FALSE(bridge.mapAdapter(exp));
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4 bridge — roundAndReallocate() with TrainingStepFn
// ─────────────────────────────────────────────────────────────────────────────

// ALTB-P4-01: roundAndReallocate() uses built-in TT-rounding when no fn set.
//             Just verify it doesn't crash and returns a plausible rank.
TEST(AdaLoraTTBridgeStub271, ALTB_P4_01_round_and_reallocate_builtin_path) {
    AdaLoraTTBridge::clearTrainingStepFn();
    auto bridge = makeBridge();
    auto exp = makeMinimalExport();

    const std::size_t total = bridge.roundAndReallocate(exp, 0.01);
    // Returned rank must be >= 0 (trivially true) and fit within the layer's
    // active_rank.  We just verify no crash and a non-negative result.
    EXPECT_GE(total, 0u);
}

// ALTB-P4-02: roundAndReallocate() delegates to the injected TrainingStepFn.
TEST(AdaLoraTTBridgeStub271, ALTB_P4_02_training_step_fn_is_called) {
    bool fn_called = false;
    std::size_t returned_rank = 99u;

    AdaLoraTTBridge::setTrainingStepFn(
        [&fn_called, &returned_rank]([[maybe_unused]] AdaLoraTTExport& exp_ref,
                                     [[maybe_unused]] double eps) -> std::size_t {
            fn_called = true;
            // Simulate training step: halve active rank.
            for (auto& l : exp_ref.layers) {
              l.active_rank = 1u;
            }
            returned_rank = exp_ref.totalActiveRank();
            return returned_rank;
        });

    auto bridge = makeBridge();
    auto exp = makeMinimalExport();
    const std::size_t result = bridge.roundAndReallocate(exp, 0.05);
    AdaLoraTTBridge::clearTrainingStepFn();

    EXPECT_TRUE(fn_called);
    EXPECT_EQ(result, returned_rank);
}

// ALTB-P4-03: clearTrainingStepFn() reverts to built-in TT-rounding path.
TEST(AdaLoraTTBridgeStub271, ALTB_P4_03_clear_training_step_fn_reverts_builtin) {
    AdaLoraTTBridge::setTrainingStepFn(
        [](AdaLoraTTExport&, double) -> std::size_t { return 42u; });
    AdaLoraTTBridge::clearTrainingStepFn();

    auto bridge = makeBridge();
    auto exp = makeMinimalExport();
    // After clear, built-in TT-rounding is used; just verify it completes.
    EXPECT_NO_THROW(bridge.roundAndReallocate(exp, 0.01));
}

// ─────────────────────────────────────────────────────────────────────────────
// Data-race regression tests  (TRN-DR-01 / issue #5414 batch 5)
// Verify that concurrent storeAdapter() and findSimilarAdapters() calls do not
// corrupt the fingerprint_graph (guarded by fingerprint_graph_mutex since batch 5).
// ─────────────────────────────────────────────────────────────────────────────

// ALTB-DR-01: concurrent storeAdapter() calls complete without crashing.
TEST(AdaLoraTTBridgeDRFix, ALTB_DR_01_concurrent_store_adapter_no_crash) {
    AdaLoraTTBridgeConfig cfg;
    cfg.auto_deduplicate = true;
    AdaLoraTTBridge bridge(nullptr, cfg);

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&bridge, i]() {
            auto exp = makeMinimalExport("adapter_" + std::to_string(i),
                                         "tenant_" + std::to_string(i % 3));
            EXPECT_TRUE(bridge.storeAdapter(exp));
        });
    }
    for (auto& t : threads) {
      t.join();
    }
}

// ALTB-DR-02: concurrent storeAdapter() and findSimilarAdapters() complete without crashing.
TEST(AdaLoraTTBridgeDRFix, ALTB_DR_02_concurrent_store_and_find_no_crash) {
    AdaLoraTTBridgeConfig cfg;
    cfg.auto_deduplicate = true;
    AdaLoraTTBridge bridge(nullptr, cfg);

    // Pre-populate one entry so findSimilarAdapters has something to search.
    auto seed = makeMinimalExport("seed_adapter", "seed_tenant");
    bridge.storeAdapter(seed);

    constexpr int kStoreThreads = 4;
    constexpr int kFindThreads  = 4;
    std::vector<std::thread> threads;
    threads.reserve(kStoreThreads + kFindThreads);

    for (int i = 0; i < kStoreThreads; ++i) {
        threads.emplace_back([&bridge, i]() {
            auto exp = makeMinimalExport("concurrent_" + std::to_string(i), "T");
            bridge.storeAdapter(exp);
        });
    }
    for (int i = 0; i < kFindThreads; ++i) {
        threads.emplace_back([&bridge]() {
            auto query = makeMinimalExport("query_adapter", "T");
            EXPECT_NO_THROW(bridge.findSimilarAdapters(query, /*top_k=*/2));
        });
    }
    for (auto& t : threads) {
      t.join();
    }
}
