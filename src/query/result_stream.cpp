/**
 * @file result_stream.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/result_stream.h"
#include "utils/error_registry.h"
#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>
#include <nlohmann/json.hpp>

namespace themis {
namespace query {

// ============================================================================
// ResultStream<T> Implementation
// ============================================================================

template<typename T>
ResultStream<T>::ResultStream(
    std::function<Result<std::vector<T>>(const PaginationCursor&, size_t)> source,
    StreamConfig config,
    PaginationStrategy strategy
)
    : source_(std::move(source))
    , config_(config)
    , strategy_(strategy)
    , is_materialized_(false)
{
    cursor_.offset = 0;
    cursor_.has_more = true;
}

template<typename T>
ResultStream<T>::ResultStream(
    std::vector<T> data,
    StreamConfig config
)
    : source_(nullptr)
    , config_(config)
    , strategy_(PaginationStrategy::OFFSET_LIMIT)
    , is_materialized_(true)
    , materialized_data_(std::move(data))
{
    cursor_.offset = 0;
    cursor_.has_more = !materialized_data_.empty();
}

template<typename T>
bool ResultStream<T>::hasNext() const {
    if (is_materialized_) {
        return cursor_.offset < materialized_data_.size();
    }
    
    // Check if we have buffered data
    if (buffer_pos_ < buffer_.size()) {
        return true;
    }
    
    // Check if more data might be available from source
    return cursor_.has_more;
}

template<typename T>
Result<T> ResultStream<T>::next() {
    if (!hasNext()) {
        return Err<T>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, 
                     "No more results available");
    }
    
    // Handle materialized data
    if (is_materialized_) {
        T item = materialized_data_[cursor_.offset];
        cursor_.offset++;
        cursor_.has_more = cursor_.offset < materialized_data_.size();
        stats_.items_read++;
        return item;
    }
    
    // Handle streaming mode
    if (buffer_pos_ >= buffer_.size()) {
        // Need to fill buffer
        auto fill_result = fillBuffer();
        if (!fill_result) {
            return Err<T>(fill_result.error().code(), fill_result.error().context());
        }
        
        if (buffer_.empty()) {
            return Err<T>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                         "No more results available");
        }
    }
    
    T item = buffer_[buffer_pos_];
    buffer_pos_++;
    stats_.items_read++;
    
    // Update cursor for pagination
    updateCursor(item);
    
    return item;
}

template<typename T>
Result<ResultBatch<T>> ResultStream<T>::nextBatch(size_t batch_size) {
    ResultBatch<T> batch;
    batch.items.reserve(batch_size);
    
    size_t fetched = 0;
    while (fetched < batch_size && hasNext()) {
        auto result = next();
        if (!result) {
            if (fetched > 0) {
                // Return partial batch
                break;
            }
            return Err<ResultBatch<T>>(result.error().code(), result.error().context());
        }
        
        batch.items.push_back(*result);
        fetched++;
    }
    
    if (fetched == 0) {
        return Err<ResultBatch<T>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                                   "No more results available");
    }

    batch.cursor = cursor_;
    batch.is_last_batch = !hasNext();
    
    return batch;
}

template<typename T>
void ResultStream<T>::reset() {
    buffer_.clear();
    buffer_pos_ = 0;
    cursor_.offset = 0;
    cursor_.last_key.clear();
    cursor_.last_value.reset();
    cursor_.has_more = true;
    stats_ = Statistics();
    
    if (is_materialized_) {
        // [W9-10-FIX: memory_leak — result_stream.cpp:156]
        // RAII enforcement: materialized_data_ is std::vector<T> (value type).
        // No raw pointer or heap allocation is made here; the vector destructs
        // automatically when the ResultStream goes out of scope.  Any future
        // extension that adds a raw T* field MUST wrap it in std::unique_ptr<T>
        // or equivalent RAII handle before this reset() site is reached.
        cursor_.has_more = !materialized_data_.empty();
    }
}

template<typename T>
size_t ResultStream<T>::position() const {
    if (is_materialized_) {
        return cursor_.offset;
    }
    return stats_.items_read;
}

template<typename T>
Result<void> ResultStream<T>::skip(size_t count) {
    if (count == 0) {
        return OkVoid();
    }
    
    if (is_materialized_) {
        // Check for potential overflow before addition
        if (count > std::numeric_limits<size_t>::max() - cursor_.offset) {
            // Would overflow - just set to end
            cursor_.offset = materialized_data_.size();
            cursor_.has_more = false;
            return OkVoid();
        }
        
        size_t new_offset = cursor_.offset + count;
        if (new_offset >= materialized_data_.size()) {
            cursor_.offset = materialized_data_.size();
            cursor_.has_more = false;
        } else {
            cursor_.offset = new_offset;
            cursor_.has_more = true;
        }
        return OkVoid();
    }
    
    // For streaming mode, we need to actually consume the items
    // This is inefficient for large skips - consider implementing
    // a more efficient skip in the data source
    for (size_t i = 0; i < count; ++i) {
        auto result = next();
        if (!result) {
            if (i > 0) {
                // Partial skip succeeded
                return OkVoid();
            }
            return ErrVoid(result.error().code(), result.error().context());
        }
    }
    
    return OkVoid();
}

template<typename T>
void ResultStream<T>::setCursor(const PaginationCursor& cursor) {
    cursor_ = cursor;
    buffer_.clear();
    buffer_pos_ = 0;
}

template<typename T>
Result<void> ResultStream<T>::fillBuffer() {
    if (!source_) {
        return ErrVoid(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                      "No data source available");
    }
    
    stats_.buffer_misses++;
    
    // Fetch next batch from source
    auto result = source_(cursor_, config_.batch_size);
    if (!result) {
        return ErrVoid(result.error().code(), result.error().context());
    }
    
    buffer_ = std::move(*result);
    buffer_pos_ = 0;
    stats_.batches_fetched++;
    
    if (buffer_.empty()) {
        cursor_.has_more = false;
        return OkVoid();
    }
    
    // Check buffer size and apply backpressure if needed
    if (config_.enable_backpressure && 
        buffer_.size() >= config_.backpressure_threshold) {
        stats_.backpressure_active = true;
    }
    
    return OkVoid();
}

template<typename T>
bool ResultStream<T>::shouldFillBuffer() const {
    return buffer_pos_ >= buffer_.size() && cursor_.has_more;
}

template<typename T>
void ResultStream<T>::updateCursor(const T& item) {
    cursor_.offset++;

    // For cursor-based and keyset pagination, update the last seen key/value
    // so that data sources can resume from the correct position on the next
    // fillBuffer() call.
    if (strategy_ == PaginationStrategy::CURSOR_BASED ||
        strategy_ == PaginationStrategy::KEYSET) {

        if constexpr (std::is_same_v<T, std::string>) {
            // The item itself is the primary key
            cursor_.last_key = item;
            if (strategy_ == PaginationStrategy::KEYSET) {
                cursor_.last_value = item;
            }
        } else if constexpr (std::is_same_v<T, nlohmann::json>) {
            // Extract the primary key from well-known JSON key fields
            if (item.is_object()) {
                static constexpr std::array<const char*, 3> key_fields = {"_id", "id", "key"};
                for (const auto* key_field : key_fields) {
                    if (item.contains(key_field)) {
                        const auto& key_value = item[key_field];
                        cursor_.last_key = key_value.is_string()
                            ? key_value.template get<std::string>()
                            : key_value.dump();
                        if (strategy_ == PaginationStrategy::KEYSET) {
                            cursor_.last_value = cursor_.last_key;
                        }
                        break;
                    }
                }
            }
        }
        // For other numeric/scalar types the offset-based cursor is sufficient.
    }

    if (buffer_pos_ >= buffer_.size()) {
        // Reached end of current buffer
        if (buffer_.empty()) {
            cursor_.has_more = false;
        }
    }
}

// ============================================================================
// Explicit Template Instantiations
// ============================================================================

// Instantiate for common types used in QueryEngine
template class ResultStream<std::string>;
template class ResultStream<int>;
template class ResultStream<double>;

// Instantiate for nlohmann::json (commonly used for query results)
template class ResultStream<nlohmann::json>;

// Helper function implementation
std::shared_ptr<ResultStream<std::string>> createKeyStream(
    std::vector<std::string> keys,
    StreamConfig config
) {
    return std::make_shared<ResultStream<std::string>>(std::move(keys), config);
}

} // namespace query
} // namespace themis

