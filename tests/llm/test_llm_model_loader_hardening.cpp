/**
 * @file test_llm_model_loader_hardening.cpp
 * @brief Focused hardening tests for LazyModelLoader lifecycle semantics.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>
#include <string>

#include "llm/llm_plugin_interface.h"
#include "utils/expected.h"

#define private public
#include "llm/model_loader.h"
#undef private

using namespace std::chrono_literals;

namespace themis { namespace llm { 
namespace {

std::shared_ptr<CachedModel> makeCachedModel(const std::string& model_id,
                                             std::chrono::system_clock::time_point last_used,
                                             size_t vram_mb,
                                             size_t ram_mb,
                                             bool keep_loaded = false) {
    auto model = std::make_shared<CachedModel>();
    model->model_id = model_id;
    model->model_path = "/tmp/" + model_id + ".gguf";
    model->last_used = last_used;
    model->loaded_at = last_used;
    model->use_count = 1;
    model->vram_mb = vram_mb;
    model->ram_mb = ram_mb;
    model->keep_loaded = keep_loaded;
    model->info.model_id = model_id;
    model->info.name = model_id;
    model->info.path = model->model_path;
    model->info.is_loaded = true;
    return model;
}

class LazyModelLoaderHardeningTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.default_n_gpu_layers = 0;
        config_.model_ttl = std::chrono::seconds(1);
        loader_ = std::make_unique<LazyModelLoader>(config_);
    }

    void seedModel(const std::shared_ptr<CachedModel>& model) {
        loader_->models_[model->model_id] = model;
        loader_->total_vram_mb_ += model->vram_mb;
        loader_->total_ram_mb_ += model->ram_mb;
    }

    LazyModelLoader::Config config_{};
    std::unique_ptr<LazyModelLoader> loader_;
};

TEST_F(LazyModelLoaderHardeningTest, SharedAccessRetainsOwnershipAcrossUnload) {
    seedModel(makeCachedModel("shared-model", std::chrono::system_clock::now(), 64, 32));

    auto shared_model = loader_->getOrLoadModelShared("shared-model", "");
    ASSERT_TRUE(shared_model);
    EXPECT_GE(shared_model.use_count(), 2u);

    EXPECT_TRUE(loader_->unloadModel("shared-model", true));
    EXPECT_FALSE(loader_->isModelLoaded("shared-model"));
    EXPECT_EQ(shared_model.use_count(), 1u);
    EXPECT_EQ(shared_model->model_id, "shared-model");

    const auto memory_stats = loader_->getMemoryStats();
    EXPECT_EQ(memory_stats.at("models_loaded").get<size_t>(), 0u);
    EXPECT_EQ(memory_stats.at("vram_used_mb").get<size_t>(), 0u);
    EXPECT_EQ(memory_stats.at("ram_used_mb").get<size_t>(), 0u);
}

TEST_F(LazyModelLoaderHardeningTest, EvictExpiredCompletesWithoutReentrantDeadlock) {
    const auto now = std::chrono::system_clock::now();
    seedModel(makeCachedModel("expired", now - 5s, 32, 16));
    seedModel(makeCachedModel("fresh", now, 48, 24));

    auto future = std::async(std::launch::async, [this]() {
        return loader_->evictExpired();
    });

    const auto status = future.wait_for(5000ms);
    ASSERT_EQ(status, std::future_status::ready);
    EXPECT_EQ(future.get(), 1u);
    EXPECT_FALSE(loader_->isModelLoaded("expired"));
    EXPECT_TRUE(loader_->isModelLoaded("fresh"));
}

TEST_F(LazyModelLoaderHardeningTest, EvictExpiredSkipsPinnedModels) {
    const auto now = std::chrono::system_clock::now();
    seedModel(makeCachedModel("pinned", now - 5s, 32, 16, true));
    seedModel(makeCachedModel("evictable", now - 5s, 48, 24));

    EXPECT_EQ(loader_->evictExpired(), 1u);
    EXPECT_TRUE(loader_->isModelLoaded("pinned"));
    EXPECT_FALSE(loader_->isModelLoaded("evictable"));
}

TEST_F(LazyModelLoaderHardeningTest, EvictLRURemovesOldestUnpinnedModel) {
    const auto now = std::chrono::system_clock::now();
    seedModel(makeCachedModel("oldest", now - 10s, 16, 8));
    seedModel(makeCachedModel("pinned", now - 20s, 64, 32, true));
    seedModel(makeCachedModel("recent", now - 1s, 24, 12));

    EXPECT_EQ(loader_->evictLRU(), 16u);
    EXPECT_FALSE(loader_->isModelLoaded("oldest"));
    EXPECT_TRUE(loader_->isModelLoaded("pinned"));
    EXPECT_TRUE(loader_->isModelLoaded("recent"));
}

}  // namespace
} } // namespace themis::llm
