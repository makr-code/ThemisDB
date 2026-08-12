/**
 * @file pii_stream_scanner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/pii_detection_engine.h"
#include "utils/lek_manager.h"

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace utils {

// ===========================================================================
// PIIStreamScanner
// ===========================================================================

PIIStreamScanner::PIIStreamScanner(std::shared_ptr<IPIIDetectionEngine> engine,
                                    PIIStreamScannerConfig cfg)
    : engine_(std::move(engine))
    , cfg_(cfg)
    , global_offset_(0)
{
    if (!engine_) throw std::invalid_argument("PIIStreamScanner: engine must not be null");
    // Auto-derive lookahead_bytes from the engine's maxPatternLength() when the
    // caller uses the default config (lookahead_bytes == kDefaultLookaheadBytes).
    // This makes the cross-chunk sliding-window overlap exactly equal to the
    // longest possible regex pattern, satisfying the chunk-boundary-aware
    // matching requirement.
    if (cfg_.lookahead_bytes == kDefaultLookaheadBytes) {
        cfg_.lookahead_bytes = engine_->maxPatternLength();
    }
}

std::vector<PIIFinding> PIIStreamScanner::scan_chunk(std::string_view chunk, bool is_last) {
    // Append incoming chunk to the lookahead buffer.
    lookahead_buf_.append(chunk.data(), chunk.size());

    // Determine how many bytes we can safely finalize: hold back the last
    // `lookahead_bytes` characters unless this is the final chunk (to handle
    // entity spans that straddle the boundary).
    size_t process_len = lookahead_buf_.size();
    size_t holdback    = 0;

    if (!is_last && lookahead_buf_.size() > cfg_.lookahead_bytes) {
        holdback    = cfg_.lookahead_bytes;
        process_len = lookahead_buf_.size() - holdback;
    }

    // Run detection on the portion we are ready to finalize.
    std::string window = lookahead_buf_.substr(0, process_len);
    auto raw_findings  = engine_->detectInText(window);

    // Filter by confidence and adjust offsets to be absolute in the document.
    std::vector<PIIFinding> result;
    result.reserve(raw_findings.size());

    for (auto& f : raw_findings) {
        if (f.confidence < cfg_.min_confidence) continue;
        // Only include findings that are entirely within the finalized window.
        if (f.end_offset > process_len) continue;

        f.start_offset += global_offset_;
        f.end_offset   += global_offset_;
        result.push_back(std::move(f));
    }

    // Advance the global offset and slide the window.
    global_offset_  += process_len;
    lookahead_buf_   = lookahead_buf_.substr(process_len);

    return result;
}

void PIIStreamScanner::reset() {
    lookahead_buf_.clear();
    global_offset_ = 0;
}

size_t PIIStreamScanner::bytes_processed() const {
    return global_offset_;
}

// ===========================================================================
// PIIStreamPseudonymizer helpers
// ===========================================================================

namespace {

/// Compute HMAC-SHA-256 of `value` keyed by `key`, return first 8 hex chars.
std::string hmacPseudonym(const std::string& key, const std::string& value) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned int  digest_len = SHA256_DIGEST_LENGTH;

    HMAC(EVP_sha256(),
         key.data(),  static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(value.data()),
         static_cast<int>(value.size()),
         digest, &digest_len);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 4; ++i) { // 4 bytes → 8 hex chars
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return oss.str();
}

} // anonymous namespace

// ===========================================================================
// PIIStreamPseudonymizer
// ===========================================================================

PIIStreamPseudonymizer::PIIStreamPseudonymizer(
        std::shared_ptr<IPIIDetectionEngine> engine,
        std::shared_ptr<LEKManager>          lek_mgr,
        Config                               cfg)
    : scanner_(std::move(engine),
               PIIStreamScannerConfig{cfg.lookahead_bytes, 0.0 /*accept all, filter later*/})
    , lek_mgr_(std::move(lek_mgr))
    , cfg_(std::move(cfg))
{
    if (!lek_mgr_) throw std::invalid_argument("PIIStreamPseudonymizer: lek_mgr must not be null");
}

std::string PIIStreamPseudonymizer::process_chunk(std::string_view chunk, bool is_last) {
    // Obtain findings with absolute document offsets.
    // We need the scanner's internal offset BEFORE this call so we can compute
    // relative positions within the returned text.
    size_t base_offset = scanner_.bytes_processed();

    auto findings = scanner_.scan_chunk(chunk, is_last);

    // Build the pseudonym key: tenant_id + ":" + current LEK id
    std::string hmac_key = cfg_.tenant_id + ":" + lek_mgr_->getCurrentLEK();

    // Reconstruct the text for this chunk with PII spans replaced.
    // `findings` use absolute offsets; we need offsets relative to this chunk.
    std::string result;
    result.reserve(chunk.size());

    // The scanner may not have returned findings for bytes it is still
    // holding in its lookahead buffer.  We only process the portion that
    // was finalized (chunk bytes up to scanner.bytes_processed() - base_offset).
    size_t finalized_len = scanner_.bytes_processed() - base_offset;
    std::string_view finalized_chunk = chunk.substr(0, finalized_len);

    size_t cursor = 0; // relative to finalized_chunk
    for (const auto& f : findings) {
        // Convert absolute offset → relative
        size_t rel_start = f.start_offset - base_offset;
        size_t rel_end   = f.end_offset   - base_offset;

        if (rel_start > finalized_chunk.size()) break;
        rel_end = std::min(rel_end, finalized_chunk.size());

        // Copy gap before this finding.
        if (rel_start > cursor) {
            result.append(finalized_chunk.data() + cursor, rel_start - cursor);
        }

        // Replace the PII span with its pseudonym.
        std::string original(finalized_chunk.data() + rel_start, rel_end - rel_start);
        result += hmacPseudonym(hmac_key, original);

        cursor = rel_end;
    }
    // Copy remaining text after last finding.
    if (cursor < finalized_chunk.size()) {
        result.append(finalized_chunk.data() + cursor, finalized_chunk.size() - cursor);
    }

    return result;
}

void PIIStreamPseudonymizer::reset() {
    scanner_.reset();
}

} // namespace utils
} // namespace themis

