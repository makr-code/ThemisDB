/**
 * @file cep_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Complex Event Processing (CEP) Engine
 * 
 * Streaming Analytics engine for real-time event processing with:
 * - Pattern matching (SEQUENCE, AND, OR, NOT, WITHIN)
 * - Window management (TUMBLING, SLIDING, SESSION, HOPPING)
 * - Aggregations (COUNT, SUM, AVG, MIN, MAX, PERCENTILE)
 * - EPL (Event Processing Language) support
 * - Stateful processing with checkpoints
 * - Integration with ThemisDB CDC
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <map>
#include <set>
#include <unordered_map>
#include <optional>
#include <variant>
#include <any>
#include <regex>

// Lock-free MPMC ring buffer for the CEP event queue.
#include "analytics/detail/ring_buffer.h"

namespace themisdb {
namespace analytics {

// ============================================================================
// Forward Declarations
// ============================================================================

class CEPEngine;
class EventStream;
class PatternMatcher;
class WindowManager;
class RuleEngine;
class Aggregator;

// ============================================================================
// Enums & Constants
// ============================================================================

/**
 * Event types for CDC and system events
 */
enum class EventType : uint16_t {
    // Database Events (CDC)
    DOCUMENT_INSERT = 0x0100,
    DOCUMENT_UPDATE = 0x0101,
    DOCUMENT_DELETE = 0x0102,
    DOCUMENT_EXPIRE = 0x0103,
    COLLECTION_CREATE = 0x0110,
    COLLECTION_DROP = 0x0111,
    COLLECTION_TRUNCATE = 0x0112,
    INDEX_CREATE = 0x0120,
    INDEX_DROP = 0x0121,
    
    // Graph Events
    VERTEX_CREATE = 0x0200,
    VERTEX_UPDATE = 0x0201,
    VERTEX_DELETE = 0x0202,
    EDGE_CREATE = 0x0210,
    EDGE_UPDATE = 0x0211,
    EDGE_DELETE = 0x0212,
    GRAPH_TRAVERSAL = 0x0220,
    
    // Query Events
    QUERY_START = 0x0300,
    QUERY_COMPLETE = 0x0301,
    QUERY_ERROR = 0x0302,
    QUERY_SLOW = 0x0303,
    
    // Security Events
    AUTH_SUCCESS = 0x0400,
    AUTH_FAILURE = 0x0401,
    PERMISSION_DENIED = 0x0402,
    TOKEN_REFRESH = 0x0403,
    API_KEY_USED = 0x0404,
    
    // System Events
    SHARD_JOIN = 0x0500,
    SHARD_LEAVE = 0x0501,
    REBALANCE_START = 0x0510,
    REBALANCE_COMPLETE = 0x0511,
    MIGRATION_START = 0x0520,
    MIGRATION_COMPLETE = 0x0521,
    BACKUP_START = 0x0530,
    BACKUP_COMPLETE = 0x0531,
    
    // Performance Events
    HIGH_LATENCY = 0x0600,
    MEMORY_PRESSURE = 0x0601,
    DISK_PRESSURE = 0x0602,
    CONNECTION_POOL_EXHAUSTED = 0x0603,
    
    // Custom Events
    CUSTOM = 0xFF00
};

/**
 * Window types for event aggregation
 */
enum class WindowType {
    TUMBLING,       // Fixed, non-overlapping windows
    SLIDING,        // Overlapping windows
    SESSION,        // Gap-based windows
    HOPPING,        // Overlapping with fixed hop
    COUNT,          // Count-based windows
    GLOBAL          // No windowing (all events)
};

/**
 * Pattern types for event matching
 */
enum class PatternType {
    SEQUENCE,       // Events in order: A → B → C
    CONJUNCTION,    // Events together: A AND B
    DISJUNCTION,    // Any event: A OR B
    NEGATION,       // Not followed by: A NOT B
    REPETITION,     // Repeated: A{3,5}
    KLEENE_PLUS,    // One or more: A+
    KLEENE_STAR,    // Zero or more: A*
    OPTIONAL        // Zero or one: A?
};

/**
 * Aggregation functions
 */
enum class AggregationType {
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX,
    FIRST,
    LAST,
    STDDEV,
    VARIANCE,
    PERCENTILE,
    DISTINCT_COUNT,
    COLLECT,        // Collect into array
    TOPN            // Top N values
};

/**
 * Action types when pattern matches
 */
enum class ActionType {
    ALERT,          // Add to alert queue
    LOG,            // Log message
    WEBHOOK,        // HTTP POST to URL
    DB_WRITE,       // Write to ThemisDB collection
    EMAIL,          // Send email
    SLACK,          // Send to Slack
    KAFKA,          // Publish to Kafka
    CUSTOM          // Custom handler
};

/**
 * Event priority
 */
enum class EventPriority : uint8_t {
    CRITICAL = 0,
    HIGH = 1,
    NORMAL = 2,
    LOW = 3,
    BACKGROUND = 4
};

// Constants
constexpr size_t DEFAULT_STREAM_BUFFER_SIZE = 1024 * 1024;  // 1M events
constexpr size_t DEFAULT_PARTITIONS = 16;
constexpr uint32_t DEFAULT_CHECKPOINT_INTERVAL_MS = 10000;

// ============================================================================
// Event Structures
// ============================================================================

/**
 * Field value (variant type)
 */
using CepFieldValue = std::variant<
    std::monostate,         // null
    bool,
    int64_t,
    double,
    std::string,
    std::vector<uint8_t>,   // binary
    std::vector<std::string>,
    std::map<std::string, std::string>
>;

/**
 * Event payload
 */
struct Event {
    std::string event_id;
    EventType type = EventType::CUSTOM;
    std::string event_name;              // Custom event name
    EventPriority priority = EventPriority::NORMAL;
    
    // Timestamps
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point processing_time;
    
    // Source
    std::string source_shard_id;
    std::string collection_name;
    std::string document_id;
    
    // Partitioning
    std::string partition_key;
    uint32_t partition_id = 0;
    
    // Payload
    std::map<std::string, CepFieldValue> fields;
    std::vector<uint8_t> raw_payload;
    
    // Tracking
    uint64_t sequence_number = 0;
    uint32_t watermark = 0;
    
    // Helper methods
    template<typename T>
    std::optional<T> getField(const std::string& name) const {
        auto it = fields.find(name);
        if (it == fields.end()) {
          return std::nullopt;
        }
        if (auto* val = std::get_if<T>(&it->second)) {
            return *val;
        }
        return std::nullopt;
    }
    
    void setField(const std::string& name, CepFieldValue value) {
        fields[name] = std::move(value);
    }
    
    std::vector<uint8_t> serialize() const;
    static std::optional<Event> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Pattern match result
 */
struct PatternMatch {
    std::string pattern_id;
    std::string rule_id;
    std::chrono::system_clock::time_point match_time;
    std::vector<Event> matched_events;
    std::map<std::string, CepFieldValue> bindings;  // Captured values
    double confidence = 1.0;
};

/**
 * Aggregation result
 */
struct AggregationResult {
    std::string aggregation_id;
    AggregationType type;
    CepFieldValue result;
    uint64_t count = 0;
    std::chrono::system_clock::time_point window_start;
    std::chrono::system_clock::time_point window_end;
    std::map<std::string, CepFieldValue> group_by_values;
};

/**
 * Alert from rule match
 */
struct Alert {
    std::string alert_id;
    std::string rule_id;
    std::string rule_name;
    std::string severity;           // "critical", "warning", "info"
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    PatternMatch match;
    std::map<std::string, CepFieldValue> context;
    bool acknowledged = false;
};

// ============================================================================
// Configuration
// ============================================================================

/**
 * Window configuration
 */
struct WindowConfig {
    WindowType type = WindowType::TUMBLING;
    std::chrono::milliseconds size{60000};      // Window size
    std::chrono::milliseconds slide{0};          // Slide interval (for SLIDING/HOPPING)
    std::chrono::milliseconds gap{30000};        // Gap for SESSION windows
    uint64_t count = 0;                          // For COUNT windows
    bool emit_on_close = true;                   // Emit results when window closes
    bool emit_on_event = false;                  // Emit on every event
    std::chrono::milliseconds allowed_lateness{0}; // Late event tolerance
    /// How often the timer thread wakes to emit GLOBAL window snapshots and
    /// close expired SESSION windows.  Smaller values reduce emission latency.
    std::chrono::milliseconds global_window_emit_interval_ms{500};
};

/**
 * Pattern configuration
 */
struct PatternConfig {
    std::string pattern_id;
    PatternType type = PatternType::SEQUENCE;
    std::vector<std::string> event_types;       // Event type names to match
    std::chrono::milliseconds within{0};        // Time constraint
    std::chrono::milliseconds tolerance{1000};  // Time tolerance for CONJUNCTION
    uint32_t min_occurrences = 1;               // For REPETITION
    uint32_t max_occurrences = UINT32_MAX;
    std::string condition;                      // Additional filter expression
    std::vector<std::string> group_by;          // Group matching by fields
};

/**
 * Action configuration
 */
struct ActionConfig {
    ActionType type = ActionType::ALERT;
    std::string target;                         // URL, collection name, etc.
    std::string template_str;                   // Message template
    std::map<std::string, std::string> headers; // HTTP headers
    uint32_t retry_count = 3;
    std::chrono::milliseconds retry_delay{1000};
    bool async = true;
};

/**
 * Rule configuration (EPL-like)
 */
struct RuleConfig {
    std::string rule_id;
    std::string rule_name;
    std::string description;
    bool enabled = true;
    EventPriority min_priority = EventPriority::BACKGROUND;
    
    // Source
    std::vector<std::string> streams;           // Input streams
    std::string filter;                         // WHERE clause
    
    // Pattern (optional)
    std::optional<PatternConfig> pattern;
    
    // Window (optional)
    std::optional<WindowConfig> window;
    
    // Aggregations
    std::vector<std::pair<std::string, AggregationType>> aggregations;
    std::vector<std::string> group_by;
    std::string having;                         // HAVING clause
    
    // Output
    std::vector<ActionConfig> actions;
    
    // Metadata
    std::map<std::string, std::string> tags;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

/**
 * Stream configuration
 */
struct StreamConfig {
    std::string stream_id;
    std::string stream_name;
    size_t buffer_size = DEFAULT_STREAM_BUFFER_SIZE;
    uint32_t partitions = DEFAULT_PARTITIONS;
    std::string partition_key_field;            // Field to partition by
    bool enable_backpressure = true;
    float backpressure_threshold = 0.8f;        // 80% full
    std::chrono::milliseconds retention{3600000}; // 1 hour default
};

/**
 * CEP Engine configuration
 */
struct CEPConfig {
    bool enabled = true;
    
    // Threading
    uint32_t worker_threads = 4;
    uint32_t io_threads = 2;
    
    // Checkpointing
    bool checkpointing_enabled = true;
    std::string checkpoint_path = "/var/lib/themisdb/cep/checkpoints";
    std::chrono::milliseconds checkpoint_interval{DEFAULT_CHECKPOINT_INTERVAL_MS};
    
    // State
    size_t max_state_size_bytes = 1024 * 1024 * 1024;  // 1GB
    
    // Backpressure
    bool backpressure_enabled = true;
    float global_backpressure_threshold = 0.9f;
    size_t max_queue_depth = 65536;             // Max pending events in engine queue
    
    // Metrics
    bool metrics_enabled = true;
    std::chrono::milliseconds metrics_interval{5000};
};

// ============================================================================
// Event Stream
// ============================================================================

/**
 * Partitioned ring buffer for events
 */
class EventStream {
public:
    explicit EventStream(const StreamConfig& config);
    ~EventStream();
    
    /**
     * Push event to stream
     */
    enum class PushResult {
        SUCCESS,
        BACKPRESSURE,       // Buffer nearly full
        DROPPED,            // Event dropped due to full buffer
        ERROR
    };
    PushResult push(Event event);
    
    /**
     * Pull event from stream
     */
    std::optional<Event> pull(uint32_t partition_id);
    
    /**
     * Peek at next event without consuming
     */
    std::optional<Event> peek(uint32_t partition_id) const;
    
    /**
     * Get stream ID
     */
    const std::string& getStreamId() const { return config_.stream_id; }
    
    /**
     * Get buffer fill level (0.0 - 1.0)
     */
    float getFillLevel(uint32_t partition_id) const;
    float getOverallFillLevel() const;
    
    /**
     * Check if under backpressure
     */
    bool isUnderBackpressure() const;
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t events_pushed = 0;
        uint64_t events_pulled = 0;
        uint64_t events_dropped = 0;
        uint64_t backpressure_count = 0;
        size_t current_size = 0;
        float fill_level = 0.0f;
    };
    Stats getStats() const;
    
    /**
     * Subscribe to events
     */
    using EventCallback = std::function<void(const Event&)>;
    uint64_t subscribe(EventCallback callback);
    void unsubscribe(uint64_t subscription_id);

private:
    StreamConfig config_;
    
    // Partitioned ring buffers
    struct Partition {
        std::deque<Event> buffer;
        std::atomic<size_t> size{0};
        mutable std::mutex mutex;
    };
    std::vector<std::unique_ptr<Partition>> partitions_;
    
    // Statistics
    std::atomic<uint64_t> events_pushed_{0};
    std::atomic<uint64_t> events_pulled_{0};
    std::atomic<uint64_t> events_dropped_{0};
    std::atomic<uint64_t> backpressure_count_{0};
    
    // Subscribers
    std::map<uint64_t, EventCallback> subscribers_;
    std::atomic<uint64_t> next_subscription_id_{0};
    mutable std::shared_mutex subscribers_mutex_;
    
    uint32_t getPartitionId(const Event& event) const;
    void notifySubscribers(const Event& event);
};

// ============================================================================
// Pattern Matcher
// ============================================================================

/**
 * Pattern matching engine using NFA
 */
class PatternMatcher {
public:
    explicit PatternMatcher(const PatternConfig& config);
    ~PatternMatcher();
    
    /**
     * Process event and check for matches
     */
    std::vector<PatternMatch> processEvent(const Event& event);
    
    /**
     * Get pattern ID
     */
    const std::string& getPatternId() const { return config_.pattern_id; }
    
    /**
     * Get match count
     */
    uint64_t getMatchCount() const { return match_count_.load(); }
    
    /**
     * Reset state
     */
    void reset();
    
    /**
     * Get pending partial matches
     */
    size_t getPendingMatchCount() const;

    /**
     * Serialize in-progress NFA partial match state to a multi-line string.
     * Used by CEPEngine::createCheckpoint() to persist stateful pattern matching
     * across restarts.
     *
     * Format (one partial match per "pm_match=" line, followed by pm_ev= lines):
     *   pm_match=<group_key_hex>|<current_state>|<age_ms>
     *   pm_ev=<event_hex>
     *   ...
     */
    std::string serializeState() const;

    /**
     * Restore in-progress NFA partial match state from the string produced by
     * serializeState().  Clears existing partial matches before restoring.
     */
    void restoreState(const std::string& data);

private:
    PatternConfig config_;
    std::atomic<uint64_t> match_count_{0};
    
    // NFA state machine
    struct NFAState {
        uint32_t state_id = 0;
        std::string expected_event_type;
        bool is_accepting = false;
        std::vector<uint32_t> transitions;
    };
    std::vector<NFAState> nfa_states_;
    
    // Active partial matches (grouped by partition key)
    struct PartialMatch {
        uint32_t current_state = 0;
        std::vector<Event> matched_events;
        std::chrono::steady_clock::time_point start_time;
        std::map<std::string, CepFieldValue> bindings;
    };
    std::map<std::string, std::vector<PartialMatch>> partial_matches_;
    mutable std::mutex state_mutex_;
    
    void buildNFA();
    bool matchesEventType(const Event& event, const std::string& expected) const;
    bool evaluateCondition(const Event& event) const;
    void pruneExpiredMatches();
};

// ============================================================================
// Window Manager
// ============================================================================

/**
 * Manages time and count-based windows
 */
class WindowManager {
public:
    explicit WindowManager(const WindowConfig& config);
    ~WindowManager();
    
    /**
     * Add event to window
     */
    void addEvent(const Event& event);
    
    /**
     * Get events in current window
     */
    std::vector<Event> getWindowEvents() const;
    
    /**
     * Get events for a specific time range
     */
    std::vector<Event> getEvents(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const;
    
    /**
     * Trigger window evaluation
     */
    using WindowCallback = std::function<void(const std::vector<Event>&, 
                                               std::chrono::system_clock::time_point start,
                                               std::chrono::system_clock::time_point end)>;
    void setWindowCallback(WindowCallback callback);
    
    /**
     * Advance watermark (for out-of-order events)
     */
    void advanceWatermark(std::chrono::system_clock::time_point watermark);
    
    /**
     * Get window statistics
     */
    struct Stats {
        uint64_t windows_created = 0;
        uint64_t windows_closed = 0;
        uint64_t events_in_window = 0;
        uint64_t late_events = 0;
    };
    Stats getStats() const;

    /**
     * Carries a snapshot of window data for deferred callback dispatch.
     * Events are moved in to avoid copies when closing windows.
     */
    struct WindowCallbackBatch {
        std::vector<Event> events;
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
    };

private:
    WindowConfig config_;
    WindowCallback callback_;
    
    // Window state
    struct Window {
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
        std::vector<Event> events;
        bool closed = false;
    };
    std::deque<Window> windows_;
    mutable std::mutex windows_mutex_;
    
    // Session window state (per partition key)
    std::map<std::string, Window> session_windows_;
    
    // Watermark
    std::chrono::system_clock::time_point watermark_;
    
    // Statistics
    std::atomic<uint64_t> windows_created_{0};
    std::atomic<uint64_t> windows_closed_{0};
    std::atomic<uint64_t> late_events_{0};
    
    // Timer thread
    std::atomic<bool> running_{false};
    std::thread timer_thread_;
    std::condition_variable timer_cv_;
    std::mutex timer_mutex_;
    
    void timerLoop();
    // Marks window closed and returns a batch for deferred dispatch (lock must
    // be held by caller; callback is NOT invoked here).
    std::optional<WindowCallbackBatch> closeWindow(Window& window);
    void handleTumblingWindow(const Event& event);
    void handleSlidingWindow(const Event& event);
    void handleSessionWindow(const Event& event);
    void handleCountWindow(const Event& event);
};

// ============================================================================
// Aggregator
// ============================================================================

/**
 * Computes aggregations over event windows
 */
class Aggregator {
public:
    Aggregator();
    ~Aggregator();
    
    /**
     * Add aggregation
     */
    void addAggregation(
        const std::string& name,
        AggregationType type,
        const std::string& field);
    
    /**
     * Process event
     */
    void processEvent(const Event& event);
    
    /**
     * Get results
     */
    std::map<std::string, AggregationResult> getResults() const;
    
    /**
     * Get result for specific aggregation
     */
    std::optional<AggregationResult> getResult(const std::string& name) const;
    
    /**
     * Reset all aggregations
     */
    void reset();
    
    /**
     * Set group by fields
     */
    void setGroupBy(const std::vector<std::string>& fields);

private:
    struct AggregationState {
        std::string name;
        AggregationType type;
        std::string field;
        
        // State for different aggregation types
        int64_t count = 0;
        double sum = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        std::vector<double> values;  // For percentile, stddev
        std::set<std::string> distinct_values;
        CepFieldValue first_value;
        CepFieldValue last_value;
        bool has_first = false;
    };
    
    std::map<std::string, AggregationState> aggregations_;
    std::vector<std::string> group_by_fields_;
    
    // Grouped aggregations
    std::map<std::string, std::map<std::string, AggregationState>> grouped_aggregations_;
    
    mutable std::mutex mutex_;
    
    std::string getGroupKey(const Event& event) const;
    void updateAggregation(AggregationState& state, const Event& event);
    CepFieldValue computeResult(const AggregationState& state) const;
};

// ============================================================================
// Rule Engine
// ============================================================================

/**
 * EPL-like rule engine
 */
class RuleEngine {
public:
    explicit RuleEngine(CEPEngine* engine);
    ~RuleEngine();
    
    /**
     * Add rule
     */
    bool addRule(const RuleConfig& config);
    
    /**
     * Remove rule
     */
    bool removeRule(const std::string& rule_id);
    
    /**
     * Enable/disable rule
     */
    void setRuleEnabled(const std::string& rule_id, bool enabled);
    
    /**
     * Get rule
     */
    std::optional<RuleConfig> getRule(const std::string& rule_id) const;
    
    /**
     * Get all rules
     */
    std::vector<RuleConfig> getRules() const;
    
    /**
     * Process event against all rules
     */
    std::vector<Alert> processEvent(const Event& event);
    
    /**
     * Parse EPL string to rule config.
     *
     * Supported syntax:
     *   [CREATE RULE <name> AS | NAME <name>]
     *   SELECT [<agg_fn>(<field>) [AS <alias>], ...] FROM <stream>
     *   [WHERE <filter>]
     *   [PATTERN (SEQUENCE|SEQ|AND|OR|NOT) (<types>) [WITHIN <n>(ms|s|MINUTES|HOURS)]]
     *   [WINDOW (TUMBLING|SLIDING|SESSION|HOPPING|COUNT)(<n> UNIT[, <n> UNIT])]
     *   [GROUP BY <field>[, ...]]
     *   [HAVING <condition>]
     *   [ACTION (alert|webhook|db_write|log|slack|kafka|email)(<params>)
     *    | ON MATCH ALERT [severity=<s>] [message=<m>]]
     *
     * Aggregation functions: COUNT, SUM, AVG, MIN, MAX, FIRST, LAST,
     *   STDDEV, VARIANCE, PERCENTILE, DISTINCT_COUNT, COLLECT, TOPN
     * Time units: ms, s/second(s), minute(s), hour(s), day(s)
     */
    static std::optional<RuleConfig> parseEPL(const std::string& epl);
    
    /**
     * Get rule statistics
     */
    struct RuleStats {
        std::string rule_id;
        uint64_t events_processed = 0;
        uint64_t matches = 0;
        uint64_t actions_triggered = 0;
        std::chrono::milliseconds avg_processing_time{0};
    };
    RuleStats getRuleStats(const std::string& rule_id) const;

    /**
     * Serialize all pattern matcher states for use in a checkpoint.
     * Returns a multi-line string with pm_rule= / pm_rule_end blocks.
     */
    std::string serializeMatcherStates() const;

    /**
     * Restore pattern matcher states from the string produced by
     * serializeMatcherStates().  Only matchers for rules that currently exist
     * in the engine are restored; unknown rule IDs are silently skipped.
     */
    void restoreMatcherStates(const std::string& data);

private:
    CEPEngine* engine_;
    
    struct RuleState {
        RuleConfig config;
        std::unique_ptr<PatternMatcher> pattern_matcher;
        std::unique_ptr<WindowManager> window_manager;
        std::unique_ptr<Aggregator> aggregator;
        RuleStats stats;
    };
    
    std::map<std::string, RuleState> rules_;
    mutable std::shared_mutex rules_mutex_;
    
    bool evaluateFilter(const Event& event, const std::string& filter) const;
    bool evaluateHaving(const std::map<std::string, AggregationResult>& results,
                       const std::string& having) const;
    void executeActions(const RuleConfig& config, const PatternMatch& match);
    void executeAction(const ActionConfig& action, const PatternMatch& match,
                      const RuleConfig& rule);
};

// ============================================================================
// CEP Engine
// ============================================================================

/**
 * Main CEP engine - singleton
 */
class CEPEngine {
public:
    static CEPEngine& getInstance();
    
    /**
     * Initialize engine
     */
    void initialize(const CEPConfig& config);
    
    /**
     * Shutdown engine
     */
    void shutdown();
    
    /**
     * Check if initialized
     */
    bool isInitialized() const { return initialized_.load(); }
    
    // ========== Stream Management ==========
    
    /**
     * Create event stream
     */
    std::shared_ptr<EventStream> createStream(const StreamConfig& config);
    
    /**
     * Get stream by ID
     */
    std::shared_ptr<EventStream> getStream(const std::string& stream_id) const;
    
    /**
     * Get all streams
     */
    std::vector<std::shared_ptr<EventStream>> getStreams() const;
    
    /**
     * Remove stream
     */
    bool removeStream(const std::string& stream_id);
    
    // ========== Event Processing ==========
    
    /**
     * Submit event for processing
     */
    bool submitEvent(Event event);
    
    /**
     * Submit event to specific stream
     */
    bool submitEvent(const std::string& stream_id, Event event);
    
    /**
     * Create event from CDC change
     */
    static Event createCDCEvent(
        EventType type,
        const std::string& collection,
        const std::string& document_id,
        const std::map<std::string, CepFieldValue>& fields);
    
    // ========== Rule Management ==========
    
    /**
     * Add rule
     */
    bool addRule(const RuleConfig& config);
    
    /**
     * Add rule from EPL string
     */
    bool addRuleFromEPL(const std::string& epl);
    
    /**
     * Remove rule
     */
    bool removeRule(const std::string& rule_id);
    
    /**
     * Get rule
     */
    std::optional<RuleConfig> getRule(const std::string& rule_id) const;
    
    /**
     * Load rules from YAML file
     */
    bool loadRulesFromFile(const std::string& path);
    
    // ========== Alert Management ==========
    
    /**
     * Get alerts
     */
    std::vector<Alert> getAlerts(
        size_t limit = 100,
        bool unacknowledged_only = false) const;
    
    /**
     * Acknowledge alert
     */
    bool acknowledgeAlert(const std::string& alert_id);
    
    /**
     * Set alert callback
     */
    using AlertCallback = std::function<void(const Alert&)>;
    void setAlertCallback(AlertCallback callback);
    
    // ========== Statistics & Metrics ==========
    
    /**
     * Get engine statistics
     */
    struct Stats {
        uint64_t events_received = 0;
        uint64_t events_processed = 0;
        uint64_t events_dropped = 0;
        uint64_t backpressure_events = 0;
        uint64_t pattern_matches = 0;
        uint64_t rules_triggered = 0;
        uint64_t alerts_generated = 0;
        size_t queue_depth = 0;
        size_t active_streams = 0;
        size_t active_rules = 0;
        std::chrono::milliseconds avg_latency{0};
        float throughput_per_second = 0.0f;
    };
    Stats getStats() const;
    
    /**
     * Export Prometheus metrics
     */
    std::string toPrometheusFormat() const;
    
    // ========== Checkpointing ==========
    
    /**
     * Create a checkpoint of the current engine state.
     *
     * Writes a text file to the configured checkpoint_path directory containing:
     *   - Basic counters (events_received, events_processed, alerts_generated)
     *   - Rule enabled/disabled states
     *   - In-progress NFA partial match states for all registered pattern matchers
     *
     * The checkpoint can be used with restoreFromCheckpoint() to resume stateful
     * pattern sequences across restarts.
     *
     * @return true on success, false if checkpointing is disabled or an I/O error
     *         occurs.
     */
    bool createCheckpoint();
    
    /**
     * Restore engine state from a previously created checkpoint.
     *
     * Reads the checkpoint file written by createCheckpoint() and restores:
     *   1. The enabled/disabled state of each rule.
     *   2. The in-progress NFA partial match state for each pattern matcher,
     *      allowing stateful sequences (e.g. SEQUENCE A→B) that were partially
     *      matched before the checkpoint to continue after restart.
     *
     * Rules / matchers that exist in the checkpoint but are no longer registered
     * in the engine are silently skipped.
     *
     * Checkpoint file format:
     *   events_received=<N>
     *   events_processed=<N>
     *   alerts_generated=<N>
     *   rule=<rule_id>:<rule_name>:<1|0>          (1 = enabled, 0 = disabled)
     *   pm_rule=<rule_id>                          (start of matcher state block)
     *   pm_match=<group_key_hex>|<nfa_state>|<age_ms>
     *   pm_ev=<hex-encoded-serialized-event>       (one line per matched event)
     *   ...additional pm_match/pm_ev lines...
     *   pm_rule_end                                (end of matcher state block)
     *
     * @param checkpoint_id  Name of the checkpoint (stem of the .txt file
     *                       inside the configured checkpoint_path directory).
     *                       Use listCheckpoints() to enumerate available IDs.
     * @return true on success, false if the checkpoint file does not exist or
     *         cannot be opened.
     */
    bool restoreFromCheckpoint(const std::string& checkpoint_id);
    
    /**
     * List checkpoints
     */
    std::vector<std::string> listCheckpoints() const;

private:
    CEPEngine() = default;
    ~CEPEngine() = default;
    
    CEPConfig config_;
    std::unique_ptr<RuleEngine> rule_engine_;
    
    // Streams
    std::map<std::string, std::shared_ptr<EventStream>> streams_;
    std::shared_ptr<EventStream> default_stream_;
    mutable std::shared_mutex streams_mutex_;
    
    // Alerts
    std::deque<Alert> alerts_;
    AlertCallback alert_callback_;
    mutable std::mutex alerts_mutex_;
    
    // Statistics
    std::atomic<uint64_t> events_received_{0};
    std::atomic<uint64_t> events_processed_{0};
    std::atomic<uint64_t> events_dropped_{0};
    std::atomic<uint64_t> backpressure_events_{0};
    std::atomic<uint64_t> pattern_matches_{0};
    std::atomic<uint64_t> alerts_generated_{0};
    
    // Threading
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::vector<std::thread> worker_threads_;
    std::thread metrics_thread_;
    std::condition_variable cv_;
    std::mutex mutex_;
    // Used by metricsLoop() to wake immediately when running_ becomes false.
    std::condition_variable metrics_cv_;
    std::mutex metrics_mutex_;
    
    // Processing — lock-free MPMC ring buffer replaces std::queue + mutex.
    // Capacity mirrors max_queue_depth from CEPConfig (set during initialize()).
    // The ring buffer is re-created if initialize() is called again.
    std::unique_ptr<themis::analytics::detail::EventRingBuffer<
        std::pair<std::string, Event>>> event_queue_;
    // size_approx() is used for backpressure fill-ratio checks and getStats().
    
    void workerLoop();
    void metricsLoop();
    void processEvent(const std::string& stream_id, const Event& event);
    void addAlert(Alert alert);
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Convert EventType to string
 */
inline const char* eventTypeToString(EventType type) {
    switch (type) {
        case EventType::DOCUMENT_INSERT: return "DOCUMENT_INSERT";
        case EventType::DOCUMENT_UPDATE: return "DOCUMENT_UPDATE";
        case EventType::DOCUMENT_DELETE: return "DOCUMENT_DELETE";
        case EventType::COLLECTION_CREATE: return "COLLECTION_CREATE";
        case EventType::COLLECTION_DROP: return "COLLECTION_DROP";
        case EventType::VERTEX_CREATE: return "VERTEX_CREATE";
        case EventType::EDGE_CREATE: return "EDGE_CREATE";
        case EventType::QUERY_START: return "QUERY_START";
        case EventType::QUERY_COMPLETE: return "QUERY_COMPLETE";
        case EventType::AUTH_SUCCESS: return "AUTH_SUCCESS";
        case EventType::AUTH_FAILURE: return "AUTH_FAILURE";
        case EventType::SHARD_JOIN: return "SHARD_JOIN";
        case EventType::SHARD_LEAVE: return "SHARD_LEAVE";
        case EventType::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

/**
 * Convert WindowType to string
 */
inline const char* windowTypeToString(WindowType type) {
    switch (type) {
        case WindowType::TUMBLING: return "TUMBLING";
        case WindowType::SLIDING: return "SLIDING";
        case WindowType::SESSION: return "SESSION";
        case WindowType::HOPPING: return "HOPPING";
        case WindowType::COUNT: return "COUNT";
        case WindowType::GLOBAL: return "GLOBAL";
        default: return "UNKNOWN";
    }
}

/**
 * Convert AggregationType to string
 */
inline const char* aggregationTypeToString(AggregationType type) {
    switch (type) {
        case AggregationType::COUNT: return "COUNT";
        case AggregationType::SUM: return "SUM";
        case AggregationType::AVG: return "AVG";
        case AggregationType::MIN: return "MIN";
        case AggregationType::MAX: return "MAX";
        case AggregationType::FIRST: return "FIRST";
        case AggregationType::LAST: return "LAST";
        case AggregationType::STDDEV: return "STDDEV";
        case AggregationType::PERCENTILE: return "PERCENTILE";
        case AggregationType::DISTINCT_COUNT: return "DISTINCT_COUNT";
        default: return "UNKNOWN";
    }
}

} // namespace analytics
} // namespace themisdb

