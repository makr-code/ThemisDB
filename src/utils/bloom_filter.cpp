/**
 * @file bloom_filter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/bloom_filter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <mutex>
#include <stdexcept>

namespace themis {
namespace utils {

// ---------------------------------------------------------------------------
// Construction helpers
// ---------------------------------------------------------------------------

// m = ceil( -n * ln(p) / (ln(2))^2 )
size_t BloomFilter::optimalBits(size_t n, double p) {
    if (n == 0) {
      return 64;
    }
    const double ln2 = std::log(2.0);
    return static_cast<size_t>(std::ceil(-static_cast<double>(n) * std::log(p) / (ln2 * ln2)));
}

// k = ceil( (m/n) * ln(2) )
size_t BloomFilter::optimalHashCount(size_t bits, size_t n) {
    if (n == 0) {
      return 1;
    }
    size_t k = static_cast<size_t>(std::ceil(static_cast<double>(bits) / static_cast<double>(n) * std::log(2.0)));
    return std::max<size_t>(1, k);
}

BloomFilter::BloomFilter(size_t expected_elements, double false_positive_rate)
    : fpr_(false_positive_rate)
{
    if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
        throw std::invalid_argument("BloomFilter: false_positive_rate must be in (0, 1)");
    }
    num_bits_   = optimalBits(expected_elements, false_positive_rate);
    num_hashes_ = optimalHashCount(num_bits_, expected_elements);
    bits_.assign(num_bits_, false);
}

// ---------------------------------------------------------------------------
// Hash functions
// ---------------------------------------------------------------------------

uint64_t BloomFilter::hash1(const std::string& key) {
    return std::hash<std::string>{}(key);
}

// Murmur-inspired finalizer mix on each byte accumulation
uint64_t BloomFilter::hash2(const std::string& key) {
    uint64_t h = 0x9e3779b97f4a7c15ULL; // golden-ratio constant
    for (unsigned char c : key) {
        h ^= static_cast<uint64_t>(c) * 0xff51afd7ed558ccdULL;
        h = (h << 31) | (h >> 33); // rotate left 31
        h *= 0xc4ceb9fe1a85ec53ULL;
    }
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    return h | 1; // ensure non-zero for double-hashing
}

size_t BloomFilter::probeIndex(const std::string& key, size_t i) const {
    // Enhanced double hashing: (h1 + i*h2) % m
    uint64_t h1 = hash1(key);
    uint64_t h2 = hash2(key);
    return static_cast<size_t>((h1 + i * h2) % static_cast<uint64_t>(num_bits_));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void BloomFilter::insert(const std::string& key) {
    std::unique_lock lock(mutex_);
    for (size_t i = 0; i < num_hashes_; ++i) {
        bits_[probeIndex(key, i)] = true;
    }
    ++approx_count_;
}

bool BloomFilter::contains(const std::string& key) const {
    std::shared_lock lock(mutex_);
    for (size_t i = 0; i < num_hashes_; ++i) {
        if (!bits_[probeIndex(key, i)]) {
            return false;
        }
    }
    return true;
}

void BloomFilter::clear() {
    std::unique_lock lock(mutex_);
    std::fill(bits_.begin(), bits_.end(), false);
    approx_count_ = 0;
}

size_t BloomFilter::size() const {
    std::shared_lock lock(mutex_);
    return approx_count_;
}

size_t BloomFilter::bitset_size() const {
    return num_bits_; // immutable after construction
}

double BloomFilter::false_positive_rate() const {
    return fpr_; // immutable after construction
}

} // namespace utils
} // namespace themis
