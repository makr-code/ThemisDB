/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            dpr_vectorizer.h                                   ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-18 18:04:35                                ║
  Author:          Copilot AI                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 BETA                                         ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     145                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔄 In Development (Wave A2: DPR Vectorizer)                  ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file dpr_vectorizer.h
 * @brief Dense Passage Retrieval (DPR) bi-encoder vectorizer for RAG
 *
 * Implements DPRVectorizer, a specialized IVectorizer that uses separate
 * query and passage encoders (BERT-based bi-encoder) for improved semantic
 * matching in open-domain QA and RAG systems.
 *
 * Architecture:
 *  - Query Encoder: BERT-based model fine-tuned for query representation
 *  - Passage Encoder: BERT-based model fine-tuned for passage representation
 *  - Both produce fixed-size embeddings (typically 384-768 dim)
 *  - Similarity: cosine distance or inner product over query/passage vectors
 *
 * Performance Targets (Wave A2):
 *  - MRR@10 improvement ≥ +15% vs. BM25-only baseline
 *  - Query latency ≤ 150 ms (single query on GPU)
 *  - Passage encoding ≥ 100 docs/sec (batch_size=32 on GPU)
 *
 * @reference Karpukhin et al. (2021) "Dense Passage Retrieval for Open-Domain QA"
 *            ICLR 2021, arXiv:2004.04906
 */

#pragma once

#include "rag/vectorizer_interface.h"

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace themis::rag {

/**
 * @brief Configuration for DPRVectorizer.
 *
 * Specifies paths to query and passage encoder models, as well as
 * device and batch-processing preferences.
 */
struct DPRVectorizerConfig {
    /// Path to query encoder model (ONNX or native format)
    std::string query_model_path;

    /// Path to passage encoder model (ONNX or native format)
    std::string passage_model_path;

    /// Device for inference: "cpu" or "cuda"
    std::string device = "cpu";

    /// Batch size for passage encoding (GPU optimization)
    size_t batch_size = 32;

    /// Embedding dimension (typically 384 or 768; auto-detected if 0)
    size_t embedding_dimension = 384;

    /// Maximum token length for queries and passages
    size_t max_token_length = 512;

    /// Normalize embeddings to unit L2 norm for cosine distance
    bool normalize_embeddings = true;
};

/**
 * @brief Dense Passage Retrieval (DPR) bi-encoder vectorizer.
 *
 * Implements the DPR approach: separate query and passage encoders that
 * produce fixed-size embeddings. Used in @ref HybridRetriever for dense
 * retrieval alongside BM25 sparse retrieval.
 *
 * Thread Safety: A single DPRVectorizer instance is NOT thread-safe.
 * Create one instance per thread or protect concurrent calls with a mutex.
 * Batch operations (@ref encodePassageBatch) are optimized for GPU utilization.
 */
class DPRVectorizer : public IVectorizer {
public:
    /**
     * @brief Construct a DPR vectorizer with configuration.
     *
     * @param config Configuration specifying model paths and inference settings.
     */
    explicit DPRVectorizer(const DPRVectorizerConfig& config);

    /**
     * @brief Destructor.
     */
    ~DPRVectorizer() override;

    /**
     * @brief Initialize the vectorizer (load models).
     *
     * Loads query and passage encoder models from configured paths.
     * Must be called before @ref encodeQuery() or @ref encodePassage().
     *
     * @throws std::runtime_error if models cannot be loaded or paths are invalid.
     * @throws std::invalid_argument if configuration is invalid.
     */
    void initialize() override;

    /**
     * @brief Check if vectorizer is initialized and ready.
     *
     * @return true if both query and passage encoders are loaded.
     */
    bool isInitialized() const override;

    /**
     * @brief Encode a query using the query encoder.
     *
     * @param query The query text.
     * @return Dense query embedding (size = embedding_dimension).
     * @throws std::runtime_error if not initialized or encoding fails.
     * @throws std::invalid_argument if query is empty.
     */
    std::vector<float> encodeQuery(const std::string& query) override;

    /**
     * @brief Encode a passage using the passage encoder.
     *
     * @param passage The passage text.
     * @return Dense passage embedding (size = embedding_dimension).
     * @throws std::runtime_error if not initialized or encoding fails.
     * @throws std::invalid_argument if passage is empty.
     */
    std::vector<float> encodePassage(const std::string& passage) override;

    /**
     * @brief Batch-encode multiple passages (GPU-optimized).
     *
     * Processes passages in batches for improved throughput on GPU.
     * Target: ≥ 100 docs/sec for batch_size=32.
     *
     * @param passages Vector of passage texts.
     * @return Vector of dense embeddings (same length as input).
     * @throws std::runtime_error if not initialized or any encoding fails.
     */
    std::vector<std::vector<float>> encodePassageBatch(
        const std::vector<std::string>& passages) override;

    /**
     * @brief Get the embedding dimension.
     *
     * @return Dimension of query/passage embeddings (e.g., 384).
     */
    size_t getEmbeddingDimension() const override;

    /**
     * @brief Get the configuration used by this vectorizer.
     *
     * @return Configuration struct.
     */
    const DPRVectorizerConfig& getConfig() const;

private:
    DPRVectorizerConfig config_;
    bool initialized_ = false;

    // Opaque implementation details (PIMPL)
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace themis::rag
