/**
 * @file arrow_export.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <cstdint>
#include <optional>

namespace themis {
namespace analytics {

/**
 * @brief Columnar record batch with zero-copy Arrow export support.
 *
 * Stores records in a columnar format compatible with Apache Arrow's RecordBatch.
 * Each numeric column (INT64, DOUBLE, TIMESTAMP) maintains a contiguous typed
 * buffer alongside the variant storage, enabling zero-copy transfer to Apache
 * Arrow arrays via arrow::Buffer::Wrap() when THEMIS_HAS_ARROW is enabled.
 *
 * When THEMIS_HAS_ARROW is not enabled the class still provides full JSON/CSV
 * export and serves as the in-process columnar representation.
 */
class ArrowRecordBatch {
public:
    /**
     * @brief Supported data types for columns
     */
    enum class DataType {
        INT64,
        DOUBLE,
        STRING,
        BOOLEAN,
        TIMESTAMP
    };

    /**
     * @brief Column schema definition
     */
    struct ColumnSchema {
        std::string name;
        DataType type;
        bool nullable = true;
    };

    /**
     * @brief Column data container
     */
    struct Column {
        ColumnSchema schema;
        std::vector<std::variant<
            std::nullptr_t,
            int64_t,
            double,
            std::string,
            bool
        >> data;
        std::vector<bool> null_bitmap;  // Track null values (true = null)

        // Typed contiguous buffers for zero-copy Arrow integration.
        // Populated for INT64/TIMESTAMP and DOUBLE columns respectively.
        // Null rows store 0 as a placeholder; consult null_bitmap for validity.
        std::vector<int64_t> int64_buffer;
        std::vector<double>  double_buffer;
    };

    ArrowRecordBatch() = default;
    ~ArrowRecordBatch() = default;

    /**
     * @brief Add a column to the record batch
     */
    void addColumn(const ColumnSchema& schema) {
        Column col;
        col.schema = schema;
        columns_.push_back(std::move(col));
    }

    /**
     * @brief Append a row of data
     * @param row_data Vector of values for each column
     */
    void appendRow(const std::vector<std::variant<
        std::nullptr_t, int64_t, double, std::string, bool>>& row_data);

    /**
     * @brief Get number of rows
     */
    size_t rowCount() const { return row_count_; }

    /**
     * @brief Get number of columns
     */
    size_t columnCount() const { return columns_.size(); }

    /**
     * @brief Get column by index
     */
    const Column& getColumn(size_t index) const {
        return columns_.at(index);
    }

    /**
     * @brief Get all columns
     */
    const std::vector<Column>& getColumns() const {
        return columns_;
    }

    /**
     * @brief Clear all data
     */
    void clear() {
        columns_.clear();
        row_count_ = 0;
    }

    /**
     * @brief Zero-copy access to INT64/TIMESTAMP column data.
     *
     * Returns a pointer to the contiguous int64_buffer of the column at
     * @p col_idx, enabling Apache Arrow's Buffer::Wrap to wrap the memory
     * without copying.  Only valid for INT64 and TIMESTAMP columns.
     *
     * @param col_idx Column index (0-based)
     * @return Raw pointer into the contiguous int64 buffer, or nullptr if
     *         the column type does not have an int64 buffer.
     */
    const int64_t* getInt64Data(size_t col_idx) const;

    /**
     * @brief Zero-copy access to DOUBLE column data.
     *
     * Returns a pointer to the contiguous double_buffer of the column at
     * @p col_idx, enabling Apache Arrow's Buffer::Wrap to wrap the memory
     * without copying.  Only valid for DOUBLE columns.
     *
     * @param col_idx Column index (0-based)
     * @return Raw pointer into the contiguous double buffer, or nullptr if
     *         the column type does not have a double buffer.
     */
    const double* getDoubleData(size_t col_idx) const;

    /**
     * @brief Export to JSON string (for testing/debugging)
     */
    std::string toJSON() const;

    /**
     * @brief Export metadata
     */
    struct Metadata {
        size_t row_count = 0;
        size_t column_count;
        size_t total_bytes;
        std::vector<ColumnSchema> schema;
    };

    Metadata getMetadata() const;

private:
    std::vector<Column> columns_;
    size_t row_count_ = 0;
};

} // namespace analytics
} // namespace themis
