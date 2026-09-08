/**
 * @file token_blacklist.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/token_blacklist.h"

#include "auth/auth_audit_logger.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

TokenBlacklist::TokenBlacklist(const Config &config)
    : config_(config), bloom_(config.max_entries), last_cleanup_(std::chrono::steady_clock::now()) {}

// ============================================================================
// ITokenBlacklist interface implementation
// ============================================================================

void TokenBlacklist::add(const std::string &jti, std::chrono::system_clock::time_point expiry) {
    revoke(jti, expiry);
}

void TokenBlacklist::purgeExpired() {
    pruneExpired();
}

// ============================================================================
// Core implementation
// ============================================================================

void TokenBlacklist::revoke(const std::string &jti, std::chrono::system_clock::time_point expires_at) {
    if (jti.empty()) {
        THEMIS_WARN("TokenBlacklist::revoke called with empty JTI – ignored");
        return;
    }

    RevocationCallback cb_snapshot;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Prune first to stay under the cap
        if (needsCleanup()) {
            pruneExpiredLocked();
        }

        // Enforce hard cap
        if (blacklist_.size() >= static_cast<size_t>(config_.max_entries)) {
            THEMIS_WARN("TokenBlacklist: max_entries ({}) reached – dropping oldest entry", config_.max_entries);
            // Remove the entry that expires soonest (cheapest to lose)
            auto oldest = blacklist_.begin();
            for (auto it = blacklist_.begin(); it != blacklist_.end(); ++it) {
                if (it->second.expires_at < oldest->second.expires_at) {
                    oldest = it;
                }
            }
            blacklist_.erase(oldest);
            stats_.pruned_entries++;
            // Bloom filter is not updated on single eviction; a rebuild
            // happens on the next scheduled pruneExpiredLocked() pass.
        }

        blacklist_[jti] = Entry{expires_at};
        bloom_.add(jti);
        stats_.total_revocations++;

        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logTokenRevoked(jti, "");
        }

        // Snapshot the callback while the lock is held; call it after release
        // to prevent deadlock if the callback re-enters this object.
        cb_snapshot = on_revoke_callback_;
    } // mutex_ released here

    THEMIS_INFO("TokenBlacklist: revoked JTI '{}'", jti);

    if (cb_snapshot) {
        cb_snapshot(jti);
    }
}

bool TokenBlacklist::isRevoked(const std::string &jti) const {
    if (jti.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    stats_.total_checks++;

    // Bloom filter fast path: a definitive NO means the token is not revoked.
    // False positives cause a fall-through to the hash-map lookup.
    if (!bloom_.mayContain(jti)) {
        stats_.bloom_negatives++;
        return false;
    }

    auto it = blacklist_.find(jti);
    if (it == blacklist_.end()) {
        return false;
    }

    // Treat expired blacklist entries as cleared (token expired naturally)
    auto now = std::chrono::system_clock::now();
    if (it->second.expires_at <= now) {
        return false;
    }

    stats_.revoked_hits++;
    return true;
}

bool TokenBlacklist::unrevoke(const std::string &jti) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = blacklist_.find(jti);
    if (it == blacklist_.end()) {
        return false;
    }
    blacklist_.erase(it);
    // Bloom filter cannot remove individual entries; the filter may still
    // return true for this JTI until the next rebuild (pruneExpiredLocked).
    // The hash-map lookup after a false-positive will correctly return false.
    THEMIS_INFO("TokenBlacklist: un-revoked JTI '{}'", jti);
    return true;
}

void TokenBlacklist::pruneExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    pruneExpiredLocked();
}

void TokenBlacklist::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    blacklist_.clear();
    bloom_.reset();
    THEMIS_INFO("TokenBlacklist cleared");
}

size_t TokenBlacklist::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(blacklist_.size());
}

TokenBlacklist::Statistics TokenBlacklist::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Statistics s   = stats_;
    s.current_size = blacklist_.size();
    return s;
}

bool TokenBlacklist::needsCleanup() const {
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup_).count();
    return static_cast<uint32_t>(elapsed) >= config_.cleanup_interval_seconds;
}

void TokenBlacklist::pruneExpiredLocked() {
    auto now = std::chrono::system_clock::now();

    // Rebuild the Bloom filter in the same pass that prunes expired entries
    // so we only traverse the blacklist once while holding the mutex.
    bloom_.reset();

    for (auto it = blacklist_.begin(); it != blacklist_.end();) {
        if (it->second.expires_at <= now) {
            it = blacklist_.erase(it);
            stats_.pruned_entries++;
        } else {
            bloom_.add(it->first);
            ++it;
        }
    }

    last_cleanup_ = std::chrono::steady_clock::now();
}

void TokenBlacklist::setOnRevokeCallback(RevocationCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_revoke_callback_ = std::move(cb);
}

void TokenBlacklist::clearOnRevokeCallback() {
    std::lock_guard<std::mutex> lock(mutex_);
    on_revoke_callback_ = RevocationCallback{};
}

} // namespace auth
} // namespace themis

