/**
 * @file policy_file_watcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace themis {
namespace governance {

class PolicyEngine;

/**
 * @brief Background file-watcher for automatic governance policy hot-reload.
 *
 * Monitors the YAML policy file that was last loaded into a `PolicyEngine` and
 * calls `PolicyEngine::reloadIfChanged()` whenever the file's modification time
 * changes, enabling zero-downtime policy updates without restarting the server.
 *
 * The watcher runs a background thread that polls at `poll_interval` intervals.
 * A reload is triggered only after the file has been stable (no further changes
 * detected) for at least `debounce` milliseconds, reducing the risk of reading a
 * partially-written file.
 *
 * Thread-safety: `start()` and `stop()` may be called from any thread.
 * The watcher does not outlive the `PolicyEngine` reference; callers must stop
 * the watcher before destroying the engine.
 */
class PolicyFileWatcher {
public:
    struct Config {
        /// How often the watcher polls the file system (default: 200 ms).
        std::chrono::milliseconds poll_interval{200};
        /// Minimum quiet period after a detected change before triggering a
        /// reload (default: 500 ms, per FUTURE_ENHANCEMENTS.md spec).
        std::chrono::milliseconds debounce{500};
        /// Optional callback invoked after each reload attempt.
        /// Signature: reload_cb(success, error_message)
        std::function<void(bool, const std::string&)> reload_cb;
    };

    /**
     * @brief Construct a watcher bound to the given PolicyEngine.
     *
     * @param engine  Governance PolicyEngine to reload on file change.
     *                Must outlive this watcher.
     */
    explicit PolicyFileWatcher(PolicyEngine& engine);
    /**
     * @brief Construct a watcher bound to the given PolicyEngine.
     *
     * @param engine  Governance PolicyEngine to reload on file change.
     *                Must outlive this watcher.
     * @param config  Watcher configuration.
     */
    explicit PolicyFileWatcher(PolicyEngine& engine, Config config);

    ~PolicyFileWatcher();

    // Non-copyable, non-movable (holds a reference to the engine)
    PolicyFileWatcher(const PolicyFileWatcher&) = delete;
    PolicyFileWatcher& operator=(const PolicyFileWatcher&) = delete;

    /**
     * @brief Start the background monitoring thread.
     * @return true  if started (or already running).
     */
    bool start();

    /**
     * @brief Stop the monitoring thread and join it synchronously.
     */
    void stop();

    /// @return true if the background thread is currently running.
    bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    /// @return Number of successful policy reloads since the last start().
    uint64_t reloadSuccessCount() const noexcept {
        return reload_success_count_.load(std::memory_order_relaxed);
    }

    /// @return Number of failed reload attempts since the last start().
    uint64_t reloadFailureCount() const noexcept {
        return reload_failure_count_.load(std::memory_order_relaxed);
    }

private:
    void run();

    PolicyEngine& engine_;
    Config config_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> reload_success_count_{0};
    std::atomic<uint64_t> reload_failure_count_{0};
};

} // namespace governance
} // namespace themis
