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

enum class TierLevel {
    L1_WORKING,      ///< Cache: Working memory (register-like, <1µs latency)
    L2_EPISODIC,     ///< Cache: Episodic/result-set memory (1-10µs)
    L3_SEMANTIC,     ///< Cache: Semantic/RAG index memory (10-100µs)
    STORAGE_HOT,     ///< Storage: NVMe-backed tier (1-10ms)
    STORAGE_WARM,    ///< Storage: HDD-backed tier (10-100ms)
    STORAGE_COLD,    ///< Storage: Object-store tier (100ms-1s)
    UNKNOWN,         ///< Sentinel value: tier not yet determined or invalid
};

enum class TierClassification {
    CACHE,     ///< In-memory tier (L1/L2/L3)
    STORAGE,   ///< Persistent tier (hot/warm/cold)
};

/**
 * @brief Classify Tier.
 * @param[in] level Input parameter.
 * @return Return value.
 * @details Calls: THEMIS_UNREACHABLE().
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
 * @brief Tier Level Name.
 * @param[in] level Input parameter.
 * @return Return value.
 * @details Implements tierLevelName without additional internal calls.
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

struct TierAccessOptions {
    std::string correlation_id;

    std::optional<TierLevel> target_tier;

    std::chrono::milliseconds max_wait_ms{0};

    void* context = nullptr;

    std::function<void(bool success, const std::string& error)> on_complete;
};

struct TierGetResult {
    bool success = false;

    std::string error_message;

    std::string value = {};

    TierLevel current_tier;

    std::size_t size_bytes = 0;

    std::chrono::microseconds latency_us;

    uint64_t access_count = 0;

    std::chrono::seconds age_secs;
};

struct TierPutResult {
    bool success = false;

    std::string error_message;

    TierLevel placed_in_tier;

    std::size_t size_bytes = 0;

    std::chrono::microseconds latency_us;

    std::string correlation_id;
};

struct TierPromotionResult {
    bool success = false;

    std::string error_message;

    std::string promoted_value;

    TierLevel from_tier;

    TierLevel to_tier;

    std::chrono::milliseconds total_latency_ms;

    std::vector<std::chrono::milliseconds> hop_latencies_ms;

    std::string correlation_id;
};

// ============================================================================
// § 3  AccessTier Interface (Abstract)
// ============================================================================

class AccessTier {
public:
    /**
     * @brief Access Tier.
     * @return Return value.
     */
    virtual ~AccessTier() = default;


    /**
     * @brief Get.
     * @param[in] key Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    virtual TierGetResult get(std::string_view key, const TierAccessOptions& options) = 0;

    /**
     * @brief Put.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    virtual TierPutResult put(std::string_view key, std::string_view value,
                             const TierAccessOptions& options) = 0;

    /**
     * @brief Invalidate.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool invalidate(std::string_view key) = 0;


    /**
     * @brief Get Tier Level.
     * @return Return value.
     */
    virtual TierLevel getTierLevel() const = 0;

    /**
     * @brief Get Tier Name.
     * @return Return value.
     */
    virtual std::string getTierName() const = 0;

    /**
     * @brief Has Key.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool hasKey(std::string_view key) const = 0;

    /**
     * @brief Get Current Size Bytes.
     * @return Return value.
     */
    virtual std::size_t getCurrentSizeBytes() const = 0;

    /**
     * @brief Get Max Capacity Bytes.
     * @return Return value.
     */
    virtual std::size_t getMaxCapacityBytes() const = 0;

    /**
     * @brief Get Entry Count.
     * @return Return value.
     */
    virtual std::size_t getEntryCount() const = 0;


    /**
     * @brief Get Hit Rate.
     * @return Return value.
     */
    virtual double getHitRate() const = 0;

    /**
     * @brief Get Average Get Latency.
     * @return Return value.
     */
    virtual std::chrono::microseconds getAverageGetLatency() const = 0;

    /**
     * @brief Get Average Put Latency.
     * @return Return value.
     */
    virtual std::chrono::microseconds getAveragePutLatency() const = 0;

    /**
     * @brief Get Access Count.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    virtual uint64_t getAccessCount(std::string_view key) const = 0;

    /**
     * @brief Get Key Age.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    virtual std::chrono::seconds getKeyAge(std::string_view key) const = 0;


    virtual bool supportsPromotion() const { return false; }

    virtual bool supportsDemotion() const { return false; }

    virtual std::chrono::milliseconds estimatePromotionLatency(
        TierLevel from_tier, std::size_t data_size_bytes) const {
        (void)from_tier;
        (void)data_size_bytes;
        return std::chrono::milliseconds(-1);
    }


    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Is Healthy.
     * @return True when the operation succeeds.
     */
    virtual bool isHealthy() const = 0;
};

// ============================================================================
// § 4  Specialized Tier Interfaces
// ============================================================================

class CacheTier : public virtual AccessTier {
public:
    /**
     * @brief Notify Eviction.
     * @param[in] key Input parameter.
     * @param[in] size_bytes Input parameter.
     * @param[in] access_count Input parameter.
     */
    virtual void notifyEviction(std::string_view key, std::size_t size_bytes,
                               uint64_t access_count) = 0;
};

class StorageTier : public virtual AccessTier {
public:
    /**
     * @brief Notify Hot Access.
     * @param[in] key Input parameter.
     * @param[in] access_count Input parameter.
     * @param[in] access_window Input parameter.
     */
    virtual void notifyHotAccess(std::string_view key, uint64_t access_count,
                                std::chrono::seconds access_window) = 0;
};

// ============================================================================
// § 5  Constants
// ============================================================================

inline constexpr std::size_t kMaxTierCapacity = std::numeric_limits<std::size_t>::max();

inline constexpr TierLevel kNoTier = static_cast<TierLevel>(-1);

}  // namespace access_model
}  // namespace themis

