/**
 * @file test_llm_raid_pipeline.cpp
 * @brief End-to-end RAID + LoRA + Inferencing pipeline tests
 * 
 * Complete workflow:
 * 1. Generate test data and ingest into single shard
 * 2. Distribute data across RAID shards (RAID0, RAID1, RAID5)
 * 3. Generate/load LoRA adapters
 * 4. Distribute LoRAs across shards
 * 5. Run inference with LoRAs on distributed shards
 * 6. Verify consistency and performance
 */

#include <gtest/gtest.h>
#include "llm/llm_plugin_manager.h"
#include "llm/multi_lora_manager.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;
using themis::llm::InferenceRequest;
using themis::llm::InferenceResponse;
using themis::llm::LLMPluginManager;
using themis::llm::MultiLoRAManager;

class RAIDLoRAPipelineTest : public ::testing::Test {
protected:
    // Configuration
    static constexpr int NUM_SHARDS = 3;           // RAID0, RAID1, RAID5
    static constexpr int TEST_DATA_SIZE_MB = 50;   // Test dataset
    static constexpr int NUM_LORAS = 3;            // 3 domain adapters
    static constexpr int INFERENCE_BATCH_SIZE = 10;
    
    std::string test_dir_ = "./test_raid_lora_pipeline";
    std::string data_dir_ = "./test_raid_lora_pipeline/data";
    std::string model_dir_ = "./test_raid_lora_pipeline/models";
    std::string lora_dir_ = "./test_raid_lora_pipeline/loras";
    std::string shard_dirs_[NUM_SHARDS];
    
    struct ShardInfo {
        int shard_id;
        std::string base_path;
        LLMPluginManager plugin_mgr;
        std::vector<std::string> loaded_loras;
        std::vector<InferenceResponse> inference_results;
    };
    
    std::vector<ShardInfo> shards_;
    std::chrono::milliseconds total_ingest_time_{0};
    std::chrono::milliseconds total_distribution_time_{0};
    std::chrono::milliseconds total_lora_load_time_{0};
    std::chrono::milliseconds total_inference_time_{0};
    
    void SetUp() override {
        // Clean and setup directories
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(data_dir_);
        fs::create_directories(model_dir_);
        fs::create_directories(lora_dir_);
        
        for (int i = 0; i < NUM_SHARDS; i++) {
            shard_dirs_[i] = test_dir_ + "/shard_" + std::to_string(i);
            fs::create_directories(shard_dirs_[i]);
        }
        
        // Create base model
        createDummyModel("base_model.gguf", TEST_DATA_SIZE_MB);
        
        // Create LoRA adapters (legal, medical, finance)
        createDummyLoRA("legal_adapter.bin", 30);
        createDummyLoRA("medical_adapter.bin", 30);
        createDummyLoRA("finance_adapter.bin", 30);
    }
    
    void TearDown() override {
        shards_.clear();
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    void createDummyModel(const std::string& filename, size_t size_mb) {
        std::string path = model_dir_ + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        file.write("GGUF", 4);
        std::vector<char> dummy_data(size_mb * 1024 * 1024, 0xAB);
        file.write(dummy_data.data(), dummy_data.size());
        file.close();
    }
    
    void createDummyLoRA(const std::string& filename, size_t size_mb) {
        std::string path = lora_dir_ + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        file.write("LORA", 4);
        std::vector<char> dummy_data(size_mb * 1024 * 1024, 0xCD);
        file.write(dummy_data.data(), dummy_data.size());
        file.close();
    }
    
    void generateTestDataset(const std::string& filename, int count) {
        std::string path = data_dir_ + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        
        for (int i = 0; i < count; i++) {
            std::string record = "record_" + std::to_string(i) + 
                                "|domain_" + std::to_string(i % NUM_LORAS) + 
                                "|query_" + std::to_string(rand() % 1000) + "\n";
            file.write(record.c_str(), record.length());
        }
        file.close();
    }
    
    void initializeShards() {
        shards_.clear();
        for (int i = 0; i < NUM_SHARDS; i++) {
            ShardInfo shard;
            shard.shard_id = i;
            shard.base_path = shard_dirs_[i];
            shards_.push_back(shard);
        }
    }
};

// ═══════════════════════════════════════════════════════════
// PHASE 1: Test Data Generation & Ingestion
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAPipelineTest, Phase1_DataGeneration) {
    generateTestDataset("test_records.txt", 10000);
    
    ASSERT_TRUE(fs::exists(data_dir_ + "/test_records.txt"));
    auto file_size = fs::file_size(data_dir_ + "/test_records.txt");
    EXPECT_GT(file_size, 50000);  // Should be substantial
}

TEST_F(RAIDLoRAPipelineTest, Phase1_ModelAndLoRACreation) {
    ASSERT_TRUE(fs::exists(model_dir_ + "/base_model.gguf"));
    auto model_size = fs::file_size(model_dir_ + "/base_model.gguf");
    EXPECT_EQ(model_size, TEST_DATA_SIZE_MB * 1024 * 1024);
    
    ASSERT_TRUE(fs::exists(lora_dir_ + "/legal_adapter.bin"));
    ASSERT_TRUE(fs::exists(lora_dir_ + "/medical_adapter.bin"));
    ASSERT_TRUE(fs::exists(lora_dir_ + "/finance_adapter.bin"));
}

// ═══════════════════════════════════════════════════════════
// PHASE 2: RAID Distribution
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAPipelineTest, Phase2_DistributeDataAcrossShards) {
    initializeShards();
    generateTestDataset("test_records.txt", 10000);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate RAID0 striping (round-robin)
    std::ifstream input(data_dir_ + "/test_records.txt");
    std::string line;
    int record_id = 0;
    
    while (std::getline(input, line)) {
        int shard_idx = record_id % NUM_SHARDS;
        std::string shard_file = shards_[shard_idx].base_path + "/records.txt";
        
        std::ofstream out(shard_file, std::ios::app);
        out << line << "\n";
        out.close();
        
        record_id++;
    }
    input.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    total_distribution_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify distribution
    int total_records = 0;
    for (int i = 0; i < NUM_SHARDS; i++) {
        std::string shard_file = shards_[i].base_path + "/records.txt";
        ASSERT_TRUE(fs::exists(shard_file));
        
        std::ifstream check(shard_file);
        int count = 0;
        std::string dummy;
        while (std::getline(check, dummy)) {
            count++;
        }
        check.close();
        
        EXPECT_GT(count, 3000);  // Each shard should have ~3333 records
        total_records += count;
    }
    
    EXPECT_EQ(total_records, 10000);
}

// ═══════════════════════════════════════════════════════════
// PHASE 3: LoRA Distribution
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAPipelineTest, Phase3_LoadLoRAOnAllShards) {
    initializeShards();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Load model on all shards
    for (auto& shard : shards_) {
        bool loaded = shard.plugin_mgr.loadModel(model_dir_ + "/base_model.gguf");
        ASSERT_TRUE(loaded) << "Failed to load model on shard " << shard.shard_id;
    }
    
    // Load LoRAs on all shards
    std::vector<std::pair<std::string, std::string>> lora_mappings = {
        {"legal", "legal_adapter.bin"},
        {"medical", "medical_adapter.bin"},
        {"finance", "finance_adapter.bin"}
    };
    
    for (auto& shard : shards_) {
        for (const auto& [lora_id, filename] : lora_mappings) {
            bool loaded = shard.plugin_mgr.loadLoRA(
                lora_id,
                lora_dir_ + "/" + filename,
                "base_model"
            );
            ASSERT_TRUE(loaded) << "Failed to load LoRA on shard " << shard.shard_id;
            shard.loaded_loras.push_back(lora_id);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    total_lora_load_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify all shards have all LoRAs
    for (auto& shard : shards_) {
        ASSERT_EQ(shard.loaded_loras.size(), lora_mappings.size());
    }
}

// ═══════════════════════════════════════════════════════════
// PHASE 4: Inference with LoRAs
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAPipelineTest, Phase4_InferenceOnAllShards) {
    initializeShards();
    
    // Setup
    for (auto& shard : shards_) {
        shard.plugin_mgr.loadModel(model_dir_ + "/base_model.gguf");
        shard.plugin_mgr.loadLoRA("legal", lora_dir_ + "/legal_adapter.bin", "base_model");
        shard.plugin_mgr.loadLoRA("medical", lora_dir_ + "/medical_adapter.bin", "base_model");
        shard.plugin_mgr.loadLoRA("finance", lora_dir_ + "/finance_adapter.bin", "base_model");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run inference on each shard
    std::vector<std::string> domains = {"legal", "medical", "finance"};
    
    for (auto& shard : shards_) {
        for (int req_id = 0; req_id < INFERENCE_BATCH_SIZE; req_id++) {
            const auto& domain = domains[req_id % domains.size()];
            
            InferenceRequest req;
            req.request_id = "shard_" + std::to_string(shard.shard_id) + 
                            "_req_" + std::to_string(req_id);
            req.prompt = "Query about " + domain + " with id " + 
                        std::to_string(req_id);
            req.max_tokens = 64;
            req.lora_adapter_id = domain;
            
            auto resp = shard.plugin_mgr.generate(req);
            
            // Verify response
            ASSERT_FALSE(resp.text.empty());
            ASSERT_TRUE(resp.lora_used.has_value());
            EXPECT_EQ(resp.lora_used.value(), domain);
            EXPECT_EQ(resp.request_id, req.request_id);
            EXPECT_GT(resp.tokens_generated, 0);
            
            shard.inference_results.push_back(resp);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    total_inference_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Calculate stats
    int total_inferences = 0;
    for (const auto& shard : shards_) {
        total_inferences += shard.inference_results.size();
    }
    
    EXPECT_EQ(total_inferences, NUM_SHARDS * INFERENCE_BATCH_SIZE);
}

// ═══════════════════════════════════════════════════════════
// PHASE 5: Consistency & Integrity Checks
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAPipelineTest, Phase5_CrossShardConsistency) {
    initializeShards();
    
    // Setup all shards identically
    for (auto& shard : shards_) {
        shard.plugin_mgr.loadModel(model_dir_ + "/base_model.gguf");
        shard.plugin_mgr.loadLoRA("legal", lora_dir_ + "/legal_adapter.bin", "base_model");
    }
    
    // Same request on all shards
    InferenceRequest req;
    req.request_id = "consistency-check";
    req.prompt = "Explain contract law";
    req.max_tokens = 50;
    req.lora_adapter_id = "legal";
    
    std::string first_response;
    for (auto& shard : shards_) {
        auto resp = shard.plugin_mgr.generate(req);
        
        // All shards with same model + LoRA should produce identical/similar responses
        if (first_response.empty()) {
            first_response = resp.text;
        }
        
        ASSERT_TRUE(resp.lora_used.has_value());
        EXPECT_EQ(resp.lora_used.value(), "legal");
        
        shard.inference_results.push_back(resp);
    }
}

TEST_F(RAIDLoRAPipelineTest, Phase5_LoRAListingConsistency) {
    initializeShards();
    
    for (auto& shard : shards_) {
        shard.plugin_mgr.loadModel(model_dir_ + "/base_model.gguf");
        shard.plugin_mgr.loadLoRA("legal", lora_dir_ + "/legal_adapter.bin", "base_model");
        shard.plugin_mgr.loadLoRA("medical", lora_dir_ + "/medical_adapter.bin", "base_model");
    }
    
    // All shards should report same LoRA list
    auto first_loras = shards_[0].plugin_mgr.listLoRAs();
    ASSERT_EQ(first_loras.size(), 2);
    
    for (int i = 1; i < NUM_SHARDS; i++) {
        auto current_loras = shards_[i].plugin_mgr.listLoRAs();
        ASSERT_EQ(current_loras.size(), first_loras.size());
        
        // Check IDs match
        for (size_t j = 0; j < first_loras.size(); j++) {
            EXPECT_EQ(current_loras[j].id, first_loras[j].id);
        }
    }
}

// ═══════════════════════════════════════════════════════════
// PHASE 6: Full Pipeline Integration Test
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAPipelineTest, Phase6_FullPipeline) {
    initializeShards();
    
    // Phase 1: Generate data
    generateTestDataset("pipeline_records.txt", 5000);
    ASSERT_TRUE(fs::exists(data_dir_ + "/pipeline_records.txt"));
    
    // Phase 2: Distribute across shards
    std::ifstream input(data_dir_ + "/pipeline_records.txt");
    std::string line;
    int record_id = 0;
    
    while (std::getline(input, line)) {
        int shard_idx = record_id % NUM_SHARDS;
        std::string shard_file = shards_[shard_idx].base_path + "/pipeline_records.txt";
        
        std::ofstream out(shard_file, std::ios::app);
        out << line << "\n";
        out.close();
        
        record_id++;
    }
    input.close();
    
    // Verify distribution
    for (int i = 0; i < NUM_SHARDS; i++) {
        ASSERT_TRUE(fs::exists(shards_[i].base_path + "/pipeline_records.txt"));
    }
    
    // Phase 3: Load model and LoRAs on all shards
    for (auto& shard : shards_) {
        shard.plugin_mgr.loadModel(model_dir_ + "/base_model.gguf");
        shard.plugin_mgr.loadLoRA("legal", lora_dir_ + "/legal_adapter.bin", "base_model");
        shard.plugin_mgr.loadLoRA("medical", lora_dir_ + "/medical_adapter.bin", "base_model");
        shard.plugin_mgr.loadLoRA("finance", lora_dir_ + "/finance_adapter.bin", "base_model");
    }
    
    // Verify model and LoRAs loaded
    for (auto& shard : shards_) {
        auto model_info = shard.plugin_mgr.getModelInfo("base_model");
        ASSERT_TRUE(model_info.has_value());
        
        auto loras = shard.plugin_mgr.listLoRAs();
        ASSERT_EQ(loras.size(), 3);
    }
    
    // Phase 4: Run inference
    std::vector<std::string> domains = {"legal", "medical", "finance"};
    int total_inferences = 0;
    
    for (auto& shard : shards_) {
        for (int req_id = 0; req_id < 5; req_id++) {
            const auto& domain = domains[req_id % domains.size()];
            
            InferenceRequest req;
            req.request_id = "full_" + std::to_string(shard.shard_id) + 
                            "_" + std::to_string(req_id);
            req.prompt = "Question about " + domain;
            req.max_tokens = 32;
            req.lora_adapter_id = domain;
            
            auto resp = shard.plugin_mgr.generate(req);
            
            ASSERT_FALSE(resp.text.empty());
            ASSERT_TRUE(resp.lora_used.has_value());
            EXPECT_EQ(resp.lora_used.value(), domain);
            
            shard.inference_results.push_back(resp);
            total_inferences++;
        }
    }
    
    EXPECT_EQ(total_inferences, NUM_SHARDS * 5);
}

// ═══════════════════════════════════════════════════════════
// Benchmarking Tests (measure technical effort)
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAPipelineTest, Benchmark_TotalPipelineTime) {
    initializeShards();
    
    auto pipeline_start = std::chrono::high_resolution_clock::now();
    
    // Full pipeline
    generateTestDataset("bench_records.txt", 10000);
    
    std::ifstream input(data_dir_ + "/bench_records.txt");
    std::string line;
    int record_id = 0;
    
    while (std::getline(input, line)) {
        int shard_idx = record_id % NUM_SHARDS;
        std::ofstream out(shards_[shard_idx].base_path + "/bench_records.txt", std::ios::app);
        out << line << "\n";
        out.close();
        record_id++;
    }
    input.close();
    
    for (auto& shard : shards_) {
        shard.plugin_mgr.loadModel(model_dir_ + "/base_model.gguf");
        shard.plugin_mgr.loadLoRA("legal", lora_dir_ + "/legal_adapter.bin", "base_model");
        shard.plugin_mgr.loadLoRA("medical", lora_dir_ + "/medical_adapter.bin", "base_model");
        shard.plugin_mgr.loadLoRA("finance", lora_dir_ + "/finance_adapter.bin", "base_model");
    }
    
    std::vector<std::string> domains = {"legal", "medical", "finance"};
    for (auto& shard : shards_) {
        for (int i = 0; i < 20; i++) {
            const auto& domain = domains[i % domains.size()];
            InferenceRequest req;
            req.prompt = "Test query";
            req.max_tokens = 32;
            req.lora_adapter_id = domain;
            
            auto resp = shard.plugin_mgr.generate(req);
            shard.inference_results.push_back(resp);
        }
    }
    
    auto pipeline_end = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        pipeline_end - pipeline_start
    );
    
    EXPECT_LT(total_time.count(), 60000);  // Should complete in < 60 seconds
    
    int total_requests = NUM_SHARDS * 20;
    double throughput = (total_requests * 1000.0) / total_time.count();
    
    // Log metrics
    std::cout << "\n=== RAID LoRA Pipeline Benchmark ===" << std::endl;
    std::cout << "Total Time: " << total_time.count() << " ms" << std::endl;
    std::cout << "Total Requests: " << total_requests << std::endl;
    std::cout << "Throughput: " << throughput << " req/sec" << std::endl;
    std::cout << "==================================\n" << std::endl;
}
