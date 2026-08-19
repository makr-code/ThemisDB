/**
 * @file ldap_connection_pool.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/ldap_connection_pool.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <spdlog/spdlog.h>

// ---------------------------------------------------------------------------
// Platform-specific LDAP includes
// ---------------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
#include <winldap.h>
#define THEMIS_HAS_LDAP 1
#elif defined(THEMIS_HAS_LDAP)
#include <lber.h>
#include <ldap.h>
#endif

namespace themis {
namespace auth {

// ===========================================================================
// PooledConnection
// ===========================================================================

PooledConnection::PooledConnection(LDAPConnectionPool &pool, LDAP *handle)
    : pool_(&pool), handle_(handle), stale_(false) {}

PooledConnection::PooledConnection(PooledConnection &&other)
    : pool_(other.pool_), handle_(other.handle_), stale_(other.stale_) {
    other.pool_   = nullptr;
    other.handle_ = nullptr;
}

PooledConnection &PooledConnection::operator=(PooledConnection &&other) {
    if (this != &other) {
        // Return the current handle before stealing the new one.
        if (pool_ && handle_) {
            pool_->returnConnection(handle_, stale_);
        }
        pool_         = other.pool_;
        handle_       = other.handle_;
        stale_        = other.stale_;
        other.pool_   = nullptr;
        other.handle_ = nullptr;
    }
    return *this;
}

PooledConnection::~PooledConnection() {
    if (pool_ && handle_) {
        pool_->returnConnection(handle_, stale_);
    }
}

// ===========================================================================
// LDAPConnectionPool — constructor / destructor
// ===========================================================================

LDAPConnectionPool::LDAPConnectionPool(const LDAPPoolConfig &config) : config_(config) {
#ifdef THEMIS_HAS_LDAP
    // Pre-warm the pool with min_idle connections.
    const int initial = std::max(0, std::min(config_.min_idle, config_.max_size));
    for (int i = 0; i < initial; ++i) {
        LDAP *ld = createConnection();
        if (ld) {
            idle_.push_back(ld);
            ++total_count_;
        }
    }
    spdlog::info("LDAPConnectionPool: initialised pool (server={}, min_idle={}, "
                 "max_size={}, pre-connected={})",
                 config_.server_url, config_.min_idle, config_.max_size, static_cast<int>(idle_.size()));
#else
    spdlog::warn("LDAPConnectionPool: LDAP support not compiled in — pool disabled");
#endif
}

LDAPConnectionPool::~LDAPConnectionPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closing_ = true;
        // Destroy all idle handles immediately.
        for (LDAP *ld : idle_) {
            destroyHandle(ld);
            --total_count_;
        }
        idle_.clear();
    }
    // Wake any threads blocked in checkout() so they can observe closing_.
    cv_.notify_all();

    // Wait (bounded) for active connections to be returned.
    // This avoids use-after-free when PooledConnection outlives the pool.
    constexpr int kShutdownWaitMs = 5000;
    const auto deadline           = std::chrono::steady_clock::now() + std::chrono::milliseconds(kShutdownWaitMs);
    std::unique_lock<std::mutex> lock(mutex_);
    while (active_count_.load(std::memory_order_acquire) > 0) {
        if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            spdlog::warn("LDAPConnectionPool: destructor timed out waiting for "
                         "{} active connection(s) to be returned",
                         active_count_.load());
            break;
        }
    }
}

// ===========================================================================
// checkout
// ===========================================================================

std::unique_ptr<PooledConnection> LDAPConnectionPool::checkout() {
#ifndef THEMIS_HAS_LDAP
    spdlog::warn("LDAPConnectionPool::checkout: LDAP not compiled in");
    return nullptr;
#else
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(config_.checkout_timeout_ms);

    std::unique_lock<std::mutex> lock(mutex_);

    while (true) {
        // Fail fast if the pool is shutting down.
        if (closing_) {
            return nullptr;
        }

        // --- 1. Try to pop an idle connection --------------------------------
        while (!idle_.empty()) {
            LDAP *candidate = idle_.front();
            idle_.pop_front();

            // Health-check the candidate outside the lock to avoid blocking
            // other callers while we wait for the LDAP round-trip.
            lock.unlock();
            const bool healthy = isHealthy(candidate);
            lock.lock();

            if (closing_) {
                // Pool shut down while we were health-checking; evict and bail.
                destroyHandle(candidate);
                --total_count_;
                return nullptr;
            }

            if (healthy) {
                ++active_count_;
                // total_count_ is unchanged (idle → active)
                return std::unique_ptr<PooledConnection>(new PooledConnection(*this, candidate));
            }

            // Stale — evict and decrement total.
            spdlog::debug("LDAPConnectionPool: evicting stale connection");
            destroyHandle(candidate);
            --total_count_;
        }

        // --- 2. No idle connection available — create one if capacity permits
        if (total_count_ < config_.max_size) {
            lock.unlock();
            LDAP *fresh = createConnection();
            lock.lock();

            if (fresh) {
                ++total_count_;
                ++active_count_;
                return std::unique_ptr<PooledConnection>(new PooledConnection(*this, fresh));
            }
            // createConnection() failed — fall through to waiting/timeout
        }

        // --- 3. Pool at capacity — wait for a connection to be returned -----
        if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            spdlog::warn("LDAPConnectionPool::checkout: timeout waiting for "
                         "connection (active={}, idle={})",
                         active_count_.load(), static_cast<int>(idle_.size()));
            return nullptr;
        }
        // Woken — retry from the top.
    }
#endif
}

// ===========================================================================
// returnConnection (called by ~PooledConnection)
// ===========================================================================

void LDAPConnectionPool::returnConnection(LDAP *handle, bool stale) {
    if (!handle) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        --active_count_;

        if (!closing_ && !stale && static_cast<int>(idle_.size()) < config_.max_size) {
            idle_.push_back(handle);
            cv_.notify_one();
            return;
        }

        // Either closing, stale, or pool already has enough idle connections — destroy.
        destroyHandle(handle);
        --total_count_;
    }
    // Notify: wakes checkout() waiters AND the destructor's active_count_ wait.
    cv_.notify_all();
}

// ===========================================================================
// createConnection
// ===========================================================================

LDAP *LDAPConnectionPool::createConnection() {
#ifndef THEMIS_HAS_LDAP
    return nullptr;
#elif defined(_WIN32)
    LDAP *ld = ldap_init(const_cast<PCHAR>(config_.server_url.c_str()),
                         config_.port > 0 ? static_cast<ULONG>(config_.port) : 389U);
    if (!ld) {
        spdlog::error("LDAPConnectionPool: ldap_init failed for server {}", config_.server_url);
        return nullptr;
    }

    ULONG timelimit = static_cast<ULONG>(config_.search_timeout_seconds);
    ldap_set_option(ld, LDAP_OPT_TIMELIMIT, static_cast<void *>(&timelimit));

    if (ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF) != LDAP_SUCCESS) {
        spdlog::error("LDAPConnectionPool: failed to disable referrals on new connection");
        ldap_unbind(ld);
        return nullptr;
    }

    if (config_.use_tls) {
        if (ldap_start_tls_s(ld, nullptr, nullptr, nullptr, nullptr) != LDAP_SUCCESS) {
            spdlog::error("LDAPConnectionPool: StartTLS failed for server {}", config_.server_url);
            ldap_unbind(ld);
            return nullptr;
        }
    }
    return ld;

#else // POSIX / OpenLDAP
    LDAP *ld = nullptr;
    int rc   = ldap_initialize(&ld, config_.server_url.c_str());
    if (rc != LDAP_SUCCESS || !ld) {
        spdlog::error("LDAPConnectionPool: ldap_initialize failed for server {}: {}", config_.server_url,
                      ldap_err2string(rc));
        return nullptr;
    }

    int version = LDAP_VERSION3;
    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

    struct timeval conn_tv{};
    conn_tv.tv_sec  = config_.connection_timeout_seconds;
    conn_tv.tv_usec = 0;
    ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &conn_tv);

    struct timeval srch_tv{};
    srch_tv.tv_sec  = config_.search_timeout_seconds;
    srch_tv.tv_usec = 0;
    ldap_set_option(ld, LDAP_OPT_TIMEOUT, &srch_tv);

    rc = ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
    if (rc != LDAP_SUCCESS) {
        spdlog::error("LDAPConnectionPool: failed to disable referrals: {}", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return nullptr;
    }

    if (config_.use_tls) {
        rc = ldap_start_tls_s(ld, nullptr, nullptr);
        if (rc != LDAP_SUCCESS) {
            spdlog::error("LDAPConnectionPool: StartTLS failed for server {}: {}", config_.server_url,
                          ldap_err2string(rc));
            ldap_unbind_ext_s(ld, nullptr, nullptr);
            return nullptr;
        }
    }
    return ld;
#endif
}

// ===========================================================================
// isHealthy — rootDSE ping
// ===========================================================================

bool LDAPConnectionPool::isHealthy(LDAP *handle) const {
    if (!handle) {
        return false;
    }

#ifndef THEMIS_HAS_LDAP
    return false;
#elif defined(_WIN32)
    // On Windows, do a base-scope search on the rootDSE.
    PCHAR attrs[]       = {const_cast<PCHAR>("supportedLDAPVersion"), nullptr};
    LDAPMessage *result = nullptr;
    const ULONG rc      = ldap_search_s(handle,
                                        const_cast<PCHAR>(""), // empty base = rootDSE
                                        LDAP_SCOPE_BASE, const_cast<PCHAR>("(objectClass=*)"), attrs, 0, &result);
    if (result) {
        ldap_msgfree(result);
    }
    return rc == LDAP_SUCCESS;

#else // POSIX / OpenLDAP
    const char *attrs[] = {"supportedLDAPVersion", nullptr};
    LDAPMessage *result = nullptr;

    // Use the configured search timeout for the liveness probe, but cap it at
    // 2 s so that a misconfigured large timeout doesn't stall checkout().
    struct timeval tv{};
    tv.tv_sec  = std::min(config_.search_timeout_seconds, 2);
    tv.tv_usec = 0;

    const int rc = ldap_search_ext_s(handle,
                                     "", // empty base = rootDSE
                                     LDAP_SCOPE_BASE, "(objectClass=*)", const_cast<char **>(attrs), 0, nullptr,
                                     nullptr, &tv, LDAP_NO_LIMIT, &result);
    if (result) {
        ldap_msgfree(result);
    }
    return rc == LDAP_SUCCESS;
#endif
}

// ===========================================================================
// destroyHandle
// ===========================================================================

void LDAPConnectionPool::destroyHandle(LDAP *handle) noexcept {
    if (!handle) {
        return;
    }
#ifdef THEMIS_HAS_LDAP
#ifdef _WIN32
    ldap_unbind(handle);
#else
    ldap_unbind_ext_s(handle, nullptr, nullptr);
#endif
#endif
}

// ===========================================================================
// Metrics accessors
// ===========================================================================

int LDAPConnectionPool::poolSize() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_count_;
}

int LDAPConnectionPool::idleConnections() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(idle_.size());
}

int LDAPConnectionPool::activeConnections() const noexcept {
    return active_count_.load(std::memory_order_relaxed);
}

} // namespace auth
} // namespace themis
