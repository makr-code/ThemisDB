/**
 * @file reindex_decision_engine.h
 * @brief Embedding & Index Version Governance - Reindex Decision Engine
 *
 * Defines the reindex trigger detection logic for embedding model, dimension,
 * chunking profile, and schema version changes. Part of Phase 3 implementation
 * of EMBEDDING_VERSION_GOVERNANCE.md.
 *
 * @see src/ingestion/EMBEDDING_VERSION_GOVERNANCE.md
 * @date 2026-09-24
 */

#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace themis::ingestion {

/**
 * @struct EmbeddingMetadata
 * @brief Embedding model versioning information
 *
 * Tracks the specific embedding model, dimensions, and hash for reproducibility.
 * Used to detect embedding model upgrades or dimension changes that trigger reindex.
 */
struct EmbeddingMetadata {
  std::string embedding_model_id;     ///< Unique model ID: "{name}-{version}"
  std::string embedding_model_name;   ///< Human-readable model name
  std::string embedding_model_version; ///< Semantic version
  uint32_t embedding_dim = 384;        ///< Embedding dimensionality (384, 1024, etc.)
  std::string embedding_hash;          ///< SHA-256 hash of model artifact for integrity
  bool embedding_normalized = true;    ///< Whether vectors are L2-normalized
  std::string embedding_provider;      ///< Provider: "huggingface", "openai", etc.
  std::string embedding_commit_hash;   ///< Git commit hash of model build (for reproducibility)

  /**
   * @brief Convert to JSON for RocksDB persistence
   */
  nlohmann::json to_json() const;

  /**
   * @brief Load from JSON for RocksDB retrieval
   */
  static EmbeddingMetadata from_json(const nlohmann::json& j);
};

/**
 * @struct ChunkingMetadata
 * @brief Chunking strategy versioning information
 *
 * Tracks chunking profile (size, overlap, strategy) changes that trigger reindex.
 */
struct ChunkingMetadata {
  std::string chunking_profile_id;    ///< Unique profile ID: e.g., "default_256_overlap_32"
  uint32_t chunk_size_bytes = 256;    ///< Target chunk size
  uint32_t chunk_overlap_bytes = 32;  ///< Overlap between consecutive chunks
  std::string chunking_strategy;      ///< Strategy: "sliding_window", "sentence_boundary", etc.
  bool preserve_sentence_boundaries = true;
  uint32_t min_chunk_size = 64;
  uint32_t max_chunk_size = 512;
  std::string version = "1.0";        ///< Chunking profile version

  /**
   * @brief Convert to JSON
   */
  nlohmann::json to_json() const;

  /**
   * @brief Load from JSON
   */
  static ChunkingMetadata from_json(const nlohmann::json& j);
};

/**
 * @struct IndexSchema
 * @brief Index schema versioning information
 *
 * Tracks index type and parameters. Major version changes trigger reindex.
 */
struct IndexSchema {
  std::string schema_version = "2.0";  ///< Semantic version (major.minor)
  std::string index_type;              ///< "hybrid_bm25_hnsw", etc.

  // BM25 parameters
  double bm25_k1 = 1.5;
  double bm25_b = 0.75;
  double bm25_delta = 0.5;

  // HNSW parameters
  uint32_t hnsw_m = 16;
  uint32_t hnsw_ef_construction = 200;
  uint32_t hnsw_ef_search = 50;
  std::string hnsw_metric = "cosine";

  // Fusion parameters
  double rrf_fusion_k = 60.0;

  /**
   * @brief Extract major version (e.g., "2.0" -> 2)
   * @return Major version component
   */
  int major_version() const;

  /**
   * @brief Convert to JSON
   */
  nlohmann::json to_json() const;

  /**
   * @brief Load from JSON
   */
  static IndexSchema from_json(const nlohmann::json& j);
};

/**
 * @struct ReindexDecision
 * @brief Result of reindex decision logic
 *
 * Indicates whether reindex is required, the reason, and estimated duration.
 * Used by ingestion pipeline to determine if full reindex is necessary.
 */
struct ReindexDecision {
  bool reindex_required = false;       ///< Whether reindex is needed
  std::string reason;                  ///< Why: "embedding_model_change", etc.
  double estimated_duration_seconds = 0.0;  ///< Estimate for reindex completion
  bool atomic_rollback_available = true;    ///< Whether previous version is recoverable

  /**
   * @brief Convert to JSON for logging/audit trail
   */
  nlohmann::json to_json() const;
};

/**
 * @class ReindexDecisionEngine
 * @brief Determines whether reindex is required based on metadata changes
 *
 * Implements the core logic from EMBEDDING_VERSION_GOVERNANCE.md §Reindex Decision Logic.
 * Checks 4 trigger conditions and returns a decision with reason and estimated duration.
 *
 * **Thread Safety:** Not thread-safe; caller must serialize calls.
 *
 * **Example:**
 * ```cpp
 * EmbeddingMetadata old_meta = {...};  // Current index metadata
 * EmbeddingMetadata new_meta = {...};  // Proposed new model
 * ChunkingMetadata new_chunking = {...};
 * IndexSchema new_schema = {...};
 *
 * ReindexDecisionEngine engine;
 * ReindexDecision decision = engine.decide_reindex(
 *     old_meta, new_meta, new_chunking, new_schema);
 *
 * if (decision.reindex_required) {
 *   LOG_INFO("Reindex needed: {} (est. {:.0f}s)",
 *            decision.reason, decision.estimated_duration_seconds);
 * }
 * ```
 */
class ReindexDecisionEngine {
 public:
  /**
   * @brief Determine if reindex is required
   *
   * Checks 4 trigger conditions in order:
   * 1. Embedding model ID change (e.g., 0.9 -> 1.0)
   * 2. Embedding dimensionality change (e.g., 384 -> 1024)
   * 3. Chunking profile change (e.g., chunk_size 256 -> 512)
   * 4. Index schema major version change (e.g., 1.x -> 2.0)
   *
   * @param old_metadata Current index embedding metadata
   * @param new_metadata Proposed embedding model
   * @param old_chunking Current chunking profile (optional for first 3 triggers)
   * @param new_chunking Proposed chunking profile
   * @param old_schema Current index schema (optional)
   * @param new_schema Proposed index schema
   *
   * @return ReindexDecision with result
   *
   * @note This function returns immediately on first trigger match.
   *       If no triggers match, returns {false, "no_reindex_needed", 0.0, true}.
   */
  ReindexDecision decide_reindex(
      const EmbeddingMetadata& old_metadata,
      const EmbeddingMetadata& new_metadata,
      const ChunkingMetadata& old_chunking,
      const ChunkingMetadata& new_chunking,
      const IndexSchema& old_schema,
      const IndexSchema& new_schema) const;

  /**
   * @brief Simplified API for embedding-only reindex decision
   *
   * Used when only embedding model/dimension change is relevant.
   * Ignores chunking and schema for faster path.
   *
   * @param old_metadata Current embedding metadata
   * @param new_metadata Proposed embedding metadata
   * @return ReindexDecision
   */
  ReindexDecision decide_reindex_embedding_only(
      const EmbeddingMetadata& old_metadata,
      const EmbeddingMetadata& new_metadata) const;

  /**
   * @brief Simplified API for chunking-only reindex decision
   *
   * Used when only chunking profile change is relevant.
   *
   * @param old_chunking Current chunking profile
   * @param new_chunking Proposed chunking profile
   * @return ReindexDecision
   */
  ReindexDecision decide_reindex_chunking_only(
      const ChunkingMetadata& old_chunking,
      const ChunkingMetadata& new_chunking) const;

  /**
   * @brief Simplified API for schema-only reindex decision
   *
   * Used when only schema version change is relevant.
   *
   * @param old_schema Current index schema
   * @param new_schema Proposed index schema
   * @return ReindexDecision
   */
  ReindexDecision decide_reindex_schema_only(
      const IndexSchema& old_schema,
      const IndexSchema& new_schema) const;
};

}  // namespace themis::ingestion
