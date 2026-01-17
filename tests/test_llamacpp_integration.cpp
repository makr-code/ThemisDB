#include <gtest/gtest.h>
#include "llm/gguf_loader.h"
#include "llm/llamacpp_inference_engine.h"
#include "llm/paged_block_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <fstream>
#include <chrono>
#if __cplusplus >= 201703L
#include <filesystem>
#endif

using namespace themis::llm;

class GGUFLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a mock GGUF file for testing
        test_file_ = "/tmp/test_model.gguf";
        createMockGGUFFile();
        
        // Create a test database for RocksDB tests
        themis::RocksDBWrapper::Config db_config;
        db_config.db_path = "/tmp/test_gguf_loader_db";
        db_config.enable_blobdb = true;
        db_config.blob_size_threshold = 4096;
        db_ = std::make_unique<themis::RocksDBWrapper>(db_config);
        db_->open();
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
        if (db_) {
            db_->close();
            db_.reset();
        }
        // Clean up test database using filesystem
        #if __cplusplus >= 201703L
        std::filesystem::remove_all("/tmp/test_gguf_loader_db");
        #else
        system("rm -rf /tmp/test_gguf_loader_db");
        #endif
    }
    
    void createMockGGUFFile() {
        std::ofstream file(test_file_, std::ios::binary);
        // Write GGUF magic number
        file.write("GGUF", 4);
        // Write version
        uint32_t version = 3;
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        // Write some dummy data
        std::vector<char> dummy(1024, 0);
        file.write(dummy.data(), dummy.size());
        file.close();
    }
    
    std::string test_file_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
};

TEST_F(GGUFLoaderTest, ParseValidFile) {
    GGUFLoader loader;
    EXPECT_TRUE(loader.parseFile(test_file_));
    
    const auto& metadata = loader.getMetadata();
    EXPECT_EQ(metadata.version, "3");
    EXPECT_FALSE(metadata.tensors.empty());
}

TEST_F(GGUFLoaderTest, ParseInvalidFile) {
    GGUFLoader loader;
    EXPECT_FALSE(loader.parseFile("/nonexistent/file.gguf"));
}

TEST_F(GGUFLoaderTest, GetTensorMetadata) {
    GGUFLoader loader;
    ASSERT_TRUE(loader.parseFile(test_file_));
    
    const auto& metadata = loader.getMetadata();
    EXPECT_GT(metadata.tensors.size(), 0);
    
    // Check first tensor
    const auto& tensor = metadata.tensors[0];
    EXPECT_FALSE(tensor.name.empty());
    EXPECT_GT(tensor.shape.size(), 0);
    EXPECT_FALSE(tensor.dtype.empty());
}

TEST_F(GGUFLoaderTest, LoadToThemisDB) {
    GGUFLoader loader(db_.get());
    ASSERT_TRUE(loader.parseFile(test_file_));
    
    std::string urn = loader.loadToThemisDB("test-model");
    EXPECT_EQ(urn, "urn:themis:model:test-model:v1");
    
    // Verify metadata was stored
    auto metadata_key = "llm:model:test-model:metadata";
    auto metadata_result = db_->get(metadata_key);
    EXPECT_TRUE(metadata_result.has_value());
    
    // Verify URN mapping was stored
    auto urn_key = "llm:model:urn:" + urn;
    auto urn_result = db_->get(urn_key);
    EXPECT_TRUE(urn_result.has_value());
}

TEST_F(GGUFLoaderTest, MemoryMappedAccess) {
    GGUFLoader loader;
    ASSERT_TRUE(loader.parseFile(test_file_));
    
    const auto& metadata = loader.getMetadata();
    if (!metadata.tensors.empty()) {
        const auto& tensor_name = metadata.tensors[0].name;
        void* ptr = loader.mmapTensor(tensor_name);
        EXPECT_NE(ptr, nullptr);
        
        loader.unmapTensor(ptr);
    }
}

class LlamaCppInferenceEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.n_ctx = 2048;
        config_.n_gpu_layers = 0;  // CPU only for tests
        config_.n_threads = 2;
        config_.gpu_backend = "cpu";
        config_.use_mmap = true;
        config_.block_size = 16;
        config_.num_blocks = 256;
    }
    
    LlamaCppInferenceEngine::Config config_;
};

TEST_F(LlamaCppInferenceEngineTest, Initialization) {
    LlamaCppInferenceEngine engine(config_);
    EXPECT_EQ(engine.getModelInfo(), "No model loaded");
}

TEST_F(LlamaCppInferenceEngineTest, LoadModel) {
    // Create mock GGUF file
    std::string test_file = "/tmp/test_model_engine.gguf";
    std::ofstream file(test_file, std::ios::binary);
    file.write("GGUF", 4);
    uint32_t version = 3;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    std::vector<char> dummy(1024, 0);
    file.write(dummy.data(), dummy.size());
    file.close();
    
    LlamaCppInferenceEngine engine(config_);
    EXPECT_TRUE(engine.loadModel(test_file, "test-model"));
    
    std::string info = engine.getModelInfo();
    EXPECT_NE(info.find("test-model"), std::string::npos);
    
    std::remove(test_file.c_str());
}

TEST_F(LlamaCppInferenceEngineTest, InferenceWithoutModel) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test prompt";
    
    EXPECT_THROW(engine.infer(request), std::runtime_error);
}

TEST_F(LlamaCppInferenceEngineTest, InferenceWithModel) {
    // Create mock GGUF file
    std::string test_file = "/tmp/test_model_infer.gguf";
    std::ofstream file(test_file, std::ios::binary);
    file.write("GGUF", 4);
    uint32_t version = 3;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    std::vector<char> dummy(1024, 0);
    file.write(dummy.data(), dummy.size());
    file.close();
    
    LlamaCppInferenceEngine engine(config_);
    ASSERT_TRUE(engine.loadModel(test_file, "test-model"));
    
    InferenceRequest request;
    request.request_id = "req-123";
    request.prompt = "What is ThemisDB?";
    request.max_tokens = 100;
    
    InferenceResponse response = engine.infer(request);
    
    EXPECT_EQ(response.request_id, "req-123");
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
    EXPECT_GT(response.latency_ms, 0);
    
    std::remove(test_file.c_str());
}

TEST_F(LlamaCppInferenceEngineTest, GetStats) {
    LlamaCppInferenceEngine engine(config_);
    
    auto stats = engine.getStats();
    EXPECT_EQ(stats.total_tokens_processed, 0);
    EXPECT_EQ(stats.cache_hits, 0);
    EXPECT_EQ(stats.cache_misses, 0);
}

TEST_F(LlamaCppInferenceEngineTest, UnloadModel) {
    std::string test_file = "/tmp/test_model_unload.gguf";
    std::ofstream file(test_file, std::ios::binary);
    file.write("GGUF", 4);
    uint32_t version = 3;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    std::vector<char> dummy(1024, 0);
    file.write(dummy.data(), dummy.size());
    file.close();
    
    LlamaCppInferenceEngine engine(config_);
    ASSERT_TRUE(engine.loadModel(test_file, "test-model"));
    
    engine.unloadModel();
    EXPECT_EQ(engine.getModelInfo(), "No model loaded");
    
    std::remove(test_file.c_str());
}

TEST_F(LlamaCppInferenceEngineTest, InferenceRuntimeBudget) {
    // Ensure inference and initial GPU upload (when applicable) stay within expected budgets
    std::string test_file = "/tmp/test_model_runtime.gguf";
    std::ofstream file(test_file, std::ios::binary);
    file.write("GGUF", 4);
    uint32_t version = 3;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    std::vector<char> dummy(2048, 0);
    file.write(dummy.data(), dummy.size());
    file.close();

    LlamaCppInferenceEngine engine(config_);

    auto load_start = std::chrono::steady_clock::now();
    ASSERT_TRUE(engine.loadModel(test_file, "runtime-model"));
    auto load_end = std::chrono::steady_clock::now();
    auto load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_start).count();

    // Loading may include VRAM upload; allow a more generous budget for that path
    EXPECT_LT(load_ms, 2000);

    InferenceRequest request;
    request.request_id = "runtime-1";
    request.prompt = "Say hello";
    request.max_tokens = 32;

    // Warm-up once to ensure caches and GPU residency where applicable
    (void)engine.infer(request);

    auto start = std::chrono::steady_clock::now();
    InferenceResponse response = engine.infer(request);
    auto end = std::chrono::steady_clock::now();
    auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(response.request_id, "runtime-1");
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
    EXPECT_GE(response.latency_ms, 0);

    // Guardrail: steady-state inference should be fast (< 500ms)
    EXPECT_LT(latency_ms, 500);

    engine.unloadModel();
    std::remove(test_file.c_str());
}

// Benchmark tests
TEST(GGUFLoaderBenchmark, ParsePerformance) {
    // Simple performance test
    std::string test_file = "/tmp/bench_model.gguf";
    std::ofstream file(test_file, std::ios::binary);
    file.write("GGUF", 4);
    uint32_t version = 3;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    std::vector<char> dummy(10 * 1024 * 1024, 0);  // 10 MB
    file.write(dummy.data(), dummy.size());
    file.close();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    GGUFLoader loader;
    loader.parseFile(test_file);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should parse reasonably fast (< 100ms for 10MB file)
    EXPECT_LT(duration.count(), 100);
    
    std::remove(test_file.c_str());
}

// Test PagedBlockManager integration
TEST_F(LlamaCppInferenceEngineTest, PagedBlockManagerIntegration) {
    // Test 1: Engine creates block manager automatically
    LlamaCppInferenceEngine engine1(config_);
    auto stats1 = engine1.getStats();
    EXPECT_EQ(stats1.total_tokens_processed, 0);
    
    // Test 2: Engine accepts provided block manager
    PagedBlockManager::Config bm_config;
    bm_config.max_blocks = 512;
    bm_config.block_size_tokens = 32;
    bm_config.token_size_bytes = 4;
    
    auto block_manager = std::make_shared<PagedBlockManager>(bm_config);
    
    LlamaCppInferenceEngine::Config config2 = config_;
    config2.block_manager = block_manager;
    config2.block_size = 32;
    config2.num_blocks = 512;
    
    LlamaCppInferenceEngine engine2(config2);
    auto stats2 = engine2.getStats();
    EXPECT_EQ(stats2.total_tokens_processed, 0);
    
    // Verify block manager is being used (check it's not null by successful init)
    EXPECT_EQ(engine2.getModelInfo(), "No model loaded");
}

// No custom main; gtest_main provides the entry point
