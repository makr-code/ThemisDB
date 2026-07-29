/*
 * ThemisDB | File: exporters_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen exporters (CSV/Parquet/Arrow/stream) contract semantics for the active v1.x line.
 */

/**
 * @file exporters_api_contract.h
 * @brief Frozen exporters contract: determinism, streaming order, null handling, and error taxonomy.
 *
 * This header defines the normative contract for the exporters module.
 * All exporter implementations (CSV, Parquet, Arrow/IPC, streaming, incremental,
 * join exporters) must honour these semantics within the v1.x major line.
 *
 * ## Determinism Contract
 *
 * Given the same input data and the same format version, the output bytes MUST
 * be byte-for-byte identical across repeated export calls on the same platform.
 * This enables content-addressable checksums of export artifacts.
 *
 * ## Streaming Contract
 *
 * Chunks in a streaming export are delivered in the order they were produced.
 * Chunk reordering is a contract violation.  The consumer MUST be able to
 * reconstruct the full export by concatenating chunks in arrival order.
 *
 * ## Null Handling Contract
 *
 *   - CSV:     null → empty cell (two adjacent delimiters, or empty quoted field).
 *   - Parquet: null → bit clear in the definition-level / null-bitmap column.
 *   - Arrow:   null → bit clear in the validity bitmap.
 *
 * ## Error Contract
 *
 * An unsupported format must be signalled before any output bytes are written.
 * A write failure mid-stream surfaces STREAM_INTERRUPTED; the consumer must
 * discard partial output and may resume from the last successfully acked chunk.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump.
 *
 * @see src/exporters/ROADMAP.md — Phase 1 item
 * @see include/exporters/exporter_interface.h
 * @see include/exporters/streaming_exporter.h
 * @see include/exporters/parquet_exporter.h
 * @see include/exporters/arrow_ipc_exporter.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace exporters {

// ============================================================================
// § 1  Format versioning
// ============================================================================

/// Current CSV format version encoded in export metadata.
inline constexpr std::uint32_t kCsvFormatVersion = 1u;

/// Current Parquet format version.
inline constexpr std::uint32_t kParquetFormatVersion = 2u;

/// Current Arrow/IPC format version.
inline constexpr std::uint32_t kArrowFormatVersion = 1u;

// ============================================================================
// § 2  CSV contract constants
// ============================================================================

/// CSV default field delimiter.
inline constexpr char kCsvDefaultDelimiter = ',';

/// CSV quote character used for fields containing the delimiter or newlines.
inline constexpr char kCsvQuoteChar = '"';

/// CSV escape character for embedded quote characters (RFC 4180 doubling).
inline constexpr char kCsvEscapeChar = '"';

/// CSV line terminator — CRLF per RFC 4180; LF acceptable when declared.
inline constexpr char kCsvDefaultLineSep[] = "\r\n";

/// Maximum number of columns in a single CSV export.
inline constexpr std::size_t kCsvMaxColumns = 4096;

/// Header row MUST appear as the very first row of every CSV export.
inline constexpr bool kCsvHeaderRequired = true;

// ============================================================================
// § 3  Streaming contract constants
// ============================================================================

/// Default chunk size for streaming exports (bytes).
inline constexpr std::size_t kDefaultChunkBytes = 64 * 1024;

/// Maximum chunk size (operator-configurable upper bound).
inline constexpr std::size_t kMaxChunkBytes = 256 * 1024 * 1024;

/// Chunk sequence numbers start at 0 and increment by 1.
inline constexpr std::uint64_t kFirstChunkSequence = 0u;

// ============================================================================
// § 4  Export quota constants
// ============================================================================

/// Default maximum rows per single export operation (0 = unlimited).
inline constexpr std::uint64_t kDefaultExportRowLimit = 0u;

/// Default maximum bytes per single export operation (0 = unlimited).
inline constexpr std::uint64_t kDefaultExportByteLimit = 0u;

// ============================================================================
// § 5  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the exporters module.
 */
enum class ExporterErrorCode : int {
    /// No error.
    OK = 0,

    /// The requested export format is not supported by this installation.
    EXPORT_FORMAT_UNSUPPORTED = 1,

    /// An I/O write failure occurred while producing export output.
    EXPORT_WRITE_FAILED = 2,

    /// A streaming export was interrupted mid-stream (network, timeout, etc.).
    STREAM_INTERRUPTED = 3,

    /// The source schema does not match the declared export schema.
    SCHEMA_MISMATCH = 4,

    /// The export would exceed the configured row or byte quota.
    QUOTA_EXCEEDED = 5,

    /// A required column is missing from the source data.
    COLUMN_NOT_FOUND = 6,

    /// The Parquet or Arrow schema could not be serialised.
    SCHEMA_SERIALIZATION_FAILED = 7,

    /// Export was aborted by the caller (clean cancellation).
    EXPORT_ABORTED = 8,

    /// Internal exporter error.
    INTERNAL_ERROR = 9,
};

/// Returns true for errors from which an export can be resumed at the last
/// successfully acknowledged chunk.
[[nodiscard]] inline constexpr bool isResumableError(ExporterErrorCode code) noexcept {
    return code == ExporterErrorCode::STREAM_INTERRUPTED;
}

/// Returns true for errors that should be retried with the same parameters.
[[nodiscard]] inline constexpr bool isTransientError(ExporterErrorCode code) noexcept {
    return code == ExporterErrorCode::EXPORT_WRITE_FAILED
        || code == ExporterErrorCode::STREAM_INTERRUPTED;
}

// ============================================================================
// § 6  Determinism contract helpers
// ============================================================================

/**
 * @brief Identifies the canonical sort order for deterministic CSV export.
 *
 * When the source query does not supply an explicit ORDER BY, the exporter
 * applies this default ordering to guarantee byte-level determinism.
 */
enum class CsvDeterministicOrder : int {
    /// Sort by primary key ascending (default).
    PrimaryKeyAsc  = 0,
    /// Sort by insertion order (only valid for append-only tables).
    InsertionOrder = 1,
    /// No ordering applied — output is non-deterministic (disabled by default).
    None           = 2,
};

} // namespace exporters
} // namespace themis
