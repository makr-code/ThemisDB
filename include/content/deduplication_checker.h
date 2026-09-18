/**
 * @file deduplication_checker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct DuplicateOf {
    std::string existing_id;  ///< Content ID of the existing near-duplicate
    double similarity;        ///< Estimated similarity in [0, 1]; 1 = identical
};

class DeduplicationChecker {
public:
    static constexpr size_t kNumHashFunctions = 128;
    static constexpr size_t kNumBands = 16;
    static constexpr size_t kBandRows = 8;
    static constexpr uint32_t kPHashThreshold = 10;
    static constexpr double kJaccardThreshold = 0.85;

    explicit DeduplicationChecker(
        std::shared_ptr<RocksDBWrapper> storage,
        size_t max_band_entries = 200'000
    );

    // Non-copyable, movable.
    DeduplicationChecker(const DeduplicationChecker&) = delete;
    DeduplicationChecker& operator=(const DeduplicationChecker&) = delete;

    /**
     * @brief Is Duplicate Image.
     * @param[in] phash_hex Input parameter.
     * @return Return value.
     */
    std::optional<DuplicateOf> isDuplicateImage(const std::string& phash_hex) const;

    /**
     * @brief Is Duplicate Text.
     * @param[in] minhash Input parameter.
     * @return Return value.
     */
    std::optional<DuplicateOf> isDuplicateText(const std::vector<uint32_t>& minhash) const;

    /**
     * @brief Register Image.
     * @param[in] content_id Identifier of the content.
     * @param[in] phash_hex Input parameter.
     */
    void registerImage(const std::string& content_id, const std::string& phash_hex);

    /**
     * @brief Register Text.
     * @param[in] content_id Identifier of the content.
     * @param[in] minhash Input parameter.
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
    /**
     * @brief Hamming Distance.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static uint32_t hammingDistance(const std::string& a, const std::string& b);
    /**
     * @brief Band Hash.
     * @param[in] sig Input parameter.
     * @param[in] band Input parameter.
     * @return Return value.
     */
    static uint64_t bandHash(const std::vector<uint32_t>& sig, size_t band);
    /**
     * @brief Make Band Key.
     * @param[in] band Input parameter.
     * @param[in] hash_val Input parameter.
     * @return Return value.
     */
    static std::string makeBandKey(size_t band, uint64_t hash_val);
};

} // namespace content
} // namespace themis
