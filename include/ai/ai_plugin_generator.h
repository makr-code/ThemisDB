/**
 * @file ai_plugin_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "utils/expected.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

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
    using CAISafetyEvalFn = std::function<Result<double>(
        const std::string& generated_response,
        const std::string& original_query)>;

    using FederatedTelemetryFn = std::function<Result<void>(const json& local_metrics)>;

    using EndpointInvokeFn = std::function<Result<std::string>(
        const std::string& endpoint,
        const std::string& request_body,
        long timeout_ms)>;

    /// @brief Callback for optional sandbox/static-analysis verification of generated code.
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
        /// @brief Enable optional sandbox verification gate for generated code artifacts.
        bool enable_sandbox_gate = false;
        /// @brief Callback invoked to verify a generated plugin before it is returned.
        SandboxVerifyFn sandbox_verify_fn;
    };

    /**
     * @brief Snapshot of per-instance counters for observability.
     *
     * All counts are accumulated since construction. Not thread-safe for
     * concurrent generatePlugin() calls; use external synchronisation if needed.
     */
    struct Stats {
        std::size_t validation_errors = 0;  ///< Prompt validation failures
        std::size_t transport_errors = 0;   ///< CURL transport failures (all retry attempts)
        std::size_t http_errors = 0;        ///< Non-2xx HTTP responses (not retried)
        std::size_t parse_errors = 0;       ///< JSON parse or schema type errors
        std::size_t safety_rejections = 0;  ///< C1 safety gate rejections
        std::size_t sandbox_rejections = 0; ///< Sandbox gate rejections
        std::size_t successes = 0;          ///< Successful generatePlugin() completions
    };

    /// @brief Return a snapshot of the current observability counters.
    Stats getStats() const;

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
    
    // ─── HttpPost bridge (stub #282) ──────────────────────────────────────────

    /// @brief Type alias for LLM HTTP POST injection.
    using HttpPostFn = std::function<Result<GeneratedPlugin>(
        const std::string&            endpoint,
        const PluginGenerationPrompt& prompt)>;

    /**
     * @brief Install an HTTP POST callable used by generatePlugin() to contact
     *        the LLM code-generation endpoint.
     *
     * When set, generatePlugin() delegates to this function instead of returning
     * ERR_PLUGIN_LOAD_FAILED.  Replaces the Phase-1 placeholder entirely.
     * @param fn Callable receiving (endpoint_url, prompt) → Result<GeneratedPlugin>.
     */
    static void setHttpPostFn(HttpPostFn fn);

    /**
     * @brief Remove the HTTP POST bridge (reverts to Phase-1 error return).
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
