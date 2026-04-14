/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            advanced_cache_manager.cpp                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-13 20:33:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     271                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f0fea9a7b5  2026-04-12  feat(performance): add NUMAMemoryManager — Issue #228 (pa... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2026 ThemisDB
// Licensed under MIT License

#include "performance/advanced_cache_manager.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>

namespace themis {
namespace performance {

// ---------------------------------------------------------------------------
// BloomFilter helpers
// ---------------------------------------------------------------------------

uint64_t AdvancedCacheManager::BloomFilter::hash(const std::string& key,
                                                   uint64_t seed) noexcept {
    // FNV-1a with seed mixing
    uint64_t h = 14695981039346656037ULL ^ seed;
    for (unsigned char c : key) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

void AdvancedCacheManager::BloomFilter::insert(const std::string& key) noexcept {
    for (uint64_t s = 0; s < 3; ++s) {
        uint64_t bit = hash(key, s) % kBits;
        bits[bit / 64] |= (1ULL << (bit % 64));
    }
}

bool AdvancedCacheManager::BloomFilter::maybe_contains(
        const std::string& key) const noexcept {
    for (uint64_t s = 0; s < 3; ++s) {
        uint64_t bit = hash(key, s) % kBits;
        if (!(bits[bit / 64] & (1ULL << (bit % 64)))) return false;
    }
    return true;
}

void AdvancedCacheManager::BloomFilter::clear() noexcept {
    std::memset(bits, 0, sizeof(bits));
}

// ---------------------------------------------------------------------------
// Compression stubs (would call LZ4/Snappy/Zstd in production)
// ---------------------------------------------------------------------------

std::string AdvancedCacheManager::compress(const std::string& val,
                                            [[maybe_unused]] CompressionAlgorithm algo) {
    // In production: call lz4_compress / snappy::Compress / ZSTD_compress
    return val;
}

std::string AdvancedCacheManager::decompress(const std::string& val,
                                              [[maybe_unused]] CompressionAlgorithm algo) {
    return val;
}

// ---------------------------------------------------------------------------
// Capacity calculation
// ---------------------------------------------------------------------------

size_t AdvancedCacheManager::entries_for_mb(size_t mb) noexcept {
    // Assume average entry of ~256 bytes (key + value)
    return (mb * 1024ULL * 1024ULL) / 256ULL;
}

// ---------------------------------------------------------------------------
// AdvancedCacheManager
// ---------------------------------------------------------------------------

AdvancedCacheManager::AdvancedCacheManager() {
    CacheConfig def;
    def.total_size_mb = 256;
    def.partitions.push_back({"default", 256, EvictionPolicy::LRU, false});
    create_partitions(def);
}

AdvancedCacheManager::AdvancedCacheManager(const CacheConfig& config) {
    create_partitions(config);
}

AdvancedCacheManager::~AdvancedCacheManager() = default;

void AdvancedCacheManager::create_partitions(const CacheConfig& config) {
    config_ = config;
    partitions_.clear();
    for (const auto& pcfg : config.partitions) {
        auto ps             = std::make_unique<PartitionState>();
        ps->cfg             = pcfg;
        ps->capacity        = entries_for_mb(pcfg.size_mb);
        if (ps->capacity == 0) ps->capacity = 16;
        partitions_.push_back(std::move(ps));
    }
}

std::vector<std::string> AdvancedCacheManager::partition_names() const {
    std::vector<std::string> names;
    for (const auto& p : partitions_) names.push_back(p->cfg.name);
    return names;
}

AdvancedCacheManager::PartitionState*
AdvancedCacheManager::find_partition(const std::string& name) const noexcept {
    for (const auto& p : partitions_) {
        if (p->cfg.name == name) return p.get();
    }
    return nullptr;
}

std::optional<std::string> AdvancedCacheManager::get(const std::string& key,
                                                       const std::string& partition) {
    PartitionState* ps = find_partition(partition);
    if (!ps) return std::nullopt;

    std::lock_guard<std::mutex> lk(ps->mtx);

    // Bloom filter fast-miss
    if (config_.enable_bloom_filters && !ps->bloom.maybe_contains(key)) {
        ++ps->stats.misses;
        double total = static_cast<double>(ps->stats.hits + ps->stats.misses);
        ps->stats.hit_rate = total > 0 ? ps->stats.hits / total : 0.0;
        return std::nullopt;
    }

    auto it = ps->index.find(key);
    if (it == ps->index.end()) {
        ++ps->stats.misses;
        double total = static_cast<double>(ps->stats.hits + ps->stats.misses);
        ps->stats.hit_rate = total > 0 ? ps->stats.hits / total : 0.0;
        return std::nullopt;
    }

    // Move to front (LRU update)
    ps->lru_list.splice(ps->lru_list.begin(), ps->lru_list, it->second);

    ++ps->stats.hits;
    double total = static_cast<double>(ps->stats.hits + ps->stats.misses);
    ps->stats.hit_rate = total > 0 ? ps->stats.hits / total : 0.0;

    const std::string& stored = it->second->value;
    if (ps->cfg.enable_compression) {
        return decompress(stored, ps->cfg.compression);
    }
    return stored;
}

void AdvancedCacheManager::put(const std::string& key,
                                const std::string& value,
                                const std::string& partition) {
    PartitionState* ps = find_partition(partition);
    if (!ps) return;

    std::lock_guard<std::mutex> lk(ps->mtx);

    std::string stored_value = ps->cfg.enable_compression
        ? compress(value, ps->cfg.compression)
        : value;

    auto it = ps->index.find(key);
    if (it != ps->index.end()) {
        // Update existing entry and move to front
        it->second->value = std::move(stored_value);
        ps->lru_list.splice(ps->lru_list.begin(), ps->lru_list, it->second);
    } else {
        // Evict if full
        if (ps->lru_list.size() >= ps->capacity) {
            auto& lru_entry = ps->lru_list.back();
            ps->index.erase(lru_entry.key);
            ps->lru_list.pop_back();
            if (ps->stats.entries > 0) --ps->stats.entries;
            ps->stats.bytes_used = ps->stats.bytes_used > lru_entry.value.size()
                ? ps->stats.bytes_used - lru_entry.value.size() : 0;
        }
        ps->lru_list.push_front({key, std::move(stored_value)});
        ps->index[key] = ps->lru_list.begin();
        ++ps->stats.entries;
        if (config_.enable_bloom_filters) ps->bloom.insert(key);
    }
    ps->stats.bytes_used += ps->lru_list.front().value.size();
}

bool AdvancedCacheManager::evict(const std::string& key,
                                  const std::string& partition) {
    PartitionState* ps = find_partition(partition);
    if (!ps) return false;

    std::lock_guard<std::mutex> lk(ps->mtx);
    auto it = ps->index.find(key);
    if (it == ps->index.end()) return false;

    ps->stats.bytes_used = ps->stats.bytes_used > it->second->value.size()
        ? ps->stats.bytes_used - it->second->value.size() : 0;
    ps->lru_list.erase(it->second);
    ps->index.erase(it);
    if (ps->stats.entries > 0) --ps->stats.entries;
    return true;
}

bool AdvancedCacheManager::contains(const std::string& key,
                                     const std::string& partition) const {
    PartitionState* ps = find_partition(partition);
    if (!ps) return false;
    std::lock_guard<std::mutex> lk(ps->mtx);
    return ps->index.count(key) > 0;
}

PartitionStats AdvancedCacheManager::get_partition_stats(
        const std::string& partition) const {
    PartitionState* ps = find_partition(partition);
    if (!ps) return {};
    std::lock_guard<std::mutex> lk(ps->mtx);
    return ps->stats;
}

void AdvancedCacheManager::reset_stats() {
    for (auto& p : partitions_) {
        std::lock_guard<std::mutex> lk(p->mtx);
        p->stats = PartitionStats{};
        p->stats.entries   = p->index.size();
        p->stats.bytes_used = 0;
        for (const auto& e : p->lru_list) p->stats.bytes_used += e.value.size();
    }
}

void AdvancedCacheManager::flush_partition(const std::string& partition) {
    PartitionState* ps = find_partition(partition);
    if (!ps) return;
    std::lock_guard<std::mutex> lk(ps->mtx);
    ps->lru_list.clear();
    ps->index.clear();
    ps->bloom.clear();
    ps->stats = PartitionStats{};
}

void AdvancedCacheManager::flush_all() {
    for (auto& p : partitions_) flush_partition(p->cfg.name);
}

}  // namespace performance
}  // namespace themis
