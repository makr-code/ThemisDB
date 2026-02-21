/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            streaming_window.cpp                               ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 19:20:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     933                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Streaming Aggregation Windows - Implementation
 *
 * Full implementation of all window types declared in
 * include/analytics/streaming_window.h.
 *
 * Design notes:
 *  - Event-time semantics with configurable watermarking
 *  - Thread-safe ingestion (mutex per window)
 *  - Watermark advances monotonically on each ingested record
 *  - Shared aggregation computation logic via anonymous namespace helpers
 *  - Session expiry via background thread (SessionWindow only)
 */

#include "analytics/streaming_window.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <random>
#include <set>
#include <sstream>

#include <spdlog/spdlog.h>

namespace themisdb {
namespace analytics {

// ============================================================================
// Anonymous helpers
// ============================================================================

namespace {

std::string genId() {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    uint64_t a = dist(rng), b = dist(rng);
    char buf[37];
    std::snprintf(buf, sizeof(buf),
        "%08x-%04x-%04x-%04x-%012llx",
        static_cast<unsigned>(a >> 32),
        static_cast<unsigned>((a >> 16) & 0xFFFF),
        static_cast<unsigned>(a & 0xFFFF),
        static_cast<unsigned>((b >> 48) & 0xFFFF),
        static_cast<unsigned long long>(b & 0x0000'FFFF'FFFF'FFFFUL));
    return std::string(buf);
}

int64_t toMicros(const std::chrono::system_clock::time_point& tp) {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        tp.time_since_epoch()).count();
}

std::chrono::system_clock::time_point fromMicros(int64_t us) {
    return std::chrono::system_clock::time_point(std::chrono::microseconds(us));
}

double toDouble(const RecordValue& v) {
    if (auto* d = std::get_if<double>(&v))   return *d;
    if (auto* i = std::get_if<int64_t>(&v))  return static_cast<double>(*i);
    if (auto* b = std::get_if<bool>(&v))      return *b ? 1.0 : 0.0;
    return 0.0;
}

std::string rvToString(const RecordValue& v) {
    if (std::holds_alternative<std::monostate>(v)) return "";
    if (auto* s = std::get_if<std::string>(&v))    return *s;
    if (auto* i = std::get_if<int64_t>(&v))        return std::to_string(*i);
    if (auto* d = std::get_if<double>(&v))         return std::to_string(*d);
    if (auto* b = std::get_if<bool>(&v))           return *b ? "true" : "false";
    return "";
}

double calcPercentile(std::vector<double> vals, double p) {
    if (vals.empty()) return 0.0;
    std::sort(vals.begin(), vals.end());
    if (p <= 0.0)   return vals.front();
    if (p >= 100.0) return vals.back();
    double idx  = (p / 100.0) * static_cast<double>(vals.size() - 1);
    size_t lo   = static_cast<size_t>(idx);
    size_t hi   = lo + 1;
    if (hi >= vals.size()) return vals.back();
    double frac = idx - static_cast<double>(lo);
    return vals[lo] + frac * (vals[hi] - vals[lo]);
}

/**
 * Compute all aggregations from a flat list of records.
 */
std::vector<AggregatedValue> computeAggregations(
    const std::vector<StreamRecord>& records,
    const std::vector<AggregateSpec>& specs)
{
    std::vector<AggregatedValue> results;
    results.reserve(specs.size());

    for (const auto& spec : specs) {
        AggregatedValue av;
        av.name  = spec.name;
        av.func  = spec.func;
        av.count = static_cast<uint64_t>(records.size());

        // Collect numeric values for all records
        std::vector<double>      nums;
        std::set<std::string>    distinct;
        RecordValue              first_val{std::monostate{}};
        RecordValue              last_val{std::monostate{}};
        bool                     has_first = false;
        double                   sum_val   = 0.0;
        double                   min_val   = std::numeric_limits<double>::max();
        double                   max_val   = std::numeric_limits<double>::lowest();

        for (const auto& rec : records) {
            RecordValue fv{std::monostate{}};
            if (!spec.field.empty()) {
                auto it = rec.fields.find(spec.field);
                if (it != rec.fields.end()) fv = it->second;
            }
            double d = toDouble(fv);
            sum_val += d;
            if (d < min_val) min_val = d;
            if (d > max_val) max_val = d;
            nums.push_back(d);
            distinct.insert(rvToString(fv));
            last_val = fv;
            if (!has_first) { first_val = fv; has_first = true; }
        }

        uint64_t n = static_cast<uint64_t>(records.size());
        switch (spec.func) {
            case AggFunc::COUNT:
                av.value = static_cast<int64_t>(n);
                break;
            case AggFunc::SUM:
                av.value = sum_val;
                break;
            case AggFunc::AVG:
                av.value = (n > 0) ? sum_val / static_cast<double>(n) : 0.0;
                break;
            case AggFunc::MIN:
                av.value = (n > 0) ? min_val : 0.0;
                break;
            case AggFunc::MAX:
                av.value = (n > 0) ? max_val : 0.0;
                break;
            case AggFunc::STDDEV: {
                if (n < 2) { av.value = 0.0; break; }
                double mean = sum_val / static_cast<double>(n);
                double var  = 0.0;
                for (double v : nums) var += (v - mean) * (v - mean);
                av.value = std::sqrt(var / static_cast<double>(n - 1));
                break;
            }
            case AggFunc::VARIANCE: {
                if (n < 2) { av.value = 0.0; break; }
                double mean = sum_val / static_cast<double>(n);
                double var  = 0.0;
                for (double v : nums) var += (v - mean) * (v - mean);
                av.value = var / static_cast<double>(n - 1);
                break;
            }
            case AggFunc::PERCENTILE:
                av.value = calcPercentile(nums, spec.percentile_p);
                break;
            case AggFunc::FIRST:
                av.value = first_val;
                break;
            case AggFunc::LAST:
                av.value = last_val;
                break;
            case AggFunc::DISTINCT_COUNT:
                av.value = static_cast<int64_t>(distinct.size());
                break;
        }

        results.push_back(std::move(av));
    }
    return results;
}

} // anonymous namespace

// ============================================================================
// WindowResult
// ============================================================================

std::optional<RecordValue> WindowResult::get(const std::string& agg_name) const {
    for (const auto& av : aggregations) {
        if (av.name == agg_name) return av.value;
    }
    return std::nullopt;
}

// ============================================================================
// makeRecord helper
// ============================================================================

StreamRecord makeRecord(
    const std::string& id,
    std::chrono::system_clock::time_point event_time,
    const std::string& partition_key,
    std::initializer_list<std::pair<std::string, RecordValue>> fields)
{
    StreamRecord r;
    r.record_id     = id.empty() ? genId() : id;
    r.event_time    = event_time;
    r.ingest_time   = std::chrono::system_clock::now();
    r.partition_key = partition_key;
    for (auto& [k, v] : fields) r.fields[k] = v;
    return r;
}

// ============================================================================
// TumblingWindow
// ============================================================================

TumblingWindow::TumblingWindow(const TumblingWindowConfig& config)
    : config_(config) {}

TumblingWindow::~TumblingWindow() {
    flush();
}

void TumblingWindow::addAggregation(const AggregateSpec& spec) {
    std::lock_guard lk(mutex_);
    agg_specs_.push_back(spec);
}

void TumblingWindow::setResultCallback(ResultCallback cb) {
    std::lock_guard lk(mutex_);
    callback_ = std::move(cb);
}

int64_t TumblingWindow::slotIndex(const std::chrono::system_clock::time_point& tp) const {
    int64_t us  = toMicros(tp);
    int64_t sz  = static_cast<int64_t>(config_.size.count()) * 1000LL; // ms → us
    return (sz > 0) ? (us / sz) : 0;
}

std::chrono::system_clock::time_point TumblingWindow::slotStart(int64_t idx) const {
    int64_t sz_us = static_cast<int64_t>(config_.size.count()) * 1000LL;
    return fromMicros(idx * sz_us);
}

void TumblingWindow::updateWatermark(const std::chrono::system_clock::time_point& event_time) {
    int64_t ev_us = toMicros(event_time);
    int64_t tol   = config_.watermark.max_out_of_orderness.count() * 1000LL;
    int64_t new_wm = ev_us - tol;
    // Monotonically advance
    int64_t old_wm = watermark_us_.load(std::memory_order_relaxed);
    while (new_wm > old_wm &&
           !watermark_us_.compare_exchange_weak(old_wm, new_wm,
               std::memory_order_release, std::memory_order_relaxed)) {}
}

void TumblingWindow::closeExpiredWindows(int64_t watermark_us) {
    // Called with mutex_ held
    std::vector<int64_t> to_close;
    for (const auto& [idx, win] : open_windows_) {
        int64_t win_end_us = toMicros(win.end);
        if (win_end_us <= watermark_us) {
            to_close.push_back(idx);
        }
    }
    for (int64_t idx : to_close) {
        auto& win = open_windows_[idx];
        if (config_.emit_empty_windows || !win.records.empty()) {
            auto result = computeResult(win, false);
            ++results_emitted_;
            if (callback_) {
                try { callback_(std::move(result)); } catch (...) {}
            }
        }
        ++windows_closed_;
        open_windows_.erase(idx);
    }
}

WindowResult TumblingWindow::computeResult(const InternalWindow& win, bool late) const {
    WindowResult r;
    r.window_id    = genId();
    r.window_start = win.start;
    r.window_end   = win.end;
    r.record_count = win.records.size();
    r.aggregations = computeAggregations(win.records, agg_specs_);
    r.is_late_firing = late;
    return r;
}

bool TumblingWindow::ingest(const StreamRecord& record) {
    ++records_ingested_;
    updateWatermark(record.event_time);
    int64_t wm = watermark_us_.load(std::memory_order_acquire);

    // Check if record is too old
    int64_t ev_us = toMicros(record.event_time);
    if (ev_us < wm && !config_.watermark.allow_late_data) {
        ++late_records_;
        ++records_dropped_;
        spdlog::debug("TumblingWindow: dropped late record (event={} < watermark={})", ev_us, wm);
        return false;
    }

    std::lock_guard lk(mutex_);

    int64_t idx = slotIndex(record.event_time);
    if (open_windows_.find(idx) == open_windows_.end()) {
        InternalWindow win;
        win.start = slotStart(idx);
        win.end   = win.start + config_.size;
        open_windows_[idx] = std::move(win);
        ++windows_opened_;
    }

    if (ev_us < wm && config_.watermark.allow_late_data) {
        ++late_records_;
    }
    open_windows_[idx].records.push_back(record);

    // Close any windows whose end is <= watermark
    closeExpiredWindows(wm);
    return true;
}

void TumblingWindow::flush() {
    std::lock_guard lk(mutex_);
    // Advance watermark to "infinite" to close all windows
    int64_t inf = std::numeric_limits<int64_t>::max();
    closeExpiredWindows(inf);
}

WindowStats TumblingWindow::getStats() const {
    return WindowStats{
        windows_opened_.load(),
        windows_closed_.load(),
        records_ingested_.load(),
        records_dropped_.load(),
        late_records_.load(),
        results_emitted_.load()
    };
}

// ============================================================================
// SlidingWindow
// ============================================================================

SlidingWindow::SlidingWindow(const SlidingWindowConfig& config)
    : config_(config) {}

SlidingWindow::~SlidingWindow() {
    flush();
}

void SlidingWindow::addAggregation(const AggregateSpec& spec) {
    std::lock_guard lk(mutex_);
    agg_specs_.push_back(spec);
}

void SlidingWindow::setResultCallback(ResultCallback cb) {
    std::lock_guard lk(mutex_);
    callback_ = std::move(cb);
}

std::string SlidingWindow::generateId() { return genId(); }

void SlidingWindow::updateWatermark(const std::chrono::system_clock::time_point& event_time) {
    int64_t ev_us  = toMicros(event_time);
    int64_t tol_us = config_.watermark.max_out_of_orderness.count() * 1000LL;
    int64_t new_wm = ev_us - tol_us;
    int64_t old_wm = watermark_us_.load(std::memory_order_relaxed);
    while (new_wm > old_wm &&
           !watermark_us_.compare_exchange_weak(old_wm, new_wm,
               std::memory_order_release, std::memory_order_relaxed)) {}
}

void SlidingWindow::ensureWindowsExist(const std::chrono::system_clock::time_point& event_time) {
    // Called with mutex_ held
    // A record at event_time must appear in all windows [start, start+size) where
    //   start ∈ { ..., t - (t % slide), t - (t % slide) + slide, ... }
    // covering [event_time - size + slide .. event_time]
    int64_t ev_us    = toMicros(event_time);
    int64_t size_us  = config_.size.count()  * 1000LL;
    int64_t slide_us = config_.slide.count() * 1000LL;
    if (slide_us <= 0) slide_us = size_us;

    // First window that could contain event_time
    int64_t first_start_us = (ev_us / slide_us) * slide_us;
    // Also check windows that started before and still cover event_time
    // i.e. start >= ev_us - size_us + slide_us, rounded to slide alignment
    int64_t earliest_start = ((ev_us - size_us) / slide_us) * slide_us;
    if (earliest_start < 0) earliest_start = 0;

    for (int64_t start_us = earliest_start; start_us <= first_start_us; start_us += slide_us) {
        int64_t end_us = start_us + size_us;
        // Only create if event_time falls in [start, end)
        if (ev_us < start_us || ev_us >= end_us) continue;
        // Check if window already exists
        bool found = false;
        for (const auto& w : windows_) {
            if (toMicros(w.start) == start_us) { found = true; break; }
        }
        if (!found) {
            InternalWindow win;
            win.window_id = genId();
            win.start = fromMicros(start_us);
            win.end   = fromMicros(end_us);
            windows_.push_back(std::move(win));
            ++windows_opened_;
        }
    }
}

void SlidingWindow::closeExpiredWindows(int64_t watermark_us) {
    // Called with mutex_ held
    for (auto& w : windows_) {
        if (!w.closed && toMicros(w.end) <= watermark_us) {
            w.closed = true;
            ++windows_closed_;
            auto result = computeResult(w, false);
            ++results_emitted_;
            if (callback_) {
                try { callback_(std::move(result)); } catch (...) {}
            }
        }
    }
    // Prune closed windows (keep deque manageable)
    while (!windows_.empty() && windows_.front().closed) {
        windows_.pop_front();
    }
}

WindowResult SlidingWindow::computeResult(const InternalWindow& win, bool late) const {
    WindowResult r;
    r.window_id    = win.window_id;
    r.window_start = win.start;
    r.window_end   = win.end;
    r.record_count = win.records.size();
    r.aggregations = computeAggregations(win.records, agg_specs_);
    r.is_late_firing = late;
    return r;
}

bool SlidingWindow::ingest(const StreamRecord& record) {
    ++records_ingested_;
    updateWatermark(record.event_time);
    int64_t wm    = watermark_us_.load(std::memory_order_acquire);
    int64_t ev_us = toMicros(record.event_time);

    if (ev_us < wm && !config_.watermark.allow_late_data) {
        ++late_records_;
        ++records_dropped_;
        return false;
    }

    std::lock_guard lk(mutex_);

    ensureWindowsExist(record.event_time);

    // Add record to all overlapping open windows
    for (auto& w : windows_) {
        if (!w.closed &&
            record.event_time >= w.start &&
            record.event_time < w.end) {
            w.records.push_back(record);
        }
    }

    if (ev_us < wm && config_.watermark.allow_late_data) {
        ++late_records_;
    }

    closeExpiredWindows(wm);
    return true;
}

void SlidingWindow::flush() {
    std::lock_guard lk(mutex_);
    closeExpiredWindows(std::numeric_limits<int64_t>::max());
}

WindowStats SlidingWindow::getStats() const {
    return WindowStats{
        windows_opened_.load(),
        windows_closed_.load(),
        records_ingested_.load(),
        records_dropped_.load(),
        late_records_.load(),
        results_emitted_.load()
    };
}

// ============================================================================
// SessionWindow
// ============================================================================

SessionWindow::SessionWindow(const SessionWindowConfig& config)
    : config_(config) {
    running_ = true;
    expiry_thread_ = std::thread([this] { expiryLoop(); });
}

SessionWindow::~SessionWindow() {
    flush();
    running_ = false;
    expiry_cv_.notify_all();
    if (expiry_thread_.joinable()) expiry_thread_.join();
}

void SessionWindow::addAggregation(const AggregateSpec& spec) {
    std::lock_guard lk(mutex_);
    agg_specs_.push_back(spec);
}

void SessionWindow::setResultCallback(ResultCallback cb) {
    std::lock_guard lk(mutex_);
    callback_ = std::move(cb);
}

std::string SessionWindow::generateId() { return genId(); }

WindowResult SessionWindow::computeResult(const Session& s) const {
    WindowResult r;
    r.window_id     = s.session_id;
    r.window_start  = s.start;
    r.window_end    = s.last_event;
    r.partition_key = s.partition_key;
    r.record_count  = s.records.size();
    r.aggregations  = computeAggregations(s.records, agg_specs_);
    return r;
}

bool SessionWindow::ingest(const StreamRecord& record) {
    ++records_ingested_;
    std::lock_guard lk(mutex_);

    const std::string& key = record.partition_key;
    auto it = sessions_.find(key);
    if (it == sessions_.end()) {
        Session s;
        s.session_id    = genId();
        s.partition_key = key;
        s.start         = record.event_time;
        s.last_event    = record.event_time;
        s.records.push_back(record);
        sessions_[key] = std::move(s);
        ++windows_opened_;
    } else {
        auto& s = it->second;
        auto gap_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
            record.event_time - s.last_event);

        if (gap_since_last > config_.gap) {
            // Gap exceeded → close current session and start new
            auto result = computeResult(s);
            ++windows_closed_;
            ++results_emitted_;
            if (callback_) {
                try { callback_(std::move(result)); } catch (...) {}
            }
            // New session
            Session ns;
            ns.session_id    = genId();
            ns.partition_key = key;
            ns.start         = record.event_time;
            ns.last_event    = record.event_time;
            ns.records.push_back(record);
            it->second = std::move(ns);
            ++windows_opened_;
        } else {
            // Extend existing session
            s.last_event = record.event_time;
            s.records.push_back(record);
        }
    }
    return true;
}

void SessionWindow::flush() {
    std::lock_guard lk(mutex_);
    for (auto& [key, s] : sessions_) {
        if (!s.records.empty()) {
            auto result = computeResult(s);
            ++windows_closed_;
            ++results_emitted_;
            if (callback_) {
                try { callback_(std::move(result)); } catch (...) {}
            }
        }
    }
    sessions_.clear();
}

WindowStats SessionWindow::getStats() const {
    return WindowStats{
        windows_opened_.load(),
        windows_closed_.load(),
        records_ingested_.load(),
        records_dropped_.load(),
        late_records_.load(),
        results_emitted_.load()
    };
}

void SessionWindow::expiryLoop() {
    while (running_) {
        {
            std::unique_lock lk(expiry_mutex_);
            expiry_cv_.wait_for(lk, std::chrono::milliseconds(200),
                                [this] { return !running_.load(); });
        }
        if (!running_) break;

        auto now = std::chrono::system_clock::now();
        std::lock_guard lk(mutex_);
        std::vector<std::string> to_close;
        for (const auto& [key, s] : sessions_) {
            auto idle = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s.last_event);
            if (idle > config_.gap) {
                to_close.push_back(key);
            }
        }
        for (const auto& key : to_close) {
            auto& s = sessions_[key];
            if (!s.records.empty()) {
                auto result = computeResult(s);
                ++windows_closed_;
                ++results_emitted_;
                if (callback_) {
                    try { callback_(std::move(result)); } catch (...) {}
                }
            }
            sessions_.erase(key);
        }
    }
}

// ============================================================================
// HoppingWindow
// ============================================================================

HoppingWindow::HoppingWindow(const HoppingWindowConfig& config)
    : config_(config) {}

HoppingWindow::~HoppingWindow() {
    flush();
}

void HoppingWindow::addAggregation(const AggregateSpec& spec) {
    std::lock_guard lk(mutex_);
    agg_specs_.push_back(spec);
}

void HoppingWindow::setResultCallback(ResultCallback cb) {
    std::lock_guard lk(mutex_);
    callback_ = std::move(cb);
}

std::string HoppingWindow::generateId() { return genId(); }

void HoppingWindow::updateWatermark(const std::chrono::system_clock::time_point& event_time) {
    int64_t ev_us  = toMicros(event_time);
    int64_t tol_us = config_.watermark.max_out_of_orderness.count() * 1000LL;
    int64_t new_wm = ev_us - tol_us;
    int64_t old_wm = watermark_us_.load(std::memory_order_relaxed);
    while (new_wm > old_wm &&
           !watermark_us_.compare_exchange_weak(old_wm, new_wm,
               std::memory_order_release, std::memory_order_relaxed)) {}
}

void HoppingWindow::ensureWindowsExist(const std::chrono::system_clock::time_point& event_time) {
    // Same as SlidingWindow but uses hop instead of slide
    int64_t ev_us   = toMicros(event_time);
    int64_t size_us = config_.size.count() * 1000LL;
    int64_t hop_us  = config_.hop.count()  * 1000LL;
    if (hop_us <= 0) hop_us = size_us;

    int64_t first_start = (ev_us / hop_us) * hop_us;
    int64_t earliest    = ((ev_us - size_us) / hop_us) * hop_us;
    if (earliest < 0) earliest = 0;

    for (int64_t start_us = earliest; start_us <= first_start; start_us += hop_us) {
        int64_t end_us = start_us + size_us;
        if (ev_us < start_us || ev_us >= end_us) continue;
        bool found = false;
        for (const auto& w : windows_) {
            if (toMicros(w.start) == start_us) { found = true; break; }
        }
        if (!found) {
            InternalWindow win;
            win.window_id = genId();
            win.start = fromMicros(start_us);
            win.end   = fromMicros(end_us);
            windows_.push_back(std::move(win));
            ++windows_opened_;
        }
    }
}

void HoppingWindow::closeExpiredWindows(int64_t watermark_us) {
    for (auto& w : windows_) {
        if (!w.closed && toMicros(w.end) <= watermark_us) {
            w.closed = true;
            ++windows_closed_;
            auto result = computeResult(w, false);
            ++results_emitted_;
            if (callback_) {
                try { callback_(std::move(result)); } catch (...) {}
            }
        }
    }
    while (!windows_.empty() && windows_.front().closed) {
        windows_.pop_front();
    }
}

WindowResult HoppingWindow::computeResult(const InternalWindow& win, bool late) const {
    WindowResult r;
    r.window_id    = win.window_id;
    r.window_start = win.start;
    r.window_end   = win.end;
    r.record_count = win.records.size();
    r.aggregations = computeAggregations(win.records, agg_specs_);
    r.is_late_firing = late;
    return r;
}

bool HoppingWindow::ingest(const StreamRecord& record) {
    ++records_ingested_;
    updateWatermark(record.event_time);
    int64_t wm    = watermark_us_.load(std::memory_order_acquire);
    int64_t ev_us = toMicros(record.event_time);

    if (ev_us < wm && !config_.watermark.allow_late_data) {
        ++late_records_;
        ++records_dropped_;
        return false;
    }

    std::lock_guard lk(mutex_);
    ensureWindowsExist(record.event_time);

    for (auto& w : windows_) {
        if (!w.closed &&
            record.event_time >= w.start &&
            record.event_time < w.end) {
            w.records.push_back(record);
        }
    }

    if (ev_us < wm && config_.watermark.allow_late_data) ++late_records_;
    closeExpiredWindows(wm);
    return true;
}

void HoppingWindow::flush() {
    std::lock_guard lk(mutex_);
    closeExpiredWindows(std::numeric_limits<int64_t>::max());
}

WindowStats HoppingWindow::getStats() const {
    return WindowStats{
        windows_opened_.load(),
        windows_closed_.load(),
        records_ingested_.load(),
        records_dropped_.load(),
        late_records_.load(),
        results_emitted_.load()
    };
}

// ============================================================================
// StreamingWindowPipeline
// ============================================================================

StreamingWindowPipeline StreamingWindowPipeline::tumbling(
    std::chrono::milliseconds size, WatermarkConfig wm)
{
    StreamingWindowPipeline p;
    p.config_.type = Type::TUMBLING;
    p.config_.size = size;
    p.config_.watermark = wm;
    return p;
}

StreamingWindowPipeline StreamingWindowPipeline::sliding(
    std::chrono::milliseconds size,
    std::chrono::milliseconds slide,
    WatermarkConfig wm)
{
    StreamingWindowPipeline p;
    p.config_.type  = Type::SLIDING;
    p.config_.size  = size;
    p.config_.slide = slide;
    p.config_.watermark = wm;
    return p;
}

StreamingWindowPipeline StreamingWindowPipeline::session(
    std::chrono::milliseconds gap, WatermarkConfig wm)
{
    StreamingWindowPipeline p;
    p.config_.type = Type::SESSION;
    p.config_.gap  = gap;
    p.config_.watermark = wm;
    return p;
}

StreamingWindowPipeline StreamingWindowPipeline::hopping(
    std::chrono::milliseconds size,
    std::chrono::milliseconds hop,
    WatermarkConfig wm)
{
    StreamingWindowPipeline p;
    p.config_.type = Type::HOPPING;
    p.config_.size = size;
    p.config_.hop  = hop;
    p.config_.watermark = wm;
    return p;
}

StreamingWindowPipeline& StreamingWindowPipeline::aggregate(const AggregateSpec& spec) {
    agg_specs_.push_back(spec);
    return *this;
}

StreamingWindowPipeline& StreamingWindowPipeline::onResult(
    std::function<void(WindowResult)> callback)
{
    callback_ = std::move(callback);
    return *this;
}

std::shared_ptr<StreamingWindowPipeline> StreamingWindowPipeline::build() {
    auto pipeline = std::make_shared<StreamingWindowPipeline>(*this);

    switch (config_.type) {
        case Type::TUMBLING: {
            TumblingWindowConfig cfg;
            cfg.size      = config_.size;
            cfg.watermark = config_.watermark;
            pipeline->tumbling_ = std::make_shared<TumblingWindow>(cfg);
            for (const auto& s : agg_specs_) pipeline->tumbling_->addAggregation(s);
            if (callback_) pipeline->tumbling_->setResultCallback(callback_);
            break;
        }
        case Type::SLIDING: {
            SlidingWindowConfig cfg;
            cfg.size      = config_.size;
            cfg.slide     = config_.slide;
            cfg.watermark = config_.watermark;
            pipeline->sliding_ = std::make_shared<SlidingWindow>(cfg);
            for (const auto& s : agg_specs_) pipeline->sliding_->addAggregation(s);
            if (callback_) pipeline->sliding_->setResultCallback(callback_);
            break;
        }
        case Type::SESSION: {
            SessionWindowConfig cfg;
            cfg.gap       = config_.gap;
            cfg.watermark = config_.watermark;
            pipeline->session_ = std::make_shared<SessionWindow>(cfg);
            for (const auto& s : agg_specs_) pipeline->session_->addAggregation(s);
            if (callback_) pipeline->session_->setResultCallback(callback_);
            break;
        }
        case Type::HOPPING: {
            HoppingWindowConfig cfg;
            cfg.size      = config_.size;
            cfg.hop       = config_.hop;
            cfg.watermark = config_.watermark;
            pipeline->hopping_ = std::make_shared<HoppingWindow>(cfg);
            for (const auto& s : agg_specs_) pipeline->hopping_->addAggregation(s);
            if (callback_) pipeline->hopping_->setResultCallback(callback_);
            break;
        }
    }
    pipeline->built_ = true;
    return pipeline;
}

bool StreamingWindowPipeline::ingest(const StreamRecord& record) {
    if (tumbling_) return tumbling_->ingest(record);
    if (sliding_)  return sliding_->ingest(record);
    if (session_)  return session_->ingest(record);
    if (hopping_)  return hopping_->ingest(record);
    return false;
}

void StreamingWindowPipeline::flush() {
    if (tumbling_) tumbling_->flush();
    if (sliding_)  sliding_->flush();
    if (session_)  session_->flush();
    if (hopping_)  hopping_->flush();
}

WindowStats StreamingWindowPipeline::getStats() const {
    if (tumbling_) return tumbling_->getStats();
    if (sliding_)  return sliding_->getStats();
    if (session_)  return session_->getStats();
    if (hopping_)  return hopping_->getStats();
    return {};
}

} // namespace analytics
} // namespace themisdb
