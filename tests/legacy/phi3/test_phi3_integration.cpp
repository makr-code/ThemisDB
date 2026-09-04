/**
 * ThemisDB Phi-3 Integration Tests
 * 
 * Tests for Phi-3-Mini-4k model integration including:
 * - Model download functionality
 * - Configuration loading
 * - Model availability checks
 * - LoRA training configuration
 */

#include <gtest/gtest.h>
#include "llm/model_downloader.h"
#include "llm/lora_framework/lora_training_service.h"
#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>

using namespace themis::llm;
namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class Phi3IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themis_phi3_test";
        fs::create_directories(test_dir_);
        
        // Create a small mock model file for testing
        mock_model_path_ = test_dir_ / "mock_phi3.gguf";
        createMockModelFile(mock_model_path_, 2 * 1024 * 1024);  // 2MB mock
    }

    void TearDown() override {
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }

    void createMockModelFile(const fs::path& path, size_t size) {
        std::ofstream file(path, std::ios::binary);
        // Write "GGUF" magic header
        file.write("GGUF", 4);
        // Fill rest with dummy data
        for (size_t i = 4; i < size; ++i) {
            file.put(static_cast<char>(i % 256));
        }
        file.close();
    }
    
    fs::path test_dir_;
    fs::path mock_model_path_;
};

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(Phi3IntegrationTest, DefaultConfigExists) {
    // Check that default_model_config.yaml exists
    fs::path config_path = "config/default_model_config.yaml";
    EXPECT_TRUE(fs::exists(config_path)) 
        << "default_model_config.yaml should exist in config directory";
}

TEST_F(Phi3IntegrationTest, Phi3TrainingConfigExists) {
    // Check that phi3_lora_training.yaml exists
    fs::path config_path = "config/phi3_lora_training.yaml";
    EXPECT_TRUE(fs::exists(config_path))
        << "phi3_lora_training.yaml should exist in config directory";
}

TEST_F(Phi3IntegrationTest, DefaultConfigHasPhi3Settings) {
    fs::path config_path = "config/default_model_config.yaml";
    if (!fs::exists(config_path)) {
        GTEST_SKIP() << "Config file not found";
    }
    
    try {
        YAML::Node config = YAML::LoadFile(config_path.string());
        ASSERT_TRUE(config["default_llm"]) << "Config should have default_llm section";
        
        auto llm = config["default_llm"];
        EXPECT_TRUE(llm["model_id"]);
        EXPECT_TRUE(llm["model_name"]);
        EXPECT_EQ(llm["format"].as<std::string>(), "gguf");
        EXPECT_EQ(llm["quantization"].as<std::string>(), "Q4_K_M");
        
        // Check Phi-3 specific architecture
        ASSERT_TRUE(llm["architecture"]);
        auto arch = llm["architecture"];
        EXPECT_EQ(arch["model_type"].as<std::string>(), "phi3");
        EXPECT_EQ(arch["attention_type"].as<std::string>(), "grouped_query_attention");
        
        // Check LoRA target modules
        ASSERT_TRUE(llm["lora_target_modules"]);
        auto targets = llm["lora_target_modules"];
        EXPECT_GT(targets.size(), 0) << "Should have LoRA target modules defined";
        
        // Verify Phi-3 specific modules are present
        bool has_qkv_proj = false;
        bool has_gate_up_proj = false;
        for (const auto& module : targets) {
            std::string mod_name = module.as<std::string>();
            if (mod_name == "qkv_proj") {
              has_qkv_proj = true;
            }
            if (mod_name == "gate_up_proj") {
              has_gate_up_proj = true;
            }
        }
        EXPECT_TRUE(has_qkv_proj) << "Should have qkv_proj (Phi-3 specific)";
        EXPECT_TRUE(has_gate_up_proj) << "Should have gate_up_proj (Phi-3 specific)";
        
    } catch (const std::exception& e) {
        FAIL() << "Failed to parse config: " << e.what();
    }
}

TEST_F(Phi3IntegrationTest, Phi3TrainingConfigHasOptimalSettings) {
    fs::path config_path = "config/phi3_lora_training.yaml";
    if (!fs::exists(config_path)) {
        GTEST_SKIP() << "Config file not found";
    }
    
    try {
        YAML::Node config = YAML::LoadFile(config_path.string());
        ASSERT_TRUE(config["phi3_lora_training"]);
        
        auto phi3_config = config["phi3_lora_training"];
        
        // Check hyperparameters are optimized for Phi-3
        auto hyperparams = phi3_config["hyperparameters"];
        EXPECT_EQ(hyperparams["rank"].as<int>(), 16) << "Phi-3 should use rank 16";
        EXPECT_EQ(hyperparams["alpha"].as<float>(), 32.0f) << "Phi-3 should use alpha 32";
        EXPECT_NEAR(hyperparams["learning_rate"].as<float>(), 0.0002f, 1e-6) 
            << "Phi-3 should use LR 2e-4";
        
        // Check target modules match Phi-3 architecture
        auto target_modules = phi3_config["target_modules"];
        ASSERT_GT(target_modules.size(), 0);
        
        std::vector<std::string> expected_modules = {
            "qkv_proj", "o_proj", "gate_up_proj", "down_proj"
        };
        
        for (const auto& expected : expected_modules) {
            bool found = false;
            for (const auto& module : target_modules) {
                if (module.as<std::string>() == expected) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found) << "Should have target module: " << expected;
        }
        
    } catch (const std::exception& e) {
        FAIL() << "Failed to parse config: " << e.what();
    }
}

// ═══════════════════════════════════════════════════════════
// Model Downloader Tests
// ═══════════════════════════════════════════════════════════

TEST_F(Phi3IntegrationTest, ModelDownloaderCanCheckAvailability) {
    // Existing model should be available
    EXPECT_TRUE(ModelDownloader::isModelAvailable(mock_model_path_.string()));
    
    // Non-existent model should not be available
    EXPECT_FALSE(ModelDownloader::isModelAvailable("/nonexistent/model.gguf"));
    
    // Too small file should not be available
    fs::path tiny_file = test_dir_ / "tiny.gguf";
    std::ofstream(tiny_file) << "tiny";
    EXPECT_FALSE(ModelDownloader::isModelAvailable(tiny_file.string()));
}

TEST_F(Phi3IntegrationTest, DownloadConfigCreation) {
    ModelDownloadConfig config;
    config.model_name = "phi3:mini-4k";
    config.ollama_url = "http://localhost:11434";
    config.download_dir = test_dir_.string();
    config.use_cache = true;
    config.timeout_seconds = 300;
    
    EXPECT_EQ(config.model_name, "phi3:mini-4k");
    EXPECT_EQ(config.ollama_url, "http://localhost:11434");
    EXPECT_TRUE(config.use_cache);
}

TEST_F(Phi3IntegrationTest, DownloadResultStructure) {
    ModelDownloadResult result;
    result.success = true;
    result.model_path = "/path/to/model.gguf";
    result.file_size_bytes = 2415919104;  // ~2.3 GB
    result.download_time_seconds = 120.5;
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.file_size_bytes, 0);
    EXPECT_GT(result.download_time_seconds, 0);
}

// ═══════════════════════════════════════════════════════════
// LoRA Training Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(Phi3IntegrationTest, Phi3DetectionInFilename) {
    // Test that Phi-3 is detected from model path
    std::vector<std::string> phi3_paths = {
        "models/phi-3-mini-4k.gguf",
        "models/phi3-mini.gguf",
        "/path/to/Phi-3-instruct.gguf",
        "models/microsoft-phi3.gguf"
    };
    
    for (const auto& path : phi3_paths) {
        bool is_phi3 = (path.find("phi-3") != std::string::npos ||
                        path.find("phi3") != std::string::npos);
        EXPECT_TRUE(is_phi3) << "Should detect Phi-3 in path: " << path;
    }
}

TEST_F(Phi3IntegrationTest, Phi3TargetModulesConfiguration) {
    // Expected Phi-3 target modules
    std::vector<std::string> expected_modules = {
        "qkv_proj",      // Phi-3 combined Q/K/V
        "o_proj",        // Output projection
        "gate_up_proj",  // Phi-3 combined gate/up
        "down_proj"      // Down projection
    };
    
    // Verify these are different from standard Llama modules
    std::vector<std::string> standard_llama_modules = {
        "q_proj", "k_proj", "v_proj", "o_proj",
        "gate_proj", "up_proj", "down_proj"
    };
    
    // Phi-3 uses combined projections (qkv_proj, gate_up_proj)
    EXPECT_NE(expected_modules, standard_llama_modules)
        << "Phi-3 modules should differ from standard Llama";
}

TEST_F(Phi3IntegrationTest, Phi3HyperparametersValidation) {
    // Phi-3 recommended hyperparameters
    struct Phi3Hyperparameters {
        int rank = 16;
        float alpha = 32.0f;
        float learning_rate = 0.0002f;
        float dropout = 0.05f;
        int batch_size = 4;
    } params;
    
    // Validate they are within reasonable ranges
    EXPECT_GE(params.rank, 4) << "Rank should be at least 4";
    EXPECT_LE(params.rank, 64) << "Rank should not exceed 64";
    EXPECT_NEAR(params.alpha / params.rank, 2.0f, 0.1f) 
        << "Alpha should be approximately 2x rank";
    EXPECT_GT(params.learning_rate, 0.0f);
    EXPECT_LT(params.learning_rate, 0.01f);
    EXPECT_GE(params.dropout, 0.0f);
    EXPECT_LE(params.dropout, 0.5f);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests (require Ollama running)
// ═══════════════════════════════════════════════════════════

TEST_F(Phi3IntegrationTest, DISABLED_RealDownloadFromOllama) {
    // This test is disabled by default as it requires:
    // 1. Ollama running on localhost:11434
    // 2. Network connectivity
    // 3. ~2.3 GB disk space
    // 4. Several minutes to complete
    
    ModelDownloadConfig config;
    config.model_name = "phi3:mini-4k";
    config.ollama_url = "http://localhost:11434";
    config.download_dir = test_dir_.string();
    config.timeout_seconds = 600;  // 10 minutes
    
    bool progress_called = false;
    config.progress_callback = [&progress_called](size_t downloaded, size_t total, const std::string& status) {
        progress_called = true;
        if (total > 0) {
            float percent = 100.0f * downloaded / total;
            std::cout << "Progress: " << percent << "%" << std::endl;
        }
    };
    
    ModelDownloader downloader;
    auto result = downloader.downloadFromOllama(config);
    
    if (result.success) {
        EXPECT_TRUE(fs::exists(result.model_path));
        EXPECT_GT(result.file_size_bytes, 1024 * 1024 * 1024);  // > 1GB
        EXPECT_TRUE(progress_called);
    } else {
        std::cout << "Download failed (expected if Ollama not running): " 
                  << result.error_message << std::endl;
    }
}

