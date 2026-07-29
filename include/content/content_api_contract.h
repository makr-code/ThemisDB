// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file content_api_contract.h
 * @brief Frozen API contract for the ThemisDB content module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The content module handles content ingestion, abuse detection, archival,
 * and content-type normalisation for ThemisDB storage operations.
 *
 * @section contracts API Contracts
 *
 * ### AbuseDetector
 * - `scan()` is deterministic for identical byte sequences + identical rule set.
 * - Returns a non-null verdict (ALLOW or BLOCK) for every input; never throws
 *   on malformed content — returns CONTENT_SCAN_ERROR instead.
 * - Verdict BLOCK includes a reason string; ALLOW reason string is empty.
 *
 * ### ArchiveProcessor
 * - `extract()` unpacks supported archive formats (ZIP, TAR, GZIP); unknown
 *   format → CONTENT_UNSUPPORTED_FORMAT.
 * - Extraction is size-bounded; zip-bomb detection → CONTENT_SIZE_LIMIT.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                       | Meaning                                       |
 * |----------------------------|-----------------------------------------------|
 * | CONTENT_SCAN_ERROR         | Abuse scan encountered internal error         |
 * | CONTENT_UNSUPPORTED_FORMAT | Archive format not supported                  |
 * | CONTENT_SIZE_LIMIT         | Extracted content exceeds size limit          |
 * | CONTENT_ENCODING_INVALID   | Content encoding cannot be decoded            |
 *
 * @section threading Threading Guarantees
 * - `AbuseDetector::scan()` is thread-safe; rule set is read-only after init.
 * - `ArchiveProcessor` is stateless; fully thread-safe.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 */

#include <cstdint>
#include <string>

namespace themis::content {

enum class ContentError : int32_t {
    kScanError           = 8000,
    kUnsupportedFormat   = 8001,
    kSizeLimit           = 8002,
    kEncodingInvalid     = 8003,
};

enum class ScanVerdict : int32_t { kAllow = 0, kBlock = 1 };

struct ScanResult {
    ScanVerdict verdict{ScanVerdict::kAllow};
    std::string reason; ///< Non-empty when verdict == kBlock
};

} // namespace themis::content
