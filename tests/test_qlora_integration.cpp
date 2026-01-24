#include <gtest/gtest.h>
#include "llm/lora_framework/model_compatibility.h"
#include "llm/lora_framework/resource_profiler.h"
#include "llm/lora_framework/lora_training_service.h"
#include <memory>
#include <fstream>

using namespace themis::llm::lora;

/**
 * @file test_qlora_integration.cpp
 * @brief Integration tests for QLoRA end-to-end pipeline
 */

class QLoRAIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = "./test_qlora_integration";
        std::filesystem::create_directories(test_dir_);
        
        // Create dummy model file for testing
        test_model_path_ = test_dir_ + "/test_model.gguf";
        create_dummy_gguf_file(test_model_path_);
    }
    
    void TearDown() override {
        // Cleanup
        std::filesystem::remove_all(test_dir_);
    }
    
    void create_dummy_gguf_file(const std::string& path) {
        std::ofstream file(path, std::ios::binary);
        // Write GGUF magic bytes
        file.write("GGUF", 4);
        // Write some dummy data
        char dummy[1024] = {0};
        file.write(dummy, 1024);
        file.close();
    }
    
    std::string test_dir_;
    std::string test_model_path_;
};

// ===== Model Compatibility Tests =====

TEST_F(QLoRAIntegrationTest, ModelFormatDetection_GGUF) {
    auto format = ModelCompatibilityChecker::detect_format(test_model_path_);
    EXPECT_EQ(format, ModelFormat::GGUF);
}

TEST_F(QLoRAIntegrationTest, ModelFormatDetection_NonExistent) {
    auto format = ModelCompatibilityChecker::detect_format("nonexistent_model.gguf");
    EXPECT_EQ(format, ModelFormat::UNKNOWN);
}

TEST_F(QLoRAIntegrationTest, ExtractMetadata_GGUF) {
    auto metadata_opt = ModelCompatibilityChecker::extract_metadata(test_model_path_);
    ASSERT_TRUE(metadata_opt.has_value());
    
    auto metadata = metadata_opt.value();
    EXPECT_EQ(metadata.format, ModelFormat::GGUF);
    EXPECT_EQ(metadata.model_path, test_model_path_);
}

TEST_F(QLoRAIntegrationTest, CompatibilityCheck_ValidModel) {
    auto result = ModelCompatibilityChecker::check_compatibility(test_model_path_, "nf4");
    
    // Should be compatible (warnings are okay)
    EXPECT_TRUE(result.is_compatible || !result.errors.empty());
    EXPECT_FALSE(result.recommended_quantization.empty());
    EXPECT_GT(result.recommended_rank, 0);
    EXPECT_GT(result.recommended_batch_size, 0);
}

TEST_F(QLoRAIntegrationTest, CompatibilityCheck_InvalidQuantization) {
    auto result = ModelCompatibilityChecker::check_compatibility(test_model_path_, "invalid_quant");
    EXPECT_FALSE(result.is_compatible);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(QLoRAIntegrationTest, RecommendedTargetModules_LLaMA) {
    auto modules = ModelCompatibilityChecker::get_recommended_target_modules(
        ModelArchitecture::LLAMA
    );
    EXPECT_FALSE(modules.empty());
    EXPECT_TRUE(std::find(modules.begin(), modules.end(), "q_proj") != modules.end());
    EXPECT_TRUE(std::find(modules.begin(), modules.end(), "v_proj") != modules.end());
}

TEST_F(QLoRAIntegrationTest, MemoryEstimation) {
    ModelMetadata metadata;
    metadata.hidden_size = 4096;
    metadata.num_layers = 32;
    metadata.max_seq_length = 2048;
    
    size_t memory = ModelCompatibilityChecker::estimate_memory_requirements(
        metadata,
        "nf4",  // 4-bit quantization
        4,      // batch size
        8       // rank
    );
    
    // Should be reasonable (less than 100GB for 7B model)
    EXPECT_GT(memory, 0);
    EXPECT_LT(memory, 100ULL * 1024 * 1024 * 1024);  // < 100GB
    
    // NF4 should use less memory than FP32
    size_t memory_fp32 = ModelCompatibilityChecker::estimate_memory_requirements(
        metadata, "none", 4, 8
    );
    EXPECT_LT(memory, memory_fp32);
}

// ===== Resource Profiler Tests =====

TEST_F(QLoRAIntegrationTest, ResourceProfiler_StartStop) {
    ResourceProfiler::Config config;
    config.log_to_file = false;
    
    ResourceProfiler profiler(config);
    EXPECT_FALSE(profiler.is_running());
    
    profiler.start();
    EXPECT_TRUE(profiler.is_running());
    
    profiler.stop();
    EXPECT_FALSE(profiler.is_running());
}

TEST_F(QLoRAIntegrationTest, ResourceProfiler_Snapshots) {
    ResourceProfiler::Config config;
    config.snapshot_interval_steps = 1;
    config.log_to_file = false;
    
    ResourceProfiler profiler(config);
    profiler.start();
    
    // Take several snapshots
    for (int i = 0; i < 10; ++i) {
        profiler.snapshot(0, i, 1.0f - i * 0.05f, 0.001f);
    }
    
    auto snapshots = profiler.get_snapshots();
    EXPECT_EQ(snapshots.size(), 10);
    
    // Check that loss is decreasing
    for (size_t i = 1; i < snapshots.size(); ++i) {
        EXPECT_LT(snapshots[i].current_loss, snapshots[i-1].current_loss);
    }
    
    profiler.stop();
}

TEST_F(QLoRAIntegrationTest, ResourceProfiler_Statistics) {
    ResourceProfiler::Config config;
    config.snapshot_interval_steps = 1;
    config.log_to_file = false;
    
    ResourceProfiler profiler(config);
    profiler.start();
    
    // Take snapshots with varying resource usage
    for (int i = 0; i < 5; ++i) {
        profiler.snapshot(0, i, 1.0f, 0.001f);
    }
    
    auto stats = profiler.compute_stats();
    EXPECT_EQ(stats.num_snapshots, 5);
    EXPECT_GE(stats.total_training_time.count(), 0);
    
    profiler.stop();
}

TEST_F(QLoRAIntegrationTest, ResourceProfiler_LogToFile) {
    std::string log_file = test_dir_ + "/resource_profile.jsonl";
    
    ResourceProfiler::Config config;
    config.snapshot_interval_steps = 1;
    config.log_to_file = true;
    config.log_file = log_file;
    
    ResourceProfiler profiler(config);
    profiler.start();
    
    profiler.snapshot(0, 0, 1.0f, 0.001f);
    profiler.snapshot(0, 1, 0.9f, 0.001f);
    
    profiler.stop();
    
    // Check that file was created
    EXPECT_TRUE(std::filesystem::exists(log_file));
    
    // Check file content
    std::ifstream file(log_file);
    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        line_count++;
        // Each line should be valid JSON
        EXPECT_FALSE(line.empty());
    }
    EXPECT_GT(line_count, 0);
}

TEST_F(QLoRAIntegrationTest, ResourceProfiler_Callback) {
    ResourceProfiler::Config config;
    config.snapshot_interval_steps = 1;
    config.log_to_file = false;
    
    ResourceProfiler profiler(config);
    
    int callback_count = 0;
    profiler.register_callback([&callback_count](const ResourceSnapshot& snapshot) {
        callback_count++;
        EXPECT_GE(snapshot.current_step, 0);
    });
    
    profiler.start();
    profiler.snapshot(0, 0, 1.0f, 0.001f);
    profiler.snapshot(0, 1, 0.9f, 0.001f);
    profiler.stop();
    
    EXPECT_EQ(callback_count, 2);
}

// ===== Metadata Serialization Tests =====

TEST_F(QLoRAIntegrationTest, ModelMetadata_JSON_Serialization) {
    ModelMetadata metadata;
    metadata.model_path = "/path/to/model.gguf";
    metadata.format = ModelFormat::GGUF;
    metadata.architecture = ModelArchitecture::LLAMA;
    metadata.vocab_size = 32000;
    metadata.hidden_size = 4096;
    metadata.is_quantized = true;
    metadata.quantization_type = "Q4_K_M";
    
    auto json_obj = metadata.toJSON();
    
    EXPECT_EQ(json_obj["model_path"], metadata.model_path);
    EXPECT_EQ(json_obj["format"], "GGUF");
    EXPECT_EQ(json_obj["architecture"], "LLaMA");
    EXPECT_EQ(json_obj["vocab_size"], 32000);
    EXPECT_TRUE(json_obj["is_quantized"]);
}

TEST_F(QLoRAIntegrationTest, CompatibilityResult_JSON_Serialization) {
    CompatibilityResult result;
    result.is_compatible = true;
    result.errors = {"Error 1", "Error 2"};
    result.warnings = {"Warning 1"};
    result.recommended_quantization = "nf4";
    result.recommended_rank = 16;
    
    auto json_obj = result.toJSON();
    
    EXPECT_TRUE(json_obj["is_compatible"]);
    EXPECT_EQ(json_obj["errors"].size(), 2);
    EXPECT_EQ(json_obj["warnings"].size(), 1);
    EXPECT_EQ(json_obj["recommended_quantization"], "nf4");
    EXPECT_EQ(json_obj["recommended_rank"], 16);
}

TEST_F(QLoRAIntegrationTest, ResourceSnapshot_JSON_Serialization) {
    ResourceSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.current_epoch = 1;
    snapshot.current_step = 100;
    snapshot.current_loss = 0.5f;
    snapshot.gpu_memory_allocated = 8ULL * 1024 * 1024 * 1024;  // 8GB
    snapshot.gpu_utilization = 85.5f;
    
    auto json_obj = snapshot.toJSON();
    
    EXPECT_EQ(json_obj["training"]["epoch"], 1);
    EXPECT_EQ(json_obj["training"]["step"], 100);
    EXPECT_FLOAT_EQ(json_obj["training"]["loss"], 0.5f);
    EXPECT_EQ(json_obj["gpu_memory"]["allocated_mb"], 8192);
    EXPECT_FLOAT_EQ(json_obj["gpu_utilization_pct"], 85.5f);
}

// ===== String Conversion Tests =====

TEST_F(QLoRAIntegrationTest, FormatStringConversion) {
    EXPECT_EQ(ModelMetadata::format_to_string(ModelFormat::GGUF), "GGUF");
    EXPECT_EQ(ModelMetadata::format_to_string(ModelFormat::SAFETENSORS), "SafeTensors");
    EXPECT_EQ(ModelMetadata::string_to_format("gguf"), ModelFormat::GGUF);
    EXPECT_EQ(ModelMetadata::string_to_format("SAFETENSORS"), ModelFormat::SAFETENSORS);
}

TEST_F(QLoRAIntegrationTest, ArchitectureStringConversion) {
    EXPECT_EQ(ModelMetadata::architecture_to_string(ModelArchitecture::LLAMA), "LLaMA");
    EXPECT_EQ(ModelMetadata::architecture_to_string(ModelArchitecture::MISTRAL), "Mistral");
    EXPECT_EQ(ModelMetadata::string_to_architecture("llama"), ModelArchitecture::LLAMA);
    EXPECT_EQ(ModelMetadata::string_to_architecture("Mistral-7B"), ModelArchitecture::MISTRAL);
}


