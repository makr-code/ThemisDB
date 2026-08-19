/**
 * @file ai_plugin_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟡 HARDENED-IMPLEMENTATION
 * @note Score: 88/100 (focused hardening implemented; full production validation still environment-dependent)
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
 * @brief Plugin generation prompt with AI model configuration and feature controls.
 *
 * Describes the intent and parameters for LLM-based code generation. All fields are validated
 * by validatePrompt() before endpoint invocation to ensure reasonable size bounds, token format,
 * and dependency consistency.
 */
struct PluginGenerationPrompt {
    /// Human-readable description of the plugin to be generated (required, ≤8192 chars).
    std::string description;
    /// Plugin type enum (e.g., DATA_SOURCE, PROCESSOR, SINK).
    PluginType type;
    /// List of required capabilities/features the plugin must implement (≤64 entries, ≤128 chars each).
    std::vector<std::string> required_capabilities;
    /// Build/runtime dependencies required by the generated code (≤64 entries, ≤256 chars each).
    std::vector<std::string> dependencies;
    /// LLM model to use for generation (CODE_LLAMA, CODEX, STARCODER, GITHUB_COPILOT, or CUSTOM).
    LLMModel llm_model = LLMModel::CODE_LLAMA;
    /// Security analysis level for generated code (LOW, MEDIUM, HIGH, PARANOID).
    SecurityLevel security_level = SecurityLevel::HIGH;
    /// Whether to generate unit tests for the plugin (default: true).
    bool generate_tests = true;
    /// Whether to generate API documentation for the plugin (default: true).
    bool generate_docs = true;
};

/**
 * @brief Generated plugin code and metadata returned by generatePlugin().
 *
 * Contains the complete generated source code (header, implementation, tests, CMake config),
 * plugin manifest with metadata, and security evaluation results. All code fields are validated
 * before return to ensure reasonable size bounds (≤1 MiB each) and required field-shape checks.
 */
struct GeneratedPlugin {
    /// Generated plugin header code (C/C++, ≤1 MiB).
    std::string header_code;
    /// Generated plugin implementation code (C/C++, ≤1 MiB).
    std::string implementation_code;
    /// Generated unit test code (C++, ≤1 MiB).
    std::string test_code;
    /// Generated CMakeLists.txt configuration (≤1 MiB).
    std::string cmake_code;
    /// Plugin metadata (name, version, description, type).
    PluginManifest manifest;
    /// Build dependencies detected/required by the generated code (oversized entries filtered).
    std::vector<std::string> build_dependencies;
    /// Whether the generated code passed security checks (set by security gate if enabled).
    bool passed_security_checks = false;
    /// Security evaluation report (≤64 KiB; populated by C1/sandbox/telemetry gates).
    std::string security_report;
};

/**
 * @brief AI Plugin Generator interface
 */
class AIPluginGenerator {
public:
    /// @brief Callback for Constitutional AI safety evaluation (Wave C C1 feature).
    /// Evaluates generated code semantics and context against safety principles.
    /// @param generated_response The generated code or response to evaluate.
    /// @param original_query The original user query for context.
    /// @return Safety score (0–1 range) or Error if evaluation fails.
    using CAISafetyEvalFn = std::function<Result<double>(
        const std::string& generated_response,
        const std::string& original_query)>;

    /// @brief Callback for federated telemetry collection (Wave C C2 feature).
    /// Collects local metrics for distributed privacy-preserving training.
    /// @param local_metrics JSON object containing local runtime metrics.
    /// @return Success or Error if telemetry reporting fails.
    using FederatedTelemetryFn = std::function<Result<void>(const json& local_metrics)>;

    /// @brief Callback for endpoint invocation with custom transport logic.
    /// Allows injection of retry wrappers, mock implementations, or alternative transports.
    /// @param endpoint Full URL of the LLM endpoint.
    /// @param request_body JSON request body as a string.
    /// @param timeout_ms Maximum timeout in milliseconds.
    /// @return Raw HTTP response body or Error if invocation fails.
    using EndpointInvokeFn = std::function<Result<std::string>(
        const std::string& endpoint,
        const std::string& request_body,
        long timeout_ms)>;

    /// @brief Callback for optional sandbox/static-analysis verification of generated code.
    /// Performs optional verification (static analysis, sandboxed execution, etc.) before
    /// returning the generated plugin to the caller.
    /// @param generated The GeneratedPlugin to verify.
    /// @return Success or Error if verification fails (will reject the plugin).
    using SandboxVerifyFn = std::function<Result<void>(const GeneratedPlugin& generated)>;

    struct Config {
        /// Full URL of the LLM endpoint (http/https) used for code generation.
        std::string llm_endpoint = "http://localhost:8080";
        /// Optional allowlist of permitted LLM endpoints (if empty, all endpoints allowed).
        std::vector<std::string> allowed_llm_endpoints;
        /// Directory used for sandbox verification work artifacts when enable_sandbox_gate=true.
        /// Generated source files and a manifest snapshot are materialized into a per-run subdirectory.
        std::string sandbox_dir = "/tmp/themis_plugin_sandbox";
        /// Directory where generated plugin artifacts are persisted for diagnostics/audit when
        /// enable_sandbox_gate=true. A per-run copy of the generated source bundle is written here.
        std::string output_dir = "./generated_plugins";
        /// Maximum HTTP timeout for endpoint invocation (milliseconds).
        long timeout_ms = 10000;
        /// Maximum allowed size of serialized request JSON body sent to endpoint (256 KiB default).
        std::size_t max_request_body_bytes = 256u * 1024u;
        /// Maximum allowed size of HTTP response body before parsing (8 MiB default).
        std::size_t max_response_body_bytes = 8u * 1024u * 1024u;
        /// Callback for endpoint invocation (overrides default CURL implementation if set).
        EndpointInvokeFn endpoint_invoke_fn;
        /// Enable Constitutional AI (CAI) safety gate (Wave C C1 feature).
        bool enable_c1_cai_safety_gate = false;
        /// Minimum acceptable CAI safety score (0–1 range; default 0.80).
        double c1_min_safety_score = 0.80;
        /// Callback for CAI safety evaluation (required if enable_c1_cai_safety_gate=true).
        CAISafetyEvalFn c1_cai_eval_fn;
        /// Enable federated telemetry collection (Wave C C2 feature).
        bool enable_c2_federated_telemetry = false;
        /// Callback for federated telemetry reporting (required if enable_c2_federated_telemetry=true).
        FederatedTelemetryFn c2_federated_telemetry_fn;
        /// Enable optional sandbox verification gate for generated code artifacts.
        /// When enabled, the generator materializes generated files into sandbox_dir/output_dir,
        /// verifies that the artifact bundle can be written/read back fail-closed, and then
        /// optionally invokes sandbox_verify_fn for additional policy checks.
        bool enable_sandbox_gate = false;
        /// Optional callback invoked after built-in artifact materialization/verification succeeds.
        /// Return an Error to reject the generated plugin.
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
     * Any thrown exception is converted to an Error result by generatePlugin().
     */
    using LlmHttpPostFn = std::function<std::string(
        const std::string& endpoint,
        const std::string& body
    )>;

    /**
     * @brief Inject an HTTP transport bridge for LLM endpoint calls.
     *
     * When set, `generatePlugin()` uses this callable if `Config::endpoint_invoke_fn`
     * is not configured, before falling back to the built-in CURL transport.
     * This is a convenience bridge for callers that want to provide a string-returning
     * HTTP POST implementation without using the Result-based Config transport hook.
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
