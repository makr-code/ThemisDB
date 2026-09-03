#include "query/index_cache.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <mutex>

namespace themis::query::fts {
namespace {

size_t estimatePostingListSize(const PostingList& list) {
  size_t size = sizeof(PostingListEntry) * list.size();
  for (const auto& entry : list) {
    size += sizeof(uint32_t) * entry.positions.capacity();
  }
  return size;
}

size_t bloomHash(const std::string& term, uint64_t seed) {
  return std::hash<std::string>{}(term) ^ (seed + 0x9e3779b97f4a7c15ULL +
                                           (seed << 6U) + (seed >> 2U));
}

}  // namespace

IndexCache::IndexCache() : IndexCache(Config{}) {}

IndexCache::IndexCache(const Config& config) : config_(config) {
  const size_t bit_count = std::max<size_t>(64, config_.bloom_filter_size_bits);
  bloom_bits_.assign((bit_count + 63U) / 64U, 0U);
  stats_.max_size_bytes = config_.max_size_mb * 1024U * 1024U;
}

std::optional<PostingList> IndexCache::lookup(const std::string& term) const {
  if (term.empty()) {
    std::unique_lock<std::shared_mutex> lock(lock_);
    stats_.misses++;
    return std::nullopt;
  }

  std::unique_lock<std::shared_mutex> lock(lock_);
  if (bloom_bits_.empty()) {
    stats_.misses++;
    return std::nullopt;
  }

  const size_t bit_count = bloom_bits_.size() * 64U;
  const size_t h1 = bloomHash(term, 0x1234ULL) % bit_count;
  const size_t h2 = bloomHash(term, 0x5678ULL) % bit_count;
  const bool maybe_present = ((bloom_bits_[h1 / 64U] >> (h1 % 64U)) & 1U) != 0U &&
                             ((bloom_bits_[h2 / 64U] >> (h2 % 64U)) & 1U) != 0U;
  if (!maybe_present) {
    stats_.misses++;
    return std::nullopt;
  }

  auto it = entries_.find(term);
  if (it == entries_.end()) {
    stats_.misses++;
    return std::nullopt;
  }

  lru_.erase(it->second.lru_it);
  lru_.push_front(term);
  it->second.lru_it = lru_.begin();
  stats_.hits++;
  return it->second.posting_list;
}

bool IndexCache::insert(const std::string& term, PostingList&& list) {
  if (term.empty()) {
    return false;
  }

  const size_t max_bytes = config_.max_size_mb * 1024U * 1024U;
  const size_t list_size = estimatePostingListSize(list);
  if (list_size > max_bytes) {
    return false;
  }

  std::unique_lock<std::shared_mutex> lock(lock_);
  stats_.max_size_bytes = max_bytes;

  auto existing = entries_.find(term);
  if (existing != entries_.end()) {
    current_size_bytes_ -= existing->second.size_bytes;
    lru_.erase(existing->second.lru_it);
    entries_.erase(existing);
  }

  while (current_size_bytes_ + list_size > max_bytes && !lru_.empty()) {
    const std::string evicted_term = lru_.back();
    lru_.pop_back();
    auto evicted = entries_.find(evicted_term);
    if (evicted != entries_.end()) {
      current_size_bytes_ -= evicted->second.size_bytes;
      entries_.erase(evicted);
      stats_.evictions++;
    }
  }

  if (current_size_bytes_ + list_size > max_bytes) {
    return false;
  }

  lru_.push_front(term);
  Entry entry;
  entry.posting_list = std::move(list);
  entry.size_bytes = list_size;
  entry.lru_it = lru_.begin();
  entries_[term] = std::move(entry);
  current_size_bytes_ += list_size;

  const size_t bit_count = bloom_bits_.size() * 64U;
  const size_t h1 = bloomHash(term, 0x1234ULL) % bit_count;
  const size_t h2 = bloomHash(term, 0x5678ULL) % bit_count;
  bloom_bits_[h1 / 64U] |= (1ULL << (h1 % 64U));
  bloom_bits_[h2 / 64U] |= (1ULL << (h2 % 64U));

  stats_.current_size_bytes = current_size_bytes_;
  return true;
}

void IndexCache::clear() {
  std::unique_lock<std::shared_mutex> lock(lock_);
  entries_.clear();
  lru_.clear();
  std::fill(bloom_bits_.begin(), bloom_bits_.end(), 0U);
  current_size_bytes_ = 0;
  stats_.current_size_bytes = 0;
}

IndexCache::Stats IndexCache::getStats() const {
  std::shared_lock<std::shared_mutex> lock(lock_);
  Stats snapshot = stats_;
  snapshot.current_size_bytes = current_size_bytes_;
  snapshot.max_size_bytes = config_.max_size_mb * 1024U * 1024U;
  return snapshot;
}

}  // namespace themis::query::fts
