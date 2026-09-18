/**
 * @file lora_pattern_classifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct PatternResult {
    std::string label;              ///< Classification label (e.g. "fraud_sequence")
    double      confidence = 0.0;  ///< In [0.0, 1.0]
    std::string adapter_id;        ///< Which adapter produced this result
    bool        used_fallback = false; ///< true iff AutoML/constant fallback was used
};

// ──────────────────────────────────────────────────────────────────────────────
// AdapterDomain
// ──────────────────────────────────────────────────────────────────────────────

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

class LoRAPatternClassifier {
public:
    using Config      = LoRAPatternClassifierConfig;

    using InferenceFn = std::function<std::string(const std::string& adapter_id,
                                                    const std::string& prompt)>;

    using EmbeddingFn = std::function<std::vector<double>(const std::string& text)>;

    explicit LoRAPatternClassifier(Config cfg = Config{});
    ~LoRAPatternClassifier() = default;

    LoRAPatternClassifier(const LoRAPatternClassifier&)            = delete;
    LoRAPatternClassifier& operator=(const LoRAPatternClassifier&) = delete;

    /**
     * @brief ── Injection ─────────────────────────────────────────────────────────────
     * @param[in] fn Inference function to inject.
     */

    void setInferenceFn(InferenceFn fn);

    /**
     * @brief Set Embedding Fn.
     * @param[in] fn Input parameter.
     */
    void setEmbeddingFn(EmbeddingFn fn);

    /**
     * @brief Register Adapter Domain.
     * @param[in] domain Input parameter.
     */
    void registerAdapterDomain(AdapterDomain domain);

    // ── Classification ────────────────────────────────────────────────────────

    [[nodiscard]] PatternResult classify(const std::vector<DataPoint>& events,
                                         const std::string& adapter_id = "");

    [[nodiscard]] std::vector<PatternResult> batchClassify(
        const std::vector<DataPoint>& events);

    [[nodiscard]] std::string selectAdapter(const std::string& context);

    // ── State queries ─────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t registeredAdapterCount() const;
    [[nodiscard]] bool        hasInferenceFn() const;

private:
    [[nodiscard]] static std::string buildPrompt(const std::vector<DataPoint>& events,
                                                  const std::string& adapter_id);

    [[nodiscard]] PatternResult parseInferenceResponse(const std::string& json,
                                                        const std::string& adapter_id) const;

    [[nodiscard]] PatternResult automlFallback(const std::vector<DataPoint>& events,
                                               const std::string& adapter_id) const;

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
