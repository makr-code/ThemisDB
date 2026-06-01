/**
 * @file tensor_midlayer.h
 * @brief Tensor Mid-Layer — second layer of the hybrid knowledge retrieval stack.
 *
 * Handles tensor summary routing, compression-aware candidate selection,
 * and passage of evidence from ANN candidates to the Graph Truth Layer.
 *
 * Planned in: docs/EPIC1_TENSOR_MIDLAYER.md
 * Sub-issue:   #5425
 */

#pragma once

#include "ann_frontdoor.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::retrieval {

/// Compression scheme applied to stored tensor summaries.
enum class TensorCompression {
    None,     ///< Full-precision float32 vectors
    FP16,     ///< Half-precision float16
    INT8,     ///< Scalar quantisation to int8
    PQ,       ///< Product quantisation
    BF16,     ///< Brain-float16
};

/// A compressed tensor summary record.
struct TensorSummary {
    std::uint64_t   doc_id;
    std::string     shard_key;
    std::vector<std::uint8_t> payload; ///< Compressed representation
    TensorCompression compression;
    float            reconstruction_error = 0.0f; ///< Optional quality signal
};

/// Query descriptor for the tensor mid-layer.
struct TensorQuery {
    std::vector<AnnCandidate> ann_candidates; ///< Input from ANN frontdoor
    std::uint32_t             rerank_top_k = 5;
    TensorCompression         preferred_compression = TensorCompression::FP16;
    bool                      include_cross_shard = false;
};

/// Result produced by the tensor mid-layer.
struct TensorResult {
    std::vector<AnnCandidate> reranked;       ///< Pruned and reranked candidates
    std::vector<TensorSummary> summaries;     ///< Attached tensor summaries
    double                     latency_ms = 0.0;
};

/// Configuration for the tensor mid-layer.
struct TensorMidlayerConfig {
    TensorCompression compression = TensorCompression::FP16;
    std::size_t       summary_cache_max_entries = 50000;
    bool              enable_cross_shard = false;
    std::string       summary_store_path; ///< Path or connection to summary KV
};

/**
 * @brief Tensor Mid-Layer interface.
 *
 * Accepts ANN candidates, fetches tensor summaries, applies compression-aware
 * reranking, and emits a refined candidate set for the Graph Truth Layer.
 */
class ITensorMidlayer {
public:
    virtual ~ITensorMidlayer() = default;

    /// Rerank ANN candidates using tensor summaries.
    virtual TensorResult rerank(const TensorQuery& query) = 0;

    /// Pre-load summaries for a given shard into the local cache.
    virtual void warmCache(const std::string& shard_key) = 0;

    /// Evict cached summaries for a shard (e.g., after a shard migration).
    virtual void evictCache(const std::string& shard_key) = 0;

    /// Return compression scheme in effect.
    virtual TensorCompression activeCompression() const = 0;
};

/// Factory: create a TensorMidlayer from configuration.
std::unique_ptr<ITensorMidlayer> makeTensorMidlayer(const TensorMidlayerConfig& cfg);

} // namespace themis::retrieval
