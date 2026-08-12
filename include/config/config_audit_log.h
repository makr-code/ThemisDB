/**
 * @file config_audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace config {

/**
 * A single audit trail entry recording one config path access.
 */
struct AuditEntry {
    std::string requested_path;                       ///< The path as requested by the caller
    std::string resolved_path;                        ///< The final resolved filesystem path
    std::chrono::system_clock::time_point timestamp;  ///< When the access occurred
    bool is_legacy{false};    ///< true if the legacy fallback path was used
    bool is_cache_hit{false}; ///< true if the result came from the LRU cache
};

/**
 * Thread-safe, bounded audit log for config path accesses.
 *
 * When the log reaches max_entries the oldest entries are evicted
 * (ring-buffer semantics).  Audit logging is disabled by default;
 * call enable() to activate it.
 *
 * Thread Safety:
 *   All public methods are protected by an internal mutex and are safe
 *   to call from multiple threads concurrently.
 */
class ConfigAuditLog {
public:
    /// Default maximum number of audit entries retained in memory.
    static constexpr std::size_t kDefaultMaxEntries = 10000;

    /**
     * Enable audit logging.
     * Subsequent calls to record() will store entries.
     */
    void enable();

    /**
     * Disable audit logging.
     * Subsequent calls to record() are no-ops.
     */
    void disable();

    /**
     * Return true if audit logging is currently enabled.
     */
    bool isEnabled() const;

    /**
     * Set the maximum number of entries retained.
     * Entries beyond this limit are evicted oldest-first.
     *
     * @param max  Maximum number of entries (clamped to >= 1).
     */
    void setMaxEntries(std::size_t max);

    /**
     * Return the current maximum entry limit.
     */
    std::size_t maxEntries() const;

    /**
     * Record a config path access.
     * No-op when audit logging is disabled.
     *
     * @param entry  The audit entry to store.
     */
    void record(AuditEntry entry);

    /**
     * Return a snapshot of all current audit entries (oldest first).
     */
    std::vector<AuditEntry> getEntries() const;

    /**
     * Return the number of entries currently stored.
     */
    std::size_t size() const;

    /**
     * Clear all audit entries.
     */
    void clear();

private:
    mutable std::mutex mutex_;
    std::deque<AuditEntry> entries_;
    std::size_t max_entries_{kDefaultMaxEntries};
    // Use atomic for the enabled flag so that isEnabled() and the fast-path
    // check in record() are a single relaxed load with no mutex acquisition.
    // This satisfies the hot-path overhead target of one atomic-equivalent
    // load when audit logging is disabled.
    std::atomic<bool> enabled_{false};
};

} // namespace config
} // namespace themis
