/*
 * ThemisDB | File: ai_plugin_generator.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 112
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #4930 [Docs][ai] Add missing AI module documentation in `src/ai` and `inc... (2026-05-10T17:25:05Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "plugins/self_healing_plugin.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

/**
 * @file ai_plugin_generator.h
 * @brief AI-based plugin code generation framework
 * 
 * This module enables automatic generation of ThemisDB plugins from
 * natural language descriptions using Large Language Models (LLMs).
 * 
 * Security features:
 * - Input sanitization (prompt injection prevention)
 * - Output validation (AST-based syntax checking)
 * - Sandboxed build environment
 * - Automated security analysis
 * - Code signing
 */

namespace themis {
namespace plugins {
namespace ai {

using json = nlohmann::json;

/**
 * @brief LLM model types for code generation
 */
enum class LLMModel {
    CODE_LLAMA,        ///< Meta's Code Llama (local)
    CODEX,             ///< OpenAI Codex (API)
    STARCODER,         ///< HuggingFace StarCoder (local)
    GITHUB_COPILOT,    ///< GitHub Copilot (API)
    CUSTOM             ///< Custom LLM endpoint
};

/**
 * @brief Security level for generated code
 */
enum class SecurityLevel {
    LOW,               ///< Basic syntax checking only
    MEDIUM,            ///< Static analysis + basic sandboxing
    HIGH,              ///< Full security analysis + strict sandboxing
    PARANOID           ///< Maximum security (may reject valid code)
};

/**
 * @brief Plugin generation prompt
 */
struct PluginGenerationPrompt {
    std::string description;
    PluginType type;
    std::vector<std::string> required_capabilities;
    std::vector<std::string> dependencies;
    LLMModel llm_model = LLMModel::CODE_LLAMA;
    SecurityLevel security_level = SecurityLevel::HIGH;
    bool generate_tests = true;
    bool generate_docs = true;
};

/**
 * @brief Generated plugin code and metadata
 */
struct GeneratedPlugin {
    std::string header_code;
    std::string implementation_code;
    std::string test_code;
    std::string cmake_code;
    PluginManifest manifest;
    std::vector<std::string> build_dependencies;
    bool passed_security_checks = false;
    std::string security_report;
};

/**
 * @brief AI Plugin Generator interface
 */
class AIPluginGenerator {
public:
    struct Config {
        std::string llm_endpoint = "http://localhost:8080";
        std::string sandbox_dir = "/tmp/themis_plugin_sandbox";
        std::string output_dir = "./generated_plugins";
    };
    
    explicit AIPluginGenerator(const Config& config);
    ~AIPluginGenerator();
    
    Result<GeneratedPlugin> generatePlugin(const PluginGenerationPrompt& prompt);
    Result<void> validatePrompt(const PluginGenerationPrompt& prompt);
    
private:
    Config config_;
};

} // namespace ai
} // namespace plugins
} // namespace themis
