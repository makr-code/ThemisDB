/**
 * @file hkdf_cache.h
 * @brief HKDF-based key derivation with caching and versioning.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Last Updated: 2026-08-08
 * @note Source: Level 0 - API Contract
 * @note SOT Domain: crypto-key-management
 * 
 * @page hkdf_cache_api HKDF Cache API
 *
 * ## Purpose
 * The HKDFCache provides deterministic, reproducible key derivation using
 * HMAC-based Key Derivation Function (HKDF) per RFC 5869. Keys are cached
 * with TTL-based eviction and LRU replacement across sharded thread-safe
 * buckets to reduce repeated cryptographic operations.
 *
 * ## Key Lifecycle
 * - **Generation**: Derive from input key material (IKM) using salt + info context
 * - **Caching**: Store derived keys with configurable TTL and capacity limits
 * - **Versioning**: Track key version via key_id to support rotation policies
 * - **Expiry**: Automatic cleanup of expired keys via TTL mechanism
 * - **Memory Safety**: All key material is zero-wiped via OPENSSL_cleanse on eviction
 *
 * ## Thread Safety
 * - All public methods are thread-safe
 * - Internal locking via sharded std::mutex for cache shards
 * - Stats are atomic across threads
 * - Thread-local instances available via threadLocal()
 *
 * ## Memory Management
 * - RAII-based resource cleanup (automatic destruction)
 * - Key material is always zero-wiped after eviction to prevent memory attacks
 * - Thread-local storage supported to reduce contention in high-concurrency scenarios
 *
 * ## Compliance
 * - Compliant with RFC 5869 (HKDF specification)
 * - Compatible with OpenSSL 3.0+ and OpenSSL 1.1.1+
 * - Constant-time operations where possible
 *
 * @see LEKManager for key rotation policies
 * @see KeyHandle for RAII-based key lifecycle management
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace utils {

/**
 * @brief HKDF Cache Configuration
 * 
 * Controls cache size, TTL-based eviction, and memory limits.
 * Configuration is applied per shard for better cache locality.
 */
struct HKDFCacheConfig {
    /// Maximum number of entries per shard (total ≈ max_entries)
    size_t max_entries = 1000;
    
    /// Time-to-live for cached keys; 0 = no expiry (not recommended)
    /// @warning Short TTL (< 60s) may impact performance
    /// @default 300 seconds (5 minutes)
    std::chrono::seconds ttl{300};
};

/**
 * @brief HKDF Cache Statistics
 * 
 * Cumulative performance metrics for cache performance analysis.
 * Useful for tuning TTL and capacity parameters.
 */
struct HKDFCacheStats {
    /// Number of successful cache lookups (key found)
    size_t hits      = 0;
    
    /// Number of cache misses (key not found, required re-derivation)
    size_t misses    = 0;
    
    /// Number of entries evicted due to capacity or TTL expiry
    size_t evictions = 0;
};

/** @brief Hkdf cache component. */
class HKDFCache {
public:
    // Expose config/stats types via aliases so call sites can use
    // HKDFCache::Config and HKDFCache::Stats as the spec requires.
    using Config = HKDFCacheConfig;
    using Stats  = HKDFCacheStats;

    // ---------------------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------------------

    /**
     * @brief Construct an HKDF cache with specified configuration.
     * 
     * Creates a new cache instance with the given configuration. The cache
     * is divided into 16 shards to reduce lock contention in multi-threaded
     * scenarios. Each shard independently manages LRU eviction.
     * 
     * @param cfg Cache configuration (capacity per shard, TTL)
     * @see HKDFCacheConfig
     * @note Thread-safe: Multiple threads can safely construct independent instances
     * @warning Do not share a cache instance across threads without external synchronization,
     *          unless using threadLocal() which returns a thread-local singleton
     */
    explicit HKDFCache(Config cfg = Config{});
    
    /**
     * @brief Destructor – cleans up all cached key material.
     * 
     * Securely wipes all cached key material using OPENSSL_cleanse before
     * releasing memory. This destructor is guaranteed to be called when
     * the instance is destroyed, even in exception scenarios (due to RAII).
     * 
     * @note Thread-safe: Safe to destroy from any thread
     * @warning Do not access cache after destructor has been called
     */
    ~HKDFCache();

    /**
     * @brief Get or create a thread-local cache instance.
     * 
     * Returns a reference to a thread-local cache singleton. Useful for
     * high-performance scenarios where per-thread caching reduces lock
     * contention. The thread-local cache is automatically created on first
     * access and destroyed when the thread exits.
     * 
     * @return Reference to thread-local HKDFCache (never null)
     * @thread_safe Yes - each thread gets its own instance
     * @note Default configuration: 1000 entries, 300-second TTL
     * 
     * @example
     * @code
     * // Preferred usage pattern in multi-threaded code
     * auto& cache = HKDFCache::threadLocal();
     * auto key = cache.derive_cached(ikm, salt, info, 32);
     * @endcode
     */
    static HKDFCache& threadLocal();

    // ---------------------------------------------------------------------------
    // Core API
    // ---------------------------------------------------------------------------

    /**
     * @brief Derive a key using HKDF with caching and TTL-based eviction.
     * 
     * Implements HKDF-SHA256 key derivation with transparent caching. If a key
     * with the same (ikm, salt, info, output_length) has been recently derived,
     * it is returned from cache. Otherwise, the key is derived using OpenSSL's
     * HKDF implementation, cached, and returned.
     * 
     * Cache keys are indexed by SHA-256 hash of the full parameter tuple to
     * ensure deterministic cache behavior while keeping storage overhead bounded.
     * 
     * @param ikm Input Key Material – cryptographically strong secret
     *            (e.g., output of KDF, random entropy, or master key)
     * @param salt Salt value – recommended but can be empty; used in HKDF-Extract
     *             (at least 16 bytes recommended per RFC 5869)
     * @param info Application/context-specific info string
     *             (e.g., "auth.session.key" or "encryption.data")
     * @param output_length Desired output key length in bytes
     *                     Valid range: 1-255*32 (per RFC 5869 SHA-256 limit)
     * 
     * @return Derived key material of length output_length bytes
     * @throws std::invalid_argument if output_length exceeds maximum
     * @throws std::runtime_error if OpenSSL HKDF fails
     * 
     * @note Memory Safety: Key material is zero-wiped when evicted from cache
     * @note Thread-Safe: All parameters and return value are thread-safe
     * @warning Caller owns the returned vector; use volatile_free() for sensitive cleanup
     * 
     * @see RFC 5869 for HKDF specification
     * @see purge_by_ikm_hash() for selective cache invalidation
     * 
     * @example
     * @code{.cpp}
     * HKDFCache cache;
     * std::vector<uint8_t> master_key = ...;
     * auto session_key = cache.derive_cached(master_key, salt, "auth.session", 32);
     * // Use session_key...
     * OPENSSL_cleanse(session_key.data(), session_key.size());
     * @endcode
     */
    std::vector<uint8_t> derive_cached(const std::vector<uint8_t>& ikm,
                                       const std::vector<uint8_t>& salt,
                                       const std::string& info,
                                       size_t output_length);

    /**
     * @brief Clear all entries from all cache shards.
     * 
     * Securely wipes and removes all cached key material. Useful for:
     * - Clearing cache after key rotation
     * - Emergency cache reset
     * - Testing and validation
     * 
     * This operation:
     * 1. Acquires locks on all cache shards (potential serialization point)
     * 2. Wipes all key material using OPENSSL_cleanse
     * 3. Resets hit/miss/eviction counters
     * 
     * @thread_safe Yes - internally synchronized
     * @note Performance: O(n) where n is number of cached entries
     * @warning Calling this frequently may impact performance; prefer TTL-based eviction
     * 
     * @example
     * @code
     * // After master key rotation, clear stale derived keys
     * cache.clear();
     * // Fresh keys will be derived on next access
     * @endcode
     */
    void clear();

    /**
     * @brief Reconfigure cache capacity (per shard).
     * 
     * Adjusts the maximum number of entries stored per cache shard. Total
     * cache size is approximately capacity * number_of_shards (16).
     * 
     * If new capacity is smaller than current entries, LRU entries are
     * evicted until the cache fits the new size.
     * 
     * @param cap Maximum entries per shard (recommended: 50-1000)
     * @thread_safe Yes - internally synchronized
     * @note Changes take effect immediately
     * @warning Setting capacity to 0 effectively disables caching
     * 
     * @example
     * @code
     * cache.setCapacity(500); // ~8000 total entries (500 * 16 shards)
     * @endcode
     */
    void setCapacity(size_t cap);

    // ---------------------------------------------------------------------------
    // Extended API (Operational and Lifecycle Management)
    // ---------------------------------------------------------------------------

    /**
     * @brief Purge all entries derived from a specific Input Key Material (IKM).
     *
     * Enables selective cache invalidation when a root key (IKM) is rotated.
     * For example, after rotating a master encryption key, all keys derived
     * from it can be invalidated via this method, forcing re-derivation
     * with the new master key.
     *
     * Implementation:
     * 1. Scans all cache shards for entries whose IKM matches the given hash
     * 2. Securely wipes matching entries using OPENSSL_cleanse
     * 3. Updates eviction counters
     *
     * @param ikm_hash SHA-256 hash of the IKM bytes, as hex string (lowercase)
     *                 Example: "abc123def456..."
     * @return Number of entries purged
     * 
     * @thread_safe Yes - internally synchronized across all shards
     * @note Performance: O(n) in worst case; consider calling during maintenance window
     * 
     * Usage Scenario:
     * @code{.cpp}
     * // Master key rotation workflow
     * std::vector<uint8_t> old_master = ...;
     * std::string old_master_hash = sha256_hex(old_master);
     * cache.purge_by_ikm_hash(old_master_hash);
     * 
     * // New keys will now be derived from new master
     * auto new_key = cache.derive_cached(new_master, salt, info, 32);
     * @endcode
     *
     * @see stats() for cache efficiency monitoring
     */
    void purge_by_ikm_hash(const std::string& ikm_hash);

    /**
     * @brief Retrieve cumulative cache performance statistics.
     *
     * Returns snapshot of hit/miss/eviction counters accumulated since
     * cache creation or last clear(). Useful for:
     * - Cache hit rate analysis (hits / (hits + misses))
     * - Performance tuning (TTL, capacity adjustments)
     * - Operational monitoring
     *
     * @return HKDFCacheStats with current hit/miss/eviction counts
     * @thread_safe Yes - uses atomic counters
     * 
     * @example
     * @code
     * auto stats = cache.stats();
     * double hit_rate = 100.0 * stats.hits / (stats.hits + stats.misses);
     * LOG(INFO) << "Cache hit rate: " << hit_rate << "%";
     * @endcode
     */
    Stats stats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace utils
} // namespace themis
