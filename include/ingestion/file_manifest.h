/**
 * @file file_manifest.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion/file_format.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <cstdint>

namespace themis {
namespace ingestion {

/**
 * @brief EXIF / embedded metadata extracted from a file.
 *
 * Keys follow the Exif tag names (e.g. "DateTime", "GPSLatitude") for image
 * files.  For PDF, DOCX and similar formats, document properties use keys
 * such as "Author", "Title", "Subject", "Creator", "Producer".
 * All values are stored as strings; callers convert as needed.
 */
using ExifMap = std::unordered_map<std::string, std::string>;

/**
 * @brief Canonical description of an ingested file.
 *
 * `FileManifest` is produced by the File Intake Layer (Stage 1) and flows
 * unchanged through every workflow step inside `ExtractionContext::manifest`.
 * Steps read from it; none of them write back to it.
 *
 * Fields
 * ──────
 * • `file_id`       — SHA-256 hex digest of the raw file bytes, prefixed with
 *                     "sha256:".  Stable identity across re-ingestion of the
 *                     same bytes.
 * • `original_path` — Absolute filesystem path at ingestion time (may be a
 *                     temporary upload path).
 * • `detected_mime` — MIME type string, e.g. "application/pdf".
 * • `detected_format` — Strongly-typed format enum resolved from MIME + magic
 *                       bytes.
 * • `file_size_bytes` — Raw byte count of the original file.
 * • `filename_stem`  — Basename without extension, e.g. "BImSchG_2024".
 * • `extension`      — Lower-case extension including dot, e.g. ".pdf".
 * • `created_at`     — File creation timestamp (filesystem or embedded).
 * • `modified_at`    — File modification timestamp.
 * • `exif`           — Format-specific metadata map (EXIF for images, XMP/PDF
 *                      info dict for documents).  May be empty.
 */
struct FileManifest {
    // ── Identity ──────────────────────────────────────────────────────────────
    std::string file_id;           ///< "sha256:<hex64>" — stable content ID
    std::string original_path;     ///< Absolute path at ingestion time

    // ── MIME / format ─────────────────────────────────────────────────────────
    std::string detected_mime;           ///< e.g. "application/pdf"
    FileFormat  detected_format{FileFormat::UNKNOWN}; ///< Strongly-typed format
    std::string detected_mime_charset;   ///< e.g. "utf-8" (text formats only)

    // ── Size ──────────────────────────────────────────────────────────────────
    std::uint64_t file_size_bytes{0};

    // ── Names ─────────────────────────────────────────────────────────────────
    std::string filename_stem;     ///< Basename without extension
    std::string extension;         ///< Lower-case with dot, e.g. ".pdf"

    // ── Timestamps ────────────────────────────────────────────────────────────
    std::string created_at;        ///< ISO-8601 or empty if unknown
    std::string modified_at;       ///< ISO-8601 or empty if unknown

    // ── Embedded metadata ─────────────────────────────────────────────────────
    ExifMap exif;                  ///< EXIF / XMP / document properties

    // ── Convenience helpers ───────────────────────────────────────────────────

    /// Returns true when the file has a known (non-UNKNOWN) format.
    bool hasKnownFormat() const { return detected_format != FileFormat::UNKNOWN; }

    /// Returns true when the MIME type is set (non-empty).
    bool hasMime() const { return !detected_mime.empty(); }

    /// Returns the EXIF value for `key`, or `default_val` when absent.
    const std::string& exifOr(const std::string& key,
                               const std::string& default_val) const {
        auto it = exif.find(key);
        return it != exif.end() ? it->second : default_val;
    }
};

} // namespace ingestion
} // namespace themis
