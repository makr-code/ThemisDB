/**
 * @file signed_plugin_repository.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/signed_plugin_repository.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Base64 decode helper (no dependency on external libs)
// Uses the OpenSSL BIO API that is already a project dependency.
#include <openssl/bio.h>
#include <openssl/buffer.h>

namespace {

// RAII wrapper for OpenSSL BIO
struct BioDeleter {
    void operator()(BIO* bio) const noexcept {
        if (bio) {
            BIO_free_all(bio);
        }
    }
};

using UniqueBio = std::unique_ptr<BIO, BioDeleter>;

// RAII wrapper for OpenSSL EVP_MD_CTX
struct EvpMdCtxDeleter {
    void operator()(EVP_MD_CTX* ctx) const noexcept {
        if (ctx) {
            EVP_MD_CTX_free(ctx);
        }
    }
};

using UniqueEvpMdCtx = std::unique_ptr<EVP_MD_CTX, EvpMdCtxDeleter>;

// RAII wrapper for OpenSSL EVP_PKEY
struct EvpPkeyDeleter {
    void operator()(EVP_PKEY* pkey) const noexcept {
        if (pkey) {
            EVP_PKEY_free(pkey);
        }
    }
};

using UniqueEvpPkey = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;

// Decode a standard Base64 string into raw bytes.
// Returns empty vector on malformed input.
std::vector<uint8_t> base64Decode(const std::string& encoded) {
    if (encoded.empty()) {
        return {};
    }
    // OpenSSL BIO chain: base64 filter -> memory source
    // Using RAII wrapper to ensure cleanup even if exception occurs
    BIO* b64 = BIO_new(BIO_f_base64());
    if (!b64) {
        return {};
    }
    
    BIO* mem = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    if (!mem) {
        BIO_free(b64);
        return {};
    }
    
    // Wrap both BIOs in RAII wrapper; BIO_push chains them
    UniqueBio bio_guard(BIO_push(b64, mem));
    BIO_set_flags(bio_guard.get(), BIO_FLAGS_BASE64_NO_NL);

    // Upper bound for decoded output
    std::vector<uint8_t> buf(encoded.size());
    int decoded_len = BIO_read(bio_guard.get(), buf.data(), static_cast<int>(buf.size()));
    // bio_guard automatically frees both BIOs when it goes out of scope

    if (decoded_len <= 0) {
        return {};
    }
    buf.resize(static_cast<size_t>(decoded_len));
    return buf;
}

// Convert raw bytes to lowercase hex string.
std::string bytesToHex(const uint8_t* data, size_t len) {
    std::ostringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return ss.str();
}

// Map PluginType enum to its canonical string representation.
std::string pluginTypeToString(themis::plugins::PluginType t) {
    using PT = themis::plugins::PluginType;
    switch (t) {
        case PT::COMPUTE_BACKEND: return "compute_backend";
        case PT::BLOB_STORAGE:    return "blob_storage";
        case PT::IMPORTER:        return "importer";
        case PT::EXPORTER:        return "exporter";
        case PT::HSM_PROVIDER:    return "hsm_provider";
        case PT::EMBEDDING:       return "embedding";
        case PT::LLM_BACKEND:     return "llm_backend";
        default:                  return "custom";
    }
}

} // anonymous namespace

namespace themis {
namespace plugins {

// =============================================================================
// Key management
// =============================================================================

void SignedPluginRepository::addPinnedKey(PinnedKey key) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Replace existing key with the same fingerprint if present
    for (auto& k : pinned_keys_) {
        if (k.fingerprint == key.fingerprint) {
            k = std::move(key);
            return;
        }
    }
    pinned_keys_.push_back(std::move(key));
}

bool SignedPluginRepository::removePinnedKey(const std::string& fingerprint) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(pinned_keys_.begin(), pinned_keys_.end(),
                           [&fingerprint](const PinnedKey& k) {
                               return k.fingerprint == fingerprint;
                           });
    if (it == pinned_keys_.end()) {
        return false;
    }
    pinned_keys_.erase(it);
    return true;
}

bool SignedPluginRepository::hasPinnedKey(const std::string& fingerprint) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const PinnedKey* k = findPinnedKeyLocked(fingerprint);
    return k != nullptr && k->active;
}

std::vector<PinnedKey> SignedPluginRepository::getPinnedKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pinned_keys_;
}

// =============================================================================
// Catalog management
// =============================================================================

bool SignedPluginRepository::addEntry(const RepositoryEntry& entry) {
    // Hold the lock for the entire verify+insert sequence to prevent a TOCTOU
    // race where a pinned key is deactivated between verification and insertion.
    std::lock_guard<std::mutex> lock(mutex_);
    if (!verifyEntryLocked(entry)) {
        return false;
    }
    // Replace existing entry with the same (name, version).
    for (auto& e : entries_) {
        if (e.manifest.name == entry.manifest.name &&
            e.manifest.version == entry.manifest.version) {
            e = entry;
            return true;
        }
    }
    entries_.push_back(entry);
    return true;
}

bool SignedPluginRepository::verifyEntry(const RepositoryEntry& entry) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return verifyEntryLocked(entry);
}

std::optional<RepositoryEntry> SignedPluginRepository::findEntry(
    const std::string& name,
    const std::string& version) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!version.empty()) {
        // Exact match
        for (const auto& e : entries_) {
            if (e.manifest.name == name && e.manifest.version == version) {
                return e;
            }
        }
        return std::nullopt;
    }
    // Return entry with lexicographically greatest version for the given name
    const RepositoryEntry* best = nullptr;
    for (const auto& e : entries_) {
        if (e.manifest.name != name) {
            continue;
        }
        if (!best || e.manifest.version > best->manifest.version) {
            best = &e;
        }
    }
    if (best) {
        return *best;
    }
    return std::nullopt;
}

std::vector<RepositoryEntry> SignedPluginRepository::findByName(
    const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RepositoryEntry> result;
    for (const auto& e : entries_) {
        if (e.manifest.name == name) {
            result.push_back(e);
        }
    }
    return result;
}

std::vector<RepositoryEntry> SignedPluginRepository::listEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

void SignedPluginRepository::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

// =============================================================================
// Utilities
// =============================================================================

std::string SignedPluginRepository::computeKeyFingerprint(
    const std::vector<uint8_t>& public_key)
{
    if (public_key.empty()) {
        return {};
    }
    uint8_t digest[SHA256_DIGEST_LENGTH];
    if (SHA256(public_key.data(), public_key.size(), digest) == nullptr) {
        return {};
    }
    return bytesToHex(digest, SHA256_DIGEST_LENGTH);
}

std::string SignedPluginRepository::canonicalManifestJson(
    const MarketplaceManifest& m)
{
    // Emit fields in strict alphabetical key order, no whitespace, to produce a
    // deterministic byte sequence regardless of JSON library or serialiser.
    // Only the subset of fields that form the plugin identity and binary contract
    // are covered; auxiliary / display fields are excluded.
    auto escape = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size() + 2);
        out += '"';
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;
            }
        }
        out += '"';
        return out;
    };

    std::ostringstream ss;
    ss << "{"
       << "\"author\":"              << escape(m.author)              << ","
       << "\"binary_linux\":"        << escape(m.binary_linux)        << ","
       << "\"binary_macos\":"        << escape(m.binary_macos)        << ","
       << "\"binary_windows\":"      << escape(m.binary_windows)      << ","
       << "\"description\":"         << escape(m.description)         << ","
       << "\"expected_hash\":"       << escape(m.expected_hash)       << ","
       << "\"license\":"             << escape(m.license)             << ","
       << "\"min_themis_version\":"  << escape(m.min_themis_version)  << ","
       << "\"name\":"                << escape(m.name)                << ","
       << "\"type\":"                << escape(pluginTypeToString(m.type)) << ","
       << "\"verified_publisher\":"  << (m.verified_publisher ? "true" : "false") << ","
       << "\"version\":"             << escape(m.version)
       << "}";
    return ss.str();
}

// =============================================================================
// Private helpers
// =============================================================================

const PinnedKey* SignedPluginRepository::findPinnedKeyLocked(
    const std::string& fingerprint) const
{
    for (const auto& k : pinned_keys_) {
        if (k.fingerprint == fingerprint) {
            return &k;
        }
    }
    return nullptr;
}

bool SignedPluginRepository::verifyEntryLocked(const RepositoryEntry& entry) const {
    const PinnedKey* key = findPinnedKeyLocked(entry.key_fingerprint);
    if (!key || !key->active) {
        return false;
    }
    std::vector<uint8_t> sig = base64Decode(entry.signature_b64);
    if (sig.empty()) {
        return false;
    }
    const std::string payload = canonicalManifestJson(entry.manifest);
    return verifyEd25519Signature(key->public_key, payload, sig);
}

bool SignedPluginRepository::verifyEd25519Signature(
    const std::vector<uint8_t>& public_key,
    const std::string& message,
    const std::vector<uint8_t>& signature) const
{
    if (public_key.size() != 32 || signature.size() != 64) {
        return false;
    }
    // Load the raw Ed25519 public key via OpenSSL EVP_PKEY using RAII wrapper
    EVP_PKEY* pkey_raw = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr, public_key.data(), public_key.size());
    if (!pkey_raw) {
        return false;
    }
    UniqueEvpPkey pkey(pkey_raw);
    
    // Create EVP_MD_CTX using RAII wrapper to ensure cleanup
    UniqueEvpMdCtx ctx(EVP_MD_CTX_new());
    if (!ctx) {
        return false;  // pkey and ctx will be automatically freed
    }
    
    bool ok = false;
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) == 1) {
        int rc = EVP_DigestVerify(
            ctx.get(),
            signature.data(), signature.size(),
            reinterpret_cast<const uint8_t*>(message.data()), message.size());
        ok = (rc == 1);
    }
    // Both ctx and pkey automatically freed when they go out of scope
    return ok;
}

} // namespace plugins
} // namespace themis

