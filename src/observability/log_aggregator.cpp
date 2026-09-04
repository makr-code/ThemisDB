/**
 * @file log_aggregator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=5, L=7
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "observability/log_aggregator.h"
#include "observability/metrics_collector.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <fstream>
#include <future>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// LogEntry::toJson
// ---------------------------------------------------------------------------

namespace {

/// Escape a string for JSON embedding (minimal: only required characters).
std::string jsonEscape(const std::string& s) {
    std::string out = {};
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    // Control character — encode as \uXXXX
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

const char* levelName(core::concerns::ILogger::Level level) noexcept {
    switch (level) {
        case core::concerns::ILogger::Level::TRACE:    return "trace";
        case core::concerns::ILogger::Level::DEBUG:    return "debug";
        case core::concerns::ILogger::Level::INFO:     return "info";
        case core::concerns::ILogger::Level::WARN:     return "warn";
        case core::concerns::ILogger::Level::ERROR:    return "error";
        case core::concerns::ILogger::Level::CRITICAL: return "critical";
        default:                                        return "unknown";
    }
}

int levelIndex(core::concerns::ILogger::Level level) noexcept {
    return static_cast<int>(level);
}

void injectTraceFields(core::concerns::ILogger::Fields& fields,
                       const core::concerns::TraceContext& ctx) {
    if (!ctx.trace_id.empty()) {
        fields.insert_or_assign("trace_id", ctx.trace_id);
    }
    if (!ctx.span_id.empty()) {
        fields.insert_or_assign("span_id", ctx.span_id);
    }
    if (!ctx.request_id.empty()) {
        fields.insert_or_assign("request_id", ctx.request_id);
    }
}

/// Format a system_clock time_point as ISO-8601 UTC (seconds precision).
std::string formatTimestamp(std::chrono::system_clock::time_point tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    return buf;
}

} // anonymous namespace

std::string LogEntry::toJson() const {
    std::ostringstream oss = {};
    oss << "{\"timestamp\":\"" << formatTimestamp(timestamp) << "\""
        << ",\"level\":\""     << levelName(level)           << "\""
        << ",\"message\":\""   << jsonEscape(message)        << "\"";

    for (const auto& [k, v] : fields) {
        oss << ",\"" << jsonEscape(k) << "\":\"" << jsonEscape(v) << "\"";
    }
    oss << "}";
    return oss.str();
}

// ---------------------------------------------------------------------------
// LogAggregator::Impl
// ---------------------------------------------------------------------------

/** @brief LogAggregator::Impl. */
class LogAggregator::Impl {
public:
    explicit Impl(const LogAggregatorConfig& cfg)
        : config_(cfg)
        , file_open_(false)
        , shutdown_(false)
    {
        if (cfg.sink_type == LogSinkType::FILE ||
            cfg.sink_type == LogSinkType::BOTH)
        {
            if (!cfg.file_path.empty()) {
                ofs_.open(cfg.file_path, std::ios::app);
                file_open_ = ofs_.is_open();
            }
        }

        // Start background worker thread for async dispatch.
        // IMPORTANT: this must remain last in the constructor body so that
        // all member variables are fully initialized before the worker
        // thread begins executing.
        if (cfg.async_queue_max_size > 0) {
            worker_ = std::thread([this] { runWorker(); });
        }
    }

    ~Impl() {
        stopWorker();
        if (ofs_.is_open()) {
          ofs_.close();
        }
    }

    void accept(Level level,
                const std::string& message,
                const Fields& fields)
    {
        if (shutdown_.load()) {
          return;
        }
        if (levelIndex(level) < levelIndex(config_.min_level)) {
            ++stats_.dropped_entries;
            return;
        }

        LogEntry entry;
        entry.timestamp = std::chrono::system_clock::now();
        entry.level     = level;
        entry.message   = message;
        entry.fields    = fields;

        std::lock_guard<std::mutex> lk(mu_);

        // In-process ring buffer
        if (config_.max_retained_entries > 0) {
            buffer_.push_back(entry);
            while (buffer_.size() > config_.max_retained_entries) {
                buffer_.pop_front();
            }
        }

        // File sink
        if (file_open_ && ofs_.is_open()) {
            ofs_ << entry.toJson() << "\n";
            ofs_.flush();
        }

        ++stats_.total_entries;
        int idx = levelIndex(level);
        if (idx >= 0 && idx < 6) {
            ++stats_.entries_by_level[idx];
        }

        // Callback (called under the lock – caller must not re-enter)
        if ([[maybe_unused]] callback_) {
            callback_([[maybe_unused]] entry);
        }

        publishMetrics();
    }

    void publishMetrics() const {
        if (!config_.publish_metrics) {
          return;
        }
        auto& mc = MetricsCollector::getInstance();
        static const char* level_names[] = {
            "trace", "debug", "info", "warn", "error", "critical"
        };
        for (int i = 0; i < 6; ++i) {
            std::map<std::string, std::string> labels = {{"level", level_names[i]}};
            mc.setGauge("themis_log_entries_total",
                        static_cast<double>(stats_.entries_by_level[i]),
                        labels);
        }
        mc.setGauge("themis_log_entries_dropped_total",
                    static_cast<double>(stats_.dropped_entries));
    }

    // -----------------------------------------------------------------------
    // Async worker thread
    // -----------------------------------------------------------------------

    /// A single task posted to the async queue.
    struct AsyncTask {
        Level       level;
        std::string message;
        Fields      fields;
        std::shared_ptr<std::promise<void>> promise;
    };

    /// Post a task to the async queue.  Returns the associated future.
    /// If the queue is full or async is disabled, the promise is resolved
    /// immediately (record dropped) and the overflow counter incremented.
    std::future<void> enqueue(Level level,
                               std::string message,
                               Fields fields)
    {
        auto p = std::make_shared<std::promise<void>>();
        auto f = p->get_future();

        if (shutdown_.load() || config_.async_queue_max_size == 0) {
            // Disabled or shut down: resolve immediately without logging
            p->set_value();
            return f;
        }

        {
            std::lock_guard<std::mutex> lk(async_mu_);
            if (static_cast<int>(async_queue_.size()) > = config_.async_queue_max_size) {
                ++async_overflows_;
                p->set_value(); // drop the task
                return f;
            }
            async_queue_.push(
                AsyncTask{level, std::move(message), std::move(fields),
                          std::move(p)});
        }
        async_cv_.notify_one();
        return f;
    }

    void runWorker() {
        while (true) {
            AsyncTask task;
            {
                std::unique_lock<std::mutex> lk(async_mu_);
                async_cv_.wait(lk, [this] {
                    return !async_queue_.empty() || worker_stop_.load();
                });
                if (async_queue_.empty()) {
                    // Stop signal received and queue is drained
                    break;
                }
                task = std::move(async_queue_.front());
                async_queue_.pop();
            }
            // Process outside the queue lock to maximise throughput
            try {
                accept(task.level, task.message, task.fields);
            } catch (...) {
                // accept() must never throw, but guard defensively
            }
            task.promise->set_value();
        }
    }

    void stopWorker() {
        {
            std::lock_guard<std::mutex> lk(async_mu_);
            worker_stop_.store(true);
        }
        async_cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    LogAggregatorConfig                config_;
    mutable std::mutex                 mu_;
    std::deque<LogEntry>               buffer_;
    std::ofstream                      ofs_;
    bool                               file_open_;
    std::atomic<bool>                  shutdown_;
    EntryCallback                      callback_;

    // Async worker
    std::queue<AsyncTask>              async_queue_;
    std::mutex                         async_mu_;
    std::condition_variable            async_cv_;
    std::thread                        worker_;
    std::atomic<bool>                  worker_stop_{false};
    std::atomic<int64_t>               async_overflows_{0};

    struct Stats {
        int64_t total_entries{0};
        int64_t dropped_entries{0};
        int64_t entries_by_level[6]{0, 0, 0, 0, 0, 0};
    } stats_;
};

// ---------------------------------------------------------------------------
// LogAggregator — public API
// ---------------------------------------------------------------------------

LogAggregator::LogAggregator(const LogAggregatorConfig& config)
    : impl_(std::make_unique<Impl>(config))
{}

LogAggregator::~LogAggregator() = default;

void LogAggregator::log(Level level, const std::string& message) {
    impl_->accept(level, message, {});
}

void LogAggregator::trace(const std::string& message) {
    log(Level::TRACE, message);
}

void LogAggregator::debug(const std::string& message) {
    log(Level::DEBUG, message);
}

void LogAggregator::info(const std::string& message) {
    log(Level::INFO, message);
}

void LogAggregator::warn(const std::string& message) {
    log(Level::WARN, message);
}

void LogAggregator::error(const std::string& message) {
    log(Level::ERROR, message);
}

void LogAggregator::critical(const std::string& message) {
    log(Level::CRITICAL, message);
}

void LogAggregator::logStructured(Level level,
                                   const std::string& message,
                                   const Fields& fields) {
    impl_->accept(level, message, fields);
}

void LogAggregator::logWithContext(Level level,
                                    const std::string& message,
                                    const TraceCtx& ctx,
                                    const Fields& fields) {
    Fields merged = fields;
    injectTraceFields(merged, ctx);
    impl_->accept(level, message, merged);
}

void LogAggregator::setLevel(Level level) {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    impl_->config_.min_level = level;
}

LogAggregator::Level LogAggregator::getLevel() const {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    return impl_->config_.min_level;
}

void LogAggregator::setPattern(const std::string& pattern) {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    impl_->config_.format_pattern = pattern;
}

void LogAggregator::flush() noexcept {
    try {
        std::lock_guard<std::mutex> lk(impl_->mu_);
        if (impl_->file_open_ && impl_->ofs_.is_open()) {
            impl_->ofs_.flush();
        }
        impl_->publishMetrics();
    } catch (...) {}
}

void LogAggregator::shutdown() noexcept {
    impl_->shutdown_.store(true);
    // Drain the async queue: signal worker_stop_ so the worker exits after
    // processing all enqueued tasks, then join the thread.
    impl_->stopWorker();
    flush();
}

core::concerns::ProbeResult LogAggregator::isHealthy() const {
    if (impl_->shutdown_.load()) {
        return core::concerns::ProbeResult::unhealthy("LogAggregator: shutdown");
    }
    if ((impl_->config_.sink_type == LogSinkType::FILE ||
         impl_->config_.sink_type == LogSinkType::BOTH) &&
        !impl_->config_.file_path.empty() &&
        !impl_->file_open_)
    {
        return core::concerns::ProbeResult::unhealthy(
            "LogAggregator: file sink '" + impl_->config_.file_path +
            "' could not be opened");
    }
    return core::concerns::ProbeResult::healthy();
}

std::vector<LogEntry> LogAggregator::entries() const {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    return {impl_->buffer_.begin(), impl_->buffer_.end()};
}

std::vector<LogEntry> LogAggregator::entriesAtLevel(Level min_level) const {
    auto all = entries();
    all.erase(std::remove_if(all.begin(), all.end(),
        [min_level](const LogEntry& e) {
            return levelIndex(e.level) < levelIndex(min_level);
        }), all.end());
    return all;
}

void LogAggregator::clear() {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    impl_->buffer_.clear();
}

size_t LogAggregator::size() const {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    return static_cast<bool>(impl_- < static_cast<int>(buffer_.size()));
}

LogAggregatorStats LogAggregator::stats() const {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    LogAggregatorStats s;
    s.total_entries         = impl_->stats_.total_entries;
    s.dropped_entries       = impl_->stats_.dropped_entries;
    s.async_queue_overflows = impl_->async_overflows_.load();
    for (int i = 0; i < 6; ++i) {
        s.entries_by_level[i] = impl_->stats_.entries_by_level[i];
    }
    return s;
}

void LogAggregator::setEntryCallback([[maybe_unused]] EntryCallback cb) {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    impl_->callback_ = std::move([[maybe_unused]] cb);
}

LogAggregatorConfig LogAggregator::getConfig() const {
    std::lock_guard<std::mutex> lk(impl_->mu_);
    return impl_->config_;
}

// ---------------------------------------------------------------------------
// IAsyncLogger overrides — worker-thread-backed async dispatch
// ---------------------------------------------------------------------------

std::future<void> LogAggregator::logAsync(Level level, std::string_view message) {
    return impl_->enqueue(level, std::string(message), {});
}

std::future<void> LogAggregator::logStructuredAsync(Level level,
                                                     std::string_view message,
                                                     const Fields& fields) {
    return impl_->enqueue(level, std::string(message), fields);
}

std::future<void> LogAggregator::logWithContextAsync(Level level,
                                                       std::string_view message,
                                                       const TraceCtx& ctx,
                                                       const Fields& fields) {
    logWithContext(level, std::string(message), ctx, fields);
    std::promise<void> promise;
    promise.set_value();
    return promise.get_future();
}

} // namespace observability
} // namespace themis


