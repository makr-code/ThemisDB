/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            storage/gguf_metadata.cpp                          ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q1 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file storage/gguf_metadata.cpp
 * @brief GGUFMetadata — GGUF v3 provenance store implementation.
 *
 * ### Stub log
 * - GMD-01  sign() / verify() use byte-XOR placeholder instead of
 *           HMAC-SHA256.  Tracked as STUB #173.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Allow GgmlTensorBridge and AdapterRepository to call
 *          sign() / verify() in unit tests without linking OpenSSL.
 * Activation: Always (no compile flag required).
 * Production Delta: XOR-based tag is NOT cryptographically secure.
 *                   An attacker who knows the key and any signed record
 *                   can forge signatures.
 * Removal Plan: Q2 2027 — replace stubSign() with
 *               HMAC_CTX_new() / HMAC_Update() / HMAC_Final() from
 *               OpenSSL, or a vendored SHA-256 (e.g. mbedTLS).
 */

#include "storage/gguf_metadata.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace storage {

// ============================================================================
// ProvenanceRecord helpers
// ============================================================================

std::string ProvenanceRecord::canonicalBytes() const {
    // Concatenate fields with null-byte separators for an unambiguous encoding.
    std::string out;
    out.reserve(source_filename.size() + source_doc_id.size()
                + tenant_id.size() + ingest_timestamp.size() + 32);
    out += source_filename;  out += '\0';
    out += std::to_string(source_page); out += '\0';
    out += std::to_string(source_line); out += '\0';
    out += source_doc_id;   out += '\0';
    out += tenant_id;       out += '\0';
    out += ingest_timestamp;
    return out;
}

// ============================================================================
// Internal helper — byte-XOR "HMAC" (STUB #173)
// ============================================================================

namespace {

/// XOR-based tag: tag[i] = canonical[i % len] XOR key[i % keylen].
/// NOT cryptographically secure — placeholder for real HMAC-SHA256.
std::string stubSign(const std::string& data, const std::string& key) {
    if (key.empty()) {
        return std::string(8, '\x00');
    }
    // Produce an 8-byte tag (64-bit) by folding XOR over the data.
    uint8_t tag[8] = {0};
    for (std::size_t i = 0; i < data.size(); ++i) {
        tag[i % 8] ^= static_cast<uint8_t>(data[i])
                    ^ static_cast<uint8_t>(key[i % key.size()]);
    }
    // Also mix in key bytes on their own pass.
    for (std::size_t i = 0; i < key.size(); ++i) {
        tag[(i + 1) % 8] ^= static_cast<uint8_t>(key[i]);
    }
    // Hex-encode.
    std::ostringstream oss;
    for (uint8_t b : tag) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast&lt;int&gt;(b);
    }
    return oss.str();
}

// ─── Serialisation helpers ────────────────────────────────────────────────

void writeU32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v >>  0));
    buf.push_back(static_cast<uint8_t>(v >>  8));
    buf.push_back(static_cast<uint8_t>(v >> 16));
    buf.push_back(static_cast<uint8_t>(v >> 24));
}

void writeI32(std::vector<uint8_t>& buf, int32_t v) {
    writeU32(buf, static_cast<uint32_t>(v));
}

void writeStr(std::vector<uint8_t>& buf, const std::string& s) {
    writeU32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

bool readU32(const uint8_t* data, std::size_t size, std::size_t& pos,
             uint32_t& out) {
    if (pos + 4 > size) return false;
    out = static_cast<uint32_t>(data[pos])
        | static_cast<uint32_t>(data[pos+1]) <<  8
        | static_cast<uint32_t>(data[pos+2]) << 16
        | static_cast<uint32_t>(data[pos+3]) << 24;
    pos += 4;
    return true;
}

bool readI32(const uint8_t* data, std::size_t size, std::size_t& pos,
             int32_t& out) {
    uint32_t u = 0;
    if (!readU32(data, size, pos, u)) return false;
    out = static_cast<int32_t>(u);
    return true;
}

bool readStr(const uint8_t* data, std::size_t size, std::size_t& pos,
             std::string& out) {
    uint32_t len = 0;
    if (!readU32(data, size, pos, len)) return false;
    if (pos + len > size) return false;
    out.assign(reinterpret_cast<const char*>(data + pos), len);
    pos += len;
    return true;
}

} // anonymous namespace

// ============================================================================
// GGUFMetadata — attach / detach
// ============================================================================

void GGUFMetadata::attach(const std::string& storage_key,
                           const ProvenanceRecord& record) {
    std::unique_lock lock(mutex_);
    store_[storage_key] = record;
}

bool GGUFMetadata::detach(const std::string& storage_key) {
    std::unique_lock lock(mutex_);
    return store_.erase(storage_key) > 0;
}

// ============================================================================
// GGUFMetadata — retrieve / has / keys
// ============================================================================

std::optional<ProvenanceRecord>
GGUFMetadata::retrieve(const std::string& storage_key) const {
    std::shared_lock lock(mutex_);
    auto it = store_.find(storage_key);
    if (it == store_.end()) return std::nullopt;
    return it->second;
}

bool GGUFMetadata::has(const std::string& storage_key) const {
    std::shared_lock lock(mutex_);
    return store_.count(storage_key) > 0;
}

std::vector<std::string> GGUFMetadata::keys() const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> result;
    result.reserve(store_.size());
    for (const auto& [k, _] : store_) {
        result.push_back(k);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::size_t GGUFMetadata::size() const noexcept {
    std::shared_lock lock(mutex_);
    return store_.size();
}

// ============================================================================
// GGUFMetadata — sign / verify (STUB #173)
// ============================================================================

void GGUFMetadata::sign(ProvenanceRecord& record,
                         const std::string& hmac_key) {
    // STUB/SIMULATION NOTE (stub #173): byte-XOR placeholder for HMAC-SHA256.
    record.hmac_signature = stubSign(record.canonicalBytes(), hmac_key);
}

bool GGUFMetadata::verify(const ProvenanceRecord& record,
                           const std::string& hmac_key) {
    if (record.hmac_signature.empty()) return false;
    const std::string expected = stubSign(record.canonicalBytes(), hmac_key);
    return record.hmac_signature == expected;
}

// ============================================================================
// GGUFMetadata — serialize / deserialize
// ============================================================================

std::vector<uint8_t> GGUFMetadata::serialize() const {
    std::shared_lock lock(mutex_);

    std::vector<uint8_t> buf;
    // Number of records.
    writeU32(buf, static_cast<uint32_t>(store_.size()));

    for (const auto& [key, rec] : store_) {
        writeStr(buf, key);
        writeStr(buf, rec.source_filename);
        writeI32(buf, rec.source_page);
        writeI32(buf, rec.source_line);
        writeStr(buf, rec.source_doc_id);
        writeStr(buf, rec.tenant_id);
        writeStr(buf, rec.ingest_timestamp);
        writeStr(buf, rec.hmac_signature);
    }
    return buf;
}

bool GGUFMetadata::deserialize(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) return false;

    const uint8_t* data = bytes.data();
    const std::size_t size = bytes.size();
    std::size_t pos = 0;

    uint32_t count = 0;
    if (!readU32(data, size, pos, count)) return false;

    std::unordered_map<std::string, ProvenanceRecord> tmp;
    tmp.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        std::string key;
        if (!readStr(data, size, pos, key)) return false;

        ProvenanceRecord rec;
        if (!readStr(data, size, pos, rec.source_filename)) return false;
        if (!readI32(data, size, pos, rec.source_page))     return false;
        if (!readI32(data, size, pos, rec.source_line))     return false;
        if (!readStr(data, size, pos, rec.source_doc_id))   return false;
        if (!readStr(data, size, pos, rec.tenant_id))       return false;
        if (!readStr(data, size, pos, rec.ingest_timestamp))return false;
        if (!readStr(data, size, pos, rec.hmac_signature))  return false;

        tmp[std::move(key)] = std::move(rec);
    }

    std::unique_lock lock(mutex_);
    store_ = std::move(tmp);
    return true;
}

} // namespace storage
} // namespace themis
