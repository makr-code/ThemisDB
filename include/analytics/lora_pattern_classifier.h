/**
 * @file lora_pattern_classifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * LoRAPatternClassifier — LoRA-adapter-based pattern classification.
 *
 * Classifies CEP event batches, time-series DataPoints, and graph paths
 * using an injected LoRA inference function. Falls back to an adaptive
 * statistical classifier when no inference function is registered.
 *
 * Thread-safety: classify(), batchClassify(), and selectAdapter() are
 * thread-safe.  Injection methods (setInferenceFn, registerAdapterDomain)
 * must not be called concurrently with classify calls.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <vector>

#include "analytics/anomaly_detection.h"  // DataPoint

namespace themisdb {
namespace analytics {

// ──────────────────────────────────────────────────────────────────────────────
// PatternResult
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Output of a single classify() call.
 */
struct PatternResult {
    std::string label;              ///< Classification label (e.g. "fraud_sequence")
    double      confidence = 0.0;  ///< In [0.0, 1.0]
    std::string adapter_id;        ///< Which adapter produced this result
    bool        used_fallback = false; ///< true iff AutoML/constant fallback was used
};

// ──────────────────────────────────────────────────────────────────────────────
// AdapterDomain
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Metadata for a registered LoRA adapter and its domain embedding.
 */
struct AdapterDomain {
    std::string         adapter_id;
    std::string         domain;      ///< Human-readable domain name
    std::vector<double> embedding;   ///< For cosine-similarity adapter selection
};

// ──────────────────────────────────────────────────────────────────────────────
// LoRAPatternClassifierConfig (hoisted outside class for default-arg use)
// ──────────────────────────────────────────────────────────────────────────────
struct LoRAPatternClassifierConfig {
    std::size_t max_parallel_workers = 4;   ///< batchClassify std::async concurrency
    double      fallback_confidence  = 0.5; ///< Prior confidence for statistical fallback
};

// ──────────────────────────────────────────────────────────────────────────────
// LoRAPatternClassifier
// ──────────────────────────────────────────────────────────────────────────────

/** @brief LoRAPatternClassifier. */
class LoRAPatternClassifier {
public:
    using Config      = LoRAPatternClassifierConfig;

    /// Injected LoRA inference function.
    /// Receives (adapter_id, prompt) → JSON string: {"label":"...","confidence":0.92}
    using InferenceFn = std::function<std::string(const std::string& adapter_id,
                                                    const std::string& prompt)>;

    /// Injected embedding function for adapter selection.
    /// Receives text context → dense embedding vector.
    using EmbeddingFn = std::function<std::vector<double>(const std::string& text)>;

    explicit LoRAPatternClassifier(Config cfg = Config{});
    ~LoRAPatternClassifier() = default;

    LoRAPatternClassifier(const LoRAPatternClassifier&)            = delete;
    LoRAPatternClassifier& operator=(const LoRAPatternClassifier&) = delete;

    // ── Injection ─────────────────────────────────────────────────────────────

    /**
     * Inject a LoRA inference backend.
     * Once set, classify() delegates to this function instead of the stub.
     */
    void setInferenceFn(InferenceFn fn);

    /**
     * Inject an embedding function for adapter domain selection.
     */
    void setEmbeddingFn(EmbeddingFn fn);

    /**
     * Register an adapter domain with its pre-computed embedding.
     */
    void registerAdapterDomain(AdapterDomain domain);

    // ── Classification ────────────────────────────────────────────────────────

    /**
     * Classify a set of DataPoints using the specified adapter.
     *
     * If adapter_id is empty, selectAdapter() is called automatically.
     * Falls back to the adaptive statistical classifier when no InferenceFn is set.
     */
    [[nodiscard]] PatternResult classify(const std::vector<DataPoint>& events,
                                         const std::string& adapter_id = "");

    /**
     * Classify each DataPoint individually, parallelised with std::async.
     * At most Config::max_parallel_workers futures are in flight simultaneously.
     *
     * @return One PatternResult per input event, in the same order.
     */
    [[nodiscard]] std::vector<PatternResult> batchClassify(
        const std::vector<DataPoint>& events);

    /**
     * Select the best-matching adapter domain for the given context string.
     *
     * Computes cosine similarity between the context embedding and each
     * registered domain embedding.  Returns the adapter_id with the highest
     * similarity.  Returns the first registered adapter if no EmbeddingFn
     * has been set.  Returns "" if no adapters are registered.
     */
    [[nodiscard]] std::string selectAdapter(const std::string& context);

    // ── State queries ─────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t registeredAdapterCount() const;
    [[nodiscard]] bool        hasInferenceFn() const;

private:
    /**
     * Build a structured text prompt from an event batch (≤ 10 events shown).
     */
    [[nodiscard]] static std::string buildPrompt(const std::vector<DataPoint>& events,
                                                  const std::string& adapter_id);

    /**
     * Parse {"label":"...","confidence":0.92} from an inference response.
     * Returns a fallback PatternResult on parse error.
     */
    [[nodiscard]] PatternResult parseInferenceResponse(const std::string& json,
                                                        const std::string& adapter_id) const;

    /**
     * Adaptive statistical fallback used when no InferenceFn is injected.
     *
     * Derives a robust label and confidence score from observed feature
     * coverage, temporal consistency, and numeric dispersion.
     */
    [[nodiscard]] PatternResult automlFallback(const std::vector<DataPoint>& events,
                                               const std::string& adapter_id) const;

    /**
     * Cosine similarity between two vectors.
     * Returns 0.0 if either vector has zero norm or different sizes.
     */
    [[nodiscard]] static double cosineSimilarity(const std::vector<double>& a,
                                                  const std::vector<double>& b);

    Config                     cfg_;
    InferenceFn                inference_fn_;
    EmbeddingFn                embedding_fn_;
    std::vector<AdapterDomain> domains_;
    mutable std::mutex         mutex_;
};

} // namespace analytics
} // namespace themisdb
