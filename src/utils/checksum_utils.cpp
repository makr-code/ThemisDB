/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            checksum_utils.cpp                                 ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:51:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     89                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/checksum_utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <openssl/evp.h>

namespace themis {
namespace utils {

// Internal helper: compute a hash using the given EVP message-digest algorithm.
// Returns a lowercase hex string, or "" on I/O or OpenSSL error.
static std::string computeFileHash(const std::string& file_path, const EVP_MD* md) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    constexpr size_t buffer_size = 32768;
    std::vector<char> buffer(buffer_size);

    while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buffer.data(), static_cast<size_t>(file.gcount())) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(hash[i]);
    }
    return oss.str();
}

std::string calculateSHA256(const std::string& file_path) {
    return computeFileHash(file_path, EVP_sha256());
}

// GAP-005 resolved: calculateMD5() now delegates to SHA-256 via the EVP API.
// The deprecated signature is kept for backward-compatible callers that still
// reference the old symbol (e.g. llm_deployment_plugin.cpp for legacy manifests).
// Production callers MUST migrate to calculateSHA256(). MD5 hard-rejection is
// planned for v2.0.0 (see FUTURE_ENHANCEMENTS.md "MD5 hard-reject").
std::string calculateMD5(const std::string& file_path) {
    return computeFileHash(file_path, EVP_sha256());
}

} // namespace utils
} // namespace themis
