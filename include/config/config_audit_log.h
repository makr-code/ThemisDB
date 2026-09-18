/**
 * @file config_audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct AuditEntry {
    std::string requested_path;                       ///< The path as requested by the caller
    std::string resolved_path;                        ///< The final resolved filesystem path
    std::chrono::system_clock::time_point timestamp;  ///< When the access occurred
    bool is_legacy{false};    ///< true if the legacy fallback path was used
    bool is_cache_hit{false}; ///< true if the result came from the LRU cache
};

class ConfigAuditLog {
public:
    static constexpr std::size_t kDefaultMaxEntries = 10000;

    /**
     * @brief Enable.
     */
    void enable();

    /**
     * @brief Disable.
     */
    void disable();

    /**
     * @brief Is Enabled.
     * @return True when the operation succeeds.
     */
    bool isEnabled() const;

    /**
     * @brief Set Max Entries.
     * @param[in] max Input parameter.
     */
    void setMaxEntries(std::size_t max);

    /**
     * @brief Max Entries.
     * @return Return value.
     */
    std::size_t maxEntries() const;

    /**
     * @brief Record.
     * @param[in] entry Input parameter.
     */
    void record(AuditEntry entry);

    /**
     * @brief Get Entries.
     * @return Return value.
     */
    std::vector<AuditEntry> getEntries() const;

    /**
     * @brief Size.
     * @return Return value.
     */
    std::size_t size() const;

    /**
     * @brief Clear.
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
