/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ai_plugin_generator.h                              ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     126                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
