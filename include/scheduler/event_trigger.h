/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            event_trigger.h                                    ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:25:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 35729aa96d  2026-02-23  feat(scheduler): implement onCDCEvent stub for event-trig... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file event_trigger.h
 * @brief Event-based trigger system for task scheduler with CDC integration
 * 
 * Provides event-based task triggering based on database changes (CDC).
 * Supports:
 * - Key prefix filtering (e.g., "users:*")
 * - Event type filtering (PUT, DELETE, TRANSACTION_COMMIT, TRANSACTION_ROLLBACK)
 * - Optional condition evaluation on event payload
 * - Debouncing for high-frequency events
 * 
 * Integrates with the Changefeed system to listen for database changes
 * and trigger tasks when matching events occur.
 */

#ifndef THEMIS_EVENT_TRIGGER_H
#define THEMIS_EVENT_TRIGGER_H

#include "cdc/changefeed.h"
#include <string>
#include <set>
#include <optional>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace themis {

/**
 * @brief Configuration for CDC-based event trigger
 */
struct CDCTriggerConfig {
    std::string key_prefix;                           // Filter by key prefix (e.g., "users:")
    std::set<Changefeed::ChangeEventType> event_types; // Event types to trigger on
    std::optional<std::string> condition;             // Optional AQL filter condition
    uint32_t debounce_ms = 0;                         // Debounce interval in milliseconds
    
    // Validation
    bool isValid() const;
    std::string getValidationError() const;
};

/**
 * @brief Event listener for CDC events with filtering and debouncing
 * 
 * This class listens to Changefeed events and determines when to trigger
 * tasks based on configured filters and debouncing rules.
 */
class EventTrigger {
public:
    using TriggerCallback = std::function<void(const Changefeed::ChangeEvent&)>;
    
    /**
     * @brief Construct an event trigger
     * @param changefeed Changefeed instance to listen to (not owned)
     * @param config Trigger configuration
     * @param callback Function to call when event matches
     */
    EventTrigger(Changefeed* changefeed,
                 const CDCTriggerConfig& config,
                 TriggerCallback callback);
    
    ~EventTrigger();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Configuration
    const CDCTriggerConfig& getConfig() const { return config_; }
    void updateConfig(const CDCTriggerConfig& config);
    
    // Statistics
    struct Stats {
        uint64_t events_received = 0;
        uint64_t events_matched = 0;
        uint64_t events_debounced = 0;
        uint64_t triggers_fired = 0;
        uint64_t callback_failures = 0;
        bool circuit_open = false;
        std::chrono::system_clock::time_point last_trigger_time;
    };
    
    Stats getStats() const;
    
    // Circuit breaker configuration
    struct CircuitBreakerConfig {
        uint32_t failure_threshold = 5;              // Open circuit after this many consecutive failures
        std::chrono::seconds cooldown{30};           // How long circuit stays open
    };
    
    void setCircuitBreakerConfig(const CircuitBreakerConfig& config);
    
private:
    Changefeed* changefeed_;
    CDCTriggerConfig config_;
    TriggerCallback callback_;
    
    // Listener thread
    std::atomic<bool> running_{false};
    std::thread listener_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    // Debouncing state
    std::chrono::steady_clock::time_point last_trigger_time_;
    mutable std::mutex debounce_mutex_;
    
    // Statistics
    mutable std::atomic<uint64_t> events_received_{0};
    mutable std::atomic<uint64_t> events_matched_{0};
    mutable std::atomic<uint64_t> events_debounced_{0};
    mutable std::atomic<uint64_t> triggers_fired_{0};
    std::chrono::system_clock::time_point last_trigger_time_sys_;
    
    // Last sequence number processed
    uint64_t last_sequence_ = 0;

    // Circuit breaker state (guards callback_ from cascading failures)
    CircuitBreakerConfig cb_config_;
    mutable std::mutex cb_mutex_;
    uint32_t cb_consecutive_failures_{0};
    bool cb_open_{false};
    std::chrono::steady_clock::time_point cb_open_since_;
    std::atomic<uint64_t> callback_failures_{0};
    
    // Check circuit breaker and attempt to close it if cooldown elapsed
    // Returns true if the callback may be invoked (circuit closed or half-open probe).
    bool circuitAllows();
    // Record a callback success (closes the circuit if it was half-open)
    void circuitRecordSuccess();
    // Record a callback failure (may open the circuit)
    void circuitRecordFailure();
    
    // Event listener loop
    void listenerLoop();
    
    // Event filtering
    bool matchesFilter(const Changefeed::ChangeEvent& event) const;
    bool matchesKeyPrefix(const std::string& key) const;
    bool matchesEventType(Changefeed::ChangeEventType type) const;
    bool matchesCondition(const Changefeed::ChangeEvent& event) const;

    // Debouncing
    bool shouldDebounce() const;

    // ── Condition caching ─────────────────────────────────────────────────
    // Parsed form of a single condition clause (e.g. "key STARTS_WITH foo").
    struct ParsedClause {
        std::string field;  // "key" or "value"
        std::string op;     // "==", "!=", "STARTS_WITH", "ENDS_WITH", "CONTAINS"
        std::string rhs;    // Right-hand side (unquoted)
    };
    // Pre-parsed clauses derived from config_.condition; rebuilt whenever
    // the condition changes.  Access is guarded by condition_cache_mutex_.
    mutable std::vector<ParsedClause> parsed_clauses_;
    mutable bool condition_parsed_{false};
    mutable std::mutex condition_cache_mutex_;

    // Parse config_.condition into parsed_clauses_; must be called under
    // condition_cache_mutex_.
    void rebuildConditionCache_() const;
};

/**
 * @brief Manager for multiple event triggers
 * 
 * Manages lifecycle and coordination of multiple event triggers,
 * allowing efficient sharing of changefeed resources.
 */
class EventTriggerManager {
public:
    explicit EventTriggerManager(Changefeed* changefeed);
    ~EventTriggerManager();
    
    /**
     * @brief Register a new event trigger
     * @param id Unique identifier for the trigger
     * @param config Trigger configuration
     * @param callback Callback function
     * @return True if registered successfully
     */
    bool registerTrigger(const std::string& id,
                        const CDCTriggerConfig& config,
                        EventTrigger::TriggerCallback callback);
    
    /**
     * @brief Unregister an event trigger
     * @param id Trigger identifier
     */
    void unregisterTrigger(const std::string& id);
    
    /**
     * @brief Update trigger configuration
     * @param id Trigger identifier
     * @param config New configuration
     */
    void updateTrigger(const std::string& id, const CDCTriggerConfig& config);
    
    /**
     * @brief Get trigger statistics
     * @param id Trigger identifier
     * @return Statistics or nullopt if trigger not found
     */
    std::optional<EventTrigger::Stats> getTriggerStats(const std::string& id) const;
    
    /**
     * @brief Start all triggers
     */
    void startAll();
    
    /**
     * @brief Stop all triggers
     */
    void stopAll();
    
private:
    Changefeed* changefeed_;
    std::map<std::string, std::unique_ptr<EventTrigger>> triggers_;
    mutable std::mutex mutex_;
};

} // namespace themis

#endif // THEMIS_EVENT_TRIGGER_H
