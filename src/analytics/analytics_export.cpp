/**
 * @file analytics_export.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "analytics/analytics_export.h"

#include <chrono>
#include <cstring>
#include <fstream>

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
static arrow::Result<std::shared_ptr<arrow::Buffer>> buildValidityBitmap(const ArrowRecordBatch::Column &col,
                                                                         int64_t length) {
    bool has_nulls = false;
    for (bool is_null : col.null_bitmap) {
        if (is_null) {
            has_nulls = true;
            break;
        }
    }
    if (!has_nulls) {
        return nullptr; // All values valid; Arrow treats nullptr as all-valid
    }

    ARROW_ASSIGN_OR_RAISE(auto bitmap, arrow::AllocateBitmap(length));
    uint8_t *raw = bitmap->mutable_data();
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
static arrow::Result<std::shared_ptr<arrow::RecordBatch>> convertToArrowRecordBatch(const ArrowRecordBatch &batch) {
    // Build schema
    std::vector<std::shared_ptr<arrow::Field>> fields;
    const auto &columns    = batch.getColumns();
    const int64_t num_rows = static_cast<int64_t>(batch.rowCount());

    for (const auto &col : columns) {
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
        const auto &col = columns[col_idx];
        std::shared_ptr<arrow::Array> array;

        switch (col.schema.type) {
            case ArrowRecordBatch::DataType::INT64: {
                // Zero-copy: wrap the contiguous int64 buffer
                ARROW_ASSIGN_OR_RAISE(auto validity, buildValidityBitmap(col, num_rows));
                auto data_buf = arrow::Buffer::Wrap(batch.getInt64Data(col_idx), num_rows);
                array         = std::make_shared<arrow::Int64Array>(num_rows, data_buf, validity);
                break;
            }
            case ArrowRecordBatch::DataType::DOUBLE: {
                // Zero-copy: wrap the contiguous double buffer
                ARROW_ASSIGN_OR_RAISE(auto validity, buildValidityBitmap(col, num_rows));
                auto data_buf = arrow::Buffer::Wrap(batch.getDoubleData(col_idx), num_rows);
                array         = std::make_shared<arrow::DoubleArray>(num_rows, data_buf, validity);
                break;
            }
            case ArrowRecordBatch::DataType::TIMESTAMP: {
                // Zero-copy: wrap the contiguous int64 buffer
                ARROW_ASSIGN_OR_RAISE(auto validity, buildValidityBitmap(col, num_rows));
                auto data_buf = arrow::Buffer::Wrap(batch.getInt64Data(col_idx), num_rows);
                array = std::make_shared<arrow::TimestampArray>(arrow::timestamp(arrow::TimeUnit::MILLI), num_rows,
                                                                data_buf, validity);
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
 * @brief JSON and CSV exporter.
 *
 * Handles ExportFormat::JSON and ExportFormat::CSV.  Arrow-based formats
 * (IPC, Parquet, Feather) are handled by dedicated exporter classes that are
 * only compiled when THEMIS_HAS_ARROW is defined.  Use
 * ExporterFactory::createExporter(format) to obtain the correct exporter for
 * a given format.
 */
class JSONCSVExporter : public IAnalyticsExporter {
  public:
    JSONCSVExporter() {
        spdlog::debug("JSONCSVExporter initialized");
    }
    ~JSONCSVExporter() override = default;

    ExportResult exportToFile(const ArrowRecordBatch &batch, const std::string &output_path,
                              [[maybe_unused]] const ExportOptions &options) override {
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
                [[fallthrough]];\n                case ExportFormat::FMT_ARROW_PARQUET:
                [[fallthrough]];\n                case ExportFormat::FMT_ARROW_FEATHER:
                    spdlog::warn("Arrow format requested on JSONCSVExporter; use createExporter(format)");
                    result.status  = ExportStatus::NOT_SUPPORTED;
                    result.message = "Arrow/Parquet/Feather export is not supported by JSONCSVExporter. "
                                     "Use ExporterFactory::createExporter(format) to obtain an Arrow exporter.";
                    return result;
            }

            // Write to file
            std::ofstream outfile(output_path);
            if (!outfile) {
                spdlog::error("Failed to open output file: {}", output_path);
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to open output file: " + output_path;
                return result;
            }

            outfile << data;
            outfile.close();

            result.rows_exported = batch.rowCount();
            result.bytes_written = data.size();

            spdlog::info("Export successful: {} rows, {} bytes", result.rows_exported, result.bytes_written);

        } catch (const std::exception &e) {
            spdlog::error("Export failed with exception: {}", e.what());
            result.status  = ExportStatus::FAILED;
            result.message = std::string("Export failed: ") + e.what();
        }

        auto end           = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

        return result;
    }

    std::string exportToString(const ArrowRecordBatch &batch, const ExportOptions &options) override {
        switch (options.format) {
            case ExportFormat::JSON:
                return batch.toJSON();

            case ExportFormat::CSV:
                return exportToCSV(batch);

            case ExportFormat::FMT_ARROW_IPC:
            [[fallthrough]];\n            case ExportFormat::FMT_ARROW_PARQUET:
            [[fallthrough]];\n            case ExportFormat::FMT_ARROW_FEATHER:
                spdlog::warn("Arrow format requested on JSONCSVExporter; use createExporter(format)");
                return "# ERROR: Arrow/Parquet/Feather export is not supported by JSONCSVExporter. "
                       "Use ExporterFactory::createExporter(format) to obtain an Arrow exporter.";
        }

        return "";
    }

    ExportResult exportWithCallback(const ArrowRecordBatch &batch,
                                    std::function<void(const std::vector<uint8_t> &)> callback,
                                    const ExportOptions &options) override {
        auto start = std::chrono::high_resolution_clock::now();

        ExportResult result;
        result.status = ExportStatus::SUCCESS;

        try {
            std::string data = exportToString(batch, options);

            // Convert string to bytes and call callback in chunks
            size_t chunk_size = options.batch_size * 100; // Approximate chunk size
            size_t offset     = 0;

            while (offset < data.size()) {
                size_t len = std::min(chunk_size, data.size() - offset);
                std::vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + len);
                callback([[maybe_unused]] chunk);
                offset += len;
            }

            result.rows_exported = batch.rowCount();
            result.bytes_written = data.size();

        } catch (const std::exception &e) {
            result.status  = ExportStatus::FAILED;
            result.message = std::string("Export failed: ") + e.what();
        }

        auto end           = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

        return result;
    }

    bool supportsFormat(ExportFormat format) const override {
        switch (format) {
            case ExportFormat::JSON:
            [[fallthrough]];\n            case ExportFormat::CSV:
                return true;
            case ExportFormat::FMT_ARROW_IPC:
            [[fallthrough]];\n            case ExportFormat::FMT_ARROW_PARQUET:
            [[fallthrough]];\n            case ExportFormat::FMT_ARROW_FEATHER:
                return false;
        }
        return false;
    }

    std::string getExporterInfo() const override {
        return "JSONCSVExporter v1.0 (JSON/CSV export)";
    }

  private:
    std::string exportToCSV(const ArrowRecordBatch &batch) const {
        std::ostringstream oss;

        // Header
        const auto &columns = batch.getColumns();
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
                const auto &column = columns[col];

                if (column.null_bitmap[row]) {
                    // Null value
                } else {
                    const auto &value = column.data[row];

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
                        bool needs_quotes = str.find(',') != std::string::npos || str.find('"') != std::string::npos
                                            || str.find('\n') != std::string::npos
                                            || str.find('\r') != std::string::npos
                                            || (!str.empty() && kFormulaChars.find(str[0]) != std::string::npos);

                        if (needs_quotes) {
                            oss << "\"";
                            for (char c : str) {
                                if (c == '"') {
                                    oss << "\"\""; // Escape double-quotes per RFC 4180
                                } else {
                                    oss << c;
                                }
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
};

#ifdef THEMIS_HAS_ARROW

/**
 * @brief Arrow IPC (stream) exporter.
 *
 * Writes data in the Apache Arrow IPC stream format.  Requires
 * THEMIS_HAS_ARROW to be defined at compile time.
 */
class ArrowIPCExporter : public IAnalyticsExporter {
  public:
    ArrowIPCExporter() {
        spdlog::debug("ArrowIPCExporter initialized");
    }
    ~ArrowIPCExporter() override = default;

    ExportResult exportToFile(const ArrowRecordBatch &batch, const std::string &output_path,
                              [[maybe_unused]] const ExportOptions &options) override {
        auto start = std::chrono::high_resolution_clock::now();
        ExportResult result;
        result.status = ExportStatus::SUCCESS;

        try {
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                spdlog::error("Failed to convert to Arrow RecordBatch: {}", arrow_batch_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Arrow conversion failed: " + arrow_batch_result.status().ToString();
                return result;
            }
            auto arrow_batch = arrow_batch_result.ValueOrDie();

            auto outfile_result = arrow::io::FileOutputStream::Open(output_path);
            if (!outfile_result.ok()) {
                spdlog::error("Failed to open Arrow IPC file: {}", outfile_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to open file: " + outfile_result.status().ToString();
                return result;
            }
            auto outfile = outfile_result.ValueOrDie();

            auto writer_result = arrow::ipc::MakeStreamWriter(outfile, arrow_batch->schema());
            if (!writer_result.ok()) {
                spdlog::error("Failed to create IPC writer: {}", writer_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to create IPC writer: " + writer_result.status().ToString();
                return result;
            }
            auto writer = writer_result.ValueOrDie();

            auto write_status = writer->WriteRecordBatch(*arrow_batch);
            if (!write_status.ok()) {
                spdlog::error("Failed to write Arrow IPC batch: {}", write_status.ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Write failed: " + write_status.ToString();
                return result;
            }

            auto close_status = writer->Close();
            if (!close_status.ok()) {
                spdlog::error("Failed to close Arrow IPC writer: {}", close_status.ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Close failed: " + close_status.ToString();
                return result;
            }

            result.bytes_written = outfile->Tell().ValueOrDie();
            result.rows_exported = batch.rowCount();
            spdlog::info("Arrow IPC export successful: {} rows, {} bytes", result.rows_exported, result.bytes_written);

        } catch (const std::exception &e) {
            spdlog::error("Arrow IPC export failed: {}", e.what());
            result.status  = ExportStatus::FAILED;
            result.message = std::string("Arrow IPC export failed: ") + e.what();
        }

        auto end           = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return result;
    }

    std::string exportToString(const ArrowRecordBatch &batch, [[maybe_unused]] const ExportOptions &options) override {
        // format is implicitly FMT_ARROW_IPC for this exporter
        try {
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                spdlog::error("Failed to convert to Arrow RecordBatch for IPC string export");
                return "# ERROR: Failed to convert to Arrow format";
            }
            auto arrow_batch = arrow_batch_result.ValueOrDie();

            auto buffer_result = arrow::io::BufferOutputStream::Create();
            if (!buffer_result.ok()) {
                return "# ERROR: Failed to create buffer";
            }
            auto buffer = buffer_result.ValueOrDie();

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

            auto result_buffer = buffer->Finish();
            if (!result_buffer.ok()) {
                return "# ERROR: Failed to finalize buffer";
            }
            auto data = result_buffer.ValueOrDie();
            return std::string(reinterpret_cast<const char *>(data->data()), data->size());

        } catch (const std::exception &e) {
            spdlog::error("Arrow IPC string export failed: {}", e.what());
            return "# ERROR: " + std::string(e.what());
        }
    }

    ExportResult exportWithCallback(const ArrowRecordBatch &batch,
                                    std::function<void(const std::vector<uint8_t> &)> callback,
                                    const ExportOptions &options) override {
        auto start = std::chrono::high_resolution_clock::now();
        ExportResult result;
        result.status = ExportStatus::SUCCESS;

        try {
            // Serialize directly to an Arrow-managed BufferOutputStream.
            // This avoids the intermediate std::string copy that the
            // exportToString() path would introduce; the IPC bytes are
            // written once into a contiguous Arrow buffer and then chunked
            // out to the callback without an extra full-buffer copy.
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                result.status  = ExportStatus::FAILED;
                result.message = "Arrow conversion failed: " + arrow_batch_result.status().ToString();
                return result;
            }
            auto arrow_batch = arrow_batch_result.ValueOrDie();

            auto bos_result = arrow::io::BufferOutputStream::Create();
            if (!bos_result.ok()) {
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to create buffer stream: " + bos_result.status().ToString();
                return result;
            }
            auto bos = bos_result.ValueOrDie();

            auto writer_result = arrow::ipc::MakeStreamWriter(bos, arrow_batch->schema());
            if (!writer_result.ok()) {
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to create IPC writer: " + writer_result.status().ToString();
                return result;
            }
            auto writer = writer_result.ValueOrDie();

            auto write_status = writer->WriteRecordBatch(*arrow_batch);
            if (!write_status.ok()) {
                result.status  = ExportStatus::FAILED;
                result.message = "Write failed: " + write_status.ToString();
                return result;
            }
            auto close_status = writer->Close();
            if (!close_status.ok()) {
                result.status  = ExportStatus::FAILED;
                result.message = "Close failed: " + close_status.ToString();
                return result;
            }

            auto buf_result = bos->Finish();
            if (!buf_result.ok()) {
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to finalize IPC buffer: " + buf_result.status().ToString();
                return result;
            }
            // ipc_buffer is the contiguous, Arrow-managed serialised IPC stream.
            // arrow::Buffer::Wrap() exposes the same memory region zero-copy for
            // consumers that accept arrow::Buffer; for this callback interface
            // (which requires std::vector<uint8_t>) one copy per chunk is made
            // directly from the buffer data pointer, eliminating the redundant
            // full-buffer copy that went through the intermediate std::string.
            auto ipc_buffer = buf_result.ValueOrDie();

            const uint8_t *base   = ipc_buffer->data();
            const size_t total_sz = static_cast<size_t>(ipc_buffer->size());
            const size_t chunk_sz = options.batch_size * 100;
            size_t offset         = 0;

            while (offset < total_sz) {
                size_t len = std::min(chunk_sz, total_sz - offset);
                callback(std::vector<uint8_t>(base + offset, base + offset + len));
                offset += len;
            }

            result.rows_exported = batch.rowCount();
            result.bytes_written = total_sz;
        } catch (const std::exception &e) {
            result.status  = ExportStatus::FAILED;
            result.message = std::string("Export failed: ") + e.what();
        }

        auto end           = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return result;
    }

    bool supportsFormat(ExportFormat format) const override {
        return format == ExportFormat::FMT_ARROW_IPC;
    }

    std::string getExporterInfo() const override {
        return "ArrowIPCExporter v1.0 (Apache Arrow IPC stream format)";
    }
};

/**
 * @brief Apache Parquet exporter.
 *
 * Writes data in the Apache Parquet columnar format with optional compression
 * (snappy, gzip, zstd, lz4).  Requires THEMIS_HAS_ARROW to be defined.
 */
class ParquetExporter : public IAnalyticsExporter {
  public:
    ParquetExporter() {
        spdlog::debug("ParquetExporter initialized");
    }
    ~ParquetExporter() override = default;

    ExportResult exportToFile(const ArrowRecordBatch &batch, const std::string &output_path,
                              [[maybe_unused]] const ExportOptions &options) override {
        auto start = std::chrono::high_resolution_clock::now();
        ExportResult result;
        result.status = ExportStatus::SUCCESS;

        try {
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                spdlog::error("Failed to convert to Arrow RecordBatch: {}", arrow_batch_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Arrow conversion failed: " + arrow_batch_result.status().ToString();
                return result;
            }
            auto arrow_batch = arrow_batch_result.ValueOrDie();

            auto outfile_result = arrow::io::FileOutputStream::Open(output_path);
            if (!outfile_result.ok()) {
                spdlog::error("Failed to open Parquet file: {}", outfile_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to open file: " + outfile_result.status().ToString();
                return result;
            }
            auto outfile = outfile_result.ValueOrDie();

            auto table = arrow::Table::Make(arrow_batch->schema(), arrow_batch->columns());

            parquet::WriterProperties::Builder props_builder;
            if (options.compress) {
                if (options.compression_codec == "snappy") {
                    props_builder.compression(parquet::Compression::SNAPPY);
                } else if (options.compression_codec == "gzip") {
                    props_builder.compression(parquet::Compression::GZIP);
                    props_builder.compression_level(options.compression_level);
                } else if (options.compression_codec == "zstd") {
                    props_builder.compression(parquet::Compression::ZSTD);
                    props_builder.compression_level(options.compression_level);
                } else if (options.compression_codec == "lz4") {
                    props_builder.compression(parquet::Compression::LZ4);
                } else {
                    props_builder.compression(parquet::Compression::SNAPPY);
                }
            }
            auto props = props_builder.build();

            auto write_status
                = parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, options.batch_size, props);

            if (!write_status.ok()) {
                spdlog::error("Failed to write Parquet file: {}", write_status.ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Parquet write failed: " + write_status.ToString();
                return result;
            }

            result.bytes_written = outfile->Tell().ValueOrDie();
            result.rows_exported = batch.rowCount();
            spdlog::info("Parquet export successful: {} rows, {} bytes", result.rows_exported, result.bytes_written);

        } catch (const std::exception &e) {
            spdlog::error("Parquet export failed: {}", e.what());
            result.status  = ExportStatus::FAILED;
            result.message = std::string("Parquet export failed: ") + e.what();
        }

        auto end           = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return result;
    }

    std::string exportToString(const ArrowRecordBatch & /*batch*/, const ExportOptions & /*options*/) override {
        // Parquet is a binary columnar format; exporting to a plain string is not
        // meaningful.  Throw to signal this clearly, consistent with the factory's
        // error-handling convention.  Use exportToFile() instead.
        throw std::runtime_error("Parquet is a binary columnar file format and cannot be meaningfully "
                                 "represented as a plain string. Use exportToFile() to write Parquet output "
                                 "to disk.");
    }

    ExportResult exportWithCallback(const ArrowRecordBatch & /*batch*/,
                                    std::function<void(const std::vector<uint8_t> &)> /*callback*/,
                                    const ExportOptions & /*options*/) override {
        ExportResult result;
        result.status  = ExportStatus::NOT_SUPPORTED;
        result.message = "Parquet streaming export via callback is not supported; use exportToFile().";
        return result;
    }

    bool supportsFormat(ExportFormat format) const override {
        return format == ExportFormat::FMT_ARROW_PARQUET;
    }

    std::string getExporterInfo() const override {
        return "ParquetExporter v1.0 (Apache Parquet columnar format)";
    }
};

/**
 * @brief Apache Feather (Arrow IPC file) exporter.
 *
 * Writes data in the Apache Feather v2 format (Arrow IPC file format).
 * Requires THEMIS_HAS_ARROW to be defined.
 */
class FeatherExporter : public IAnalyticsExporter {
  public:
    FeatherExporter() {
        spdlog::debug("FeatherExporter initialized");
    }
    ~FeatherExporter() override = default;

    ExportResult exportToFile(const ArrowRecordBatch &batch, const std::string &output_path,
                              [[maybe_unused]] const ExportOptions &options) override {
        auto start = std::chrono::high_resolution_clock::now();
        ExportResult result;
        result.status = ExportStatus::SUCCESS;

        try {
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                spdlog::error("Failed to convert to Arrow RecordBatch: {}", arrow_batch_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Arrow conversion failed: " + arrow_batch_result.status().ToString();
                return result;
            }
            auto arrow_batch = arrow_batch_result.ValueOrDie();

            auto outfile_result = arrow::io::FileOutputStream::Open(output_path);
            if (!outfile_result.ok()) {
                spdlog::error("Failed to open Feather file: {}", outfile_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to open file: " + outfile_result.status().ToString();
                return result;
            }
            auto outfile = outfile_result.ValueOrDie();

            auto writer_result = arrow::ipc::MakeFileWriter(outfile, arrow_batch->schema());
            if (!writer_result.ok()) {
                spdlog::error("Failed to create Feather writer: {}", writer_result.status().ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Failed to create Feather writer: " + writer_result.status().ToString();
                return result;
            }
            auto writer = writer_result.ValueOrDie();

            auto write_status = writer->WriteRecordBatch(*arrow_batch);
            if (!write_status.ok()) {
                spdlog::error("Failed to write Feather batch: {}", write_status.ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Write failed: " + write_status.ToString();
                return result;
            }

            auto close_status = writer->Close();
            if (!close_status.ok()) {
                spdlog::error("Failed to close Feather writer: {}", close_status.ToString());
                result.status  = ExportStatus::FAILED;
                result.message = "Close failed: " + close_status.ToString();
                return result;
            }

            result.bytes_written = outfile->Tell().ValueOrDie();
            result.rows_exported = batch.rowCount();
            spdlog::info("Feather export successful: {} rows, {} bytes", result.rows_exported, result.bytes_written);

        } catch (const std::exception &e) {
            spdlog::error("Feather export failed: {}", e.what());
            result.status  = ExportStatus::FAILED;
            result.message = std::string("Feather export failed: ") + e.what();
        }

        auto end           = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return result;
    }

    std::string exportToString(const ArrowRecordBatch &batch, [[maybe_unused]] const ExportOptions &options) override {
        // format is implicitly FMT_ARROW_FEATHER for this exporter
        try {
            auto arrow_batch_result = convertToArrowRecordBatch(batch);
            if (!arrow_batch_result.ok()) {
                spdlog::error("Failed to convert to Arrow RecordBatch for Feather string export");
                return "# ERROR: Failed to convert to Arrow format";
            }
            auto arrow_batch = arrow_batch_result.ValueOrDie();

            auto buffer_result = arrow::io::BufferOutputStream::Create();
            if (!buffer_result.ok()) {
                return "# ERROR: Failed to create buffer";
            }
            auto buffer = buffer_result.ValueOrDie();

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

            auto result_buffer = buffer->Finish();
            if (!result_buffer.ok()) {
                return "# ERROR: Failed to finalize buffer";
            }
            auto data = result_buffer.ValueOrDie();
            return std::string(reinterpret_cast<const char *>(data->data()), data->size());

        } catch (const std::exception &e) {
            spdlog::error("Feather string export failed: {}", e.what());
            return "# ERROR: " + std::string(e.what());
        }
    }

    ExportResult exportWithCallback(const ArrowRecordBatch &batch,
                                    std::function<void(const std::vector<uint8_t> &)> callback,
                                    const ExportOptions &options) override {
        auto start = std::chrono::high_resolution_clock::now();
        ExportResult result;
        result.status = ExportStatus::SUCCESS;

        try {
            std::string data  = exportToString(batch, options);
            size_t chunk_size = options.batch_size * 100;
            size_t offset     = 0;
            while (offset < data.size()) {
                size_t len = std::min(chunk_size, data.size() - offset);
                std::vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + len);
                callback([[maybe_unused]] chunk);
                offset += len;
            }
            result.rows_exported = batch.rowCount();
            result.bytes_written = data.size();
        } catch (const std::exception &e) {
            result.status  = ExportStatus::FAILED;
            result.message = std::string("Export failed: ") + e.what();
        }

        auto end           = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return result;
    }

    bool supportsFormat(ExportFormat format) const override {
        return format == ExportFormat::FMT_ARROW_FEATHER;
    }

    std::string getExporterInfo() const override {
        return "FeatherExporter v1.0 (Apache Feather / Arrow IPC file format)";
    }
};

#endif // THEMIS_HAS_ARROW

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

#ifndef THEMIS_HAS_ARROW
[[noreturn]] static void throwArrowUnavailable(const char *format_name) {
    spdlog::warn("ExporterFactory: {} export is not available – rebuild with -DTHEMIS_HAS_ARROW=ON", format_name);
    throw std::runtime_error(std::string(format_name)
                             + " export is not available. "
                               "Rebuild with -DTHEMIS_HAS_ARROW=ON and install Apache Arrow library.");
}
#endif

// Factory implementations
std::unique_ptr<IAnalyticsExporter> ExporterFactory::createExporter(ExportFormat format) {
    switch (format) {
        case ExportFormat::JSON:
        [[fallthrough]];\n        case ExportFormat::CSV:
            return std::make_unique<JSONCSVExporter>();

        case ExportFormat::FMT_ARROW_IPC:
#ifdef THEMIS_HAS_ARROW
            return std::make_unique<ArrowIPCExporter>();
#else
            throwArrowUnavailable("Arrow IPC");
#endif

        case ExportFormat::FMT_ARROW_PARQUET:
#ifdef THEMIS_HAS_ARROW
            return std::make_unique<ParquetExporter>();
#else
            throwArrowUnavailable("Parquet");
#endif

        case ExportFormat::FMT_ARROW_FEATHER:
#ifdef THEMIS_HAS_ARROW
            return std::make_unique<FeatherExporter>();
#else
            throwArrowUnavailable("Feather");
#endif
    }
    throw std::runtime_error("Unknown export format");
}

std::unique_ptr<IAnalyticsExporter> ExporterFactory::createDefaultExporter() {
    return std::make_unique<JSONCSVExporter>();
}

// ============================================================================
// IAnalyticsExporter — BoundedExecutionPolicy wrapper
// ============================================================================

ExportResult IAnalyticsExporter::exportToFile(
        const ArrowRecordBatch &batch,
        const std::string      &output_path,
        const ExportOptions    &options,
        const BoundedExecutionPolicy &policy) {

    // Resolve the effective policy: prefer the explicitly-passed policy when
    // it is constrained; fall back to the options-embedded default otherwise.
    const BoundedExecutionPolicy &effective_policy =
        policy.isConstrained() ? policy : options.policy;

    // Fast path: no limits declared in either policy source.
    if (!effective_policy.isConstrained()) {
        return exportToFile(batch, output_path, options);
    }

    // ── Concurrency enforcement ──────────────────────────────────────────────
    if (effective_policy.max_concurrent_requests > 0u) {
        uint32_t concurrent_snapshot = inflight_export_count_.load(std::memory_order_relaxed);
        while (true) {
            if (concurrent_snapshot >= effective_policy.max_concurrent_requests) {
                ExportResult result;
                result.status  = ExportStatus::POLICY_REJECTED;
                result.message = "BoundedExecutionPolicy: max_concurrent_requests ("
                                 + std::to_string(effective_policy.max_concurrent_requests)
                                 + ") exceeded (current=" + std::to_string(concurrent_snapshot) + ")";
                spdlog::warn("IAnalyticsExporter::exportToFile: export rejected by "
                             "BoundedExecutionPolicy (max_concurrent_requests={}, current={})",
                             effective_policy.max_concurrent_requests, concurrent_snapshot);
                return result;
            }
            if (inflight_export_count_.compare_exchange_weak(
                    concurrent_snapshot, concurrent_snapshot + 1u, std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                break;
            }
        }
    } else {
        inflight_export_count_.fetch_add(1u, std::memory_order_acq_rel);
    }

    // RAII guard for in-flight count.
    struct Guard {
        std::atomic<uint32_t> &counter;
        ~Guard() { counter.fetch_sub(1u, std::memory_order_acq_rel); }
    } guard{inflight_export_count_};

    // ── Timeout enforcement ──────────────────────────────────────────────────
    if (effective_policy.max_latency_ms > 0u) {
        auto fut = std::async(std::launch::async,
            [this, &batch, &output_path, &options]() -> ExportResult {
                return exportToFile(batch, output_path, options);
            });
        const auto deadline = std::chrono::milliseconds(effective_policy.max_latency_ms);
        if (fut.wait_for(deadline) == std::future_status::timeout) {
            ExportResult result;
            result.status  = ExportStatus::POLICY_REJECTED;
            result.message = "BoundedExecutionPolicy: export did not complete within "
                             + std::to_string(effective_policy.max_latency_ms) + " ms";
            spdlog::warn("IAnalyticsExporter::exportToFile: export timed out after {} ms "
                         "(policy deadline)", effective_policy.max_latency_ms);
            return result;
        }
        return fut.get();
    }

    return exportToFile(batch, output_path, options);
}

} // namespace analytics
} // namespace themis
