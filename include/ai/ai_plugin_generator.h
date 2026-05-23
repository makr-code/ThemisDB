/*
 * ThemisDB | File: ai_plugin_generator.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=2; TODO=0, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
#include <functional>
#include <mutex>
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

    /**
     * @brief Injectable bridge for LLM-based plugin code generation.
     *
     * @param prompt  The validated generation prompt.
     * @return Expected<GeneratedPlugin, Error> produced by the LLM back-end.
     *
     * Set via setLLMGenerateFn() to wire a real HTTP POST to `config_.llm_endpoint`
     * (or any other generation strategy) without recompiling the generator.
     * When not set, generatePlugin() returns ERR_PLUGIN_LOAD_FAILED to signal
     * that no LLM back-end is available (stub #282 resolution).
     */
    using LLMGenerateFn = std::function<Result<GeneratedPlugin>(const PluginGenerationPrompt&)>;

    /**
     * @brief Install the LLM generation bridge (thread-safe).
     * @param fn  Callable invoked by generatePlugin() after input validation.
     *            Pass nullptr to revert to the not-wired error response.
     */
    void setLLMGenerateFn(LLMGenerateFn fn);
    
    explicit AIPluginGenerator(const Config& config);
    ~AIPluginGenerator();
    
    /**
     * @brief Function type for delivering an HTTP POST to the LLM endpoint.
     *
     * Parameters:
     *   - endpoint : Full URL of the LLM code-generation endpoint.
     *   - body     : JSON request body (serialised PluginGenerationPrompt fields).
     *
     * Returns the raw HTTP response body as a string.
     * Must throw on network or HTTP errors.
     */
    using LlmHttpPostFn = std::function<std::string(
        const std::string& endpoint,
        const std::string& body
    )>;

    /**
     * @brief Inject a real HTTP transport for LLM endpoint calls (resolves stub #282).
     *
     * When set, `generatePlugin()` performs an HTTP POST to `config_.llm_endpoint`
     * via this function and parses the JSON response into a `GeneratedPlugin`.
     * Without an injected function the call returns an error indicating that
     * Phase 2 is not available in this build.
     *
     * @param fn  Callable that performs the HTTP POST and returns the response body.
     */
    void setLlmHttpPostFn(LlmHttpPostFn fn);

    Result<GeneratedPlugin> generatePlugin(const PluginGenerationPrompt& prompt);
    Result<void> validatePrompt(const PluginGenerationPrompt& prompt);
    
private:
    Config config_;
    std::optional<LlmHttpPostFn> llm_http_post_fn_;
};

} // namespace ai
} // namespace plugins
} // namespace themis
