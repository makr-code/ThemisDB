/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_interfaces.h                                 ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:44:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     389                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d012eef80c  2026-03-10  feat(cache): implement 4 missing items from cache module ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file cache_interfaces.h
 * @brief Pluggable cache abstraction interfaces for the ThemisDB cache module.
 *
 * This header provides the following pure-virtual interfaces:
 *
 *   - IEvictionPolicy   — pluggable eviction strategy (LRU, LFU, ARC, …)
 *   - ICacheAdminOps    — privileged runtime inspection and management
 *   - ICacheWarmup      — batch pre-population from a WarmupSource
 *   - IGDPRPurgeHook    — synchronous, audited PII erasure (GDPR Art. 17)
 *   - ITTLAdapter       — workload-driven adaptive TTL computation
 *
 * Design constraints (from include/cache/FUTURE_ENHANCEMENTS.md):
 *   - All types are forward-declarable; no implementation headers are
 *     pulled in transitively.
 *   - ICacheAdminOps is only obtainable via an authenticated admin
 *     accessor; it is NOT exposed through IEvictionPolicy or ITTLAdapter.
 *   - IGDPRPurgeHook::purge() is synchronous; the audit-log entry is
 *     written before the method returns.
 *   - ITTLAdapter::computeTTL() must never exceed TTLAdapterConfig::maxTTL.
 *   - IEvictionPolicy callbacks receive only keys, never stored values.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include &lt;optional&gt;
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace cache {

// ============================================================================
// IEvictionPolicy — pluggable eviction strategy
// ============================================================================

/**
 * @brief Notification event type delivered to IEvictionPolicy callbacks.
 *
 * Receivers use this enum to distinguish access, insert, remove, and expiry
 * notifications.  The IEvictionPolicy implementation must not access stored
 * values; it receives only the key.
 */
enum class EvictionEventType : uint8_t {
    ACCESS,  ///< An existing entry was read (cache hit).
    INSERT,  ///< A new entry was inserted into the cache.
    REMOVE,  ///< An entry was explicitly removed or evicted.
    EXPIRY,  ///< An entry's TTL expired.
};

/**
 * @brief Describes a single eviction-relevant cache event.
 */
struct EvictionEvent {
    EvictionEventType type;  ///< What happened.
    std::string       key;   ///< Affected cache key (never empty).
};

/**
 * @brief Pure-virtual pluggable eviction policy interface.
 *
 * Thread-safety: callers must serialise access; implementations are not
 * required to be internally thread-safe.
 */
struct IEvictionPolicy {
    virtual ~IEvictionPolicy() = default;

    /// Notify the policy that @p key was accessed (cache hit).
    virtual void onAccess(const std::string& key) = 0;

    /// Notify the policy that @p key was inserted.
    virtual void onInsert(const std::string& key) = 0;

    /// Notify the policy that @p key was removed or expired.
    virtual void onRemove(const std::string& key) = 0;

    /**
     * @brief Choose and return the key that should be evicted next.
     *
     * The returned key must be present in the current live key set.
     * Behaviour when called on an empty key set is undefined.
     */
    [[nodiscard]] virtual std::string evict() = 0;

    /**
     * @brief Human-readable policy name for observability and admin display.
     *
     * Examples: "LRU", "LFU", "ARC".
     */
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

// ============================================================================
// ICacheAdminOps — privileged runtime inspection and management
// ============================================================================

/**
 * @brief Point-in-time snapshot of cache metrics.
 */
struct CacheStats {
    uint64_t hit_count      = 0;   ///< Total cache hits across all tiers.
    uint64_t miss_count     = 0;   ///< Total cache misses.
    uint64_t eviction_count = 0;   ///< Total entries evicted.
    size_t   current_size   = 0;   ///< Number of entries currently held.
    size_t   capacity       = 0;   ///< Maximum number of entries (0 = unlimited).
};

/**
 * @brief Predicate for selecting a subset of cache keys.
 *
 * At most one of the filter fields should be set; if multiple are set,
 * implementations may combine them with AND semantics.
 */
struct KeyFilter {
    std::optional<std::string> prefix;   ///< Match keys starting with this prefix.
    std::optional<std::string> pattern;  ///< Match keys against this regex pattern.
    /// Match keys whose remaining TTL is ≤ ttl_max_seconds (0 = no filter).
    uint32_t ttl_max_seconds = 0;
};

/**
 * @brief Privileged admin interface for runtime cache management.
 *
 * Implementations must only be obtainable via an authenticated admin
 * accessor; IEvictionPolicy and ITTLAdapter must NOT expose this interface.
 *
 * Thread-safety: all methods are thread-safe.
 */
struct ICacheAdminOps {
    virtual ~ICacheAdminOps() = default;

    /**
     * @brief Flush all entries from the cache.
     *
     * Blocking call; all in-flight get/put operations complete before
     * flush returns.  After this call, stats().current_size == 0.
     */
    virtual void flush() = 0;

    /**
     * @brief Collect and return a point-in-time metrics snapshot.
     *
     * Must complete in ≤ 100 µs regardless of cache size.
     */
    [[nodiscard]] virtual CacheStats stats() const = 0;

    /**
     * @brief Resize the cache to @p new_capacity maximum entries.
     *
     * If new_capacity < current_size, excess entries are evicted
     * using the active eviction policy before this call returns.
     */
    virtual void resize(size_t new_capacity) = 0;

    /**
     * @brief Return all keys matching @p filter.
     *
     * Returns an empty vector when no keys match or the cache is empty.
     */
    [[nodiscard]] virtual std::vector<std::string> listKeys(const KeyFilter& filter) const = 0;
};

// ============================================================================
// ICacheWarmup — batch pre-population
// ============================================================================

/**
 * @brief Typed cache entry used as input to a warmup source.
 *
 * @tparam K  Key type.
 * @tparam V  Value type.
 */
template<typename K, typename V>
struct CacheEntry {
    K        key;
    V        value;
    uint32_t ttl_seconds = 0;  ///< Remaining TTL; 0 means "use cache default".
    std::string tenant_id;     ///< Tenant namespace; empty = global.
};

/**
 * @brief Statistics collected during a warmup operation.
 */
struct WarmupStats {
    size_t   entries_inserted = 0;  ///< Entries successfully inserted.
    size_t   entries_skipped  = 0;  ///< Entries skipped (expired TTL, quota exceeded, …).
    uint64_t duration_ms      = 0;  ///< Wall-clock duration of the warmup call.
    size_t   error_count      = 0;  ///< Entries that caused an error during insertion.
};

/**
 * @brief Result returned by ICacheWarmup::warm().
 */
struct WarmupResult {
    bool        ok    = true;
    std::string error;     ///< Non-empty when ok == false.
    WarmupStats stats;
};

/**
 * @brief Pure-virtual source that provides entries to ICacheWarmup::warm().
 *
 * Implementations read from NDJSON logs, snapshots, or in-memory fixtures.
 */
struct IWarmupSource {
    virtual ~IWarmupSource() = default;

    /**
     * @brief Return the next batch of entries; empty vector signals EOF.
     *
     * Implementations should return a non-empty batch on every call until
     * the source is exhausted.
     */
    [[nodiscard]] virtual std::vector<CacheEntry<std::string, std::string>> nextBatch() = 0;
};

/**
 * @brief Interface for pre-populating a cache from an IWarmupSource.
 *
 * Callers use this interface at startup to warm in-memory tiers from a
 * query log or snapshot file so that the first queries after a restart
 * hit cache instead of the persistence tier or the compute path.
 *
 * Contract: warm() respects existing TTL values from the source and does
 * not override entries that are already live in the cache.
 */
struct ICacheWarmup {
    virtual ~ICacheWarmup() = default;

    /**
     * @brief Pre-populate the cache from @p source.
     *
     * @param source  Entry source; nextBatch() is called until it returns
     *                an empty vector.
     * @return WarmupResult describing inserted/skipped counts and duration.
     */
    [[nodiscard]] virtual WarmupResult warm(IWarmupSource& source) = 0;
};

// ============================================================================
// IGDPRPurgeHook — synchronous GDPR Art. 17 erasure
// ============================================================================

/**
 * @brief Reason for a GDPR purge request.
 */
enum class PurgeReason : uint8_t {
    RIGHT_TO_ERASURE,  ///< GDPR Art. 17 – data subject requested erasure.
    RETENTION_EXPIRED, ///< Retention period exceeded; data must be deleted.
    CONSENT_WITHDRAWN, ///< Data subject withdrew processing consent.
    OTHER,             ///< Any other reason; details in PurgeDescriptor::notes.
};

/**
 * @brief Structured descriptor for a GDPR purge request.
 *
 * Free-form string key patterns are rejected; only structured field values
 * are accepted to prevent injection attacks.
 */
struct PurgeDescriptor {
    std::string             subject_id;  ///< Data subject identifier (non-empty).
    std::vector<std::string> key_patterns; ///< Cache key regex patterns to purge.
    PurgeReason             reason = PurgeReason::RIGHT_TO_ERASURE;
    std::string             notes;       ///< Optional human-readable context.
};

/**
 * @brief Result of a GDPR purge operation.
 */
struct PurgeResult {
    size_t      purged_key_count  = 0;  ///< Number of cache keys removed.
    std::string audit_log_entry_id;     ///< ID of the audit-log entry written.
    int64_t     timestamp_utc_ms  = 0;  ///< Wall-clock time of the purge (ms since epoch).
};

/**
 * @brief Synchronous, audited PII erasure hook for GDPR Art. 17 compliance.
 *
 * purge() blocks until:
 *   1. All matching cache entries are removed from all tiers.
 *   2. The audit-log entry has been durably written.
 *
 * If the audit-log write fails, purge() must treat this as a fatal error
 * and throw; partial purge without an audit trail is not acceptable.
 *
 * Thread-safety: implementations must be thread-safe.
 */
struct IGDPRPurgeHook {
    virtual ~IGDPRPurgeHook() = default;

    /**
     * @brief Purge all cache entries matching @p descriptor.
     *
     * @param descriptor  Structured purge request.
     * @return PurgeResult with purged count, audit-log entry ID, and timestamp.
     * @throws std::runtime_error when the audit-log write fails or when
     *         descriptor.subject_id is empty.
     */
    [[nodiscard]] virtual PurgeResult purge(const PurgeDescriptor& descriptor) = 0;
};

// ============================================================================
// ITTLAdapter — workload-driven adaptive TTL computation
// ============================================================================

/**
 * @brief Access-pattern statistics used by ITTLAdapter::computeTTL().
 */
struct AccessPattern {
    uint64_t access_frequency = 0;   ///< Accesses per second (smoothed).
    uint64_t last_access_age_ms = 0; ///< Milliseconds since the last access.
    double   write_ratio = 0.0;      ///< Fraction of operations that are writes [0, 1].
};

/**
 * @brief Configuration bounds for ITTLAdapter.
 *
 * Hard contract: computeTTL() must never return a value exceeding maxTTL.
 */
struct TTLAdapterConfig {
    std::chrono::milliseconds minTTL{1'000};           ///< Lower bound.
    std::chrono::milliseconds maxTTL{3'600'000};        ///< Upper bound (hard cap).
    double aggressiveness = 1.0;  ///< Scaling factor; higher = faster TTL growth.
};

/**
 * @brief Pure-virtual interface for workload-driven TTL adaptation.
 *
 * computeTTL() is a stateless computation; implementations must not
 * cache state between calls.  The returned value is always clamped to
 * [TTLAdapterConfig::minTTL, TTLAdapterConfig::maxTTL].
 *
 * Thread-safety: computeTTL() and configure() must be thread-safe.
 */
struct ITTLAdapter {
    virtual ~ITTLAdapter() = default;

    /**
     * @brief Compute the adapted TTL for @p key given @p pattern.
     *
     * @param key      Cache key (used only for logging/observability; not
     *                 stored by the adapter).
     * @param pattern  Access-pattern statistics for this key.
     * @return Adapted TTL clamped to [config.minTTL, config.maxTTL].
     */
    [[nodiscard]] virtual std::chrono::milliseconds computeTTL(const std::string& key,
                                                 const AccessPattern& pattern) const = 0;

    /**
     * @brief Update the adapter bounds and aggressiveness.
     *
     * May be called at runtime to change adaptation parameters without
     * restarting the cache.
     */
    virtual void configure(const TTLAdapterConfig& config) = 0;
};

// ============================================================================
// ICacheBackend<K, V> — common read/write/management interface
// ============================================================================

/**
 * @brief Generic typed cache backend interface.
 *
 * Provides a minimal, uniform API for all cache implementations in the
 * ThemisDB cache module (AdaptiveQueryCache, BoundedLRUCache, EmbeddingCache,
 * SemanticCache, …).  New cache implementations MUST inherit from this
 * interface to ensure interoperability with cache-agnostic consumers such as
 * query optimisers, RAG pipelines, and the admin API.
 *
 * @tparam K  Key type.  Must be equality-comparable and hashable.
 * @tparam V  Value type.  Must be move-constructible.
 *
 * ### Contract
 *   - `get()` returns `std::nullopt` on a cache miss; it must never block
 *     indefinitely.
 *   - `put()` may silently evict existing entries when capacity is exhausted.
 *   - `remove()` is idempotent; returning false when the key is absent is
 *     acceptable.
 *   - `clear()` must complete in bounded time regardless of cache size.
 *   - All methods must be thread-safe.
 *
 * ### Relation to other interfaces
 *   - IEvictionPolicy handles the *which-to-evict* decision; ICacheBackend
 *     handles the *get/put/remove* operations.
 *   - ICacheAdminOps provides privileged management; obtain it through a
 *     separate authenticated accessor, not via ICacheBackend.
 *   - ITTLAdapter computes TTL values; ICacheBackend stores and honours them.
 */
template<typename K, typename V>
struct ICacheBackend {
    virtual ~ICacheBackend() = default;

    /**
     * @brief Look up @p key in the cache.
     *
     * @return The cached value, or `std::nullopt` on a miss.
     */
    virtual std::optional<V> get(const K& key) = 0;

    /**
     * @brief Insert or update @p key → @p value with optional TTL.
     *
     * @param key         Cache key (must not be empty for string keys).
     * @param value       Value to store.
     * @param ttl_seconds Time-to-live in seconds; 0 = use cache default.
     */
    virtual void put(const K& key, V value, uint32_t ttl_seconds = 0) = 0;

    /**
     * @brief Explicitly remove @p key from the cache.
     *
     * @return true  if the key existed and was removed.
     * @return false if the key was not present (idempotent).
     */
    virtual bool remove(const K& key) = 0;

    /**
     * @brief Return true if @p key is currently held in the cache.
     *
     * Must complete in O(1) amortised time.
     */
    virtual bool contains(const K& key) const = 0;

    /**
     * @brief Remove all entries from the cache.
     *
     * After this call, `size() == 0`.
     */
    virtual void clear() = 0;

    /**
     * @brief Return the number of entries currently held.
     *
     * The returned value is a snapshot; concurrent modifications may change
     * it immediately after the call returns.
     */
    virtual std::size_t size() const = 0;
};

} // namespace cache
} // namespace themis
