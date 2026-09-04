/**
 * @file deduplication_checker.cpp
 * @brief Content deduplication engine using cryptographic hashing and similarity detection.
 * @version 0.0.15
 * @note Maturity: 🟡 BETA
 * @note Score: 80/100
 * @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=4, C=1, H=2, M=5, L=0
 * @note Status: Production Ready; Hash-based dedup working; advanced similarity detection deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/deduplication_checker.h"
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace themis {
namespace content {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DeduplicationChecker::DeduplicationChecker(
    std::shared_ptr<RocksDBWrapper> storage,
    size_t max_band_entries
)
    : storage_(std::move(storage))
{
    // 30-day TTL: band entries should outlive normal ingestion sessions to avoid
    // false negatives from TTL expiry.
    static constexpr int kBandCacheTTLDays = 30;
    cache::BoundedLRUCache::Config cfg;
    cfg.max_entries        = max_band_entries;
    cfg.ttl                = std::chrono::seconds{kBandCacheTTLDays * 24 * 3600};
    cfg.enable_statistics  = false;  // Avoid overhead for an internal index
    band_cache_ = std::make_unique<cache::BoundedLRUCache>(cfg);
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static uint64_t hexToU64(const std::string& hex) {
    uint64_t v = 0;
    size_t n = std::min(hex.size(), static_cast<size_t>(16));
    for (size_t i = 0; i < n; ++i) {
        v <<= 4;
        char c = hex[i];
        if (c >= '0' && c <= '9') {
          v |= static_cast<uint64_t>(c - '0');
        }
        else if (c >= 'a' && c <= 'f') v |= static_cast<uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= static_cast<uint64_t>(c - 'A' + 10);
    }
    return v;
}

/*static*/ uint32_t DeduplicationChecker::hammingDistance(
    const std::string& a,
    const std::string& b
) {
    uint64_t ha = hexToU64(a);
    uint64_t hb = hexToU64(b);
    uint64_t diff = ha ^ hb;
    // Portable popcount
    uint32_t count = 0;
    while (diff) {
        count += diff & 1u;
        diff >>= 1;
    }
    return count;
}

/*static*/ uint64_t DeduplicationChecker::bandHash(
    const std::vector<uint32_t>& sig,
    size_t band
) {
    // FNV-1a over the band's kBandRows uint32 values
    uint64_t hash = 14695981039346656037ULL;
    size_t start = band * kBandRows;
    size_t end   = std::min(start + kBandRows, sig.size());
    for (size_t i = start; i < end; ++i) {
        uint32_t v = sig[i];
        for (int b = 0; b < 4; ++b) {
            hash ^= static_cast<uint64_t>((v >> (b * 8)) & 0xFF);
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

/*static*/ std::string DeduplicationChecker::makeBandKey(size_t band, uint64_t hash_val) {
    std::ostringstream oss;
    oss << 'b' << band << ':' << std::hex << std::setw(16) << std::setfill('0') << hash_val;
    return oss.str();
}

// ---------------------------------------------------------------------------
// pHash — image deduplication
// ---------------------------------------------------------------------------

std::optional<DuplicateOf> DeduplicationChecker::isDuplicateImage(
    const std::string& phash_hex
) const {
    if (!storage_ || phash_hex.size() < 16) {
      return std::nullopt;
    }

    std::optional<DuplicateOf> result;

    // Scan all persisted pHash entries and compare Hamming distance.
    // In practice the pHash index remains small (one entry per ingested image)
    // so a prefix scan is efficient enough; a VP-tree would be used at scale.
    static const std::string kPrefix = "phash_idx:";
    storage_->scanPrefix(kPrefix, [&](std::string_view key, std::string_view value) -> bool {
        if (result) return false; // stop after first match

        std::string stored_hex(key.substr(kPrefix.size()));
        if (stored_hex.size() < 16) {
          return true;
        }

        uint32_t dist = hammingDistance(phash_hex, stored_hex);
        if (dist <= kPHashThreshold) {
            std::string cid(value);
            double sim = 1.0 - static_cast<double>(dist) / 64.0;
            result = DuplicateOf{cid, sim};
            return false; // stop scan
        }
        return true; // continue
    });

    return result;
}

void DeduplicationChecker::registerImage(
    const std::string& content_id,
    const std::string& phash_hex
) {
    if (!storage_ || phash_hex.empty() || content_id.empty()) {
      return;
    }
    std::string key = "phash_idx:" + phash_hex;
    storage_->put(key, content_id);
}

// ---------------------------------------------------------------------------
// MinHash band-LSH — text deduplication (backed by BoundedLRUCache)
// ---------------------------------------------------------------------------

std::optional<DuplicateOf> DeduplicationChecker::isDuplicateText(
    const std::vector<uint32_t>& minhash
) const {
    if (minhash.size() < kNumHashFunctions) {
      return std::nullopt;
    }

    for (size_t b = 0; b < kNumBands; ++b) {
        uint64_t bh  = bandHash(minhash, b);
        auto val = band_cache_->get(makeBandKey(b, bh));
        if (val) {
            // A band collision implies estimated Jaccard ≥ kJaccardThreshold
            // under the 16-band × 8-row configuration.
            if (!val->is_string()) return std::nullopt; // guard against corrupt cache entry
            return DuplicateOf{val->get<std::string>(), kJaccardThreshold};
        }
    }
    return std::nullopt;
}

void DeduplicationChecker::registerText(
    const std::string& content_id,
    const std::vector<uint32_t>& minhash
) {
    if (minhash.size() < kNumHashFunctions || content_id.empty()) {
      return;
    }

    for (size_t b = 0; b < kNumBands; ++b) {
        uint64_t bh = bandHash(minhash, b);
        // BoundedLRUCache handles capacity eviction (LRU) automatically.
        // Only store the first registrant per band slot (don't overwrite).
        std::string key = makeBandKey(b, bh);
        if (!band_cache_->get(key)) {
            band_cache_->put(key, nlohmann::json(content_id));
        }
    }
}

} // namespace content
} // namespace themis
