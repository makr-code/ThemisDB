/**
 * @file ldap_connection_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct LDAPPoolConfig {
    int min_idle{2};

    int max_size{16};

    int checkout_timeout_ms{5000};

    // Backward-compatible fields used by the auth tests.
    std::string host;

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

class PooledConnection {
public:
    PooledConnection(const PooledConnection&)            = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;
    PooledConnection(PooledConnection&&);
    PooledConnection& operator=(PooledConnection&&);

    ~PooledConnection();

    LDAP* rawHandle() const noexcept { return handle_; }

    void markStale() noexcept { stale_ = true; }

    bool isStale() const noexcept { return stale_; }

private:
    friend class LDAPConnectionPool;

    PooledConnection(LDAPConnectionPool& pool, LDAP* handle);

    LDAPConnectionPool* pool_{nullptr};
    LDAP*               handle_{nullptr};
    bool                stale_{false};
};

class LDAPConnectionPool {
public:
    /**
     * @brief LDAPConnection Pool.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LDAPConnectionPool(const LDAPPoolConfig& config);
    ~LDAPConnectionPool();

    // Non-copyable, non-movable
    LDAPConnectionPool(const LDAPConnectionPool&)            = delete;
    LDAPConnectionPool& operator=(const LDAPConnectionPool&) = delete;
    LDAPConnectionPool(LDAPConnectionPool&&)                 = delete;
    LDAPConnectionPool& operator=(LDAPConnectionPool&&)      = delete;

    /**
     * @brief Checkout.
     * @return Return value.
     */
    std::unique_ptr<PooledConnection> checkout();

    const LDAPPoolConfig& config() const noexcept { return config_; }

    void setAuditLogger(utils::AuditLogger* logger) noexcept { audit_logger_ = logger; }

    /**
     * @brief ----------------------------------------------------------------------- Metrics accessors (used by auth_metrics) -----------------------------------------------------------------------
     * @return Return value.
     * @note Exception safety: noexcept.
     */

    int poolSize() const noexcept;

    /**
     * @brief Idle Connections.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    int idleConnections() const noexcept;

    /**
     * @brief Active Connections.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    int activeConnections() const noexcept;

private:
    friend class PooledConnection;

    /**
     * @brief Return Connection.
     * @param[in,out] handle Input/output parameter.
     * @param[in] stale Input parameter.
     */
    void returnConnection(LDAP* handle, bool stale);

    /**
     * @brief Create Connection.
     * @return Pointer to the result.
     */
    LDAP* createConnection();

    /**
     * @brief Is Healthy.
     * @param[in,out] handle Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool isHealthy(LDAP* handle) const;

    /**
     * @brief Destroy Handle.
     * @param[in,out] handle Input/output parameter.
     * @note Exception safety: noexcept.
     */
    void destroyHandle(LDAP* handle) noexcept;

    LDAPPoolConfig config_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    bool closing_{false};

    std::deque<LDAP*> idle_;

    std::atomic<int> active_count_{0};

    int total_count_{0};

    utils::AuditLogger* audit_logger_{nullptr};
};

} // namespace auth
} // namespace themis
