/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            token_quota_manager.cpp                            ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:17:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/token_quota_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {
namespace llm {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/*static*/
void TokenQuotaManager::prune(QuotaEntry& entry) {
    const auto cutoff = std::chrono::steady_clock::now() - WINDOW;
    auto it = std::remove_if(entry.events.begin(), entry.events.end(),
                             [cutoff](const Event& e) { return e.at < cutoff; });
    entry.events.erase(it, entry.events.end());
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void TokenQuotaManager::setQuota(const std::string& user_id,
                                  const std::string& model_id,
                                  size_t limit) {
    const auto key = makeKey(user_id, model_id);
    std::lock_guard<std::mutex> lock(mutex_);
    auto& entry = entries_[key];
    entry.limit = limit;
    spdlog::debug("TokenQuotaManager: set quota user='{}' model='{}' limit={}/min",
                  user_id, model_id, limit);
}

bool TokenQuotaManager::removeQuota(const std::string& user_id,
                                     const std::string& model_id) {
    const auto key = makeKey(user_id, model_id);
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.erase(key) > 0;
}

QuotaCheckResult TokenQuotaManager::check(const std::string& user_id,
                                           const std::string& model_id,
                                           size_t estimated_tokens) const {
    const auto key = makeKey(user_id, model_id);
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end() || it->second.limit == 0) {
        // No quota configured — always allow.
        return QuotaCheckResult{true, "", 0, 0};
    }

    QuotaEntry& entry = const_cast<QuotaEntry&>(it->second);
    prune(entry);

    size_t used = 0;
    for (const auto& ev : entry.events) {
        used += ev.tokens;
    }

    if (used + estimated_tokens > entry.limit) {
        QuotaCheckResult result;
        result.allowed       = false;
        result.tokens_used   = used;
        result.tokens_limit  = entry.limit;
        result.reason        = "Token quota exceeded for user='" + user_id +
                               "' model='" + model_id +
                               "': used=" + std::to_string(used) +
                               " + estimated=" + std::to_string(estimated_tokens) +
                               " > limit=" + std::to_string(entry.limit) + "/min";
        spdlog::warn("TokenQuotaManager: {}", result.reason);
        return result;
    }

    return QuotaCheckResult{true, "", used, entry.limit};
}

void TokenQuotaManager::consume(const std::string& user_id,
                                 const std::string& model_id,
                                 size_t tokens) {
    if (tokens == 0) {
        return;
    }
    const auto key = makeKey(user_id, model_id);
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return;  // No quota entry — consume is a no-op
    }

    it->second.events.push_back({std::chrono::steady_clock::now(), tokens});
    spdlog::debug("TokenQuotaManager: consumed {} tokens user='{}' model='{}'",
                  tokens, user_id, model_id);
}

size_t TokenQuotaManager::currentUsage(const std::string& user_id,
                                        const std::string& model_id) const {
    const auto key = makeKey(user_id, model_id);
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return 0;
    }

    QuotaEntry& entry = const_cast<QuotaEntry&>(it->second);
    prune(entry);

    size_t used = 0;
    for (const auto& ev : entry.events) {
        used += ev.tokens;
    }
    return used;
}

std::optional<size_t> TokenQuotaManager::getLimit(const std::string& user_id,
                                                    const std::string& model_id) const {
    const auto key = makeKey(user_id, model_id);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it->second.limit;
}

} // namespace llm
} // namespace themis
