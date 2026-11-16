// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "storage/tensor_network_storage_engine.h"
#include "training/adalora_tt_bridge.h"

#include <memory>
#include <stdexcept>
#include <vector>

namespace {

themis::training::AdaLoRAAdapter makeAdapter(bool orthogonal = true) {
    themis::training::AdaLoRAAdapter adapter(2, 4.0f);
    adapter.addLayer("projection", 3, 3, 2, 6.0f);
    const std::vector<float> b = orthogonal
        ? std::vector<float>{1, 0, 0, 1, 0, 0}
        : std::vector<float>{1, 0, 1, 1, 0, 0};
    const std::vector<float> a{1, 0, 0, 0, 1, 0};
    adapter.setWeights("projection", b, a);
    return adapter;
}

std::shared_ptr<themis::storage::TensorNetworkStorageEngine> makeEngine() {
    return std::make_shared<themis::storage::TensorNetworkStorageEngine>(
        std::make_shared<themis::storage::InMemoryTensorBackend>());
}

}  // namespace

TEST(AdaLoraTTBridgeHardening, ExportPreservesEffectiveScalingAndUpdate) {
    auto adapter = makeAdapter();
    themis::training::AdaLoraTTBridge bridge(nullptr);

    const auto exported = bridge.exportToTT(adapter, "test-adapter");
    ASSERT_EQ(exported.layers.size(), 1u);
    EXPECT_FLOAT_EQ(exported.layers.front().scaling, 3.0f);
    EXPECT_TRUE(exported.layers.front().orthogonal_validated);

    const auto imported = bridge.importFromTT(exported);
    const auto original_output = adapter.forward("projection", {2.0f, 3.0f, 7.0f}, 1);
    const auto imported_output = imported.forward("projection", {2.0f, 3.0f, 7.0f}, 1);
    ASSERT_EQ(imported_output.size(), original_output.size());
    for (std::size_t i = 0; i < original_output.size(); ++i) {
        EXPECT_NEAR(imported_output[i], original_output[i], 1e-5f);
    }
}

TEST(AdaLoraTTBridgeHardening, NonOrthogonalExportIsNotMarkedValidated) {
    auto adapter = makeAdapter(false);
    themis::training::AdaLoraTTBridge bridge(nullptr);

    const auto exported = bridge.exportToTT(adapter, "non-orthogonal");

    ASSERT_EQ(exported.layers.size(), 1u);
    EXPECT_FALSE(exported.layers.front().orthogonal_validated);
    EXPECT_EQ(bridge.stats().orth_violations, 1u);
}

TEST(AdaLoraTTBridgeHardening, FalseOrthogonalityClaimIsRejected) {
    auto adapter = makeAdapter(false);
    themis::training::AdaLoraTTBridge bridge(makeEngine());
    auto exported = bridge.exportToTT(adapter, "unvalidated");
    exported.layers.front().orthogonal_validated = true;

    EXPECT_THROW(bridge.importFromTT(exported), std::invalid_argument);
    EXPECT_FALSE(bridge.store(exported));
}

TEST(AdaLoraTTBridgeHardening, StoreRequiresEngineAndReloadsFromStorage) {
    auto adapter = makeAdapter();
    themis::training::AdaLoraTTBridge no_storage(nullptr);
    const auto exported = no_storage.exportToTT(adapter, "stored-adapter", "tenant");
    EXPECT_FALSE(no_storage.store(exported));

    auto engine = makeEngine();
    themis::training::AdaLoraTTBridge writer(engine);
    ASSERT_TRUE(writer.store(exported));

    themis::training::AdaLoraTTBridge reader(engine);
    const auto loaded = reader.loadAdapter("tenant", "stored-adapter");
    ASSERT_TRUE(loaded.has_value());
    const auto original_output = adapter.forward("projection", {2.0f, 3.0f, 7.0f}, 1);
    const auto loaded_output = loaded->forward("projection", {2.0f, 3.0f, 7.0f}, 1);
    ASSERT_EQ(loaded_output.size(), original_output.size());
    for (std::size_t i = 0; i < original_output.size(); ++i) {
        EXPECT_NEAR(loaded_output[i], original_output[i], 1e-5f);
    }
}
