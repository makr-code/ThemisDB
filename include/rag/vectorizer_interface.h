/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vectorizer_interface.h                             ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-18 18:04:35                                ║
  Author:          Copilot AI                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 BETA                                         ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     85                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔄 In Development (Wave A2)                                  ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file vectorizer_interface.h
 * @brief Abstract interface for vector encoding in RAG systems
 *
 * Provides IVectorizer, a pluggable interface for dense vector generation
 * used in hybrid retrieval and RAG systems. Implementations can include
 * generic embedders (Sentence-BERT) or specialized models (DPR bi-encoders).
 *
 * @reference Karpukhin et al. (2021) "Dense Passage Retrieval for Open-Domain QA"
 *            arXiv:2004.04906
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>

namespace themis::rag {

/**
 * @brief Abstract interface for vector encoding.
 *
 * Implementations encode text (queries, passages) into dense vectors suitable
 * for similarity-based retrieval and ranking. Implementations must be
 * thread-safe.
 *
 * Throws std::runtime_error on initialization or encoding failures.
 */
class IVectorizer {
public:
    virtual ~IVectorizer() = default;

    /**
     * @brief Initialize the vectorizer with configuration.
     *
     * Called before any encoding operations. Must succeed before
     * @ref encodeQuery() or @ref encodePassage() are called.
     *
     * @throws std::runtime_error if initialization fails.
     */
    virtual void initialize() = 0;

    /**
     * @brief Check if the vectorizer is ready for encoding.
     *
     * @return true if @ref initialize() succeeded and models are loaded.
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Encode a query string into a dense vector.
     *
     * Query encoding typically uses a query-specific model/projection.
     * Implementations may optimize for query-shaped inputs.
     *
     * @param query The query text to encode.
     * @return Dense vector of query embeddings.
     * @throws std::runtime_error if encoding fails.
     */
    virtual std::vector<float> encodeQuery(const std::string& query) = 0;

    /**
     * @brief Encode a passage/document into a dense vector.
     *
     * Passage encoding typically uses a passage-specific model/projection
     * and is often batch-optimized for high throughput.
     *
     * @param passage The passage text to encode.
     * @return Dense vector of passage embeddings.
     * @throws std::runtime_error if encoding fails.
     */
    virtual std::vector<float> encodePassage(const std::string& passage) = 0;

    /**
     * @brief Encode multiple passages in a batch.
     *
     * Default implementation calls @ref encodePassage() for each passage.
     * Subclasses may optimize this for GPU batch processing.
     *
     * @param passages Vector of passage texts.
     * @return Vector of dense vectors (same length as input).
     * @throws std::runtime_error if any encoding fails.
     */
    virtual std::vector<std::vector<float>> encodePassageBatch(
        const std::vector<std::string>& passages) {
        std::vector<std::vector<float>> results;
        results.reserve(passages.size());
        for (const auto& passage : passages) {
            results.push_back(encodePassage(passage));
        }
        return results;
    }

    /**
     * @brief Get the embedding dimension of vectors produced by this vectorizer.
     *
     * @return Dimension (e.g., 384 for CLIP, 768 for DistilBERT).
     */
    virtual size_t getEmbeddingDimension() const = 0;
};

}  // namespace themis::rag
