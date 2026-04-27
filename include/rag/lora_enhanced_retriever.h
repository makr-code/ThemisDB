#pragma once

#include "rag/rag_judge.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// STUB/SIMULATION NOTE:
// Purpose: Real LoRA adapter integration requires THEMIS_ENABLE_LLM and
//          MultiLoRAManager. This file provides a full interface with an
//          ILoRAScorer plugin so that real and heuristic adapters can be
//          substituted transparently.
// Activation: Heuristic scorer is used by default. Real LoRA scoring via
//             MultiLoRAManager requires THEMIS_ENABLE_LLM and a concrete
//             MultiLoRAManagerScorer implementation.
// Production Delta: Real implementation calls MultiLoRAManager::score();
//                   heuristic scorer uses query-document token overlap.
// Removal Plan: Replace HeuristicLoRAScorer with real adapter call in
//               v2.2.0 once MultiLoRAManager::selectAdapterForQuery() is stable.

namespace themis::rag {

// ─────────────────────────────────────────────────────────────────────────────
// ILoRAScorer — plugin interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Plugin interface for LoRA-based document scoring.
 *
 * Implement this interface to supply a real LoRA adapter scorer.
 * The `HeuristicLoRAScorer` provides a lightweight token-overlap baseline.
 *
 * Thread safety: `score()` must be callable from multiple threads.
 */
class ILoRAScorer {
public:
    virtual ~ILoRAScorer() = default;

    /**
     * @brief Score a document for domain relevance given the query.
     *
     * @param query    Original user query.
     * @param content  Document content to score.
     * @param domain   Optional domain hint (e.g. "legal", "medical").
     * @return         Score in [0, 1].  Higher = more relevant.
     */
    virtual double score(const std::string& query,
                         const std::string& content,
                         const std::string& domain = "") = 0;

    /**
     * @brief Return the domain this scorer is specialised for (may be empty).
     */
    virtual std::string domain() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// HeuristicLoRAScorer — lightweight baseline
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Heuristic LoRA scorer based on query–document token overlap.
 *
 * Uses normalised Jaccard similarity over space-split tokens as a
 * proxy for domain relevance.  MRR@10 improvement from this scorer is
 * marginal; replace with a real `MultiLoRAManager`-backed scorer for
 * the +5% MRR@10 target.
 */
class HeuristicLoRAScorer : public ILoRAScorer {
public:
    explicit HeuristicLoRAScorer(std::string domain_hint = "");

    double score(const std::string& query,
                 const std::string& content,
                 const std::string& domain = "") override;

    std::string domain() const override;

private:
    std::string domain_;
};

// ─────────────────────────────────────────────────────────────────────────────
// LoRAEnhancedRetrieverConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for LoRAEnhancedRetriever.
 */
struct LoRARetrieverConfig {
    /// Maximum number of documents to re-rank with the LoRA scorer.
    /// Documents beyond this limit retain their original score.
    std::size_t top_k_rerank = 50;

    /// Weight for the LoRA score in the final fused score.
    /// final_score = orig * (1 - lora_weight) + lora_score * lora_weight
    double lora_weight = 0.4;

    /// Domain hint forwarded to `ILoRAScorer::score()` (e.g. "legal", "medical").
    std::string domain;

    /// Minimum LoRA score threshold; documents below this threshold are
    /// moved to the end of the result list (but not removed).
    double min_lora_score = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// LoRAEnhancedRetriever
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Re-ranks retrieved documents using a domain-specific LoRA scorer.
 *
 * @par Overview
 * Wraps an `ILoRAScorer` and uses it to compute a domain relevance signal for
 * the top-K retrieved documents.  The original retrieval score is fused with
 * the LoRA score via a configurable weight.
 *
 * Production target: MRR@10 ≥ +5% vs pure RRF baseline (Q2 2027).
 *
 * @par Guard
 * The `HeuristicLoRAScorer` is used by default and provides a lightweight
 * token-overlap baseline.  For production quality, inject a real
 * `ILoRAScorer` implementation backed by `MultiLoRAManager` when
 * `THEMIS_ENABLE_LLM` is defined.
 *
 * @par Thread safety
 * `rerank()` is const and safe to call concurrently once constructed.
 *
 * @par Usage example
 * @code{.cpp}
 *   LoRARetrieverConfig cfg;
 *   cfg.domain      = "legal";
 *   cfg.lora_weight = 0.35;
 *   cfg.top_k_rerank = 20;
 *
 *   auto scorer = std::make_shared<HeuristicLoRAScorer>("legal");
 *   LoRAEnhancedRetriever retriever(scorer, cfg);
 *
 *   auto results = retriever.rerank(query, initial_docs);
 * @endcode
 */
class LoRAEnhancedRetriever {
public:
    /**
     * @brief Construct with a LoRA scorer and optional config.
     *
     * @param scorer  LoRA scorer (must outlive this retriever).
     * @param config  Retrieval configuration.
     */
    explicit LoRAEnhancedRetriever(std::shared_ptr<ILoRAScorer> scorer,
                                   LoRARetrieverConfig          config = {});

    ~LoRAEnhancedRetriever();

    LoRAEnhancedRetriever(const LoRAEnhancedRetriever&)            = delete;
    LoRAEnhancedRetriever& operator=(const LoRAEnhancedRetriever&) = delete;
    LoRAEnhancedRetriever(LoRAEnhancedRetriever&&)                 = default;
    LoRAEnhancedRetriever& operator=(LoRAEnhancedRetriever&&)      = default;

    /**
     * @brief Re-rank @p candidates using LoRA scoring.
     *
     * The top `top_k_rerank` documents (by original score) are scored with
     * the LoRA adapter.  The remaining documents are appended with their
     * original scores after the re-ranked set.
     *
     * @param query      Original user query.
     * @param candidates Initial retrieval results (sorted by score DESC).
     * @return           Re-ranked document list sorted by fused score DESC.
     */
    [[nodiscard]] std::vector<judge::RetrievedDocument> rerank(
        const std::string&                           query,
        const std::vector<judge::RetrievedDocument>& candidates) const;

    /**
     * @brief Return the current configuration.
     */
    const LoRARetrieverConfig& config() const noexcept;

    /**
     * @brief Replace configuration.
     */
    void setConfig(const LoRARetrieverConfig& config);

    /**
     * @brief Replace the LoRA scorer.
     */
    void setScorer(std::shared_ptr<ILoRAScorer> scorer);

private:
    std::shared_ptr<ILoRAScorer> scorer_;
    LoRARetrieverConfig          config_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory helpers for common LoRAEnhancedRetriever configurations.
 */
class LoRAEnhancedRetrieverFactory {
public:
    /**
     * @brief Lightweight: top-20 re-rank, low LoRA weight (0.2), no domain.
     */
    static std::unique_ptr<LoRAEnhancedRetriever> createLightweight();

    /**
     * @brief Balanced: top-50 re-rank, moderate LoRA weight (0.35), optional domain.
     */
    static std::unique_ptr<LoRAEnhancedRetriever> createBalanced(
        const std::string& domain = "");

    /**
     * @brief Domain-specific: top-50 re-rank, high LoRA weight (0.5), explicit domain.
     */
    static std::unique_ptr<LoRAEnhancedRetriever> createDomainSpecific(
        const std::string& domain);
};

} // namespace themis::rag
