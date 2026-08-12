/**
 * @file storage_parquet_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "storage/columnar_format.h"
#include "utils/expected.h"

namespace themis {
namespace storage {

// ============================================================================
// Schema descriptor for a ColumnarFormat table export
// ============================================================================

/// Describes one column of the exported table.
struct ParquetColumnDesc {
    std::string name;        ///< Column name in the output Parquet file.
    ColumnType  type;        ///< Column element type (must match the ColumnSegment).
};

/// Configuration for a Parquet export operation.
struct ParquetExportConfig {
    /// Column descriptors (parallel to the ColumnSegment vectors).
    std::vector<ParquetColumnDesc> columns;

    /// Optional compression codec for Parquet data pages.
    /// Accepted values: "NONE", "SNAPPY", "ZSTD" (case-insensitive).
    /// Defaults to "SNAPPY" when Arrow is available, "NONE" otherwise.
    std::string compression = "SNAPPY";

    /// Row-group size in number of rows (0 = all rows in a single group).
    /// Default: 1 048 576 (1 M rows).
    size_t row_group_size = 1'048'576;
};

// ============================================================================
// StorageParquetExporter
// ============================================================================

/// Exports a ColumnarFormat table to an Apache Parquet v2 file.
///
/// When ARROW_ENABLED is defined at compile time the full Apache Arrow /
/// Parquet C++ library is used, producing a standards-compliant file with
/// proper native types and optional SNAPPY/ZSTD compression.
///
/// When ARROW_ENABLED is NOT defined a portable Parquet v2 binary writer
/// is used: all columns are stored as BYTE_ARRAY (UTF-8 string
/// representation), PLAIN encoding, UNCOMPRESSED compression.  Files are
/// readable by pyarrow, Pandas, Spark, and any compliant Parquet reader.
///
/// Error conditions:
///  - ERR_EXPORT_FORMAT_INVALID — unknown ColumnType encountered in schema
///    (ExportError::UNSUPPORTED_TYPE equivalent).
///  - ERR_EXPORT_IO_ERROR       — cannot open / write output file.
///  - ERR_EXPORT_CONFIG_INVALID — column count mismatch or empty schema.
///
/// Usage:
/// @code
///     StorageParquetExporter exporter;
///
///     ParquetExportConfig cfg;
///     cfg.columns = {{"id", ColumnType::INT64}, {"price", ColumnType::FLOAT64}};
///
///     // One ColumnSegment per column per row-group.
///     // segments[c] = all segments for column c.
///     auto res = exporter.exportToFile(segments, cfg, "/tmp/out.parquet");
///     if (!res) { /* handle error */ }
/// @endcode
class StorageParquetExporter {
public:
    StorageParquetExporter() = default;

    /// Export a table to a Parquet file on disk.
    ///
    /// @param column_segments  One entry per column; each entry is the ordered
    ///                         list of decoded ColumnSegments for that column
    ///                         (matching row partitioning across all columns).
    /// @param config           Export configuration (column names, compression).
    /// @param output_path      Destination file path.
    /// @returns                Ok(void) on success, or an Error.
    Result<void> exportToFile(
        const std::vector<std::vector<ColumnSegment>>& column_segments,
        const ParquetExportConfig& config,
        const std::string& output_path);

    /// Export to an in-memory buffer (useful for testing and streaming).
    Result<std::vector<uint8_t>> exportToBuffer(
        const std::vector<std::vector<ColumnSegment>>& column_segments,
        const ParquetExportConfig& config);

    /// Statistics from the most recent export operation.
    struct ExportStats {
        size_t rows_written   = 0;
        size_t columns_written = 0;
        size_t bytes_written  = 0;
        double elapsed_us     = 0.0;
        bool   arrow_used     = false;  ///< true if Apache Arrow path was taken
    };

    const ExportStats& lastStats() const noexcept { return stats_; }

private:
    ExportStats stats_;

    // Internal helpers
    Result<std::vector<uint8_t>> buildParquet(
        const std::vector<std::vector<ColumnSegment>>& column_segments,
        const ParquetExportConfig& config);
};

} // namespace storage
} // namespace themis
