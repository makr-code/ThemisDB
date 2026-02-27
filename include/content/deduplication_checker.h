#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstdint>
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
 *    rows stored in an in-memory map.  Near-duplicates are flagged when the
 *    estimated Jaccard similarity is ≥ kJaccardThreshold (default: 0.85).
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
     * @param max_band_entries Maximum number of MinHash band-index entries kept
     *                         in memory before LRU eviction kicks in.
     */
    explicit DeduplicationChecker(
        std::shared_ptr<storage::RocksDBWrapper> storage,
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
    std::shared_ptr<storage::RocksDBWrapper> storage_;

    // In-memory MinHash band index: band_index_[band][band_hash] = content_id
    mutable std::mutex band_mutex_;
    std::unordered_map<uint64_t, std::string> band_index_[kNumBands];
    size_t band_entry_count_ = 0;
    size_t max_band_entries_;

    // Helpers
    static uint32_t hammingDistance(const std::string& a, const std::string& b);
    static uint64_t bandHash(const std::vector<uint32_t>& sig, size_t band);
};

} // namespace content
} // namespace themis
