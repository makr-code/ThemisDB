/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            opa_adapter.cpp                                    ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:47:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     206                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 99dc8e3f41  2026-02-27  feat(governance): integrate OPA as alternative policy eva... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/opa_adapter.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <stdexcept>

namespace themis {
namespace governance {

namespace {

/// libcurl write callback: appends received data to a std::string.
size_t curl_write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

/// Guard that calls curl_global_init exactly once per process.
void ensure_curl_global_init() {
    static std::once_flag flag;
    std::call_once(flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

OpaAdapter::OpaAdapter(const Config& config) : config_(config) {
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

std::string OpaAdapter::buildUrl() const {
    // Ensure exactly one '/' between endpoint and path
    std::string url = config_.endpoint_url;
    if (!url.empty() && url.back() == '/') url.pop_back();
    std::string path = config_.policy_path;
    if (!path.empty() && path.front() == '/') path.erase(path.begin());
    return url + "/v1/data/" + path;
}

std::string OpaAdapter::buildRequestBody(
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& route)
{
    nlohmann::json input;
    input["route"] = route;

    nlohmann::json headers_obj = nlohmann::json::object();
    for (const auto& kv : headers) {
        headers_obj[kv.first] = kv.second;
    }
    input["headers"] = std::move(headers_obj);

    nlohmann::json body;
    body["input"] = std::move(input);
    return body.dump();
}

std::optional<PolicyDecision> OpaAdapter::parseOpaResponse(const std::string& response_body) {
    try {
        auto j = nlohmann::json::parse(response_body);
        if (!j.contains("result")) return std::nullopt;
        const auto& result = j["result"];

        // Simple boolean result
        if (result.is_boolean()) {
            if (!result.get<bool>()) {
                // OPA explicitly denied: return strict deny decision
                PolicyDecision d;
                d.classification          = "streng-geheim";
                d.mode                    = "enforce";
                d.encrypt_logs            = true;
                d.redaction               = "strict";
                d.ann_allowed             = false;
                d.require_content_encryption = true;
                d.export_allowed          = false;
                d.cache_allowed           = false;
                d.retention_days          = 7;
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

std::optional<PolicyDecision> OpaAdapter::evaluate(
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& route) const
{
    const std::string url  = buildUrl();
    const std::string body = buildRequestBody(headers, route);

    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    std::string response_body;
    long http_code = 0;

    struct curl_slist* req_headers = nullptr;
    req_headers = curl_slist_append(req_headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,              url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST,              1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,        body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,     static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,        req_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,     curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,         &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,        config_.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, config_.timeout_ms);
    // Disable signal handling for thread safety
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,          1L);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    curl_slist_free_all(req_headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code < 200 || http_code >= 300) {
        return std::nullopt;
    }

    return parseOpaResponse(response_body);
}

} // namespace governance
} // namespace themis
