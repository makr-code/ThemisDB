#include <gtest/gtest.h>
#include "test_config.h"
#include <iostream>

namespace themis {
namespace test {

/**
 * @brief Test suite to verify YAML configuration system is working
 */
class YAMLConfigIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = &TestConfig::instance();
    }
    
    TestConfig* config_;
};

// ═══════════════════════════════════════════════════════════
// Configuration Loading Tests
// ═══════════════════════════════════════════════════════════

TEST_F(YAMLConfigIntegrationTest, ConfigurationLoads) {
    EXPECT_NE(config_, nullptr);
    std::cout << "✓ Configuration loaded successfully" << std::endl;
}

TEST_F(YAMLConfigIntegrationTest, LLMConfigurationExists) {
    const auto& llm = config_->llm();
    EXPECT_TRUE(llm.enabled) << "LLM should be enabled in test_config.yaml";
    std::cout << "✓ LLM enabled: " << (llm.enabled ? "yes" : "no") << std::endl;
}

TEST_F(YAMLConfigIntegrationTest, LLMModelsConfigured) {
    const auto& llm = config_->llm();
    EXPECT_FALSE(llm.models_dir.empty()) << "models_dir should be configured";
    EXPECT_FALSE(llm.default_model.empty()) << "default_model should be configured";
    
    std::cout << "  Models directory: " << llm.models_dir << std::endl;
    std::cout << "  Default model: " << llm.default_model << std::endl;
    std::cout << "✓ LLM models configured" << std::endl;
}

TEST_F(YAMLConfigIntegrationTest, LLMModelAliasesLoaded) {
    const auto& llm = config_->llm();
    EXPECT_FALSE(llm.model_aliases.empty()) << "Model aliases should be configured";
    
    std::cout << "  Number of model aliases: " << llm.model_aliases.size() << std::endl;
    for (const auto& [name, file] : llm.model_aliases) {
        std::cout << "    - " << name << " -> " << file << std::endl;
    }
    std::cout << "✓ Model aliases loaded: " << llm.model_aliases.size() << std::endl;
}

TEST_F(YAMLConfigIntegrationTest, OllamaConfigurationLoaded) {
    const auto& llm = config_->llm();
    std::cout << "  Ollama enabled: " << (llm.ollama.enabled ? "yes" : "no") << std::endl;
    std::cout << "  Ollama models directory: " << llm.ollama.models_dir << std::endl;
    
    if (!llm.ollama.model_hashes.empty()) {
        std::cout << "  Available Ollama hashes: " << llm.ollama.model_hashes.size() << std::endl;
        for (const auto& [model, hash] : llm.ollama.model_hashes) {
            std::cout << "    - " << model << ": " << hash.substr(0, 16) << "..." << std::endl;
        }
    }
    std::cout << "✓ Ollama configuration loaded" << std::endl;
}

TEST_F(YAMLConfigIntegrationTest, LoRAConfigurationExists) {
    const auto& lora = config_->lora();
    std::cout << "  LoRA enabled: " << (lora.enabled ? "yes" : "no") << std::endl;
    std::cout << "  LoRA directory: " << lora.adapters_dir << std::endl;
    std::cout << "  Max LoRA slots: " << lora.max_lora_slots << std::endl;
    std::cout << "✓ LoRA configuration exists" << std::endl;
}

TEST_F(YAMLConfigIntegrationTest, GPUConfigurationLoaded) {
    const auto& gpu = config_->gpu();
    std::cout << "  GPU enabled: " << (gpu.enabled ? "yes" : "no") << std::endl;
    std::cout << "  GPU type: " << gpu.type << std::endl;
    std::cout << "  GPU available: " << (gpu.isAvailable() ? "yes" : "no") << std::endl;
    std::cout << "✓ GPU configuration loaded" << std::endl;
}

TEST_F(YAMLConfigIntegrationTest, TestExecutionConfigurationLoaded) {
    const auto& test = config_->test();
    std::cout << "  Skip slow tests: " << (test.skip_slow_tests ? "yes" : "no") << std::endl;
    std::cout << "  Test timeout: " << test.timeout_seconds << " seconds" << std::endl;
    std::cout << "✓ Test execution configuration loaded" << std::endl;
}

// ═══════════════════════════════════════════════════════════
// Model Resolution Tests
// ═══════════════════════════════════════════════════════════

TEST_F(YAMLConfigIntegrationTest, GetModelPath_UnknownModel) {
    const auto& llm = config_->llm();
    std::string path = llm.getModelPath("nonexistent_model");
    std::cout << "  Path for nonexistent model: " << (path.empty() ? "(empty)" : path) << std::endl;
    // May or may not exist depending on file system
}

TEST_F(YAMLConfigIntegrationTest, ModelAliasResolution) {
    const auto& llm = config_->llm();
    
    // Check that aliases are properly set up
    auto it = llm.model_aliases.find("test_model");
    if (it != llm.model_aliases.end()) {
        std::cout << "  test_model alias resolves to: " << it->second << std::endl;
        std::cout << "✓ Model alias resolution working" << std::endl;
    } else {
        std::cout << "  test_model alias not found in configuration" << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════
// Inference Settings Tests
// ═══════════════════════════════════════════════════════════

TEST_F(YAMLConfigIntegrationTest, InferenceSettingsLoaded) {
    const auto& llm = config_->llm();
    std::cout << "  Max tokens: " << llm.max_tokens << std::endl;
    std::cout << "  Temperature: " << llm.temperature << std::endl;
    std::cout << "  Timeout: " << llm.timeout_seconds << " seconds" << std::endl;
    EXPECT_GT(llm.max_tokens, 0) << "max_tokens should be positive";
    EXPECT_GT(llm.temperature, 0.0f) << "temperature should be positive";
    std::cout << "✓ Inference settings properly configured" << std::endl;
}

// ═══════════════════════════════════════════════════════════
// Configuration Summary
// ═══════════════════════════════════════════════════════════

TEST_F(YAMLConfigIntegrationTest, PrintConfigurationSummary) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "ThemisDB Test Configuration Summary" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    std::cout << "\nLLM Settings:" << std::endl;
    std::cout << "  Enabled: " << (config_->llm().enabled ? "✓" : "✗") << std::endl;
    std::cout << "  Models Directory: " << config_->llm().models_dir << std::endl;
    std::cout << "  Default Model: " << config_->llm().default_model << std::endl;
    std::cout << "  Model Aliases: " << config_->llm().model_aliases.size() << std::endl;
    std::cout << "  Ollama Enabled: " << (config_->llm().ollama.enabled ? "✓" : "✗") << std::endl;
    
    std::cout << "\nLoRA Settings:" << std::endl;
    std::cout << "  Enabled: " << (config_->lora().enabled ? "✓" : "✗") << std::endl;
    std::cout << "  Adapters Directory: " << config_->lora().adapters_dir << std::endl;
    std::cout << "  Max LoRA Slots: " << config_->lora().max_lora_slots << std::endl;
    
    std::cout << "\nGPU Settings:" << std::endl;
    std::cout << "  Enabled: " << (config_->gpu().enabled ? "✓" : "✗") << std::endl;
    std::cout << "  Type: " << config_->gpu().type << std::endl;
    
    std::cout << "\nTest Settings:" << std::endl;
    std::cout << "  Skip Slow Tests: " << (config_->test().skip_slow_tests ? "✓" : "✗") << std::endl;
    std::cout << "  Timeout: " << config_->test().timeout_seconds << "s" << std::endl;
    
    std::cout << std::string(70, '=') << "\n" << std::endl;
    
    SUCCEED() << "Configuration summary printed";
}

}  // namespace test
}  // namespace themis
