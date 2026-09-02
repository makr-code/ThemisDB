// ============================================================================
// include/query/fts_executor.h
// ============================================================================
// Full-Text Search (FTS) Executor Backend
// Critical module for Query pipeline (ROADMAP line 63)
// 
// Status: WAVE B OPTION B2 — Phase 1 Implementation
// Timeline: Sept 16-30, 2026 (15 days)
// Criticality: CRITICAL (0% implementation, Q4 deadline)
//
// Thread-Safety Contract:
//   - execute() is thread-safe for concurrent reads (acquires shared_lock)
//   - updateIndex() requires exclusive lock (acquires unique_lock)
//   - Concurrent readers allowed via std::shared_mutex
//   - Lock ordering: index_lock_ → cache_lock_ (no deadlock guarantee)
//
// Error Handling:
//   - All APIs return Result<T> (success or FtsError)
//   - FtsError codes in range [7200, 7299]
//   - Timeout & OOM return partial results (not full failure)
//
// ============================================================================

#pragma once

#include <chrono>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>
#include <optional>

#include "absl/status/statusor.h"  // Result type alias
#include "query/fts_parser.h"      // SearchNode AST from parser
#include "query/index_cache.h"     // LRU + Bloom filter cache
#include "query/bm25_scorer.h"     // BM25 scoring

namespace themis::query::fts {

// Error codes for FTS operations (7200-7299 range)
enum class FtsError : uint32_t {
  OK = 0,
  INDEX_NOT_FOUND = 7200,        ///< Index file doesn't exist
  INVALID_INDEX_FORMAT = 7201,   ///< Checksum mismatch or corrupted magic
  INDEX_LOCKED = 7202,           ///< Another process holding index lock
  PARSING_ERROR = 7203,          ///< Malformed query (should not reach here)
  EXECUTION_TIMEOUT = 7204,      ///< Query exceeded time limit
  OUT_OF_MEMORY = 7205,          ///< Cache or index too large
  TERM_NOT_FOUND = 7206,         ///< Term not in index (returns empty results)
  INTERNAL_ERROR = 7299,         ///< Unexpected state
};

// Query result from FTS search
struct SearchResult {
  uint64_t doc_id;                           ///< Document identifier
  float score;                               ///< BM25 score [0, ∞)
  std::vector<uint32_t> term_positions;      ///< Positions of query terms (for highlighting)
  std::string snippet;                       ///< First 200 chars with query context
};

// Execution options for FTS queries
struct ExecutionOptions {
  size_t limit = 100;                                    ///< Max results to return
  std::chrono::milliseconds timeout{1000};              ///< Query time budget
  bool include_snippets = true;                          ///< Generate snippets
  bool parallel_merge = true;                            ///< Use N threads for large result sets
};

// Index statistics (diagnostic API)
struct IndexStatistics {
  uint32_t document_count = 0;               ///< Total documents in index
  uint32_t term_count = 0;                   ///< Unique terms in vocabulary
  uint64_t index_size_bytes = 0;             ///< On-disk index size
  float average_doc_length = 0.0f;           ///< Average document length in tokens
  std::string index_language = "en";         ///< Language of indexed content
};

// Type alias for Result (using Abseil or std::expected)
template <typename T>
using Result = absl::StatusOr<T>;

// ============================================================================
// FtsExecutor — Main Query Execution Engine
// ============================================================================
class FtsExecutor {
 public:
  /// Construction
  /// @param index_path: filesystem path to FTS index directory
  /// @throws std::invalid_argument if index_path invalid
  explicit FtsExecutor(const std::string& index_path);
  
  /// Destruction
  /// @note flushes cache and releases index locks
  ~FtsExecutor();
  
  // ========================================================================
  // Public Query Execution API (Thread-safe for concurrent reads)
  // ========================================================================
  
  /// Execute a single FTS query
  /// @param query: SearchNode AST from FtsParser (already parsed)
  /// @param options: execution options (limit, timeout, snippets, etc.)
  /// @return vector of SearchResult sorted by score (descending)
  /// @thread-safe: acquires shared_lock (multiple readers allowed)
  /// @error: EXECUTION_TIMEOUT if query exceeds timeout_ms
  /// @error: OUT_OF_MEMORY if cache eviction fails
  Result<std::vector<SearchResult>> execute(
      const SearchNode& query,
      const ExecutionOptions& options = ExecutionOptions{});
  
  /// Batch execute multiple queries (optimization for bulk processing)
  /// @param queries: vector of SearchNode ASTs
  /// @param options: common execution options for all queries
  /// @return vector of result vectors (one per input query)
  /// @thread-safe: acquires shared_lock once, amortizes lock overhead
  Result<std::vector<std::vector<SearchResult>>> executeBatch(
      const std::vector<SearchNode>& queries,
      const ExecutionOptions& options = ExecutionOptions{});
  
  // ========================================================================
  // Index Update API (Exclusive lock required)
  // ========================================================================
  
  /// Update FTS index with new documents
  /// @param updates: batch of document additions/deletions
  /// @return status (OK or FtsError)
  /// @thread-safe: acquires unique_lock (exclusive access, blocks all readers)
  /// @error: INDEX_LOCKED if timeout waiting for readers to finish
  Result<void> updateIndex(const IndexUpdateBatch& updates);
  
  // ========================================================================
  // Diagnostic API (Read-only, thread-safe)
  // ========================================================================
  
  /// Get index statistics (diagnostic)
  /// @thread-safe: acquires shared_lock
  /// @return index metadata (document count, term count, size, etc.)
  IndexStatistics getStatistics() const;
  
  /// Check index health
  /// @thread-safe: acquires shared_lock
  /// @return true if index passes integrity checks, false otherwise
  bool isIndexHealthy() const;
  
  /// Get cache hit/miss statistics
  /// @thread-safe: reads atomic counters
  struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t evictions = 0;
    
    float hitRate() const {
      uint64_t total = hits + misses;
      return total > 0 ? static_cast<float>(hits) / total : 0.0f;
    }
  };
  
  /// @thread-safe: no locking required (reads atomic counters)
  CacheStats getCacheStats() const;
  
 private:
  // ========================================================================
  // Private Implementation Details
  // ========================================================================
  
  /// Synchronization primitive for concurrent access
  /// @note: shared_lock for queries, unique_lock for updates
  mutable std::shared_mutex index_lock_;
  
  /// In-memory cache for hot posting lists
  /// @thread-safe: IndexCache has internal locking
  std::unique_ptr<IndexCache> cache_;
  
  /// FTS index abstraction
  std::unique_ptr<FtsIndex> index_;
  
  /// BM25 scorer instance
  std::unique_ptr<BM25Scorer> scorer_;
  
  /// Execution metrics (latency, hit rate, etc.)
  struct ExecutionMetrics {
    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> total_timeout_queries{0};
    std::atomic<uint64_t> total_result_count{0};
  } metrics_;
  
  // Internal helper methods (implementation detail)
  Result<std::vector<SearchResult>> traverseAndScore(
      const SearchNode& query,
      const ExecutionOptions& options);
};

}  // namespace themis::query::fts

