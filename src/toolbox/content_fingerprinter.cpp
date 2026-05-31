/*
 * ThemisDB | File: content_fingerprinter.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:27:23
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 82
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "toolbox/content_fingerprinter.h"

#include <openssl/sha.h>

#include <iomanip>
#include <sstream>

namespace themis {
namespace toolbox {

namespace {

/// Compute SHA-256 of @p len bytes at @p data and return a 64-char hex string.
std::string sha256Hex(const unsigned char* data, std::size_t len) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(data, len, digest);

    std::ostringstream oss;
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
    return fp;
}

ContentFingerprint ContentFingerprinter::compute(
    const unsigned char* data, std::size_t len) const
{
    ContentFingerprint fp;
    fp.byte_len      = len;
    fp.token_estimate = (len == 0)
        ? 0
        : static_cast<std::size_t>(static_cast<double>(len) / kCharsPerToken + 0.5);

    if (len > 0) {
        fp.sha256_hex = sha256Hex(data, len);
    }
    return fp;
}

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

ContentFingerprint fingerprint(std::string_view text) {
    return ContentFingerprinter{}.compute(text);
}

} // namespace toolbox
} // namespace themis
