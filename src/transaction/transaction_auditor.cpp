/**
 * @file transaction_auditor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/transaction_auditor.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// enableAuditing
// ─────────────────────────────────────────────────────────────────────────────

void TransactionAuditor::enableAuditing([[maybe_unused]] bool enabled)
{
    enabled_.store(enabled, std::memory_order_release);
}

// ─────────────────────────────────────────────────────────────────────────────
// record
// ─────────────────────────────────────────────────────────────────────────────

void TransactionAuditor::record(AuditRecord record)
{
    if (!enabled_.load(std::memory_order_acquire)) {
      return;
    }

    std::lock_guard<std::mutex> lk(log_mutex_);
    // Sprint 8 Phase 1 (GAP A-2): Record is moved to log vector.
    // All subsequent accesses use index-based iteration (queryAuditLog), never direct references.
    // Pattern: Move to vector, access via iterator/index; never access moved object.
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

    std::vector<AuditRecord> result = {};

    result.reserve(limit == 0 ?static_cast<int>(log_.size()) : std::min(log_.size(), limit));

    // Iterate in reverse (most-recent-first).
    for (auto it = log_.rbegin(); it != log_.rend(); ++it) {
        const auto& rec = *it;

        if (user_id    && rec.user_id   != *user_id) {
          continue;
        }
        if (start_time && rec.timestamp <  *start_time) {
          continue;
        }
        if (end_time   && rec.timestamp >  *end_time) {
          continue;
        }

        result.push_back(rec);
        if (limit != 0 && static_cast<int>(result.size()) >= limit) {
          break;
        }
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// size / clear
// ─────────────────────────────────────────────────────────────────────────────

size_t TransactionAuditor::size() const
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    return static_cast<int>(log_.size());
}

void TransactionAuditor::clear()
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    log_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Serialise a single AuditRecord to a compact JSON object.
nlohmann::json recordToJson(const TransactionAuditor::AuditRecord& rec) {
    using json = nlohmann::json;

    // Timestamp → ISO-8601 string
    auto t = std::chrono::system_clock::to_time_t(rec.timestamp);
    std::ostringstream ts_stream = {};
    ts_stream << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");

    auto result_str = [&]() -> std::string {
        switch (rec.result) {
            case TransactionAuditor::AuditRecord::Result::COMMITTED: return "COMMITTED";
            case TransactionAuditor::AuditRecord::Result::ABORTED:   return "ABORTED";
            case TransactionAuditor::AuditRecord::Result::DEADLOCK:  return "DEADLOCK";
        }
        return "UNKNOWN";
    };

    json ops = json::array();
    for (const auto& op : rec.operations) {
        json j;
        switch (op.type) {
            case TransactionAuditor::Operation::Type::PUT:         j["type"] = "PUT";         break;
            case TransactionAuditor::Operation::Type::DELETE:      j["type"] = "DELETE";      break;
            case TransactionAuditor::Operation::Type::ADD_EDGE:    j["type"] = "ADD_EDGE";    break;
            case TransactionAuditor::Operation::Type::DELETE_EDGE: j["type"] = "DELETE_EDGE"; break;
            case TransactionAuditor::Operation::Type::ADD_VECTOR:  j["type"] = "ADD_VECTOR";  break;
        }
        j["table"] = op.table;
        j["key"]   = op.key;
        if (op.old_value) {
          j["old_value"] = *op.old_value;
        }
        if (op.new_value) {
          j["new_value"] = *op.new_value;
        }
        ops.push_back(std::move(j));
    }

    return {
        {"txn_id",      rec.txn_id},
        {"user_id",     rec.user_id},
        {"session_id",  rec.session_id},
        {"timestamp",   ts_stream.str()},
        {"isolation",   static_cast<int>(rec.isolation)},
        {"result",      result_str()},
        {"duration_us", rec.duration_us},
        {"operations",  std::move(ops)}
    };
}

/// Serialise all records to newline-delimited JSON (NDJSON).
std::string serializeToNDJSON(const std::vector<TransactionAuditor::AuditRecord>& records) {
    std::string out = {};
    out.reserve(records.size() * 256);
    for (const auto& rec : records) {
        out += recordToJson(rec).dump();
        out += '\n';
    }
    return out;
}

/// Build an S3 object key from a prefix and the current UTC time.
std::string buildS3Key(const std::string& prefix) {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss = {};
    ss << std::put_time(std::gmtime(&t), "%Y%m%dT%H%M%SZ");
    std::string key = prefix;
    if (!key.empty() && key.back() != '/') {
      key += '/';
    }
    key += "audit_" + ss.str() + ".ndjson";
    return key;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// setExportTransport
// ─────────────────────────────────────────────────────────────────────────────

void TransactionAuditor::setExportTransport(IAuditExportTransport* transport)
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    export_transport_ = transport;
}

// ─────────────────────────────────────────────────────────────────────────────
// exportToKafka / exportToS3
// ─────────────────────────────────────────────────────────────────────────────

TransactionAuditor::Status TransactionAuditor::exportToKafka(const std::string& topic)
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    if (!export_transport_) {
        return Status::Error("export transport not configured — "
                             "call setExportTransport() before exporting");
    }
    if (log_.empty()) {
        return Status::OK();
    }
    const std::string payload = serializeToNDJSON(log_);
    return export_transport_->sendKafka(topic, payload);
}

TransactionAuditor::Status TransactionAuditor::exportToS3(const std::string& bucket,
                                                          const std::string& prefix)
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    if (!export_transport_) {
        return Status::Error("export transport not configured — "
                             "call setExportTransport() before exporting");
    }
    if (log_.empty()) {
        return Status::OK();
    }
    const std::string payload = serializeToNDJSON(log_);
    const std::string key     = buildS3Key(prefix);
    return export_transport_->writeS3(bucket, key, payload);
}

} // namespace themis

