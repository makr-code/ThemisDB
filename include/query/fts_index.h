// ============================================================================
// include/query/fts_index.h
// ============================================================================
// FTS Index Abstraction Layer
// Provides uniform interface to on-disk index (regardless of backend)
//
// Data Structures:
//   - Vocabulary: map of term → (df, posting_list_offset)
//   - Posting Lists: (doc_id, term_freq, positions) sorted by doc_id
//   - Document Stats: doc_length, avg_tf, language per document
//
// Thread-Safety: NOT thread-safe on its own
//   - FtsExecutor wraps this with shared_mutex
//   - Safe to share const pointer across threads (read-only)
//
// ============================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace themis::query::fts {

// Document metadata
struct DocumentMetadata {
  uint64_t doc_id = 0;                 ///< Document identifier
  uint32_t length_tokens;          ///< Document length in tokens
  float avg_term_freq;             ///< Average term frequency in document
  std::string language;            ///< Document language ("en", "de", etc.)
};

// Posting list entry: (doc_id, term_freq, positions)
struct PostingListEntry {
  uint64_t doc_id = 0;                 ///< Document identifier
  uint32_t term_freq;              ///< Raw term frequency in document
  std::vector<uint32_t> positions; ///< Byte offsets of term occurrences
};

// Posting list: collection of documents containing a term
using PostingList = std::vector<PostingListEntry>;

// Index statistics
struct IndexStatistics {
  uint32_t document_count = 0;               ///< Total documents
  uint32_t term_count = 0;                   ///< Unique terms
  uint64_t index_size_bytes = 0;             ///< On-disk index size
  float average_doc_length = 0.0f;           ///< Average doc length
  std::string language = "en";               ///< Index language
};

// ============================================================================
// FtsIndex — Abstraction for on-disk FTS index
// ============================================================================
class FtsIndex {
 public:
  /**
   * @brief Open.
   * @param[in] index_path Path to the index.
   * @return Return value.
   */
  static std::unique_ptr<FtsIndex> open(const std::string& index_path);
  
  /**
   * @brief Fts Index.
   * @return Return value.
   */
  virtual ~FtsIndex() = default;
  
  /**
   * @brief ======================================================================== Query API (Read-only operations) ========================================================================
   * @param[in] term Input parameter.
   * @return Return value.
   */
  
  virtual PostingList lookupTerm(const std::string& term) const = 0;
  
  /**
   * @brief Get Document Stats.
   * @param[in] doc_id Identifier of the doc.
   * @return Return value.
   */
  virtual DocumentMetadata getDocumentStats(uint64_t doc_id) const = 0;
  
  /**
   * @brief Return access control statistics.
   * @return Access control statistics.
   */
  virtual IndexStatistics getStatistics() const = 0;
  
  /**
   * @brief Is Healthy.
   * @return True when the operation succeeds.
   */
  virtual bool isHealthy() const = 0;
  
  // ========================================================================
  // Update API (Write operations)
  // ========================================================================
  
  virtual void addDocuments(
      const std::vector<std::pair<uint64_t, std::string>>& documents) = 0;
  
  /**
   * @brief Remove Documents.
   * @param[in] doc_ids Input parameter.
   */
  virtual void removeDocuments(
      const std::vector<uint64_t>& doc_ids) = 0;
};

struct IndexUpdateBatch {
  std::vector<std::pair<uint64_t, std::string>> additions;  ///< Docs to add
  std::vector<uint64_t> deletions;                          ///< Doc IDs to remove
};

}  // namespace themis::query::fts
