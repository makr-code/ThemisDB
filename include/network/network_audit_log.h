/**
 * @file network_audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace network {

/**
 * @brief Types of audit events recorded by NetworkAuditLog.
 */
enum class AuditEventType {
    CONNECTION_OPEN,   ///< New TCP/TLS connection accepted
    CONNECTION_CLOSE,  ///< Connection closed (graceful or error)
    AUTH_SUCCESS,      ///< Client authenticated successfully
    AUTH_FAILURE,      ///< Authentication attempt failed
    RATE_LIMITED,      ///< Connection rejected or throttled by rate limiter
};

/**
 * @brief A single immutable audit event record.
 *
 * Security note: authentication tokens are never stored directly.
 * When a token is involved, only a 16-hex-char prefix of its SHA-256
 * hash is retained (token_hash field).
 */
struct AuditEvent {
    AuditEventType type;

    /// Wall-clock timestamp when the event occurred.
    std::chrono::system_clock::time_point timestamp;

    /// Remote peer address (IP[:port] string).
    std::string remote_address;

    /// Optional: session/connection identifier assigned by the server.
    uint64_t connection_id = 0;

    /// Optional: authenticated principal / user name (empty if not yet known).
    std::string principal;

    /**
     * @brief Truncated SHA-256 hash of the authentication token.
     *
     * Populated only for AUTH_SUCCESS and AUTH_FAILURE events.
     * Format: 16 lower-case hex characters (first 8 bytes of SHA-256).
     * Empty for all other event types.
     */
    std::string token_hash;

    /// Human-readable detail (e.g., close reason, rate-limit rule name).
    std::string detail;
};

/**
 * @brief Thread-safe structured audit log for the network layer.
 *
 * Records connection and authentication events in a fixed-capacity
 * in-memory ring buffer.  An optional callback lets callers forward
 * events to external sinks (file, syslog, OpenTelemetry, …).
 *
 * Design goals:
 * - Zero allocation on the hot path once the ring buffer is full
 *   (old entries are evicted rather than heap-growing the container).
 * - Lock is held only while appending/querying; the user-supplied
 *   callback is invoked *outside* the lock to avoid priority inversion.
 * - Authentication tokens are never stored; only a truncated SHA-256
 *   hash is kept for correlation without leaking credentials.
 *
 * Usage:
 * @code
 * NetworkAuditLog::Config cfg;
 * cfg.max_entries = 10000;
 * NetworkAuditLog log(cfg);
 *
 * log.record({AuditEventType::CONNECTION_OPEN, now, "192.168.1.1:54321", 42});
 *
 * auto recent = log.getRecentEvents(100);
 * @endcode
 */
class NetworkAuditLog {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Maximum number of events kept in the in-memory ring buffer.
        /// When the buffer is full the oldest entry is evicted.
        size_t max_entries = 10'000;

        /// When true, every recorded event is also forwarded to the
        /// user-supplied callback (if one is registered).
        bool enable_callback = true;

        Config() = default;
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    struct Stats {
        uint64_t total_recorded    = 0; ///< Total events appended since construction
        uint64_t total_evicted     = 0; ///< Events dropped due to ring-buffer overflow
        uint64_t connection_opens  = 0;
        uint64_t connection_closes = 0;
        uint64_t auth_successes    = 0;
        uint64_t auth_failures     = 0;
        uint64_t rate_limited      = 0;
        size_t   current_size      = 0; ///< Events currently in the buffer
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    explicit NetworkAuditLog(const Config& config = Config{});
    ~NetworkAuditLog() = default;

    NetworkAuditLog(const NetworkAuditLog&)            = delete;
    NetworkAuditLog& operator=(const NetworkAuditLog&) = delete;
    NetworkAuditLog(NetworkAuditLog&&)                 = delete;
    NetworkAuditLog& operator=(NetworkAuditLog&&)      = delete;

    // -----------------------------------------------------------------------
    // Callback
    // -----------------------------------------------------------------------

    /**
     * @brief Register a sink callback invoked for every new event.
     *
     * The callback is called *outside* the internal mutex, so it may
     * safely call back into the log (e.g., to read stats).  Only one
     * callback can be registered at a time; a second call replaces the
     * previous one.
     *
     * @param cb  Callable receiving a const-reference to the new event.
     *            Pass nullptr to clear the callback.
     */
    void setEventCallback(std::function<void(const AuditEvent&)> cb);

    // -----------------------------------------------------------------------
    // Recording
    // -----------------------------------------------------------------------

    /**
     * @brief Append an event to the audit log.
     *
     * If the ring buffer is at capacity the oldest entry is silently
     * dropped (stats.total_evicted is incremented).
     *
     * @param event  The event to record.  If the event's timestamp is the
     *               default-constructed (epoch), it is replaced with the
     *               current wall-clock time.
     */
    void record(AuditEvent event);

    /**
     * @brief Convenience helper: record a connection-open event.
     *
     * @param remote_address  Peer IP[:port] string.
     * @param connection_id   Server-assigned connection identifier.
     */
    void recordConnectionOpen(const std::string& remote_address,
                              uint64_t connection_id = 0);

    /**
     * @brief Convenience helper: record a connection-close event.
     *
     * @param remote_address  Peer IP[:port] string.
     * @param connection_id   Server-assigned connection identifier.
     * @param reason          Optional human-readable close reason.
     */
    void recordConnectionClose(const std::string& remote_address,
                               uint64_t connection_id = 0,
                               const std::string& reason = {});

    /**
     * @brief Convenience helper: record an authentication event.
     *
     * The raw @p token is *never* stored.  A truncated SHA-256 hash
     * (16 lower-case hex chars) is derived and stored in the event's
     * token_hash field.
     *
     * @param success         true → AUTH_SUCCESS; false → AUTH_FAILURE.
     * @param remote_address  Peer IP[:port] string.
     * @param connection_id   Server-assigned connection identifier.
     * @param principal       User / principal name (may be empty).
     * @param token           Raw token (hashed; never persisted as-is).
     */
    void recordAuth(bool success,
                    const std::string& remote_address,
                    uint64_t connection_id = 0,
                    const std::string& principal = {},
                    const std::string& token = {});

    /**
     * @brief Convenience helper: record a rate-limit rejection event.
     *
     * @param remote_address  Peer IP[:port] string.
     * @param connection_id   Server-assigned connection identifier.
     * @param rule            Name of the rate-limit rule that fired.
     */
    void recordRateLimited(const std::string& remote_address,
                           uint64_t connection_id = 0,
                           const std::string& rule = {});

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    /**
     * @brief Return the @p n most-recent events (newest last).
     *
     * @param n  Maximum number of events to return; 0 means all.
     */
    std::vector<AuditEvent> getRecentEvents(size_t n = 0) const;

    /**
     * @brief Return all events matching the given type.
     */
    std::vector<AuditEvent> getEventsByType(AuditEventType type) const;

    /**
     * @brief Return a snapshot of counters.
     */
    Stats getStats() const;

    /**
     * @brief Clear the in-memory buffer.  Stats are retained.
     */
    void clear();

    // -----------------------------------------------------------------------
    // Helpers (public for testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Compute the 16-char truncated SHA-256 hash used for tokens.
     *
     * Returns the first 8 bytes of the SHA-256 digest of @p input,
     * encoded as lower-case hexadecimal.  Returns an empty string when
     * @p input is empty.
     */
    static std::string truncatedSha256Hex(const std::string& input);

private:
    Config                config_;
    std::deque<AuditEvent> buffer_;  ///< Ring buffer (front = oldest)
    mutable std::mutex     mutex_;

    std::function<void(const AuditEvent&)> callback_;

    // Counters (updated under mutex_)
    uint64_t total_recorded_    = 0;
    uint64_t total_evicted_     = 0;
    uint64_t connection_opens_  = 0;
    uint64_t connection_closes_ = 0;
    uint64_t auth_successes_    = 0;
    uint64_t auth_failures_     = 0;
    uint64_t rate_limited_      = 0;

    void updateCounters(AuditEventType type);
};

} // namespace network
} // namespace themis
