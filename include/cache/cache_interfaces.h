/**
 * @file cache_interfaces.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace cache {

// ============================================================================
// IEvictionPolicy — pluggable eviction strategy
// ============================================================================

enum class EvictionEventType : uint8_t {
    ACCESS,  ///< An existing entry was read (cache hit).
    INSERT,  ///< A new entry was inserted into the cache.
    REMOVE,  ///< An entry was explicitly removed or evicted.
    EXPIRY,  ///< An entry's TTL expired.
};

struct EvictionEvent {
    EvictionEventType type;  ///< What happened.
    std::string       key;   ///< Affected cache key (never empty).
};

struct IEvictionPolicy {
    /**
     * @brief IEviction Policy.
     * @return Return value.
     */
    virtual ~IEvictionPolicy() = default;

    /**
     * @brief On Access.
     * @param[in] key Input parameter.
     */
    virtual void onAccess(const std::string& key) = 0;

    /**
     * @brief On Insert.
     * @param[in] key Input parameter.
     */
    virtual void onInsert(const std::string& key) = 0;

    /**
     * @brief On Remove.
     * @param[in] key Input parameter.
     */
    virtual void onRemove(const std::string& key) = 0;

    [[nodiscard]] virtual std::string evict() = 0;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

// ============================================================================
// ICacheAdminOps — privileged runtime inspection and management
// ============================================================================

struct CacheStats {
    uint64_t hit_count      = 0;   ///< Total cache hits across all tiers.
    uint64_t miss_count     = 0;   ///< Total cache misses.
    uint64_t eviction_count = 0;   ///< Total entries evicted.
    size_t   current_size   = 0;   ///< Number of entries currently held.
    size_t   capacity       = 0;   ///< Maximum number of entries (0 = unlimited).
};

struct KeyFilter {
    std::optional<std::string> prefix;   ///< Match keys starting with this prefix.
    std::optional<std::string> pattern;  ///< Match keys against this regex pattern.
    uint32_t ttl_max_seconds = 0;
};

struct ICacheAdminOps {
    /**
     * @brief ICache Admin Ops.
     * @return Return value.
     */
    virtual ~ICacheAdminOps() = default;

    /**
     * @brief Flush.
     */
    virtual void flush() = 0;

    [[nodiscard]] virtual CacheStats stats() const = 0;

    /**
     * @brief Resize.
     * @param[in] new_capacity Input parameter.
     */
    virtual void resize(size_t new_capacity) = 0;

    [[nodiscard]] virtual std::vector<std::string> listKeys(const KeyFilter& filter) const = 0;
};

// ============================================================================
// ICacheWarmup — batch pre-population
// ============================================================================

template<typename K, typename V>
struct CacheEntry {
    K        key;
    V        value;
    uint32_t ttl_seconds = 0;  ///< Remaining TTL; 0 means "use cache default".
    std::string tenant_id;     ///< Tenant namespace; empty = global.
};

struct WarmupStats {
    size_t   entries_inserted = 0;  ///< Entries successfully inserted.
    size_t   entries_skipped  = 0;  ///< Entries skipped (expired TTL, quota exceeded, …).
    uint64_t duration_ms      = 0;  ///< Wall-clock duration of the warmup call.
    size_t   error_count      = 0;  ///< Entries that caused an error during insertion.
};

struct WarmupResult {
    bool        ok    = true;
    std::string error;     ///< Non-empty when ok == false.
    WarmupStats stats;
};

struct IWarmupSource {
    /**
     * @brief IWarmup Source.
     * @return Return value.
     */
    virtual ~IWarmupSource() = default;

    [[nodiscard]] virtual std::vector<CacheEntry<std::string, std::string>> nextBatch() = 0;
};

struct ICacheWarmup {
    /**
     * @brief ICache Warmup.
     * @return Return value.
     */
    virtual ~ICacheWarmup() = default;

    [[nodiscard]] virtual WarmupResult warm(IWarmupSource& source) = 0;
};

// ============================================================================
// IGDPRPurgeHook — synchronous GDPR Art. 17 erasure
// ============================================================================

enum class PurgeReason : uint8_t {
    RIGHT_TO_ERASURE,  ///< GDPR Art. 17 – data subject requested erasure.
    RETENTION_EXPIRED, ///< Retention period exceeded; data must be deleted.
    CONSENT_WITHDRAWN, ///< Data subject withdrew processing consent.
    OTHER,             ///< Any other reason; details in PurgeDescriptor::notes.
};

struct PurgeDescriptor {
    std::string             subject_id;  ///< Data subject identifier (non-empty).
    std::vector<std::string> key_patterns; ///< Cache key regex patterns to purge.
    PurgeReason             reason = PurgeReason::RIGHT_TO_ERASURE;
    std::string             notes;       ///< Optional human-readable context.
};

struct PurgeResult {
    size_t      purged_key_count  = 0;  ///< Number of cache keys removed.
    std::string audit_log_entry_id;     ///< ID of the audit-log entry written.
    int64_t     timestamp_utc_ms  = 0;  ///< Wall-clock time of the purge (ms since epoch).
};

struct IGDPRPurgeHook {
    /**
     * @brief IGDPRPurge Hook.
     * @return Return value.
     */
    virtual ~IGDPRPurgeHook() = default;

    [[nodiscard]] virtual PurgeResult purge(const PurgeDescriptor& descriptor) = 0;
};

// ============================================================================
// ITTLAdapter — workload-driven adaptive TTL computation
// ============================================================================

struct AccessPattern {
    uint64_t access_frequency = 0;   ///< Accesses per second (smoothed).
    uint64_t last_access_age_ms = 0; ///< Milliseconds since the last access.
    double   write_ratio = 0.0;      ///< Fraction of operations that are writes [0, 1].
};

struct TTLAdapterConfig {
    std::chrono::milliseconds minTTL{1'000};           ///< Lower bound.
    std::chrono::milliseconds maxTTL{3'600'000};        ///< Upper bound (hard cap).
    double aggressiveness = 1.0;  ///< Scaling factor; higher = faster TTL growth.
};

struct ITTLAdapter {
    /**
     * @brief ITTLAdapter.
     * @return Return value.
     */
    virtual ~ITTLAdapter() = default;

    [[nodiscard]] virtual std::chrono::milliseconds computeTTL(const std::string& key,
                                                 const AccessPattern& pattern) const = 0;

    /**
     * @brief Configure.
     * @param[in] config Input parameter.
     */
    virtual void configure(const TTLAdapterConfig& config) = 0;
};

// ============================================================================
// ICacheBackend<K, V> — common read/write/management interface
// ============================================================================

template<typename K, typename V>
struct ICacheBackend {
    /**
     * @brief ICache Backend.
     * @return Return value.
     */
    virtual ~ICacheBackend() = default;

    /**
     * @brief Get.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    virtual std::optional<V> get(const K& key) = 0;

    virtual void put(const K& key, V value, uint32_t ttl_seconds = 0) = 0;

    /**
     * @brief Remove.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool remove(const K& key) = 0;

    /**
     * @brief Contains.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool contains(const K& key) const = 0;

    /**
     * @brief Clear.
     */
    virtual void clear() = 0;

    /**
     * @brief Size.
     * @return Return value.
     */
    virtual std::size_t size() const = 0;
};

} // namespace cache
} // namespace themis
