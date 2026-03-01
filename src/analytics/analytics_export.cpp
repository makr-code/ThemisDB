/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analytics_export.cpp                               ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   65.0/100                                       ║
    • Total Lines:     679                                            ║
    • Open Issues:     TODOs: 0, Stubs: 7                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "analytics/analytics_export.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstring>
#include <string_view>
#include <spdlog/spdlog.h>

// Apache Arrow integration (optional)
// Enable with: cmake -DTHEMIS_HAS_ARROW=ON and install Apache Arrow via vcpkg
#ifdef THEMIS_HAS_ARROW
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <parquet/arrow/writer.h>
#endif

namespace themis {
namespace analytics {

#ifdef THEMIS_HAS_ARROW
/**
 * @brief Build an Arrow validity bitmap from the column's null_bitmap.
 *
 * Arrow convention: bit i = 1 means valid (non-null), bit i = 0 means null.
 * Our null_bitmap stores the opposite (true = null).
 *
 * Returns nullptr when there are no nulls, which is the common case and
 * allows Arrow to skip allocating a validity buffer entirely.
 */
static arrow::Result<std::shared_ptr<arrow::Buffer>> buildValidityBitmap(
    const ArrowRecordBatch::Column& col, int64_t length) {

    bool has_nulls = false;
    for (bool is_null : col.null_bitmap) {
        if (is_null) { has_nulls = true; break; }
    }
    if (!has_nulls) {
        return nullptr;  // All values valid; Arrow treats nullptr as all-valid
    }

    ARROW_ASSIGN_OR_RAISE(auto bitmap, arrow::AllocateBitmap(length));
    uint8_t* raw = bitmap->mutable_data();
    // Initialise to all-valid (all bits = 1)
    std::memset(raw, 0xFF, static_cast<size_t>(bitmap->size()));
    // Clear bits for null positions
    for (int64_t i = 0; i < length; ++i) {
        if (col.null_bitmap[static_cast<size_t>(i)]) {
            raw[i / 8] &= static_cast<uint8_t>(~(uint8_t(1) << (i % 8)));
        }
    }
    return bitmap;
}

/**
 * @brief Convert ArrowRecordBatch to Apache Arrow RecordBatch using
 *        zero-copy optimizations for numeric column types.
 *
 * INT64, DOUBLE, and TIMESTAMP columns are transferred without copying the
 * underlying data by wrapping the contiguous typed buffers maintained by
 * ArrowRecordBatch with arrow::Buffer::Wrap().  STRING and BOOLEAN columns
 * still use Arrow builders because their data is not stored contiguously.
 */
static arrow::Result<std::shared_ptr<arrow::RecordBatch>> convertToArrowRecordBatch(
    const ArrowRecordBatch& batch) {

    // Build schema
    std::vector<std::shared_ptr<arrow::Field>> fields;
    const auto& columns = batch.getColumns();
    const int64_t num_rows = static_cast<int64_t>(batch.rowCount());

    for (const auto& col : columns) {
        std::shared_ptr<arrow::DataType> arrow_type;

        switch (col.schema.type) {
            case ArrowRecordBatch::DataType::INT64:
                arrow_type = arrow::int64();
                break;
            case ArrowRecordBatch::DataType::DOUBLE:
                arrow_type = arrow::float64();
                break;
            case ArrowRecordBatch::DataType::STRING:
                arrow_type = arrow::utf8();
                break;
            case ArrowRecordBatch::DataType::BOOLEAN:
                arrow_type = arrow::boolean();
                break;
            case ArrowRecordBatch::DataType::TIMESTAMP:
                arrow_type = arrow::timestamp(arrow::TimeUnit::MILLI);
                break;
            default:
                return arrow::Status::TypeError("Unsupported data type");
        }

        fields.push_back(arrow::field(col.schema.name, arrow_type, col.schema.nullable));
    }

    auto schema = arrow::schema(fields);

    // Build arrays – zero-copy for numeric types, builder-based for others
    std::vector<std::shared_ptr<arrow::Array>> arrays;

    for (size_t col_idx = 0; col_idx < columns.size(); ++col_idx) {
        const auto& col = columns[col_idx];
        std::shared_ptr<arrow::Array> array;

        switch (col.schema.type) {
            case ArrowRecordBatch::DataType::INT64: {
                // Zero-copy: wrap the contiguous int64 buffer
                ARROW_ASSIGN_OR_RAISE(auto validity, buildValidityBitmap(col, num_rows));
                auto data_buf = arrow::Buffer::Wrap(
                    batch.getInt64Data(col_idx), num_rows);
                array = std::make_shared<arrow::Int64Array>(
                    num_rows, data_buf, validity);
                break;
            }
            case ArrowRecordBatch::DataType::DOUBLE: {
                // Zero-copy: wrap the contiguous double buffer
                ARROW_ASSIGN_OR_RAISE(auto validity, buildValidityBitmap(col, num_rows));
                auto data_buf = arrow::Buffer::Wrap(
                    batch.getDoubleData(col_idx), num_rows);
                array = std::make_shared<arrow::DoubleArray>(
                    num_rows, data_buf, validity);
                break;
            }
            case ArrowRecordBatch::DataType::TIMESTAMP: {
                // Zero-copy: wrap the contiguous int64 buffer
                ARROW_ASSIGN_OR_RAISE(auto validity, buildValidityBitmap(col, num_rows));
                auto data_buf = arrow::Buffer::Wrap(
                    batch.getInt64Data(col_idx), num_rows);
                array = std::make_shared<arrow::TimestampArray>(
                    arrow::timestamp(arrow::TimeUnit::MILLI),
                    num_rows, data_buf, validity);
                break;
            }
            case ArrowRecordBatch::DataType::STRING: {
                // Strings are stored as variants; use builder (no contiguous buffer)
                arrow::StringBuilder builder;
                for (size_t i = 0; i < col.data.size(); ++i) {
                    if (col.null_bitmap[i]) {
                        ARROW_RETURN_NOT_OK(builder.AppendNull());
                    } else {
                        ARROW_RETURN_NOT_OK(builder.Append(std::get<std::string>(col.data[i])));
                    }
                }
                ARROW_RETURN_NOT_OK(builder.Finish(&array));
                break;
            }
            case ArrowRecordBatch::DataType::BOOLEAN: {
                // Booleans are stored as variants; use builder (no contiguous buffer)
                arrow::BooleanBuilder builder;
                for (size_t i = 0; i < col.data.size(); ++i) {
                    if (col.null_bitmap[i]) {
                        ARROW_RETURN_NOT_OK(builder.AppendNull());
                    } else {
                        ARROW_RETURN_NOT_OK(builder.Append(std::get<bool>(col.data[i])));
                    }
                }
                ARROW_RETURN_NOT_OK(builder.Finish(&array));
                break;
            }
        }

        arrays.push_back(array);
    }

    return arrow::RecordBatch::Make(schema, num_rows, arrays);
}
#endif // THEMIS_HAS_ARROW

/**
 * @brief Analytics Exporter with optional Arrow support
 * 
 * This implementation provides JSON/CSV export always, and Arrow exports
 * when the THEMIS_HAS_ARROW flag is enabled.
 */
class StubAnalyticsExporter : public IAnalyticsExporter {
public:
    StubAnalyticsExporter() {
        spdlog::debug("AnalyticsExporter initialized (Arrow support: {})", 
#ifdef THEMIS_HAS_ARROW
            "enabled"
#else
            "disabled"
#endif
        );
    }
    ~StubAnalyticsExporter() override = default;

    ExportResult exportToFile(
        const ArrowRecordBatch& batch,
        const std::string& output_path,
        const ExportOptions& options) override {
        
        auto start = std::chrono::high_resolution_clock::now();
        
        ExportResult result;
        result.status = ExportStatus::SUCCESS;
        
        spdlog::debug("Exporting {} rows to file: {}", batch.rowCount(), output_path);
        
        try {
            std::string data;
            
            switch (options.format) {
                case ExportFormat::JSON:
                    data = batch.toJSON();
                    break;
                
                case ExportFormat::CSV:
                    data = exportToCSV(batch);
                    break;
                
                case ExportFormat::FMT_ARROW_IPC:
                case ExportFormat::FMT_ARROW_PARQUET:
                case ExportFormat::FMT_ARROW_FEATHER:
#ifdef THEMIS_HAS_ARROW
                {
                    // Real Arrow implementation
                    spdlog::debug("Using Apache Arrow for export format");
                    return exportToFileArrow(batch, output_path, options);
                }
#else
                {
                    // Arrow not available
                    spdlog::warn("Arrow export requested but THEMIS_HAS_ARROW not enabled");
                    result.status = ExportStatus::NOT_SUPPORTED;
                    result.message = "Arrow/Parquet/Feather export is not available. "
                                   "Rebuild with -DTHEMIS_HAS_ARROW=ON and install Apache Arrow library.";
                    return result;
                }
#endif
            }
            
            // Write to file (JSON/CSV)
            std::ofstream outfile(output_path);
            if (!outfile) {
                spdlog::error("Failed to open output file: {}", output_path);
                result.status = ExportStatus::FAILED;
                result.message = "Failed to open output file: " + output_path;
                return result;
            }
            
            outfile << data;
            outfile.close();
            
            result.rows_exported = batch.rowCount();
            result.bytes_written = data.size();
            
            spdlog::info("Export successful: {} rows, {} bytes", 
                        result.rows_exported, result.bytes_written);
            
        } catch (const std::exception& e) {
            spdlog::error("Export failed with exception: {}", e.what());
            result.status = ExportStatus::FAILED;
            result.message = std::string("Export failed: ") + e.what();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        return result;
    }

    std::string exportToString(
        const ArrowRecordBatch& batch,
        const ExportOptions& options) override {
        
        switch (options.format) {
            case ExportFormat::JSON:
                return batch.toJSON();
            
            case ExportFormat::CSV:
                return exportToCSV(batch);
            
            case ExportFormat::FMT_ARROW_IPC:
            case ExportFormat::FMT_ARROW_PARQUET:
            case ExportFormat::FMT_ARROW_FEATHER:
#ifdef THEMIS_HAS_ARROW
            {
                spdlog::debug("Arrow string export requested for {} rows", batch.rowCount());
                return exportToStringArrow(batch, options);
            }
#else
            {
                spdlog::warn("Arrow export to string requested but not available");
                return "# ERROR: Arrow export not available. "
                       "Rebuild with -DTHEMIS_HAS_ARROW=ON and install Apache Arrow library.";
            }
#endif
        }
        
        return "";
    }

    ExportResult exportWithCallback(
        const ArrowRecordBatch& batch,
        std::function<void(const std::vector<uint8_t>&)> callback,
        const ExportOptions& options) override {
        
        auto start = std::chrono::high_resolution_clock::now();
        
        ExportResult result;
        result.status = ExportStatus::SUCCESS;
        
        try {
            std::string data = exportToString(batch, options);
            
            // Convert string to bytes and call callback in chunks
            size_t chunk_size = options.batch_size * 100;  // Approximate chunk size
            size_t offset = 0;
            
            while (offset < data.size()) {
                size_t len = std::min(chunk_size, data.size() - offset);
                std::vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + len);
                callback(chunk);
                offset += len;
            }
            
            result.rows_exported = batch.rowCount();
            result.bytes_written = data.size();
            
        } catch (const std::exception& e) {
            result.status = ExportStatus::FAILED;
            result.message = std::string("Export failed: ") + e.what();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        return result;
    }

    bool supportsFormat(ExportFormat format) const override {
#ifdef THEMIS_HAS_ARROW
        // With Arrow, all formats are supported
        return true;
#else
        // Without Arrow, only JSON and CSV are supported
        switch (format) {
            case ExportFormat::JSON:
            case ExportFormat::CSV:
                return true;
            case ExportFormat::FMT_ARROW_IPC:
            case ExportFormat::FMT_ARROW_PARQUET:
            case ExportFormat::FMT_ARROW_FEATHER:
                return false;
        }
        return false;
#endif
    }

    std::string getExporterInfo() const override {
#ifdef THEMIS_HAS_ARROW
        return "AnalyticsExporter v1.0 (Apache Arrow enabled)";
#else
        return "AnalyticsExporter v1.0 (JSON/CSV only, Arrow support not compiled)";
#endif
    }

private:
    std::string exportToCSV(const ArrowRecordBatch& batch) const {
        std::ostringstream oss;
        
        // Header
        const auto& columns = batch.getColumns();
        for (size_t i = 0; i < columns.size(); ++i) {
            oss << columns[i].schema.name;
            if (i < columns.size() - 1) {
                oss << ",";
            }
        }
        oss << "\n";
        
        // Data rows
        for (size_t row = 0; row < batch.rowCount(); ++row) {
            for (size_t col = 0; col < columns.size(); ++col) {
                const auto& column = columns[col];
                
                if (column.null_bitmap[row]) {
                    // Null value
                } else {
                    const auto& value = column.data[row];
                    
                    if (std::holds_alternative<int64_t>(value)) {
                        oss << std::get<int64_t>(value);
                    } else if (std::holds_alternative<double>(value)) {
                        oss << std::get<double>(value);
                    } else if (std::holds_alternative<std::string>(value)) {
                        // CSV string escaping (RFC 4180) with formula-injection protection.
                        // Quote the value when it contains a comma, double-quote, newline,
                        // or carriage-return, OR when it starts with a character that
                        // spreadsheet applications may interpret as a formula prefix
                        // (=, +, -, @).  Quoting prevents formula evaluation in
                        // LibreOffice Calc and Google Sheets; Excel additionally requires
                        // the user to acknowledge formula evaluation for quoted cells.
                        std::string str = std::get<std::string>(value);

                        static constexpr std::string_view kFormulaChars = "=+-@";
                        bool needs_quotes =
                            str.find(',')  != std::string::npos ||
                            str.find('"')  != std::string::npos ||
                            str.find('\n') != std::string::npos ||
                            str.find('\r') != std::string::npos ||
                            (!str.empty() && kFormulaChars.find(str[0]) != std::string::npos);

                        if (needs_quotes) {
                            oss << "\"";
                            for (char c : str) {
                                if (c == '"') oss << "\"\"";  // Escape double-quotes per RFC 4180
                                else oss << c;
                            }
                            oss << "\"";
                        } else {
                            oss << str;
                        }
                    } else if (std::holds_alternative<bool>(value)) {
                        oss << (std::get<bool>(value) ? "true" : "false");
                    }
                }
                
                if (col < columns.size() - 1) {
                    oss << ",";
                }
            }
            oss << "\n";
        }
        
        return oss.str();
    }

#ifdef THEMIS_HAS_ARROW
    ExportResult exportToFileArrow(
        const ArrowRecordBatch& batch,
        const std::string& output_path,
        const ExportOptions& options) {
        
        ExportResult result;
        result.status = ExportStatus::SUCCESS;
        
        try {
            // Convert to Arrow RecordBatch
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                spdlog::error("Failed to convert to Arrow RecordBatch: {}", 
                             arrow_batch_result.status().ToString());
                result.status = ExportStatus::FAILED;
                result.message = "Arrow conversion failed: " + arrow_batch_result.status().ToString();
                return result;
            }
            
            auto arrow_batch = arrow_batch_result.ValueOrDie();
            
            switch (options.format) {
                case ExportFormat::FMT_ARROW_IPC: {
                    // Arrow IPC Stream format
                    std::shared_ptr<arrow::io::FileOutputStream> outfile;
                    auto outfile_result = arrow::io::FileOutputStream::Open(output_path);
                    if (!outfile_result.ok()) {
                        spdlog::error("Failed to open Arrow IPC file: {}", 
                                     outfile_result.status().ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Failed to open file: " + outfile_result.status().ToString();
                        return result;
                    }
                    outfile = outfile_result.ValueOrDie();
                    
                    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
                    auto writer_result = arrow::ipc::MakeStreamWriter(outfile, arrow_batch->schema());
                    if (!writer_result.ok()) {
                        spdlog::error("Failed to create IPC writer: {}", 
                                     writer_result.status().ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Failed to create IPC writer: " + writer_result.status().ToString();
                        return result;
                    }
                    writer = writer_result.ValueOrDie();
                    
                    auto write_status = writer->WriteRecordBatch(*arrow_batch);
                    if (!write_status.ok()) {
                        spdlog::error("Failed to write Arrow IPC batch: {}", 
                                     write_status.ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Write failed: " + write_status.ToString();
                        return result;
                    }
                    
                    auto close_status = writer->Close();
                    if (!close_status.ok()) {
                        spdlog::error("Failed to close Arrow IPC writer: {}", 
                                     close_status.ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Close failed: " + close_status.ToString();
                        return result;
                    }
                    
                    result.bytes_written = outfile->Tell().ValueOrDie();
                    break;
                }
                
                case ExportFormat::FMT_ARROW_PARQUET: {
                    // Parquet format
                    std::shared_ptr<arrow::io::FileOutputStream> outfile;
                    auto outfile_result = arrow::io::FileOutputStream::Open(output_path);
                    if (!outfile_result.ok()) {
                        spdlog::error("Failed to open Parquet file: {}", 
                                     outfile_result.status().ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Failed to open file: " + outfile_result.status().ToString();
                        return result;
                    }
                    outfile = outfile_result.ValueOrDie();
                    
                    // Create table from batch
                    auto table = arrow::Table::Make(arrow_batch->schema(), arrow_batch->columns());
                    
                    // Build writer properties with optional compression
                    parquet::WriterProperties::Builder props_builder;
                    if (options.compress) {
                        if (options.compression_codec == "snappy") {
                            props_builder.compression(parquet::Compression::SNAPPY);
                        } else if (options.compression_codec == "gzip") {
                            props_builder.compression(parquet::Compression::GZIP);
                        } else if (options.compression_codec == "zstd") {
                            props_builder.compression(parquet::Compression::ZSTD);
                        } else if (options.compression_codec == "lz4") {
                            props_builder.compression(parquet::Compression::LZ4);
                        }
                        props_builder.compression_level(options.compression_level);
                    }
                    auto props = props_builder.build();
                    
                    auto write_status = parquet::arrow::WriteTable(
                        *table, 
                        arrow::default_memory_pool(),
                        outfile, 
                        options.batch_size,
                        props
                    );
                    
                    if (!write_status.ok()) {
                        spdlog::error("Failed to write Parquet file: {}", 
                                     write_status.ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Parquet write failed: " + write_status.ToString();
                        return result;
                    }
                    
                    result.bytes_written = outfile->Tell().ValueOrDie();
                    break;
                }
                
                case ExportFormat::FMT_ARROW_FEATHER: {
                    // Feather format (Arrow IPC File format)
                    std::shared_ptr<arrow::io::FileOutputStream> outfile;
                    auto outfile_result = arrow::io::FileOutputStream::Open(output_path);
                    if (!outfile_result.ok()) {
                        spdlog::error("Failed to open Feather file: {}", 
                                     outfile_result.status().ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Failed to open file: " + outfile_result.status().ToString();
                        return result;
                    }
                    outfile = outfile_result.ValueOrDie();
                    
                    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
                    auto writer_result = arrow::ipc::MakeFileWriter(outfile, arrow_batch->schema());
                    if (!writer_result.ok()) {
                        spdlog::error("Failed to create Feather writer: {}", 
                                     writer_result.status().ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Failed to create Feather writer: " + writer_result.status().ToString();
                        return result;
                    }
                    writer = writer_result.ValueOrDie();
                    
                    auto write_status = writer->WriteRecordBatch(*arrow_batch);
                    if (!write_status.ok()) {
                        spdlog::error("Failed to write Feather batch: {}", 
                                     write_status.ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Write failed: " + write_status.ToString();
                        return result;
                    }
                    
                    auto close_status = writer->Close();
                    if (!close_status.ok()) {
                        spdlog::error("Failed to close Feather writer: {}", 
                                     close_status.ToString());
                        result.status = ExportStatus::FAILED;
                        result.message = "Close failed: " + close_status.ToString();
                        return result;
                    }
                    
                    result.bytes_written = outfile->Tell().ValueOrDie();
                    break;
                }
                
                default:
                    result.status = ExportStatus::FAILED;
                    result.message = "Unsupported export format";
                    return result;
            }
            
            result.rows_exported = batch.rowCount();
            spdlog::info("Arrow export successful: {} rows, {} bytes", 
                        result.rows_exported, result.bytes_written);
            
        } catch (const std::exception& e) {
            spdlog::error("Arrow export failed with exception: {}", e.what());
            result.status = ExportStatus::FAILED;
            result.message = std::string("Arrow export failed: ") + e.what();
        }
        
        return result;
    }
    
    std::string exportToStringArrow(
        const ArrowRecordBatch& batch,
        const ExportOptions& options) {
        
        try {
            // Convert to Arrow RecordBatch
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                spdlog::error("Failed to convert to Arrow RecordBatch for string export");
                return "# ERROR: Failed to convert to Arrow format";
            }
            
            auto arrow_batch = arrow_batch_result.ValueOrDie();
            
            // Use in-memory buffer
            auto buffer_result = arrow::io::BufferOutputStream::Create();
            if (!buffer_result.ok()) {
                return "# ERROR: Failed to create buffer";
            }
            auto buffer = buffer_result.ValueOrDie();
            
            switch (options.format) {
                case ExportFormat::FMT_ARROW_IPC: {
                    auto writer_result = arrow::ipc::MakeStreamWriter(buffer, arrow_batch->schema());
                    if (!writer_result.ok()) {
                        return "# ERROR: Failed to create IPC writer";
                    }
                    auto writer = writer_result.ValueOrDie();
                    
                    if (!writer->WriteRecordBatch(*arrow_batch).ok()) {
                        return "# ERROR: Failed to write IPC batch";
                    }
                    if (!writer->Close().ok()) {
                        return "# ERROR: Failed to close IPC writer";
                    }
                    break;
                }
                
                case ExportFormat::FMT_ARROW_FEATHER: {
                    auto writer_result = arrow::ipc::MakeFileWriter(buffer, arrow_batch->schema());
                    if (!writer_result.ok()) {
                        return "# ERROR: Failed to create Feather writer";
                    }
                    auto writer = writer_result.ValueOrDie();
                    
                    if (!writer->WriteRecordBatch(*arrow_batch).ok()) {
                        return "# ERROR: Failed to write Feather batch";
                    }
                    if (!writer->Close().ok()) {
                        return "# ERROR: Failed to close Feather writer";
                    }
                    break;
                }
                
                case ExportFormat::FMT_ARROW_PARQUET: {
                    // Parquet to string is not practical due to binary format
                    return "# ERROR: Parquet format cannot be exported to string (use file export instead)";
                }
                
                default:
                    return "# ERROR: Unsupported format";
            }
            
            auto result_buffer = buffer->Finish();
            if (!result_buffer.ok()) {
                return "# ERROR: Failed to finalize buffer";
            }
            
            auto data = result_buffer.ValueOrDie();
            return std::string(reinterpret_cast<const char*>(data->data()), data->size());
            
        } catch (const std::exception& e) {
            spdlog::error("Arrow string export failed: {}", e.what());
            return "# ERROR: " + std::string(e.what());
        }
    }
#endif // THEMIS_HAS_ARROW
};

// Factory implementations
std::unique_ptr<IAnalyticsExporter> ExporterFactory::createExporter(ExportFormat format) {
    // For now, return stub exporter for all formats
    // In the future, this would return format-specific exporters
    return std::make_unique<StubAnalyticsExporter>();
}

std::unique_ptr<IAnalyticsExporter> ExporterFactory::createDefaultExporter() {
    return std::make_unique<StubAnalyticsExporter>();
}

} // namespace analytics
} // namespace themis
