/**
 * @file policy_file_watcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PolicyFileWatcher {
public:
    struct Config {
        std::chrono::milliseconds poll_interval{200};
        std::chrono::milliseconds debounce{500};
        std::function<void(bool, const std::string&)> reload_cb;
    };

    /**
     * @brief Policy File Watcher.
     * @param[in,out] engine Input/output parameter.
     * @return Return value.
     */
    explicit PolicyFileWatcher(PolicyEngine& engine);
    /**
     * @brief Policy File Watcher.
     * @param[in,out] engine Input/output parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PolicyFileWatcher(PolicyEngine& engine, Config config);

    ~PolicyFileWatcher();

    // Non-copyable, non-movable (holds a reference to the engine)
    PolicyFileWatcher(const PolicyFileWatcher&) = delete;
    PolicyFileWatcher& operator=(const PolicyFileWatcher&) = delete;

    /**
     * @brief Start.
     * @return True when the operation succeeds.
     */
    bool start();

    /**
     * @brief Stop.
     */
    void stop();

    bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    uint64_t reloadSuccessCount() const noexcept {
        return reload_success_count_.load(std::memory_order_relaxed);
    }

    uint64_t reloadFailureCount() const noexcept {
        return reload_failure_count_.load(std::memory_order_relaxed);
    }

private:
    /**
     * @brief Run.
     */
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
