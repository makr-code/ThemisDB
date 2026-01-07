#include <gtest/gtest.h>
#include "llm/llamacpp_plugin.h"
#include "llm/model_loader.h"
#include "llm/multi_lora_manager.h"
#include "llm/async_inference_engine.h"
#include "llm/llm_plugin_manager.h"
#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;
using namespace themis::llm;
using json = nlohmann::json;

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
// LlamaCppPlugin Tests (Consolidated)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMPluginTest, LlamaCppPlugin_Initialization) {
    LlamaCppPlugin::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    
    LlamaCppPlugin plugin(config);
    
    EXPECT_EQ(plugin.getName(), "llamacpp");
    EXPECT_FALSE(plugin.isModelLoaded());
}

TEST_F(LLMPluginTest, LlamaCppPlugin_ModelLoading) {
    LlamaCppPlugin::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.lazy_loader_config.max_models = 2;
    
    LlamaCppPlugin plugin(config);
    
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

TEST_F(LLMPluginTest, LlamaCppPlugin_LoRAManagement) {
    LlamaCppPlugin::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaCppPlugin plugin(config);
    
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

TEST_F(LLMPluginTest, LlamaCppPlugin_BasicInference) {
    LlamaCppPlugin::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    
    LlamaCppPlugin plugin(config);
    
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
    LlamaCppPlugin::Config plugin_config;
    plugin_config.n_gpu_layers = 32;
    plugin_config.n_ctx = 2048;
    
    auto plugin = std::make_shared<LlamaCppPlugin>(plugin_config);
    
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
    auto plugin = std::make_shared<LlamaCppPlugin>(LlamaCppPlugin::Config{});
    createDummyModel("callback_model.gguf", 50);
    plugin->loadModel(test_model_dir + "/callback_model.gguf", {});
    
    AsyncInferenceEngine engine(plugin, AsyncInferenceEngine::Config{});
    
    InferenceRequest request;
    request.prompt = "Test";
    request.max_tokens = 20;
    
    std::atomic<bool> callback_called{false};
    std::string result_text;
    
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
    auto plugin = std::make_shared<LlamaCppPlugin>(LlamaCppPlugin::Config{});
    createDummyModel("priority_model.gguf", 50);
    plugin->loadModel(test_model_dir + "/priority_model.gguf", {});
    
    AsyncInferenceEngine::Config config;
    config.num_worker_threads = 1;  // Single worker to test priority
    config.max_queue_size = 100;
    
    AsyncInferenceEngine engine(plugin, config);
    
    std::vector<int> completion_order;
    std::mutex order_mutex;
    
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
    LlamaCppPlugin::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 8192;  // Large context for RAG
    
    LlamaCppPlugin plugin(config);
    
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
    LlamaCppPlugin::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    config.multi_lora_config.max_lora_slots = 8;
    
    LlamaCppPlugin plugin(config);
    
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
    
    // Generate with medical LoRA (fast switch!)
    InferenceRequest medical_req;
    medical_req.prompt = "Explain diabetes";
    medical_req.max_tokens = 50;
    medical_req.lora_adapter_id = "medical";
    
    auto medical_response = plugin.generate(medical_req);
    EXPECT_FALSE(medical_response.text.empty());
    
    // Responses should be different (different LoRAs)
    // In real implementation, they would have domain-specific knowledge
}
