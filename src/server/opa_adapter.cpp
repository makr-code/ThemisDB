/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            opa_adapter.cpp                                    ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:10:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     165                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/opa_adapter.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <mutex>

namespace themis {

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
        throw std::invalid_argument("OpaAdapter: endpoint_url must not be empty");
    }
    if (config_.policy_path.empty()) {
        throw std::invalid_argument("OpaAdapter: policy_path must not be empty");
    }
    if (config_.timeout_ms <= 0) {
        throw std::invalid_argument("OpaAdapter: timeout_ms must be positive");
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
    const std::string& user_id,
    const std::string& action,
    const std::string& resource_path,
    const std::optional<std::string>& client_ip,
    const std::optional<std::string>& user_agent)
{
    nlohmann::json input;
    input["user"]     = user_id;
    input["action"]   = action;
    input["resource"] = resource_path;
    if (client_ip)  input["client_ip"]  = *client_ip;
    if (user_agent) input["user_agent"] = *user_agent;

    nlohmann::json body;
    body["input"] = std::move(input);
    return body.dump();
}

std::optional<bool> OpaAdapter::parseOpaResponse(const std::string& response_body) {
    try {
        auto j = nlohmann::json::parse(response_body);
        if (!j.contains("result")) return std::nullopt;
        const auto& result = j["result"];
        if (result.is_boolean()) return result.get<bool>();
        // A non-null, non-empty OPA result object means the rule evaluated to
        // a defined value (e.g. a partial rule or set comprehension that is
        // non-empty).  Treat this as "allow" consistent with OPA's convention
        // where undefined / empty results indicate denial.
        if (result.is_object() && !result.empty()) return true;
    } catch (...) {
        // Parse failure → treat as unavailable
    }
    return std::nullopt;
}

std::optional<PolicyEngine::Decision> OpaAdapter::evaluate(
    const std::string& user_id,
    const std::string& action,
    const std::string& resource_path,
    const std::optional<std::string>& client_ip,
    const std::optional<std::string>& user_agent) const
{
    const std::string url  = buildUrl();
    const std::string body = buildRequestBody(
        user_id, action, resource_path, client_ip, user_agent);

    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    std::string response_body;
    long http_code = 0;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST,           1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,     config_.timeout_ms);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, config_.timeout_ms);
    // Disable signal handling for thread safety
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code < 200 || http_code >= 300) {
        return std::nullopt;
    }

    auto allowed = parseOpaResponse(response_body);
    if (!allowed.has_value()) return std::nullopt;

    PolicyEngine::Decision d;
    d.allowed   = *allowed;
    d.policy_id = "opa:" + config_.policy_path;
    d.reason    = *allowed ? "opa_allow" : "opa_deny";
    return d;
}

} // namespace themis
