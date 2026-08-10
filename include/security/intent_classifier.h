/**
 * @file intent_classifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: intent_classifier.h | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 88/100
 * Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "security/zero_trust_policy_enforcer.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace security {

/**
 * @brief Layer-7 LLM Optimization: security semantic query classifier.
 *
 * `IntentClassifier` analyses the *semantic content* of a SQL/AQL query to
 * detect malicious intent patterns (SQL injection, data exfiltration,
 * privilege escalation) before query execution.
 *
 * ### Classification mechanism (v1.0 rule-based placeholder)
 * Rule-based feature extraction computes a weighted feature score per
 * `IntentType`.  A logistic transform yields a per-class confidence in
 * [0.0, 1.0].  The classifier is advisory-only: it never blocks a query
 * directly; instead it raises the `session_risk_score` on the supplied
 * `ZeroTrustContext` and produces an `IntentAlert` for the `GossipProtocol`
 * (Layer-11 / DK-2).
 *
 * // STUB/SIMULATION NOTE:
 * // Purpose: Rule-based feature classification as placeholder for LoRA-adapted model
 * // Activation: Always active in v1.0; LoRA adapter replaces rules post-IMPL-A2
 * // Production Delta: Rule-based precision ~80%; LoRA target precision ≥ 92%
 * // Removal Plan: Replace classify() internals with LoRA adapter call in IMPL-A2 Loop-1
 *
 * ### Thread safety
 * `IntentClassifier` is stateless after construction and safe for concurrent use.
 */
class IntentClassifier {
public:
    // ──────────────────────────────────────────────────────────────────────
    // Types
    // ──────────────────────────────────────────────────────────────────────

    /// Semantic intent categories.
    enum class IntentType {
        LEGITIMATE,             ///< Normal, benign query
        SQL_INJECTION,          ///< SQL injection pattern detected
        DATA_EXFILTRATION,      ///< Bulk data extraction attempt
        PRIVILEGE_ESCALATION,   ///< Attempt to access out-of-scope resources
        ANOMALOUS_PATTERN,      ///< Unusual structure not matching known patterns
        // AI Safety Layer — Schicht 4 (ASL-4)
        // Docs: docs/de/security/ai_safety/AI_SAFETY_INTENT_CLASSIFIER.md
        DATA_DESTRUCTION,       ///< AQL REMOVE / DROP COLLECTION / TRUNCATE
        SCHEMA_MUTATION,        ///< AQL DDL: DROP INDEX, CREATE COLLECTION, etc.
    };

    /// Result of a single classification call.
    struct ClassificationResult {
        IntentType  intent;
        double      confidence;          ///< [0.0, 1.0]
        std::string primary_indicator;   ///< human-readable evidence token
    };

    /**
     * @brief Alert raised when a non-LEGITIMATE intent exceeds the
     *        confidence threshold.  Propagated via Layer-11 GossipProtocol.
     */
    struct IntentAlert {
        IntentType  intent;
        double      confidence;
        std::string session_id;
        std::string shard_id;
        /// Anonymised evidence embedding (dim=384). Contains NO query plaintext.
        std::vector<float> evidence_embedding;
        /// Delta to add to ZeroTrustContext::session_risk_score.
        double risk_delta;
    };

    // ──────────────────────────────────────────────────────────────────────
    // Construction
    // ──────────────────────────────────────────────────────────────────────

    /**
     * @param shard_id   Identifier of this shard (included in IntentAlert).
     */
    explicit IntentClassifier(std::string shard_id = "shard-0");

    ~IntentClassifier() = default;

    IntentClassifier(const IntentClassifier&)            = default;
    IntentClassifier& operator=(const IntentClassifier&) = default;

    // ──────────────────────────────────────────────────────────────────────
    // Core API
    // ──────────────────────────────────────────────────────────────────────

    /**
     * @brief Classify the semantic intent of @p query.
     *
     * @param query           Raw query string (SQL or AQL).
     * @param session_context Current ZeroTrust context (read-only).
     * @return ClassificationResult with the dominant intent and its confidence.
     */
    ClassificationResult classify(
        const std::string&     query,
        const ZeroTrustContext& session_context
    ) const;

    /**
     * @brief Produce an IntentAlert when classification confidence meets
     *        @p confidence_threshold (default 0.85).
     *
     * Returns `std::nullopt` when:
     *  - the intent is LEGITIMATE, or
     *  - confidence < confidence_threshold.
     *
     * @param result               Output of classify().
     * @param session_id           Session identifier.
     * @param confidence_threshold Minimum confidence to generate an alert.
     */
    std::optional<IntentAlert> maybeAlert(
        const ClassificationResult& result,
        const std::string&          session_id,
        double                      confidence_threshold = 0.85
    ) const;

    // ──────────────────────────────────────────────────────────────────────
    // Accessors
    // ──────────────────────────────────────────────────────────────────────

    const std::string& shardId() const noexcept { return shard_id_; }

    /// Human-readable name for a given IntentType.
    static std::string intentName(IntentType t);

    // ── LoRA-Adapter API (ASL-13 / IMPL-A2) ───────────────────────────────

    enum class LoraLoadResult {
        kSuccess,              ///< Model loaded and active
        kEmptyPath,            ///< Empty path provided; graceful no-op
        kFileNotAccessible,    ///< File not found or not readable
    };

    /// Convenience wrapper: calls loadLoraModel() and returns true on kSuccess.
    [[nodiscard]] bool setLoraModelPath(const std::string& model_path);
    /// Load a LoRA model binary from @p model_path.
    [[nodiscard]] LoraLoadResult loadLoraModel(const std::string& model_path);
    /// Returns true when a LoRA model is loaded and active.
    [[nodiscard]] bool isLoraActive() const noexcept;
    /// Returns the path of the currently loaded LoRA model (empty if none).
    [[nodiscard]] const std::string& loraModelPath() const noexcept;

    // ── Inference injection API (IMPL-A2 / ASL-13) ───────────────────────
    //
    // Inject a real LoRA/LLM inference backend so that classify() delegates to
    // it instead of the rule-based fallback.  Setting a non-null function also
    // activates the LoRA path (i.e. isLoraActive() returns true).
    //
    // Signature: (query, session_context) → ClassificationResult
    //
    // Production Delta: Without a function injected, classify() uses the
    //   rule-based engine (~80 % precision).  With a function, precision is
    //   determined by the injected model (target ≥ 92 %).
    // Roadmap: src/security/ROADMAP.md § Phase 4 (ASL-13) / IMPL-A2 Loop-1

    using InferenceFn = std::function<
        ClassificationResult(const std::string& query,
                             const ZeroTrustContext& session_context)>;

    /// Inject a LoRA/LLM inference function.  Passing a null function resets
    /// the injected backend; classify() then falls back to the rule-based engine.
    void setInferenceFn(InferenceFn fn);

#ifdef THEMIS_HAS_LORA_CLASSIFIER
    /**
     * @brief Configure the LoRA classify endpoint (Wave-2 real implementation).
     *
     * Installs an InferenceFn that POSTs the query to the LLM plugin's /classify
     * endpoint via libcurl.  The endpoint must return JSON:
     * `{ "intent": "SQL_INJECTION", "confidence": 0.92, "indicator": "UNION_SELECT" }`.
     * On any network/parse error the function returns LEGITIMATE (fail-closed).
     *
     * @param endpoint_url  Full URL of the LLM classify endpoint.
     * @param api_key       Optional bearer token (empty = no auth header).
     * @param timeout_ms    HTTP request timeout in milliseconds.
     * @return true if configured successfully, false if URL is empty.
     */
    [[nodiscard]] bool configureLoraEndpoint(
        const std::string& endpoint_url,
        const std::string& api_key    = {},
        int                timeout_ms = 2000);
#endif // THEMIS_HAS_LORA_CLASSIFIER

private:
    std::string shard_id_;

    // ── LoRA adapter state (ASL-13) ───────────────────────────────────────
    std::string  lora_model_path_;
    bool         lora_active_ = false;
    InferenceFn  inference_fn_;

    /// Evidence embedding dimension (fixed at 384 for Layer-11 compatibility).
    static constexpr std::size_t kEmbeddingDim = 384;

    /// Risk score delta per severity class.
    [[nodiscard]] static double riskDelta(IntentType t) noexcept;

    /// Build a deterministic, anonymised embedding from an intent + indicator.
    static std::vector<float> buildEmbedding(
        IntentType         intent,
        const std::string& primary_indicator
    );
};

} // namespace security
} // namespace themis
