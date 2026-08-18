/**
 * @file dpr_vectorizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/vectorizer_interface.h"

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <atomic>

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
     * An empty @p passages input is valid and returns an empty vector immediately
     * without any I/O or ONNX calls.
     *
     * @param passages Vector of passage texts (may be empty).
     * @return Vector of dense embeddings (same length as input, empty when input is empty).
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
    std::atomic<bool> initialized_{false};

    // Opaque implementation details (PIMPL)
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace themis::rag
