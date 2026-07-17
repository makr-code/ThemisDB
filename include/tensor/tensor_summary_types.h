/**
 * @file tensor_summary_types.h
 * @brief Tensor summary type abstractions for the tensor mid-layer.
 * 
 * Defines concrete summary types for different entity categories:
 * adapters, packages, shards, entities, chunks, and fingerprints.
 */

#pragma once

#include "tensor/compression_strategy.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// BaseTensorSummary — common base for all summary types
// ============================================================================

/**
 * @brief Base class for tensor summaries.
 * 
 * Contains metadata common to all summary types:
 * identifier, compression info, similarity scoring, and routing metadata.
 */
struct BaseTensorSummary {
    /// Unique identifier (varies by type: adapter_key, package_id, entity_id, etc.).
    std::string id;

    /// Tenant namespace.
    std::string tenant_id;

    /// Domain tag (e.g., "legal", "medical", "scientific").
    std::string domain;

    /// Compression strategy used to produce this summary.
    std::string compression_strategy;

    /// Compression metadata and metrics.
    CompressionResult compression_info;

    /// Query result: similarity score in [0, 1].
    float similarity_score = 0.0f;

    /// Confidence in the similarity score (0.0-1.0).
    float confidence = 1.0f;

    /// Whether this summary has been validated against truth data.
    bool validated = false;

    /// Timestamp (ISO-8601) when the summary was created or updated.
    std::string created_at;

    /// Routing reason for selection (human-readable).
    std::string routing_reason;
};

// ============================================================================
// AdapterSummary — summary for LoRA/PEFT adapters
// ============================================================================

/**
 * @brief Tensor summary for a LoRA/PEFT adapter.
 * 
 * Represents an adapter in compressed form with metadata about
 * its base model, parameter count, and fingerprint similarity.
 */
struct AdapterSummary : public BaseTensorSummary {
    /// Adapter storage key (e.g., "__adapters__:t1:legal:llama3").
    std::string adapter_key;

    /// Base model identifier (e.g., "llama3-8b", "gpt-4-turbo").
    std::string base_model_id;

    /// Total parameter count across all TT-cores.
    std::size_t param_count = 0;

    /// Average TT-rank across all cores.
    std::size_t avg_tt_rank = 0;

    /// Frobenius norm of the adapter (for normalization).
    float adapter_norm = 0.0f;

    /// Fingerprint vector (first-core column means or LSH hash).
    std::vector<float> fingerprint;

    /// Metadata about the adapter (author, creation date, etc.).
    std::string adapter_metadata;

    /// Whether this adapter is suitable for inference in the current query context.
    bool inference_ready = true;
};

// ============================================================================
// PackageSummary — summary for adapter packages
// ============================================================================

/**
 * @brief Tensor summary for an adapter package (collection of related adapters).
 * 
 * Aggregates multiple adapters sharing similar purpose or domain,
 * enabling efficient multi-adapter retrieval and composition.
 */
struct PackageSummary : public BaseTensorSummary {
    /// Package identifier.
    std::string package_id;

    /// Number of adapters in this package.
    std::size_t adapter_count = 0;

    /// Adapter keys contained in the package.
    std::vector<std::string> adapter_keys;

    /// Average similarity within the package (inter-adapter similarity).
    float internal_similarity = 0.0f;

    /// Package-level fingerprint (centroid of contained adapters).
    std::vector<float> package_fingerprint;

    /// Whether the package is ready for production use.
    bool production_ready = true;

    /// Version tag for the package (e.g., "v1.0", "v1.2-beta").
    std::string version;

    /// Human-readable description of the package.
    std::string description;
};

// ============================================================================
// Freshness state enumeration for shard summaries
// ============================================================================

/**
 * @brief Freshness state of a shard summary.
 * 
 * Used to track whether a summary is suitable for routing decisions,
 * exact fragment loading, or must be rejected as stale.
 */
enum class SummaryFreshnessState : uint8_t {
    FRESH = 0,      ///< Summary is current and can be used for routing
    STALE = 1,      ///< Summary has exceeded TTL and should be rejected
    INVALID = 2     ///< Summary is corrupted or invalid
};

// ============================================================================
// ShardSummary — summary for cross-shard results
// ============================================================================

/**
 * @brief Tensor summary for results from a specific shard.
 * 
 * Represents compression of candidates retrieved from a single shard,
 * with metadata about shard relevance and contribution.
 * Includes freshness tracking for distributed summary-first routing.
 */
struct ShardSummary : public BaseTensorSummary {
    /// Shard identifier (e.g., "shard_0", "eu-west-1/shard_5").
    std::string shard_id;

    /// Shard scope kind (matches AnnScopeKind).
    uint8_t scope_kind = 0;

    /// Number of candidates returned by this shard before compression.
    std::size_t candidates_before_compression = 0;

    /// Number of candidates after compression/sampling.
    std::size_t candidates_after_compression = 0;

    /// Shard relevance score (how relevant is this shard to the query).
    float shard_relevance = 0.0f;

    /// Latency of shard retrieval in milliseconds.
    float retrieval_latency_ms = 0.0f;

    /// Whether the shard responded successfully.
    bool shard_healthy = true;

    /// Shard-local routing reason or error message.
    std::string shard_routing_reason;

    /// Compressed candidate summaries from this shard.
    std::vector<std::string> compressed_candidates;

    // ─── Freshness & Staleness Tracking ───────────────────────────────────

    /// Timestamp (ISO-8601) when this summary was last updated on the shard.
    std::string last_update_timestamp;

    /// Time-to-live for this summary in seconds (0 = no TTL).
    uint32_t freshness_ttl_seconds = 3600;

    /// Current freshness state (FRESH, STALE, or INVALID).
    SummaryFreshnessState freshness_state = SummaryFreshnessState::FRESH;

    /**
     * @brief Check if this summary is stale based on current time.
     * 
     * A summary is considered stale if:
     * - freshness_state == STALE or INVALID, OR
     * - Current time > last_update_timestamp + freshness_ttl_seconds
     * 
     * @param now_timestamp ISO-8601 timestamp for comparison (defaults to now)
     * @return true if summary is stale or invalid
     */
    [[nodiscard]] bool isStale(const std::string& now_timestamp = "") const noexcept;

    /**
     * @brief Mark this summary as stale.
     * 
     * Used when discovery or validation detects that the summary is outdated.
     */
    void markAsStale() noexcept {
        freshness_state = SummaryFreshnessState::STALE;
    }

    /**
     * @brief Mark this summary as invalid.
     * 
     * Used when discovery detects corruption or integrity failure.
     */
    void markAsInvalid() noexcept {
        freshness_state = SummaryFreshnessState::INVALID;
    }

    /**
     * @brief Mark this summary as fresh.
     * 
     * Updates freshness_state to FRESH and optionally updates timestamp.
     * 
     * @param update_timestamp if true, sets created_at to current time
     */
    void markAsFresh(bool update_timestamp = false) noexcept;
};

// ============================================================================
// EntitySummary — summary for knowledge graph entities
// ============================================================================

/**
 * @brief Tensor summary for a knowledge graph entity.
 * 
 * Represents a compressed entity with its relationships,
 * attributes, and similarity to the query context.
 */
struct EntitySummary : public BaseTensorSummary {
    /// Entity identifier (URI or internal ID).
    std::string entity_id;

    /// Entity type or class (e.g., "Person", "Organization", "Concept").
    std::string entity_type;

    /// Entity label or name (human-readable).
    std::string entity_label;

    /// Number of relationships this entity participates in.
    std::size_t relationship_count = 0;

    /// Keys of related entities (IDs of neighbors in graph).
    std::vector<std::string> related_entity_ids;

    /// Relationship types to neighbors.
    std::vector<std::string> relationship_types;

    /// Entity embedding or features (compressed).
    std::vector<float> entity_embedding;

    /// Optional entity attributes as key-value pairs.
    std::string entity_attributes;

    /// Graph centrality measure (e.g., pagerank score).
    float centrality_score = 0.0f;
};

// ============================================================================
// ChunkSummary — summary for document chunks
// ============================================================================

/**
 * @brief Tensor summary for a document chunk or passage.
 * 
 * Represents compressed chunk with metadata about source document,
 * relevance, and embeddings.
 */
struct ChunkSummary : public BaseTensorSummary {
    /// Chunk identifier (e.g., "doc_123#chunk_5").
    std::string chunk_id;

    /// Source document identifier.
    std::string document_id;

    /// Document title or name.
    std::string document_title;

    /// Chunk content (may be truncated or abstracted).
    std::string chunk_content;

    /// Chunk position in the document (byte offset or chunk number).
    std::size_t chunk_position = 0;

    /// Chunk content length in characters.
    std::size_t content_length = 0;

    /// BM25 relevance score from full-text search (if available).
    float bm25_score = 0.0f;

    /// TF-IDF score for the query terms.
    float tfidf_score = 0.0f;

    /// Chunk embedding or features (compressed).
    std::vector<float> chunk_embedding;

    /// Document metadata (author, date, source, etc.).
    std::string document_metadata;

    /// Whether chunk is from a trusted/verified source.
    bool verified_source = false;
};

// ============================================================================
// FingerprintSummary — summary based on LSH fingerprints
// ============================================================================

/**
 * @brief Tensor summary based on locality-sensitive hashing fingerprints.
 * 
 * Represents results retrieved through fingerprint similarity,
 * used for fast approximate matching without full vector operations.
 */
struct FingerprintSummary : public BaseTensorSummary {
    /// Fingerprint hash value (64-bit or variable-length).
    std::string fingerprint_hash;

    /// Fingerprint bit-length (64, 128, 256, etc.).
    uint8_t fingerprint_bits = 0;

    /// Hash function used (e.g., "SHA256", "MINHASH", "LSH_COSINE").
    std::string hash_function;

    /// Number of candidates matching this fingerprint.
    std::size_t candidates_matched = 0;

    /// Hamming distance to query fingerprint (0 = exact match).
    uint32_t hamming_distance = 0;

    /// LSH band and row information for bucketing.
    std::string lsh_band_info;

    /// Candidate IDs that matched this fingerprint (top-k).
    std::vector<std::string> matched_candidates;

    /// Estimated collision probability for this bucket.
    float collision_probability = 0.0f;

    /// Whether this fingerprint had false positives in validation.
    bool had_false_positives = false;
};

// ============================================================================
// SummaryFactory — factory for creating summary instances
// ============================================================================

/**
 * @brief Factory for creating tensor summary instances.
 */
class SummaryFactory {
public:
    /**
     * @brief Create an adapter summary from compression result.
     * 
     * @param adapter_key       Adapter storage key.
     * @param base_model_id     Base model identifier.
     * @param compression_result Compression operation result.
     * @return AdapterSummary with populated fields.
     */
    static AdapterSummary createAdapterSummary(
        const std::string&        adapter_key,
        const std::string&        base_model_id,
        const CompressionResult&  compression_result);

    /**
     * @brief Create a package summary from contained adapters.
     * 
     * @param package_id   Package identifier.
     * @param adapter_keys Adapter keys in the package.
     * @return PackageSummary with aggregated metadata.
     */
    static PackageSummary createPackageSummary(
        const std::string&              package_id,
        const std::vector<std::string>& adapter_keys);

    /**
     * @brief Create a shard summary from shard-local candidates.
     * 
     * @param shard_id              Shard identifier.
     * @param candidates_before     Candidate count before compression.
     * @param compression_result    Compression operation result.
     * @return ShardSummary with shard metadata.
     */
    static ShardSummary createShardSummary(
        const std::string&        shard_id,
        std::size_t               candidates_before,
        const CompressionResult&  compression_result);
};

} // namespace tensor
} // namespace themis
