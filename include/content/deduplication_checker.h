/**
 * @file deduplication_checker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include "cache/bounded_lru_cache.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace content {

/**
 * @brief Result returned when a near-duplicate of an ingested item is found.
 */
struct DuplicateOf {
    std::string existing_id;  ///< Content ID of the existing near-duplicate
    double similarity;        ///< Estimated similarity in [0, 1]; 1 = identical
};

/**
 * @brief Perceptual near-duplicate detector for content items.
 *
 * Supports two hashing strategies:
 *  - **pHash** (images): DCT-based 64-bit perceptual hash stored in RocksDB.
 *    Near-duplicates are detected when the Hamming distance between hashes
 *    is ≤ kPHashThreshold (default: 10 bits out of 64).
 *  - **MinHash + band-LSH** (text): 128-permutation MinHash with 16 bands × 8
 *    rows.  The band index is backed by a `cache::BoundedLRUCache` for O(1)
 *    lookup and automatic LRU eviction when the capacity is reached.
 *    Near-duplicates are flagged when the estimated Jaccard similarity is
 *    ≥ kJaccardThreshold (default: 0.85).
 *
 * Deduplication must be enabled per-collection via `ContentPolicy::enable_deduplication`
 * before `ContentManager::ingestRawBlob()` consults this checker.
 *
 * Thread safety: all public methods are thread-safe.
 */
class DeduplicationChecker {
public:
    /// Number of MinHash permutations (hash functions).
    static constexpr size_t kNumHashFunctions = 128;
    /// Number of LSH bands.
    static constexpr size_t kNumBands = 16;
    /// Number of MinHash rows per band (kNumHashFunctions / kNumBands).
    static constexpr size_t kBandRows = 8;
    /// Maximum Hamming distance (in bits) for two pHashes to be near-duplicates.
    static constexpr uint32_t kPHashThreshold = 10;
    /// Minimum estimated Jaccard similarity for two MinHash signatures to be near-duplicates.
    static constexpr double kJaccardThreshold = 0.85;

    /**
     * @param storage          RocksDB wrapper used to persist pHash → content_id mappings.
     * @param max_band_entries Maximum number of MinHash band-index entries kept in the
     *                         `BoundedLRUCache` before LRU eviction kicks in.
     *                         With 16 bands per document, `max_band_entries / 16`
     *                         gives the approximate number of unique text documents
     *                         that can be held in the index simultaneously
     *                         (e.g. the default 200,000 ≈ 12,500 documents).
     */
    explicit DeduplicationChecker(
        std::shared_ptr<RocksDBWrapper> storage,
        size_t max_band_entries = 200'000
    );

    // Non-copyable, movable.
    DeduplicationChecker(const DeduplicationChecker&) = delete;
    DeduplicationChecker& operator=(const DeduplicationChecker&) = delete;

    /**
     * @brief Test whether an image is a near-duplicate of any already-indexed item.
     *
     * @param phash_hex  64-bit perceptual hash as a 16-character lowercase hex string
     *                   (as returned by `ImageProcessor::computePHash()`).
     * @return DuplicateOf if a near-duplicate exists; std::nullopt otherwise.
     */
    std::optional<DuplicateOf> isDuplicateImage(const std::string& phash_hex) const;

    /**
     * @brief Test whether a text document is a near-duplicate of any already-indexed item.
     *
     * @param minhash  128-element MinHash signature (as returned by
     *                 `TextProcessor::computeMinHash()`).
     * @return DuplicateOf if a near-duplicate exists; std::nullopt otherwise.
     */
    std::optional<DuplicateOf> isDuplicateText(const std::vector<uint32_t>& minhash) const;

    /**
     * @brief Register an image content item in the pHash index.
     *
     * Must be called after the item has been committed to storage so that a
     * subsequent `isDuplicateImage()` can return it.
     *
     * @param content_id  UUID of the stored content item.
     * @param phash_hex   Perceptual hash as returned by `ImageProcessor::computePHash()`.
     */
    void registerImage(const std::string& content_id, const std::string& phash_hex);

    /**
     * @brief Register a text content item in the MinHash band index.
     *
     * @param content_id  UUID of the stored content item.
     * @param minhash     128-element MinHash signature.
     */
    void registerText(const std::string& content_id, const std::vector<uint32_t>& minhash);

private:
    std::shared_ptr<RocksDBWrapper> storage_;

    // MinHash band-LSH index backed by BoundedLRUCache:
    //   key  = "b<band>:<hash_hex16>"
    //   value = JSON string holding the content_id of the first registered document
    //           that hashed to this band slot.
    // BoundedLRUCache provides O(1) lookup, thread safety, and automatic LRU
    // eviction when max_band_entries is reached.
    std::unique_ptr<cache::BoundedLRUCache> band_cache_;

    // Helpers
    static uint32_t hammingDistance(const std::string& a, const std::string& b);
    static uint64_t bandHash(const std::vector<uint32_t>& sig, size_t band);
    static std::string makeBandKey(size_t band, uint64_t hash_val);
};

} // namespace content
} // namespace themis
