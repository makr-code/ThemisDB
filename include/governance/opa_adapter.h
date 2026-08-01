/**
 * @file opa_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=11; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: opa_adapter.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 176
 * Gap Summary: total=11; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3076 feat(governance): Integrate... (2026-03-12) | #2775 [auth] OPA integration for ... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * @brief Error classification for OPA adapter failures (Phase 3).
 *
 * Used to categorize different types of errors encountered during
 * OPA policy evaluation for proper fallback handling.
 */
enum class OpaErrorType {
    kTimeout         = 0,  // Request exceeded timeout
    kMalformedResponse = 1,  // Response could not be parsed
    kNetworkError    = 2,  // Connection/network failure
    kInvalidPolicy   = 3,  // OPA policy error
    kUnknown         = 4,  // Other error
};

/**
 * @brief Error information from OPA evaluation failure.
 */
struct OpaError {
    OpaErrorType type = OpaErrorType::kUnknown;
    std::string message;
    int64_t timestamp_ms = 0;
};

/**
 * @brief Adapter that evaluates governance policy decisions via an OPA sidecar.
 *
 * Implements PolicyEngine::IPolicyEvaluator by posting a JSON input document
 * to `POST /v1/data/{policy_path}` on a local OPA server.  If OPA is
 * unreachable or returns a non-2xx response within the configured timeout,
 * evaluate() returns std::nullopt so the caller falls back to native
 * PolicyEngine evaluation.
 *
 * OPA input format:
 * @code{.json}
 * {
 *   "input": {
 *     "headers": {
 *       "X-Classification": "vs-nfd",
 *       "X-User-Id":        "alice"
 *     },
 *     "route": "/vector/search"
 *   }
 * }
 * @endcode
 *
 * OPA response (structured):
 * @code{.json}
 * {
 *   "result": {
 *     "allow":                    true,
 *     "classification":           "offen",
 *     "mode":                     "enforce",
 *     "encrypt_logs":             false,
 *     "redaction":                "standard",
 *     "ann_allowed":              true,
 *     "require_content_encryption": false,
 *     "export_allowed":           true,
 *     "cache_allowed":            true,
 *     "retention_days":           365
 *   }
 * }
 * @endcode
 *
 * OPA response (simple boolean):
 * - `{"result": false}` — deny with strict defaults (no export, encrypt everything).
 * - `{"result": true}`  — OPA allows but provides no governance details;
 *                         returns std::nullopt so native evaluation fills in the
 *                         classification details.
 *
 * Usage:
 * @code{.cpp}
 * governance::OpaAdapter::Config cfg;
 * cfg.endpoint_url = "http://localhost:8181";
 * cfg.policy_path  = "themis/governance/allow";
 * cfg.timeout_ms   = 50;
 *
 * auto adapter = std::make_shared<governance::OpaAdapter>(cfg);
 * governance_engine.setOpaEvaluator(adapter.get());
 * @endcode
 */
class OpaAdapter : public PolicyEngine::IPolicyEvaluator {
public:
    struct Config {
        /// Base URL of the OPA server (e.g. "http://localhost:8181").
        std::string endpoint_url = "http://localhost:8181";
        /// Rego decision path below /v1/data/ (e.g. "themis/governance/allow").
        std::string policy_path  = "themis/governance/allow";
        /// Total request timeout in milliseconds (default: 50 ms).
        long timeout_ms = 50;

        /// Evaluation mode: REST (default) or WASM bundle.
        enum class EvalMode { REST, WASM };
        EvalMode mode = EvalMode::REST;

        /// Path to pre-compiled OPA bundle (.wasm) for WASM evaluation mode.
        /// Ignored when mode == REST.
        std::string wasm_bundle_path;
    };

    explicit OpaAdapter(const Config& config);
    ~OpaAdapter() override;

    OpaAdapter(const OpaAdapter&)            = delete;
    OpaAdapter& operator=(const OpaAdapter&) = delete;
    OpaAdapter(OpaAdapter&&)                 = delete;
    OpaAdapter& operator=(OpaAdapter&&)      = delete;

    /**
     * @brief WASM evaluation callback type.
     *
     * When injected via setWasmEvalFn(), this function is called by
     * evaluateWasm() instead of the built-in stub.  It receives the
     * same (headers, route) inputs as evaluate() and must return an
     * optional PolicyDecision (std::nullopt = fall through to REST).
     *
     * Injection enables tests and alternative runtime integrations
     * (e.g. a real wasmer/wasmtime binding) to exercise WASM-mode
     * policy evaluation without the THEMIS_ENABLE_OPA_WASM build flag.
     *
     * Passing nullptr clears the override and restores the built-in
     * stub behaviour (bundle-exists → permissive allow).
     */
    using WasmEvalFn = std::function<std::optional<PolicyDecision>(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route)>;

    /**
     * @brief Inject a custom WASM evaluator (replaces the built-in stub).
     *
     * @param fn  Evaluator callback; pass nullptr to restore stub behaviour.
     */
    void setWasmEvalFn(WasmEvalFn fn);

    /**
     * @brief Query OPA for a governance policy decision.
     *
     * Sends a synchronous HTTP POST to `{endpoint_url}/v1/data/{policy_path}`.
     * Returns a PolicyDecision on success.
     * Returns std::nullopt when OPA is unreachable, times out, or returns an
     * unexpected response (including `{"result": true}` without governance
     * details) so the caller can fall back to native evaluation.
     */
    std::optional<PolicyDecision> evaluate(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route) const override;

    const Config& getConfig() const { return config_; }

private:
    Config config_;
    WasmEvalFn wasm_eval_fn_;

    /// Build the full OPA query URL from config.
    std::string buildUrl() const;

    /// Build the JSON request body.
    static std::string buildRequestBody(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route);

    /// Parse OPA response and extract a PolicyDecision.
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
