/**
 * @file test_lora_hot_loading.cpp
 * @brief Tests for LoRA adapter hot-loading at inference time.
 *
 * Validates InferenceEngineEnhanced::loadLoRAAdapter(),
 * unloadLoRAAdapter(), and getLoadedLoRAAdapters().
 *
 * Acceptance criteria (from roadmap issue):
 * - Adapters can be loaded / unloaded without restarting the engine
 * - Per-request lora_adapter_id is forwarded to the plugin
 * - Thread-safety: concurrent load/unload/generate is race-free
 * - getLoadedLoRAAdapters() reflects the current registration state
 */

#include <gtest/gtest.h>
#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_interface.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace themis::llm;

// ── Helpers ────────────────────────────────────────────────────────────────

/// Minimal mock plugin that tracks which LoRA adapters it has loaded.
class LoRATrackingPlugin : public ILLMPlugin {
public:
    explicit LoRATrackingPlugin(const std::string& model_id = "mock-model")
        : model_id_(model_id) {}

    // ── ILLMPlugin interface ────────────────────────────────────────────
    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    bool isModelLoaded() const override { return true; }
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = model_id_;
        info.name     = model_id_;
        info.is_loaded = true;
        return info;
    }

    InferenceResponse generate(const InferenceRequest& request) override {
        InferenceResponse resp;
        resp.request_id     = request.request_id;
        resp.model_id       = model_id_;
        resp.text           = "response";
        resp.lora_used      = request.lora_adapter_id;
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&,
                                   const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities  getCapabilities()    const override { return {}; }
    json             getMemoryStats()     const override { return {}; }
    json             getPerformanceStats()const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

    bool loadLoRA(const std::string& id,
                  const std::string& /*path*/,
                  float /*scale*/) override {
        std::lock_guard<std::mutex> lk(mu_);
        loaded_loras_.insert(id);
        return true;
    }
    bool unloadLoRA(const std::string& id) override {
        std::lock_guard<std::mutex> lk(mu_);
        return loaded_loras_.erase(id) > 0;
    }
    std::vector<LoRAInfo> listLoRAs() const override {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<LoRAInfo> result = {};

        for (const auto& id : loaded_loras_) {
            LoRAInfo info;
            info.id = id;
            info.lora_id = id;
            info.is_loaded = true;
            result.push_back(info);
        }
        return result;
    }

    bool isLoRALoaded(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mu_);
        return loaded_loras_.count(id) > 0;
    }

private:
    std::string              model_id_;
    mutable std::mutex       mu_;
    std::unordered_set<std::string> loaded_loras_;
};

/// Fixture that provides a running InferenceEngineEnhanced with one mock model.
class LoRAHotLoadingTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceEngineEnhanced::Config cfg;
        cfg.num_worker_threads    = 2;
        cfg.max_queue_size        = 100;
        cfg.batch_timeout_ms      = 20;
        cfg.enable_batch_processing = true;
        cfg.enable_context_caching  = false;  // keep tests simple
        cfg.enable_load_balancing   = false;

        engine_ = std::make_unique<InferenceEngineEnhanced>(cfg);
        plugin_ = std::make_shared<LoRATrackingPlugin>("model1");
        engine_->registerModel("model1", plugin_);
        engine_->start();
    }

    void TearDown() override {
        engine_->shutdown();
    }

    /// Helper: create a mock adapter file and return its path.
    static std::string makeMockAdapterFile(const std::string& name) {
        auto dir  = std::filesystem::temp_directory_path() / "themis_lora_hot_test";
        std::filesystem::create_directories(dir);
        auto path = dir / (name + ".bin");
        std::ofstream f(path, std::ios::binary);
        // Write minimal content so the file exists
        f.write("LORA_MOCK", 9);
        return path.string();
    }

    std::unique_ptr<InferenceEngineEnhanced> engine_;
    std::shared_ptr<LoRATrackingPlugin>      plugin_;
};

// ── Tests ──────────────────────────────────────────────────────────────────

/// loadLoRAAdapter registers the adapter and pre-loads it on the plugin.
TEST_F(LoRAHotLoadingTest, LoadAdapter_RegistersAndPreloadsOnPlugin) {
    auto path = makeMockAdapterFile("legal_v1");

    engine_->loadLoRAAdapter("legal-lora", path, 1.0f);

    // Registry should report it
    auto adapters = engine_->getLoadedLoRAAdapters();
    ASSERT_EQ(adapters.size(), 1u);
    EXPECT_EQ(adapters[0].id, "legal-lora");
    EXPECT_EQ(adapters[0].path, path);
    EXPECT_FLOAT_EQ(adapters[0].scale, 1.0f);

    // Plugin should have been pre-loaded
    EXPECT_TRUE(plugin_->isLoRALoaded("legal-lora"));
}

/// loadLoRAAdapter with empty adapter_id throws std::invalid_argument.
TEST_F(LoRAHotLoadingTest, LoadAdapter_EmptyId_Throws) {
    EXPECT_THROW(engine_->loadLoRAAdapter("", "/some/path.bin"),
                 std::invalid_argument);
}

/// loadLoRAAdapter with empty path throws std::invalid_argument.
TEST_F(LoRAHotLoadingTest, LoadAdapter_EmptyPath_Throws) {
    EXPECT_THROW(engine_->loadLoRAAdapter("my-lora", ""),
                 std::invalid_argument);
}

/// Multiple adapters can be loaded independently.
TEST_F(LoRAHotLoadingTest, LoadAdapter_Multiple_AllRegistered) {
    auto p1 = makeMockAdapterFile("a1");
    auto p2 = makeMockAdapterFile("a2");
    auto p3 = makeMockAdapterFile("a3");

    engine_->loadLoRAAdapter("lora-a1", p1);
    engine_->loadLoRAAdapter("lora-a2", p2);
    engine_->loadLoRAAdapter("lora-a3", p3);

    EXPECT_EQ(engine_->getLoadedLoRAAdapters().size(), 3u);
    EXPECT_TRUE(plugin_->isLoRALoaded("lora-a1"));
    EXPECT_TRUE(plugin_->isLoRALoaded("lora-a2"));
    EXPECT_TRUE(plugin_->isLoRALoaded("lora-a3"));
}

/// unloadLoRAAdapter removes registration and tells the plugin to unload.
TEST_F(LoRAHotLoadingTest, UnloadAdapter_RemovesRegistrationAndPlugin) {
    auto path = makeMockAdapterFile("med_v1");
    engine_->loadLoRAAdapter("med-lora", path);
    ASSERT_EQ(engine_->getLoadedLoRAAdapters().size(), 1u);

    bool unloaded = engine_->unloadLoRAAdapter("med-lora");
    EXPECT_TRUE(unloaded);
    EXPECT_EQ(engine_->getLoadedLoRAAdapters().size(), 0u);
    EXPECT_FALSE(plugin_->isLoRALoaded("med-lora"));
}

/// Unloading a non-existent adapter returns false without error.
TEST_F(LoRAHotLoadingTest, UnloadAdapter_NotRegistered_ReturnsFalse) {
    bool result = engine_->unloadLoRAAdapter("ghost-lora");
    EXPECT_FALSE(result);
}

/// Hot-loading adapter while inference is running works safely.
TEST_F(LoRAHotLoadingTest, HotLoad_DuringActiveInference_ThreadSafe) {
    auto path = makeMockAdapterFile("code_v1");

    // Submit several requests while also hot-loading adapters in parallel.
    std::atomic<int> completed{0};
    const int num_requests = 10;

    std::thread loader([&]() {
        for (int i = 0; i < 5; ++i) {
            engine_->loadLoRAAdapter("hot-lora-" + std::to_string(i), path);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    for (int i = 0; i < num_requests; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id         = "r" + std::to_string(i);
        req.base_request.prompt = "test prompt " + std::to_string(i);
        req.timeout             = std::chrono::milliseconds(5000);

        auto handle = engine_->submit(req);
        auto resp   = handle.get();
        EXPECT_FALSE(resp.text.empty());
        completed++;
    }

    loader.join();
    EXPECT_EQ(completed.load(), num_requests);
}

/// Per-request lora_adapter_id is forwarded through the engine to the plugin.
TEST_F(LoRAHotLoadingTest, PerRequestAdapter_ForwardedToPlugin) {
    auto path = makeMockAdapterFile("qa_v1");
    engine_->loadLoRAAdapter("qa-lora", path, 0.8f);

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id                     = "req-with-lora";
    req.base_request.prompt            = "answer this question";
    req.base_request.lora_adapter_id   = "qa-lora";
    req.timeout                        = std::chrono::milliseconds(5000);

    auto handle = engine_->submit(req);
    auto resp   = handle.get();

    EXPECT_FALSE(resp.text.empty());
    // The mock plugin echoes lora_used from the request
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(*resp.lora_used, "qa-lora");
}

/// Requests referencing an unknown adapter still complete (degraded mode).
TEST_F(LoRAHotLoadingTest, UnknownAdapter_RequestStillCompletes) {
    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id                    = "req-bad-lora";
    req.base_request.prompt           = "prompt with bad lora";
    req.base_request.lora_adapter_id  = "does-not-exist";
    req.timeout                       = std::chrono::milliseconds(5000);

    auto handle = engine_->submit(req);
    // Should not throw; should complete with a valid (possibly empty) response
    EXPECT_NO_THROW({
        auto resp = handle.get();
        (void)resp;  // result is acceptable even if degraded
    });
}

/// loadLoRAAdapter on a specific model_id only loads the adapter on that model.
TEST_F(LoRAHotLoadingTest, LoadAdapter_ModelScoped_OnlyAffectsTargetModel) {
    // Register a second plugin
    auto plugin2 = std::make_shared<LoRATrackingPlugin>("model2");
    engine_->registerModel("model2", plugin2);

    auto path = makeMockAdapterFile("scope_v1");
    engine_->loadLoRAAdapter("scope-lora", path, 1.0f, "model1");

    EXPECT_TRUE(plugin_->isLoRALoaded("scope-lora"));   // model1 — should be loaded
    EXPECT_FALSE(plugin2->isLoRALoaded("scope-lora"));  // model2 — should NOT be loaded
}

/// getLoadedLoRAAdapters returns empty when no adapters are registered.
TEST_F(LoRAHotLoadingTest, GetLoadedAdapters_InitiallyEmpty) {
    EXPECT_TRUE(engine_->getLoadedLoRAAdapters().empty());
}

/// AC-5 performance gate: loadLoRAAdapter must complete in ≤ 5 s wall-clock.
/// Gated on THEMIS_RUN_PERF_TESTS=1 so it is opt-in in CI.
TEST_F(LoRAHotLoadingTest, HotLoad_WallClock_Under5Seconds) {
    if (!std::getenv("THEMIS_RUN_PERF_TESTS")) {
        GTEST_SKIP() << "Set THEMIS_RUN_PERF_TESTS=1 to enable performance gate";
    }

    auto path = makeMockAdapterFile("perf_7b");

    auto t0 = std::chrono::steady_clock::now();
    engine_->loadLoRAAdapter("perf-lora", path, 1.0f);
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - t0)
                          .count();

    EXPECT_LE(elapsed_ms, 5000)
        << "Hot-load took " << elapsed_ms << " ms, expected ≤ 5000 ms";

    // Adapter must be registered and pre-loaded on the plugin after the call.
    EXPECT_TRUE(plugin_->isLoRALoaded("perf-lora"));
}

/// Calling loadLoRAAdapter before any model is registered still completes,
/// and the adapter is pre-loaded when a model is registered afterwards.
TEST(LoRAHotLoadingStandaloneTest, LoadAdapter_BeforeModelRegistered_PreloadedOnLaterModel) {
    InferenceEngineEnhanced::Config cfg;
    cfg.num_worker_threads = 1;
    InferenceEngineEnhanced engine(cfg);

    // Create a real temp file so the plugin sees a valid path.
    auto dir  = std::filesystem::temp_directory_path() / "themis_lora_hot_test";
    std::filesystem::create_directories(dir);
    auto path = (dir / "orphan.bin").string();
    { std::ofstream f(path, std::ios::binary); f.write("LORA", 4); }

    // Load adapter before any model is registered — should not throw
    EXPECT_NO_THROW(engine.loadLoRAAdapter("orphan-lora", path));
    EXPECT_EQ(engine.getLoadedLoRAAdapters().size(), 1u);

    // Register a model afterwards — adapter should be pre-loaded on it
    auto plugin = std::make_shared<LoRATrackingPlugin>("late-model");
    engine.registerModel("late-model", plugin);
    EXPECT_TRUE(plugin->isLoRALoaded("orphan-lora"));
}

