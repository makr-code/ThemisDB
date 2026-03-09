/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_canceller.cpp                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "query/query_canceller.h"

namespace themis {
namespace query {

// ── QueryCanceller ──────────────────────────────────────────────────────────

QueryCanceller& QueryCanceller::instance() {
    static QueryCanceller inst;
    return inst;
}

std::shared_ptr<QueryCancellationToken>
QueryCanceller::registerQuery(const std::string& request_id) {
    auto token = std::make_shared<QueryCancellationToken>();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tokens_[request_id] = token;
    }
    return token;
}

bool QueryCanceller::cancel(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tokens_.find(request_id);
    if (it == tokens_.end()) {
        return false;
    }
    auto token = it->second.lock();
    if (!token) {
        tokens_.erase(it);
        return false;
    }
    token->cancel();
    return true;
}

void QueryCanceller::unregisterQuery(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_.erase(request_id);
}

} // namespace query
} // namespace themis
