/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_canceller.cpp                                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:36:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     66                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ab3b22a88e  2026-03-09  feat(query): implement query cancellation via request ID ... ║
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
