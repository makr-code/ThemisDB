/**
 * @file config_file_watcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

namespace themis {
namespace config {

class ConfigFileWatcher {
public:
    explicit ConfigFileWatcher(
        std::string watch_path,
        std::function<void()> callback,
        std::chrono::milliseconds debounce = std::chrono::milliseconds(200));

    ~ConfigFileWatcher();

    // Non-copyable, non-movable
    ConfigFileWatcher(const ConfigFileWatcher&) = delete;
    ConfigFileWatcher& operator=(const ConfigFileWatcher&) = delete;
    ConfigFileWatcher(ConfigFileWatcher&&) = delete;
    ConfigFileWatcher& operator=(ConfigFileWatcher&&) = delete;

    /**
     * @brief Start.
     * @return True when the operation succeeds.
     */
    bool start();

    /**
     * @brief Stop.
     */
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    const std::string& watchPath() const { return watch_path_; }

    std::chrono::milliseconds debounceInterval() const { return debounce_; }

private:
    /**
     * @brief ── Entry point for the watcher thread ──────────────────────────────
     */
    void watchLoop();

    // ── Platform-specific watch loops ───────────────────────────────────
#if defined(__linux__)
    /**
     * @brief Watch Loop Inotify.
     */
    void watchLoopInotify();
#elif defined(__APPLE__)
    /**
     * @brief Watch Loop Kqueue.
     */
    void watchLoopKqueue();
#elif defined(_WIN32)
    /**
     * @brief Watch Loop Read Dir Changes.
     */
    void watchLoopReadDirChanges();
#endif

    /**
     * @brief ── Debounce helper ─────────────────────────────────────────────────
     */
    void scheduleCallback();

    // ── Members ─────────────────────────────────────────────────────────
    std::string                watch_path_;
    std::function<void()>      callback_;
    std::chrono::milliseconds  debounce_;

    std::atomic<bool>          running_{false};
    std::thread                thread_;

    // Debounce state
    std::mutex                             debounce_mutex_;
    std::chrono::steady_clock::time_point  last_event_time_;
    bool                                   event_pending_{false};

    // Platform-specific stop signalling
#if defined(__linux__)
    int pipe_read_fd_{-1};
    int pipe_write_fd_{-1};
#elif defined(__APPLE__)
    int kqueue_fd_{-1};
    int pipe_read_fd_{-1};
    int pipe_write_fd_{-1};
#elif defined(_WIN32)
    void* stop_event_{nullptr};   // HANDLE
#endif
};

} // namespace config
} // namespace themis
