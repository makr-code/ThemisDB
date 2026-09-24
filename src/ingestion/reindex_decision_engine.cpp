/**
 * @file reindex_decision_engine.cpp
 * @brief Embedding & Index Version Governance - Reindex Decision Engine Implementation
 *
 * Implements the reindex trigger detection logic per EMBEDDING_VERSION_GOVERNANCE.md
 * §Reindex Decision Logic. Detects changes in:
 * 1. Embedding model ID
 * 2. Embedding dimensionality
 * 3. Chunking profile ID
 * 4. Index schema major version
 *
 * @date 2026-09-24
 */

#include "ingestion/reindex_decision_engine.h"

#include <spdlog/spdlog.h>

namespace themis::ingestion {

// ============================================================================
// EmbeddingMetadata Implementation
// ============================================================================

nlohmann::json EmbeddingMetadata::to_json() const {
  return nlohmann::json{
      {"embedding_model_id", embedding_model_id},
      {"embedding_model_name", embedding_model_name},
      {"embedding_model_version", embedding_model_version},
      {"embedding_dim", embedding_dim},
      {"embedding_hash", embedding_hash},
      {"embedding_normalized", embedding_normalized},
      {"embedding_provider", embedding_provider},
      {"embedding_commit_hash", embedding_commit_hash},
  };
}

EmbeddingMetadata EmbeddingMetadata::from_json(const nlohmann::json& j) {
  EmbeddingMetadata result;
  if (j.contains("embedding_model_id")) result.embedding_model_id = j["embedding_model_id"];
  if (j.contains("embedding_model_name")) result.embedding_model_name = j["embedding_model_name"];
  if (j.contains("embedding_model_version")) {
    result.embedding_model_version = j["embedding_model_version"];
  }
  if (j.contains("embedding_dim")) result.embedding_dim = j["embedding_dim"];
  if (j.contains("embedding_hash")) result.embedding_hash = j["embedding_hash"];
  if (j.contains("embedding_normalized")) result.embedding_normalized = j["embedding_normalized"];
  if (j.contains("embedding_provider")) result.embedding_provider = j["embedding_provider"];
  if (j.contains("embedding_commit_hash")) {
    result.embedding_commit_hash = j["embedding_commit_hash"];
  }
  return result;
}

// ============================================================================
// ChunkingMetadata Implementation
// ============================================================================

nlohmann::json ChunkingMetadata::to_json() const {
  return nlohmann::json{
      {"chunking_profile_id", chunking_profile_id},
      {"chunk_size_bytes", chunk_size_bytes},
      {"chunk_overlap_bytes", chunk_overlap_bytes},
      {"chunking_strategy", chunking_strategy},
      {"preserve_sentence_boundaries", preserve_sentence_boundaries},
      {"min_chunk_size", min_chunk_size},
      {"max_chunk_size", max_chunk_size},
      {"version", version},
  };
}

ChunkingMetadata ChunkingMetadata::from_json(const nlohmann::json& j) {
  ChunkingMetadata result;
  if (j.contains("chunking_profile_id")) result.chunking_profile_id = j["chunking_profile_id"];
  if (j.contains("chunk_size_bytes")) result.chunk_size_bytes = j["chunk_size_bytes"];
  if (j.contains("chunk_overlap_bytes")) result.chunk_overlap_bytes = j["chunk_overlap_bytes"];
  if (j.contains("chunking_strategy")) result.chunking_strategy = j["chunking_strategy"];
  if (j.contains("preserve_sentence_boundaries")) {
    result.preserve_sentence_boundaries = j["preserve_sentence_boundaries"];
  }
  if (j.contains("min_chunk_size")) result.min_chunk_size = j["min_chunk_size"];
  if (j.contains("max_chunk_size")) result.max_chunk_size = j["max_chunk_size"];
  if (j.contains("version")) result.version = j["version"];
  return result;
}

// ============================================================================
// IndexSchema Implementation
// ============================================================================

int IndexSchema::major_version() const {
  // Extract major version from "2.0" -> 2
  size_t dot_pos = schema_version.find('.');
  if (dot_pos == std::string::npos) {
    return 0;  // Fallback for malformed versions
  }
  try {
    return std::stoi(schema_version.substr(0, dot_pos));
  } catch (...) {
    return 0;  // Fallback for parsing errors
  }
}

nlohmann::json IndexSchema::to_json() const {
  return nlohmann::json{
      {"schema_version", schema_version},
      {"index_type", index_type},
      {"bm25_params",
       nlohmann::json{{"k1", bm25_k1}, {"b", bm25_b}, {"delta", bm25_delta}}},
      {"hnsw_params",
       nlohmann::json{{"m", hnsw_m},
                      {"ef_construction", hnsw_ef_construction},
                      {"ef_search", hnsw_ef_search},
                      {"metric", hnsw_metric}}},
      {"rrf_fusion_k", rrf_fusion_k},
  };
}

IndexSchema IndexSchema::from_json(const nlohmann::json& j) {
  IndexSchema result;
  if (j.contains("schema_version")) result.schema_version = j["schema_version"];
  if (j.contains("index_type")) result.index_type = j["index_type"];

  if (j.contains("bm25_params")) {
    const auto& bm25 = j["bm25_params"];
    if (bm25.contains("k1")) result.bm25_k1 = bm25["k1"];
    if (bm25.contains("b")) result.bm25_b = bm25["b"];
    if (bm25.contains("delta")) result.bm25_delta = bm25["delta"];
  }

  if (j.contains("hnsw_params")) {
    const auto& hnsw = j["hnsw_params"];
    if (hnsw.contains("m")) result.hnsw_m = hnsw["m"];
    if (hnsw.contains("ef_construction")) result.hnsw_ef_construction = hnsw["ef_construction"];
    if (hnsw.contains("ef_search")) result.hnsw_ef_search = hnsw["ef_search"];
    if (hnsw.contains("metric")) result.hnsw_metric = hnsw["metric"];
  }

  if (j.contains("rrf_fusion_k")) result.rrf_fusion_k = j["rrf_fusion_k"];
  return result;
}

// ============================================================================
// ReindexDecision Implementation
// ============================================================================

nlohmann::json ReindexDecision::to_json() const {
  return nlohmann::json{
      {"reindex_required", reindex_required},
      {"reason", reason},
      {"estimated_duration_seconds", estimated_duration_seconds},
      {"atomic_rollback_available", atomic_rollback_available},
  };
}

// ============================================================================
// ReindexDecisionEngine Implementation
// ============================================================================

ReindexDecision ReindexDecisionEngine::decide_reindex(
    const EmbeddingMetadata& old_metadata,
    const EmbeddingMetadata& new_metadata,
    const ChunkingMetadata& old_chunking,
    const ChunkingMetadata& new_chunking,
    const IndexSchema& old_schema,
    const IndexSchema& new_schema) const {
  
  // Trigger 1: Embedding model ID change
  // Example: "all-minilm-l6-v2-0.9" -> "all-minilm-l6-v2-1.0"
  if (old_metadata.embedding_model_id != new_metadata.embedding_model_id) {
    spdlog::info(
        "[ReindexDecisionEngine] Embedding model change detected: {} -> {}",
        old_metadata.embedding_model_id, new_metadata.embedding_model_id);
    return {true, "embedding_model_change", 3600.0, true};
  }

  // Trigger 2: Embedding dimensionality change
  // Example: 384 -> 1024
  if (old_metadata.embedding_dim != new_metadata.embedding_dim) {
    spdlog::info(
        "[ReindexDecisionEngine] Embedding dimension change detected: {} -> {}",
        old_metadata.embedding_dim, new_metadata.embedding_dim);
    return {true, "embedding_dim_change", 3600.0, true};
  }

  // Trigger 3: Chunking profile ID change
  // Example: "default_256_overlap_32" -> "default_512_overlap_64"
  if (old_chunking.chunking_profile_id != new_chunking.chunking_profile_id) {
    spdlog::info(
        "[ReindexDecisionEngine] Chunking profile change detected: {} -> {}",
        old_chunking.chunking_profile_id, new_chunking.chunking_profile_id);
    return {true, "chunking_profile_change", 1800.0, true};
  }

  // Trigger 4: Index schema major version change
  // Example: 1.x -> 2.0
  int old_major = old_schema.major_version();
  int new_major = new_schema.major_version();
  if (old_major != new_major) {
    spdlog::info(
        "[ReindexDecisionEngine] Index schema major version change detected: {}.x -> {}.x",
        old_major, new_major);
    return {true, "index_schema_major_change", 7200.0, true};
  }

  // No reindex needed
  spdlog::debug("[ReindexDecisionEngine] No reindex triggers matched");
  return {false, "no_reindex_needed", 0.0, true};
}

ReindexDecision ReindexDecisionEngine::decide_reindex_embedding_only(
    const EmbeddingMetadata& old_metadata,
    const EmbeddingMetadata& new_metadata) const {
  
  if (old_metadata.embedding_model_id != new_metadata.embedding_model_id) {
    return {true, "embedding_model_change", 3600.0, true};
  }

  if (old_metadata.embedding_dim != new_metadata.embedding_dim) {
    return {true, "embedding_dim_change", 3600.0, true};
  }

  return {false, "no_reindex_needed", 0.0, true};
}

ReindexDecision ReindexDecisionEngine::decide_reindex_chunking_only(
    const ChunkingMetadata& old_chunking,
    const ChunkingMetadata& new_chunking) const {
  
  if (old_chunking.chunking_profile_id != new_chunking.chunking_profile_id) {
    return {true, "chunking_profile_change", 1800.0, true};
  }

  return {false, "no_reindex_needed", 0.0, true};
}

ReindexDecision ReindexDecisionEngine::decide_reindex_schema_only(
    const IndexSchema& old_schema,
    const IndexSchema& new_schema) const {
  
  int old_major = old_schema.major_version();
  int new_major = new_schema.major_version();
  if (old_major != new_major) {
    return {true, "index_schema_major_change", 7200.0, true};
  }

  return {false, "no_reindex_needed", 0.0, true};
}

}  // namespace themis::ingestion
