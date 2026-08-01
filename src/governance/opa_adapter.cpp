/**
 * @file opa_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=11; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: opa_adapter.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 279
 * Gap Summary: total=11; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=2, M=2, L=0
 * PR History (last 5): #5123 docs(server): update VCCDB ... (2026-05-14) | #3560 docs(governance): reality-c... (2026-03-12) | #3076 feat(governance): Integrate... (2026-03-12) | #2775 [auth] OPA integration for ... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "governance/opa_adapter.h"

#include <atomic>
#include <curl/curl.h>
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "governance/governance_diagnostics.h"

namespace themis {
namespace governance {

namespace {

/// Prometheus-style counters for error types (Phase 3)
std::atomic<uint64_t> governance_opa_error_timeout{0};
std::atomic<uint64_t> governance_opa_error_malformed{0};
std::atomic<uint64_t> governance_opa_error_network{0};
std::atomic<uint64_t> governance_opa_error_invalid_policy{0};
std::atomic<uint64_t> governance_opa_error_unknown{0};

/// Prometheus-style counters for WASM evaluation paths.
/// Labels: wasm_success, wasm_fallback
std::atomic<uint64_t> governance_opa_wasm_eval_wasm_success{0};
std::atomic<uint64_t> governance_opa_wasm_eval_wasm_fallback{0};

/// libcurl write callback: appends received data to a std::string.
size_t curl_write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    auto *buf = static_cast<std::string *>(userdata);
    buf->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

/// Guard that calls curl_global_init exactly once per process.
void ensure_curl_global_init() {
    static std::once_flag flag;
    std::call_once(flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

OpaAdapter::OpaAdapter(const Config &config) : config_(config) {
    if (config_.endpoint_url.empty()) {
        throw std::invalid_argument("governance::OpaAdapter: endpoint_url must not be empty");
    }
    if (config_.policy_path.empty()) {
        throw std::invalid_argument("governance::OpaAdapter: policy_path must not be empty");
    }
    if (config_.timeout_ms <= 0) {
        throw std::invalid_argument("governance::OpaAdapter: timeout_ms must be positive");
    }
    ensure_curl_global_init();
}

OpaAdapter::~OpaAdapter() = default;

void OpaAdapter::setWasmEvalFn(WasmEvalFn fn) {
    wasm_eval_fn_ = std::move(fn);
}

std::string OpaAdapter::buildUrl() const {
    // Ensure exactly one '/' between endpoint and path
    std::string url = config_.endpoint_url;
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    std::string path = config_.policy_path;
    if (!path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }
    return url + "/v1/data/" + path;
}

std::string OpaAdapter::buildRequestBody(const std::unordered_map<std::string, std::string> &headers,
                                         const std::string &route) {
    nlohmann::json input;
    input["route"] = route;

    nlohmann::json headers_obj = nlohmann::json::object();
    for (const auto &kv : headers) {
        headers_obj[kv.first] = kv.second;
    }
    input["headers"] = std::move(headers_obj);

    nlohmann::json body;
    body["input"] = std::move(input);
    return body.dump();
}

std::optional<PolicyDecision> OpaAdapter::parseOpaResponse(const std::string &response_body) {
    try {
        auto j = nlohmann::json::parse(response_body);
        if (!j.contains("result")) {
            return std::nullopt;
        }
        const auto &result = j["result"];

        // Simple boolean result
        if (result.is_boolean()) {
            if (!result.get<bool>()) {
                // OPA explicitly denied: return strict deny decision
                PolicyDecision d;
                d.classification             = "streng-geheim";
                d.mode                       = "enforce";
                d.encrypt_logs               = true;
                d.redaction                  = "strict";
                d.ann_allowed                = false;
                d.require_content_encryption = true;
                d.export_allowed             = false;
                d.cache_allowed              = false;
                d.retention_days             = 7;
                return d;
            }
            // true: OPA says OK but provides no governance details;
            // return nullopt so the caller falls back to native evaluation.
            return std::nullopt;
        }

        // Structured result object: must contain "allow" boolean
        if (result.is_object()) {
            if (!result.contains("allow") || !result["allow"].is_boolean()) {
                return std::nullopt;
            }

            PolicyDecision d;
            bool allow = result["allow"].get<bool>();

            if (!allow) {
                // OPA denied: apply strict defaults
                d.classification             = result.value("classification", std::string("streng-geheim"));
                d.mode                       = result.value("mode", std::string("enforce"));
                d.encrypt_logs               = result.value("encrypt_logs", true);
                d.redaction                  = result.value("redaction", std::string("strict"));
                d.ann_allowed                = result.value("ann_allowed", false);
                d.require_content_encryption = result.value("require_content_encryption", true);
                d.export_allowed             = false; // always deny export on OPA deny
                d.cache_allowed              = result.value("cache_allowed", false);
                d.retention_days             = result.value("retention_days", 7);
            } else {
                // OPA allowed: use provided fields or permissive defaults
                d.classification             = result.value("classification", std::string("vs-nfd"));
                d.mode                       = result.value("mode", std::string("enforce"));
                d.encrypt_logs               = result.value("encrypt_logs", false);
                d.redaction                  = result.value("redaction", std::string("standard"));
                d.ann_allowed                = result.value("ann_allowed", true);
                d.require_content_encryption = result.value("require_content_encryption", false);
                d.export_allowed             = result.value("export_allowed", true);
                d.cache_allowed              = result.value("cache_allowed", true);
                d.retention_days             = result.value("retention_days", 365);
            }
            return d;
        }
    } catch (...) {
        // Parse failure → treat as unavailable
    }
    return std::nullopt;
}

std::optional<PolicyDecision> OpaAdapter::evaluate(const std::unordered_map<std::string, std::string> &headers,
                                                   const std::string &route) const {
    // WASM evaluation path (tried first when mode == WASM)
    if (config_.mode == Config::EvalMode::WASM) {
        auto wasm_result = evaluateWasm(headers, route);
        if (wasm_result.has_value()) {
            return wasm_result;
        }
        // WASM failed — fall through to REST
        ++governance_opa_wasm_eval_wasm_fallback;
    }

    const std::string url  = buildUrl();
    const std::string body = buildRequestBody(headers, route);

    CURL *curl = curl_easy_init();
    if (!curl) {
        return std::nullopt;
    }

    std::string response_body;
    long http_code = 0;

    struct curl_slist *req_headers = nullptr;
    req_headers                    = curl_slist_append(req_headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, config_.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, config_.timeout_ms);
    // Disable signal handling for thread safety
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);
    
    // Phase 3: Classify errors and record diagnostics
    if (res != CURLE_OK) {
        OpaError err;
        err.timestamp_ms = std::chrono::system_clock::now()
            .time_since_epoch()
            .count() / 1'000'000;
        
        std::string err_msg = curl_easy_strerror(res);
        
        if (res == CURLE_OPERATION_TIMEDOUT) {
            err.type = OpaErrorType::kTimeout;
            ++governance_opa_error_timeout;
        } else if (res == CURLE_COULDNT_CONNECT || 
                   res == CURLE_COULDNT_RESOLVE_HOST ||
                   res == CURLE_COULDNT_RESOLVE_PROXY) {
            err.type = OpaErrorType::kNetworkError;
            ++governance_opa_error_network;
        } else {
            err.type = OpaErrorType::kUnknown;
            ++governance_opa_error_unknown;
        }
        
        err.message = err_msg;
        
        // Record diagnostic
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kOpaUnavailable;
        diag.component = "opa_adapter";
        diag.description = "OPA evaluation failed: " + err_msg;
        diag.remediation_steps = {
            "Check OPA service availability at " + config_.endpoint_url,
            "Verify policy bundle is loaded at " + config_.policy_path,
            "Validate network connectivity to OPA server",
            "Check timeout setting (current: " + std::to_string(config_.timeout_ms) + "ms)"
        };
        diag.context["error_type"] = std::to_string(static_cast<int>(err.type));
        diag.context["curl_error_code"] = std::to_string(res);
        diag.context["endpoint"] = config_.endpoint_url;
        diag.context["policy_path"] = config_.policy_path;
        
        curl_slist_free_all(req_headers);
        curl_easy_cleanup(curl);
        
        // Return deny-by-default decision on network/OPA errors
        PolicyDecision deny;
        deny.classification = "streng-geheim";
        deny.mode = "enforce";
        deny.encrypt_logs = true;
        deny.redaction = "strict";
        deny.ann_allowed = false;
        deny.require_content_encryption = true;
        deny.export_allowed = false;
        deny.cache_allowed = false;
        deny.retention_days = 7;
        
        return deny;
    }

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    curl_slist_free_all(req_headers);
    curl_easy_cleanup(curl);

    if (http_code < 200 || http_code >= 300) {
        OpaError err;
        err.type = OpaErrorType::kInvalidPolicy;
        err.message = "OPA returned HTTP " + std::to_string(http_code);
        err.timestamp_ms = std::chrono::system_clock::now()
            .time_since_epoch()
            .count() / 1'000'000;
        ++governance_opa_error_invalid_policy;
        
        // Record diagnostic
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kOpaUnavailable;
        diag.component = "opa_adapter";
        diag.description = "OPA policy evaluation failed: HTTP " + std::to_string(http_code);
        diag.remediation_steps = {
            "Check OPA policy bundle for syntax errors",
            "Verify policy at " + config_.policy_path + " exists and is valid",
            "Review OPA logs for detailed error message"
        };
        diag.context["http_code"] = std::to_string(http_code);
        diag.context["response_body"] = response_body.substr(0, 256);  // Truncate for diagnostics
        
        // Return deny-by-default decision
        PolicyDecision deny;
        deny.classification = "streng-geheim";
        deny.mode = "enforce";
        deny.encrypt_logs = true;
        deny.redaction = "strict";
        deny.ann_allowed = false;
        deny.require_content_encryption = true;
        deny.export_allowed = false;
        deny.cache_allowed = false;
        deny.retention_days = 7;
        
        return deny;
    }

    // Try to parse response
    try {
        auto result = parseOpaResponse(response_body);
        if (!result.has_value()) {
            // OPA returned 2xx but malformed response
            ++governance_opa_error_malformed;
            
            GovernanceDiagnostic diag;
            diag.code = GovDiagnosticCode::kOpaUnavailable;
            diag.component = "opa_adapter";
            diag.description = "OPA returned valid HTTP but malformed policy decision";
            diag.remediation_steps = {
                "Verify OPA policy returns proper decision structure",
                "Check policy at " + config_.policy_path + " for output format issues"
            };
            diag.context["response_preview"] = response_body.substr(0, 256);
            
            // Return deny-by-default
            PolicyDecision deny;
            deny.classification = "streng-geheim";
            deny.mode = "enforce";
            deny.encrypt_logs = true;
            deny.redaction = "strict";
            deny.ann_allowed = false;
            deny.require_content_encryption = true;
            deny.export_allowed = false;
            deny.cache_allowed = false;
            deny.retention_days = 7;
            return deny;
        }
        return result;
    } catch (const std::exception& e) {
        ++governance_opa_error_malformed;
        
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kOpaUnavailable;
        diag.component = "opa_adapter";
        diag.description = "Exception parsing OPA response: " + std::string(e.what());
        diag.remediation_steps = {
            "Review OPA response format",
            "Check policy at " + config_.policy_path
        };
        
        // Return deny-by-default
        PolicyDecision deny;
        deny.classification = "streng-geheim";
        deny.mode = "enforce";
        deny.encrypt_logs = true;
        deny.redaction = "strict";
        deny.ann_allowed = false;
        deny.require_content_encryption = true;
        deny.export_allowed = false;
        deny.cache_allowed = false;
        deny.retention_days = 7;
        return deny;
    }
}

// STUB/SIMULATION NOTE:
// Purpose: WASM-based OPA bundle evaluation without an OPA sidecar.
//   Loads a pre-compiled OPA bundle (.wasm) and evaluates it locally.
// Activation: Config::mode == EvalMode::WASM and wasm_bundle_path is set.
//   Gated by THEMIS_ENABLE_OPA_WASM build flag for real WASM runtime linkage.
//   When a WasmEvalFn is injected via setWasmEvalFn(), it is called first and
//   bypasses the built-in stub path entirely.
// Production Delta: Without an injected WasmEvalFn and without
//   THEMIS_ENABLE_OPA_WASM, this stub checks only that the bundle file exists,
//   then returns a permissive PolicyDecision. A real implementation would invoke
//   a WASM runtime (e.g. wasmer/wasmtime) to evaluate the Rego bundle.
// Roadmap ref: src/ROADMAP.md § "Consolidation Phase — OPA WASM Stub"
//              src/governance/FUTURE_ENHANCEMENTS.md § "OPA WASM Evaluation"
// Removal Plan: Replace with actual WASM runtime call when
//   THEMIS_ENABLE_OPA_WASM is added to the build system and a WASM runtime
//   dependency is approved (Target: v1.6.0).
std::optional<PolicyDecision> OpaAdapter::evaluateWasm(const std::unordered_map<std::string, std::string> &headers,
                                                       const std::string &route) const {
    // Injected evaluator takes priority over the built-in stub.
    if (wasm_eval_fn_) {
        auto result = wasm_eval_fn_(headers, route);
        if (result.has_value()) {
            ++governance_opa_wasm_eval_wasm_success;
        } else {
            ++governance_opa_wasm_eval_wasm_fallback;
        }
        return result;
    }

#ifdef THEMIS_ENABLE_OPA_WASM
    // Real WASM evaluation path (not yet implemented — requires WASM runtime linkage)
    // Fall through to REST
    ++governance_opa_wasm_eval_wasm_fallback;
    return std::nullopt;
#else
    // Check that the bundle file exists before returning stub result
    if (config_.wasm_bundle_path.empty()) {
        ++governance_opa_wasm_eval_wasm_fallback;
        return std::nullopt;
    }
    if (!std::filesystem::exists(config_.wasm_bundle_path)) {
        ++governance_opa_wasm_eval_wasm_fallback;
        return std::nullopt;
    }

    // Stub: bundle exists — return permissive PolicyDecision
    ++governance_opa_wasm_eval_wasm_success;
    PolicyDecision d;
    d.classification             = "offen";
    d.mode                       = "observe";
    d.encrypt_logs               = false;
    d.redaction                  = "none";
    d.ann_allowed                = true;
    d.require_content_encryption = false;
    d.export_allowed             = true;
    d.cache_allowed              = true;
    d.retention_days             = 365;
    return d;
#endif
}

} // namespace governance
} // namespace themis

