/**
 * @file crash_recovery_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/crash_recovery_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <array>

namespace themis {
namespace transaction {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ int64_t CrashRecoveryManager::nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Simple RFC 4648 base64 (no-padding variant for compactness)
static constexpr const char* B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*static*/ std::string CrashRecoveryManager::base64Encode(const std::string& s) {
    std::string out = {};
    out.reserve(((static_cast<int>(s.size()) + 2) / 3) * 4);
    size_t i = 0;
    const auto* p = reinterpret_cast<const unsigned char*>(s.data());
    for (; i + 2 < s.size(); i += 3) {
        out += B64_CHARS[(p[i]   >> 2)];
        out += B64_CHARS[((p[i]   & 0x03) << 4) | (p[i+1] >> 4)];
        out += B64_CHARS[((p[i+1] & 0x0F) << 2) | (p[i+2] >> 6)];
        out += B64_CHARS[  p[i+2] & 0x3F];
    }
    if (static_cast<int>(s.size()) > i) {
        out += B64_CHARS[(p[i] >> 2)];
        if (i + 1 < s.size()) {
            out += B64_CHARS[((p[i] & 0x03) << 4) | (p[i+1] >> 4)];
            out += B64_CHARS[ (p[i+1] & 0x0F) << 2];
        } else {
            out += B64_CHARS[(p[i] & 0x03) << 4];
            out += '=';
        }
        out += '=';
    }
    return out;
}

/*static*/ std::string CrashRecoveryManager::base64Decode(const std::string& s) {
    // Build decode LUT: valid b64 char → 6-bit value; 0xFF = invalid
    std::array<unsigned char, 256> lut;
    lut.fill(0xFF);
    for (int i = 0; i < 64; ++i)
        lut[static_cast<unsigned char>(B64_CHARS[i])] = static_cast<unsigned char>(i);
    lut[static_cast<unsigned char>('=')] = 0;

    std::string out = {};
    out.reserve((s.size() / 4) * 3);
    for (size_t i = 0; i + 3 < s.size(); i += 4) {
        unsigned char a = lut[static_cast<unsigned char>(s[i])];
        unsigned char b = lut[static_cast<unsigned char>(s[i+1])];
        unsigned char c = lut[static_cast<unsigned char>(s[i+2])];
        unsigned char d = lut[static_cast<unsigned char>(s[i+3])];
        if (a == 0xFF || b == 0xFF) {
          break;
        }
        out += static_cast<char>((a << 2) | (b >> 4));
        if (s[i+2] != '=') {
          out += static_cast<char>((b << 4) | (c >> 2));
        }
        if (s[i+3] != '=') {
          out += static_cast<char>((c << 6) | d);
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Serialization
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::string CrashRecoveryManager::serialize(const LogEntry& e) {
    json j;
    j["t"]   = e.timestamp_ms;
    j["txn"] = e.txn_id;
    j["type"] = static_cast<int>(e.type);

    if (e.type == EntryType::BEGIN) {
        j["iso"] = static_cast<int>(e.isolation);
    } else if (e.type == EntryType::OPERATION) {
        j["op"]  = e.operation.op;
        j["key"] = base64Encode(e.operation.key);
        j["had_old"] = e.operation.had_old;
        if (e.operation.had_old)
            j["old"] = base64Encode(e.operation.old_value);
        if (!e.operation.new_value.empty())
            j["new"] = base64Encode(e.operation.new_value);
    }
    return j.dump();
}

/*static*/ std::optional<CrashRecoveryManager::LogEntry>
CrashRecoveryManager::deserialize(const std::string& line) {
    if (line.empty()) {
      return std::nullopt;
    }
    try {
        auto j = json::parse(line);
        LogEntry e;
        e.timestamp_ms = j.value("t",   int64_t{0});
        e.txn_id       = j.value("txn", uint64_t{0});
        e.type         = static_cast<EntryType>(j.value("type", 0));

        if (e.type == EntryType::BEGIN) {
            e.isolation = static_cast<IsolationLevel>(j.value("iso", 1));
        } else if (e.type == EntryType::OPERATION) {
            e.operation.op        = j.value("op", std::string{});
            e.operation.key       = base64Decode(j.value("key", std::string{}));
            e.operation.had_old   = j.value("had_old", false);
            if (e.operation.had_old && j.contains("old"))
                e.operation.old_value = base64Decode(j["old"].get<std::string>());
            if (j.contains("new"))
                e.operation.new_value = base64Decode(j["new"].get<std::string>());
        }
        return e;
    } catch (const json::exception& e) {
        THEMIS_DEBUG("JSON parse error in deserialize: {}", e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        THEMIS_WARN("Unexpected exception in deserialize: {}", e.what());
        return std::nullopt;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

CrashRecoveryManager::CrashRecoveryManager(const std::string& wal_path,
                                             bool sync_on_write)
    : wal_path_(wal_path), sync_on_write_(sync_on_write) {}

CrashRecoveryManager::~CrashRecoveryManager() = default;

// ─────────────────────────────────────────────────────────────────────────────
// WAL write methods
// ─────────────────────────────────────────────────────────────────────────────

void CrashRecoveryManager::appendLine(const std::string& json_line) {
    // mutex_ must be held by caller
    std::ofstream f(wal_path_, std::ios::app | std::ios::binary);
    if (!f.is_open()) {
        THEMIS_WARN("CrashRecoveryManager: cannot open WAL file '{}'", wal_path_);
        return;
    }
    f << json_line << '\n';
    if (sync_on_write_) {
      f.flush();
    }
}

void CrashRecoveryManager::logBegin(uint64_t txn_id, IsolationLevel isolation) {
    std::lock_guard<std::mutex> lk(mutex_);
    pending_ops_.emplace(txn_id, std::vector<OperationEntry>{});
    committed_ids_.erase(txn_id);
    aborted_ids_.erase(txn_id);

    LogEntry e;
    e.timestamp_ms = nowMs();
    e.txn_id       = txn_id;
    e.type         = EntryType::BEGIN;
    e.isolation    = isolation;
    appendLine(serialize(e));

    metric_begins_.fetch_add(1, std::memory_order_relaxed);
}

void CrashRecoveryManager::logOperation(
    uint64_t txn_id,
    const std::string& op,
    const std::string& key,
    const std::optional<std::string>& old_value,
    const std::optional<std::string>& new_value)
{
    std::lock_guard<std::mutex> lk(mutex_);

    OperationEntry oe;
    oe.op        = op;
    oe.key       = key;
    oe.had_old   = old_value.has_value();
    oe.old_value = old_value.value_or("");
    oe.new_value = new_value.value_or("");

    // Keep in-memory undo list for this txn
    auto it = pending_ops_.find(txn_id);
    if (it != pending_ops_.end()) {
        it->second.push_back(oe);
    }

    LogEntry e;
    e.timestamp_ms = nowMs();
    e.txn_id       = txn_id;
    e.type         = EntryType::OPERATION;
    e.operation    = oe;
    appendLine(serialize(e));

    metric_ops_.fetch_add(1, std::memory_order_relaxed);
}

void CrashRecoveryManager::logCommit(uint64_t txn_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    pending_ops_.erase(txn_id);
    committed_ids_.insert(txn_id);

    LogEntry e;
    e.timestamp_ms = nowMs();
    e.txn_id       = txn_id;
    e.type         = EntryType::COMMIT;
    appendLine(serialize(e));

    metric_commits_.fetch_add(1, std::memory_order_relaxed);
}

void CrashRecoveryManager::logAbort(uint64_t txn_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    pending_ops_.erase(txn_id);
    aborted_ids_.insert(txn_id);

    LogEntry e;
    e.timestamp_ms = nowMs();
    e.txn_id       = txn_id;
    e.type         = EntryType::ABORT;
    appendLine(serialize(e));

    metric_aborts_.fetch_add(1, std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// Recovery
// ─────────────────────────────────────────────────────────────────────────────

std::unordered_set<uint64_t>
CrashRecoveryManager::scanInFlight() const {
    // mutex_ must NOT be held (we take a shared view of the file)
    std::unordered_set<uint64_t> begun;
    std::unordered_set<uint64_t> finished;

    std::ifstream f(wal_path_, std::ios::binary);
    if (!f.is_open()) return {};

    std::string line = {};
    while (std::getline(f, line)) {
        auto entry = deserialize(line);
        if (!entry) {
          continue;
        }
        switch (entry->type) {
            case EntryType::BEGIN:
                begun.insert(entry->txn_id);
                break;
            case EntryType::COMMIT:
            [[fallthrough]];
            case EntryType::ABORT:
                finished.insert(entry->txn_id);
                break;
            case EntryType::CHECKPOINT:
                // Everything before a CHECKPOINT is already clean; reset.
                begun.clear();
                finished.clear();
                break;
            default:
                break;
        }
    }

    // In-flight = begun but not finished
    for (auto id : finished) {
      begun.erase(id);
    }
    return begun;
}

bool CrashRecoveryManager::needsRecovery() const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto in_flight = scanInFlight();
    last_in_flight_ids_.assign(in_flight.begin(), in_flight.end());
    return !in_flight.empty();
}

CrashRecoveryManager::RecoveryResult
CrashRecoveryManager::recover(RocksDBWrapper& db) {
    std::lock_guard<std::mutex> lk(mutex_);

    RecoveryResult result;

    // 1. Find in-flight transactions from the WAL file
    std::unordered_set<uint64_t> in_flight = scanInFlight();
    result.in_flight_found = in_flight.size();
    last_in_flight_ids_.assign(in_flight.begin(), in_flight.end());

    if (in_flight.empty()) {
        result.success = true;
        result.message = "No in-flight transactions found; WAL is clean.";
        THEMIS_INFO("CrashRecoveryManager: WAL is clean, no recovery needed.");
        return result;
    }

    THEMIS_WARN("CrashRecoveryManager: {} in-flight transaction(s) found; beginning crash recovery.",
                in_flight.size());

    // 2. Collect per-transaction operation lists from the WAL file
    std::unordered_map<uint64_t, std::vector<OperationEntry>> ops_by_txn;
    for (auto id : in_flight) ops_by_txn[id] = {};

    {
        std::ifstream f(wal_path_, std::ios::binary);
        if (!f.is_open()) {
            result.message = "Cannot open WAL file for recovery: " + wal_path_;
            THEMIS_ERROR("CrashRecoveryManager: {}", result.message);
            return result;
        }

        std::string line = {};
        std::vector<std::string> all_lines = {};

        while (std::getline(f, line)) {
          all_lines.push_back(line);
        }

        // Walk forward, but reset on CHECKPOINT (same logic as scanInFlight)
        std::unordered_map<uint64_t, std::vector<OperationEntry>> tmp_ops;
        for (const auto& ln : all_lines) {
            auto entry = deserialize(ln);
            if (!entry) {
              continue;
            }
            if (entry->type == EntryType::CHECKPOINT) {
                tmp_ops.clear();
                continue;
            }
            if (entry->type == EntryType::OPERATION &&
                in_flight.count(entry->txn_id)) {
                tmp_ops[entry->txn_id].push_back(entry->operation);
            }
        }
        ops_by_txn = std::move(tmp_ops);
    }

    // 3. Undo each in-flight transaction in reverse operation order
    for (auto txn_id : in_flight) {
        auto& ops = ops_by_txn[txn_id];
        // Reverse: undo last operation first
        for (auto rit = ops.rbegin(); rit != ops.rend(); ++rit) {
            const auto& op = *rit;
            bool ok = false;
            if (op.had_old) {
                // Restore old value
                std::vector<uint8_t> bytes(op.old_value.begin(), op.old_value.end());
                ok = db.put(op.key, bytes);
                THEMIS_DEBUG("CrashRecoveryManager: undo txn={} key='{}' → restored old value",
                             txn_id, op.key);
            } else {
                // Key did not exist before – delete it
                ok = db.del(op.key);
                THEMIS_DEBUG("CrashRecoveryManager: undo txn={} key='{}' → deleted (was new)",
                             txn_id, op.key);
            }
            if (ok) {
                ++result.operations_undone;
            } else {
                THEMIS_WARN("CrashRecoveryManager: undo op for txn={} key='{}' returned false "
                            "(key may not exist; continuing)", txn_id, op.key);
            }
        }

        result.rolled_back_ids.push_back(txn_id);
        ++result.rolled_back;

        // Log an ABORT entry for the recovered transaction so future scans
        // won't re-attempt to recover it.
        LogEntry abort_entry;
        abort_entry.timestamp_ms = nowMs();
        abort_entry.txn_id       = txn_id;
        abort_entry.type         = EntryType::ABORT;
        appendLine(serialize(abort_entry));
    }

    // 4. Append CHECKPOINT so next startup can skip these entries
    LogEntry ckpt;
    ckpt.timestamp_ms = nowMs();
    ckpt.txn_id       = 0;
    ckpt.type         = EntryType::CHECKPOINT;
    appendLine(serialize(ckpt));

    result.success = true;
    result.message = "Recovery complete: " + std::to_string(result.rolled_back) +
                     " transaction(s) rolled back, " +
                     std::to_string(result.operations_undone) + " operation(s) undone.";

    THEMIS_INFO("CrashRecoveryManager: {}", result.message);
    return result;
}

std::vector<uint64_t>
CrashRecoveryManager::getInFlightTransactionIds() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return last_in_flight_ids_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Maintenance
// ─────────────────────────────────────────────────────────────────────────────

size_t CrashRecoveryManager::pruneLog() {
    std::lock_guard<std::mutex> lk(mutex_);

    // Scan once to find in-flight ids
    std::unordered_set<uint64_t> in_flight = scanInFlight();

    std::ifstream f(wal_path_, std::ios::binary);
    if (!f.is_open()) {
      return 0;
    }

    std::vector<std::string> keep_lines;
    size_t total = 0, removed = 0;

    std::string line = {};
    while (std::getline(f, line)) {
        ++total;
        auto entry = deserialize(line);
        if (!entry) {
            // Keep malformed lines to avoid data loss
            keep_lines.push_back(line);
            continue;
        }
        // Keep entries for in-flight transactions and CHECKPOINTs
        if (entry->type == EntryType::CHECKPOINT ||
            in_flight.count(entry->txn_id)) {
            keep_lines.push_back(line);
        } else {
            ++removed;
        }
    }
    f.close();

    // Rewrite file
    std::ofstream out(wal_path_, std::ios::trunc | std::ios::binary);
    for (const auto& l : keep_lines) {
      out << l << '\n';
    }
    if (sync_on_write_) {
      out.flush();
    }

    metric_prunes_.fetch_add(1, std::memory_order_relaxed);
    THEMIS_INFO("CrashRecoveryManager::pruneLog: removed {} of {} entries", removed, total);
    return removed;
}

// ─────────────────────────────────────────────────────────────────────────────
// Metrics / Introspection
// ─────────────────────────────────────────────────────────────────────────────

CrashRecoveryManager::RecoveryMetrics
CrashRecoveryManager::getMetrics() const {
    RecoveryMetrics m;
    m.total_begins_logged    = metric_begins_.load(std::memory_order_relaxed);
    m.total_operations_logged = metric_ops_.load(std::memory_order_relaxed);
    m.total_commits_logged   = metric_commits_.load(std::memory_order_relaxed);
    m.total_aborts_logged    = metric_aborts_.load(std::memory_order_relaxed);
    m.wal_prune_count        = metric_prunes_.load(std::memory_order_relaxed);
    return m;
}

std::vector<CrashRecoveryManager::LogEntry>
CrashRecoveryManager::readAllEntries() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<LogEntry> result;

    std::ifstream f(wal_path_, std::ios::binary);
    if (!f.is_open()) {
      return result;
    }

    std::string line = {};
    while (std::getline(f, line)) {
        auto e = deserialize(line);
        if (e) {
          result.push_back(*e);
        }
    }
    return result;
}

size_t CrashRecoveryManager::pendingTransactionCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(pending_ops_.size());
}

} // namespace transaction
} // namespace themis


