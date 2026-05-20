/*
 * ThemisDB | File: query_canceller.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=19, M=3, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
