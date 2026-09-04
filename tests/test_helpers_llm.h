#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "test_config.h"

namespace themis {
namespace test {

/**
 * @brief Versucht den Pfad zu einem LLM-Modell zu ermitteln
 * @return std::optional mit Pfad oder std::nullopt wenn nicht verfügbar
 * @note Wirft KEINE Exception - Test muss GTEST_SKIP() selbst aufrufen
 */
inline std::optional<std::string> tryGetModelPath(const std::string& model_name = "") {
    namespace fs = std::filesystem;
    auto& config = TestConfig::instance();
    if (!config.llm().enabled) {
      return std::nullopt;
    }
    
    std::string target = model_name.empty() ? config.llm().default_model : model_name;
    std::string path = config.llm().getModelPath(target);

    if (!path.empty() && fs::exists(path)) {
        return path;
    }

    // Fallback for local development: resolve models relative to repository.
    std::string filename = target;
    auto alias_it = config.llm().model_aliases.find(target);
    if (alias_it != config.llm().model_aliases.end()) {
        filename = alias_it->second;
    }

    const fs::path repo_models = fs::path(__FILE__).parent_path().parent_path() / "models";
    if (fs::exists(repo_models) && fs::is_directory(repo_models)) {
        if (!filename.empty()) {
            const fs::path candidate = repo_models / filename;
            if (fs::exists(candidate)) {
                return candidate.string();
            }
        }

        if (model_name.empty()) {
            for (const auto& entry : fs::directory_iterator(repo_models)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                const auto ext = entry.path().extension().string();
                if (ext == ".gguf" || ext == ".bin") {
                    return entry.path().string();
                }
            }
        }
    }

    return std::nullopt;
}

/**
 * @brief Versucht den Modellverzeichnis-Pfad zu ermitteln
 * @return std::optional mit Pfad oder std::nullopt wenn nicht verfügbar
 */
inline std::optional<std::string> tryGetModelDir() {
    auto& config = TestConfig::instance();
    if (!config.llm().enabled) {
      return std::nullopt;
    }
    
    std::string models_dir = config.llm().models_dir;
    if (!std::filesystem::exists(models_dir)) {
      return std::nullopt;
    }
    return models_dir;
}

/**
 * @brief Versucht den Pfad zu einem LoRA-Adapter zu ermitteln
 * @return std::optional mit Pfad oder std::nullopt wenn nicht verfügbar
 */
inline std::optional<std::string> tryGetLoRAAdapterPath(const std::string& adapter_name) {
    auto& config = TestConfig::instance();
    if (!config.lora().enabled) {
      return std::nullopt;
    }
    
    std::string path = config.lora().getAdapterPath(adapter_name);
    if (path.empty() || !std::filesystem::exists(path)) {
      return std::nullopt;
    }
    return path;
}

/**
 * @brief Prüft ob echte Modelle verfügbar sind (ohne Test zu überspringen)
 */
inline bool hasRealModels() {
    return tryGetModelPath().has_value();
}

/**
 * @brief Prüft ob GPU verfügbar ist (ohne Test zu überspringen)
 */
inline bool hasGPU() {
    auto& config = TestConfig::instance();
    return config.gpu().enabled && config.gpu().isAvailable();
}

/**
 * @brief Prüft ob langsame Tests deaktiviert sind
 */
inline bool shouldSkipSlowTest() {
    auto& config = TestConfig::instance();
    return config.test().skip_slow_tests;
}

}  // namespace test
}  // namespace themis

namespace ollama_models {
    constexpr const char* LLAMA3_2_SHA256 = "dde5aa3fc5ffc17176b5e8bdc82f587b24b2678c6c66101bf7da77af9f7ccdff";
}
