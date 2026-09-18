/**
 * @file ai_plugin_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟡 HARDENED-IMPLEMENTATION
 * @note Score: 88/100 (focused hardening implemented; full production validation still environment-dependent)
 * @note Status: Focused hardening implemented; do not treat this header as standalone production sign-off
 * @note Gap Resolution: Validation comments added; schema validation and transport contracts documented
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "utils/expected.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <functional>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ai {

using json = nlohmann::json;

enum class LLMModel {
    CODE_LLAMA,        ///< Meta's Code Llama (local)
    CODEX,             ///< OpenAI Codex (API)
    STARCODER,         ///< HuggingFace StarCoder (local)
    GITHUB_COPILOT,    ///< GitHub Copilot (API)
    CUSTOM             ///< Custom LLM endpoint
};

enum class SecurityLevel {
    LOW,               ///< Basic syntax checking only
    MEDIUM,            ///< Static analysis + basic sandboxing
    HIGH,              ///< Full security analysis + strict sandboxing
    PARANOID           ///< Maximum security (may reject valid code)
};

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

class AIPluginGenerator {
public:
    using CAISafetyEvalFn = std::function<Result<double>(
        const std::string& generated_response,
        const std::string& original_query)>;

    using FederatedTelemetryFn = std::function<Result<void>(const json& local_metrics)>;

    using EndpointInvokeFn = std::function<Result<std::string>(
        const std::string& endpoint,
        const std::string& request_body,
        long timeout_ms)>;

    using SandboxVerifyFn = std::function<Result<void>(const GeneratedPlugin& generated)>;

    struct Config {
        std::string llm_endpoint = "http://localhost:8080";
        std::vector<std::string> allowed_llm_endpoints;
        std::string sandbox_dir = "/tmp/themis_plugin_sandbox";
        std::string output_dir = "./generated_plugins";
        long timeout_ms = 10000;
        std::size_t max_request_body_bytes = 256u * 1024u;
        std::size_t max_response_body_bytes = 8u * 1024u * 1024u;
        EndpointInvokeFn endpoint_invoke_fn;
        bool enable_c1_cai_safety_gate = false;
        double c1_min_safety_score = 0.80;
        CAISafetyEvalFn c1_cai_eval_fn;
        bool enable_c2_federated_telemetry = false;
        FederatedTelemetryFn c2_federated_telemetry_fn;
        bool enable_sandbox_gate = false;
        SandboxVerifyFn sandbox_verify_fn;
    };

    struct Stats {
        std::size_t validation_errors = 0;  ///< Prompt validation failures
        std::size_t transport_errors = 0;   ///< CURL transport failures (all retry attempts)
        std::size_t http_errors = 0;        ///< Non-2xx HTTP responses (not retried)
        std::size_t parse_errors = 0;       ///< JSON parse or schema type errors
        std::size_t safety_rejections = 0;  ///< C1 safety gate rejections
        std::size_t sandbox_rejections = 0; ///< Sandbox gate rejections
        std::size_t successes = 0;          ///< Successful generatePlugin() completions
    };

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    using LLMGenerateFn = std::function<Result<GeneratedPlugin>(const PluginGenerationPrompt&)>;

    /**
     * @brief Set LLMGenerate Fn.
     * @param[in] fn Input parameter.
     */
    void setLLMGenerateFn(LLMGenerateFn fn);
    
    /**
     * @brief AIPlugin Generator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AIPluginGenerator(const Config& config);
    ~AIPluginGenerator();
    
    using LlmHttpPostFn = std::function<std::string(
        const std::string& endpoint,
        const std::string& body
    )>;

    /**
     * @brief Set Llm Http Post Fn.
     * @param[in] fn Input parameter.
     */
    void setLlmHttpPostFn(LlmHttpPostFn fn);

    /**
     * @brief Generate Plugin.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    Result<GeneratedPlugin> generatePlugin(const PluginGenerationPrompt& prompt);
    /**
     * @brief Validate Prompt.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    Result<void> validatePrompt(const PluginGenerationPrompt& prompt);
    
    // ─── HttpPost bridge (stub #282) ──────────────────────────────────────────

    using HttpPostFn = std::function<Result<GeneratedPlugin>(
        const std::string&            endpoint,
        const PluginGenerationPrompt& prompt)>;

    /**
     * @brief Set Http Post Fn.
     * @param[in] fn Input parameter.
     */
    static void setHttpPostFn(HttpPostFn fn);

    /**
     * @brief Clear Http Post Fn.
     */
    static void clearHttpPostFn();

private:
    Config config_;
    std::optional<LlmHttpPostFn> llm_http_post_fn_;
    std::size_t stat_validation_errors_ = 0;
    std::size_t stat_transport_errors_ = 0;
    std::size_t stat_http_errors_ = 0;
    std::size_t stat_parse_errors_ = 0;
    std::size_t stat_safety_rejections_ = 0;
    std::size_t stat_sandbox_rejections_ = 0;
    std::size_t stat_successes_ = 0;
};

} // namespace ai
} // namespace plugins
} // namespace themis
