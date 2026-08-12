/**
 * @file arrow_export.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "analytics/arrow_export.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace themis {
namespace analytics {

void ArrowRecordBatch::appendRow(const std::vector<std::variant<
    std::nullptr_t, int64_t, double, std::string, bool>>& row_data) {
    
    if (row_data.size() != columns_.size()) {
        throw std::runtime_error(
            "Row data size (" + std::to_string(row_data.size()) + 
            ") does not match column count (" + std::to_string(columns_.size()) + ")");
    }

    for (size_t i = 0; i < row_data.size(); ++i) {
        columns_[i].data.push_back(row_data[i]);
        
        // Track null values
        bool is_null = std::holds_alternative<std::nullptr_t>(row_data[i]);
        columns_[i].null_bitmap.push_back(is_null);

        // Populate typed contiguous buffers for zero-copy Arrow access
        switch (columns_[i].schema.type) {
            case DataType::INT64:
            case DataType::TIMESTAMP:
                columns_[i].int64_buffer.push_back(
                    is_null ? int64_t(0) : std::get<int64_t>(row_data[i]));
                break;
            case DataType::DOUBLE:
                columns_[i].double_buffer.push_back(
                    is_null ? 0.0 : std::get<double>(row_data[i]));
                break;
            default:
                break;
        }
    }

    ++row_count_;
}

const int64_t* ArrowRecordBatch::getInt64Data(size_t col_idx) const {
    const auto& col = columns_.at(col_idx);
    if (col.int64_buffer.empty()) {
        return nullptr;
    }
    return col.int64_buffer.data();
}

const double* ArrowRecordBatch::getDoubleData(size_t col_idx) const {
    const auto& col = columns_.at(col_idx);
    if (col.double_buffer.empty()) {
        return nullptr;
    }
    return col.double_buffer.data();
}

std::string ArrowRecordBatch::toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"metadata\": {\n";
    oss << "    \"row_count\": " << row_count_ << ",\n";
    oss << "    \"column_count\": " << columns_.size() << "\n";
    oss << "  },\n";
    oss << "  \"schema\": [\n";
    
    for (size_t i = 0; i < columns_.size(); ++i) {
        const auto& col = columns_[i];
        oss << "    {\n";
        oss << "      \"name\": \"" << col.schema.name << "\",\n";
        oss << "      \"type\": \"";
        
        switch (col.schema.type) {
            case DataType::INT64:
                oss << "INT64";
                break;
            case DataType::DOUBLE:
                oss << "DOUBLE";
                break;
            case DataType::STRING:
                oss << "STRING";
                break;
            case DataType::BOOLEAN:
                oss << "BOOLEAN";
                break;
            case DataType::TIMESTAMP:
                oss << "TIMESTAMP";
                break;
        }
        
        oss << "\",\n";
        oss << "      \"nullable\": " << (col.schema.nullable ? "true" : "false") << "\n";
        oss << "    }";
        
        if (i < columns_.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }
    
    oss << "  ],\n";
    oss << "  \"data\": [\n";
    
    // Export first 10 rows as sample
    size_t sample_rows = std::min(row_count_, size_t(10));
    for (size_t row = 0; row < sample_rows; ++row) {
        oss << "    {";
        
        for (size_t col = 0; col < columns_.size(); ++col) {
            const auto& column = columns_[col];
            oss << "\"" << column.schema.name << "\": ";
            
            if (column.null_bitmap[row]) {
                oss << "null";
            } else {
                const auto& value = column.data[row];
                
                if (std::holds_alternative<int64_t>(value)) {
                    oss << std::get<int64_t>(value);
                } else if (std::holds_alternative<double>(value)) {
                    oss << std::fixed << std::setprecision(2) << std::get<double>(value);
                } else if (std::holds_alternative<std::string>(value)) {
                    oss << "\"" << std::get<std::string>(value) << "\"";
                } else if (std::holds_alternative<bool>(value)) {
                    oss << (std::get<bool>(value) ? "true" : "false");
                }
            }
            
            if (col < columns_.size() - 1) {
                oss << ", ";
            }
        }
        
        oss << "}";
        if (row < sample_rows - 1) {
            oss << ",";
        }
        oss << "\n";
    }
    
    if (row_count_ > 10) {
        oss << "    // ... " << (row_count_ - 10) << " more rows\n";
    }
    
    oss << "  ]\n";
    oss << "}\n";
    
    return oss.str();
}

ArrowRecordBatch::Metadata ArrowRecordBatch::getMetadata() const {
    Metadata meta;
    meta.row_count = row_count_;
    meta.column_count = columns_.size();
    
    // Calculate approximate size
    size_t total_bytes = 0;
    for (const auto& col : columns_) {
        // Approximate size calculation
        total_bytes += col.data.size() * 8;  // Rough estimate (variant storage)
        total_bytes += col.null_bitmap.size() / 8;  // Bitmap size
        total_bytes += col.int64_buffer.size() * sizeof(int64_t);  // Zero-copy int64 buffer
        total_bytes += col.double_buffer.size() * sizeof(double);  // Zero-copy double buffer
    }
    meta.total_bytes = total_bytes;
    
    // Copy schema
    for (const auto& col : columns_) {
        meta.schema.push_back(col.schema);
    }
    
    return meta;
}

} // namespace analytics
} // namespace themis
