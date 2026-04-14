/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            history_manager.cpp                                ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:52:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     409                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • dfa2c62531  2026-02-25  Merge branch 'develop' into copilot/implement-gpu-profili... ║
    • eb5e037bce  2026-02-25  feat(storage/transaction): harden history/conflict layer ... ║
    • 68f2e8e3f8  2026-02-24  Fix conflict_id separator to use underscore instead of dot ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/history_manager.h"
#include "storage/mvcc_store.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Hex encode/decode helpers (private to this TU)
// ─────────────────────────────────────────────────────────────────────────────

static std::string bytesToHex(const std::vector<uint8_t>& v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : v) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> out;
    if (hex.size() % 2 != 0) return out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto hi = hex[i];
        auto lo = hex[i + 1];
        auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
            return 0;
        };
        out.push_back(static_cast<uint8_t>((nibble(hi) << 4) | nibble(lo)));
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// HistoryManager
// ─────────────────────────────────────────────────────────────────────────────

HistoryManager::HistoryManager(
    std::shared_ptr<RocksDBWrapper>     db,
    std::shared_ptr<HybridLogicalClock> clock
)
    : db_(std::move(db))
    , clock_(clock ? std::move(clock) : std::make_shared<HybridLogicalClock>())
{
    if (!db_) {
        throw std::invalid_argument("HistoryManager: db cannot be null");
    }
}

// ── Key encoding ─────────────────────────────────────────────────────────────

std::string HistoryManager::historyKey(std::string_view base_key, HLCTimestamp ts) {
    // Format: "hist:" + base_key + '\x00' + 8-byte-big-endian-ts
    std::string key;
    key.reserve(5 + base_key.size() + 1 + 8);
    key += "hist:";
    key.append(base_key.data(), base_key.size());
    key.push_back('\x00');
    key.append(ts.encodeToString());
    return key;
}

std::string HistoryManager::historyPrefix(std::string_view base_key) {
    std::string prefix;
    prefix.reserve(5 + base_key.size() + 1);
    prefix += "hist:";
    prefix.append(base_key.data(), base_key.size());
    prefix.push_back('\x00');
    return prefix;
}

// ── Serialization ─────────────────────────────────────────────────────────────

std::vector<uint8_t> HistoryManager::serializeHistoryRecord(const HistoryRecord& rec) {
    nlohmann::json j;
    j["v"]        = rec.version;
    j["base_key"] = rec.base_key;
    j["ts"]       = rec.timestamp.value;
    j["op"]       = rec.op;
    j["value"]    = bytesToHex(rec.value);
    j["txn_id"]   = rec.txn_id;
    auto s = j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

std::optional<HistoryRecord> HistoryManager::deserializeHistoryRecord(std::string_view data) {
    try {
        auto j = nlohmann::json::parse(data.begin(), data.end());
        HistoryRecord rec;
        rec.version   = j.value("v", 1);
        rec.base_key  = j.value("base_key", std::string{});
        rec.timestamp = HLCTimestamp{j.value("ts", uint64_t{0})};
        rec.op        = j.value("op", std::string{"put"});
        rec.value     = hexToBytes(j.value("value", std::string{}));
        rec.txn_id    = j.value("txn_id", uint64_t{0});
        return rec;
    } catch (...) {
        return std::nullopt;
    }
}

// ── Transactional write helpers ───────────────────────────────────────────────

std::optional<HLCTimestamp> HistoryManager::recordPut(
    RocksDBWrapper::TransactionWrapper& txn,
    std::string_view base_key,
    const std::vector<uint8_t>& value,
    uint64_t txn_id
) {
    HLCTimestamp ts = clock_->now();
    HistoryRecord rec;
    rec.base_key  = std::string(base_key);
    rec.timestamp = ts;
    rec.op        = "put";
    rec.value     = value;
    rec.txn_id    = txn_id;
    auto hkey  = historyKey(base_key, ts);
    auto hval  = serializeHistoryRecord(rec);
    if (!txn.put(hkey, hval)) {
        return std::nullopt;
    }
    return ts;
}

std::optional<HLCTimestamp> HistoryManager::recordDel(
    RocksDBWrapper::TransactionWrapper& txn,
    std::string_view base_key,
    uint64_t txn_id
) {
    HLCTimestamp ts = clock_->now();
    HistoryRecord rec;
    rec.base_key  = std::string(base_key);
    rec.timestamp = ts;
    rec.op        = "del";
    rec.txn_id    = txn_id;
    auto hkey  = historyKey(base_key, ts);
    auto hval  = serializeHistoryRecord(rec);
    if (!txn.put(hkey, hval)) {
        return std::nullopt;
    }
    return ts;
}

// ── Time-travel reads ─────────────────────────────────────────────────────────

std::optional<HistoryRecord> HistoryManager::getAtTimestamp(
    std::string_view base_key,
    HLCTimestamp ts
) const {
    std::string prefix = historyPrefix(base_key);

    // Build exclusive seek target just after ts.
    std::string seek_key;
    if (ts.value == UINT64_MAX) {
        // Step past all versions of this base key.
        seek_key = "hist:";
        seek_key.append(base_key.data(), base_key.size());
        seek_key.push_back('\x01');
    } else {
        seek_key = historyKey(base_key, HLCTimestamp{ts.value + 1});
    }

    auto iter_result = db_->newSafeIterator();
    if (!iter_result.has_value()) return std::nullopt;
    auto& it = iter_result.value();
    if (!it) return std::nullopt;

    it.Seek(seek_key);

    if (!it.Valid()) {
        it.SeekToLast();
    } else {
        it.Prev();
    }

    if (!it.Valid()) return std::nullopt;

    std::string_view found_key = it.key();
    if (found_key.size() < prefix.size() ||
        found_key.substr(0, prefix.size()) != std::string_view(prefix)) {
        return std::nullopt;
    }

    // Verify the timestamp is within the requested range.
    HLCTimestamp found_ts = MVCCStore::decodeTimestamp(found_key);
    if (found_ts > ts) return std::nullopt;

    return deserializeHistoryRecord(it.value());
}

std::vector<HistoryRecord> HistoryManager::listVersions(std::string_view base_key) const {
    std::string prefix = historyPrefix(base_key);
    std::vector<HistoryRecord> result;
    db_->scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view val) -> bool {
        auto rec = deserializeHistoryRecord(val);
        if (rec) result.push_back(std::move(*rec));
        return true;
    });
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// ConflictManager
// ─────────────────────────────────────────────────────────────────────────────

ConflictManager::ConflictManager(
    std::shared_ptr<RocksDBWrapper>     db,
    std::shared_ptr<HybridLogicalClock> clock
)
    : db_(std::move(db))
    , clock_(clock ? std::move(clock) : std::make_shared<HybridLogicalClock>())
{
    if (!db_) {
        throw std::invalid_argument("ConflictManager: db cannot be null");
    }
}

// ── Key encoding ──────────────────────────────────────────────────────────────

std::string ConflictManager::conflictKey(std::string_view conflict_id) {
    std::string key;
    key.reserve(9 + conflict_id.size());
    key += "conflict:";
    key.append(conflict_id.data(), conflict_id.size());
    return key;
}

std::string ConflictManager::conflictSetKey(std::string_view conflict_set_id) {
    std::string key;
    key.reserve(12 + conflict_set_id.size());
    key += "conflictset:";
    key.append(conflict_set_id.data(), conflict_set_id.size());
    return key;
}

// ── Serialization ─────────────────────────────────────────────────────────────

std::vector<uint8_t> ConflictManager::serializeConflictRecord(const ConflictRecord& rec) {
    nlohmann::json j;
    j["v"]           = rec.version;
    j["conflict_id"] = rec.conflict_id;
    j["base_key"]    = rec.base_key;
    j["detected_at"] = rec.detected_at.value;
    j["txn_id"]      = rec.txn_id;
    j["base_hex"]    = bytesToHex(rec.base_value);
    j["ours_hex"]    = bytesToHex(rec.ours_value);
    j["theirs_hex"]  = bytesToHex(rec.theirs_value);
    j["type"]        = rec.type;
    auto s = j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

std::optional<ConflictRecord> ConflictManager::deserializeConflictRecord(std::string_view data) {
    try {
        auto j = nlohmann::json::parse(data.begin(), data.end());
        ConflictRecord rec;
        rec.version      = j.value("v", 1);
        rec.conflict_id  = j.value("conflict_id", std::string{});
        rec.base_key     = j.value("base_key", std::string{});
        rec.detected_at  = HLCTimestamp{j.value("detected_at", uint64_t{0})};
        rec.txn_id       = j.value("txn_id", uint64_t{0});
        rec.base_value   = hexToBytes(j.value("base_hex", std::string{}));
        rec.ours_value   = hexToBytes(j.value("ours_hex", std::string{}));
        rec.theirs_value = hexToBytes(j.value("theirs_hex", std::string{}));
        rec.type         = j.value("type", std::string{});
        return rec;
    } catch (...) {
        return std::nullopt;
    }
}

// ── Write ─────────────────────────────────────────────────────────────────────

std::string ConflictManager::storeConflict(ConflictRecord& record) {
    if (record.conflict_id.empty()) {
        // Generate a unique ID from the HLC clock.
        HLCTimestamp ts = clock_->now();
        record.conflict_id = std::to_string(ts.physical()) + "_" +
                             std::to_string(ts.logical());
        record.detected_at = ts;
    }
    auto ckey = conflictKey(record.conflict_id);
    auto cval = serializeConflictRecord(record);
    db_->put(ckey, cval);
    return record.conflict_id;
}

// ── Read ──────────────────────────────────────────────────────────────────────

std::optional<ConflictRecord> ConflictManager::getConflict(std::string_view conflict_id) const {
    auto ckey = conflictKey(conflict_id);
    auto raw = db_->get(ckey);
    if (!raw) return std::nullopt;
    return deserializeConflictRecord(
        std::string_view(reinterpret_cast<const char*>(raw->data()), raw->size())
    );
}

std::vector<ConflictRecord> ConflictManager::listConflicts() const {
    std::vector<ConflictRecord> result;
    db_->scanPrefix("conflict:", [&](std::string_view /*key*/, std::string_view val) -> bool {
        auto rec = deserializeConflictRecord(val);
        if (rec) result.push_back(std::move(*rec));
        return true;
    });
    return result;
}

// ── ConflictSet serialization ─────────────────────────────────────────────────

std::vector<uint8_t> ConflictManager::serializeConflictSet(const ConflictSet& set) {
    nlohmann::json j;
    j["v"]                   = set.version;
    j["conflict_set_id"]     = set.conflict_set_id;
    j["detected_at"]         = set.detected_at.value;
    j["txn_id"]              = set.txn_id;
    j["conflict_record_ids"] = set.conflict_record_ids;
    j["affected_keys"]       = set.affected_keys;
    auto s = j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

std::optional<ConflictSet> ConflictManager::deserializeConflictSet(std::string_view data) {
    try {
        auto j = nlohmann::json::parse(data.begin(), data.end());
        ConflictSet set;
        set.version           = j.value("v", 1);
        set.conflict_set_id   = j.value("conflict_set_id", std::string{});
        set.detected_at       = HLCTimestamp{j.value("detected_at", uint64_t{0})};
        set.txn_id            = j.value("txn_id", uint64_t{0});
        set.conflict_record_ids = j.value("conflict_record_ids", std::vector<std::string>{});
        set.affected_keys     = j.value("affected_keys", std::vector<std::string>{});
        return set;
    } catch (...) {
        return std::nullopt;
    }
}

// ── ConflictSet write/read ────────────────────────────────────────────────────

std::string ConflictManager::storeConflictSet(ConflictSet& set) {
    if (set.conflict_set_id.empty()) {
        HLCTimestamp ts = clock_->now();
        set.conflict_set_id = std::to_string(ts.physical()) + "_" +
                              std::to_string(ts.logical());
        set.detected_at = ts;
    }
    auto ckey = conflictSetKey(set.conflict_set_id);
    auto cval = serializeConflictSet(set);
    db_->put(ckey, cval);
    return set.conflict_set_id;
}

std::optional<ConflictSet> ConflictManager::getConflictSet(std::string_view conflict_set_id) const {
    auto ckey = conflictSetKey(conflict_set_id);
    auto raw = db_->get(ckey);
    if (!raw) return std::nullopt;
    return deserializeConflictSet(
        std::string_view(reinterpret_cast<const char*>(raw->data()), raw->size())
    );
}

std::vector<ConflictSet> ConflictManager::listConflictSets() const {
    std::vector<ConflictSet> result;
    db_->scanPrefix("conflictset:", [&](std::string_view /*key*/, std::string_view val) -> bool {
        auto set = deserializeConflictSet(val);
        if (set) result.push_back(std::move(*set));
        return true;
    });
    return result;
}

} // namespace themis
