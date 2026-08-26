/**
 * @file ldap_connection_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include "auth/auth_audit_logger.h"

// ---------------------------------------------------------------------------
// Forward-declare the platform LDAP handle type without pulling in platform
// headers here.  The implementation (.cpp) includes <ldap.h> / <winldap.h>.
// Compatible re-declarations are explicitly permitted by the C++ standard
// so including both this header and <ldap.h> in the same TU is safe.
// ---------------------------------------------------------------------------
struct ldap;
typedef struct ldap LDAP;

namespace themis {
namespace auth {

/**
 * @brief Configuration parameters used exclusively by the connection pool.
 *
 * These are embedded inside LDAPConfig and consumed by LDAPConnectionPool.
 */
struct LDAPPoolConfig {
    /// Minimum number of idle connections kept alive in the pool.
    int min_idle{2};

    /// Maximum total connections (idle + active) allowed in the pool.
    int max_size{16};

    /**
     * @brief Maximum time (milliseconds) to wait for an available connection
     * before checkout() fails.
     */
    int checkout_timeout_ms{5000};

    /// Connection / TLS setup parameters duplicated from LDAPConfig so the
    /// pool can create new connections independently.
    std::string server_url;
    int         port{389};
    bool        use_tls{false};
    int         connection_timeout_seconds{10};
    int         search_timeout_seconds{10};
};

// ---------------------------------------------------------------------------
// Forward declaration — RAII wrapper returned to callers on checkout.
// ---------------------------------------------------------------------------
class LDAPConnectionPool;

/**
 * @brief RAII wrapper around a checked-out LDAP connection.
 *
 * On destruction the connection is returned to the pool (or evicted if it is
 * marked stale).  Callers obtain a PooledConnection via
 * LDAPConnectionPool::checkout() and use rawHandle() to access the underlying
 * LDAP* for bind / search operations.
 *
 * Example:
 * @code
 *   auto conn = pool.checkout();
 *   if (!conn) { return LDAPAuthResult::Failed("pool exhausted"); }
 *   ldap_sasl_bind_s(conn->rawHandle(), ...);
 * @endcode
 */
class PooledConnection {
public:
    PooledConnection(const PooledConnection&)            = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;
    PooledConnection(PooledConnection&&);
    PooledConnection& operator=(PooledConnection&&);

    ~PooledConnection();

    /// Access the raw LDAP handle.
    LDAP* rawHandle() const noexcept { return handle_; }

    /// Mark this connection as stale so it is evicted (not returned) on destruction.
    void markStale() noexcept { stale_ = true; }

    /// Returns true if the connection is still usable.
    bool isStale() const noexcept { return stale_; }

private:
    friend class LDAPConnectionPool;

    PooledConnection(LDAPConnectionPool& pool, LDAP* handle);

    LDAPConnectionPool* pool_{nullptr};
    LDAP*               handle_{nullptr};
    bool                stale_{false};
};

/**
 * @brief Thread-safe pool of pre-established LDAP connections.
 *
 * The pool maintains a set of idle LDAP* handles that survive across
 * authentication calls.  This avoids the repeated TCP + TLS + bind overhead
 * (typically 10–50 ms) for every user authentication under load.
 *
 * Thread safety:
 * - All public methods are safe to call concurrently.
 * - Connections are protected by an internal mutex.
 * - checkout() blocks (up to checkout_timeout_ms) when no idle connection
 *   is available and the pool is at max_size.
 *
 * Health checking:
 * - On every checkout, the pooled connection is validated by issuing a
 *   lightweight ldap_search_ext_s to the rootDSE ("" base, LDAP_SCOPE_BASE,
 *   requesting supportedLDAPVersion).  Stale connections are evicted and a
 *   fresh one is created transparently.
 *
 * Compliance: NIST SP 800-63B (authentication), SOC 2 CC6.1 (availability).
 */
class LDAPConnectionPool {
public:
    explicit LDAPConnectionPool(const LDAPPoolConfig& config);
    ~LDAPConnectionPool();

    // Non-copyable, non-movable
    LDAPConnectionPool(const LDAPConnectionPool&)            = delete;
    LDAPConnectionPool& operator=(const LDAPConnectionPool&) = delete;
    LDAPConnectionPool(LDAPConnectionPool&&)                 = delete;
    LDAPConnectionPool& operator=(LDAPConnectionPool&&)      = delete;

    /**
     * @brief Check out a connection from the pool.
     *
     * Blocks up to config.checkout_timeout_ms waiting for an available
     * connection.  Returns nullptr if the timeout expires, the pool is
     * shutting down, or LDAP support is not compiled in.
     *
     * @return A non-null std::unique_ptr<PooledConnection> on success, or
     *         nullptr on failure.
     */
    std::unique_ptr<PooledConnection> checkout();

    /// Return the pool configuration.
    const LDAPPoolConfig& config() const noexcept { return config_; }

    /**
     * @brief Attach an audit logger for pool-level security events.
     *
     * [W8-17] When attached, pool exhaustion timeouts emit a structured
     * PROVIDER_DEGRADED audit event via @p logger so operators can correlate
     * pool saturation with downstream auth failures.
     *
     * @param logger Non-owning; may be nullptr (disables audit events).
     */
    void setAuditLogger(utils::AuditLogger* logger) noexcept { audit_logger_ = logger; }

    // -----------------------------------------------------------------------
    // Metrics accessors (used by auth_metrics)
    // -----------------------------------------------------------------------

    /// Total capacity of the pool (idle + active slots, capped at max_size).
    int poolSize() const noexcept;

    /// Number of connections currently sitting idle in the pool.
    int idleConnections() const noexcept;

    /// Number of connections currently checked out by callers.
    int activeConnections() const noexcept;

private:
    friend class PooledConnection;

    /// Called by ~PooledConnection to return or evict a handle.
    void returnConnection(LDAP* handle, bool stale);

    /// Create and initialise a new LDAP connection (does NOT bind user credentials).
    LDAP* createConnection();

    /// Perform a lightweight health-check on an existing connection.
    /// Returns true if the connection is alive, false if it should be evicted.
    bool isHealthy(LDAP* handle) const;

    /// Destroy and free an LDAP handle.
    void destroyHandle(LDAP* handle) noexcept;

    LDAPPoolConfig config_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    /// Set to true when the pool is shutting down; causes checkout() to return
    /// nullptr and returnConnection() to destroy handles rather than re-pool them.
    bool closing_{false};

    /// Idle connections available for checkout.
    std::deque<LDAP*> idle_;

    /// Number of connections currently checked out.
    std::atomic<int> active_count_{0};

    /// Total live connections (idle + active); used to enforce max_size.
    int total_count_{0};

    /// [W8-17] Non-owning optional audit logger for pool-level security events.
    utils::AuditLogger* audit_logger_{nullptr};
};

} // namespace auth
} // namespace themis

