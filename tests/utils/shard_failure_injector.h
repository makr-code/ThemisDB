#pragma once

#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <functional>

namespace themis {
namespace test {

/**
 * @brief Shard failure injector for chaos testing
 * 
 * Provides controlled and random failure injection for testing:
 * - Transient failures (temporary)
 * - Permanent failures
 * - Cascading failures
 * - Random failure scenarios
 */
class ShardFailureInjector {
public:
    /**
     * @brief Failure type
     */
    enum class FailureType {
        TRANSIENT,  // Temporary failure (auto-recovers)
        PERMANENT,  // Permanent failure (manual recovery required)
        SLOW,       // Slow response (high latency)
        CORRUPT     // Data corruption
    };

    /**
     * @brief Failure scenario
     */
    struct FailureScenario {
        FailureType type;
        int shard_id;
        std::chrono::milliseconds duration;
        float probability = 1.0f;  // 0.0 - 1.0
        bool active = false;
        std::chrono::steady_clock::time_point start_time;
    };

    /**
     * @brief Callback for failure events
     */
    using FailureCallback = std::function<void(int shard_id, FailureType type)>;

    /**
     * @brief Callback for recovery events
     */
    using RecoveryCallback = std::function<void(int shard_id)>;

    /**
     * @brief Constructor
     */
    ShardFailureInjector();

    /**
     * @brief Destructor
     */
    ~ShardFailureInjector();

    // ═══════════════════════════════════════════════════════════
    // Failure Injection
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Inject shard failure
     * @param shard_id Shard to fail
     * @param type Type of failure
     * @param duration Duration (0 for permanent)
     * @return Scenario ID
     */
    int injectFailure(int shard_id, FailureType type,
                     std::chrono::milliseconds duration = std::chrono::milliseconds{0});

    /**
     * @brief Inject random failures
     * @param num_failures Number of failures to inject
     * @param max_shard_id Maximum shard ID
     * @param transient If true, failures auto-recover
     * @param max_duration Maximum failure duration
     * @return Vector of scenario IDs
     */
    std::vector<int> injectRandomFailures(
        int num_failures,
        int max_shard_id,
        bool transient = true,
        std::chrono::milliseconds max_duration = std::chrono::seconds{30});

    /**
     * @brief Inject cascading failures
     * @param initial_shard Initial shard to fail
     * @param cascade_count Number of additional shards to fail
     * @param cascade_delay Delay between cascading failures
     * @return Vector of scenario IDs
     */
    std::vector<int> injectCascadingFailures(
        int initial_shard,
        int cascade_count,
        std::chrono::milliseconds cascade_delay);

    /**
     * @brief Clear all failures
     */
    void clearAllFailures();

    /**
     * @brief Recover specific scenario
     * @param scenario_id Scenario to recover
     * @return true if recovered
     */
    bool recoverScenario(int scenario_id);

    // ═══════════════════════════════════════════════════════════
    // Status Queries
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Check if shard has active failure
     * @param shard_id Shard to check
     * @return true if failed
     */
    bool isShardFailed(int shard_id) const;

    /**
     * @brief Get active failures for shard
     * @param shard_id Shard to check
     * @return Vector of active failure types
     */
    std::vector<FailureType> getActiveFailures(int shard_id) const;

    /**
     * @brief Get all active scenarios
     * @return Vector of active scenario IDs
     */
    std::vector<int> getActiveScenarios() const;

    /**
     * @brief Get scenario details
     * @param scenario_id Scenario ID
     * @return Scenario or nullptr if not found
     */
    const FailureScenario* getScenario(int scenario_id) const;

    // ═══════════════════════════════════════════════════════════
    // Callbacks
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Set failure callback
     * @param callback Callback function
     */
    void setFailureCallback(FailureCallback callback);

    /**
     * @brief Set recovery callback
     * @param callback Callback function
     */
    void setRecoveryCallback(RecoveryCallback callback);

    // ═══════════════════════════════════════════════════════════
    // Maintenance
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Update failure states (handle auto-recovery)
     * Should be called periodically
     */
    void update();

    /**
     * @brief Get failure statistics
     * @return JSON with statistics
     */
    std::map<std::string, int> getStatistics() const;

private:
    std::map<int, FailureScenario> scenarios_;
    int next_scenario_id_;
    FailureCallback failure_callback_;
    RecoveryCallback recovery_callback_;

    /**
     * @brief Generate unique scenario ID
     * @return New scenario ID
     */
    int generateScenarioId();
};

} // namespace test
} // namespace themis
