/**
 * @file content_fingerprinter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/content_fingerprinter.h"

#include <openssl/sha.h>

#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace toolbox {

namespace {

/**
 * @brief Sha256 Hex.
 * @param[in] data Input parameter.
 * @param[in] len Input parameter.
 * @return Return value.
 * @details Calls: SHA256(), std::setfill(), std::setw(), str().
 */
std::string sha256Hex(const unsigned char* data, std::size_t len) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(data, len, digest);

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    return oss.str();
}

constexpr double kCharsPerToken = 4.0;

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// ContentFingerprinter
// ─────────────────────────────────────────────────────────────────────────────

ContentFingerprint ContentFingerprinter::compute(std::string_view text) const {
    auto t0 = std::chrono::steady_clock::now();
    
    ContentFingerprint fp;
    fp.byte_len      = text.size();
    fp.token_estimate = (text.empty())
        ? 0
        : static_cast<std::size_t>(static_cast<double>(text.size()) / kCharsPerToken + 0.5);

    if (!text.empty()) {
        fp.sha256_hex = sha256Hex(
            reinterpret_cast<const unsigned char*>(text.data()),
            text.size());
    }
    
    auto t1 = std::chrono::steady_clock::now();
    fp.latency_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    
    return fp;
}

ContentFingerprint ContentFingerprinter::compute(
    const unsigned char* data, std::size_t len) const
{
    auto t0 = std::chrono::steady_clock::now();
    
    ContentFingerprint fp;
    fp.byte_len      = len;
    fp.token_estimate = (len == 0)
        ? 0
        : static_cast<std::size_t>(static_cast<double>(len) / kCharsPerToken + 0.5);

    if (len > 0) {
        fp.sha256_hex = sha256Hex(data, len);
    }
    
    auto t1 = std::chrono::steady_clock::now();
    fp.latency_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    
    return fp;
}

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fingerprint.
 * @param[in] text Input parameter.
 * @return Return value.
 * @details Calls: compute().
 */
ContentFingerprint fingerprint(std::string_view text) {
    return ContentFingerprinter{}.compute(text);
}

} // namespace toolbox
} // namespace themis
