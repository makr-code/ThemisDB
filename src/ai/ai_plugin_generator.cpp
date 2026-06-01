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
#include <mutex>
#include <stdexcept>
#include <string>
#include <array>

namespace themis {
namespace plugins {
namespace ai {

namespace {

size_t curlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

void ensureCurlGlobalInit() {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
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

Result<void> AIPluginGenerator::validatePrompt(const PluginGenerationPrompt& prompt)
{
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
    return {};  // success
}

Result<GeneratedPlugin> AIPluginGenerator::generatePlugin(
    const PluginGenerationPrompt& prompt)
{
    // 1. Validate inputs first.
    auto vr = validatePrompt(prompt);
    if (!vr) {
        return tl::unexpected(vr.error());
    }

    spdlog::debug(
        "[AIPluginGenerator] generatePlugin: description='{}' endpoint='{}' timeout_ms={}",
        prompt.description.substr(0, 80), config_.llm_endpoint, config_.timeout_ms);

    json request;
    request["description"] = prompt.description;
    request["plugin_type"] = static_cast<int>(prompt.type);
    request["required_capabilities"] = prompt.required_capabilities;
    request["dependencies"] = prompt.dependencies;
    request["llm_model"] = static_cast<int>(prompt.llm_model);
    request["security_level"] = static_cast<int>(prompt.security_level);
    request["generate_tests"] = prompt.generate_tests;
    request["generate_docs"] = prompt.generate_docs;
    const std::string request_body = request.dump();

    Result<std::string> endpoint_result = config_.endpoint_invoke_fn
        ? config_.endpoint_invoke_fn(config_.llm_endpoint, request_body, config_.timeout_ms)
        : invokeEndpointWithCurl(config_.llm_endpoint, request_body, config_.timeout_ms);
    if (!endpoint_result) {
        return tl::unexpected(endpoint_result.error());
    }

    json response;
    try {
        response = json::parse(*endpoint_result);
    } catch (const std::exception& e) {
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

    generated.manifest.name = payload.value("name", std::string("generated_plugin"));
    generated.manifest.version = payload.value("version", std::string("0.1.0"));
    generated.manifest.description = payload.value("description", prompt.description);
    generated.manifest.type = prompt.type;

    if (payload.contains("build_dependencies") && payload["build_dependencies"].is_array()) {
        for (const auto& dep : payload["build_dependencies"]) {
            if (dep.is_string()) {
                generated.build_dependencies.push_back(dep.get<std::string>());
            }
        }
    }

    if (generated.implementation_code.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: endpoint response missing non-empty implementation_code"));
    }

    return generated;
}

} // namespace ai
} // namespace plugins
} // namespace themis
