/**
 * @file arrow_export.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ArrowRecordBatch {
public:
    enum class DataType {
        INT64,
        DOUBLE,
        STRING,
        BOOLEAN,
        TIMESTAMP
    };

    struct ColumnSchema {
        std::string name;
        DataType type;
        bool nullable = true;
    };

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
     * @brief Add Column.
     * @param[in] schema Input parameter.
     * @details Calls: push_back(), std::move().
     */
    void addColumn(const ColumnSchema& schema) {
        Column col;
        col.schema = schema;
        columns_.push_back(std::move(col));
    }

    void appendRow(const std::vector<std::variant<
        std::nullptr_t, int64_t, double, std::string, bool>>& row_data);

    size_t rowCount() const { return row_count_; }

    size_t columnCount() const { return columns_.size(); }

    const Column& getColumn(size_t index) const {
        return columns_.at(index);
    }

    const std::vector<Column>& getColumns() const {
        return columns_;
    }

    /**
     * @brief Clear.
     * @details Implements clear without additional internal calls.
     */
    void clear() {
        columns_.clear();
        row_count_ = 0;
    }

    /**
     * @brief Get Int64 Data.
     * @param[in] col_idx Input parameter.
     * @return Pointer to the result.
     */
    const int64_t* getInt64Data(size_t col_idx) const;

    /**
     * @brief Get Double Data.
     * @param[in] col_idx Input parameter.
     * @return Pointer to the result.
     */
    const double* getDoubleData(size_t col_idx) const;

    /**
     * @brief To JSON.
     * @return Return value.
     */
    std::string toJSON() const;

    struct Metadata {
        size_t row_count = 0;
        size_t column_count;
        size_t total_bytes;
        std::vector<ColumnSchema> schema;
    };

    /**
     * @brief Get Metadata.
     * @return Return value.
     */
    Metadata getMetadata() const;

private:
    std::vector<Column> columns_;
    size_t row_count_ = 0;
};

} // namespace analytics
} // namespace themis
