/**
 * @file history_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/history_manager.h"
#include "storage/mvcc_store.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstring>
#include "utils/logger.h"

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Integrity helpers: CRC32 (table-based, no external dependency)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Compute CRC32 over [data, data+len).  Uses the standard IEEE polynomial.
static uint32_t history_crc32(const void* data, size_t len) {
    static const auto table = []() {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
              c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    uint32_t crc = 0xFFFFFFFFu;
    const auto* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
      crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

// Append a 4-byte little-endian CRC32 of the payload to buf.
static void append_crc32(std::vector<uint8_t>& buf) {
    buf.reserve(static_cast<int>(buf.size()) + 4);
    uint32_t crc = history_crc32(buf.data(), buf.size());
    for (int i = 0; i < 4; ++i) {
      buf.push_back(static_cast<uint8_t>(crc >> (8 * i)));
    }
}

// Verify a 4-byte trailing CRC32 appended by append_crc32().
// Returns the payload range [data, data+(size-4)) on success, or nullopt on
// checksum mismatch.  Falls back to accepting the full data as-is if it is
// not in the new framing format (legacy path: no CRC trailer).
static std::optional<std::string_view> verify_crc32(std::string_view data) {
    constexpr size_t kCrcSize = 4;
    if (data.size() < kCrcSize) {
        // Too short for CRC trailer — treat as legacy (no checksum).
        return data;
    }
    const size_t payload_size = static_cast<int>(data.size()) - kCrcSize;
    const uint8_t* crc_bytes  = reinterpret_cast<const uint8_t*>(data.data()) + payload_size;
    uint32_t stored_crc = 0;
    for (int i = 0; i < 4; ++i) {
      stored_crc |= (static_cast<uint32_t>(crc_bytes[i]) << (8 * i));
    }
    uint32_t computed = history_crc32(data.data(), payload_size);
    if (computed != stored_crc) {
        // CRC mismatch — either corrupted data or a legacy record without
        // the trailer.  Return the full span so JSON parsing can try.
        return data;
    }
    return std::string_view(data.data(), payload_size);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Hex encode/decode helpers (private to this TU)
// ─────────────────────────────────────────────────────────────────────────────

static std::string bytesToHex(const std::vector<uint8_t>& v) {
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (auto b : v) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> out = {};

    if (hex.size() % 2 != 0) {
      return out;
    }
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto hi = hex[i];
        auto lo = hex[i + 1];
        auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') {
              return static_cast<uint8_t>(c - '0');
            }
            if (c >= 'a' && c <= 'f') {
              return static_cast<uint8_t>(c - 'a' + 10);
            }
            if (c >= 'A' && c <= 'F') {
              return static_cast<uint8_t>(c - 'A' + 10);
            }
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
    std::string key = {};
    key.reserve(5 + static_cast<int>(base_key.size()) + 1 + 8);
    key += "hist:";
    key.append(base_key.data(), base_key.size());
    key.push_back('\x00');
    key.append(ts.encodeToString());
    return key;
}

std::string HistoryManager::historyPrefix(std::string_view base_key) {
    std::string prefix = {};
    prefix.reserve(5 + static_cast<int>(base_key.size()) + 1);
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
    std::vector<uint8_t> buf(s.begin(), s.end());
    append_crc32(buf);
    return buf;
}

std::optional<HistoryRecord> HistoryManager::deserializeHistoryRecord(std::string_view data) {
    try {
        auto payload = verify_crc32(data);
        if (!payload) {
          return std::nullopt;
        }
        auto j = nlohmann::json::parse(payload->begin(), payload->end());
        HistoryRecord rec;
        rec.version   = j.value("v", 1);
        rec.base_key  = j.value("base_key", std::string{});
        rec.timestamp = HLCTimestamp(j.value("ts", uint64_t{0}));
        rec.op        = j.value("op", std::string{"put"});
        rec.value     = hexToBytes(j.value("value", std::string{}));
        rec.txn_id    = j.value("txn_id", uint64_t{0});
        return rec;
    } catch (...) {
        THEMIS_WARN("history_manager: unhandled exception caught");
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
    std::string seek_key = {};
    if (ts.value == UINT64_MAX) {
        // Step past all versions of this base key.
        seek_key = "hist:";
        seek_key.append(base_key.data(), base_key.size());
        seek_key.push_back('\x01');
    } else {
        seek_key = historyKey(base_key, HLCTimestamp(ts.value + 1));
    }

    auto iter_result = db_->newSafeIterator();
    if (!iter_result.has_value()) {
      return std::nullopt;
    }
    auto& it = iter_result.value();
    if (!it) {
      return std::nullopt;
    }

    it.Seek(seek_key);

    if (!it.Valid()) {
        it.SeekToLast();
    } else {
        it.Prev();
    }

    if (!it.Valid()) {
      return std::nullopt;
    }

    std::string_view found_key = it.key();
    if (found_key.size() < prefix.size() ||
        found_key.substr(0, prefix.size()) != std::string_view(prefix)) {
        return std::nullopt;
    }

    // Verify the timestamp is within the requested range.
    HLCTimestamp found_ts = MVCCStore::decodeTimestamp(found_key);
    if (found_ts > ts) {
      return std::nullopt;
    }

    return deserializeHistoryRecord(it.value());
}

std::vector<HistoryRecord> HistoryManager::listVersions(std::string_view base_key) const {
    std::string prefix = historyPrefix(base_key);
    std::vector<HistoryRecord> result;
    db_->scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view val) -> bool {
        auto rec = deserializeHistoryRecord(val);
        if (rec) {
          result.push_back(std::move(*rec));
        }
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
    std::string key = {};
    key.reserve(9 + conflict_id.size());
    key += "conflict:";
    key.append(conflict_id.data(), conflict_id.size());
    return key;
}

std::string ConflictManager::conflictSetKey(std::string_view conflict_set_id) {
    std::string key = {};
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
    std::vector<uint8_t> buf(s.begin(), s.end());
    append_crc32(buf);
    return buf;
}

std::optional<ConflictRecord> ConflictManager::deserializeConflictRecord(std::string_view data) {
    try {
        auto payload = verify_crc32(data);
        if (!payload) {
          return std::nullopt;
        }
        auto j = nlohmann::json::parse(payload->begin(), payload->end());
        ConflictRecord rec;
        rec.version      = j.value("v", 1);
        rec.conflict_id  = j.value("conflict_id", std::string{});
        rec.base_key     = j.value("base_key", std::string{});
        rec.detected_at  = HLCTimestamp(j.value("detected_at", uint64_t{0}));
        rec.txn_id       = j.value("txn_id", uint64_t{0});
        rec.base_value   = hexToBytes(j.value("base_hex", std::string{}));
        rec.ours_value   = hexToBytes(j.value("ours_hex", std::string{}));
        rec.theirs_value = hexToBytes(j.value("theirs_hex", std::string{}));
        rec.type         = j.value("type", std::string{});
        return rec;
    } catch (...) {
        THEMIS_WARN("history_manager: unhandled exception caught");
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
    if (!raw) {
      return std::nullopt;
    }
    return deserializeConflictRecord(
        std::string_view(reinterpret_cast<const char*>(raw->data()), raw->size())
    );
}

std::vector<ConflictRecord> ConflictManager::listConflicts() const {
    std::vector<ConflictRecord> result;
    db_->scanPrefix("conflict:", [&](std::string_view /*key*/, std::string_view val) -> bool {
        auto rec = deserializeConflictRecord(val);
        if (rec) {
          result.push_back(std::move(*rec));
        }
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
    std::vector<uint8_t> buf(s.begin(), s.end());
    append_crc32(buf);
    return buf;
}

std::optional<ConflictSet> ConflictManager::deserializeConflictSet(std::string_view data) {
    try {
        auto payload = verify_crc32(data);
        if (!payload) {
          return std::nullopt;
        }
        auto j = nlohmann::json::parse(payload->begin(), payload->end());
        ConflictSet set;
        set.version           = j.value("v", 1);
        set.conflict_set_id   = j.value("conflict_set_id", std::string{});
        set.detected_at       = HLCTimestamp(j.value("detected_at", uint64_t{0}));
        set.txn_id            = j.value("txn_id", uint64_t{0});
        set.conflict_record_ids = j.value("conflict_record_ids", std::vector<std::string>{});
        set.affected_keys     = j.value("affected_keys", std::vector<std::string>{});
        return set;
    } catch (...) {
        THEMIS_WARN("history_manager: unhandled exception caught");
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
    if (!raw) {
      return std::nullopt;
    }
    return deserializeConflictSet(
        std::string_view(reinterpret_cast<const char*>(raw->data()), raw->size())
    );
}

std::vector<ConflictSet> ConflictManager::listConflictSets() const {
    std::vector<ConflictSet> result;
    db_->scanPrefix("conflictset:", [&](std::string_view /*key*/, std::string_view val) -> bool {
        auto set = deserializeConflictSet(val);
        if (set) {
          result.push_back(std::move(*set));
        }
        return true;
    });
    return result;
}

} // namespace themis

