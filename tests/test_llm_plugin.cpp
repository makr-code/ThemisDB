#include <gtest/gtest.h>
#include "llm/llama_wrapper.h"
#include "llm/model_loader.h"
#include "llm/multi_lora_manager.h"
#include "llm/async_inference_engine.h"
#include "llm/llm_plugin_manager.h"
#include "test_helpers_llm.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <cstdlib>

namespace fs = std::filesystem;
using namespace themis::llm;
using json = nlohmann::json;

/**
 * @brief Check if GPU is available, skip test if not
 * 
 * Checks for CUDA/HIP GPU availability. If no GPU found, test is skipped
 * instead of failing. This is appropriate for development machines without GPU.
 */
static void requireGPUOrSkip() {
    // TODO: Implement actual CUDA/HIP detection
    // For now, check environment variable as workaround
    const char* has_gpu = std::getenv("THEMIS_HAS_GPU");
    
    if (has_gpu == nullptr || std::string(has_gpu) != "1") {
        GTEST_SKIP() << "capability:gpu_runtime_available=false;reason=no_gpu_available;details=\n"
                     << "To enable GPU tests: $env:THEMIS_HAS_GPU = \"1\"";
    }
}

class LLMPluginTest : public ::testing::Test {
protected:
    std::string test_model_dir = "./test_llm_models";
    std::string test_lora_dir = "./test_llm_loras";
    
    void SetUp() override {
        // Clean up
        if (fs::exists(test_model_dir)) {
            fs::remove_all(test_model_dir);
        }
        if (fs::exists(test_lora_dir)) {
            fs::remove_all(test_lora_dir);
        }
        fs::create_directories(test_model_dir);
        fs::create_directories(test_lora_dir);
    }
    
    void TearDown() override {
        // Clean up
        if (fs::exists(test_model_dir)) {
            fs::remove_all(test_model_dir);
        }
        if (fs::exists(test_lora_dir)) {
            fs::remove_all(test_lora_dir);
        }
    }
    
    // Create a dummy model file for testing
    void createDummyModel(const std::string& filename, size_t size_mb = 100) {
        std::string path = test_model_dir + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        
        // Write GGUF magic bytes
        file.write("GGUF", 4);
        
        // Write some dummy data to simulate model size
        std::vector<char> dummy_data(size_mb * 1024 * 1024, 0);
        file.write(dummy_data.data(), dummy_data.size());
        file.close();
    }
    
    // Create a dummy LoRA file
    void createDummyLoRA(const std::string& filename, size_t size_mb = 10) {
        std::string path = test_lora_dir + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        
        // Write GGUF magic bytes
        file.write("GGUF", 4);
        
        // Write dummy LoRA data
        std::vector<char> dummy_data(size_mb * 1024 * 1024, 0);
        file.write(dummy_data.data(), dummy_data.size());
        file.close();
    }
};

// ═══════════════════════════════════════════════════════════
// Lazy Model Loader Tests (Ollama-style)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, LazyModelLoader_BasicLoading) {
    LazyModelLoader::Config config;
    config.max_models = 3;
    config.max_vram_mb = 10240;
    config.enable_lazy_load = true;
    
    LazyModelLoader loader(config);
    
    createDummyModel("model1.gguf", 100);
    std::string model_path = test_model_dir + "/model1.gguf";
    
    json load_config = {
        {"n_gpu_layers", 32},
        {"n_ctx", 2048}
    };
    
    // First load - should be cache miss
    auto* model1 = loader.getOrLoadModel("model1", model_path, load_config);
    ASSERT_NE(model1, nullptr);
    EXPECT_EQ(model1->use_count, 1);
    
    // Second load - should be cache hit
    auto* model2 = loader.getOrLoadModel("model1", model_path, load_config);
    ASSERT_NE(model2, nullptr);
    EXPECT_EQ(model2, model1);  // Same pointer
    EXPECT_EQ(model2->use_count, 2);
    
    auto stats = loader.getStatistics();
    EXPECT_EQ(stats.cache_hits, 1);
    EXPECT_EQ(stats.cache_misses, 1);
}

TEST_F(LLMPluginTest, LazyModelLoader_LRUEviction) {
    LazyModelLoader::Config config;
    config.max_models = 2;  // Only keep 2 models
    config.max_vram_mb = 10240;
    config.enable_lazy_load = true;
    
    LazyModelLoader loader(config);
    
    createDummyModel("model1.gguf", 50);
    createDummyModel("model2.gguf", 50);
    createDummyModel("model3.gguf", 50);
    
    json load_config = {{"n_gpu_layers", 32}};
    
    // Load 2 models
    auto* m1 = loader.getOrLoadModel("model1", test_model_dir + "/model1.gguf", load_config);
    auto* m2 = loader.getOrLoadModel("model2", test_model_dir + "/model2.gguf", load_config);
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    
    // Load 3rd model - should evict model1 (LRU)
    auto* m3 = loader.getOrLoadModel("model3", test_model_dir + "/model3.gguf", load_config);
    ASSERT_NE(m3, nullptr);
    
    // Try to get model1 again - should be cache miss (evicted)
    auto* m1_reload = loader.getOrLoadModel("model1", test_model_dir + "/model1.gguf", load_config);
    ASSERT_NE(m1_reload, nullptr);
    
    auto stats = loader.getStatistics();
    EXPECT_GE(stats.cache_misses, 3);  // At least 3 cache misses
}

TEST_F(LLMPluginTest, LazyModelLoader_ModelPinning) {
    LazyModelLoader::Config config;
    config.max_models = 2;
    config.max_vram_mb = 10240;
    
    LazyModelLoader loader(config);
    
    createDummyModel("important.gguf", 50);
    createDummyModel("temp1.gguf", 50);
    createDummyModel("temp2.gguf", 50);
    
    json load_config = {{"n_gpu_layers", 32}};
    
    // Load and pin important model
    auto* important = loader.getOrLoadModel("important", test_model_dir + "/important.gguf", load_config);
    ASSERT_NE(important, nullptr);
    loader.pinModel("important");
    
    // Load another model
    auto* temp1 = loader.getOrLoadModel("temp1", test_model_dir + "/temp1.gguf", load_config);
    ASSERT_NE(temp1, nullptr);
    
    // Load 3rd model - should evict temp1, not the pinned model
    auto* temp2 = loader.getOrLoadModel("temp2", test_model_dir + "/temp2.gguf", load_config);
    ASSERT_NE(temp2, nullptr);
    
    // Pinned model should still be accessible (cache hit)
    auto* important_reaccess = loader.getOrLoadModel("important", test_model_dir + "/important.gguf", load_config);
    EXPECT_EQ(important_reaccess, important);
}

// ═══════════════════════════════════════════════════════════
// Multi-LoRA Manager Tests (vLLM-style)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, MultiLoRAManager_BasicLoading) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 16;
    config.max_lora_vram_mb = 2048;
    config.enable_multi_lora_batch = true;
    
    MultiLoRAManager manager(config);
    
    createDummyLoRA("legal.bin", 20);
    std::string lora_path = test_lora_dir + "/legal.bin";
    
    // Load LoRA with scale factor
    bool loaded = manager.loadLoRA("legal-qa", lora_path, "model1", 1.0f);
    EXPECT_TRUE(loaded);
    
    // Check if loaded
    EXPECT_TRUE(manager.isLoRALoaded("legal-qa"));
    
    // Get LoRA info
    auto info = manager.getLoRAInfo("legal-qa");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->adapter_id, "legal-qa");
    EXPECT_EQ(info->base_model_id, "model1");
}

TEST_F(LLMPluginTest, MultiLoRAManager_MultipleLoRAs) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 16;
    config.max_lora_vram_mb = 2048;
    
    MultiLoRAManager manager(config);
    
    createDummyLoRA("legal.bin", 20);
    createDummyLoRA("medical.bin", 20);
    createDummyLoRA("finance.bin", 20);
    
    // Load multiple LoRAs with scale factor
    EXPECT_TRUE(manager.loadLoRA("legal", test_lora_dir + "/legal.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("medical", test_lora_dir + "/medical.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("finance", test_lora_dir + "/finance.bin", "model1", 1.0f));
    
    // All should be loaded
    EXPECT_TRUE(manager.isLoRALoaded("legal"));
    EXPECT_TRUE(manager.isLoRALoaded("medical"));
    EXPECT_TRUE(manager.isLoRALoaded("finance"));
    
    // List LoRAs
    auto loras = manager.listLoRAs("model1");
    EXPECT_EQ(loras.size(), 3);
}

TEST_F(LLMPluginTest, MultiLoRAManager_SlotLimit) {
    MultiLoRAManager::Config config;
    config.max_lora_slots = 3;  // Only 3 slots
    config.max_lora_vram_mb = 2048;
    
    MultiLoRAManager manager(config);
    
    createDummyLoRA("lora1.bin", 10);
    createDummyLoRA("lora2.bin", 10);
    createDummyLoRA("lora3.bin", 10);
    createDummyLoRA("lora4.bin", 10);
    
    // Load 3 LoRAs - should succeed
    EXPECT_TRUE(manager.loadLoRA("lora1", test_lora_dir + "/lora1.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("lora2", test_lora_dir + "/lora2.bin", "model1", 1.0f));
    EXPECT_TRUE(manager.loadLoRA("lora3", test_lora_dir + "/lora3.bin", "model1", 1.0f));
    
    // Load 4th LoRA - should evict LRU
    EXPECT_TRUE(manager.loadLoRA("lora4", test_lora_dir + "/lora4.bin", "model1", 1.0f));
    
    // Check stats - should have evictions count
    auto stats = manager.getStatistics();
    EXPECT_GE(stats.evictions, 1);  // At least 1 eviction occurred
}

// ═══════════════════════════════════════════════════════════
// LlamaWrapper Tests (Consolidated)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, LlamaWrapper_Initialization) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    
    LlamaWrapper plugin(config);
    
    EXPECT_EQ(plugin.getName(), "llamacpp");
    EXPECT_FALSE(plugin.isModelLoaded());
}

TEST_F(LLMPluginTest, LlamaWrapper_ModelLoading) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.lazy_loader_config.max_models = 2;
    
    LlamaWrapper plugin(config);
    
    createDummyModel("mistral.gguf", 100);
    std::string model_path = test_model_dir + "/mistral.gguf";
    
    json load_config = {
        {"n_gpu_layers", 32},
        {"n_ctx", 4096}
    };
    
    // Load model
    bool loaded = plugin.loadModel(model_path, load_config);
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(plugin.isModelLoaded());
    
    // Get model info
    auto info = plugin.getModelInfo();
    ASSERT_TRUE(info.has_value());
}

TEST_F(LLMPluginTest, LlamaWrapper_LoRAManagement) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    
    createDummyModel("mistral.gguf", 100);
    createDummyLoRA("legal.bin", 20);
    
    // Load model first
    plugin.loadModel(test_model_dir + "/mistral.gguf", {});
    
    // Load LoRA
    bool loaded = plugin.loadLoRA("legal-qa", test_lora_dir + "/legal.bin", 1.0f);
    EXPECT_TRUE(loaded);
    
    // List LoRAs
    auto loras = plugin.listLoRAs();
    EXPECT_GE(loras.size(), 1);
}

TEST_F(LLMPluginTest, LlamaWrapper_BasicInference) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    
    LlamaWrapper plugin(config);
    
    createDummyModel("tiny.gguf", 50);
    plugin.loadModel(test_model_dir + "/tiny.gguf", {});
    
    InferenceRequest request;
    request.prompt = "What is ThemisDB?";
    request.max_tokens = 100;
    request.temperature = 0.7;
    request.top_p = 0.9;
    
    // Generate response
    auto response = plugin.generate(request);
    
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
    EXPECT_GE(response.inference_time_ms, 0);
}

// ═══════════════════════════════════════════════════════════
// Async Inference Engine Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, AsyncInference_NonBlocking) {
    LlamaWrapper::Config plugin_config;
    plugin_config.n_gpu_layers = 32;
    plugin_config.n_ctx = 2048;
    
    auto plugin = std::make_shared<LlamaWrapper>(plugin_config);
    
    createDummyModel("async_model.gguf", 50);
    plugin->loadModel(test_model_dir + "/async_model.gguf", {});
    
    AsyncInferenceEngine::Config engine_config;
    engine_config.num_worker_threads = 2;
    engine_config.max_queue_size = 100;
    
    AsyncInferenceEngine engine(plugin, engine_config);
    
    InferenceRequest request;
    request.prompt = "Test prompt";
    request.max_tokens = 50;
    
    // Submit request - should return immediately
    auto start = std::chrono::high_resolution_clock::now();
    auto handle = engine.submit(request, 10);
    auto submit_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start
    ).count();
    
    // Submission should be very fast (< 10ms)
    EXPECT_LT(submit_time, 10);
    
    // Wait for result
    auto response = handle.get();
    EXPECT_FALSE(response.text.empty());
}

TEST_F(LLMPluginTest, AsyncInference_Callback) {
    auto plugin = std::make_shared<LlamaWrapper>(LlamaWrapper::Config{});
    createDummyModel("callback_model.gguf", 50);
    plugin->loadModel(test_model_dir + "/callback_model.gguf", {});
    
    AsyncInferenceEngine engine(plugin, AsyncInferenceEngine::Config{});
    
    InferenceRequest request;
    request.prompt = "Test";
    request.max_tokens = 20;
    
    std::atomic<bool> callback_called{false};
    std::string result_text = {};
    
    engine.submitAsync(
        request,
        [&](const InferenceResponse& response) {
            result_text = response.text;
            callback_called = true;
        },
        5
    );
    
    // Wait for callback
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(result_text.empty());
}

TEST_F(LLMPluginTest, AsyncInference_PriorityScheduling) {
    auto plugin = std::make_shared<LlamaWrapper>(LlamaWrapper::Config{});
    createDummyModel("priority_model.gguf", 50);
    plugin->loadModel(test_model_dir + "/priority_model.gguf", {});
    
    AsyncInferenceEngine::Config config;
    config.num_worker_threads = 1;  // Single worker to test priority
    config.max_queue_size = 100;
    
    AsyncInferenceEngine engine(plugin, config);
    
    std::vector<int> completion_order;
    std::mutex order_mutex = {};
    
    // Submit low priority request
    engine.submitAsync(
        InferenceRequest{.prompt = "Low priority", .max_tokens = 10},
        [&](const InferenceResponse& r) {
            std::lock_guard<std::mutex> lock(order_mutex);
            completion_order.push_back(1);
        },
        1  // Low priority
    );
    
    // Submit high priority request
    engine.submitAsync(
        InferenceRequest{.prompt = "High priority", .max_tokens = 10},
        [&](const InferenceResponse& r) {
            std::lock_guard<std::mutex> lock(order_mutex);
            completion_order.push_back(10);
        },
        10  // High priority
    );
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // High priority should complete first (if not already processing)
    EXPECT_GE(completion_order.size(), 2);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, Integration_RAGWorkflow) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 8192;  // Large context for RAG
    
    LlamaWrapper plugin(config);
    
    createDummyModel("rag_model.gguf", 100);
    plugin.loadModel(test_model_dir + "/rag_model.gguf", {});
    
    // Create RAG context
    RAGContext rag_context;
    rag_context.documents = {
        {"ThemisDB is a distributed graph database."},
        {"It supports vector search and full-text search."},
        {"ThemisDB uses RocksDB as storage backend."}
    };
    rag_context.max_context_tokens = 2048;
    
    InferenceRequest request;
    request.prompt = "What is ThemisDB?";
    request.max_tokens = 100;
    
    // Generate with RAG
    auto response = plugin.generateRAG(rag_context, request);
    
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
}

TEST_F(LLMPluginTest, Integration_MultiLoRASwitch) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    
    createDummyModel("base.gguf", 100);
    createDummyLoRA("legal.bin", 20);
    createDummyLoRA("medical.bin", 20);
    
    plugin.loadModel(test_model_dir + "/base.gguf", {});
    plugin.loadLoRA("legal", test_lora_dir + "/legal.bin", {});
    plugin.loadLoRA("medical", test_lora_dir + "/medical.bin", {});
    
    // Generate with legal LoRA
    InferenceRequest legal_req;
    legal_req.prompt = "Explain contract law";
    legal_req.max_tokens = 50;
    legal_req.lora_adapter_id = "legal";
    
    auto legal_response = plugin.generate(legal_req);
    EXPECT_FALSE(legal_response.text.empty());
    ASSERT_TRUE(legal_response.lora_used.has_value());
    EXPECT_EQ(legal_response.lora_used.value(), "legal");
    
    // Generate with medical LoRA (fast switch!)
    InferenceRequest medical_req;
    medical_req.prompt = "Explain diabetes";
    medical_req.max_tokens = 50;
    medical_req.lora_adapter_id = "medical";
    
    auto medical_response = plugin.generate(medical_req);
    EXPECT_FALSE(medical_response.text.empty());
    ASSERT_TRUE(medical_response.lora_used.has_value());
    EXPECT_EQ(medical_response.lora_used.value(), "medical");
    
    // Responses should be different (different LoRAs)
    // In real implementation, they would have domain-specific knowledge
}

// ═══════════════════════════════════════════════════════════
// LoRA Inference Verification Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, InferenceLoRAInclusion_LoRAFieldSet) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    
    createDummyModel("model.gguf", 100);
    createDummyLoRA("adapter.bin", 20);
    
    plugin.loadModel(test_model_dir + "/model.gguf", {});
    plugin.loadLoRA("adapter", test_lora_dir + "/adapter.bin", 1.0f);
    
    // Verify LoRA is loaded before inference
    auto loaded_loras = plugin.listLoRAs();
    ASSERT_EQ(loaded_loras.size(), 1);
    EXPECT_EQ(loaded_loras[0].id, "adapter");
    EXPECT_TRUE(loaded_loras[0].is_loaded);
    
    // Inference with LoRA specified
    InferenceRequest req;
    req.prompt = "Test prompt";
    req.max_tokens = 32;
    req.lora_adapter_id = "adapter";
    
    InferenceResponse resp = plugin.generate(req);
    
    // Critical check: Response must have lora_used field set
    ASSERT_TRUE(resp.lora_used.has_value()) 
        << "lora_used field not set in response when LoRA was specified!";
    EXPECT_EQ(resp.lora_used.value(), "adapter");
    EXPECT_FALSE(resp.text.empty());
    EXPECT_GT(resp.tokens_generated, 0);
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_InvalidLoRAFails) {
    LlamaWrapper::Config config;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    createDummyModel("model.gguf", 100);
    plugin.loadModel(test_model_dir + "/model.gguf", {});
    
    // Try to use non-existent LoRA
    InferenceRequest req;
    req.prompt = "Test";
    req.max_tokens = 32;
    req.lora_adapter_id = "nonexistent-lora";
    
    // Should handle gracefully (fallback to base model or error)
    // Expected behavior: try to apply LoRA, fallback if not loaded
    InferenceResponse resp = plugin.generate(req);
    EXPECT_FALSE(resp.text.empty());
    
    // lora_used should NOT be set since LoRA wasn't loaded
    if (resp.lora_used.has_value()) {
        EXPECT_NE(resp.lora_used.value(), "nonexistent-lora");
    }
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_WithoutLoRA) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    
    LlamaWrapper plugin(config);
    createDummyModel("base.gguf", 100);
    plugin.loadModel(test_model_dir + "/base.gguf", {});
    
    // Inference WITHOUT LoRA
    InferenceRequest req;
    req.prompt = "Test";
    req.max_tokens = 32;
    // No lora_adapter_id specified
    
    InferenceResponse resp = plugin.generate(req);
    EXPECT_FALSE(resp.text.empty());
    EXPECT_GT(resp.tokens_generated, 0);
    
    // lora_used should not be set
    EXPECT_FALSE(resp.lora_used.has_value());
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_ModelNotLoaded) {
    LlamaWrapper::Config config;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    createDummyLoRA("orphan.bin", 20);
    
    // Load LoRA without model (should fail or be deferred)
    bool lora_loaded = plugin.loadLoRA("orphan", test_lora_dir + "/orphan.bin", 1.0f);
    EXPECT_FALSE(lora_loaded) << "LoRA should not load without base model";
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_VerifyScaleFactor) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    createDummyModel("model.gguf", 100);
    createDummyLoRA("scaled.bin", 20);
    
    plugin.loadModel(test_model_dir + "/model.gguf", {});
    plugin.loadLoRA("scaled", test_lora_dir + "/scaled.bin", 2.0f);
    
    // Verify LoRA was loaded with correct scale
    auto loras = plugin.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_EQ(loras[0].scale, 2.0f);
    EXPECT_EQ(loras[0].id, "scaled");
    
    // Inference with scaled LoRA
    InferenceRequest req;
    req.prompt = "Test";
    req.max_tokens = 32;
    req.lora_adapter_id = "scaled";
    
    InferenceResponse resp = plugin.generate(req);
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(resp.lora_used.value(), "scaled");
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_MultipleSequentialRequests) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    createDummyModel("seq_model.gguf", 100);
    createDummyLoRA("lora_a.bin", 20);
    createDummyLoRA("lora_b.bin", 20);
    
    plugin.loadModel(test_model_dir + "/seq_model.gguf", {});
    plugin.loadLoRA("lora_a", test_lora_dir + "/lora_a.bin", 1.0f);
    plugin.loadLoRA("lora_b", test_lora_dir + "/lora_b.bin", 1.0f);
    
    // Request 1: Use lora_a
    InferenceRequest req1;
    req1.prompt = "Request with A";
    req1.max_tokens = 32;
    req1.lora_adapter_id = "lora_a";
    req1.request_id = "req-001";
    
    InferenceResponse resp1 = plugin.generate(req1);
    EXPECT_EQ(resp1.request_id, "req-001");
    ASSERT_TRUE(resp1.lora_used.has_value());
    EXPECT_EQ(resp1.lora_used.value(), "lora_a");
    
    // Request 2: Use lora_b (different LoRA - verify switching works)
    InferenceRequest req2;
    req2.prompt = "Request with B";
    req2.max_tokens = 32;
    req2.lora_adapter_id = "lora_b";
    req2.request_id = "req-002";
    
    InferenceResponse resp2 = plugin.generate(req2);
    EXPECT_EQ(resp2.request_id, "req-002");
    ASSERT_TRUE(resp2.lora_used.has_value());
    EXPECT_EQ(resp2.lora_used.value(), "lora_b");
    
    // Request 3: Back to lora_a (verify switching back)
    InferenceRequest req3;
    req3.prompt = "Request with A again";
    req3.max_tokens = 32;
    req3.lora_adapter_id = "lora_a";
    req3.request_id = "req-003";
    
    InferenceResponse resp3 = plugin.generate(req3);
    EXPECT_EQ(resp3.request_id, "req-003");
    ASSERT_TRUE(resp3.lora_used.has_value());
    EXPECT_EQ(resp3.lora_used.value(), "lora_a");
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_CacheVerification) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    createDummyModel("cache_model.gguf", 100);
    createDummyLoRA("cached.bin", 20);
    
    plugin.loadModel(test_model_dir + "/cache_model.gguf", {});
    
    // Load LoRA twice (should hit cache)
    bool loaded1 = plugin.loadLoRA("cached", test_lora_dir + "/cached.bin", 1.0f);
    EXPECT_TRUE(loaded1);
    
    bool loaded2 = plugin.loadLoRA("cached", test_lora_dir + "/cached.bin", 1.0f);
    EXPECT_TRUE(loaded2) << "Cached LoRA load should succeed";
    
    // Get cache stats
    auto loras = plugin.listLoRAs();
    ASSERT_EQ(loras.size(), 1);
    EXPECT_TRUE(loras[0].is_loaded);
}

TEST_F(LLMPluginTest, InferenceLoRAInclusion_UnloadAndReload) {
    LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaWrapper plugin(config);
    createDummyModel("unload_model.gguf", 100);
    createDummyLoRA("unload.bin", 20);
    
    plugin.loadModel(test_model_dir + "/unload_model.gguf", {});
    
    // Load
    EXPECT_TRUE(plugin.loadLoRA("unload", test_lora_dir + "/unload.bin", 1.0f));
    EXPECT_EQ(plugin.listLoRAs().size(), 1);
    
    // Unload
    EXPECT_TRUE(plugin.unloadLoRA("unload"));
    EXPECT_EQ(plugin.listLoRAs().size(), 0);
    
    // Reload
    EXPECT_TRUE(plugin.loadLoRA("unload", test_lora_dir + "/unload.bin", 1.0f));
    EXPECT_EQ(plugin.listLoRAs().size(), 1);
    
    // Inference should work after reload
    InferenceRequest req;
    req.prompt = "Test";
    req.max_tokens = 32;
    req.lora_adapter_id = "unload";
    
    InferenceResponse resp = plugin.generate(req);
    ASSERT_TRUE(resp.lora_used.has_value());
    EXPECT_EQ(resp.lora_used.value(), "unload");
}

// ═══════════════════════════════════════════════════════════
// RoPE Scaling Tests (Phase 3.1)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, RopeScaling_ConfigValidation) {
    LlamaWrapper::Config config;
    
    // Test valid RoPE scaling configuration
    config.rope_scaling.enabled = true;
    config.rope_scaling.method = RopeScalingMethod::YARN;
    config.rope_scaling.max_context = 32768;
    config.rope_scaling.original_context = 4096;
    
    // Should not throw
    EXPECT_NO_THROW(LlamaWrapper wrapper(config));
}

TEST_F(LLMPluginTest, RopeScaling_InvalidConfig) {
    LlamaWrapper::Config config;
    
    // Test invalid configuration: max_context < original_context
    config.rope_scaling.enabled = true;
    config.rope_scaling.max_context = 2048;
    config.rope_scaling.original_context = 4096;
    
    // Should issue a warning but not throw (warning is logged, not an error)
    EXPECT_NO_THROW(LlamaWrapper wrapper(config));
}

TEST_F(LLMPluginTest, RopeScaling_YarnParameters) {
    LlamaWrapper::Config config;
    
    config.rope_scaling.enabled = true;
    config.rope_scaling.method = RopeScalingMethod::YARN;
    config.rope_scaling.max_context = 32768;
    config.rope_scaling.original_context = 4096;
    
    // Test YaRN-specific parameters
    config.rope_scaling.yarn_ext_factor = 1.5f;
    config.rope_scaling.yarn_attn_factor = 1.2f;
    config.rope_scaling.yarn_beta_fast = 32.0f;
    config.rope_scaling.yarn_beta_slow = 1.0f;
    
    EXPECT_NO_THROW(LlamaWrapper wrapper(config));
}

// ═══════════════════════════════════════════════════════════
// QW-27: LLMPluginManager Fail-Closed Guard Tests
// ═══════════════════════════════════════════════════════════

class LLMPluginManagerTest : public ::testing::Test {
protected:
    LLMPluginManager& manager = LLMPluginManager::instance();
};

TEST_F(LLMPluginManagerTest, LoadModelFailsClosedForEmptyModelId) {
    // Fail-closed: reject empty model_id immediately
    const bool result = manager.loadModel("", "/path/to/model.gguf");
    EXPECT_FALSE(result) << "loadModel should return false for empty model_id";
}

TEST_F(LLMPluginManagerTest, LoadModelFailsClosedForEmptyPath) {
    // Fail-closed: reject empty path immediately
    const bool result = manager.loadModel("test_model", "");
    EXPECT_FALSE(result) << "loadModel should return false for empty path";
}

TEST_F(LLMPluginManagerTest, LoadLoRAFailsClosedForEmptyLoRAId) {
    // Fail-closed: reject empty lora_id immediately
    const bool result = manager.loadLoRA("", "/path/to/lora.bin", "base_model");
    EXPECT_FALSE(result) << "loadLoRA should return false for empty lora_id";
}

TEST_F(LLMPluginManagerTest, LoadLoRAFailsClosedForEmptyPath) {
    // Fail-closed: reject empty path immediately
    const bool result = manager.loadLoRA("test_lora", "", "base_model");
    EXPECT_FALSE(result) << "loadLoRA should return false for empty path";
}

TEST_F(LLMPluginTest, RopeScaling_AllMethods) {
    // Test all scaling methods
    std::vector<RopeScalingMethod> methods = {
        RopeScalingMethod::LINEAR,
        RopeScalingMethod::NTK,
        RopeScalingMethod::YARN,
        RopeScalingMethod::DYNAMIC
    };
    
    for (auto method : methods) {
        LlamaWrapper::Config config;
        config.rope_scaling.enabled = true;
        config.rope_scaling.method = method;
        config.rope_scaling.max_context = 16384;
        config.rope_scaling.original_context = 4096;
        
        EXPECT_NO_THROW(LlamaWrapper wrapper(config));
    }
}

TEST_F(LLMPluginTest, RopeScaling_InvalidYarnParameters) {
    LlamaWrapper::Config config;
    
    config.rope_scaling.enabled = true;
    config.rope_scaling.method = RopeScalingMethod::YARN;
    config.rope_scaling.max_context = 32768;
    config.rope_scaling.original_context = 4096;
    
    // Test negative yarn_ext_factor
    config.rope_scaling.yarn_ext_factor = -1.0f;
    EXPECT_THROW(LlamaWrapper wrapper(config), std::invalid_argument);
    
    // Reset to valid value
    config.rope_scaling.yarn_ext_factor = 1.0f;
    
    // Test zero yarn_beta_fast
    config.rope_scaling.yarn_beta_fast = 0.0f;
    EXPECT_THROW(LlamaWrapper wrapper(config), std::invalid_argument);
}
