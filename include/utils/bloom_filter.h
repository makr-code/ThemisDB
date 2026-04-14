/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bloom_filter.h                                     ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 06:58:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     110                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15a0bb6700  2026-03-09  feat(utils): add BloomFilter, ConsistentHashRing, RateLim... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themis {
namespace utils {

/**
 * @brief Probabilistic set membership data structure (Bloom filter).
 *
 * Guarantees zero false negatives. False positive rate is bounded by the
 * configured rate supplied at construction time. Thread-safe via
 * std::shared_mutex (concurrent readers, exclusive writers).
 *
 * Hash strategy:
 *  h1 = std::hash<std::string>
 *  h2 = murmur-style mix (fnv-style rotate-xor)
 *  hash_i(k) = (h1(k) + i * h2(k)) % m      (double-hashing)
 */
class BloomFilter {
public:
    /**
     * @brief Construct a Bloom filter.
     * @param expected_elements  Anticipated number of distinct insertions.
     * @param false_positive_rate  Desired max false-positive probability (0 < p < 1).
     */
    BloomFilter(size_t expected_elements, double false_positive_rate);

    /**
     * @brief Insert a key into the filter.
     */
    void insert(const std::string& key);

    /**
     * @brief Test membership.
     * @return false → definitely not in set. true → probably in set.
     */
    bool contains(const std::string& key) const;

    /**
     * @brief Reset all bits to zero and the approximate element counter.
     */
    void clear();

    /**
     * @brief Approximate number of elements inserted since last clear().
     */
    size_t size() const;

    /**
     * @brief Total number of bits in the underlying bit array.
     */
    size_t bitset_size() const;

    /**
     * @brief Configured target false-positive rate.
     */
    double false_positive_rate() const;

private:
    /// Compute optimal bit-count from expected_n and target fpr.
    static size_t optimalBits(size_t n, double p);
    /// Compute optimal hash-function count.
    static size_t optimalHashCount(size_t bits, size_t n);

    /// Double-hashing: return the i-th probe index for `key`.
    size_t probeIndex(const std::string& key, size_t i) const;

    /// First hash (std::hash).
    static uint64_t hash1(const std::string& key);
    /// Second independent hash (murmur-style mix).
    static uint64_t hash2(const std::string& key);

    size_t       num_bits_;
    size_t       num_hashes_;
    double       fpr_;
    size_t       approx_count_{0};

    mutable std::shared_mutex mutex_;
    std::vector<bool>         bits_;
};

} // namespace utils
} // namespace themis
