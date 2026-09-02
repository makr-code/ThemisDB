// ============================================================================
// include/query/index_cache.h
// ============================================================================
// In-Memory Cache for Hot Posting Lists
// LRU Cache + Bloom Filter for fast negative lookups
//
// Strategy:
//   - Bloom filter (1% false-positive rate) for quick "term exists?" check
//   - LRU cache with 100 MB default budget for frequently accessed posting lists
//   - Thread-safe: std::shared_mutex for concurrent reads + exclusive writes
//
// Thread-Safety: THREAD-SAFE for concurrent access
//   - lookup() acquires shared_lock (multiple readers allowed)
//   - insert() acquires unique_lock (exclusive access)
//   - No deadlock (single lock per cache)
//
// ============================================================================

#pragma once

#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include "query/fts_index.h"  // PostingList type

namespace themis::query::fts {

class IndexCache {
 public:
  /// Configuration for cache
  struct Config {
    size_t max_size_mb = 100;              ///< Cache budget (default: 100 MB)
    size_t bloom_filter_size_bits = 1<<20; ///< Bloom filter size (default: 1M bits)
    float bloom_fpp = 0.01f;               ///< False-positive probability (default: 1%)
  };
  
  /// @brief Construct the cache with size and bloom-filter settings.
  /// @param config: cache configuration (size, bloom filter params)
  explicit IndexCache(const Config& config = Config{});
  
  /// @brief Look up a posting list in the in-memory cache.
  /// @param term: search term
  /// @return posting list if present in cache, empty optional if not cached
  /// @note Thread safety: acquires shared_lock (concurrent readers allowed).
  /// @note: bloom filter consulted first (fast negative lookup)
  std::optional<PostingList> lookup(const std::string& term) const;
  
  /// @brief Insert a posting list into the cache.
  /// @param term: search term
  /// @param list: posting list to cache
  /// @return true if inserted, false if cache full (LRU eviction needed)
  /// @note Thread safety: acquires unique_lock (exclusive access).
  /// @note: may evict LRU entries to fit new posting list
  bool insert(const std::string& term, PostingList&& list);
  
  /// @brief Remove all posting lists from the cache.
  /// @note Thread safety: acquires unique_lock.
  void clear();
  
  /// @brief Cache counters and memory-usage totals.
  struct Stats {
    uint64_t hits = 0;                     ///< Cache hit count
    uint64_t misses = 0;                   ///< Cache miss count
    uint64_t evictions = 0;                ///< LRU eviction count
    size_t current_size_bytes = 0;         ///< Current cache size
    size_t max_size_bytes = 0;             ///< Maximum cache size
    
    /// @brief Compute hit ratio over all cache lookups.
    /// @return Hit ratio in range [0, 1].
    float hitRate() const {
      uint64_t total = hits + misses;
      return total > 0 ? static_cast<float>(hits) / total : 0.0f;
    }
  };
  
  /// @brief Return cache statistics snapshot.
  /// @note Thread safety: acquires shared_lock.
  Stats getStats() const;
  
 private:
  mutable std::shared_mutex lock_;
  Config config_;
  
  // LRU cache implementation (intrusive linked-list + hash map)
  // This is an implementation detail, hidden from public interface
};

}  // namespace themis::query::fts
