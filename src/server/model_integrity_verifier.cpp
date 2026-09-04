/**
 * @file model_integrity_verifier.cpp
 * @brief Implementierung der SHA-256-Modellverifikation für ThemisDB.
 *
 * Verwendet OpenSSL EVP-API für kryptografische Hash-Berechnungen.
 * Thread-safe über std::shared_mutex.
 */

#include "server/model_integrity_verifier.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include "utils/logger.h"

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

std::shared_mutex ModelIntegrityVerifier::manifest_mutex_;
std::unordered_map<std::string, std::string> ModelIntegrityVerifier::manifest_hashes_;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string ModelIntegrityVerifier::computeSha256(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        THEMIS_WARN("ModelIntegrityVerifier: cannot open file '{}'", path);
        return {};
    }

    // RAII-Wrapper für EVP_MD_CTX
    struct EvpCtxDeleter {
        void operator()(EVP_MD_CTX* ctx) const noexcept { EVP_MD_CTX_free(ctx); }
    };
    std::unique_ptr<EVP_MD_CTX, EvpCtxDeleter> ctx(EVP_MD_CTX_new());
    if (!ctx) {
        THEMIS_ERROR("ModelIntegrityVerifier: EVP_MD_CTX_new() failed");
        return {};
    }

    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        THEMIS_ERROR("ModelIntegrityVerifier: EVP_DigestInit_ex failed");
        return {};
    }

    constexpr std::size_t kBufSize = 65536;
    std::array<char, kBufSize> buf{};
    while (file.read(buf.data(), static_cast<std::streamsize>(kBufSize)) || file.gcount() > 0) {
        const auto bytes_read = static_cast<std::size_t>(file.gcount());
        if (EVP_DigestUpdate(ctx.get(), buf.data(), bytes_read) != 1) {
            THEMIS_ERROR("ModelIntegrityVerifier: EVP_DigestUpdate failed");
            return {};
        }
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digest_len) != 1) {
        THEMIS_ERROR("ModelIntegrityVerifier: EVP_DigestFinal_ex failed");
        return {};
    }

    std::ostringstream hex = {};
    hex << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        hex << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    return hex.str();
}

bool ModelIntegrityVerifier::verifyModel(const std::string& path,
                                         const std::string& expected_sha256) {
    if (path.empty() || expected_sha256.empty()) {
        return false;
    }
    const std::string actual = computeSha256(path);
    if (actual.empty()) {
        return false;
    }
    // Constant-time string comparison to prevent timing side-channels on hash
    if (actual.size() != expected_sha256.size()) {
        return false;
    }
    // Use OpenSSL CRYPTO_memcmp for timing-safe comparison
    // (both strings are hex, same length if SHA-256)
    return (CRYPTO_memcmp(actual.c_str(), expected_sha256.c_str(), actual.size()) == 0);
}

bool ModelIntegrityVerifier::loadManifest(const std::string& manifest_path) {
    std::ifstream f(manifest_path);
    if (!f.is_open()) {
        THEMIS_WARN("ModelIntegrityVerifier: manifest not found at '{}'", manifest_path);
        return false;
    }

    nlohmann::json j;
    try {
        f >> j;
    } catch (const nlohmann::json::exception& ex) {
        THEMIS_ERROR("ModelIntegrityVerifier: failed to parse manifest '{}': {}", manifest_path,
                     ex.what());
        return false;
    }

    if (!j.contains("models") || !j["models"].is_object()) {
        THEMIS_ERROR("ModelIntegrityVerifier: manifest missing 'models' object");
        return false;
    }

    std::unordered_map<std::string, std::string> new_hashes = {};

    for (const auto& [model_id, model_info] : j["models"].items()) {
        if (!model_info.contains("sha256") || !model_info["sha256"].is_string()) {
            THEMIS_WARN("ModelIntegrityVerifier: model '{}' has no valid sha256 field", model_id);
            continue;
        }
        new_hashes[model_id] = model_info["sha256"].get<std::string>();
    }

    const std::size_t loaded_entries = new_hashes.size();
    {
        std::unique_lock<std::shared_mutex> lock(manifest_mutex_);
        manifest_hashes_ = std::move(new_hashes);
    }
    THEMIS_INFO("ModelIntegrityVerifier: manifest loaded ({} entries)", loaded_entries);
    return true;
}

std::optional<std::string> ModelIntegrityVerifier::getExpectedHash(
    const std::string& model_id) {
    std::shared_lock<std::shared_mutex> lock(manifest_mutex_);
    auto it = manifest_hashes_.find(model_id);
    if (it == manifest_hashes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void ModelIntegrityVerifier::clearManifest() {
    std::unique_lock<std::shared_mutex> lock(manifest_mutex_);
    manifest_hashes_.clear();
}

}  // namespace server
}  // namespace themis
