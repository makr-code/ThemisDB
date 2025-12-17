#include <gtest/gtest.h>
#include "llm/gguf_loader.h"
#include "llm/llamacpp_inference_engine.h"
#include <fstream>

using namespace themis::llm;

class GGUFLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a mock GGUF file for testing
        test_file_ = "/tmp/test_model.gguf";
        createMockGGUFFile();
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
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
    GGUFLoader loader;
    ASSERT_TRUE(loader.parseFile(test_file_));
    
    std::string urn = loader.loadToThemisDB("test-model");
    EXPECT_EQ(urn, "urn:themis:model:test-model:v1");
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

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
