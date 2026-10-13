#include <gtest/gtest.h>

#include "llama_cpp/llama_cpp_plugin.h"

#include <nlohmann/json.hpp>

using namespace themis::llamacpp;
using json = nlohmann::json;

namespace {

std::unique_ptr<LlamaCppPlugin> make_loaded_plugin(const std::string& model_path = "/stub/model.gguf") {
    auto plugin = std::make_unique<LlamaCppPlugin>();
    EXPECT_TRUE(plugin->loadModel(model_path, json::object()));
    return plugin;
}

}  // namespace

TEST(LlamaCppModelLoadingTests, M01_LoadModel_AcceptsStubPath) {
    LlamaCppPlugin plugin;
    EXPECT_TRUE(plugin.loadModel("/stub/model.gguf", json::object()));
    EXPECT_TRUE(plugin.isModelLoaded());
}

TEST(LlamaCppModelLoadingTests, M02_LoadModel_EmptyPathStillSucceedsInStubMode) {
    LlamaCppPlugin plugin;
    EXPECT_TRUE(plugin.loadModel("", json::object()));
    EXPECT_TRUE(plugin.isModelLoaded());
}

TEST(LlamaCppModelLoadingTests, M03_LoadModel_WithContextConfigSetsMetadata) {
    LlamaCppPlugin plugin;
    json cfg = { {"n_ctx", 8192}, {"n_batch", 32} };

    EXPECT_TRUE(plugin.loadModel("/stub/context.gguf", cfg));
    auto info = plugin.getModelInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->model_id, "/stub/context.gguf");
    EXPECT_GE(info->context_length, 8192u);
}

TEST(LlamaCppModelLoadingTests, M04_LoadModel_DoubleLoadIsSafe) {
    LlamaCppPlugin plugin;
    EXPECT_TRUE(plugin.loadModel("/stub/first.gguf", json::object()));
    EXPECT_TRUE(plugin.loadModel("/stub/second.gguf", json::object()));
    EXPECT_TRUE(plugin.isModelLoaded());
    auto info = plugin.getModelInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->model_id, "/stub/second.gguf");
}

TEST(LlamaCppModelLoadingTests, M05_GetModelInfo_IsAbsentBeforeLoad) {
    LlamaCppPlugin plugin;
    EXPECT_FALSE(plugin.getModelInfo().has_value());
    EXPECT_FALSE(plugin.isModelLoaded());
}

TEST(LlamaCppModelLoadingTests, M06_GetModelInfo_PopulatesModelMetadataAfterLoad) {
    auto plugin = make_loaded_plugin("/stub/meta.gguf");
    auto info = plugin->getModelInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->path, "/stub/meta.gguf");
    EXPECT_EQ(info->model_id, "/stub/meta.gguf");
    EXPECT_GT(info->context_length, 0u);
}

TEST(LlamaCppModelLoadingTests, M07_UnloadModel_ClearsLoadedState) {
    auto plugin = make_loaded_plugin();
    plugin->unloadModel();
    EXPECT_FALSE(plugin->isModelLoaded());
    EXPECT_FALSE(plugin->getModelInfo().has_value());
}
