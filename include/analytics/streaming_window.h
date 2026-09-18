/**
 * @file streaming_window.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.32
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

using RecordValue = std::variant<
    std::monostate,   // null
    bool,
    int64_t,
    double,
    std::string
>;

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

    /**
     * @brief Set.
     * @param[in] field Input parameter.
     * @param[in] value Input parameter.
     * @details Calls: std::move().
     */
    void set(const std::string& field, RecordValue value) {
        fields[field] = std::move(value);
    }
};

// ============================================================================
// Aggregation spec & result
// ============================================================================

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

struct AggregatedValue {
    std::string name;
    AggFunc     func;
    RecordValue value;
    uint64_t    count = 0;
};

struct WindowResult {
    std::string window_id;
    std::chrono::system_clock::time_point window_start;
    std::chrono::system_clock::time_point window_end;
    std::string partition_key;          ///< non-empty for session windows
    uint64_t    record_count = 0;
    std::vector<AggregatedValue> aggregations;
    bool        is_late_firing = false; ///< true when triggered by late data
    bool        is_early_firing = false;///< true when triggered before window close

    /**
     * @brief Get.
     * @param[in] agg_name Name of the agg.
     * @return Return value.
     */
    std::optional<RecordValue> get(const std::string& agg_name) const;
};

// ============================================================================
// Watermark configuration
// ============================================================================

struct WatermarkConfig {
    std::chrono::milliseconds max_out_of_orderness{0};
    std::chrono::milliseconds idle_timeout{60000};
    bool allow_late_data = false;
};

// ============================================================================
// Window configuration structs
// ============================================================================

struct TumblingWindowConfig {
    std::chrono::milliseconds size{60000};
    WatermarkConfig watermark;
    bool emit_empty_windows = false;
    uint64_t max_open_windows = 0;
    uint64_t max_records_per_window = 0;
    uint64_t max_distinct_partition_keys = 0;
};

struct SlidingWindowConfig {
    std::chrono::milliseconds size{60000};
    std::chrono::milliseconds slide{10000};
    WatermarkConfig watermark;
    uint64_t max_open_windows = 0;
    uint64_t max_records_per_window = 0;
    uint64_t max_distinct_partition_keys = 0;
};

struct SessionWindowConfig {
    std::chrono::milliseconds gap{30000};
    WatermarkConfig watermark;
    std::chrono::milliseconds session_expiry_check_interval_ms{200};
    uint64_t max_open_sessions = 0;
    uint64_t max_records_per_session = 0;
};

struct HoppingWindowConfig {
    std::chrono::milliseconds size{60000};
    std::chrono::milliseconds hop{10000};
    WatermarkConfig watermark;
    uint64_t max_open_windows = 0;
    uint64_t max_records_per_window = 0;
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
    uint64_t windows_evicted  = 0;
    uint64_t partition_keys_rejected = 0;
};

// ============================================================================
// TumblingWindow
// ============================================================================

class TumblingWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    /**
     * @brief Tumbling Window.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TumblingWindow(const TumblingWindowConfig& config);
    ~TumblingWindow();

    // Non-copyable, movable
    TumblingWindow(const TumblingWindow&) = delete;
    TumblingWindow& operator=(const TumblingWindow&) = delete;

    /**
     * @brief Add Aggregation.
     * @param[in] spec Input parameter.
     */
    void addAggregation(const WindowAggregateSpec& spec);

    /**
     * @brief Set Result Callback.
     * @param[in] cb Input parameter.
     */
    void setResultCallback(ResultCallback cb);

    /**
     * @brief Ingest.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */
    bool ingest(const StreamRecord& record);

    /**
     * @brief Flush.
     */
    void flush();

    /**
     * @brief Get Stats.
     * @return Return value.
     */
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
    std::atomic<uint64_t> windows_evicted_{0};
    std::atomic<uint64_t> partition_keys_rejected_{0};
    std::thread idle_thread_;
    std::atomic<bool> idle_running_{false};
    std::condition_variable idle_cv_;
    std::mutex idle_mutex_;
    std::atomic<int64_t> last_event_us_{0};

    /**
     * @brief Slot Index.
     * @param[in] tp Input parameter.
     * @return Return value.
     */
    int64_t slotIndex(const std::chrono::system_clock::time_point& tp) const;
    /**
     * @brief Slot Start.
     * @param[in] idx Input parameter.
     * @return Return value.
     */
    std::chrono::system_clock::time_point slotStart(int64_t idx) const;
    /**
     * @brief Compute Result.
     * @param[in] win Input parameter.
     * @param[in] late Input parameter.
     * @return Return value.
     */
    WindowResult computeResult(const InternalWindow& win, bool late) const;
    /**
     * @brief Returns results to emit; caller fires the callback outside the mutex.
     * @param[in] watermark_us Input parameter.
     * @return Return value.
     */
    std::vector<WindowResult> closeExpiredWindows(int64_t watermark_us);
    /**
     * @brief Update Watermark.
     * @param[in] event_time Input parameter.
     */
    void updateWatermark(const std::chrono::system_clock::time_point& event_time);
    /**
     * @brief Idle Timeout Loop.
     */
    void idleTimeoutLoop();
};

/**
 * @brief Create Tumbling Window.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<TumblingWindow> createTumblingWindow(const TumblingWindowConfig& config);

// ============================================================================
// SlidingWindow
// ============================================================================

class SlidingWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    /**
     * @brief Sliding Window.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SlidingWindow(const SlidingWindowConfig& config);
    ~SlidingWindow();

    SlidingWindow(const SlidingWindow&) = delete;
    SlidingWindow& operator=(const SlidingWindow&) = delete;

    /**
     * @brief Add Aggregation.
     * @param[in] spec Input parameter.
     */
    void addAggregation(const WindowAggregateSpec& spec);
    /**
     * @brief Set Result Callback.
     * @param[in] cb Input parameter.
     */
    void setResultCallback(ResultCallback cb);

    /**
     * @brief Ingest.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */
    bool ingest(const StreamRecord& record);

    /**
     * @brief Flush.
     */
    void flush();

    /**
     * @brief Get Stats.
     * @return Return value.
     */
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
    std::atomic<uint64_t> windows_evicted_{0};
    std::atomic<uint64_t> partition_keys_rejected_{0};

    // O(1) duplicate-detection index keyed on window start (TODO #5)
    std::unordered_set<int64_t> window_start_set_;
    std::unordered_set<std::string> seen_partition_keys_;

    // Idle-timeout background thread (TODO #1)
    std::thread idle_thread_;
    std::atomic<bool> idle_running_{false};
    std::condition_variable idle_cv_;
    std::mutex idle_mutex_;
    std::atomic<int64_t> last_event_us_{0};

    /**
     * @brief Compute Result.
     * @param[in] win Input parameter.
     * @param[in] late Input parameter.
     * @return Return value.
     */
    WindowResult computeResult(const InternalWindow& win, bool late) const;
    /**
     * @brief Ensure Windows Exist.
     * @param[in] event_time Input parameter.
     * @param[in] partition_key Input parameter.
     */
    void ensureWindowsExist(const std::chrono::system_clock::time_point& event_time,
                            const std::string& partition_key);
    /**
     * @brief Returns results to emit; caller fires the callback outside the mutex.
     * @param[in] watermark_us Input parameter.
     * @return Return value.
     */
    std::vector<WindowResult> closeExpiredWindows(int64_t watermark_us);
    /**
     * @brief Update Watermark.
     * @param[in] event_time Input parameter.
     */
    void updateWatermark(const std::chrono::system_clock::time_point& event_time);
    /**
     * @brief Idle Timeout Loop.
     */
    void idleTimeoutLoop();
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    static std::string generateId();
};

/**
 * @brief Create Sliding Window.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<SlidingWindow> createSlidingWindow(const SlidingWindowConfig& config);

// ============================================================================
// SessionWindow
// ============================================================================

class SessionWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    /**
     * @brief Session Window.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SessionWindow(const SessionWindowConfig& config);
    ~SessionWindow();

    SessionWindow(const SessionWindow&) = delete;
    SessionWindow& operator=(const SessionWindow&) = delete;

    /**
     * @brief Add Aggregation.
     * @param[in] spec Input parameter.
     */
    void addAggregation(const WindowAggregateSpec& spec);
    /**
     * @brief Set Result Callback.
     * @param[in] cb Input parameter.
     */
    void setResultCallback(ResultCallback cb);

    /**
     * @brief Ingest.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */
    bool ingest(const StreamRecord& record);

    /**
     * @brief Flush.
     */
    void flush();

    /**
     * @brief Get Stats.
     * @return Return value.
     */
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
    std::atomic<uint64_t> windows_evicted_{0};

    WindowResult computeResult(const Session& s, bool late = false) const;
    /**
     * @brief Expiry Loop.
     */
    void expiryLoop();
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    static std::string generateId();
};

/**
 * @brief Create Session Window.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<SessionWindow> createSessionWindow(const SessionWindowConfig& config);

// ============================================================================
// HoppingWindow
// ============================================================================

class HoppingWindow {
public:
    using ResultCallback = std::function<void(WindowResult)>;

    /**
     * @brief Hopping Window.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HoppingWindow(const HoppingWindowConfig& config);
    ~HoppingWindow();

    HoppingWindow(const HoppingWindow&) = delete;
    HoppingWindow& operator=(const HoppingWindow&) = delete;

    /**
     * @brief Add Aggregation.
     * @param[in] spec Input parameter.
     */
    void addAggregation(const WindowAggregateSpec& spec);
    /**
     * @brief Set Result Callback.
     * @param[in] cb Input parameter.
     */
    void setResultCallback(ResultCallback cb);

    /**
     * @brief Ingest.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */
    bool ingest(const StreamRecord& record);

    /**
     * @brief Flush.
     */
    void flush();

    /**
     * @brief Get Stats.
     * @return Return value.
     */
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
    std::atomic<uint64_t> windows_evicted_{0};
    std::atomic<uint64_t> partition_keys_rejected_{0};

    // O(1) duplicate-detection index keyed on window start (TODO #5)
    std::unordered_set<int64_t> window_start_set_;
    std::unordered_set<std::string> seen_partition_keys_;

    /**
     * @brief Compute Result.
     * @param[in] win Input parameter.
     * @param[in] late Input parameter.
     * @return Return value.
     */
    WindowResult computeResult(const InternalWindow& win, bool late) const;
    /**
     * @brief Ensure Windows Exist.
     * @param[in] event_time Input parameter.
     */
    void ensureWindowsExist(const std::chrono::system_clock::time_point& event_time);
    /**
     * @brief Returns results to emit; caller fires the callback outside the mutex.
     * @param[in] watermark_us Input parameter.
     * @return Return value.
     */
    std::vector<WindowResult> closeExpiredWindows(int64_t watermark_us);
    /**
     * @brief Update Watermark.
     * @param[in] event_time Input parameter.
     */
    void updateWatermark(const std::chrono::system_clock::time_point& event_time);
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    static std::string generateId();
};

/**
 * @brief Create Hopping Window.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<HoppingWindow> createHoppingWindow(const HoppingWindowConfig& config);

// ============================================================================
// StreamingWindowPipeline
// ============================================================================

class StreamingWindowPipeline {
public:
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

    /**
     * @brief ---- Builder methods ----
     * @param[in] spec Input parameter.
     * @return Return value.
     */

    StreamingWindowPipeline& aggregate(const WindowAggregateSpec& spec);
    StreamingWindowPipeline& onResult(std::function<void(WindowResult)> callback);

    /**
     * @brief Build.
     * @return Return value.
     */
    std::shared_ptr<StreamingWindowPipeline> build();

    /**
     * @brief ---- Runtime interface (available after build()) ----
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */

    bool ingest(const StreamRecord& record);

    /**
     * @brief Flush.
     */
    void flush();

    /**
     * @brief Get Stats.
     * @return Return value.
     */
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

StreamRecord makeRecord(
    const std::string& id,
    std::chrono::system_clock::time_point event_time,
    const std::string& partition_key = "",
    std::initializer_list<std::pair<std::string, RecordValue>> fields = {});

/**
 * @brief Agg Func To String.
 * @param[in] f Input parameter.
 * @return Pointer to the result.
 * @details Implements aggFuncToString without additional internal calls.
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
