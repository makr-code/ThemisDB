/**
 * @file access_tier_interface.h
 * @brief Base interface for unified access tier abstraction (cache & storage).
 *
 * ThemisDB | File: access_tier_interface.h | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 1 API Definition) | Status: Frozen for v1.x
 * Author: Copilot | Date: 2026-08-03
 *
 * This header defines the abstract `AccessTier` contract that both cache and
 * storage tiers implement. It enables the `AccessCoordinator` to manage
 * promotion/demotion transitions uniformly without direct cache↔storage coupling.
 *
 * @see include/access_model/access_coordinator.h
 * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Cross-platform unreachable macro
#if defined(__GNUC__) || defined(__clang__)
#define THEMIS_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define THEMIS_UNREACHABLE() __assume(false)
#else
#define THEMIS_UNREACHABLE() ((void)0)
#endif

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Tier Identity & Classification
// ============================================================================

/**
 * @brief Unique identifier for a tier in the access hierarchy.
 *
 * Used by coordinator to route promotion/demotion decisions and metrics
 * collection uniformly across cache and storage tiers.
 */
enum class TierLevel {
    L1_WORKING,      ///< Cache: Working memory (register-like, <1µs latency)
    L2_EPISODIC,     ///< Cache: Episodic/result-set memory (1-10µs)
    L3_SEMANTIC,     ///< Cache: Semantic/RAG index memory (10-100µs)
    STORAGE_HOT,     ///< Storage: NVMe-backed tier (1-10ms)
    STORAGE_WARM,    ///< Storage: HDD-backed tier (10-100ms)
    STORAGE_COLD,    ///< Storage: Object-store tier (100ms-1s)
    UNKNOWN,         ///< Sentinel value: tier not yet determined or invalid
};

/**
 * @brief Tier classification: memory-based vs. persistent.
 */
enum class TierClassification {
    CACHE,     ///< In-memory tier (L1/L2/L3)
    STORAGE,   ///< Persistent tier (hot/warm/cold)
};

/**
 * @brief Get tier classification from level.
 */
constexpr TierClassification classifyTier(TierLevel level) {
    switch (level) {
        case TierLevel::L1_WORKING:
        case TierLevel::L2_EPISODIC:
        case TierLevel::L3_SEMANTIC:
            return TierClassification::CACHE;
        case TierLevel::STORAGE_HOT:
        case TierLevel::STORAGE_WARM:
        case TierLevel::STORAGE_COLD:
        case TierLevel::UNKNOWN:
            return TierClassification::STORAGE;
    }
    THEMIS_UNREACHABLE();
}

/**
 * @brief String representation of tier level.
 */
constexpr std::string_view tierLevelName(TierLevel level) {
    switch (level) {
        case TierLevel::L1_WORKING:
            return "L1_WORKING";
        case TierLevel::L2_EPISODIC:
            return "L2_EPISODIC";
        case TierLevel::L3_SEMANTIC:
            return "L3_SEMANTIC";
        case TierLevel::STORAGE_HOT:
            return "STORAGE_HOT";
        case TierLevel::STORAGE_WARM:
            return "STORAGE_WARM";
        case TierLevel::STORAGE_COLD:
            return "STORAGE_COLD";
        case TierLevel::UNKNOWN:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

// ============================================================================
// § 2  Data Transfer Options & Results
// ============================================================================

/**
 * @brief Options for get/put/promote/demote operations on a tier.
 */
struct TierAccessOptions {
    /// Correlation ID for tracing (generated if empty)
    std::string correlation_id;

    /// Target tier hint (for routing promotion/demotion)
    std::optional<TierLevel> target_tier;

    /// Maximum wait time for async operations (0 = async fire-and-forget)
    std::chrono::milliseconds max_wait_ms{0};

    /// User-provided context (opaque, passed to callbacks)
    void* context = nullptr;

    /// Callback on operation completion (for async operations)
    std::function<void(bool success, const std::string& error)> on_complete;
};

/**
 * @brief Result of a get operation from a tier.
 */
struct TierGetResult {
    /// Operation succeeded
    bool success = false;

    /// Error message (if success=false)
    std::string error_message;

    /// Data retrieved (owned by caller after return)
    std::string value = {};

    /// Current tier where data resides
    TierLevel current_tier;

    /// Size of retrieved value in bytes
    std::size_t size_bytes = 0;

    /// Latency in microseconds for this operation
    std::chrono::microseconds latency_us;

    /// Access count for this key
    uint64_t access_count = 0;

    /// Age of data in this tier (seconds since write)
    std::chrono::seconds age_secs;
};

/**
 * @brief Result of a put operation on a tier.
 */
struct TierPutResult {
    /// Operation succeeded
    bool success = false;

    /// Error message (if success=false)
    std::string error_message;

    /// Tier where data was placed (may differ from request if tier was full)
    TierLevel placed_in_tier;

    /// Size stored in bytes
    std::size_t size_bytes = 0;

    /// Latency in microseconds
    std::chrono::microseconds latency_us;

    /// Correlation ID for tracing
    std::string correlation_id;
};

/**
 * @brief Result of a promotion operation (from lower to higher tier).
 */
struct TierPromotionResult {
    /// Operation succeeded
    bool success = false;

    /// Error message (if success=false)
    std::string error_message;

    /// Data that was promoted (if success=true)
    std::string promoted_value;

    /// Source tier (where data was promoted from)
    TierLevel from_tier;

    /// Destination tier (where data was promoted to)
    TierLevel to_tier;

    /// Total latency for promotion (end-to-end, in ms)
    std::chrono::milliseconds total_latency_ms;

    /// Per-hop latencies in ms (if multi-hop promotion)
    std::vector<std::chrono::milliseconds> hop_latencies_ms;

    /// Correlation ID for tracing
    std::string correlation_id;
};

// ============================================================================
// § 3  AccessTier Interface (Abstract)
// ============================================================================

/**
 * @brief Abstract interface for a single tier in the access hierarchy.
 *
 * Both cache and storage tiers implement this interface, enabling the
 * `AccessCoordinator` to manage promotion/demotion uniformly.
 *
 * **Invariants:**
 * - All operations are thread-safe
 * - get() returns data if key exists; empty result if key not found (not an error)
 * - put() always succeeds unless capacity exceeded or I/O error occurs
 * - Errors are explicit (no silent failures)
 */
class AccessTier {
public:
    virtual ~AccessTier() = default;

    /// ────────────────────────────────────────────────────────────────────
    /// Core Operations
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Get data from this tier by key.
     *
     * @param key Data key
     * @param options Access options (correlation ID, timeout, callback)
     * @return Get result (includes success, error, value, latency)
     *
     * **Thread Safety:** Yes
     * **Blocking:** Depends on implementation (cache: <1µs, storage: 1-500ms)
     * **Async:** If options.max_wait_ms == 0, returns immediately with pending status
     */
    virtual TierGetResult get(std::string_view key, const TierAccessOptions& options) = 0;

    /**
     * @brief Put data into this tier by key.
     *
     * @param key Data key
     * @param value Data value (caller retains ownership; copied by tier)
     * @param options Access options
     * @return Put result (includes success, tier placement, latency)
     *
     * **Thread Safety:** Yes
     * **Blocking:** Implementation-dependent
     * **Capacity:** Returns error if tier is full and no eviction candidate found
     */
    virtual TierPutResult put(std::string_view key, std::string_view value,
                             const TierAccessOptions& options) = 0;

    /**
     * @brief Invalidate (remove) data from this tier by key.
     *
     * @param key Data key to remove
     * @return True if key existed and was removed; false if key not found
     *
     * **Thread Safety:** Yes
     * **Blocking:** Implementation-dependent
     */
    virtual bool invalidate(std::string_view key) = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Tier Information
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Get the tier level this instance represents.
     */
    virtual TierLevel getTierLevel() const = 0;

    /**
     * @brief Get human-readable tier name.
     */
    virtual std::string getTierName() const = 0;

    /**
     * @brief Check if a key exists in this tier.
     */
    virtual bool hasKey(std::string_view key) const = 0;

    /**
     * @brief Get current size in bytes of all data in this tier.
     */
    virtual std::size_t getCurrentSizeBytes() const = 0;

    /**
     * @brief Get maximum capacity in bytes for this tier.
     *
     * Return 0 or kMaxTierCapacity for unlimited capacity.
     */
    virtual std::size_t getMaxCapacityBytes() const = 0;

    /**
     * @brief Get number of entries in this tier.
     */
    virtual std::size_t getEntryCount() const = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Tier Metrics & Statistics
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Get hit rate for this tier (0.0 to 1.0).
     *
     * @return Hit rate, or -1.0 if metrics not available
     */
    virtual double getHitRate() const = 0;

    /**
     * @brief Get average latency for get operations in microseconds.
     */
    virtual std::chrono::microseconds getAverageGetLatency() const = 0;

    /**
     * @brief Get average latency for put operations in microseconds.
     */
    virtual std::chrono::microseconds getAveragePutLatency() const = 0;

    /**
     * @brief Get access count for a specific key.
     *
     * @return Access count, or 0 if key not found or not tracked
     */
    virtual uint64_t getAccessCount(std::string_view key) const = 0;

    /**
     * @brief Get age of a key in this tier (time since write).
     *
     * @return Age in seconds, or -1 if key not found or not tracked
     */
    virtual std::chrono::seconds getKeyAge(std::string_view key) const = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Promotion/Demotion Support (Optional)
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Check if this tier supports promotion from lower tiers.
     *
     * Cache tiers typically return true; storage tiers may return false.
     */
    virtual bool supportsPromotion() const { return false; }

    /**
     * @brief Check if this tier supports demotion to lower tiers.
     *
     * Storage tiers typically return true; cache tiers may return false.
     */
    virtual bool supportsDemotion() const { return false; }

    /**
     * @brief Estimate time to promote data from a lower tier to this tier.
     *
     * @param from_tier Source tier
     * @param data_size_bytes Size of data to promote
     * @return Estimated latency, or -1 ms if not promotable
     */
    virtual std::chrono::milliseconds estimatePromotionLatency(
        TierLevel from_tier, std::size_t data_size_bytes) const {
        (void)from_tier;
        (void)data_size_bytes;
        return std::chrono::milliseconds(-1);
    }

    /// ────────────────────────────────────────────────────────────────────
    /// Tier Lifecycle
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Initialize this tier (create storage, start workers, etc.).
     *
     * @return True if initialization succeeded
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown this tier gracefully.
     *
     * Flushes pending data, stops workers, closes resources.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if this tier is currently healthy (operational).
     */
    virtual bool isHealthy() const = 0;
};

// ============================================================================
// § 4  Specialized Tier Interfaces
// ============================================================================

/**
 * @brief Interface for cache-tier-specific behavior.
 *
 * Cache tiers may implement specialized eviction, warmup, and prefetch logic.
 */
class CacheTier : public virtual AccessTier {
public:
    /**
     * @brief Called when this tier is about to evict a key due to capacity.
     *
     * Implementations should notify the `AccessCoordinator` to consider
     * promoting data to a higher-tier storage level.
     */
    virtual void notifyEviction(std::string_view key, std::size_t size_bytes,
                               uint64_t access_count) = 0;
};

/**
 * @brief Interface for storage-tier-specific behavior.
 *
 * Storage tiers implement durable persistence, tiering, and redundancy logic.
 */
class StorageTier : public virtual AccessTier {
public:
    /**
     * @brief Called when this tier detects hot access patterns.
     *
     * Implementations should notify the `AccessCoordinator` to consider
     * promoting data to a higher-tier cache level.
     */
    virtual void notifyHotAccess(std::string_view key, uint64_t access_count,
                                std::chrono::seconds access_window) = 0;
};

// ============================================================================
// § 5  Constants
// ============================================================================

/// Constant representing unlimited tier capacity
inline constexpr std::size_t kMaxTierCapacity = std::numeric_limits<std::size_t>::max();

/// Constant representing no tier (invalid)
inline constexpr TierLevel kNoTier = static_cast<TierLevel>(-1);

}  // namespace access_model
}  // namespace themis

