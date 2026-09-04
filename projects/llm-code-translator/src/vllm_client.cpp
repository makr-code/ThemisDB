/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vllm_client.cpp                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     263                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "vllm_client.h"
#include <curl/curl.h>
#include <stdexcept>
#include <chrono>
#include <sstream>

namespace themis {
namespace llm_translator {

// Callback for CURL to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* out = static_cast<std::string*>(userp);
    const auto* data = static_cast<const char*>(contents);
    out->append(data, size * nmemb);
    return size * nmemb;
}

VLLMClient::VLLMClient(const VLLMConfig& config)
    : config_(config)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

VLLMClient::~VLLMClient() {
    curl_global_cleanup();
}

VLLMClient::VLLMClient(VLLMClient&& other) noexcept
    : config_(std::move(other.config_))
{
}

VLLMClient& VLLMClient::operator=(VLLMClient&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
    }
    return *this;
}

std::string VLLMClient::makeRequest(
    const std::string& endpoint,
    const nlohmann::json& payload
) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    std::string response_string;
    std::string url = config_.base_url + endpoint;
    std::string payload_str = payload.dump();
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        std::string error_msg = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("CURL request failed: " + error_msg);
    }
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    
    if (http_code != 200) {
        throw std::runtime_error("HTTP request failed with code: " + std::to_string(http_code));
    }
    
    return response_string;
}

VLLMResponse VLLMClient::parseResponse(const std::string& response_json) {
    VLLMResponse response;
    
    try {
        auto json = nlohmann::json::parse(response_json);
        
        // vLLM response format: {"text": [...], "usage": {...}}
        if (json.contains("choices") && json["choices"].is_array() && !json["choices"].empty()) {
            response.text = json["choices"][0]["text"].get<std::string>();
            response.success = true;
        } else if (json.contains("text") && json["text"].is_array() && !json["text"].empty()) {
            response.text = json["text"][0].get<std::string>();
            response.success = true;
        } else {
            response.success = false;
            response.error_message = "Unexpected response format";
        }
        
        // Extract token usage if available
        if (json.contains("usage")) {
            if (json["usage"].contains("total_tokens")) {
                response.tokens_used = json["usage"]["total_tokens"].get<int>();
            }
        }
        
        response.metadata = json;
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = std::string("Failed to parse response: ") + e.what();
    }
    
    return response;
}

VLLMResponse VLLMClient::generate(
    const std::string& prompt,
    std::optional<float> temperature,
    std::optional<int> max_tokens
) {
    auto start = std::chrono::high_resolution_clock::now();
    
    nlohmann::json payload = {
        {"prompt", prompt},
        {"model", config_.model_name},
        {"temperature", temperature.value_or(config_.temperature)},
        {"max_tokens", max_tokens.value_or(config_.max_tokens)},
        {"top_p", config_.top_p},
        {"top_k", config_.top_k}
    };
    
    if (!config_.stop_sequences.empty()) {
        payload["stop"] = config_.stop_sequences;
    }
    
    VLLMResponse response;
    
    try {
        std::string response_json = makeRequest("/v1/completions", payload);
        response = parseResponse(response_json);
        
        auto end = std::chrono::high_resolution_clock::now();
        response.latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = e.what();
    }
    
    return response;
}

std::vector<VLLMResponse> VLLMClient::generateMultiple(
    const std::string& prompt,
    int n,
    float temperature
) {
    auto start = std::chrono::high_resolution_clock::now();
    
    nlohmann::json payload = {
        {"prompt", prompt},
        {"model", config_.model_name},
        {"temperature", temperature},
        {"max_tokens", config_.max_tokens},
        {"top_p", config_.top_p},
        {"top_k", config_.top_k},
        {"n", n}  // Request n completions
    };
    
    if (!config_.stop_sequences.empty()) {
        payload["stop"] = config_.stop_sequences;
    }
    
    std::vector<VLLMResponse> responses;
    
    try {
        std::string response_json = makeRequest("/v1/completions", payload);
        auto json = nlohmann::json::parse(response_json);
        
        auto end = std::chrono::high_resolution_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Parse multiple choices
        if (json.contains("choices") && json["choices"].is_array()) {
            for (const auto& choice : json["choices"]) {
                VLLMResponse resp;
                resp.text = choice["text"].get<std::string>();
                resp.success = true;
                resp.latency_ms = latency_ms;
                resp.metadata = choice;
                
                if (json.contains("usage") && json["usage"].contains("total_tokens")) {
                    resp.tokens_used = json["usage"]["total_tokens"].get<int>() / n;
                }
                
                responses.push_back(resp);
            }
        }
        
    } catch (const std::exception& e) {
        // Return single error response
        VLLMResponse error_resp;
        error_resp.success = false;
        error_resp.error_message = e.what();
        responses.push_back(error_resp);
    }
    
    return responses;
}

bool VLLMClient::healthCheck() {
    try {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return false;
        }
        
        std::string response_string = {};
        std::string url = config_.base_url + "/health";
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        
        CURLcode res = curl_easy_perform(curl);
        
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);
        
        return (res == CURLE_OK && http_code == 200);
        
    } catch (const std::exception&) {
        return false;
    } catch (const std::string&) {
        return false;
    } catch (const char*) {
        return false;
    }
}

} // namespace llm_translator
} // namespace themis
