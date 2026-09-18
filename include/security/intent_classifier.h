/**
 * @file intent_classifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/zero_trust_policy_enforcer.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace security {

class IntentClassifier {
public:
    // ──────────────────────────────────────────────────────────────────────
    // Types
    // ──────────────────────────────────────────────────────────────────────

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

    struct ClassificationResult {
        IntentType  intent;
        double      confidence;          ///< [0.0, 1.0]
        std::string primary_indicator;   ///< human-readable evidence token
    };

    struct IntentAlert {
        IntentType  intent;
        double      confidence;
        std::string session_id;
        std::string shard_id;
        std::vector<float> evidence_embedding;
        double risk_delta;
    };

    // ──────────────────────────────────────────────────────────────────────
    // Construction
    // ──────────────────────────────────────────────────────────────────────

    explicit IntentClassifier(std::string shard_id = "shard-0");

    ~IntentClassifier() = default;

    IntentClassifier(const IntentClassifier&)            = default;
    IntentClassifier& operator=(const IntentClassifier&) = default;


    /**
     * @brief Classify the semantic intent of a query.
     * @param[in] query Raw query string to classify.
     * @param[in] session_context Current zero-trust context for the query.
     * @return Return value.
     */
    ClassificationResult classify(
        const std::string&     query,
        const ZeroTrustContext& session_context
    ) const;

    std::optional<IntentAlert> maybeAlert(
        const ClassificationResult& result,
        const std::string&          session_id,
        double                      confidence_threshold = 0.85
    ) const;

    // ──────────────────────────────────────────────────────────────────────
    // Accessors
    // ──────────────────────────────────────────────────────────────────────

    const std::string& shardId() const noexcept { return shard_id_; }

    /**
     * @brief Return a human-readable name for an intent type.
     * @param[in] t Intent type to render.
     * @return Return value.
     */
    static std::string intentName(IntentType t);

    // ── LoRA-Adapter API (ASL-13 / IMPL-A2) ───────────────────────────────

    enum class LoraLoadResult {
        kSuccess,              ///< Model loaded and active
        kEmptyPath,            ///< Empty path provided; graceful no-op
        kFileNotAccessible,    ///< File not found or not readable
    };

    [[nodiscard]] bool setLoraModelPath(const std::string& model_path);
    [[nodiscard]] LoraLoadResult loadLoraModel(const std::string& model_path);
    [[nodiscard]] bool isLoraActive() const noexcept;
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

    /**
     * @brief Inject an inference function used by classify().
     * @param[in] fn Inference function to inject.
     */
    void setInferenceFn(InferenceFn fn);

#ifdef THEMIS_HAS_LORA_CLASSIFIER
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

    static constexpr std::size_t kEmbeddingDim = 384;

    [[nodiscard]] static double riskDelta(IntentType t) noexcept;

    /**
     * @brief Build a deterministic anonymized embedding from an intent.
     * @param[in] intent Intent category to encode.
     * @param[in] primary_indicator Primary indicator token used for the embedding.
     * @return Return value.
     */
    static std::vector<float> buildEmbedding(
        IntentType         intent,
        const std::string& primary_indicator
    );
};

} // namespace security
} // namespace themis
