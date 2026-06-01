/*
 * ThemisDB | File: ai_plugin_generator.cpp | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 204
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=2, L=0
 * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #4827 refactor: flatten plugin/ h... (2026-05-04)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file ai_plugin_generator.cpp
 * @brief Production implementation of AIPluginGenerator.
 */

#include "ai/ai_plugin_generator.h"
#include "utils/error_registry.h"
#include "utils/expected.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <cmath>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <array>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace themis {
namespace plugins {
namespace ai {

namespace {

// Redaction policy: user-supplied and LLM-generated content is never logged
// verbatim.  Log helpers truncate strings to kLogMaxLen characters and append
// "[…]" when truncation occurs.  Error messages must not embed raw LLM output.
static constexpr std::size_t kLogMaxLen = 120u;

std::string truncateForLog(const std::string& s) {
    if (s.size() <= kLogMaxLen) {
        return s;
    }
    return s.substr(0, kLogMaxLen) + "[…]";
}

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

void ensureCurlGlobalInit() {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

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

AIPluginGenerator::AIPluginGenerator(const Config& config)
    : config_(config)
{}

AIPluginGenerator::~AIPluginGenerator() = default;

void AIPluginGenerator::setLlmHttpPostFn(LlmHttpPostFn fn) {
    llm_http_post_fn_ = std::move(fn);
}

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
    static constexpr std::size_t kMaxPromptListEntries = 64u;
    static constexpr std::size_t kMaxCapabilityTokenLen = 128u;
    static constexpr std::size_t kMaxDependencyTokenLen = 256u;

    if (prompt.description.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description must not be empty"));
    }
    if (prompt.description.size() > 8192u) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description exceeds 8192-character limit"));
    }

    if (prompt.required_capabilities.size() > kMaxPromptListEntries) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: required_capabilities exceeds maximum entry count"));
    }
    if (prompt.dependencies.size() > kMaxPromptListEntries) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: dependencies exceeds maximum entry count"));
    }

    std::unordered_set<std::string> unique_capabilities;
    unique_capabilities.reserve(prompt.required_capabilities.size());
    for (const auto& capability : prompt.required_capabilities) {
        if (capability.size() > kMaxCapabilityTokenLen || !isValidPromptListToken(capability)) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: required_capabilities contains invalid token"));
        }
        if (!unique_capabilities.insert(capability).second) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: required_capabilities contains duplicate token"));
        }
    }

    std::unordered_set<std::string> unique_dependencies;
    unique_dependencies.reserve(prompt.dependencies.size());
    for (const auto& dependency : prompt.dependencies) {
        if (dependency.size() > kMaxDependencyTokenLen || !isValidPromptListToken(dependency)) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: dependencies contains invalid token"));
        }
        if (!unique_dependencies.insert(dependency).second) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: dependencies contains duplicate token"));
        }
    }

    return {};  // success
}

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
        std::string out;
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
    if (request_body.size() > config_.max_request_body_bytes) {
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

    // Invoke endpoint with retry (up to 3 attempts, exponential backoff 100→400 ms).
    static constexpr int kMaxRetries = 3;
    Result<std::string> endpoint_result =
        tl::unexpected(Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                             "AIPluginGenerator: endpoint not attempted"));
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        endpoint_result = config_.endpoint_invoke_fn
            ? config_.endpoint_invoke_fn(config_.llm_endpoint, request_body, config_.timeout_ms)
            : invokeEndpointWithCurl(config_.llm_endpoint, request_body, config_.timeout_ms);
        if (endpoint_result) {
            break;
        }
        if (attempt + 1 < kMaxRetries) {
            spdlog::warn("[AIPluginGenerator] endpoint attempt {} failed: {}; retrying",
                         attempt + 1, endpoint_result.error().message());
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << attempt)));
        }
    }
    if (!endpoint_result) {
        if (endpoint_result.error().message().find("HTTP ") != std::string::npos) {
            ++stat_http_errors_;
        } else {
            ++stat_transport_errors_;
        }
        return tl::unexpected(endpoint_result.error());
    }
    if (endpoint_result->size() > config_.max_response_body_bytes) {
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

    const json& payload = (response.contains("generated_plugin") && response["generated_plugin"].is_object())
                        ? response["generated_plugin"]
                        : response;

    GeneratedPlugin generated;
    generated.header_code = payload.value("header_code", std::string{});
    generated.implementation_code = payload.value("implementation_code", std::string{});
    generated.test_code = payload.value("test_code", std::string{});
    generated.cmake_code = payload.value("cmake_code", std::string{});
    generated.security_report = payload.value("security_report", std::string{});
    generated.passed_security_checks = payload.value("passed_security_checks", false);

    // Validate LLM output: enforce reasonable size bounds and character constraints.
    static constexpr std::size_t kMaxCodeSize   = 1u << 20u;   // 1 MiB per code field
    static constexpr std::size_t kMaxReportSize = 64u << 10u;  // 64 KiB for security_report
    static constexpr std::size_t kMaxNameLen    = 256u;
    static constexpr std::size_t kMaxVersionLen = 64u;
    static constexpr std::size_t kMaxDescLen    = 8192u;
    static constexpr std::size_t kMaxDepEntryLen = 256u;
    if (generated.implementation_code.size() > kMaxCodeSize ||
        generated.header_code.size()         > kMaxCodeSize ||
        generated.test_code.size()           > kMaxCodeSize) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: LLM output exceeds maximum allowed code size"));
    }
    if (generated.cmake_code.size() > kMaxCodeSize) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: LLM cmake_code exceeds maximum allowed code size"));
    }
    if (generated.security_report.size() > kMaxReportSize) {
        ++stat_parse_errors_;
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: LLM security_report exceeds maximum allowed size"));
    }

    std::string raw_name = payload.value("name", std::string("generated_plugin"));
    if (raw_name.size() > kMaxNameLen || raw_name.empty()) {
        raw_name = "generated_plugin";
    }
    generated.manifest.name = std::move(raw_name);
    generated.manifest.version = payload.value("version", std::string("0.1.0"));
    if (generated.manifest.version.size() > kMaxVersionLen || generated.manifest.version.empty()) {
        generated.manifest.version = "0.1.0";
    }
    generated.manifest.description = payload.value("description", prompt.description);
    if (generated.manifest.description.size() > kMaxDescLen) {
        generated.manifest.description = generated.manifest.description.substr(0, kMaxDescLen);
    }
    generated.manifest.type = prompt.type;

    if (payload.contains("build_dependencies") && payload["build_dependencies"].is_array()) {
        const auto& deps_arr = payload["build_dependencies"];
        generated.build_dependencies.reserve(deps_arr.size());
        for (const auto& dep : deps_arr) {
            if (dep.is_string()) {
                auto dep_str = dep.get<std::string>();
                if (dep_str.size() <= kMaxDepEntryLen) {
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

    std::optional<double> c1_safety_score;
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
        if (!config_.sandbox_verify_fn) {
            ++stat_sandbox_rejections_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: sandbox gate enabled but sandbox_verify_fn is not configured"));
        }

        auto sandbox_result = config_.sandbox_verify_fn(generated);
        if (!sandbox_result) {
            ++stat_sandbox_rejections_;
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: sandbox verification failed: " +
                          sandbox_result.error().message()));
        }
        if (!generated.security_report.empty()) {
            generated.security_report += "\n";
        }
        generated.security_report += "Sandbox verification: pass";
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
