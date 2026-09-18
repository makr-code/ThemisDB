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
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>
#include <optional>

#include "query/fts_parser.h"      // SearchNode AST from parser
#include "query/index_cache.h"     // LRU + Bloom filter cache
#include "query/bm25_scorer.h"     // BM25 scoring
#include "utils/expected.h"

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
  uint64_t doc_id = 0;                           ///< Document identifier
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

// Type alias for Result (using Abseil or std::expected)
template <typename T>
using Result = tl::expected<T, FtsError>;

// ============================================================================
// FtsExecutor — Main Query Execution Engine
// ============================================================================
class FtsExecutor {
 public:
  /**
   * @brief Fts Executor.
   * @param[in] index_path Path to the index.
   * @return Return value.
   */
  explicit FtsExecutor(const std::string& index_path);
  
  FtsExecutor(const std::string& index_path, const IndexCache::Config& cache_config);
  
  ~FtsExecutor();
  
  // ========================================================================
  // Public Query Execution API (Thread-safe for concurrent reads)
  // ========================================================================
  
  Result<std::vector<SearchResult>> execute(
      const SearchNode& query,
      const ExecutionOptions& options = ExecutionOptions{});
  
  Result<std::vector<std::vector<SearchResult>>> executeBatch(
      const std::vector<SearchNode>& queries,
      const ExecutionOptions& options = ExecutionOptions{});
  
  /**
   * @brief ======================================================================== Index Update API (Exclusive lock required) ========================================================================
   * @param[in] updates Input parameter.
   * @return Return value.
   */
  
  Result<void> updateIndex(const IndexUpdateBatch& updates);
  
  /**
   * @brief ======================================================================== Diagnostic API (Read-only, thread-safe) ========================================================================
   * @return Access control statistics.
   */
  
  IndexStatistics getStatistics() const;
  
  /**
   * @brief Is Index Healthy.
   * @return True when the operation succeeds.
   */
  bool isIndexHealthy() const;
  
  struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t evictions = 0;
    
    float hitRate() const {
      uint64_t total = hits + misses;
      return total > 0 ? static_cast<float>(hits) / total : 0.0f;
    }
  };
  
  /**
   * @brief Get Cache Stats.
   * @return Return value.
   */
  CacheStats getCacheStats() const;
  
 private:
  // ========================================================================
  // Private Implementation Details
  // ========================================================================
  
  mutable std::shared_mutex index_lock_;
  
  std::unique_ptr<IndexCache> cache_;
  
  std::unique_ptr<FtsIndex> index_;
  
  std::unique_ptr<BM25Scorer> scorer_;
  
  struct ExecutionMetrics {
    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> total_timeout_queries{0};
    std::atomic<uint64_t> total_result_count{0};
  } metrics_;
  
  /**
   * @brief Internal helper methods (implementation detail)
   * @param[in] query Input parameter.
   * @param[in] options Input parameter.
   * @return Return value.
   */
  Result<std::vector<SearchResult>> traverseAndScore(
      const SearchNode& query,
      const ExecutionOptions& options);
};

}  // namespace themis::query::fts
