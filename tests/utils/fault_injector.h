/**
 * @file fault_injector.h
 * @brief Comprehensive fault injection framework for chaos testing
 * 
 * Provides reusable fault injection primitives for testing system resilience:
 * - Crash injection (coordinator, shard, replica)
 * - Network faults (partition, delay, packet loss)
 * - Timeout injection
 * - Data corruption
 * - Memory faults
 * 
 * All fault injectors are:
 * - Deterministic (seeded RNG for reproducibility)
 * - Thread-safe
 * - Time-aware (for simulating duration-based faults)
 * - Logging-enabled (for debugging)
 * 
 * Date: 2026-08-16
 * Wave A Batch A-9: Chaos Testing & Fault Injection
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace themis {
namespace test {

// ============================================================================
// FAULT INJECTOR BASE CLASS
// ============================================================================

/**
 * @brief Abstract base class for fault injection
 */
class FaultInjector {
public:
    enum class FaultType {
        NONE = 0,
        CRASH,
        NETWORK_PARTITION,
        NETWORK_DELAY,
        NETWORK_PACKET_LOSS,
        TIMEOUT,
        CORRUPTION,
        MEMORY_LIMIT,
        DISK_FULL
    };

    enum class InjectionState {
        INACTIVE,
        PENDING,
        ACTIVE,
        RECOVERING,
        RECOVERED
    };

    struct InjectionConfig {
        FaultType type;
        std::string target_component;
        std::chrono::milliseconds duration{0};  // 0 = permanent
        float probability{1.0f};                 // 0.0 - 1.0 (for random)
        uint64_t rng_seed{42};                   // For reproducibility
        bool auto_recover{true};                 // Auto-recover after duration
        std::string description;                 // Human-readable description
    };

    struct InjectionResult {
        bool success = 0;
        InjectionState state;
        std::string component;
        FaultType type;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point estimated_recovery;
        std::string error_message;
    };

    explicit FaultInjector(const InjectionConfig& config)
        : config_(config), state_(InjectionState::INACTIVE),
          rng_(config.rng_seed), start_time_{} {}

    virtual ~FaultInjector() = default;

    // ─────────────────────────────────────────────────────────────────
    // Lifecycle
    // ─────────────────────────────────────────────────────────────────

    /**
     * @brief Inject the fault
     * @return Injection result with status
     */
    virtual InjectionResult inject() = 0;

    /**
     * @brief Recover from the fault
     * @return Recovery status
     */
    virtual InjectionResult recover() = 0;

    /**
     * @brief Update fault state (handle auto-recovery, timeouts, etc.)
     * Called periodically to maintain fault state
     */
    virtual void update() = 0;

    // ─────────────────────────────────────────────────────────────────
    // State Query
    // ─────────────────────────────────────────────────────────────────

    InjectionState getState() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return state_;
    }

    bool isActive() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return state_ == InjectionState::ACTIVE;
    }

    const InjectionConfig& getConfig() const { return config_; }

    std::string getStateName() const {
        switch (getState()) {
            case InjectionState::INACTIVE:
                return "INACTIVE";
            case InjectionState::PENDING:
                return "PENDING";
            case InjectionState::ACTIVE:
                return "ACTIVE";
            case InjectionState::RECOVERING:
                return "RECOVERING";
            case InjectionState::RECOVERED:
                return "RECOVERED";
        }
        return "UNKNOWN";
    }

    // ─────────────────────────────────────────────────────────────────
    // Utilities
    // ─────────────────────────────────────────────────────────────────

    /**
     * @brief Convert FaultType to string
     */
    static std::string faultTypeToString(FaultType type) {
        switch (type) {
            case FaultType::NONE:
                return "NONE";
            case FaultType::CRASH:
                return "CRASH";
            case FaultType::NETWORK_PARTITION:
                return "NETWORK_PARTITION";
            case FaultType::NETWORK_DELAY:
                return "NETWORK_DELAY";
            case FaultType::NETWORK_PACKET_LOSS:
                return "NETWORK_PACKET_LOSS";
            case FaultType::TIMEOUT:
                return "TIMEOUT";
            case FaultType::CORRUPTION:
                return "CORRUPTION";
            case FaultType::MEMORY_LIMIT:
                return "MEMORY_LIMIT";
            case FaultType::DISK_FULL:
                return "DISK_FULL";
        }
        return "UNKNOWN";
    }

protected:
    InjectionConfig config_;
    InjectionState state_;
    mutable std::mt19937_64 rng_;
    std::chrono::steady_clock::time_point start_time_;
    mutable std::mutex mutex_;

    /**
     * @brief Set injection state (thread-safe)
     */
    void setState(InjectionState new_state) {
        std::lock_guard<std::mutex> lk(mutex_);
        state_ = new_state;
    }

    /**
     * @brief Generate random value [0, 1)
     */
    double randomDouble() const {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng_);
    }

    /**
     * @brief Generate random integer [min, max]
     */
    int64_t randomInt(int64_t min, int64_t max) const {
        std::uniform_int_distribution<int64_t> dist(min, max);
        return dist(rng_);
    }

    /**
     * @brief Check if duration has elapsed
     */
    bool isDurationElapsed() const {
        if (config_.duration.count() == 0) {
            return false;  // Permanent fault
        }
        auto elapsed =
            std::chrono::steady_clock::now() - start_time_;
        return elapsed >= config_.duration;
    }

    /**
     * @brief Get time until recovery
     */
    std::chrono::milliseconds timeUntilRecovery() const {
        if (config_.duration.count() == 0) {
            return std::chrono::milliseconds{-1};  // Never
        }
        auto elapsed =
            std::chrono::steady_clock::now() - start_time_;
        if (elapsed >= config_.duration) {
            return std::chrono::milliseconds{0};
        }
        return config_.duration - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    }
};

// ============================================================================
// SPECIALIZED FAULT INJECTORS
// ============================================================================

/**
 * @brief Crash injection - simulates node/process crash
 * 
 * Usage:
 *   CrashInjector::InjectionConfig cfg{
 *       .target_component = "coordinator",
 *       .crash_point = CrashInjector::CrashPoint::BEFORE_FSYNC,
 *       .duration = 5s  // Simulate recovery after 5s
 *   };
 *   CrashInjector crash(cfg);
 *   crash.inject();
 *   // ... execute operation, expect failure
 *   crash.recover();
 *   // ... verify recovered state
 */
class CrashInjector : public FaultInjector {
public:
    enum class CrashPoint {
        IMMEDIATE,
        BEFORE_FSYNC,
        AFTER_FSYNC,
        DURING_COMMIT,
        DURING_ABORT,
        RANDOM
    };

    struct CrashConfig : public InjectionConfig {
        CrashPoint crash_point = CrashPoint::IMMEDIATE;
        bool should_crash_flag = false;
    };

    explicit CrashInjector(const CrashConfig& cfg) : FaultInjector(cfg), crash_cfg_(cfg) {
        config_.type = FaultType::CRASH;
    }

    InjectionResult inject() override;
    InjectionResult recover() override;
    void update() override;

    /**
     * @brief Check if we should crash at given point
     */
    bool shouldCrashAt(CrashPoint point) const {
        std::lock_guard<std::mutex> lk(mutex_);
        if (!crash_cfg_.should_crash_flag) {
            return false;
        }
        if (crash_cfg_.crash_point == CrashPoint::RANDOM) {
            return randomDouble() < config_.probability;
        }
        return crash_cfg_.crash_point == point;
    }

private:
    CrashConfig crash_cfg_;
};

/**
 * @brief Network fault injection - simulates network partitions, delays, packet loss
 */
class NetworkInjector : public FaultInjector {
public:
    enum class NetworkFaultType {
        PARTITION,      // Complete partition (all packets dropped)
        DELAY,          // Add latency
        PACKET_LOSS,    // Drop random packets
        CORRUPTION      // Corrupt some packets
    };

    struct NetworkConfig : public InjectionConfig {
        NetworkFaultType network_type = NetworkFaultType::PARTITION;
        std::string target_node;
        std::chrono::milliseconds latency{0};  // For DELAY type
        float packet_loss_rate{0.0f};          // For PACKET_LOSS (0.0 - 1.0)
        std::vector<std::string> partition_members;  // Nodes to partition from
    };

    explicit NetworkInjector(const NetworkConfig& cfg) : FaultInjector(cfg), net_cfg_(cfg) {
        config_.type = determineType(cfg.network_type);
    }

    InjectionResult inject() override;
    InjectionResult recover() override;
    void update() override;

    /**
     * @brief Check if packet should be dropped
     */
    bool shouldDropPacket() const {
        std::lock_guard<std::mutex> lk(mutex_);
        if (state_ != InjectionState::ACTIVE) {
            return false;
        }
        return randomDouble() < net_cfg_.packet_loss_rate;
    }

    /**
     * @brief Check if node is partitioned
     */
    bool isPartitioned(const std::string& node_id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        if (state_ != InjectionState::ACTIVE) {
            return false;
        }
        for (const auto& member : net_cfg_.partition_members) {
            if (member == node_id) {
                return true;
            }
        }
        return false;
    }

private:
    NetworkConfig net_cfg_;

    static FaultType determineType(NetworkFaultType nft) {
        switch (nft) {
            case NetworkFaultType::PARTITION:
                return FaultType::NETWORK_PARTITION;
            case NetworkFaultType::DELAY:
                return FaultType::NETWORK_DELAY;
            case NetworkFaultType::PACKET_LOSS:
                return FaultType::NETWORK_PACKET_LOSS;
            case NetworkFaultType::CORRUPTION:
                return FaultType::CORRUPTION;
        }
        return FaultType::NONE;
    }
};

/**
 * @brief Timeout injection - simulates timeout conditions
 */
class TimeoutInjector : public FaultInjector {
public:
    struct TimeoutConfig : public InjectionConfig {
        std::chrono::milliseconds timeout_delay{0};  // Additional delay to trigger timeout
        bool trigger_immediately = false;
    };

    explicit TimeoutInjector(const TimeoutConfig& cfg) : FaultInjector(cfg), timeout_cfg_(cfg) {
        config_.type = FaultType::TIMEOUT;
    }

    InjectionResult inject() override;
    InjectionResult recover() override;
    void update() override;

    /**
     * @brief Check if should timeout
     */
    bool shouldTimeout() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return state_ == InjectionState::ACTIVE && timeout_cfg_.trigger_immediately;
    }

private:
    TimeoutConfig timeout_cfg_;
};

/**
 * @brief Corruption injection - simulates data corruption
 */
class CorruptionInjector : public FaultInjector {
public:
    struct CorruptionConfig : public InjectionConfig {
        std::string data_field;
        float corruption_rate{0.1f};  // 0.0 - 1.0 (percentage of bytes)
        bool corrupt_checksum = false;
    };

    explicit CorruptionInjector(const CorruptionConfig& cfg) : FaultInjector(cfg), corr_cfg_(cfg) {
        config_.type = FaultType::CORRUPTION;
    }

    InjectionResult inject() override;
    InjectionResult recover() override;
    void update() override;

    /**
     * @brief Corrupt data buffer
     * @param buffer Data to corrupt
     * @param size Buffer size
     * @return Number of bytes corrupted
     */
    size_t corruptData(uint8_t* buffer, size_t size);

private:
    CorruptionConfig corr_cfg_;
};

// ============================================================================
// FAULT INJECTION ORCHESTRATOR
// ============================================================================

/**
 * @brief Manages multiple fault injectors for complex scenarios
 * 
 * Usage:
 *   FaultOrchestrator orch;
 *   orch.addInjector(crash_cfg);
 *   orch.addInjector(network_cfg);
 *   orch.injectAll();
 *   // ... execute operations
 *   orch.recoverAll();
 */
class FaultOrchestrator {
public:
    FaultOrchestrator() = default;
    ~FaultOrchestrator() = default;

    // Non-copyable
    FaultOrchestrator(const FaultOrchestrator&) = delete;
    FaultOrchestrator& operator=(const FaultOrchestrator&) = delete;

    /**
     * @brief Add fault injector
     * @param injector Injector to add
     * @return Injector ID for tracking
     */
    int addInjector(std::unique_ptr<FaultInjector> injector);

    /**
     * @brief Remove injector by ID
     */
    void removeInjector(int id);

    /**
     * @brief Inject all registered faults
     */
    void injectAll();

    /**
     * @brief Recover all injected faults
     */
    void recoverAll();

    /**
     * @brief Update all injectors (call periodically)
     */
    void updateAll();

    /**
     * @brief Check if any fault is currently active
     */
    bool hasActiveFaults() const;

    /**
     * @brief Get active fault count
     */
    size_t getActiveFaultCount() const;

    /**
     * @brief Get injector by ID
     */
    FaultInjector* getInjector(int id);

    /**
     * @brief Get all injector IDs
     */
    std::vector<int> getAllInjectorIds() const;

    /**
     * @brief Clear all injectors
     */
    void clear();

    /**
     * @brief Get statistics
     */
    std::map<std::string, int> getStatistics() const;

private:
    std::map<int, std::unique_ptr<FaultInjector>> injectors_;
    int next_id_ = 0;
    mutable std::mutex mutex_;

    int generateId() {
        std::lock_guard<std::mutex> lk(mutex_);
        return next_id_++;
    }
};

// ============================================================================
// CHAOS SCENARIO BUILDER
// ============================================================================

/**
 * @brief Fluent builder for complex chaos scenarios
 * 
 * Usage:
 *   ChaosScenario()
 *       .crash("coordinator", 5s)
 *       .networkPartition("node-1", {"node-2", "node-3"}, 10s)
 *       .timeout("prepare_phase", 100ms)
 *       .build()
 */
class ChaosScenario {
public:
    ChaosScenario() : orchestrator_(std::make_unique<FaultOrchestrator>()) {}

    /**
     * @brief Add crash injection
     */
    ChaosScenario& crash(
        const std::string& target,
        std::chrono::milliseconds duration = std::chrono::milliseconds{0},
        CrashInjector::CrashPoint point = CrashInjector::CrashPoint::IMMEDIATE);

    /**
     * @brief Add network partition
     */
    ChaosScenario& networkPartition(
        const std::string& target_node,
        const std::vector<std::string>& partition_members,
        std::chrono::milliseconds duration);

    /**
     * @brief Add network delay
     */
    ChaosScenario& networkDelay(
        const std::string& target_node,
        std::chrono::milliseconds latency,
        std::chrono::milliseconds duration);

    /**
     * @brief Add packet loss
     */
    ChaosScenario& packetLoss(
        const std::string& target_node,
        float loss_rate,
        std::chrono::milliseconds duration);

    /**
     * @brief Add timeout injection
     */
    ChaosScenario& timeout(
        const std::string& target,
        std::chrono::milliseconds duration);

    /**
     * @brief Add data corruption
     */
    ChaosScenario& corruption(
        const std::string& target,
        const std::string& field,
        float corruption_rate);

    /**
     * @brief Build and inject scenario
     */
    FaultOrchestrator* build();

    /**
     * @brief Get orchestrator for direct access
     */
    FaultOrchestrator* getOrchestrator() { return orchestrator_.get(); }

private:
    std::unique_ptr<FaultOrchestrator> orchestrator_;
};

}  // namespace test
}  // namespace themis
