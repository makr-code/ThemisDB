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
#include <list>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "query/fts_index.h"  // PostingList type

namespace themis::query::fts {

class IndexCache {
 public:
  struct Config {
    size_t max_size_mb = 100;              ///< Cache budget (default: 100 MB)
    size_t bloom_filter_size_bits = 1<<20; ///< Bloom filter size (default: 1M bits)
    float bloom_fpp = 0.01f;               ///< False-positive probability (default: 1%)
  };
  
  IndexCache();
  /**
   * @brief Index Cache.
   * @param[in] config Input parameter.
   * @return Return value.
   */
  explicit IndexCache(const Config& config);
  
  /**
   * @brief Lookup.
   * @param[in] term Input parameter.
   * @return Return value.
   */
  std::optional<PostingList> lookup(const std::string& term) const;
  
  /**
   * @brief Insert.
   * @param[in] term Input parameter.
   * @param[in] list Input parameter.
   * @return True when the operation succeeds.
   */
  bool insert(const std::string& term, PostingList&& list);
  
  /**
   * @brief Clear.
   */
  void clear();
  
  struct Stats {
    uint64_t hits = 0;                     ///< Cache hit count
    uint64_t misses = 0;                   ///< Cache miss count
    uint64_t evictions = 0;                ///< LRU eviction count
    size_t current_size_bytes = 0;         ///< Current cache size
    size_t max_size_bytes = 0;             ///< Maximum cache size
    
    float hitRate() const {
      uint64_t total = hits + misses;
      return total > 0 ? static_cast<float>(hits) / total : 0.0f;
    }
  };
  
  /**
   * @brief Get Stats.
   * @return Return value.
   */
  Stats getStats() const;
  
 private:
  struct Entry {
    PostingList posting_list;
    size_t size_bytes = 0;
    std::list<std::string>::iterator lru_it;
  };

  mutable std::shared_mutex lock_;
  Config config_;
  mutable std::unordered_map<std::string, Entry> entries_;
  mutable std::list<std::string> lru_;
  mutable std::vector<uint64_t> bloom_bits_;
  mutable size_t current_size_bytes_ = 0;
  mutable Stats stats_;
};

}  // namespace themis::query::fts
