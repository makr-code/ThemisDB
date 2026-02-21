/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            token_blacklist.h                                  ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

namespace themis {
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
    };

    explicit TokenBlacklist(const Config& config = Config());
    ~TokenBlacklist() = default;

    // Non-copyable, movable
    TokenBlacklist(const TokenBlacklist&) = delete;
    TokenBlacklist& operator=(const TokenBlacklist&) = delete;
    TokenBlacklist(TokenBlacklist&&) = default;
    TokenBlacklist& operator=(TokenBlacklist&&) = default;

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

    bool needsCleanup() const;
};

} // namespace auth
} // namespace themis
