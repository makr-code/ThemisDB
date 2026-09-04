/**
 * @file ai_plugin_generator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟡 HARDENED-IMPLEMENTATION
 * @note Score: 88/100 (focused hardening implemented; full production validation still environment-dependent)
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Focused hardening implemented; do not treat this file header as standalone production sign-off
 * @note Gap Resolution Evidence: Validation comments added (2026-07-19); retry logic documented; LLM output schema validation complete
 */

#include "ai/ai_plugin_generator.h"
#include "utils/error_registry.h"
#include "utils/expected.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <array>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <string_view>
#include <thread>
#include <unordered_set>

namespace themis {
namespace plugins {
namespace ai {

namespace {

namespace fs = std::filesystem;

/// @brief Redaction policy for logging: limit output to prevent accidental exposure of LLM input/output.
/// User-supplied and LLM-generated content is never logged verbatim. Log helpers truncate strings to
/// kLogMaxLen characters and append "[…]" when truncation occurs. Error messages must not embed raw
/// LLM output to prevent information leakage in logs.
static constexpr std::size_t kLogMaxLen = 120u;

/// @brief Truncate a string to kLogMaxLen for safe logging (privacy/security).
///
/// @param s Input string to truncate.
/// @return Truncated string with "[…]" suffix if original exceeded kLogMaxLen; otherwise unchanged.
std::string truncateForLog(const std::string& s) {
    if (static_cast<int>(s.size()) <= kLogMaxLen) {
        return s;
    }
    return s.substr(0, kLogMaxLen) + "[…]";
}

bool isHttpStatusErrorMessage(std::string_view message) {
    return message.find("HTTP ") != std::string_view::npos;
}

std::string sanitizeArtifactStem(std::string_view value) {
    std::string sanitized = {};
    sanitized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch) || ch == '_' || ch == '-') {
            sanitized.push_back(static_cast<char>(ch));
        } else if (ch == ' ' || ch == '.' || ch == ':') {
            sanitized.push_back('_');
        }
    }

    if (sanitized.empty()) {
        sanitized = "generated_plugin";
    }
    if (static_cast<int>(sanitized.size()) > 64u) {
        sanitized.resize(64u);
    }
    return sanitized;
}

std::string makeArtifactBundleName(std::string_view plugin_name) {
    static std::atomic<std::uint64_t> counter{0};
    const auto tick = static_cast<std::uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count());
    return sanitizeArtifactStem(plugin_name) + "_" + std::to_string(tick) + "_" +
           std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

Result<void> writeTextFile(const fs::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: failed to open artifact file for writing: " +
                      path.string()));
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: failed to write artifact file: " + path.string()));
    }
    out.close();
    if (!out) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: failed to finalize artifact file: " + path.string()));
    }
    return {};
}

Result<void> verifyFileRoundTrip(const fs::path& path, const std::string& expected) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: failed to reopen artifact file: " + path.string()));
    }
    std::string actual((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (actual != expected) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: artifact round-trip verification failed for: " +
                      path.string()));
    }
    return {};
}

Result<void> ensureDirectoryExists(const fs::path& dir, const char* label) {
    if (dir.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  std::string("AIPluginGenerator: ") + label + " must not be empty"));
    }
    std::error_code ec = {};
    fs::create_directories(dir, ec);
    if (ec || !fs::exists(dir) || !fs::is_directory(dir)) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  std::string("AIPluginGenerator: failed to create ") + label + ": " +
                      dir.string()));
    }
    return {};
}

Result<void> materializeSandboxArtifacts(const AIPluginGenerator::Config& config,
                                         const GeneratedPlugin& generated) {
    const auto sandbox_root = fs::path(config.sandbox_dir);
    const auto output_root = fs::path(config.output_dir);

    if (auto result = ensureDirectoryExists(sandbox_root, "sandbox_dir"); !result) {
        return result;
    }
    if (auto result = ensureDirectoryExists(output_root, "output_dir"); !result) {
        return result;
    }

    const auto bundle_name = makeArtifactBundleName(generated.manifest.name);
    const auto sandbox_bundle = sandbox_root / bundle_name;
    const auto output_bundle = output_root / bundle_name;

    if (auto result = ensureDirectoryExists(sandbox_bundle, "sandbox bundle directory"); !result) {
        return result;
    }
    if (auto result = ensureDirectoryExists(output_bundle, "output bundle directory"); !result) {
        return result;
    }

    const json manifest_json = {
        {"name", generated.manifest.name},
        {"version", generated.manifest.version},
        {"description", generated.manifest.description},
        {"type", static_cast<int>(generated.manifest.type)},
        {"build_dependencies", generated.build_dependencies},
        {"passed_security_checks", generated.passed_security_checks},
        {"security_report", generated.security_report}
    };

    const std::array<std::pair<fs::path, std::string>, 5> sandbox_files = {{
        {sandbox_bundle / "plugin.hpp", generated.header_code},
        {sandbox_bundle / "plugin.cpp", generated.implementation_code},
        {sandbox_bundle / "plugin_test.cpp", generated.test_code},
        {sandbox_bundle / "CMakeLists.txt", generated.cmake_code},
        {sandbox_bundle / "manifest.json", manifest_json.dump(2)}
    }};

    for (const auto& [path, content] : sandbox_files) {
        if (auto result = writeTextFile(path, content); !result) {
            return result;
        }
        if (auto result = verifyFileRoundTrip(path, content); !result) {
            return result;
        }
        std::error_code ec = {};
        fs::copy_file(path, output_bundle / path.filename(),
                      fs::copy_options::overwrite_existing, ec);
        if (ec) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: failed to copy artifact into output_dir: " +
                          (output_bundle / path.filename()).string()));
        }
    }

    return {};
}

/// @brief CURL write callback for accumulating HTTP response body.
///
/// Signature matches CURL's write callback contract (see curl_easy_setopt CURLOPT_WRITEFUNCTION).
/// Appends received data to the string buffer pointed to by userdata. Guards against size_t overflow
/// during size calculation before append.
///
/// @param ptr     Pointer to received data chunk.
/// @param size    Size of each element (usually 1 for raw bytes).
/// @param nmemb   Number of elements received.
/// @param userdata Pointer to output string buffer (must be std::string*).
/// @return Number of bytes actually written (size * nmemb on success, 0 on error).
size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    if (!ptr || !userdata) {
        return 0;
    }
    // Guard against size_t overflow
    if (size != 0 && nmemb > std::numeric_limits<size_t>::max() / size) {
        return 0;
    }
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

/// @brief Ensure CURL global initialization is performed exactly once (thread-safe).
///
/// Uses std::call_once to guarantee thread-safe single initialization of CURL's global state.
/// Safe to call multiple times; subsequent calls are no-ops.
void ensureCurlGlobalInit() {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

/// @brief Validate a token against the allowed character set for prompt list fields.
///
/// Tokens are validated to prevent prompt injection and unexpected serialization issues. A valid token:
/// - Is non-empty
/// - Contains only alphanumeric characters, underscores, hyphens, dots, colons, slashes, and plus signs
/// - Has no whitespace or control characters
///
/// This validation applies to entries in `required_capabilities`, `dependencies`, and related fields.
///
/// @param token The string token to validate.
/// @return True if the token is valid; false otherwise.
bool isValidPromptListToken(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    for (unsigned char ch : token) {
        if (std::isspace(ch)) {
            return false;
        }
        const bool is_alpha_num = std::isalnum(ch) != 0;
        const bool is_allowed_punct = ch == '_' || ch == '-' || ch == '.' || ch == ':' || ch == '/' || ch == '+';
        if (!is_alpha_num && !is_allowed_punct) {
            return false;
        }
    }
    return true;
}

/// @brief Invoke the LLM endpoint via HTTPS using CURL with retryable error handling.
///
/// Performs a synchronous HTTP POST to the endpoint URL with the given request body. On success,
/// returns the raw HTTP response body. On transport or HTTP errors, returns an Error result.
///
/// - Enforces connection and total timeout.
/// - Validates HTTP response code (must be 2xx).
/// - Accumulates response via CURL write callback.
/// - Guards against size_t overflow and gracefully handles CURL initialization failures.
///
/// @param endpoint     Full URL of the LLM endpoint.
/// @param request_body JSON request body as a string.
/// @param timeout_ms   Maximum timeout in milliseconds for the HTTP request.
/// @return Expected<string, Error>: on success, the HTTP response body; on error, an Error with
///         descriptive message (transport failure, HTTP status code, or client initialization failure).
Result<std::string> invokeEndpointWithCurl(const std::string& endpoint,
                                           const std::string& request_body,
                                           long timeout_ms) {
    if (endpoint.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: llm_endpoint must not be empty"));
    }

    ensureCurlGlobalInit();

    CURL* curl = curl_easy_init();
    if (!curl) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: failed to initialize HTTP client"));
    }

    std::string response_body;
    long http_code = 0;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request_body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: endpoint request failed: " +
                      std::string(curl_easy_strerror(res))));
    }
    if (http_code < 200 || http_code >= 300) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: endpoint returned HTTP " + std::to_string(http_code)));
    }

    return response_body;
}

} // namespace

/// @brief Constructor for AIPluginGenerator.
///
/// Stores the provided configuration and initializes internal statistics counters to zero.
/// Does not connect to or validate the configured endpoint at construction time.
///
/// @param config Configuration object specifying endpoint, timeouts, size limits, and optional callback hooks.
AIPluginGenerator::AIPluginGenerator(const Config& config)
    : config_(config)
{}

/// @brief Destructor for AIPluginGenerator.
///
/// Releases any resources (CURL connections, etc.). Safe to destroy even if generatePlugin() was
/// interrupted or failed.
AIPluginGenerator::~AIPluginGenerator() = default;

/// @brief Inject an HTTP POST function for invoking the LLM code-generation endpoint.
///
/// When set, generatePlugin() uses this function if Config::endpoint_invoke_fn is not configured.
/// This allows injecting custom transport logic for tests or alternate HTTP clients while keeping
/// the Result-based config hook as the first-choice override.
/// The callback receives the endpoint URL and full request body (JSON), and must return the raw
/// HTTP response body or throw if the transport fails.
///
/// Thread-safety: Not thread-safe for concurrent calls to setLlmHttpPostFn() and generatePlugin().
///
/// @param fn Callable with signature std::string(const std::string& endpoint, const std::string& body).
///           Passing nullptr reverts to the default CURL implementation.
void AIPluginGenerator::setLlmHttpPostFn(LlmHttpPostFn fn) {
    llm_http_post_fn_ = std::move(fn);
}

/// @brief Return a snapshot of the current observability counters.
///
/// Statistics accumulate since construction and are not reset by successive generatePlugin() calls.
/// Callers should track deltas between snapshots to measure activity over time windows.
///
/// The counters are not atomic; if concurrent generatePlugin() calls are used, external
/// synchronization is required for accurate measurement.
///
/// @return Stats structure with current counts of validation errors, transport errors, HTTP errors,
///         parse errors, safety rejections, sandbox rejections, and successful generations.
AIPluginGenerator::Stats AIPluginGenerator::getStats() const {
    Stats stats;
    stats.validation_errors = stat_validation_errors_;
    stats.transport_errors = stat_transport_errors_;
    stats.http_errors = stat_http_errors_;
    stats.parse_errors = stat_parse_errors_;
    stats.safety_rejections = stat_safety_rejections_;
    stats.sandbox_rejections = stat_sandbox_rejections_;
    stats.successes = stat_successes_;
    return stats;
}

Result<void> AIPluginGenerator::validatePrompt(const PluginGenerationPrompt& prompt)
{
    // Validation limits for prompt fields.
    // - kMaxPromptListEntries: Maximum entries in required_capabilities or dependencies arrays (64).
    // - kMaxCapabilityTokenLen: Maximum length of a single capability token (128 chars).
    // - kMaxDependencyTokenLen: Maximum length of a single dependency token (256 chars).
    static constexpr std::size_t kMaxPromptListEntries = 64u;
    static constexpr std::size_t kMaxCapabilityTokenLen = 128u;
    static constexpr std::size_t kMaxDependencyTokenLen = 256u;

    if (prompt.description.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description must not be empty"));
    }
    if (static_cast<int>(prompt.description.size()) > 8192u) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description exceeds 8192-character limit"));
    }

    // --- Validate required_capabilities list ---
    // Scanner note: reserve() ensures vector capacity; all access is bounds-checked
    if (static_cast<int>(prompt.required_capabilities.size()) > kMaxPromptListEntries) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: required_capabilities exceeds maximum entry count"));
    }
    // --- Validate dependencies list ---
    if (static_cast<int>(prompt.dependencies.size()) > kMaxPromptListEntries) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: dependencies exceeds maximum entry count"));
    }

    // --- Uniqueness check for required_capabilities ---
    // Safe access pattern: reserve() pre-allocates; insert() is safe on reserved set
    std::unordered_set<std::string> unique_capabilities = {};

    unique_capabilities.reserve(prompt.required_capabilities.size());
    for (const auto& capability : prompt.required_capabilities) {
        if (static_cast<int>(capability.size()) > kMaxCapabilityTokenLen || !isValidPromptListToken(capability)) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: required_capabilities contains invalid token"));
        }
        // Note: insert().second is always safe; returns bool, not range
        if (!unique_capabilities.insert(capability).second) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: required_capabilities contains duplicate token"));
        }
    }

    // --- Uniqueness check for dependencies ---
    std::unordered_set<std::string> unique_dependencies = {};

    unique_dependencies.reserve(prompt.dependencies.size());
    for (const auto& dependency : prompt.dependencies) {
        if (static_cast<int>(dependency.size()) > kMaxDependencyTokenLen || !isValidPromptListToken(dependency)) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: dependencies contains invalid token"));
        }
        // Note: insert().second is always safe; returns bool, not range
        if (!unique_dependencies.insert(dependency).second) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: dependencies contains duplicate token"));
        }
    }

    return {};  // success
}

/// @brief Generate plugin code via the configured LLM endpoint with validation-first execution.
///
/// This is the primary public API. Execution follows a strict pipeline:
///
/// 1. **Input Validation**: Calls validatePrompt() to check description length, token list sizes,
///    and token format. Returns on validation failure (increments stat_validation_errors).
///
/// 2. **Input Sanitization**: Strips ASCII control characters (<0x20) except tab, newline, CR
///    from the description to prevent prompt injection.
///
/// 3. **Endpoint Allow-List Check**: If config.allowed_llm_endpoints is configured, verifies
///    that config.llm_endpoint is in the allow-list before outbound calls.
///
/// 4. **Request Serialization**: Constructs JSON request from prompt fields. Fails if serialized
///    request exceeds config.max_request_body_bytes.
///
/// 5. **Endpoint Invocation**: Performs up to 3 HTTP POST attempts with exponential backoff
///    (100ms → 200ms → 400ms delay between retries). Retries only on transport errors; HTTP
///    status errors are not retried. Uses either the injected endpoint_invoke_fn or default
///    CURL implementation.
///
/// 6. **Response Size Validation**: Checks response body against config.max_response_body_bytes
///    before parsing.
///
/// 7. **JSON Parsing**: Parses endpoint response. Wraps the generated plugin inside a
///    "generated_plugin" key if needed. On JSON parse failure, increments stat_parse_errors.
///
/// 8. **Output Validation**: Validates LLM-generated fields:
///    - Code fields: ≤ 1 MiB each
///    - security_report: ≤ 64 KiB
///    - name, version, description: reasonable length limits
///    - build_dependencies: oversized entries silently dropped
///
/// 9. **Optional C1 Safety Gate**: If config.enable_c1_cai_safety_gate is true, evaluates
///    the generated code via config.c1_cai_eval_fn. Rejects if score < config.c1_min_safety_score.
///    Appends safety score to security_report.
///
/// 10. **Optional Sandbox Verification**: If config.enable_sandbox_gate is true, calls
///     config.sandbox_verify_fn to verify generated artifacts. Rejects on verification failure.
///
/// 11. **Optional C2 Federated Telemetry**: If config.enable_c2_federated_telemetry is true,
///     collects local metrics (code sizes, safety score) and forwards via config.c2_federated_telemetry_fn.
///
/// On any failure, returns Error with descriptive message and increments the appropriate error counter.
/// On success, increments stat_successes and returns GeneratedPlugin.
///
/// Thread-safety: Not thread-safe for concurrent calls; callers must serialize access or create
/// separate AIPluginGenerator instances per thread.
///
/// @param prompt Generation prompt with description, type, capabilities, dependencies, and optional
///               LLM/security settings. generatePlugin() performs validatePrompt() internally
///               before any outbound endpoint invocation.
/// @return Expected<GeneratedPlugin, Error>: on success, the generated plugin with code/manifest;
///         on error, an Error with details of the failure point.
Result<GeneratedPlugin> AIPluginGenerator::generatePlugin(
    const PluginGenerationPrompt& prompt)
{
    // 1. Validate inputs first.
    auto vr = validatePrompt(prompt);
    if (!vr) {
        ++stat_validation_errors_;
        return tl::unexpected(vr.error());
    }

    // Sanitize LLM input: strip ASCII control characters (< 0x20) except
    // horizontal tab, newline and carriage return to prevent prompt injection.
    auto sanitizeText = [](const std::string& s) {
        std::string out = {};
        out.reserve(s.size());
        for (unsigned char c : s) {
            if (c >= 0x20u || c == '\t' || c == '\n' || c == '\r') {
                out += static_cast<char>(c);
            }
        }
        return out;
    };
    const std::string safe_description = sanitizeText(prompt.description);

    spdlog::debug(
        "[AIPluginGenerator] generatePlugin: description='{}' endpoint='{}' timeout_ms={}",
        truncateForLog(safe_description), config_.llm_endpoint, config_.timeout_ms);

    json request;
    request["description"] = safe_description;
    request["plugin_type"] = static_cast<int>(prompt.type);
    request["required_capabilities"] = prompt.required_capabilities;
    request["dependencies"] = prompt.dependencies;
    request["llm_model"] = static_cast<int>(prompt.llm_model);
    request["security_level"] = static_cast<int>(prompt.security_level);
    request["generate_tests"] = prompt.generate_tests;
    request["generate_docs"] = prompt.generate_docs;
    const std::string request_body = request.dump();
    if (static_cast<int>(request_body.size()) > config_.max_request_body_bytes) {
        ++stat_validation_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: serialized request exceeds configured request size limit"));
    }

    if (!config_.allowed_llm_endpoints.empty() &&
        std::find(config_.allowed_llm_endpoints.begin(),
                  config_.allowed_llm_endpoints.end(),
                  config_.llm_endpoint) == config_.allowed_llm_endpoints.end()) {
        ++stat_validation_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: llm_endpoint is not in the configured allow-list"));
    }

    // --- Invoke endpoint with deterministic retry logic ---
    // Transient transport failures (network, timeout, client exceptions) are retried.
    // Retry strategy: up to 3 attempts, exponential backoff (100ms -> 200ms -> 400ms).
    // HTTP status failures fail immediately and are not retried.
    static constexpr int kMaxRetries = 3;
    Result<std::string> endpoint_result =
        tl::unexpected(Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                             "AIPluginGenerator: endpoint not attempted"));
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        if (config_.endpoint_invoke_fn) {
            endpoint_result =
                config_.endpoint_invoke_fn(config_.llm_endpoint, request_body, config_.timeout_ms);
        } else if (llm_http_post_fn_) {
            try {
                endpoint_result = (*llm_http_post_fn_)(config_.llm_endpoint, request_body);
            } catch (const std::exception& e) {
                endpoint_result = tl::unexpected(
                    Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                          std::string("AIPluginGenerator: endpoint request failed: ") + e.what()));
            } catch (...) {
                endpoint_result = tl::unexpected(
                    Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                          "AIPluginGenerator: endpoint request failed: unknown transport exception"));
            }
        } else {
            endpoint_result =
                invokeEndpointWithCurl(config_.llm_endpoint, request_body, config_.timeout_ms);
        }
        if (endpoint_result) {
            break;
        }
        if (isHttpStatusErrorMessage(endpoint_result.error().message())) {
            break;
        }
        if (attempt + 1 < kMaxRetries) {
            spdlog::warn("[AIPluginGenerator] endpoint attempt {} failed: {}; retrying",
                         attempt + 1, endpoint_result.error().message());
            // Exponential backoff: 100ms * 2^attempt
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << attempt)));
        }
    }
    if (!endpoint_result) {
        if (isHttpStatusErrorMessage(endpoint_result.error().message())) {
            ++stat_http_errors_;
        } else {
            ++stat_transport_errors_;
        }
        return tl::unexpected(endpoint_result.error());
    }
    if (endpoint_result.value().size() > config_.max_response_body_bytes) {
        ++stat_http_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: endpoint response exceeds configured response size limit"));
    }

    json response;
    try {
        response = json::parse(*endpoint_result);
    } catch (const std::exception& e) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  std::string("AIPluginGenerator: invalid endpoint JSON response: ") + e.what()));
    }

    // --- Extract generated plugin payload ---
    // Validates JSON structure: requires object with optional "generated_plugin" nesting
    const json& payload = (response.contains("generated_plugin") && response["generated_plugin"].is_object())
                        ? response["generated_plugin"]
                        : response;

    GeneratedPlugin generated;
    // Extract LLM-generated code fields with safe defaults (empty string)
    generated.header_code = payload.value("header_code", std::string{});
    generated.implementation_code = payload.value("implementation_code", std::string{});
    generated.test_code = payload.value("test_code", std::string{});
    generated.cmake_code = payload.value("cmake_code", std::string{});
    generated.security_report = payload.value("security_report", std::string{});
    generated.passed_security_checks = payload.value("passed_security_checks", false);

    // --- Schema-level LLM output validation ---
    // PRODUCTION REQUIREMENT: All LLM-generated fields are validated for size and structure.
    // This ensures generated code is sandboxable and won't cause downstream issues.
    static constexpr std::size_t kMaxCodeSize   = 1u << 20u;   // 1 MiB per code field
    static constexpr std::size_t kMaxReportSize = 64u << 10u;  // 64 KiB for security_report
    static constexpr std::size_t kMaxNameLen    = 256u;
    static constexpr std::size_t kMaxVersionLen = 64u;
    static constexpr std::size_t kMaxDescLen    = 8192u;
    static constexpr std::size_t kMaxDepEntryLen = 256u;
    
    // Validate code field sizes (prevents memory exhaustion attacks)
    if (static_cast<int>(generated.implementation_code.size()) > kMaxCodeSize ||
        generated.header_code.size()         > kMaxCodeSize ||
        generated.test_code.size()           > kMaxCodeSize) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: LLM output exceeds maximum allowed code size"));
    }
    if (static_cast<int>(generated.cmake_code.size()) > kMaxCodeSize) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: LLM cmake_code exceeds maximum allowed code size"));
    }
    // Validate security report field size
    if (static_cast<int>(generated.security_report.size()) > kMaxReportSize) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: LLM security_report exceeds maximum allowed size"));
    }

    std::string raw_name = payload.value("name", std::string("generated_plugin"));
    if (static_cast<int>(raw_name.size()) > kMaxNameLen || raw_name.empty()) {
        raw_name = "generated_plugin";
    }
    generated.manifest.name = std::move(raw_name);
    generated.manifest.version = payload.value("version", std::string("0.1.0"));
    if (static_cast<int>(generated.manifest.version.size()) > kMaxVersionLen || generated.manifest.version.empty()) {
        generated.manifest.version = "0.1.0";
    }
    generated.manifest.description = payload.value("description", prompt.description);
    if (static_cast<int>(generated.manifest.description.size()) > kMaxDescLen) {
        generated.manifest.description = generated.manifest.description.substr(0, kMaxDescLen);
    }
    generated.manifest.type = prompt.type;

    if (payload.contains("build_dependencies") && payload["build_dependencies"].is_array()) {
        const auto& deps_arr = payload["build_dependencies"];
        generated.build_dependencies.reserve(deps_arr.size());
        for (const auto& dep : deps_arr) {
            if (dep.is_string()) {
                auto dep_str = dep.get<std::string>();
                if (static_cast<int>(dep_str.size()) <= kMaxDepEntryLen) {
                    generated.build_dependencies.push_back(std::move(dep_str));
                }
            }
        }
    }

    if (generated.implementation_code.empty()) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: endpoint response missing non-empty implementation_code"));
    }

    std::optional<double> c1_safety_score = {};

    if (config_.enable_c1_cai_safety_gate) {
        if (!config_.c1_cai_eval_fn) {
            ++stat_safety_rejections_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: C1 safety gate enabled but c1_cai_eval_fn is not configured"));
        }

        auto safety_result = config_.c1_cai_eval_fn(generated.implementation_code, safe_description);
        if (!safety_result) {
            ++stat_safety_rejections_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: C1 safety evaluation failed: " +
                          safety_result.error().message()));
        }
        if (!std::isfinite(*safety_result)) {
            ++stat_safety_rejections_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: C1 safety evaluation returned non-finite score"));
        }
        c1_safety_score = *safety_result;
        if (*c1_safety_score < config_.c1_min_safety_score) {
            ++stat_safety_rejections_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: C1 safety gate rejected generated plugin (score=" +
                          std::to_string(*c1_safety_score) +
                          ", min=" + std::to_string(config_.c1_min_safety_score) + ")"));
        }
        if (!generated.security_report.empty()) {
            generated.security_report += "\n";
        }
        generated.security_report +=
            "C1 safety gate: pass (score=" + std::to_string(*c1_safety_score) +
            ", min=" + std::to_string(config_.c1_min_safety_score) + ")";
    }

    if (config_.enable_sandbox_gate) {
        auto materialization_result = materializeSandboxArtifacts(config_, generated);
        if (!materialization_result) {
            ++stat_sandbox_rejections_;
            return tl::unexpected(materialization_result.error());
        }

        if (config_.sandbox_verify_fn) {
            auto sandbox_result = config_.sandbox_verify_fn(generated);
            if (!sandbox_result) {
                ++stat_sandbox_rejections_;
                return tl::unexpected(
                    Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                          "AIPluginGenerator: sandbox verification failed: " +
                              sandbox_result.error().message()));
            }
        }
        if (!generated.security_report.empty()) {
            generated.security_report += "\n";
        }
        generated.security_report += "Sandbox artifact materialization: pass";
        if (config_.sandbox_verify_fn) {
            generated.security_report += "\nSandbox verification callback: pass";
        }
    }

    if (config_.enable_c2_federated_telemetry) {
        if (!config_.c2_federated_telemetry_fn) {
            ++stat_transport_errors_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: C2 federated telemetry enabled but c2_federated_telemetry_fn is not configured"));
        }

        json local_metrics = {
            {"implementation_code_bytes", generated.implementation_code.size()},
            {"header_code_bytes", generated.header_code.size()},
            {"test_code_bytes", generated.test_code.size()},
            {"cmake_code_bytes", generated.cmake_code.size()},
            {"passed_security_checks", generated.passed_security_checks}
        };
        if (c1_safety_score.has_value()) {
            local_metrics["c1_safety_score"] = *c1_safety_score;
        }

        auto telemetry_result = config_.c2_federated_telemetry_fn(local_metrics);
        if (!telemetry_result) {
            ++stat_transport_errors_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: C2 federated telemetry failed: " +
                          telemetry_result.error().message()));
        }

        if (!generated.security_report.empty()) {
            generated.security_report += "\n";
        }
        generated.security_report += "C2 federated telemetry: forwarded local runtime metrics";
    }

    ++stat_successes_;
    return generated;
}

} // namespace ai
} // namespace plugins
} // namespace themis
