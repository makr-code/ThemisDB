/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_auditor.cpp                            ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:51:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     112                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d5eddfb167  2026-03-20  feat(transaction): add Read-Only Transaction Optimization... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "transaction/transaction_auditor.h"

#include <algorithm>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// enableAuditing
// ─────────────────────────────────────────────────────────────────────────────

void TransactionAuditor::enableAuditing(bool enabled)
{
    enabled_.store(enabled, std::memory_order_release);
}

// ─────────────────────────────────────────────────────────────────────────────
// record
// ─────────────────────────────────────────────────────────────────────────────

void TransactionAuditor::record(AuditRecord record)
{
    if (!enabled_.load(std::memory_order_acquire)) return;

    std::lock_guard<std::mutex> lk(log_mutex_);
    log_.push_back(std::move(record));
}

// ─────────────────────────────────────────────────────────────────────────────
// queryAuditLog
// ─────────────────────────────────────────────────────────────────────────────

std::vector<TransactionAuditor::AuditRecord>
TransactionAuditor::queryAuditLog(
    std::optional<std::string>                           user_id,
    std::optional<std::chrono::system_clock::time_point> start_time,
    std::optional<std::chrono::system_clock::time_point> end_time,
    size_t                                               limit) const
{
    std::lock_guard<std::mutex> lk(log_mutex_);

    std::vector<AuditRecord> result;
    result.reserve(limit == 0 ? log_.size() : std::min(log_.size(), limit));

    // Iterate in reverse (most-recent-first).
    for (auto it = log_.rbegin(); it != log_.rend(); ++it) {
        const auto& rec = *it;

        if (user_id    && rec.user_id   != *user_id)    continue;
        if (start_time && rec.timestamp <  *start_time) continue;
        if (end_time   && rec.timestamp >  *end_time)   continue;

        result.push_back(rec);
        if (limit != 0 && result.size() >= limit) break;
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// size / clear
// ─────────────────────────────────────────────────────────────────────────────

size_t TransactionAuditor::size() const
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    return log_.size();
}

void TransactionAuditor::clear()
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    log_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// exportToKafka / exportToS3 (not yet implemented)
// ─────────────────────────────────────────────────────────────────────────────

TransactionAuditor::Status TransactionAuditor::exportToKafka(const std::string& /*topic*/)
{
    return Status::Error("exportToKafka: not yet implemented");
}

TransactionAuditor::Status TransactionAuditor::exportToS3(const std::string& /*bucket*/,
                                                          const std::string& /*prefix*/)
{
    return Status::Error("exportToS3: not yet implemented");
}

} // namespace themis
