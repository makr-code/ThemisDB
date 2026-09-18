/**
 * @file opa_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_engine.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace themis {
namespace governance {

enum class OpaErrorType {
    kTimeout         = 0,  // Request exceeded timeout
    kMalformedResponse = 1,  // Response could not be parsed
    kNetworkError    = 2,  // Connection/network failure
    kInvalidPolicy   = 3,  // OPA policy error
    kUnknown         = 4,  // Other error
};

struct OpaError {
    OpaErrorType type = OpaErrorType::kUnknown;
    std::string message;
    int64_t timestamp_ms = 0;
};

class OpaAdapter : public PolicyEngine::IPolicyEvaluator {
public:
    struct Config {
        std::string endpoint_url = "http://localhost:8181";
        std::string policy_path  = "themis/governance/allow";
        long timeout_ms = 50;

        enum class EvalMode { REST, WASM };
        EvalMode mode = EvalMode::REST;

        std::string wasm_bundle_path;
    };

    /**
     * @brief Opa Adapter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OpaAdapter(const Config& config);
    ~OpaAdapter() override;

    OpaAdapter(const OpaAdapter&)            = delete;
    OpaAdapter& operator=(const OpaAdapter&) = delete;
    OpaAdapter(OpaAdapter&&)                 = delete;
    OpaAdapter& operator=(OpaAdapter&&)      = delete;

    using WasmEvalFn = std::function<std::optional<PolicyDecision>(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route)>;

    /**
     * @brief Set Wasm Eval Fn.
     * @param[in] fn Input parameter.
     */
    void setWasmEvalFn(WasmEvalFn fn);

    std::optional<PolicyDecision> evaluate(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route) const override;

    const Config& getConfig() const { return config_; }

private:
    Config config_;
    WasmEvalFn wasm_eval_fn_;

    /**
     * @brief Build Url.
     * @return Return value.
     */
    std::string buildUrl() const;

    static std::string buildRequestBody(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route);

    /**
     * @brief Parse Opa Response.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    static std::optional<PolicyDecision> parseOpaResponse(const std::string& body);

    // STUB/SIMULATION NOTE:
    // Purpose: WASM-based OPA bundle evaluation path — evaluates pre-compiled
    //          OPA bundles (.wasm) locally without requiring an OPA sidecar.
    // Activation: Config::mode == EvalMode::WASM and wasm_bundle_path is set.
    // Production Delta: Returns a stub PolicyDecision (allow=true, defaults)
    //   rather than a real WASM evaluation. Actual WASM execution requires the
    //   THEMIS_ENABLE_OPA_WASM build flag and a linked WASM runtime.
    // Removal Plan: Replace stub with real opa-go-wasm binding when
    //   THEMIS_ENABLE_OPA_WASM is enabled in the build.
    std::optional<PolicyDecision> evaluateWasm(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route) const;
};

} // namespace governance
} // namespace themis
