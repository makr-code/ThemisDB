/**
 * @file streaming_window.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.32
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=5, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Streaming Aggregation Windows
 *
 * Standalone, thread-safe streaming window library for real-time aggregations.
 * Provides four window types (TUMBLING, SLIDING, SESSION, HOPPING), all with
 * the same aggregation functions as the OLAP engine (COUNT, SUM, AVG, MIN,
 * MAX, STDDEV, VARIANCE, PERCENTILE).
 *
 * Designed to be:
 *   - Self-contained (no dependency on CEPEngine or OLAPEngine)
 *   - Complementary to the CEP WindowManager (different, higher-level API)
 *   - Usable in streaming pipelines via StreamingWindowPipeline
 *   - Compatible with watermarking / late-event handling
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace themisdb {
namespace analytics {

// ============================================================================
// Forward declarations
// ============================================================================

class TumblingWindow;
class SlidingWindow;
class SessionWindow;
class HoppingWindow;
class StreamingWindowPipeline;

// ============================================================================
// Value types
// ============================================================================

/**
 * A record value – can be null, bool, int64, double, or string.
 */
using RecordValue = std::variant<
    std::monostate,   // null
    bool,
    int64_t,
    double,
    std::string
>;

/**
 * Aggregation function type.
 */
enum class AggFunc {
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX,
    STDDEV,
    VARIANCE,
    PERCENTILE,   // requires percentile p in [0,100] via WindowAggregateSpec::percentile_p
    FIRST,
    LAST,
    DISTINCT_COUNT
};

// ============================================================================
// StreamRecord
// ============================================================================

/**
 * A single timestamped record fed into a streaming window.
 * Fields are key-value pairs; partition_key is used to scope SESSION windows.
 */
struct StreamRecord {
    std::string record_id;
    std::chrono::system_clock::time_point event_time;   ///< event-time timestamp
    std::chrono::system_clock::time_point ingest_time;  ///< processing-time stamp
    std::string partition_key;                           ///< used by session windows
    std::map<std::string, RecordValue> fields;

    // Helper accessors
    template<typename T>
    std::optional<T> get(const std::string& field) const {
        auto it = fields.find(field);
        if (it == fields.end()) {
          return std::nullopt;
        }
        if (auto* v = std::get_if<T>(&it->second)) {
          return *v;
        }
        return std::nullopt;
    }

    void set(const std::string& field, RecordValue value) {
        fields[field] = std::move(value);
    }
};

// ============================================================================
// Aggregation spec & result
// ============================================================================

/**
 * Specifies one aggregation to compute in a window.
 */
struct WindowAggregateSpec {
    std::string name;           ///< output name in WindowResult
    AggFunc     func = AggFunc::COUNT;
    std::string field;          ///< input field name (empty → operate on presence)
    double      percentile_p = 50.0; ///< only used for PERCENTILE

    WindowAggregateSpec() = default;
    WindowAggregateSpec(std::string name_, AggFunc func_, std::string field_,
                  double percentile_p_ = 50.0)
        : name(std::move(name_)),
          func(func_),
          field(std::move(field_)),
          percentile_p(percentile_p_) {}

        WindowAggregateSpec(const WindowAggregateSpec&) = default;
        WindowAggregateSpec& operator=(const WindowAggregateSpec&) = default;
        WindowAggregateSpec(WindowAggregateSpec&&) noexcept = default;
        WindowAggregateSpec& operator=(WindowAggregateSpec&&) noexcept = default;
        ~WindowAggregateSpec() = default;
};

/**
 * A single aggregated value for one aggregation in one window.
 */
struct AggregatedValue {
    std::string name;
    AggFunc     func;
    RecordValue value;
    uint64_t    count = 0;
};

/**
 * The result of a closed (or emitted) window.
 */
struct WindowResult {
    std::string window_id;
    std::chrono::system_clock::time_point window_start;
    std::chrono::system_clock::time_point window_end;
    std::string partition_key;          ///< non-empty for session windows
    uint64_t    record_count = 0;
    std::vector<AggregatedValue> aggregations;
    bool        is_late_firing = false; ///< true when triggered by late data
    bool        is_early_firing = false;///< true when triggered before window close

    /// Retrieve a specific aggregation result by name
    std::optional<RecordValue> get(const std::string& agg_name) const;
};

// ============================================================================
// Watermark configuration
// ============================================================================

/**
 * Controls how late events are handled.
 */
struct WatermarkConfig {
    /// Maximum allowed out-of-order delay; events older than watermark - tolerance are dropped.
    std::chrono::milliseconds max_out_of_orderness{0};
    /// If no events arrive for this duration, advance watermark to processing time.
    std::chrono::milliseconds idle_timeout{60000};
    /// If true, late events still trigger result updates.
    bool allow_late_data = false;
};

// ============================================================================
// Window configuration structs
// ============================================================================

struct TumblingWindowConfig {
    std::chrono::milliseconds size{60000};
    WatermarkConfig watermark;
    bool emit_empty_windows = false;
    /// Maximum number of concurrently open windows. 0 = unlimited.
    /// When the limit is reached on a new-window allocation, the oldest open
    /// window is emitted and evicted to bound memory under sustained load.
    uint64_t max_open_windows = 0;
    /// Maximum records accepted per open window. 0 = unlimited.
    /// Records that arrive when a window is at capacity are dropped and counted
    /// in WindowStats::records_dropped.
    uint64_t max_records_per_window = 0;
    /// Maximum number of distinct non-empty partition_key values tracked per
    /// open window. 0 = unlimited. When the limit is reached, records carrying
    /// a new (previously unseen) partition_key for that window are rejected and
    /// counted in WindowStats::partition_keys_rejected.
    /// Bounds memory in high-cardinality key spaces (e.g. per-user analytics
    /// with millions of distinct user IDs sharing the same time bucket).
    uint64_t max_distinct_partition_keys = 0;
};

struct SlidingWindowConfig {
    std::chrono::milliseconds size{60000};
    std::chrono::milliseconds slide{10000};
    WatermarkConfig watermark;
    /// Maximum number of concurrently open (non-closed) windows. 0 = unlimited.
    /// When at limit, new window creation is skipped; the record is still added
    /// to any existing open windows that cover its event_time.
    uint64_t max_open_windows = 0;
    /// Maximum records accepted per open window. 0 = unlimited.
    /// Records that arrive when a window is at capacity are skipped for that window.
    uint64_t max_records_per_window = 0;
    /// Maximum number of distinct non-empty partition_key values accepted across
    /// the lifetime of this SlidingWindow instance. 0 = unlimited.
    /// Records carrying a new unseen partition_key when the limit is already
    /// reached are rejected and counted in WindowStats::partition_keys_rejected.
    uint64_t max_distinct_partition_keys = 0;
};

struct SessionWindowConfig {
    std::chrono::milliseconds gap{30000};
    WatermarkConfig watermark;
    /// How often the background expiry thread wakes to check for idle sessions.
    /// Smaller values reduce session-close latency at the cost of more CPU wakeups.
    std::chrono::milliseconds session_expiry_check_interval_ms{200};
    /// Maximum number of concurrently open sessions (partitions). 0 = unlimited.
    /// When the limit is reached on new-session creation, the session with the
    /// oldest last_event is emitted and evicted to bound memory.
    uint64_t max_open_sessions = 0;
    /// Maximum records accepted per open session. 0 = unlimited.
    /// Records that arrive when a session is at capacity are dropped.
    uint64_t max_records_per_session = 0;
};

struct HoppingWindowConfig {
    std::chrono::milliseconds size{60000};
    std::chrono::milliseconds hop{10000};
    WatermarkConfig watermark;
    /// Maximum number of concurrently open (non-closed) windows. 0 = unlimited.
    /// When at limit, new window creation is skipped for that hop slot.
    uint64_t max_open_windows = 0;
    /// Maximum records accepted per open window. 0 = unlimited.
    /// Records that arrive when a window is at capacity are skipped for that window.
    uint64_t max_records_per_window = 0;
    /// Maximum number of distinct non-empty partition_key values accepted across
    /// the lifetime of this HoppingWindow instance. 0 = unlimited.
    /// Records carrying a new unseen partition_key when the limit is already
    /// reached are rejected and counted in WindowStats::partition_keys_rejected.
    uint64_t max_distinct_partition_keys = 0;
};

// ============================================================================
// Window statistics
// ============================================================================

struct WindowStats {
    uint64_t windows_opened  = 0;
    uint64_t windows_closed  = 0;
    uint64_t records_ingested = 0;
    uint64_t records_dropped  = 0;
    uint64_t late_records     = 0;
    uint64_t results_emitted  = 0;
    /// Backpressure counter for runtime limit enforcement.
    /// Incremented in two distinct situations:
    ///   - TumblingWindow / SessionWindow: the oldest open window or session
    ///     was **force-closed and evicted** to satisfy the max_open_windows /
    ///     max_open_sessions hard limit.
    ///   - SlidingWindow / HoppingWindow: creation of a new overlapping window
    ///     was **skipped** because the number of currently open windows already
    ///     reached max_open_windows (no eviction; the triggering record is
    ///     still ingested into existing windows).
    /// A non-zero value indicates the window is operating under backpressure.
    uint64_t windows_evicted  = 0;
    /// Incremented when a record is rejected because its non-empty partition_key
    /// was not yet seen and max_distinct_partition_keys was already reached.
    /// Only populated by TumblingWindow, SlidingWindow, and HoppingWindow;
    /// SessionWindow uses max_open_sessions for the equivalent cardinality cap.
    uint64_t partition_keys_rejected = 0;
};

// ============================================================================
// TumblingWindow
// ============================================================================

/**
 * Fixed, non-overlapping time-based windows.
 *
 *  |── window 1 ──|── window 2 ──|── window 3 ──|
 *
 * Each record falls into exactly one window.
 * On close, the window emits a WindowResult and resets.
 */
class TumblingWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    explicit TumblingWindow(const TumblingWindowConfig& config);
    ~TumblingWindow();

    // Non-copyable, movable
    TumblingWindow(const TumblingWindow&) = delete;
    TumblingWindow& operator=(const TumblingWindow&) = delete;

    /**
     * Register an aggregation to compute.
     * Must be called before the first ingest().
     */
    void addAggregation(const WindowAggregateSpec& spec);

    /**
     * Register a callback that is invoked when a window closes.
     */
    void setResultCallback(ResultCallback cb);

    /**
     * Ingest a record. Thread-safe.
     * Returns false if the record is older than the watermark and
     * late data is not allowed.
     */
    bool ingest(const StreamRecord& record);

    /**
     * Flush any open windows immediately (useful at shutdown).
     */
    void flush();

    WindowStats getStats() const;

private:
    TumblingWindowConfig config_;
    ResultCallback callback_;
    std::vector<WindowAggregateSpec> agg_specs_;

    struct InternalWindow {
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
        std::vector<StreamRecord> records;
        std::string partition_key = {};
        /// Tracks distinct non-empty partition_key values seen in this window.
        /// Used to enforce TumblingWindowConfig::max_distinct_partition_keys.
        std::unordered_set<std::string> seen_partition_keys;
    };

    // One window per time-slot
    std::map<int64_t, InternalWindow> open_windows_;  // key = slot index
    mutable std::mutex mutex_;

    // Watermark
    std::atomic<int64_t> watermark_us_{0};  // microseconds since epoch

    // Stats
    std::atomic<uint64_t> windows_opened_{0};
    std::atomic<uint64_t> windows_closed_{0};
    std::atomic<uint64_t> records_ingested_{0};
    std::atomic<uint64_t> records_dropped_{0};
    std::atomic<uint64_t> late_records_{0};
    std::atomic<uint64_t> results_emitted_{0};
    /// Incremented when a window is force-evicted due to max_open_windows limit.
    std::atomic<uint64_t> windows_evicted_{0};
    /// Incremented when a record is dropped because its partition_key exceeds
    /// max_distinct_partition_keys for the current window slot.
    std::atomic<uint64_t> partition_keys_rejected_{0};
    std::thread idle_thread_;
    std::atomic<bool> idle_running_{false};
    std::condition_variable idle_cv_;
    std::mutex idle_mutex_;
    std::atomic<int64_t> last_event_us_{0};

    int64_t slotIndex(const std::chrono::system_clock::time_point& tp) const;
    std::chrono::system_clock::time_point slotStart(int64_t idx) const;
    WindowResult computeResult(const InternalWindow& win, bool late) const;
    // Returns results to emit; caller fires the callback outside the mutex.
    std::vector<WindowResult> closeExpiredWindows(int64_t watermark_us);
    void updateWatermark(const std::chrono::system_clock::time_point& event_time);
    void idleTimeoutLoop();
};

std::unique_ptr<TumblingWindow> createTumblingWindow(const TumblingWindowConfig& config);

// ============================================================================
// SlidingWindow
// ============================================================================

/**
 * Overlapping windows: a record can appear in multiple windows.
 *
 *  |── W1 ──────────────|
 *       |── W2 ──────────────|
 *            |── W3 ──────────────|
 *
 * Window size  = config.size
 * Window slide = config.slide (how far to advance each new window start)
 *
 * Each arriving record is assigned to all currently open windows that
 * contain its event_time.
 */
class SlidingWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    explicit SlidingWindow(const SlidingWindowConfig& config);
    ~SlidingWindow();

    SlidingWindow(const SlidingWindow&) = delete;
    SlidingWindow& operator=(const SlidingWindow&) = delete;

    void addAggregation(const WindowAggregateSpec& spec);
    void setResultCallback(ResultCallback cb);

    /** Ingest a record. Thread-safe. */
    bool ingest(const StreamRecord& record);

    /** Flush all currently open windows. */
    void flush();

    WindowStats getStats() const;

private:
    SlidingWindowConfig config_;
    ResultCallback callback_;
    std::vector<WindowAggregateSpec> agg_specs_;

    struct InternalWindow {
        std::string window_id;
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
        std::vector<StreamRecord> records;
        bool closed = false;
        std::string partition_key;
    };

    std::deque<InternalWindow> windows_;
    mutable std::mutex mutex_;

    std::atomic<int64_t> watermark_us_{0};
    std::atomic<uint64_t> windows_opened_{0};
    std::atomic<uint64_t> windows_closed_{0};
    std::atomic<uint64_t> records_ingested_{0};
    std::atomic<uint64_t> records_dropped_{0};
    std::atomic<uint64_t> late_records_{0};
    std::atomic<uint64_t> results_emitted_{0};
    /// Incremented when new window creation is skipped due to max_open_windows limit.
    std::atomic<uint64_t> windows_evicted_{0};
    /// Incremented when a record is rejected because its partition_key was not yet
    /// seen and max_distinct_partition_keys was already reached.
    std::atomic<uint64_t> partition_keys_rejected_{0};

    // O(1) duplicate-detection index keyed on window start (TODO #5)
    std::unordered_set<int64_t> window_start_set_;
    /// Tracks distinct non-empty partition_key values seen across all ingest calls.
    /// Protected by mutex_. Used to enforce SlidingWindowConfig::max_distinct_partition_keys.
    std::unordered_set<std::string> seen_partition_keys_;

    // Idle-timeout background thread (TODO #1)
    std::thread idle_thread_;
    std::atomic<bool> idle_running_{false};
    std::condition_variable idle_cv_;
    std::mutex idle_mutex_;
    std::atomic<int64_t> last_event_us_{0};

    WindowResult computeResult(const InternalWindow& win, bool late) const;
    void ensureWindowsExist(const std::chrono::system_clock::time_point& event_time,
                            const std::string& partition_key);
    // Returns results to emit; caller fires the callback outside the mutex.
    std::vector<WindowResult> closeExpiredWindows(int64_t watermark_us);
    void updateWatermark(const std::chrono::system_clock::time_point& event_time);
    void idleTimeoutLoop();
    static std::string generateId();
};

std::unique_ptr<SlidingWindow> createSlidingWindow(const SlidingWindowConfig& config);

// ============================================================================
// SessionWindow
// ============================================================================

/**
 * Gap-based windows, one per partition key.
 * A session ends when no record for a partition arrives within the gap period.
 *
 * Session for user "alice":
 *   event → event → event → [gap] → NEW SESSION → event
 *
 * Sessions are keyed by partition_key so parallel sessions for different
 * partitions are independent.
 */
class SessionWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    explicit SessionWindow(const SessionWindowConfig& config);
    ~SessionWindow();

    SessionWindow(const SessionWindow&) = delete;
    SessionWindow& operator=(const SessionWindow&) = delete;

    void addAggregation(const WindowAggregateSpec& spec);
    void setResultCallback(ResultCallback cb);

    /** Ingest a record. Thread-safe. Returns false on late/dropped record. */
    bool ingest(const StreamRecord& record);

    /** Close all open sessions. */
    void flush();

    WindowStats getStats() const;

private:
    SessionWindowConfig config_;
    ResultCallback callback_;
    std::vector<WindowAggregateSpec> agg_specs_;

    struct Session {
        std::string session_id;
        std::string partition_key = {};
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point last_event;
        std::vector<StreamRecord> records;
        bool has_late_records = false;
    };

    std::unordered_map<std::string, Session> sessions_;  // keyed by partition_key
    mutable std::mutex mutex_;

    // Timer thread for session expiry
    std::thread expiry_thread_;
    std::atomic<bool> running_{false};
    std::condition_variable expiry_cv_;
    std::mutex expiry_mutex_;

    // Watermark (monotonically increasing, in microseconds since epoch)
    std::atomic<int64_t> watermark_us_{0};

    std::atomic<uint64_t> windows_opened_{0};
    std::atomic<uint64_t> windows_closed_{0};
    std::atomic<uint64_t> records_ingested_{0};
    std::atomic<uint64_t> records_dropped_{0};
    std::atomic<uint64_t> late_records_{0};
    std::atomic<uint64_t> results_emitted_{0};
    /// Incremented when a session is force-evicted due to max_open_sessions limit.
    std::atomic<uint64_t> windows_evicted_{0};

    WindowResult computeResult(const Session& s, bool late = false) const;
    void expiryLoop();
    static std::string generateId();
};

std::unique_ptr<SessionWindow> createSessionWindow(const SessionWindowConfig& config);

// ============================================================================
// HoppingWindow
// ============================================================================

/**
 * Overlapping windows with an explicit hop (advance) interval.
 * Logically identical to SlidingWindow but with clearer naming convention:
 *   - size  = total duration of each window
 *   - hop   = how often a new window starts
 *
 * When hop == size → same as TumblingWindow.
 * When hop < size  → overlapping windows (each record in size/hop windows).
 */
class HoppingWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    explicit HoppingWindow(const HoppingWindowConfig& config);
    ~HoppingWindow();

    HoppingWindow(const HoppingWindow&) = delete;
    HoppingWindow& operator=(const HoppingWindow&) = delete;

    void addAggregation(const WindowAggregateSpec& spec);
    void setResultCallback(ResultCallback cb);

    /** Ingest a record. Thread-safe. */
    bool ingest(const StreamRecord& record);

    /** Flush all open windows. */
    void flush();

    WindowStats getStats() const;

private:
    HoppingWindowConfig config_;
    ResultCallback callback_;
    std::vector<WindowAggregateSpec> agg_specs_;

    struct InternalWindow {
        std::string window_id;
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
        std::vector<StreamRecord> records;
        bool closed = false;
    };

    std::deque<InternalWindow> windows_;
    mutable std::mutex mutex_;

    std::atomic<int64_t> watermark_us_{0};
    std::atomic<uint64_t> windows_opened_{0};
    std::atomic<uint64_t> windows_closed_{0};
    std::atomic<uint64_t> records_ingested_{0};
    std::atomic<uint64_t> records_dropped_{0};
    std::atomic<uint64_t> late_records_{0};
    std::atomic<uint64_t> results_emitted_{0};
    /// Incremented when new window creation is skipped due to max_open_windows limit.
    std::atomic<uint64_t> windows_evicted_{0};
    /// Incremented when a record is rejected because its partition_key was not yet
    /// seen and max_distinct_partition_keys was already reached.
    std::atomic<uint64_t> partition_keys_rejected_{0};

    // O(1) duplicate-detection index keyed on window start (TODO #5)
    std::unordered_set<int64_t> window_start_set_;
    /// Tracks distinct non-empty partition_key values seen across all ingest calls.
    /// Protected by mutex_. Used to enforce HoppingWindowConfig::max_distinct_partition_keys.
    std::unordered_set<std::string> seen_partition_keys_;

    WindowResult computeResult(const InternalWindow& win, bool late) const;
    void ensureWindowsExist(const std::chrono::system_clock::time_point& event_time);
    // Returns results to emit; caller fires the callback outside the mutex.
    std::vector<WindowResult> closeExpiredWindows(int64_t watermark_us);
    void updateWatermark(const std::chrono::system_clock::time_point& event_time);
    static std::string generateId();
};

/** @brief Factory function for HoppingWindow. */
std::unique_ptr<HoppingWindow> createHoppingWindow(const HoppingWindowConfig& config);

// ============================================================================
// StreamingWindowPipeline
// ============================================================================

/**
 * Fluent builder for streaming window pipelines.
 *
 * Example usage:
 * @code
 *   auto pipeline = StreamingWindowPipeline::tumbling(std::chrono::minutes(1))
 *       .aggregate({"total", AggFunc::SUM, "amount"})
 *       .aggregate({"count", AggFunc::COUNT, ""})
 *       .onResult([](const WindowResult& r) {
 *           // handle result
 *       })
 *       .build();
 *
 *   pipeline->ingest(record);
 *   pipeline->flush();
 * @endcode
 */
class StreamingWindowPipeline {
public:
    /// Window type selector
    enum class Type { TUMBLING, SLIDING, SESSION, HOPPING };

    struct Config {
        Type type = Type::TUMBLING;
        std::chrono::milliseconds size{60000};
        std::chrono::milliseconds slide{10000};
        std::chrono::milliseconds hop{10000};
        std::chrono::milliseconds gap{30000};
        WatermarkConfig watermark;
        std::chrono::milliseconds session_expiry_interval_ms{200};
    };

    // ---- Static factory methods (fluent interface) ----

    static StreamingWindowPipeline tumbling(std::chrono::milliseconds size,
                                             WatermarkConfig wm = {});
    static StreamingWindowPipeline sliding(std::chrono::milliseconds size,
                                            std::chrono::milliseconds slide,
                                            WatermarkConfig wm = {});
    static StreamingWindowPipeline session(std::chrono::milliseconds gap,
                                            WatermarkConfig wm = {},
                                            std::chrono::milliseconds expiry_interval_ms = std::chrono::milliseconds{200});
    static StreamingWindowPipeline hopping(std::chrono::milliseconds size,
                                            std::chrono::milliseconds hop,
                                            WatermarkConfig wm = {});

    // ---- Builder methods ----

    StreamingWindowPipeline& aggregate(const WindowAggregateSpec& spec);
    StreamingWindowPipeline& onResult(std::function<void(WindowResult)> callback);

    /**
     * Finalize the pipeline; returns a shared_ptr to the window
     * (one of Tumbling/Sliding/Session/Hopping) wrapped in a uniform interface.
     */
    std::shared_ptr<StreamingWindowPipeline> build();

    // ---- Runtime interface (available after build()) ----

    /** Feed a record into the pipeline. */
    bool ingest(const StreamRecord& record);

    /** Flush all pending windows and emit results. */
    void flush();

    WindowStats getStats() const;

private:
    Config config_;
    std::vector<WindowAggregateSpec> agg_specs_;
    std::function<void(WindowResult)> callback_;

    // Underlying window after build()
    std::shared_ptr<TumblingWindow> tumbling_;
    std::shared_ptr<SlidingWindow>  sliding_;
    std::shared_ptr<SessionWindow>  session_;
    std::shared_ptr<HoppingWindow>  hopping_;

    bool built_ = false;
};

// ============================================================================
// Utility helpers
// ============================================================================

/**
 * Helper to create a StreamRecord with a given event_time and field map.
 */
StreamRecord makeRecord(
    const std::string& id,
    std::chrono::system_clock::time_point event_time,
    const std::string& partition_key = "",
    std::initializer_list<std::pair<std::string, RecordValue>> fields = {});

/**
 * Convert AggFunc to human-readable string.
 */
inline const char* aggFuncToString(AggFunc f) {
    switch (f) {
        case AggFunc::COUNT:         return "COUNT";
        case AggFunc::SUM:           return "SUM";
        case AggFunc::AVG:           return "AVG";
        case AggFunc::MIN:           return "MIN";
        case AggFunc::MAX:           return "MAX";
        case AggFunc::STDDEV:        return "STDDEV";
        case AggFunc::VARIANCE:      return "VARIANCE";
        case AggFunc::PERCENTILE:    return "PERCENTILE";
        case AggFunc::FIRST:         return "FIRST";
        case AggFunc::LAST:          return "LAST";
        case AggFunc::DISTINCT_COUNT:return "DISTINCT_COUNT";
        default:                     return "UNKNOWN";
    }
}

} // namespace analytics
} // namespace themisdb
