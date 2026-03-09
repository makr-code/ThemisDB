/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            token_blacklist.h                                  ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:52:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     188                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 4318adfb2  2026-03-01  feat(auth): add real-time revocation callback to TokenBla... ║
    • 92608937d  2026-02-26  fix: GCC default-arg error in 18 headers - add ::defaults... ║
    • 4b86c62ed  2026-02-24  fix: ROADMAP audit logging status and token_blacklist sta... ║
    • 5e72bf49f  2026-02-24  Add audit logging to TokenBlacklist and ApiKeyAuthenticat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <functional>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

/**
 * @brief JTI-based JWT token blacklist for token revocation
 *
 * Stores the JTI (JWT ID) of revoked tokens together with their
 * expiry timestamps.  Expired entries are pruned automatically so
 * memory consumption stays bounded even under sustained revocation load.
 *
 * Usage:
 *   TokenBlacklist bl;
 *   // Revoke a token at logout or key-compromise:
 *   bl.revoke(claims.jti, claims.expiration);
 *
 *   // On every incoming request, after signature verification:
 *   if (bl.isRevoked(claims.jti)) { reject(); }
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Distributed deployments should back this with a shared cache (Redis)
 * and synchronise revocations across nodes.  This implementation provides
 * the single-node baseline.
 */
class TokenBlacklist {
public:
    struct Config {
        /// How often expired entries are pruned automatically (seconds).
        uint32_t cleanup_interval_seconds = 300;
        /// Hard cap on number of stored JTIs (prevents unbounded growth).
        size_t max_entries = 100'000;
        static Config defaults() { return {}; }
    };

    explicit TokenBlacklist(const Config& config = Config::defaults());
    ~TokenBlacklist() = default;

    // Non-copyable, movable
    TokenBlacklist(const TokenBlacklist&) = delete;
    TokenBlacklist& operator=(const TokenBlacklist&) = delete;
    TokenBlacklist(TokenBlacklist&&) = default;
    TokenBlacklist& operator=(TokenBlacklist&&) = default;

    /**
     * @brief Attach an AuditLogger to receive TOKEN_REVOKED events.
     * Pass nullptr to detach.  The blacklist does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Callback type invoked immediately after a JTI is added to the
     *        blacklist.  Enables real-time push-style invalidation: callers
     *        can react to a revocation event without polling isRevoked().
     *
     * The callback is invoked outside the internal mutex so that it is safe to
     * call any TokenBlacklist method from within the callback body.
     *
     * Signature: void(const std::string& jti)
     */
    using RevocationCallback = std::function<void(const std::string& jti)>;

    /**
     * @brief Register a callback to be invoked synchronously when a JTI is
     *        revoked via revoke().  Only one callback can be registered at a
     *        time; a subsequent call replaces the previous one.
     *
     * @param cb  Callable to invoke on revocation.  Pass an empty function or
     *            call clearOnRevokeCallback() to detach.
     */
    void setOnRevokeCallback(RevocationCallback cb);

    /**
     * @brief Remove a previously registered revocation callback.
     */
    void clearOnRevokeCallback();

    /**
     * @brief Revoke a token by its JTI.
     *
     * @param jti       JWT ID claim of the token to revoke.
     * @param expires_at Token expiry; entries are pruned after this time so the
     *                   blacklist does not grow without bound.
     */
    void revoke(const std::string& jti,
                std::chrono::system_clock::time_point expires_at);

    /**
     * @brief Check whether a JTI is currently on the blacklist.
     *
     * @param jti JWT ID claim to check.
     * @return true if the token has been revoked (and has not yet expired).
     */
    bool isRevoked(const std::string& jti) const;

    /**
     * @brief Remove a JTI from the blacklist (e.g. on admin un-revoke).
     *
     * @param jti JWT ID to remove.
     * @return true if the entry existed and was removed.
     */
    bool unrevoke(const std::string& jti);

    /**
     * @brief Remove all entries whose token expiry is in the past.
     *
     * Called automatically inside revoke() / isRevoked() based on
     * cleanup_interval_seconds, but can also be invoked manually.
     */
    void pruneExpired();

    /**
     * @brief Remove all entries (testing / maintenance).
     */
    void clear();

    /**
     * @brief Current number of blacklisted JTIs (including not-yet-pruned
     *        expired ones).
     */
    size_t size() const;

    struct Statistics {
        size_t current_size          = 0;
        uint64_t total_revocations   = 0;
        uint64_t total_checks        = 0;
        uint64_t revoked_hits        = 0;  ///< isRevoked() returned true
        uint64_t pruned_entries      = 0;
    };

    Statistics getStatistics() const;

private:
    Config config_;

    struct Entry {
        std::chrono::system_clock::time_point expires_at;
    };

    std::unordered_map<std::string, Entry> blacklist_;

    mutable Statistics stats_;
    mutable std::mutex mutex_;
    std::chrono::steady_clock::time_point last_cleanup_;
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.
    RevocationCallback on_revoke_callback_;      ///< Invoked outside mutex on revoke.

    bool needsCleanup() const;
};

} // namespace auth
} // namespace themis
