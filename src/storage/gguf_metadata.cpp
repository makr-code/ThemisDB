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
 * Default sign/verify operations use OpenSSL HMAC-SHA256. A custom
 * implementation can be injected via GGUFMetadata::setHmacFn().
 */

#include "storage/gguf_metadata.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
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
// Internal helpers
// ============================================================================

namespace {

[[nodiscard]] std::string toHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return oss.str();
}

[[nodiscard]] std::string computeHmacSha256(const std::string& data,
                                            const std::string& key) {
    if (key.size() > static_cast<size_t>(INT_MAX) ||
        data.size() > static_cast<size_t>(INT_MAX)) {
        std::fprintf(stderr,
            "[ThemisDB][SECURITY] GGUFMetadata: HMAC input exceeds INT_MAX; "
            "operation failed.\n");
        return {};
    }

    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    if (!HMAC(EVP_sha256(),
              reinterpret_cast<const unsigned char*>(key.data()),
              static_cast<int>(key.size()),
              reinterpret_cast<const unsigned char*>(data.data()),
              static_cast<int>(data.size()),
              md,
              &md_len)) {
        return {};
    }

    return toHex(md, md_len);
}

[[nodiscard]] bool constantTimeEquals(const std::string& lhs,
                                      const std::string& rhs) {
    constexpr std::size_t kHexSha256Len = 64;
    std::array<unsigned char, kHexSha256Len> lhs_buf{};
    std::array<unsigned char, kHexSha256Len> rhs_buf{};

    const auto lhs_copy = std::min(lhs.size(), kHexSha256Len);
    const auto rhs_copy = std::min(rhs.size(), kHexSha256Len);
    std::memcpy(lhs_buf.data(), lhs.data(), lhs_copy);
    std::memcpy(rhs_buf.data(), rhs.data(), rhs_copy);

    const int cmp = CRYPTO_memcmp(lhs_buf.data(), rhs_buf.data(), kHexSha256Len);
    const unsigned char cmp_ok =
        static_cast<unsigned char>(cmp == 0 ? 1 : 0);
    const unsigned char lhs_ok =
        static_cast<unsigned char>(lhs.size() == kHexSha256Len ? 1 : 0);
    const unsigned char rhs_ok =
        static_cast<unsigned char>(rhs.size() == kHexSha256Len ? 1 : 0);
    return static_cast<unsigned char>(cmp_ok & lhs_ok & rhs_ok) == 1;
}

[[nodiscard]] bool isValidHexSha256Signature(const std::string& signature) {
    constexpr std::size_t kHexSha256Len = 64;
    if (signature.size() != kHexSha256Len) {
        return false;
    }
    for (unsigned char ch : signature) {
        if (!std::isxdigit(ch)) {
            return false;
        }
    }
    return true;
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
// GGUFMetadata — HmacFn injection bridge
// ============================================================================

static std::mutex& hmacFnMutex() { static std::mutex m; return m; }
static GGUFMetadata::HmacFn& hmacFnStorage() {
    static GGUFMetadata::HmacFn fn;
    return fn;
}

/*static*/
void GGUFMetadata::setHmacFn(HmacFn fn) {
    std::lock_guard<std::mutex> lk(hmacFnMutex());
    hmacFnStorage() = std::move(fn);
}

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
// GGUFMetadata — sign / verify
// ============================================================================

void GGUFMetadata::sign(ProvenanceRecord& record,
                         const std::string& hmac_key) {
    const std::string canonical = record.canonicalBytes();

    // Try injected HMAC fn first.
    {
        std::lock_guard<std::mutex> lk(hmacFnMutex());
        const auto& fn = hmacFnStorage();
        if (fn) {
            try {
                const std::string injected = fn(canonical, hmac_key);
                if (isValidHexSha256Signature(injected)) {
                    record.hmac_signature = injected;
                    return;
                }
                std::fprintf(stderr,
                    "[ThemisDB][SECURITY] GGUFMetadata::sign: injected HmacFn returned "
                    "an invalid signature format; clearing signature.\n");
                record.hmac_signature.clear();
                return;
            } catch (...) {
                std::fprintf(stderr,
                    "[ThemisDB][SECURITY] GGUFMetadata::sign: injected HmacFn threw; "
                    "clearing signature.\n");
                record.hmac_signature.clear();
                return;
            }
        }
    }

    record.hmac_signature = computeHmacSha256(canonical, hmac_key);
    if (record.hmac_signature.empty()) {
        std::fprintf(stderr,
            "[ThemisDB][SECURITY] GGUFMetadata::sign: built-in HMAC-SHA256 "
            "failed; clearing signature.\n");
    }
}

bool GGUFMetadata::verify(const ProvenanceRecord& record,
                            const std::string& hmac_key) {
    if (record.hmac_signature.empty()) return false;
    const std::string canonical = record.canonicalBytes();

    // Try injected HMAC fn first.
    {
        std::lock_guard<std::mutex> lk(hmacFnMutex());
        const auto& fn = hmacFnStorage();
        if (fn) {
            try {
                const std::string expected = fn(canonical, hmac_key);
                if (!isValidHexSha256Signature(expected)) {
                    std::fprintf(stderr,
                        "[ThemisDB][SECURITY] GGUFMetadata::verify: injected HmacFn "
                        "returned invalid signature format.\n");
                    return false;
                }
                return constantTimeEquals(record.hmac_signature, expected);
            } catch (...) {
                std::fprintf(stderr,
                    "[ThemisDB][SECURITY] GGUFMetadata::verify: injected HmacFn threw; "
                    "returning false (fail-closed). Operator should diagnose why the "
                    "HMAC function is failing.\n");
                return false;  // fail-closed on exception
            }
        }
    }

    const std::string expected = computeHmacSha256(canonical, hmac_key);
    if (expected.empty()) {
        return false;
    }
    return constantTimeEquals(record.hmac_signature, expected);
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
