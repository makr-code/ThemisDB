/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_audit_log.h                                      ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:44:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     242                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 50ae658f67  2026-03-09  feat(core): implement dynamic log level adjustment and au... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <chrono>
#include &lt;map&gt;
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief A single compliance audit event.
 *
 * Carries the who/what/when/outcome of every security-relevant operation
 * so that a compliance audit trail can be built from these records.
 *
 * Design: plain value type — all fields are `std::string` for easy
 * serialisation; the `details` map holds any additional structured context
 * (e.g. IP address, query text, affected row count).
 */
struct AuditEvent {
    /// Category of event (e.g. "authentication", "data_access",
    /// "config_change", "authorization").
    std::string event_type;

    /// Identity that performed the action (user ID, service account, etc.).
    std::string actor;

    /// The target of the action (table name, collection, secret name, etc.).
    std::string resource;

    /// The performed operation (e.g. "read", "write", "delete", "execute",
    /// "login", "logout").
    std::string action;

    /// Result of the operation: "success", "denied", or "error".
    std::string outcome;

    /// Wall-clock time of the event in milliseconds since the Unix epoch.
    /// Defaults to the current time when constructed via the convenience
    /// factory methods below.
    int64_t timestamp_ms = 0;

    /// Optional structured key/value pairs for additional context.
    std::map<std::string, std::string> details;

    // -----------------------------------------------------------------------
    // Convenience factories
    // -----------------------------------------------------------------------

    /// Build an event with timestamp set to "now".
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

    /// Build a "success" event.
    static AuditEvent success(std::string_view event_type,
                              std::string_view actor,
                              std::string_view resource,
                              std::string_view action,
                              std::map<std::string, std::string> details = {}) {
        return make(event_type, actor, resource, action, "success",
                    std::move(details));
    }

    /// Build a "denied" event (authorization failure).
    static AuditEvent denied(std::string_view event_type,
                             std::string_view actor,
                             std::string_view resource,
                             std::string_view action,
                             std::map<std::string, std::string> details = {}) {
        return make(event_type, actor, resource, action, "denied",
                    std::move(details));
    }

    /// Build an "error" event (unexpected failure).
    static AuditEvent error(std::string_view event_type,
                            std::string_view actor,
                            std::string_view resource,
                            std::string_view action,
                            std::map<std::string, std::string> details = {}) {
        return make(event_type, actor, resource, action, "error",
                    std::move(details));
    }
};

/**
 * @brief Abstract interface for compliance audit logging.
 *
 * Implementations may write to an in-memory buffer (for testing), a
 * structured log sink, a dedicated audit database, or any immutable
 * append-only store required by the compliance policy.
 *
 * Thread-safety: all methods must be safe to call concurrently.
 *
 * Reliability contract:
 * - `record()` MUST NOT throw; any internal failure must be suppressed
 *   and, if possible, surfaced through `isHealthy()`.
 * - Implementations that buffer events MUST flush on `flush()` and
 *   `shutdown()` to ensure no records are silently discarded.
 */
class IAuditLog {
public:
    virtual ~IAuditLog() = default;

    /**
     * @brief Append an audit event to the log.
     *
     * Must be non-throwing and thread-safe.  Any internal failure (e.g.
     * I/O error) should be recorded internally and reflected in
     * `isHealthy()` rather than propagated to the caller.
     *
     * @param event The audit event to record.
     */
    virtual void record(const AuditEvent& event) noexcept = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    /**
     * @brief Flush buffered events to the underlying sink.
     *
     * Call before process suspension to ensure pending records are not
     * lost.  No-op for implementations with synchronous writes.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down the audit log and release resources.
     *
     * Flushes any buffered events before teardown.  After shutdown(),
     * calls to record() are silently dropped.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Probe whether the audit log backend is reachable and healthy.
     *
     * @return ProbeResult with ok=true when the backend can accept writes.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

// ---------------------------------------------------------------------------
// In-memory implementation (for testing and single-process deployments)
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe in-memory audit log.
 *
 * Appends events to an internal vector protected by a mutex.  Suitable for
 * unit tests and environments where a persistent audit trail is not required.
 * Call `getEvents()` in tests to assert on recorded events.
 */
class InMemoryAuditLog : public IAuditLog {
public:
    void record(const AuditEvent& event) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
    }

    /**
     * @brief Return a snapshot of all recorded events.
     *
     * The returned vector is a copy; modifications do not affect the log.
     */
    std::vector<AuditEvent> getEvents() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_;
    }

    /**
     * @brief Return the number of recorded events.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

    /**
     * @brief Discard all recorded events.
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
