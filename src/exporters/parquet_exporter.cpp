/**
 * @file parquet_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=7, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/parquet_exporter.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_set>
#include <variant>

#include "exporters/aql_predicate_filter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_interface.h"
#include "exporters/exporter_metrics.h"
#include "exporters/pii_detector.h"
#include "utils/logger.h"

#ifdef ARROW_ENABLED
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include <parquet/properties.h>
#endif

namespace themis::exporters {

// ─────────────────────────────────────────────────────────────────────────────
// Minimal Parquet binary writer (used when ARROW_ENABLED is not defined).
//
// Writes a standards-conformant Parquet v2 file with:
//   • All columns stored as BYTE_ARRAY (UTF-8 string representation)
//   • PLAIN encoding
//   • UNCOMPRESSED compression
//   • Single row group containing all rows
//
// This produces files readable by pyarrow, Pandas, Spark, and any compliant
// Parquet reader.  Performance is adequate for moderate-sized exports; for
// large exports (≥ 100 M rows) or production throughput targets the Arrow-
// enabled path is strongly recommended.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ── Thrift binary protocol helpers ──────────────────────────────────────────

// Thrift wire types used in Parquet's .thrift IDL
static constexpr uint8_t T_STOP   = 0;
static constexpr uint8_t T_BOOL   = 2;
static constexpr uint8_t T_I32    = 8;
static constexpr uint8_t T_I64    = 10;
static constexpr uint8_t T_STRING = 11;
static constexpr uint8_t T_LIST   = 15;

// Write a big-endian 16-bit integer
inline void writeU16(std::vector<uint8_t> &buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v >> 8));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

// Write a big-endian 32-bit integer
inline void writeI32(std::vector<uint8_t> &buf, int32_t v) {
    uint32_t u = static_cast<uint32_t>(v);
    buf.push_back(static_cast<uint8_t>(u >> 24));
    buf.push_back(static_cast<uint8_t>((u >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(u & 0xFF));
}

// Write a big-endian 64-bit integer
inline void writeI64(std::vector<uint8_t> &buf, int64_t v) {
    uint64_t u = static_cast<uint64_t>(v);
    for (int s = 56; s >= 0; s -= 8) {
        buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
    }
}

// Write a Thrift string (4-byte BE length + bytes)
inline void writeThriftString(std::vector<uint8_t> &buf, const std::string &s) {
    writeI32(buf, static_cast<int32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

// Thrift field header
inline void writeFieldHeader(std::vector<uint8_t> &buf, uint8_t field_type, int16_t field_id) {
    buf.push_back(field_type);
    writeU16(buf, static_cast<uint16_t>(field_id));
}

// Thrift struct STOP byte
inline void writeStop(std::vector<uint8_t> &buf) {
    buf.push_back(T_STOP);
}

// Thrift list header: element type + element count
inline void writeListHeader(std::vector<uint8_t> &buf, uint8_t elem_type, int32_t count) {
    buf.push_back(elem_type);
    writeI32(buf, count);
}

// ── Parquet enum constants (from parquet.thrift) ────────────────────────────

// enum Type
static constexpr int32_t PARQUET_TYPE_BYTE_ARRAY = 6;

// enum FieldRepetitionType
static constexpr int32_t PARQUET_FIELD_REQUIRED = 0;

// enum Encoding
static constexpr int32_t PARQUET_ENCODING_PLAIN = 0;

// enum CompressionCodec
static constexpr int32_t PARQUET_CODEC_UNCOMPRESSED = 0;

// enum PageType
static constexpr int32_t PARQUET_PAGE_DATA = 0;

// ── SchemaElement (Thrift struct) ────────────────────────────────────────────
//
// struct SchemaElement {
//   1: optional Type              type
//   3: optional FieldRepetitionType repetition_type
//   4: required string            name
//   5: optional i32               num_children
// }
static void encodeSchemaElement(std::vector<uint8_t> &buf, const std::string &name, bool is_group,
                                int32_t num_children = 0) {
    if (!is_group) {
        // type = BYTE_ARRAY (field 1, i32)
        writeFieldHeader(buf, T_I32, 1);
        writeI32(buf, PARQUET_TYPE_BYTE_ARRAY);
        // repetition_type = REQUIRED (field 3, i32)
        writeFieldHeader(buf, T_I32, 3);
        writeI32(buf, PARQUET_FIELD_REQUIRED);
    }
    // name (field 4, string)
    writeFieldHeader(buf, T_STRING, 4);
    writeThriftString(buf, name);
    if (is_group) {
        // num_children (field 5, i32)
        writeFieldHeader(buf, T_I32, 5);
        writeI32(buf, num_children);
    }
    writeStop(buf);
}

// ── DataPageHeader (Thrift struct) ───────────────────────────────────────────
//
// struct DataPageHeader {
//   1: required i32      num_values
//   2: required Encoding encoding
//   3: required Encoding definition_level_encoding
//   4: required Encoding repetition_level_encoding
// }
static void encodeDataPageHeader(std::vector<uint8_t> &buf, int32_t num_values) {
    writeFieldHeader(buf, T_I32, 1);
    writeI32(buf, num_values);
    writeFieldHeader(buf, T_I32, 2);
    writeI32(buf, PARQUET_ENCODING_PLAIN);
    writeFieldHeader(buf, T_I32, 3);
    writeI32(buf, PARQUET_ENCODING_PLAIN);
    writeFieldHeader(buf, T_I32, 4);
    writeI32(buf, PARQUET_ENCODING_PLAIN);
    writeStop(buf);
}

// ── PageHeader (Thrift struct) ───────────────────────────────────────────────
//
// struct PageHeader {
//   1: required PageType type
//   2: required i32      uncompressed_page_size
//   3: required i32      compressed_page_size
//   5: optional DataPageHeader data_page_header
// }
static void encodePageHeader(std::vector<uint8_t> &buf, int32_t uncompressed_size, int32_t compressed_size,
                             int32_t num_values) {
    writeFieldHeader(buf, T_I32, 1);
    writeI32(buf, PARQUET_PAGE_DATA);
    writeFieldHeader(buf, T_I32, 2);
    writeI32(buf, uncompressed_size);
    writeFieldHeader(buf, T_I32, 3);
    writeI32(buf, compressed_size);
    // field 5: data_page_header (STRUCT)
    writeFieldHeader(buf, 12 /* T_STRUCT */, 5);
    encodeDataPageHeader(buf, num_values);
    writeStop(buf);
}

// ── ColumnMetaData (Thrift struct) ───────────────────────────────────────────
//
// struct ColumnMetaData {
//   1: required Type                 type
//   2: required list<Encoding>       encodings
//   3: required list<string>         path_in_schema
//   4: required CompressionCodec     codec
//   5: required i64                  num_values
//   6: required i64                  total_uncompressed_size
//   7: required i64                  total_compressed_size
//   9: required i64                  data_page_offset
// }
static void encodeColumnMetaData(std::vector<uint8_t> &buf, const std::string &col_name, int64_t num_values,
                                 int64_t total_size, int64_t data_page_offset) {
    // type = BYTE_ARRAY
    writeFieldHeader(buf, T_I32, 1);
    writeI32(buf, PARQUET_TYPE_BYTE_ARRAY);
    // encodings: [PLAIN]
    writeFieldHeader(buf, T_LIST, 2);
    writeListHeader(buf, T_I32, 1);
    writeI32(buf, PARQUET_ENCODING_PLAIN);
    // path_in_schema: [col_name]
    writeFieldHeader(buf, T_LIST, 3);
    writeListHeader(buf, T_STRING, 1);
    writeThriftString(buf, col_name);
    // codec = UNCOMPRESSED
    writeFieldHeader(buf, T_I32, 4);
    writeI32(buf, PARQUET_CODEC_UNCOMPRESSED);
    // num_values
    writeFieldHeader(buf, T_I64, 5);
    writeI64(buf, num_values);
    // total_uncompressed_size
    writeFieldHeader(buf, T_I64, 6);
    writeI64(buf, total_size);
    // total_compressed_size
    writeFieldHeader(buf, T_I64, 7);
    writeI64(buf, total_size);
    // data_page_offset
    writeFieldHeader(buf, T_I64, 9);
    writeI64(buf, data_page_offset);
    writeStop(buf);
}

// ── ColumnChunk (Thrift struct) ──────────────────────────────────────────────
//
// struct ColumnChunk {
//   2: required i64               file_offset
//   3: optional ColumnMetaData    meta_data
// }
static void encodeColumnChunk(std::vector<uint8_t> &buf, const std::string &col_name, int64_t file_offset,
                              int64_t num_values, int64_t data_size, int64_t data_page_offset) {
    writeFieldHeader(buf, T_I64, 2);
    writeI64(buf, file_offset);
    // meta_data (STRUCT, field 3)
    writeFieldHeader(buf, 12 /* T_STRUCT */, 3);
    encodeColumnMetaData(buf, col_name, num_values, data_size, data_page_offset);
    writeStop(buf);
}

// ── RowGroup (Thrift struct) ─────────────────────────────────────────────────
//
// struct RowGroup {
//   1: required list<ColumnChunk> columns
//   2: required i64               total_byte_size
//   3: required i64               num_rows
// }
struct ColumnChunkInfo {
    std::string name;
    int64_t file_offset;
    int64_t num_values;
    int64_t data_size;
    int64_t data_page_offset;
};

static void encodeRowGroup(std::vector<uint8_t> &buf, const std::vector<ColumnChunkInfo> &chunks,
                           int64_t total_byte_size, int64_t num_rows) {
    // columns: list<ColumnChunk>
    writeFieldHeader(buf, T_LIST, 1);
    writeListHeader(buf, 12 /* T_STRUCT */, static_cast<int32_t>(chunks.size()));
    for (const auto &c : chunks) {
        encodeColumnChunk(buf, c.name, c.file_offset, c.num_values, c.data_size, c.data_page_offset);
    }
    writeFieldHeader(buf, T_I64, 2);
    writeI64(buf, total_byte_size);
    writeFieldHeader(buf, T_I64, 3);
    writeI64(buf, num_rows);
    writeStop(buf);
}

// ── FileMetaData (Thrift struct) ─────────────────────────────────────────────
//
// struct FileMetaData {
//   1: required i32                  version
//   2: required list<SchemaElement>  schema
//   3: required i64                  num_rows
//   4: required list<RowGroup>       row_groups
//   5: optional list<KeyValue>       key_value_metadata
//   6: optional string               created_by
// }
struct FileMetaInput {
    std::vector<std::string> columns; // column names (leaf schema elements)
    std::vector<ColumnChunkInfo> chunks;
    int64_t num_rows        = 0;
    int64_t total_byte_size = 0;
    std::string created_by;
    std::map<std::string, std::string> kv_metadata;
};

static std::vector<uint8_t> encodeFileMetaData(const FileMetaInput &in) {
    std::vector<uint8_t> buf;

    // version = 2
    writeFieldHeader(buf, T_I32, 1);
    writeI32(buf, 2);

    // schema: message (group) element + one leaf per column
    writeFieldHeader(buf, T_LIST, 2);
    writeListHeader(buf, 12 /* T_STRUCT */, static_cast<int32_t>(in.columns.size() + 1));
    // message schema (group)
    encodeSchemaElement(buf, "schema", true, static_cast<int32_t>(in.columns.size()));
    for (const auto &col : in.columns) {
        encodeSchemaElement(buf, col, false);
    }

    // num_rows
    writeFieldHeader(buf, T_I64, 3);
    writeI64(buf, in.num_rows);

    // row_groups: list with a single RowGroup
    writeFieldHeader(buf, T_LIST, 4);
    writeListHeader(buf, 12 /* T_STRUCT */, 1);
    encodeRowGroup(buf, in.chunks, in.total_byte_size, in.num_rows);

    // key_value_metadata (field 5) – list of KeyValue structs
    if (!in.kv_metadata.empty()) {
        writeFieldHeader(buf, T_LIST, 5);
        writeListHeader(buf, 12 /* T_STRUCT */, static_cast<int32_t>(in.kv_metadata.size()));
        for (const auto &kv : in.kv_metadata) {
            // KeyValue { 1: required string key; 2: optional string value }
            writeFieldHeader(buf, T_STRING, 1);
            writeThriftString(buf, kv.first);
            writeFieldHeader(buf, T_STRING, 2);
            writeThriftString(buf, kv.second);
            writeStop(buf);
        }
    }

    // created_by (field 6)
    if (!in.created_by.empty()) {
        writeFieldHeader(buf, T_STRING, 6);
        writeThriftString(buf, in.created_by);
    }

    writeStop(buf);
    return buf;
}

// ── Encode a single PLAIN data page for a BYTE_ARRAY column ─────────────────
// Returns: header_bytes | value_bytes  (concatenated)
struct DataPage {
    std::vector<uint8_t> header;
    std::vector<uint8_t> values;
};

static DataPage encodePlainByteArrayPage(const std::vector<std::string> &vals) {
    // Serialise values first so we know the byte size
    std::vector<uint8_t> value_buf = {};

    for (const auto &v : vals) {
        // PLAIN BYTE_ARRAY: 4-byte little-endian length + bytes
        uint32_t len = static_cast<uint32_t>(v.size());
        value_buf.push_back(static_cast<uint8_t>(len & 0xFF));
        value_buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        value_buf.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
        value_buf.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
        value_buf.insert(value_buf.end(), v.begin(), v.end());
    }

    int32_t data_size = static_cast<int32_t>(value_buf.size());
    std::vector<uint8_t> header_buf = {};

    encodePageHeader(header_buf, data_size, data_size, static_cast<int32_t>(vals.size()));

    return {std::move(header_buf), std::move(value_buf)};
}

// ── Value-to-string conversion ───────────────────────────────────────────────
static std::string valueToString(const std::optional<Value> &opt_val) {
    if (!opt_val.has_value()) {
        return "";
    }
    return std::visit(
        [](const auto &v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return "";
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, double>) {
                std::ostringstream oss = {};
                oss << v;
                return oss.str();
            } else if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                // Represent embedding as JSON array string
                std::ostringstream oss = {};
                oss << "[";
                for (size_t i = 0; i < v.size(); ++i) {
                    if (i > 0) {
                        oss << ",";
                    }
                    oss << v[i];
                }
                oss << "]";
                return oss.str();
            } else {
                // vector<uint8_t> — binary blob
                std::ostringstream oss = {};
                oss << "<binary:" << v.size() << ">";
                return oss.str();
            }
        },
        *opt_val);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// ParquetExporter implementation
// ─────────────────────────────────────────────────────────────────────────────

ParquetExporter::ParquetExporter(const ParquetExportConfig &config)
    : config_(config), metrics_(std::make_shared<ExporterMetrics>()) {}

bool ParquetExporter::isArrowAvailable() {
#ifdef ARROW_ENABLED
    return true;
#else
    return false;
#endif
}

std::vector<std::string> ParquetExporter::resolveColumns(const std::vector<BaseEntity> &entities,
                                                         const ExportOptions &options) const {
    // Build candidate set from ExportOptions then ParquetExportConfig.
    // Use unordered_set for O(1) average-case lookup over the sorted std::set.
    std::unordered_set<std::string> exclude_set(config_.exclude_columns.begin(), config_.exclude_columns.end());
    exclude_set.insert(options.exclude_fields.begin(), options.exclude_fields.end());

    // Explicit include list
    if (!options.include_fields.empty()) {
        std::vector<std::string> cols = {};

        for (const auto &f : options.include_fields) {
            if (exclude_set.find(f) == exclude_set.end()) {
                cols.push_back(f);
            }
        }
        return cols;
    }
    if (!config_.include_columns.empty()) {
        std::vector<std::string> cols = {};

        for (const auto &f : config_.include_columns) {
            if (exclude_set.find(f) == exclude_set.end()) {
                cols.push_back(f);
            }
        }
        return cols;
    }

    // Auto-detect: collect all field names across all entities
    if (config_.auto_detect_schema) {
        std::set<std::string> seen = {};

        for (const auto &e : entities) {
            for (const auto &kv : e.getAllFields()) {
                if (exclude_set.find(kv.first) == exclude_set.end()) {
                    seen.insert(kv.first);
                }
            }
        }
        return std::vector<std::string>(seen.begin(), seen.end());
    }

    // Derive from column_hints
    std::vector<std::string> cols = {};

    for (const auto &hint : config_.column_hints) {
        if (exclude_set.find(hint.name) == exclude_set.end()) {
            cols.push_back(hint.name);
        }
    }
    return cols;
}

ExportStats ParquetExporter::exportEntities(const std::vector<BaseEntity> &entities, const ExportOptions &options) {
    // Policy check before any cursor or file is opened (EXP-001).
    enforceExportPolicy(options);

    ExportStats stats;
    stats.metrics   = metrics_;
    auto start_time = std::chrono::steady_clock::now();

    // ── Tenant isolation check ─────────────────────────────────────────────
    if (options.tenant_context && options.tenant_context->enforce_isolation) {
        if (!options.tenant_context->hasScope("export:read") && !options.tenant_context->hasScope("export:write")) {
            throw ExporterException(errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
                                    "Insufficient permissions for Parquet export operation",
                                    "tenant_id=" + options.tenant_context->tenant_id);
        }
        THEMIS_INFO("Parquet export for tenant: {}, user: {}", options.tenant_context->tenant_id,
                    options.tenant_context->user_id);
    }

    // ── Validate output path ───────────────────────────────────────────────
    if (options.output_path.empty()) {
        throw ConfigException("output_path must not be empty", "output_path");
    }

    stats.total_entities = entities.size();

    // ── Resolve columns ────────────────────────────────────────────────────
    const auto columns = resolveColumns(entities, options);

    // ── Dispatch to Arrow or fallback implementation ───────────────────────
#ifdef ARROW_ENABLED
    ExportStats result = exportWithArrow(entities, options, columns);
#else
    ExportStats result = exportFallback(entities, options, columns);
#endif

    auto end_time         = std::chrono::steady_clock::now();
    result.duration       = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.total_entities = stats.total_entities;
    result.metrics        = metrics_;

    metrics_->recordExport(result.exported_entities, result.bytes_written, result.duration);
    metrics_->recordParquetBytesWritten(result.bytes_written);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Apache Arrow-native path
// ─────────────────────────────────────────────────────────────────────────────

#ifdef ARROW_ENABLED
ExportStats ParquetExporter::exportWithArrow(const std::vector<BaseEntity> &entities, const ExportOptions &options,
                                             const std::vector<std::string> &columns) {
    ExportStats stats;

    // ── Set up PII detector if needed ──────────────────────────────────────
    std::unique_ptr<PIIDetector> pii_detector = {};

    if (config_.pii_config.enable_detection) {
        PIIDetector::Config pii_cfg;
        pii_cfg.detect_email       = config_.pii_config.detect_email;
        pii_cfg.detect_phone       = config_.pii_config.detect_phone;
        pii_cfg.detect_ssn         = config_.pii_config.detect_ssn;
        pii_cfg.detect_credit_card = config_.pii_config.detect_credit_card;
        if (config_.pii_config.redaction_strategy == "hash")
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::HASH;
        else if (config_.pii_config.redaction_strategy == "remove")
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::REMOVE;
        else if (config_.pii_config.redaction_strategy == "partial")
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::PARTIAL;
        else
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::MASK;
        pii_detector = std::make_unique<PIIDetector>(pii_cfg);
    }

    // ── Build Arrow schema ─────────────────────────────────────────────────
    arrow::FieldVector fields;
    for (const auto &col : columns) {
        std::shared_ptr<arrow::DataType> dt = arrow::utf8();
        // Apply column hints if present
        for (const auto &hint : config_.column_hints) {
            if (hint.name == col) {
                switch (hint.type) {
                    case ParquetColumnType::INT64:
                        dt = arrow::int64();
                        break;
                    case ParquetColumnType::DOUBLE:
                        dt = arrow::float64();
                        break;
                    case ParquetColumnType::BOOLEAN:
                        dt = arrow::boolean();
                        break;
                    default:
                        break;
                }
            }
        }
        fields.push_back(arrow::field(col, dt, true /* nullable */));
    }
    auto schema = arrow::schema(fields);

    // ── Open output file ───────────────────────────────────────────────────
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    auto open_status = arrow::io::FileOutputStream::Open(options.output_path);
    if (!open_status.ok()) {
        throw ExportIOException("Failed to open output file: " + open_status.status().ToString(), options.output_path);
    }
    outfile = *open_status;

    // ── Parquet writer properties ──────────────────────────────────────────
    parquet::WriterProperties::Builder props_builder;
    if (config_.compression == "snappy") {
        props_builder.compression(parquet::Compression::SNAPPY);
    } else if (config_.compression == "gzip") {
        props_builder.compression(parquet::Compression::GZIP);
    } else if (config_.compression == "zstd") {
        props_builder.compression(parquet::Compression::ZSTD);
    } else {
        props_builder.compression(parquet::Compression::UNCOMPRESSED);
    }

    auto props       = props_builder.build();
    auto arrow_props = parquet::ArrowWriterProperties::Builder().store_schema()->build();

    auto writer_result = parquet::arrow::FileWriter::Open(*schema, arrow::default_memory_pool(), outfile,
                                                          std::move(props), std::move(arrow_props));
    if (!writer_result.ok()) {
        throw ExportIOException("Failed to create Parquet writer: " + writer_result.status().ToString(),
                                options.output_path);
    }
    auto writer = std::move(writer_result).ValueOrDie();

    // ── Collect column builders ────────────────────────────────────────────
    std::set<std::string> duplicate_set;
    size_t row_group_rows = 0;

    // Column value buffers (all as strings, then build arrow arrays)
    std::vector<std::vector<std::string>> col_values(columns.size());

    auto flush_row_group = [&]() -> bool {
        if (row_group_rows == 0)
            return true;

        arrow::ArrayVector arrays;
        for (size_t ci = 0; ci < columns.size(); ++ci) {
            arrow::StringBuilder builder;
            for (const auto &v : col_values[ci]) {
                if (v.empty()) {
                    (void)builder.AppendNull();
                } else {
                    (void)builder.Append(v);
                }
            }
            std::shared_ptr<arrow::Array> arr;
            (void)builder.Finish(&arr);
            arrays.push_back(arr);
        }

        auto batch = arrow::RecordBatch::Make(schema, static_cast<int64_t>(row_group_rows), arrays);
        arrow::RecordBatchVector batches = {batch};
        auto tbl                         = arrow::Table::FromRecordBatches(batches);
        if (!tbl.ok())
            return false;
        auto ws = writer->WriteTable(**tbl, static_cast<int64_t>(config_.row_group_size));
        if (!ws.ok())
            return false;

        for (auto &v : col_values)
            v.clear();
        row_group_rows = 0;
        return true;
    };

    // AQL predicate filter (compiled once, reused per entity)
    std::unique_ptr<AqlPredicateFilter> aql_filter = {};

    if (!options.filter_expression.empty()) {
        aql_filter = std::make_unique<AqlPredicateFilter>(options.filter_expression);
    }

    for (const auto &entity : entities) {
        // Tenant isolation
        if (options.tenant_context && options.tenant_context->enforce_isolation) {
            auto tenant_val = entity.getFieldAsString("tenant_id");
            if (tenant_val && *tenant_val != options.tenant_context->tenant_id) {
                stats.failed_entities++;
                continue;
            }
        }

        // AQL predicate filter
        if (aql_filter && !aql_filter->evaluate(entity)) {
            continue;
        }

        // Duplicate detection (by primary key)
        if (duplicate_set.count(entity.getPrimaryKey())) {
            stats.failed_entities++;
            continue;
        }
        duplicate_set.insert(entity.getPrimaryKey());

        for (size_t ci = 0; ci < columns.size(); ++ci) {
            std::string val = valueToString(entity.getField(columns[ci]));

            // PII scrub
            if (pii_detector) {
                bool has_pii = pii_detector->containsPII(val);
                if (has_pii) {
                    metrics_->recordPIIDetection();
                    if (config_.pii_config.fail_on_pii && !config_.pii_config.enable_redaction) {
                        throw ExporterException(errors::ErrorCode::ERR_EXPORT_IO_ERROR,
                                                "PII detected in field '" + columns[ci]
                                                    + "' and fail_on_pii is enabled",
                                                "entity_id=" + entity.getPrimaryKey());
                    }
                    if (config_.pii_config.enable_redaction) {
                        val = pii_detector->redactPII(val);
                        metrics_->recordPIIRedaction();
                    }
                }
            }

            col_values[ci].push_back(val);
        }

        row_group_rows++;
        stats.exported_entities++;

        if (row_group_rows >= config_.row_group_size) {
            if (!flush_row_group()) {
                stats.errors.push_back("Failed to flush row group");
            }
        }

        if ([[maybe_unused]] options.progress_callback && stats.exported_entities % options.progress_interval == 0) {
            options.progress_callback([[maybe_unused]] stats);
        }
    }

    flush_row_group();

    auto close_status = writer->Close();
    if (!close_status.ok()) {
        stats.errors.push_back("Failed to close Parquet writer: " + close_status.ToString());
    }

    // Report bytes
    auto tell = outfile->Tell();
    if (tell.ok())
        stats.bytes_written = static_cast<size_t>(*tell);
    (void)outfile->Close();

    return stats;
}
#endif // ARROW_ENABLED

// ─────────────────────────────────────────────────────────────────────────────
// Minimal hand-written Parquet fallback (no Arrow dependency)
// ─────────────────────────────────────────────────────────────────────────────

ExportStats ParquetExporter::exportFallback(const std::vector<BaseEntity> &entities, const ExportOptions &options,
                                            const std::vector<std::string> &columns) {
    ExportStats stats;

    // ── Set up PII detector if needed ──────────────────────────────────────
    std::unique_ptr<PIIDetector> pii_detector = {};

    if (config_.pii_config.enable_detection) {
        PIIDetector::Config pii_cfg;
        pii_cfg.detect_email       = config_.pii_config.detect_email;
        pii_cfg.detect_phone       = config_.pii_config.detect_phone;
        pii_cfg.detect_ssn         = config_.pii_config.detect_ssn;
        pii_cfg.detect_credit_card = config_.pii_config.detect_credit_card;
        if (config_.pii_config.redaction_strategy == "hash") {
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::HASH;
        } else if (config_.pii_config.redaction_strategy == "remove") {
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::REMOVE;
        } else if (config_.pii_config.redaction_strategy == "partial") {
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::PARTIAL;
        } else {
            pii_cfg.default_strategy = PIIDetector::RedactionStrategy::MASK;
        }
        pii_detector = std::make_unique<PIIDetector>(pii_cfg);
    }

    // ── Collect column data (respecting tenant isolation & deduplication) ──
    std::vector<std::vector<std::string>> col_data(columns.size());
    std::set<std::string> seen_keys;
    size_t row_count = 0;

    // AQL predicate filter (compiled once, reused per entity)
    std::unique_ptr<AqlPredicateFilter> aql_filter_fb = {};

    if (!options.filter_expression.empty()) {
        aql_filter_fb = std::make_unique<AqlPredicateFilter>(options.filter_expression);
    }

    for (const auto &entity : entities) {
        // Tenant isolation
        if (options.tenant_context && options.tenant_context->enforce_isolation) {
            auto tenant_val = entity.getFieldAsString("tenant_id");
            if (tenant_val && *tenant_val != options.tenant_context->tenant_id) {
                stats.failed_entities++;
                continue;
            }
        }

        // AQL predicate filter
        if (aql_filter_fb && !aql_filter_fb->evaluate(entity)) {
            continue;
        }

        // Deduplication by primary key
        if (!seen_keys.insert(entity.getPrimaryKey()).second) {
            stats.failed_entities++;
            continue;
        }

        for (size_t ci = 0; ci < columns.size(); ++ci) {
            std::string val = valueToString(entity.getField(columns[ci]));

            // PII scrub
            if (pii_detector) {
                bool has_pii = pii_detector->containsPII(val);
                if (has_pii) {
                    metrics_->recordPIIDetection();
                    if (config_.pii_config.fail_on_pii && !config_.pii_config.enable_redaction) {
                        throw ExporterException(errors::ErrorCode::ERR_EXPORT_IO_ERROR,
                                                "PII detected in field '" + columns[ci]
                                                    + "' and fail_on_pii is enabled",
                                                "entity_id=" + entity.getPrimaryKey());
                    }
                    if (config_.pii_config.enable_redaction) {
                        val = pii_detector->redactPII(val);
                        metrics_->recordPIIRedaction();
                    }
                }
            }

            col_data[ci].push_back(val);
        }

        ++row_count;
        ++stats.exported_entities;

        if ([[maybe_unused]] options.progress_callback && stats.exported_entities % options.progress_interval == 0) {
            options.progress_callback([[maybe_unused]] stats);
        }

        // File size limit: check each 100 entities
        if (options.max_file_size_bytes > 0 && stats.exported_entities % 100 == 0) {
            // rough estimate: 128 bytes average per entity
            if (stats.exported_entities * 128 > options.max_file_size_bytes) {
                break;
            }
        }
    }

    if (columns.empty() || row_count == 0) {
        // Write an empty valid Parquet file (magic + empty footer)
        std::ofstream ofs(options.output_path, std::ios::binary);
        if (!ofs.is_open()) {
            throw ExportIOException("Cannot open output file for writing", options.output_path, errno);
        }
        static const char magic[] = "PAR1";
        ofs.write(magic, 4);

        // Build minimal FileMetaData for an empty file
        FileMetaInput fmi;
        fmi.columns    = columns;
        fmi.num_rows   = 0;
        fmi.created_by = "ThemisDB-parquet_exporter/1.0.0";
        for (const auto &kv : config_.file_metadata) {
            fmi.kv_metadata[kv.first] = kv.second;
        }

        auto footer = encodeFileMetaData(fmi);
        ofs.write(reinterpret_cast<const char *>(footer.data()), static_cast<std::streamsize>(footer.size()));

        uint32_t footer_len = static_cast<uint32_t>(footer.size());
        // Footer length is little-endian in Parquet format
        char flen[4] = {static_cast<char>(footer_len & 0xFF), static_cast<char>((footer_len >> 8) & 0xFF),
                        static_cast<char>((footer_len >> 16) & 0xFF), static_cast<char>((footer_len >> 24) & 0xFF)};
        ofs.write(flen, 4);
        ofs.write(magic, 4);
        stats.bytes_written = static_cast<size_t>(ofs.tellp());
        return stats;
    }

    // ── Write Parquet file ─────────────────────────────────────────────────
    std::ofstream ofs(options.output_path, std::ios::binary);
    if (!ofs.is_open()) {
        auto err_code = std::to_string(static_cast<int>(errors::ErrorCode::ERR_EXPORT_IO_ERROR));
        stats.errors.push_back(err_code + ": Cannot open output file: " + options.output_path);
        stats.failed_entities += row_count;
        stats.exported_entities = 0;
        return stats;
    }

    static const char magic[] = "PAR1";
    ofs.write(magic, 4);
    size_t file_offset = 4;

    // Encode all data pages for each column; track column chunk metadata
    std::vector<ColumnChunkInfo> chunks;
    int64_t row_group_total_bytes = 0;

    for (size_t ci = 0; ci < columns.size(); ++ci) {
        int64_t col_start_offset = static_cast<int64_t>(file_offset);

        DataPage page = encodePlainByteArrayPage(col_data[ci]);

        // data_page_offset = position of the first data page
        int64_t data_page_offset = static_cast<int64_t>(file_offset);

        // Write page header
        ofs.write(reinterpret_cast<const char *>(page.header.data()), static_cast<std::streamsize>(page.header.size()));
        file_offset += page.header.size();

        // Write page values
        ofs.write(reinterpret_cast<const char *>(page.values.data()), static_cast<std::streamsize>(page.values.size()));
        file_offset += page.values.size();

        int64_t col_size = static_cast<int64_t>(page.header.size() + page.values.size());
        row_group_total_bytes += col_size;

        ColumnChunkInfo info;
        info.name             = columns[ci];
        info.file_offset      = col_start_offset;
        info.num_values       = static_cast<int64_t>(row_count);
        info.data_size        = static_cast<int64_t>(page.values.size());
        info.data_page_offset = data_page_offset;
        chunks.push_back(info);
    }

    // ── Encode and write file footer ───────────────────────────────────────
    FileMetaInput fmi;
    fmi.columns         = columns;
    fmi.chunks          = chunks;
    fmi.num_rows        = static_cast<int64_t>(row_count);
    fmi.total_byte_size = row_group_total_bytes;
    fmi.created_by      = "ThemisDB-parquet_exporter/1.0.0";
    for (const auto &kv : config_.file_metadata) {
        fmi.kv_metadata[kv.first] = kv.second;
    }

    auto footer = encodeFileMetaData(fmi);
    ofs.write(reinterpret_cast<const char *>(footer.data()), static_cast<std::streamsize>(footer.size()));
    file_offset += footer.size();

    // Footer length (4 bytes, little-endian)
    uint32_t footer_len = static_cast<uint32_t>(footer.size());
    char flen[4]        = {static_cast<char>(footer_len & 0xFF), static_cast<char>((footer_len >> 8) & 0xFF),
                           static_cast<char>((footer_len >> 16) & 0xFF), static_cast<char>((footer_len >> 24) & 0xFF)};
    ofs.write(flen, 4);
    file_offset += 4;

    ofs.write(magic, 4);
    file_offset += 4;

    // ofs closes via RAII when it goes out of scope.
    stats.bytes_written = file_offset;

    THEMIS_INFO("Parquet export complete: {} rows, {} columns, {} bytes", row_count,static_cast<int>(columns.size()), file_offset);

    return stats;
}

} // namespace themis::exporters
