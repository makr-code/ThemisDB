/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            token_blacklist.cpp                                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/token_blacklist.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

TokenBlacklist::TokenBlacklist(const Config& config)
    : config_(config)
    , last_cleanup_(std::chrono::steady_clock::now())
{}

void TokenBlacklist::revoke(const std::string& jti,
                             std::chrono::system_clock::time_point expires_at) {
    if (jti.empty()) {
        THEMIS_WARN("TokenBlacklist::revoke called with empty JTI – ignored");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Prune first to stay under the cap
    if (needsCleanup()) {
        // Inline prune (lock already held)
        auto now = std::chrono::system_clock::now();
        for (auto it = blacklist_.begin(); it != blacklist_.end(); ) {
            if (it->second.expires_at <= now) {
                it = blacklist_.erase(it);
                stats_.pruned_entries++;
            } else {
                ++it;
            }
        }
        last_cleanup_ = std::chrono::steady_clock::now();
    }

    // Enforce hard cap
    if (blacklist_.size() >= config_.max_entries) {
        THEMIS_WARN("TokenBlacklist: max_entries ({}) reached – dropping oldest entry",
                    config_.max_entries);
        // Remove the entry that expires soonest (cheapest to lose)
        auto oldest = blacklist_.begin();
        for (auto it = blacklist_.begin(); it != blacklist_.end(); ++it) {
            if (it->second.expires_at < oldest->second.expires_at) {
                oldest = it;
            }
        }
        blacklist_.erase(oldest);
        stats_.pruned_entries++;
    }

    blacklist_[jti] = Entry{expires_at};
    stats_.total_revocations++;
    THEMIS_INFO("TokenBlacklist: revoked JTI '{}'", jti);
}

bool TokenBlacklist::isRevoked(const std::string& jti) const {
    if (jti.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    stats_.total_checks++;

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

bool TokenBlacklist::unrevoke(const std::string& jti) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = blacklist_.find(jti);
    if (it == blacklist_.end()) return false;
    blacklist_.erase(it);
    THEMIS_INFO("TokenBlacklist: un-revoked JTI '{}'", jti);
    return true;
}

void TokenBlacklist::pruneExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    for (auto it = blacklist_.begin(); it != blacklist_.end(); ) {
        if (it->second.expires_at <= now) {
            it = blacklist_.erase(it);
            stats_.pruned_entries++;
        } else {
            ++it;
        }
    }
    last_cleanup_ = std::chrono::steady_clock::now();
}

void TokenBlacklist::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    blacklist_.clear();
    THEMIS_INFO("TokenBlacklist cleared");
}

size_t TokenBlacklist::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blacklist_.size();
}

TokenBlacklist::Statistics TokenBlacklist::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Statistics s = stats_;
    s.current_size = blacklist_.size();
    return s;
}

bool TokenBlacklist::needsCleanup() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_cleanup_).count();
    return static_cast<uint32_t>(elapsed) >= config_.cleanup_interval_seconds;
}

} // namespace auth
} // namespace themis
