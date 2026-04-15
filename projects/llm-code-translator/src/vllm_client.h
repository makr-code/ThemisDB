/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vllm_client.h                                      ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:10:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     137                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm_translator {

/**
 * @brief Configuration for vLLM REST API client
 */
struct VLLMConfig {
    std::string base_url = "http://localhost:8000";  // vLLM server URL
    std::string model_name = "codegen-16B";          // Model name for code generation
    int timeout_seconds = 30;                         // Request timeout
    int max_retries = 3;                             // Max retry attempts
    bool use_streaming = false;                      // Enable streaming responses
    
    // Generation parameters
    float temperature = 0.7;
    int max_tokens = 2048;
    float top_p = 0.95;
    int top_k = 50;
    std::vector<std::string> stop_sequences = {};
};

/**
 * @brief Response from vLLM API
 */
struct VLLMResponse {
    std::string text;                    // Generated text
    int tokens_used = 0;                 // Number of tokens consumed
    double latency_ms = 0.0;             // Request latency
    bool success = false;                // Whether request succeeded
    std::string error_message = "";      // Error message if failed
    nlohmann::json metadata = {};        // Additional metadata
};

/**
 * @brief Client for communicating with vLLM via REST API
 * 
 * Handles HTTP communication with a local vLLM server for LLM inference.
 * Supports both synchronous and streaming modes.
 */
class VLLMClient {
public:
    explicit VLLMClient(const VLLMConfig& config = VLLMConfig());
    ~VLLMClient();
    
    // Disable copy, enable move
    VLLMClient(const VLLMClient&) = delete;
    VLLMClient& operator=(const VLLMClient&) = delete;
    VLLMClient(VLLMClient&&) noexcept;
    VLLMClient& operator=(VLLMClient&&) noexcept;
    
    /**
     * @brief Generate text completion from prompt
     * @param prompt Input prompt
     * @param temperature Sampling temperature (optional override)
     * @param max_tokens Maximum tokens to generate (optional override)
     * @return Response from vLLM server
     */
    VLLMResponse generate(
        const std::string& prompt,
        std::optional<float> temperature = std::nullopt,
        std::optional<int> max_tokens = std::nullopt
    );
    
    /**
     * @brief Generate multiple completions (for multi-sample generation)
     * @param prompt Input prompt
     * @param n Number of completions to generate
     * @param temperature Sampling temperature
     * @return Vector of responses
     */
    std::vector<VLLMResponse> generateMultiple(
        const std::string& prompt,
        int n = 10,
        float temperature = 0.8
    );
    
    /**
     * @brief Check if vLLM server is healthy and reachable
     * @return true if server is healthy
     */
    bool healthCheck();
    
    /**
     * @brief Get current configuration
     */
    const VLLMConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const VLLMConfig& config) { config_ = config; }
    
private:
    VLLMConfig config_;
    
    // Internal HTTP request handler
    std::string makeRequest(
        const std::string& endpoint,
        const nlohmann::json& payload
    );
    
    // Parse vLLM response JSON
    VLLMResponse parseResponse(const std::string& response_json);
};

} // namespace llm_translator
} // namespace themis
