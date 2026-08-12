/**
 * @file continuous_profiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace observability {

using json = nlohmann::json;

/**
 * @brief Profile types supported by the continuous profiler.
 */
enum class ProfileType {
    CPU,    ///< CPU call-stack sampling
    HEAP,   ///< Heap allocation sampling
    MUTEX,  ///< Mutex contention profiling
    BLOCK   ///< Blocking operation profiling
};

/**
 * @brief A single profile snapshot captured by the continuous profiler.
 *
 * The @c data field holds the profile in the pprof folded-stacks text
 * format (one stack per line, space-separated frames, followed by a
 * sample count).  This format is directly consumable by:
 *   - `go tool pprof`
 *   - `flamegraph.pl` (Brendan Gregg)
 *   - async-profiler's flamegraph output pipeline
 */
struct ProfileSnapshot {
    ProfileType type;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::seconds duration{0};
    std::vector<uint8_t> data;  ///< Profile payload (pprof folded-stacks text, UTF-8)

    /**
     * @brief Convenience accessor: profile payload as a UTF-8 string.
     */
    std::string dataAsString() const {
        return std::string(data.begin(), data.end());
    }

    /**
     * @brief Persist the snapshot to @p filename.
     * @param filename Destination path (created or overwritten).
     * @throws std::runtime_error on I/O failure.
     */
    void saveToFile(const std::string& filename) const;

    /**
     * @brief Load a previously saved snapshot from @p filename.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static ProfileSnapshot loadFromFile(const std::string& filename);

    /** @brief Serialize to JSON (metadata only; data is base64-encoded). */
    json toJSON() const;
};

/**
 * @brief Differential comparison result between two ProfileSnapshot objects.
 */
struct ProfileDiff {
    double cpu_regression_percent = 0.0;      ///< Positive = regression
    double memory_regression_percent = 0.0;   ///< Positive = regression
    std::vector<std::string> new_hotspots;     ///< Frames that appeared in current
    std::vector<std::string> removed_hotspots; ///< Frames absent from current
    std::vector<std::string> changed_hotspots; ///< Frames whose sample count changed significantly

    /** @brief Serialize to JSON. */
    json toJSON() const;
};

/**
 * @brief Configuration for the continuous profiler.
 */
struct ContinuousProfilerConfig {
    bool enabled = false;
    /// Target CPU overhead fraction (0.01 = 1 %).  Controls sampling frequency.
    double cpu_sample_rate = 0.01;
    /// How often a snapshot is automatically flushed to disk.
    std::chrono::seconds snapshot_interval{60};
    /// Maximum number of in-memory snapshots retained per profile type.
    size_t max_snapshots_retained = 1440;  // 24 h at 1/min
    /// Directory where snapshots are persisted.  Empty string disables persistence.
    std::string output_dir = "/var/lib/themisdb/profiles";
    bool enable_cpu_profiling = true;
    bool enable_heap_profiling = false;
    bool enable_mutex_profiling = false;
    bool enable_block_profiling = false;
};

/**
 * @brief Continuous, low-overhead profiler with pprof / async-profiler
 *        compatible output.
 *
 * ### Design
 * A background sampling thread wakes at a configurable interval (derived
 * from @c ContinuousProfilerConfig::cpu_sample_rate) and collects a stack
 * trace of the calling process.  Collected stacks are accumulated in an
 * in-memory store and periodically flushed as pprof folded-stacks snapshots.
 *
 * ### Thread Safety
 * All public methods are thread-safe.
 *
 * ### pprof Compatibility
 * Snapshots use the folded-stacks text format:
 * ```
 * frame1;frame2;frame3 <count>
 * ```
 * Load with:
 * ```
 * go tool pprof -http=:8080 profile.folded
 * ```
 * Or convert to flame graph SVG via `flamegraph.pl` from Brendan Gregg's
 * FlameGraph toolkit.
 *
 * ### async-profiler Compatibility
 * The same folded-stacks format is produced by async-profiler's
 * `-o collapsed` mode, ensuring interoperability with its toolchain.
 *
 * ### Usage
 * ```cpp
 * ContinuousProfilerConfig cfg;
 * cfg.enabled = true;
 * cfg.output_dir = "/var/lib/themisdb/profiles";
 *
 * ContinuousProfiler profiler(cfg);
 * profiler.start();
 *
 * // ... workload ...
 *
 * auto snap = profiler.snapshot(ProfileType::CPU);
 * snap.saveToFile("/tmp/profile.folded");
 * profiler.stop();
 * ```
 */
class ContinuousProfiler {
public:
    explicit ContinuousProfiler(const ContinuousProfilerConfig& config = ContinuousProfilerConfig{});
    ~ContinuousProfiler();

    // Non-copyable
    ContinuousProfiler(const ContinuousProfiler&) = delete;
    ContinuousProfiler& operator=(const ContinuousProfiler&) = delete;

    /**
     * @brief Start the background sampling loop.
     * No-op if already running or if @c enabled is false.
     */
    void start();

    /**
     * @brief Stop the background sampling loop and flush remaining data.
     * Blocks until the background thread has exited.
     */
    void stop();

    /**
     * @brief Capture and return a snapshot of the current accumulated profile.
     * @param type  Profile type to capture.  Currently only @c ProfileType::CPU
     *              is collected; other types return an empty snapshot.
     * @return      A @c ProfileSnapshot whose @c data field contains the
     *              pprof folded-stacks text for the requested type.
     */
    ProfileSnapshot snapshot(ProfileType type = ProfileType::CPU);

    /**
     * @brief Retrieve all snapshots of @p type within [@p from, @p to).
     */
    std::vector<ProfileSnapshot> getSnapshots(
        ProfileType type,
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) const;

    /**
     * @brief Compare two snapshots and return a differential report.
     */
    ProfileDiff compare(const ProfileSnapshot& baseline,
                        const ProfileSnapshot& current) const;

    /**
     * @brief Register a callback that fires when a CPU regression is detected.
     *
     * The callback receives the snapshot that triggered the detection and a
     * human-readable description of the anomaly.  Called from the background
     * thread; implementations must be thread-safe.
     */
    void registerAnomalyCallback(
        std::function<void(const ProfileSnapshot&, const std::string& anomaly)> cb);

    /** @brief Dynamically enable/disable profiling without restarting. */
    void enable();
    void disable();
    bool isEnabled() const;

    /** @brief Retrieve the active configuration. */
    ContinuousProfilerConfig getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis

