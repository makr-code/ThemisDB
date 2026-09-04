/**
 * @file continuous_profiler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=16, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/continuous_profiler.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

// Platform-specific stack-trace includes
#if defined(__linux__) || defined(__APPLE__)
#  include <execinfo.h>
#  define THEMIS_HAS_BACKTRACE 1
#elif defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <dbghelp.h>
#  define THEMIS_HAS_BACKTRACE 0
#  define THEMIS_HAS_WIN32_DBGHELP 1
#else
#  define THEMIS_HAS_BACKTRACE 0
#  define THEMIS_HAS_WIN32_DBGHELP 0
#endif

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/** Collect a raw call stack, up to @p max_depth frames. */
std::vector<std::string> captureStack([[maybe_unused]] int max_depth = 64) {
    std::vector<std::string> frames;
#if THEMIS_HAS_BACKTRACE
    std::vector<void*> buffer(static_cast<size_t>(max_depth));
    int count = ::backtrace(buffer.data(), max_depth);
    if (count <= 0) {
        return frames;
    }
    char** symbols = ::backtrace_symbols(buffer.data(), count);
    if (!symbols) {
        return frames;
    }
    frames.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        frames.emplace_back(symbols[i] ? symbols[i] : "??");
    }
    ::free(symbols);
#else
#if defined(THEMIS_HAS_WIN32_DBGHELP) && THEMIS_HAS_WIN32_DBGHELP
    // Windows DbgHelp path: CaptureStackBackTrace + SymFromAddr
    constexpr DWORD kMaxFrames = 64;
    void* frame_ptrs[kMaxFrames] = {};
    const DWORD captured = ::CaptureStackBackTrace(
        /*FramesToSkip=*/1,  // skip this captureStack() frame
        /*FramesToCapture=*/std::min(static_cast<DWORD>(max_depth), kMaxFrames),
        frame_ptrs,
        /*BackTraceHash=*/nullptr);

    if (captured > 0) {
        const HANDLE process = ::GetCurrentProcess();
        ::SymInitialize(process, nullptr, TRUE);

        // SymFromAddr needs a SYMBOL_INFO buffer with space for the name
        constexpr DWORD kNameLen = 256;
        alignas(SYMBOL_INFO) char sym_buf[sizeof(SYMBOL_INFO) + kNameLen * sizeof(TCHAR)];
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(sym_buf);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen   = kNameLen;

        frames.reserve(static_cast<size_t>(captured));
        for (DWORD i = 0; i < captured; ++i) {
            DWORD64 displacement = 0;
            if (::SymFromAddr(process, reinterpret_cast<DWORD64>(frame_ptrs[i]),
                              &displacement, sym)) {
                frames.emplace_back(sym->Name);
            } else {
                char addr_buf[32];
                std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", frame_ptrs[i]);
                frames.emplace_back(addr_buf);
            }
        }
    } else {
        frames.emplace_back("(stack-trace-unavailable)");
    }
#else
    frames.emplace_back("(stack-trace-unavailable)");
#endif
#endif
    return frames;
}

/** Encode @p bytes as base64 (for JSON serialisation). */
std::string base64Encode(const std::vector<uint8_t>& bytes) {
    static const char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        uint32_t b = static_cast<uint32_t>(bytes[i]) << 16;
        if (i + 1 < bytes.size()) b |= static_cast<uint32_t>(bytes[i + 1]) << 8;
        if (i + 2 < bytes.size()) b |= static_cast<uint32_t>(bytes[i + 2]);
        out += kTable[(b >> 18) & 0x3f];
        out += kTable[(b >> 12) & 0x3f];
        out += (i + 1 < bytes.size()) ? kTable[(b >> 6) & 0x3f] : '=';
        out += (i + 2 < bytes.size()) ? kTable[b & 0x3f] : '=';
    }
    return out;
}

/** Parse folded-stacks text into a {stack -> count} map. */
std::map<std::string, uint64_t> parseFolded(const std::string& text) {
    std::map<std::string, uint64_t> result;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        auto space = line.rfind(' ');
        if (space == std::string::npos) continue;
        std::string stack = line.substr(0, space);
        uint64_t count = 0;
        try {
            count = std::stoull(line.substr(space + 1));
        } catch (...) {
            continue;
        }
        result[stack] += count;
    }
    return result;
}

const char* profileTypeName(ProfileType t) {
    switch (t) {
        case ProfileType::CPU:   return "cpu";
        case ProfileType::HEAP:  return "heap";
        case ProfileType::MUTEX: return "mutex";
        case ProfileType::BLOCK: return "block";
        default:                 return "unknown";
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ProfileSnapshot
// ---------------------------------------------------------------------------

void ProfileSnapshot::saveToFile(const std::string& filename) const {
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error("ContinuousProfiler: cannot open file for writing: " + filename);
    }
    ofs.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!ofs) {
        throw std::runtime_error("ContinuousProfiler: write error to file: " + filename);
    }
}

ProfileSnapshot ProfileSnapshot::loadFromFile(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
    if (!ifs) {
        throw std::runtime_error("ContinuousProfiler: cannot open file for reading: " + filename);
    }
    auto size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    ProfileSnapshot snap;
    snap.type = ProfileType::CPU;  // default; caller can override
    snap.timestamp = std::chrono::system_clock::now();
    if (size > 0) {
        snap.data.resize(static_cast<size_t>(size));
        ifs.read(reinterpret_cast<char*>(snap.data.data()), size);
    }
    return snap;
}

json ProfileSnapshot::toJSON() const {
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                  timestamp.time_since_epoch())
                  .count();
    return json{
        {"type", profileTypeName(type)},
        {"timestamp_ms", ts},
        {"duration_s", duration.count()},
        {"data_size_bytes", data.size()},
        {"data_base64", base64Encode(data)}
    };
}

// ---------------------------------------------------------------------------
// ProfileDiff
// ---------------------------------------------------------------------------

json ProfileDiff::toJSON() const {
    return json{
        {"cpu_regression_percent", cpu_regression_percent},
        {"memory_regression_percent", memory_regression_percent},
        {"new_hotspots", new_hotspots},
        {"removed_hotspots", removed_hotspots},
        {"changed_hotspots", changed_hotspots}
    };
}

// ---------------------------------------------------------------------------
// ContinuousProfiler::Impl
// ---------------------------------------------------------------------------

/** @brief ContinuousProfiler::Impl. */
class ContinuousProfiler::Impl {
public:
    explicit Impl(const ContinuousProfilerConfig& config)
        : enabled_(config.enabled), config_(config) {}

    ~Impl() {
        stopInternal();
    }

    void start() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (running_ || !enabled_) return;
        running_ = true;
        worker_ = std::thread(&Impl::workerLoop, this);
    }

    void stop() {
        stopInternal();
    }

    ProfileSnapshot snapshot(ProfileType type) {
        std::unique_lock<std::mutex> lock(mutex_);

        ProfileSnapshot snap;
        snap.type = type;
        snap.timestamp = std::chrono::system_clock::now();
        snap.duration = config_.snapshot_interval;

        if (type == ProfileType::CPU) {
            // Under heavy scheduling/load, the background sampler may not have
            // produced the first sample yet when snapshot() is called shortly
            // after start(). Capture one synchronous sample as a safe fallback
            // so callers get a non-empty snapshot whenever CPU profiling is
            // enabled.
            if (cpu_stacks_.empty() &&
                enabled_.load(std::memory_order_acquire) &&
                config_.enable_cpu_profiling) {
                auto frames = captureStack(64);
                std::string key;
                for (size_t i = 0; i < frames.size(); ++i) {
                    if (i > 0) key += ';';
                    key += frames[i];
                }
                cpu_stacks_[key]++;
            }

            // Serialise current accumulated stacks to folded-stacks text
            std::string text;
            for (const auto& [stack, count] : cpu_stacks_) {
                text += stack;
                text += ' ';
                text += std::to_string(count);
                text += '\n';
            }
            snap.data.assign(text.begin(), text.end());

            // Record snapshot in history and trim if necessary
            addSnapshot(ProfileType::CPU, snap);
        }

        return snap;
    }

    std::vector<ProfileSnapshot> getSnapshots(
            ProfileType type,
            std::chrono::system_clock::time_point from,
            std::chrono::system_clock::time_point to) const {
        std::unique_lock<std::mutex> lock(mutex_);
        std::vector<ProfileSnapshot> result;
        auto it = history_.find(type);
        if (it == history_.end()) return result;
        for (const auto& s : it->second) {
            if (s.timestamp >= from && s.timestamp < to) {
                result.push_back(s);
            }
        }
        return result;
    }

    ProfileDiff compare(const ProfileSnapshot& baseline,
                        const ProfileSnapshot& current) const {
        ProfileDiff diff;
        if (baseline.type != ProfileType::CPU || current.type != ProfileType::CPU) {
            return diff;
        }

        auto baseMap = parseFolded(baseline.dataAsString());
        auto curMap  = parseFolded(current.dataAsString());

        uint64_t baseTotal = 0, curTotal = 0;
        for (auto& [k, v] : baseMap) baseTotal += v;
        for (auto& [k, v] : curMap)  curTotal  += v;

        if (baseTotal > 0 && curTotal > 0) {
            diff.cpu_regression_percent =
                (static_cast<double>(curTotal) / static_cast<double>(baseTotal) - 1.0) * 100.0;
        }

        constexpr double kChangeThreshold = 0.10;  // 10 % change = "changed"

        for (const auto& [stack, curCnt] : curMap) {
            auto baseIt = baseMap.find(stack);
            if (baseIt == baseMap.end()) {
                diff.new_hotspots.push_back(stack);
            } else {
                double baseNorm = (baseTotal > 0)
                    ? static_cast<double>(baseIt->second) / static_cast<double>(baseTotal)
                    : 0.0;
                double curNorm  = (curTotal > 0)
                    ? static_cast<double>(curCnt) / static_cast<double>(curTotal)
                    : 0.0;
                if (std::abs(curNorm - baseNorm) > kChangeThreshold) {
                    diff.changed_hotspots.push_back(stack);
                }
            }
        }

        for (const auto& [stack, _] : baseMap) {
            if (curMap.find(stack) == curMap.end()) {
                diff.removed_hotspots.push_back(stack);
            }
        }

        // Deduplicate lists (they may be long; cap at 20 entries for usability)
        auto trim = [](std::vector<std::string>& v) {
            if (v.size() > 20) v.resize(20);
        };
        trim(diff.new_hotspots);
        trim(diff.removed_hotspots);
        trim(diff.changed_hotspots);

        return diff;
    }

    void registerAnomalyCallback(
            std::function<void(const ProfileSnapshot&, const std::string&)> cb) {
        std::unique_lock<std::mutex> lock(mutex_);
        anomaly_cb_ = std::move(cb);
    }

    void enable() {
        enabled_.store(true, std::memory_order_release);
    }

    void disable() {
        enabled_.store(false, std::memory_order_release);
    }

    bool isEnabled() const {
        return enabled_.load(std::memory_order_acquire);
    }

    ContinuousProfilerConfig getConfig() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return config_;
    }

private:
    // -----------------------------------------------------------------------
    // Background worker
    // -----------------------------------------------------------------------

    void workerLoop() {
        // Determine the sampling interval from cpu_sample_rate.
        // cpu_sample_rate = desired overhead fraction → sample period ≈ 1/rate ms
        // Clamp to [1 ms, 1000 ms] to stay practical.
        double rate = config_.cpu_sample_rate;
        if (rate <= 0.0) rate = 0.01;
        if (rate > 1.0)  rate = 1.0;
        auto sample_period_ms = static_cast<int64_t>(1.0 / rate);
        if (sample_period_ms < 1)    sample_period_ms = 1;
        if (sample_period_ms > 1000) sample_period_ms = 1000;
        const auto sample_period = std::chrono::milliseconds(sample_period_ms);

        auto next_flush = std::chrono::steady_clock::now() + config_.snapshot_interval;
        ProfileSnapshot last_flush_snap;
        bool have_last = false;

        while (true) {
            // Wait for sample_period or until stopped
            {
                std::unique_lock<std::mutex> lk(mutex_);
                cv_.wait_for(lk, sample_period, [this] { return !running_; });
                if (!running_) break;
            }

            if (!enabled_.load(std::memory_order_acquire)) {
                continue;
            }

            // Collect a sample
            if (config_.enable_cpu_profiling) {
                auto frames = captureStack(64);
                // Build the folded key (semicolon-joined frames, no count suffix)
                std::string key;
                for (size_t i = 0; i < frames.size(); ++i) {
                    if (i > 0) key += ';';
                    key += frames[i];
                }
                std::unique_lock<std::mutex> lk(mutex_);
                cpu_stacks_[key]++;
            }

            // Periodic flush
            auto now = std::chrono::steady_clock::now();
            if (now >= next_flush) {
                next_flush = now + config_.snapshot_interval;

                // Take snapshot and copy the callback under the mutex so we
                // never hold the lock while invoking user code.
                ProfileSnapshot snap;
                std::function<void(const ProfileSnapshot&, const std::string&)> cb;
                {
                    std::unique_lock<std::mutex> lk(mutex_);
                    snap = snapshotNoLock(ProfileType::CPU);
                    cb = anomaly_cb_;
                }

                // Anomaly detection: compare with last flush
                if (have_last && cb) {
                    auto diff = compare(last_flush_snap, snap);
                    if (diff.cpu_regression_percent > 20.0) {
                        std::string msg = "CPU regression detected: +" +
                            std::to_string(static_cast<int>(diff.cpu_regression_percent)) + "%";
                        try { cb(snap, msg); } catch (...) {}
                    }
                }
                last_flush_snap = snap;
                have_last = true;

                // Emit metrics
                MetricsCollector::getInstance().recordSpanDuration(
                    "continuous_profiler_snapshot",
                    static_cast<double>(snap.data.size()));

                // Persist to disk if configured
                if (!config_.output_dir.empty()) {
                    persistSnapshot(snap);
                }
            }
        }
    }

    /** Snapshot without acquiring the mutex (caller holds it). */
    ProfileSnapshot snapshotNoLock(ProfileType type) {
        ProfileSnapshot snap;
        snap.type = type;
        snap.timestamp = std::chrono::system_clock::now();
        snap.duration = config_.snapshot_interval;
        if (type == ProfileType::CPU) {
            std::string text;
            for (const auto& [stack, count] : cpu_stacks_) {
                text += stack;
                text += ' ';
                text += std::to_string(count);
                text += '\n';
            }
            snap.data.assign(text.begin(), text.end());
            addSnapshot(type, snap);
        }
        return snap;
    }

    void addSnapshot(ProfileType type, const ProfileSnapshot& snap) {
        auto& vec = history_[type];
        vec.push_back(snap);
        while (vec.size() > config_.max_snapshots_retained) {
            vec.erase(vec.begin());
        }
    }

    void persistSnapshot(const ProfileSnapshot& snap) {
        // Build filename: <output_dir>/<type>_<timestamp_s>.folded
        auto ts_s = std::chrono::duration_cast<std::chrono::seconds>(
                        snap.timestamp.time_since_epoch())
                        .count();
        std::string path = config_.output_dir + "/" +
                           profileTypeName(snap.type) + "_" +
                           std::to_string(ts_s) + ".folded";
        try {
            snap.saveToFile(path);
        } catch (...) {
            // Silently ignore persistence errors to avoid disrupting the profiler
        }
    }

    void stopInternal() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!running_) return;
            running_ = false;
        }
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    bool running_ = false;
    std::atomic<bool> enabled_;

    ContinuousProfilerConfig config_;

    // Accumulated CPU stacks: folded-stack-string → sample count
    std::map<std::string, uint64_t> cpu_stacks_;

    // Per-type snapshot history
    std::map<ProfileType, std::vector<ProfileSnapshot>> history_;

    // Optional anomaly callback
    std::function<void(const ProfileSnapshot&, const std::string&)> anomaly_cb_;
};

// ---------------------------------------------------------------------------
// ContinuousProfiler – public API (forwards to Impl)
// ---------------------------------------------------------------------------

ContinuousProfiler::ContinuousProfiler(const ContinuousProfilerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

ContinuousProfiler::~ContinuousProfiler() = default;

void ContinuousProfiler::start() { impl_->start(); }
void ContinuousProfiler::stop()  { impl_->stop();  }

ProfileSnapshot ContinuousProfiler::snapshot(ProfileType type) {
    return impl_->snapshot(type);
}

std::vector<ProfileSnapshot> ContinuousProfiler::getSnapshots(
        ProfileType type,
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) const {
    return impl_->getSnapshots(type, from, to);
}

ProfileDiff ContinuousProfiler::compare(const ProfileSnapshot& baseline,
                                         const ProfileSnapshot& current) const {
    return impl_->compare(baseline, current);
}

void ContinuousProfiler::registerAnomalyCallback(
        std::function<void(const ProfileSnapshot&, const std::string&)> cb) {
    impl_->registerAnomalyCallback([[maybe_unused]] std::move(cb));
}

void ContinuousProfiler::enable()       { impl_->enable();    }
void ContinuousProfiler::disable()      { impl_->disable();   }
bool ContinuousProfiler::isEnabled() const { return impl_->isEnabled(); }

ContinuousProfilerConfig ContinuousProfiler::getConfig() const {
    return impl_->getConfig();
}

} // namespace observability
} // namespace themis


