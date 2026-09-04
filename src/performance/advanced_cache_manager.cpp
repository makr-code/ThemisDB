/**
 * @file advanced_cache_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#include "performance/advanced_cache_manager.h"
#include "storage/codec_tags.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

// Optional compression back-ends — each guarded by its own compile-time flag.
// Install via vcpkg: lz4, snappy, or zstd features.
#ifdef THEMIS_ENABLE_LZ4
#   include <lz4.h>
#   include <lz4hc.h>
#endif
#ifdef THEMIS_ENABLE_SNAPPY
#   include <snappy.h>
#endif
#ifdef THEMIS_ENABLE_ZSTD
#   include <zstd.h>
#endif

namespace themis {
namespace performance {

// ---------------------------------------------------------------------------
// BloomFilter helpers
// ---------------------------------------------------------------------------

uint64_t AdvancedCacheManager::BloomFilter::hash(const std::string& key,
                                                   uint64_t seed) noexcept {
    // FNV-1a with seed mixing
    uint64_t h = 14695981039346656037 ^ seed;
    for (unsigned char c : key) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211;
    }
    return h;
}

void AdvancedCacheManager::BloomFilter::insert(const std::string& key) noexcept {
    for (uint64_t s = 0; s < 3; ++s) {
        uint64_t bit = hash(key, s) % kBits;
        bits[bit / 64] |= (1 << (bit % 64));
    }
}

bool AdvancedCacheManager::BloomFilter::maybe_contains(
        const std::string& key) const noexcept {
    for (uint64_t s = 0; s < 3; ++s) {
        uint64_t bit = hash(key, s) % kBits;
        if (!(bits[bit / 64] & (1 << (bit % 64)))) {
          return false;
        }
    }
    return true;
}

void AdvancedCacheManager::BloomFilter::clear() noexcept {
    std::memset(bits, 0, sizeof(bits));
}

// ---------------------------------------------------------------------------
// Compression helpers
//
// STUB/SIMULATION NOTE:
// Purpose:     passthrough (kTagPassthrough) path is a safe dev/CI fallback
//              when no compression library is compiled in.
// Activation:  default when THEMIS_ENABLE_LZ4, THEMIS_ENABLE_SNAPPY, and
//              THEMIS_ENABLE_ZSTD are all absent from the build.
// Production Delta: passthrough means no size reduction — wire-format is
//              1 byte larger than the original value (the algorithm tag byte).
// Roadmap ref: src/ROADMAP.md § "Consolidation Phase — Compression Codec"
//              (Target: v1.5.0 — install vcpkg features lz4/snappy/zstd)
// Removal Plan: define THEMIS_ENABLE_LZ4 / _SNAPPY / _ZSTD; the passthrough
//              path remains as a graceful fallback, not for removal.
//
// Wire-format defined in include/storage/codec_tags.h (canonical reference):
//   kTagPassthrough (0x00) = no compression
//   kTagLZ4         (0x01) = LZ4 HC + 4-byte LE original size header
//   kTagSnappy      (0x02) = Snappy
//   kTagZstd        (0x03) = Zstd level 3 + 4-byte LE original size header
// ---------------------------------------------------------------------------

namespace {
std::mutex                         s_codec_bridge_mutex;
AdvancedCacheManager::CompressFn   s_compress_fn;
AdvancedCacheManager::DecompressFn s_decompress_fn;

// Import canonical tag constants; using-declarations keep the existing code
// below readable without adding a namespace qualifier to every use.
using themis::compression::kTagPassthrough;
using themis::compression::kTagLZ4;
using themis::compression::kTagSnappy;
using themis::compression::kTagZstd;

[[maybe_unused]] static void write_le32(uint8_t* dst, uint32_t v) noexcept {
    dst[0] = static_cast<uint8_t>(v & 0xFFu);
    dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    dst[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    dst[3] = static_cast<uint8_t>(v >> 24);
}

[[maybe_unused]] static uint32_t read_le32(const uint8_t* src) noexcept {
    return static_cast<uint32_t>(src[0])
         | (static_cast<uint32_t>(src[1]) << 8)
         | (static_cast<uint32_t>(src[2]) << 16)
         | (static_cast<uint32_t>(src[3]) << 24);
}

} // anonymous namespace

void AdvancedCacheManager::setCompressFn(CompressFn fn) {
    std::lock_guard<std::mutex> lk(s_codec_bridge_mutex);
    s_compress_fn = std::move(fn);
}

void AdvancedCacheManager::setDecompressFn(DecompressFn fn) {
    std::lock_guard<std::mutex> lk(s_codec_bridge_mutex);
    s_decompress_fn = std::move(fn);
}

std::string AdvancedCacheManager::compress(const std::string& val,
                                             [[maybe_unused]] CompressionAlgorithm algo) {
    if (val.empty()) {
        // Empty input: tag-only frame, decompress returns ""
        return std::string(1, static_cast<char>(kTagPassthrough));
    }

    [[maybe_unused]] const auto src      = reinterpret_cast<const char*>(val.data());
    [[maybe_unused]] const auto src_size = static_cast<int>(val.size());
    [[maybe_unused]] const auto orig_u32 = static_cast<uint32_t>(val.size());

#ifdef THEMIS_ENABLE_LZ4
    if (algo == CompressionAlgorithm::LZ4) {
        // LZ4 HC (high compression) — max output bound from LZ4 API.
        const int max_dst = LZ4_compressBound(src_size);
        if (max_dst > 0) {
            // Frame: [tag(1)] [orig_size_le32(4)] [lz4_data]
            std::string out(1 + 4 + static_cast<size_t>(max_dst), '\0');
            out[0] = static_cast<char>(kTagLZ4);
            write_le32(reinterpret_cast<uint8_t*>(&out[1]), orig_u32);
            const int compressed = LZ4_compress_HC(
                src, &out[5], src_size, max_dst, LZ4HC_CLEVEL_DEFAULT);
            if (compressed > 0) {
                out.resize(1 + 4 + static_cast<size_t>(compressed));
                return out;
            }
        }
        // Fall through to passthrough on error
    }
#endif

#ifdef THEMIS_ENABLE_SNAPPY
    if (algo == CompressionAlgorithm::Snappy) {
        std::string compressed_body = {};
        snappy::Compress(src, static_cast<size_t>(src_size), &compressed_body);
        if (!compressed_body.empty()) {
            // Frame: [tag(1)] [snappy_data] (Snappy encodes original size internally)
            std::string out = {};
            out.reserve(1 + compressed_body.size());
            out += static_cast<char>(kTagSnappy);
            out += compressed_body;
            return out;
        }
    }
#endif

#ifdef THEMIS_ENABLE_ZSTD
    if (algo == CompressionAlgorithm::Zstd) {
        const size_t bound = ZSTD_compressBound(static_cast<size_t>(src_size));
        if (!ZSTD_isError(bound)) {
            // Frame: [tag(1)] [orig_size_le32(4)] [zstd_data]
            std::string out(1 + 4 + bound, '\0');
            out[0] = static_cast<char>(kTagZstd);
            write_le32(reinterpret_cast<uint8_t*>(&out[1]), orig_u32);
            const size_t written = ZSTD_compress(
                &out[5], bound,
                src, static_cast<size_t>(src_size),
                /*compressionLevel=*/3);
            if (!ZSTD_isError(written)) {
                out.resize(1 + 4 + written);
                return out;
            }
        }
    }
#endif

    {
        CompressFn fn;
        {
            std::lock_guard<std::mutex> lk(s_codec_bridge_mutex);
            fn = s_compress_fn;
        }
        if (fn) {
            try {
                auto bridged = fn(val, algo);
                if (!bridged.empty()) {
                    return bridged;
                }
            } catch (...) {
            }
        }
    }

    // Passthrough: [tag(1)] [original data]
    std::string out = {};
    out.reserve(1 + val.size());
    out += static_cast<char>(kTagPassthrough);
    out += val;
    return out;
}

std::string AdvancedCacheManager::decompress(const std::string& val,
                                              [[maybe_unused]] CompressionAlgorithm algo) {
    if (val.empty()) return {};

    const auto tag = static_cast<uint8_t>(val[0]);

    if (tag == kTagPassthrough) {
        // Strip leading tag byte and return original data.
        return val.size() > 1 ? val.substr(1) : std::string{};
    }

#ifdef THEMIS_ENABLE_LZ4
    if (tag == kTagLZ4 && val.size() > 5) {
        const uint32_t orig_size =
            read_le32(reinterpret_cast<const uint8_t*>(&val[1]));
        if (orig_size == 0) return {};
        std::string out(orig_size, '\0');
        const int decoded = LZ4_decompress_safe(
            &val[5],
            &out[0],
            static_cast<int>(val.size() - 5),
            static_cast<int>(orig_size));
        if (decoded == static_cast<int>(orig_size)) {
            return out;
        }
        // Corrupted or truncated data — return raw payload without the tag.
        return val.substr(5);
    }
#endif

#ifdef THEMIS_ENABLE_SNAPPY
    if (tag == kTagSnappy && val.size() > 1) {
        std::string out = {};
        if (snappy::Uncompress(&val[1], static_cast<int>(val.size()) - 1, &out)) {
            return out;
        }
        return val.substr(1);
    }
#endif

#ifdef THEMIS_ENABLE_ZSTD
    if (tag == kTagZstd && val.size() > 5) {
        const uint32_t orig_size =
            read_le32(reinterpret_cast<const uint8_t*>(&val[1]));
        if (orig_size == 0) return {};
        std::string out(orig_size, '\0');
        const size_t decoded = ZSTD_decompress(
            &out[0], orig_size,
            &val[5], static_cast<int>(val.size()) - 5);
        if (!ZSTD_isError(decoded) && decoded == orig_size) {
            return out;
        }
        return val.substr(5);
    }
#endif

    {
        DecompressFn fn;
        {
            std::lock_guard<std::mutex> lk(s_codec_bridge_mutex);
            fn = s_decompress_fn;
        }
        if (fn) {
            try {
                auto bridged = fn(val, algo);
                if (!bridged.empty()) {
                    return bridged;
                }
            } catch (...) {
            }
        }
    }

    // Unknown tag (data written by a build with a library we don't have):
    // return the payload without the tag byte as the safest fallback.
    return val.size() > 1 ? val.substr(1) : std::string{};
}

// ---------------------------------------------------------------------------
// Capacity calculation
// ---------------------------------------------------------------------------

size_t AdvancedCacheManager::entries_for_mb(size_t mb) noexcept {
    // Assume average entry of ~256 bytes (key + value)
    return (mb * 1024 * 1024) / 256;
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
        if (ps->capacity == 0) {
          ps->capacity = 16;
        }
        partitions_.push_back(std::move(ps));
    }
}

std::vector<std::string> AdvancedCacheManager::partition_names() const {
    std::vector<std::string> names = {};

    for (const auto& p : partitions_) {
      names.push_back(p->cfg.name);
    }
    return names;
}

AdvancedCacheManager::PartitionState*
AdvancedCacheManager::find_partition(const std::string& name) const noexcept {
    for (const auto& p : partitions_) {
        if (p->cfg.name == name) {
          return p.get();
        }
    }
    return nullptr;
}

std::optional<std::string> AdvancedCacheManager::get(const std::string& key,
                                                       const std::string& partition) {
    PartitionState* ps = find_partition(partition);
    if (!ps) {
      return std::nullopt;
    }

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
    if (!ps) {
      return;
    }

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
            if (ps->stats.entries > 0) {
              --ps->stats.entries;
            }
            ps->stats.bytes_used = ps->stats.bytes_used > lru_entry.value.size()
                ? ps->stats.bytes_used - lru_entry.value.size() : 0;
        }
        ps->lru_list.push_front({key, std::move(stored_value)});
        ps->index[key] = ps->lru_list.begin();
        ++ps->stats.entries;
        if (config_.enable_bloom_filters) {
          ps->bloom.insert(key);
        }
    }
    ps->stats.bytes_used += ps->lru_list.front().value.size();
}

bool AdvancedCacheManager::evict(const std::string& key,
                                  const std::string& partition) {
    PartitionState* ps = find_partition(partition);
    if (!ps) {
      return false;
    }

    std::lock_guard<std::mutex> lk(ps->mtx);
    auto it = ps->index.find(key);
    if (it == ps->index.end()) {
      return false;
    }

    ps->stats.bytes_used = ps->stats.bytes_used > it->second->value.size()
        ? ps->stats.bytes_used - it->second->value.size() : 0;
    ps->lru_list.erase(it->second);
    ps->index.erase(it);
    if (ps->stats.entries > 0) {
      --ps->stats.entries;
    }
    return true;
}

bool AdvancedCacheManager::contains(const std::string& key,
                                     const std::string& partition) const {
    PartitionState* ps = find_partition(partition);
    if (!ps) {
      return false;
    }
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
        for (const auto& e : p->lru_list) {
          p->stats.bytes_used += e.value.size();
        }
    }
}

void AdvancedCacheManager::flush_partition(const std::string& partition) {
    PartitionState* ps = find_partition(partition);
    if (!ps) {
      return;
    }
    std::lock_guard<std::mutex> lk(ps->mtx);
    ps->lru_list.clear();
    ps->index.clear();
    ps->bloom.clear();
    ps->stats = PartitionStats{};
}

void AdvancedCacheManager::flush_all() {
    for (auto& p : partitions_) {
      flush_partition(p->cfg.name);
    }
}

}  // namespace performance
}  // namespace themis


