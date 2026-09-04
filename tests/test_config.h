#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

namespace themis {
namespace test {

/**
 * @brief Central configuration for ThemisDB tests
 * 
 * Loads configuration from test_config.yaml in the tests directory.
 * Can be overridden via THEMIS_TEST_CONFIG environment variable.
 * 
 * Usage:
 *   auto& config = TestConfig::instance();
 *   if (config.llm().enabled) {
 *       std::string model_path = config.llm().getModelPath("llama-2-7b");
 *       // Use model...
 *   }
 */
class TestConfig {
public:
    // ═══════════════════════════════════════════════════════════
    // Nested Configuration Structures
    // ═══════════════════════════════════════════════════════════
    
    struct OllamaConfig {
        bool enabled = false;
        std::string models_dir = {};
        std::map<std::string, std::string> model_hashes;
    };
    
    struct LLMConfig {
        bool enabled = true;
        std::string models_dir = "./models";
        OllamaConfig ollama;
        std::map<std::string, std::string> model_aliases;
        std::string default_model = "test_model";
        
        int max_tokens = 512;
        float temperature = 0.7f;
        int timeout_seconds = 120;
        
        /**
         * @brief Get absolute path to a model file
         * @param model_name Model name or alias (e.g., "llama-2-7b", "test_model")
         * @return Full path to model file, or empty if not found
         */
        std::string getModelPath(const std::string& model_name) const {
            // Check if it's an alias
            auto alias_it = model_aliases.find(model_name);
            std::string filename = (alias_it != model_aliases.end()) 
                ? alias_it->second 
                : model_name;
            
            // Try Ollama first if enabled
            if (ollama.enabled && !ollama.models_dir.empty()) {
                auto hash_it = ollama.model_hashes.find(model_name);
                if (hash_it != ollama.model_hashes.end()) {
                    std::string ollama_path = ollama.models_dir + "/sha256-" + hash_it->second;
                    if (std::filesystem::exists(ollama_path)) {
                        return ollama_path;
                    }
                }
            }
            
            // Try local models directory
            std::string local_path = models_dir + "/" + filename;
            if (std::filesystem::exists(local_path)) {
                return local_path;
            }
            
            return "";  // Not found
        }
    };
    
    struct LoRAConfig {
        bool enabled = true;
        std::string adapters_dir = "./lora_adapters";
        std::map<std::string, std::string> test_adapters;
        int max_lora_slots = 16;
        int max_lora_vram_mb = 2048;
        int ttl_seconds = 300;
        
        /**
         * @brief Get absolute path to a LoRA adapter file
         * @param adapter_name Adapter name (e.g., "legal-qa")
         * @return Full path to adapter file, or empty if not found
         */
        std::string getAdapterPath(const std::string& adapter_name) const {
            auto it = test_adapters.find(adapter_name);
            if (it != test_adapters.end()) {
                std::string path = adapters_dir + "/" + it->second;
                if (std::filesystem::exists(path)) {
                    return path;
                }
            }
            return "";
        }
    };
    
    struct GPUConfig {
        bool enabled = false;
        std::string type = "none";  // "cuda", "hip", "none"
        std::vector<int> devices;
        
        bool isCUDA() const { return type == "cuda"; }
        bool isHIP() const { return type == "hip"; }
        bool isAvailable() const { return enabled && (isCUDA() || isHIP()); }
    };
    
    struct TestExecutionConfig {
        bool skip_slow_tests = false;
        int timeout_seconds = 300;
        int retry_count = 0;
        std::string fixtures_dir = "./test_fixtures";
        std::string temp_dir = "./test_temp";
    };
    
    // ═══════════════════════════════════════════════════════════
    // Singleton Access
    // ═══════════════════════════════════════════════════════════
    
    static TestConfig& instance() {
        static TestConfig config;
        return config;
    }
    
    // Delete copy/move constructors (singleton)
    TestConfig(const TestConfig&) = delete;
    TestConfig& operator=(const TestConfig&) = delete;
    TestConfig(TestConfig&&) = delete;
    TestConfig& operator=(TestConfig&&) = delete;
    
    // ═══════════════════════════════════════════════════════════
    // Configuration Accessors
    // ═══════════════════════════════════════════════════════════
    
    const LLMConfig& llm() const { return llm_; }
    const LoRAConfig& lora() const { return lora_; }
    const GPUConfig& gpu() const { return gpu_; }
    const TestExecutionConfig& test() const { return test_; }
    
    /**
     * @brief Reload configuration from file
     * @param config_path Optional path to config file (defaults to test_config.yaml)
     * @return true if loaded successfully
     */
    bool reload(const std::string& config_path = "") {
        try {
            std::string path = config_path.empty() ? getDefaultConfigPath() : config_path;
            
            if (!std::filesystem::exists(path)) {
                spdlog::warn("Test config not found: {}, using defaults", path);
                return false;
            }
            
            YAML::Node config = YAML::LoadFile(path);
            parseConfig(config);
            
            spdlog::info("Test configuration loaded from: {}", path);
            return true;
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to load test config: {}", e.what());
            return false;
        }
    }
    
private:
    TestConfig() {
        // Try to load config on construction
        reload();
    }
    
    std::string getDefaultConfigPath() const {
        // Check environment variable first
        const char* env_path = std::getenv("THEMIS_TEST_CONFIG");
        if (env_path) {
            return env_path;
        }
        
        // Default: test_config.yaml in tests directory
        // Assuming tests are run from build directory
        std::vector<std::string> candidate_paths = {
            "../tests/test_config.yaml",
            "../../tests/test_config.yaml",
            "../../../tests/test_config.yaml",
            "./tests/test_config.yaml",
            "./test_config.yaml"
        };

        candidate_paths.push_back(
            (std::filesystem::path(__FILE__).parent_path() / "test_config.yaml").string());
        
        for (const auto& path : candidate_paths) {
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
        
        return "test_config.yaml";
    }
    
    void parseConfig(const YAML::Node& config) {
        // Parse LLM configuration
        if (config["llm"]) {
            auto llm_node = config["llm"];
            llm_.enabled = llm_node["enabled"].as<bool>(true);
            llm_.models_dir = llm_node["models_dir"].as<std::string>("./models");
            llm_.default_model = llm_node["default_model"].as<std::string>("test_model");
            
            if (llm_node["ollama"]) {
                auto ollama = llm_node["ollama"];
                llm_.ollama.enabled = ollama["enabled"].as<bool>(false);
                llm_.ollama.models_dir = ollama["models_dir"].as<std::string>("");
                
                if (ollama["models"]) {
                    for (auto model : ollama["models"]) {
                        std::string name = model.first.as<std::string>();
                        std::string hash = model.second["hash"].as<std::string>("");
                        if (!hash.empty()) {
                            llm_.ollama.model_hashes[name] = hash;
                        }
                    }
                }
            }
            
            if (llm_node["models"]) {
                for (auto model : llm_node["models"]) {
                    llm_.model_aliases[model.first.as<std::string>()] = 
                        model.second.as<std::string>();
                }
            }
            
            if (llm_node["inference"]) {
                auto inf = llm_node["inference"];
                llm_.max_tokens = inf["max_tokens"].as<int>(512);
                llm_.temperature = inf["temperature"].as<float>(0.7f);
                llm_.timeout_seconds = inf["timeout_seconds"].as<int>(120);
            }
        }
        
        // Parse LoRA configuration
        if (config["lora"]) {
            auto lora_node = config["lora"];
            lora_.enabled = lora_node["enabled"].as<bool>(true);
            lora_.adapters_dir = lora_node["adapters_dir"].as<std::string>("./lora_adapters");
            
            if (lora_node["test_adapters"]) {
                for (auto adapter : lora_node["test_adapters"]) {
                    lora_.test_adapters[adapter.first.as<std::string>()] = 
                        adapter.second.as<std::string>();
                }
            }
            
            if (lora_node["settings"]) {
                auto settings = lora_node["settings"];
                lora_.max_lora_slots = settings["max_lora_slots"].as<int>(16);
                lora_.max_lora_vram_mb = settings["max_lora_vram_mb"].as<int>(2048);
                lora_.ttl_seconds = settings["ttl_seconds"].as<int>(300);
            }
        }
        
        // Parse GPU configuration
        if (config["gpu"]) {
            auto gpu_node = config["gpu"];
            gpu_.enabled = gpu_node["enabled"].as<bool>(false);
            gpu_.type = gpu_node["type"].as<std::string>("none");
            
            if (gpu_node["cuda"] && gpu_node["cuda"]["devices"]) {
                gpu_.devices = gpu_node["cuda"]["devices"].as<std::vector<int>>();
            }
        }
        
        // Parse test execution configuration
        if (config["test"]) {
            auto test_node = config["test"];
            test_.skip_slow_tests = test_node["skip_slow_tests"].as<bool>(false);
            test_.timeout_seconds = test_node["timeout_seconds"].as<int>(300);
            test_.retry_count = test_node["retry_count"].as<int>(0);
            
            if (test_node["data"]) {
                auto data = test_node["data"];
                test_.fixtures_dir = data["fixtures_dir"].as<std::string>("./test_fixtures");
                test_.temp_dir = data["temp_dir"].as<std::string>("./test_temp");
            }
        }
    }
    
    // Configuration storage
    LLMConfig llm_;
    LoRAConfig lora_;
    GPUConfig gpu_;
    TestExecutionConfig test_;
};

}  // namespace test
}  // namespace themis
