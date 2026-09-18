/**
 * @file i_audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

struct AuditEvent {
    std::string event_type;

    std::string actor;

    std::string resource;

    std::string action;

    std::string outcome;

    int64_t timestamp_ms = 0;

    std::map<std::string, std::string> details;

    // -----------------------------------------------------------------------
    // Convenience factories
    // -----------------------------------------------------------------------

    static AuditEvent make(std::string_view event_type,
                           std::string_view actor,
                           std::string_view resource,
                           std::string_view action,
                           std::string_view outcome,
                           std::map<std::string, std::string> details = {}) {
        using namespace std::chrono;
        AuditEvent ev;
        ev.event_type    = std::string(event_type);
        ev.actor         = std::string(actor);
        ev.resource      = std::string(resource);
        ev.action        = std::string(action);
        ev.outcome       = std::string(outcome);
        ev.timestamp_ms  = static_cast<int64_t>(
            duration_cast<milliseconds>(system_clock::now().time_since_epoch())
                .count());
        ev.details       = std::move(details);
        return ev;
    }

    static AuditEvent success(std::string_view event_type,
                              std::string_view actor,
                              std::string_view resource,
                              std::string_view action,
                              std::map<std::string, std::string> details = {}) {
        return make(event_type, actor, resource, action, "success",
                    std::move(details));
    }

    static AuditEvent denied(std::string_view event_type,
                             std::string_view actor,
                             std::string_view resource,
                             std::string_view action,
                             std::map<std::string, std::string> details = {}) {
        return make(event_type, actor, resource, action, "denied",
                    std::move(details));
    }

    static AuditEvent error(std::string_view event_type,
                            std::string_view actor,
                            std::string_view resource,
                            std::string_view action,
                            std::map<std::string, std::string> details = {}) {
        return make(event_type, actor, resource, action, "error",
                    std::move(details));
    }
};

class IAuditLog {
public:
    /**
     * @brief IAudit Log.
     * @return Return value.
     */
    virtual ~IAuditLog() = default;

    /**
     * @brief Record.
     * @param[in] event Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void record(const AuditEvent& event) noexcept = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    virtual void flush() noexcept {}

    virtual void shutdown() noexcept {}

    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

// ---------------------------------------------------------------------------
// In-memory implementation (for testing and single-process deployments)
// ---------------------------------------------------------------------------

class InMemoryAuditLog : public IAuditLog {
public:
    void record(const AuditEvent& event) noexcept override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
    }

    std::vector<AuditEvent> getEvents() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return events_;
    }

    size_t size() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

    /**
     * @brief Clear.
     * @details Calls: lock().
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.clear();
    }

    void flush() noexcept override {}
    void shutdown() noexcept override { clear(); }
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

private:
    mutable std::mutex mutex_;
    std::vector<AuditEvent> events_;
};

} // namespace concerns
} // namespace core
} // namespace themis
