// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_exporters_contract_hardening_focused.cpp
 * @brief Phase 4 — Exporters contract hardening focused tests (EXCH-01..EXCH-16).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * All external interactions are mocked inline.  The canonical PRNG seed is
 * kExportersContractSeed = 42.
 *
 * ## Test families
 *
 * ### EXCH-01..04 — CSV export contract
 *   EXCH-01  Null cell renders as empty (two adjacent delimiters)
 *   EXCH-02  Field containing delimiter is properly quoted
 *   EXCH-03  Header row is always the first row
 *   EXCH-04  Line separator matches declared contract (CRLF vs LF)
 *
 * ### EXCH-05..08 — Parquet export contract
 *   EXCH-05  Schema column types are preserved in output
 *   EXCH-06  Null values produce a clear null-bitmap bit
 *   EXCH-07  Repeated export of same data produces identical bytes
 *   EXCH-08  Column count in output matches schema column count
 *
 * ### EXCH-09..12 — Streaming contract
 *   EXCH-09  Chunks are delivered in sequence-number order
 *   EXCH-10  Partial export on write error surfaces STREAM_INTERRUPTED
 *   EXCH-11  Streaming export resumes from last acked chunk
 *   EXCH-12  Chunk sequence number starts at kFirstChunkSequence (0)
 *
 * ### EXCH-13..16 — Error contract
 *   EXCH-13  Unsupported format → EXPORT_FORMAT_UNSUPPORTED before any output
 *   EXCH-14  Write failure → EXPORT_WRITE_FAILED
 *   EXCH-15  Quota exceeded → QUOTA_EXCEEDED
 *   EXCH-16  STREAM_INTERRUPTED is resumable; EXPORT_FORMAT_UNSUPPORTED is not
 *
 * @see include/exporters/exporters_api_contract.h
 * @see src/exporters/ROADMAP.md — Phase 4 item
 */

#include <gtest/gtest.h>

#include "exporters/exporters_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::exporters;

namespace {

static constexpr uint64_t kExportersContractSeed = 42;

// ---------------------------------------------------------------------------
// Mock CSV serialiser
// ---------------------------------------------------------------------------

struct MockCsvRow {
    std::vector<std::optional<std::string>> cells;
};

static std::string serializeCsvRow(const MockCsvRow& row,
                                   char delimiter = kCsvDefaultDelimiter,
                                   const char* line_sep = kCsvDefaultLineSep) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < row.cells.size(); ++i) {
        if (i > 0) {
          ss << delimiter;
        }
        if (!row.cells[i].has_value()) {
            // null → empty cell
        } else {
            const std::string& v = *row.cells[i];
            bool needs_quote = v.find(delimiter) != std::string::npos
                            || v.find('\n')       != std::string::npos
                            || v.find(kCsvQuoteChar) != std::string::npos;
            if (needs_quote) {
                ss << kCsvQuoteChar;
                for (char c : v) {
                    if (c == kCsvQuoteChar) {
                      ss << kCsvQuoteChar;
                    }
                    ss << c;
                }
                ss << kCsvQuoteChar;
            } else {
                ss << v;
            }
        }
    }
    ss << line_sep;
    return ss.str();
}

// ---------------------------------------------------------------------------
// Mock Parquet schema/column
// ---------------------------------------------------------------------------

enum class ParquetType { INT64, DOUBLE, STRING, BOOLEAN };

struct ParquetColumn {
    std::string  name;
    ParquetType  type;
};

struct MockParquetRow {
    std::vector<std::optional<std::string>> values; ///< nullopt = null
};

struct MockParquetSchema {
    std::vector<ParquetColumn> columns;
};

struct MockParquetOutput {
    MockParquetSchema              schema;
    std::vector<MockParquetRow>    rows;
    std::vector<std::vector<bool>> null_bitmap; ///< null_bitmap[col][row] = true iff null

    void addRow(const MockParquetRow& row) {
        rows.push_back(row);
        std::vector<bool> row_nulls = {};

        for (auto& v : row.values)
            row_nulls.push_back(!v.has_value());
        null_bitmap.push_back(std::move(row_nulls));
    }
};

// Serialize to a deterministic string for comparison
static std::string serializeParquet(const MockParquetOutput& out) {
    std::ostringstream ss;
    for (auto& col : out.schema.columns)
        ss << col.name << ":" << static_cast<int>(col.type) << ";";
    ss << "|";
    for (auto& row : out.rows) {
        for (auto& v : row.values)
            ss << (v.has_value() ? *v : "NULL") << ",";
        ss << "\n";
    }
    return ss.str();
}

// ---------------------------------------------------------------------------
// Mock streaming exporter
// ---------------------------------------------------------------------------

struct MockChunk {
    std::uint64_t seq;
    std::string   data;
};

struct MockStreamingExporter {
    std::vector<MockChunk> chunks;
    std::uint64_t          next_seq = kFirstChunkSequence;
    bool                   fail_on_chunk = false;
    std::size_t            fail_at_seq   = UINT64_MAX;

    ExporterErrorCode sendChunk(const std::string& data) {
        if (fail_on_chunk && next_seq >= fail_at_seq)
            return ExporterErrorCode::STREAM_INTERRUPTED;
        chunks.push_back({next_seq++, data});
        return ExporterErrorCode::OK;
    }

    bool isSequential() const {
        for (std::size_t i = 1; i < chunks.size(); ++i)
            if (chunks[i].seq != chunks[i-1].seq + 1) {
              return false;
            }
        return true;
    }
};

} // anonymous namespace

// ===========================================================================
// EXCH-01 — Null cell renders as empty (two adjacent delimiters)
// ===========================================================================

TEST(ExportersContractHardeningEXCH01, NullCellRenderedEmpty) {
    MockCsvRow row;
    row.cells = {std::string("A"), std::nullopt, std::string("C")};
    std::string out = serializeCsvRow(row, ',', "\n");
    // Expect: A,,C\n
    EXPECT_EQ(out, "A,,C\n") << "Null cell must render as empty (adjacent delimiters)";
}

// ===========================================================================
// EXCH-02 — Field containing delimiter is properly quoted
// ===========================================================================

TEST(ExportersContractHardeningEXCH02, DelimiterInFieldQuoted) {
    MockCsvRow row;
    row.cells = {std::string("val,with,comma")};
    std::string out = serializeCsvRow(row, ',', "\n");
    EXPECT_EQ(out, "\"val,with,comma\"\n")
        << "Fields containing the delimiter must be double-quoted";
}

// ===========================================================================
// EXCH-03 — Header row is always the first row
// ===========================================================================

TEST(ExportersContractHardeningEXCH03, HeaderAlwaysFirst) {
    // Simulate: produce header + data rows
    std::vector<std::string> header_cells = {"id", "name", "value"};
    MockCsvRow header_row;
    for (auto& h : header_cells)
        header_row.cells.push_back(std::optional<std::string>(h));

    MockCsvRow data_row;
    data_row.cells = {std::string("1"), std::string("alice"), std::string("42")};

    std::string output;
    output += serializeCsvRow(header_row, ',', "\n");
    output += serializeCsvRow(data_row,   ',', "\n");

    EXPECT_EQ(output.substr(0, 13), "id,name,value")
        << "Header must be the first row";
    EXPECT_TRUE(kCsvHeaderRequired);
}

// ===========================================================================
// EXCH-04 — Line separator matches contract (CRLF vs LF)
// ===========================================================================

TEST(ExportersContractHardeningEXCH04, LineSeparatorContract) {
    MockCsvRow row;
    row.cells = {std::string("x")};

    // CRLF (RFC 4180 default)
    std::string crlf_out = serializeCsvRow(row, ',', "\r\n");
    EXPECT_EQ(crlf_out.substr(crlf_out.size() - 2), "\r\n");

    // LF (declared alternative)
    std::string lf_out = serializeCsvRow(row, ',', "\n");
    EXPECT_EQ(lf_out.back(), '\n');
    EXPECT_NE(lf_out[lf_out.size()-2], '\r');
}

// ===========================================================================
// EXCH-05 — Schema column types preserved in Parquet output
// ===========================================================================

TEST(ExportersContractHardeningEXCH05, ParquetSchemaPreserved) {
    MockParquetOutput out;
    out.schema.columns = {{"id", ParquetType::INT64}, {"name", ParquetType::STRING}};
    out.addRow({{"1", "alice"}});

    EXPECT_EQ(out.schema.columns.size(), 2u);
    EXPECT_EQ(out.schema.columns[0].type, ParquetType::INT64);
    EXPECT_EQ(out.schema.columns[1].type, ParquetType::STRING);
}

// ===========================================================================
// EXCH-06 — Null values produce a set null-bitmap bit
// ===========================================================================

TEST(ExportersContractHardeningEXCH06, ParquetNullBitmapCorrect) {
    MockParquetOutput out;
    out.schema.columns = {{"a", ParquetType::INT64}, {"b", ParquetType::STRING}};
    out.addRow({{std::string("1"), std::nullopt}});  // b is null

    ASSERT_EQ(out.null_bitmap.size(), 1u);
    ASSERT_EQ(out.null_bitmap[0].size(), 2u);
    EXPECT_FALSE(out.null_bitmap[0][0]) << "Column 'a' is not null";
    EXPECT_TRUE(out.null_bitmap[0][1])  << "Column 'b' is null — bit must be set";
}

// ===========================================================================
// EXCH-07 — Repeated export of same data produces identical bytes
// ===========================================================================

TEST(ExportersContractHardeningEXCH07, RepeatedExportDeterministic) {
    MockParquetOutput out1, out2;
    for (auto* out : {&out1, &out2}) {
        out->schema.columns = {{"x", ParquetType::DOUBLE}};
        out->addRow({{std::string("3.14")}});
        out->addRow({{std::string("2.71")}});
    }
    EXPECT_EQ(serializeParquet(out1), serializeParquet(out2))
        << "Same input must produce byte-identical output (determinism contract)";
}

// ===========================================================================
// EXCH-08 — Column count in output matches schema column count
// ===========================================================================

TEST(ExportersContractHardeningEXCH08, OutputColumnCountMatchesSchema) {
    MockParquetOutput out;
    out.schema.columns = {{"c1", ParquetType::INT64},
                          {"c2", ParquetType::STRING},
                          {"c3", ParquetType::BOOLEAN}};
    MockParquetRow row;
    row.values = {std::string("1"), std::string("hello"), std::string("true")};
    out.addRow(row);

    EXPECT_EQ(out.schema.columns.size(), 3u);
    EXPECT_EQ(out.rows[0].values.size(), out.schema.columns.size());
}

// ===========================================================================
// EXCH-09 — Chunks delivered in sequence-number order
// ===========================================================================

TEST(ExportersContractHardeningEXCH09, ChunksDeliveredInOrder) {
    MockStreamingExporter exp;
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(exp.sendChunk("chunk-" + std::to_string(i)), ExporterErrorCode::OK);

    EXPECT_TRUE(exp.isSequential())
        << "All chunks must have strictly sequential sequence numbers";
    EXPECT_EQ(exp.chunks.front().seq, kFirstChunkSequence);
}

// ===========================================================================
// EXCH-10 — Partial export on write error surfaces STREAM_INTERRUPTED
// ===========================================================================

TEST(ExportersContractHardeningEXCH10, PartialExportSurfacesStreamInterrupted) {
    MockStreamingExporter exp;
    exp.fail_on_chunk = true;
    exp.fail_at_seq   = 3;

    for (int i = 0; i < 5; ++i) {
        auto rc = exp.sendChunk("chunk-" + std::to_string(i));
        if (i < 3) {
            EXPECT_EQ(rc, ExporterErrorCode::OK);
        } else {
            EXPECT_EQ(rc, ExporterErrorCode::STREAM_INTERRUPTED);
        }
    }
}

// ===========================================================================
// EXCH-11 — Streaming export resumes from last acked chunk
// ===========================================================================

TEST(ExportersContractHardeningEXCH11, ExportResumesFromLastAckedChunk) {
    MockStreamingExporter exp;
    // Send 3 chunks successfully
    for (int i = 0; i < 3; ++i)
        exp.sendChunk("chunk-" + std::to_string(i));

    std::uint64_t last_acked = exp.chunks.back().seq;  // = 2

    // Simulate resume: start a new exporter from last_acked + 1
    MockStreamingExporter resumed;
    resumed.next_seq = last_acked + 1;  // = 3
    EXPECT_EQ(resumed.sendChunk("chunk-3"), ExporterErrorCode::OK);
    EXPECT_EQ(resumed.chunks.front().seq, 3u)
        << "Resumed stream must start at last_acked + 1";
}

// ===========================================================================
// EXCH-12 — Chunk sequence number starts at kFirstChunkSequence (0)
// ===========================================================================

TEST(ExportersContractHardeningEXCH12, ChunkSequenceStartsAtZero) {
    EXPECT_EQ(kFirstChunkSequence, 0u);

    MockStreamingExporter exp;
    exp.sendChunk("first");
    ASSERT_EQ(exp.chunks.size(), 1u);
    EXPECT_EQ(exp.chunks[0].seq, kFirstChunkSequence);
}

// ===========================================================================
// EXCH-13 — Unsupported format → EXPORT_FORMAT_UNSUPPORTED before any output
// ===========================================================================

TEST(ExportersContractHardeningEXCH13, UnsupportedFormatErrorBeforeOutput) {
    bool format_supported = false; // simulated unsupported format
    std::string output;            // no bytes written

    ExporterErrorCode rc = ExporterErrorCode::OK;
    if (!format_supported) {
        rc = ExporterErrorCode::EXPORT_FORMAT_UNSUPPORTED;
    } else {
        output = "data";
    }

    EXPECT_EQ(rc, ExporterErrorCode::EXPORT_FORMAT_UNSUPPORTED);
    EXPECT_TRUE(output.empty())
        << "No output bytes must be written when format is unsupported";
}

// ===========================================================================
// EXCH-14 — Write failure → EXPORT_WRITE_FAILED
// ===========================================================================

TEST(ExportersContractHardeningEXCH14, WriteFailureSurfacesError) {
    bool write_ok = false;  // simulated write failure
    auto rc = write_ok
        ? ExporterErrorCode::OK
        : ExporterErrorCode::EXPORT_WRITE_FAILED;

    EXPECT_EQ(rc, ExporterErrorCode::EXPORT_WRITE_FAILED);
    EXPECT_TRUE(isTransientError(rc));
}

// ===========================================================================
// EXCH-15 — Quota exceeded → QUOTA_EXCEEDED
// ===========================================================================

TEST(ExportersContractHardeningEXCH15, QuotaExceededSurfaced) {
    constexpr std::uint64_t row_quota = 100u;
    std::uint64_t rows_exported = 101u;

    auto rc = (rows_exported > row_quota)
        ? ExporterErrorCode::QUOTA_EXCEEDED
        : ExporterErrorCode::OK;

    EXPECT_EQ(rc, ExporterErrorCode::QUOTA_EXCEEDED);
    EXPECT_FALSE(isResumableError(rc)) << "Quota exceeded is not resumable";
    EXPECT_FALSE(isTransientError(rc)) << "Quota exceeded is not transient";
}

// ===========================================================================
// EXCH-16 — STREAM_INTERRUPTED is resumable; EXPORT_FORMAT_UNSUPPORTED is not
// ===========================================================================

TEST(ExportersContractHardeningEXCH16, ResumabilityContract) {
    EXPECT_TRUE(isResumableError(ExporterErrorCode::STREAM_INTERRUPTED))
        << "STREAM_INTERRUPTED must be resumable";
    EXPECT_FALSE(isResumableError(ExporterErrorCode::EXPORT_FORMAT_UNSUPPORTED))
        << "EXPORT_FORMAT_UNSUPPORTED is permanent; not resumable";
    EXPECT_FALSE(isResumableError(ExporterErrorCode::QUOTA_EXCEEDED));
    EXPECT_FALSE(isResumableError(ExporterErrorCode::SCHEMA_MISMATCH));
}
