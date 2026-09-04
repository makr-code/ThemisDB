/**
 * @file hkdf_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/hkdf_cache.h"
#include "utils/hkdf_helper.h"
#include "utils/error_contracts.h"

#include <openssl/crypto.h>
#include <openssl/sha.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <list>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_map>

namespace themis {
namespace utils {

// ---------------------------------------------------------------------------
// Shard count — power-of-two for cheap modulo
// ---------------------------------------------------------------------------
static constexpr size_t kShards = 16;

// ---------------------------------------------------------------------------
// Per-entry record
// ---------------------------------------------------------------------------
struct CacheEntry {
    std::vector<uint8_t>                     value;
    std::chrono::steady_clock::time_point    inserted_at;
};

// ---------------------------------------------------------------------------
// One LRU shard
// ---------------------------------------------------------------------------
struct Shard {
    using Key  = std::string;
    using List = std::list<Key>;
    using Map  = std::unordered_map<Key, std::pair<List::iterator, CacheEntry>>;

    size_t capacity = 64; // overridden from HKDFCache::Config

    List lru;
    Map  map;
    std::mutex mu;

    // Atomic stats visible across threads via the owning Impl
    std::atomic<uint64_t> hits{0};
    std::atomic<uint64_t> misses{0};
    std::atomic<uint64_t> evictions{0};

    /// Remove one entry by iterator; wipes the key buffer via OPENSSL_cleanse.
    void erase_entry(Map::iterator it) {
        auto& entry = it->second.second;
        OPENSSL_cleanse(entry.value.data(), entry.value.size());
        lru.erase(it->second.first);
        map.erase(it);
        ++evictions;
    }

    /// Evict all entries whose TTL has expired.
    void evict_expired(std::chrono::seconds ttl) {
        if (ttl.count() == 0) {
          return;
        }
        auto now = std::chrono::steady_clock::now();
        for (auto it = map.begin(); it != map.end(); ) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                           now - it->second.second.inserted_at);
            if (age >= ttl) {
                auto next = std::next(it);
                erase_entry(it);
                it = next;
            } else {
                ++it;
            }
        }
    }

    /// Evict LRU tail until size <= capacity.
    void evict_lru() {
        while (map.size() > capacity && !lru.empty()) {
            auto tail = lru.back();
            auto it = map.find(tail);
            if (it != map.end()) {
              erase_entry(it);
            }
            else { lru.pop_back(); } // defensive
        }
    }
};

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct HKDFCache::Impl {
    std::array<Shard, kShards> shards;
    HKDFCache::Config cfg;

    explicit Impl(HKDFCache::Config c) : cfg(std::move(c)) {
        size_t per_shard = std::max<size_t>(1, cfg.max_entries / kShards);
        for (auto& s : shards) {
          s.capacity = per_shard;
        }
    }

    // Build the binary cache key: ikm | 0x00 | salt | 0x00 | info | 0x00 | len
    static std::string make_key(const std::vector<uint8_t>& ikm,
                                 const std::vector<uint8_t>& salt,
                                 const std::string& info,
                                 size_t outlen)
    {
        std::string k;
        k.reserve(ikm.size() + 1 + salt.size() + 1 + info.size() + 1 + 8);
        k.append(reinterpret_cast<const char*>(ikm.data()), ikm.size());
        k.push_back('\x00');
        k.append(reinterpret_cast<const char*>(salt.data()), salt.size());
        k.push_back('\x00');
        k.append(info);
        k.push_back('\x00');
        k.append(std::to_string(outlen));
        return k;
    }

    // Shard index = hash(key) & (kShards-1)  — cheap power-of-two modulo
    static size_t shard_index(const std::string& k) {
        return std::hash<std::string>{}(k) & (kShards - 1);
    }

    // SHA-256 of data → hex string
    static std::string sha256_hex(const uint8_t* data, size_t len) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(data, len, digest);
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (unsigned char b : digest) {
          oss << std::setw(2) << static_cast<int>(b);
        }
        return oss.str();
    }
};

// ---------------------------------------------------------------------------
// HKDFCache public methods
// ---------------------------------------------------------------------------

HKDFCache::HKDFCache(Config cfg) : impl_(std::make_unique<Impl>(std::move(cfg))) {}
HKDFCache::~HKDFCache() = default;

HKDFCache& HKDFCache::threadLocal() {
    thread_local HKDFCache instance;
    return instance;
}

void HKDFCache::setCapacity([[maybe_unused]] size_t cap) {
    impl_->cfg.max_entries = cap ? cap : 1;
    size_t per_shard = std::max<size_t>(1, impl_->cfg.max_entries / kShards);
    for (auto& s : impl_->shards) {
        std::lock_guard<std::mutex> lk(s.mu);
        s.capacity = per_shard;
    }
}

void HKDFCache::clear() {
    for (auto& s : impl_->shards) {
        std::lock_guard<std::mutex> lk(s.mu);
        // Cleanse all cached key material before releasing memory.
        for (auto& [key, pair] : s.map) {
            OPENSSL_cleanse(pair.second.value.data(), pair.second.value.size());
        }
        s.lru.clear();
        s.map.clear();
    }
}

std::vector<uint8_t> HKDFCache::derive_cached(const std::vector<uint8_t>& ikm,
                                               const std::vector<uint8_t>& salt,
                                               const std::string& info,
                                               size_t output_length)
{
    auto k         = Impl::make_key(ikm, salt, info, output_length);
    size_t idx     = Impl::shard_index(k);
    Shard& shard   = impl_->shards[idx];

    std::lock_guard<std::mutex> lk(shard.mu);

    // Evict expired entries on each access (lazy eviction)
    shard.evict_expired(impl_->cfg.ttl);

    auto it = shard.map.find(k);
    if (it != shard.map.end()) {
        // Cache hit — move to LRU front
        shard.lru.splice(shard.lru.begin(), shard.lru, it->second.first);
        ++shard.hits;
        return it->second.second.value;
    }

    // Cache miss — derive and store (fail-closed on derivation error)
    ++shard.misses;
    std::vector<uint8_t> out;
    try {
        out = HKDFHelper::derive(ikm, salt, info, output_length);
    } catch (const std::exception& e) {
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::CRYPTO_KEY_DERIVATION_FAILED,
            "HKDF derivation failed on cache miss – key unavailable (fail-closed); "
            "output_length=" + std::to_string(output_length) + "; error=" + e.what(),
            "HKDFCache::derive_cached",
            themis::utils::ErrorSeverity::Critical,
            false);
        themis::utils::logErrorWithContext(ctx);
        throw;  // fail-closed: do not return default/empty key material
    }

    shard.lru.push_front(k);
    CacheEntry entry{out, std::chrono::steady_clock::now()};
    shard.map.emplace(k, std::make_pair(shard.lru.begin(), std::move(entry)));

    shard.evict_lru();
    return out;
}

void HKDFCache::purge_by_ikm_hash(const std::string& ikm_hash) {
    for (auto& shard : impl_->shards) {
        std::lock_guard<std::mutex> lk(shard.mu);
        for (auto it = shard.map.begin(); it != shard.map.end(); ) {
            // The key starts with the raw IKM bytes; compute its SHA-256 hex.
            const std::string& raw_key = it->first;
            // IKM ends at the first 0x00 separator.
            size_t ikm_end = raw_key.find('\x00');
            if (ikm_end == std::string::npos) {
              ikm_end = raw_key.size();
            }

            std::string candidate_hash = Impl::sha256_hex(
                reinterpret_cast<const uint8_t*>(raw_key.data()), ikm_end);

            if (candidate_hash == ikm_hash) {
                auto next = std::next(it);
                shard.erase_entry(it);
                it = next;
            } else {
                ++it;
            }
        }
    }
}

HKDFCache::Stats HKDFCache::stats() const {
    Stats s;
    for (const auto& shard : impl_->shards) {
        s.hits      += shard.hits.load(std::memory_order_relaxed);
        s.misses    += shard.misses.load(std::memory_order_relaxed);
        s.evictions += shard.evictions.load(std::memory_order_relaxed);
    }
    return s;
}

} // namespace utils
} // namespace themis
