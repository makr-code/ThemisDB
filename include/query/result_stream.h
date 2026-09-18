/**
 * @file result_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <functional>
#include <cstddef>
#include "utils/expected.h"

namespace themis {
namespace query {

struct StreamConfig {
    size_t batch_size = 100;              // Number of results per batch
    size_t max_buffer_size = 1000;        // Maximum buffered results
    bool enable_backpressure = true;       // Enable backpressure handling
    size_t backpressure_threshold = 800;   // Threshold for backpressure (80% of max_buffer)
};

struct PaginationCursor {
    std::string last_key;                  // Last primary key returned
    std::optional<std::string> last_value; // Last sort column value (for keyset pagination)
    size_t offset = 0;                     // Current offset (for offset/limit pagination)
    bool has_more = true;                  // Whether more results are available
};

enum class PaginationStrategy {
    OFFSET_LIMIT,    // Simple offset/limit (inefficient for large offsets)
    CURSOR_BASED,    // Cursor-based using last key
    KEYSET           // Keyset pagination using sort column values
};

template<typename T>
struct ResultBatch {
    std::vector<T> items;                  // Batch items
    PaginationCursor cursor;               // Cursor for next batch
    size_t total_count = 0;                // Total count (if known)
    bool is_last_batch = false;            // Whether this is the last batch
};

template<typename T>
class ResultIterator {
public:
    /**
     * @brief Result Iterator.
     * @return Return value.
     */
    virtual ~ResultIterator() = default;
    
    [[nodiscard]] virtual bool hasNext() const = 0;
    
    [[nodiscard]] virtual Result<T> next() = 0;
    
    [[nodiscard]] virtual Result<ResultBatch<T>> nextBatch(size_t batch_size) = 0;
    
    /**
     * @brief Reset the modification detection flag.
     */
    virtual void reset() = 0;
    
    [[nodiscard]] virtual size_t position() const = 0;
    
    [[nodiscard]] virtual Result<void> skip(size_t count) = 0;
};

template<typename T>
class ResultStream : public ResultIterator<T> {
public:
    ~ResultStream() override = default;
    ResultStream(
        std::function<Result<std::vector<T>>(const PaginationCursor&, size_t)> source,
        StreamConfig config = StreamConfig(),
        PaginationStrategy strategy = PaginationStrategy::CURSOR_BASED
    );
    
    explicit ResultStream(
        std::vector<T> data,
        StreamConfig config = StreamConfig()
    );
    
    // ResultIterator interface implementation
    bool hasNext() const override;
    Result<T> next() override;
    Result<ResultBatch<T>> nextBatch(size_t batch_size) override;
    void reset() override;
    size_t position() const override;
    Result<void> skip(size_t count) override;
    
    PaginationCursor cursor() const { return cursor_; }
    
    /**
     * @brief Set Cursor.
     * @param[in] cursor Input parameter.
     */
    void setCursor(const PaginationCursor& cursor);
    
    struct Statistics {
        size_t items_read = 0;
        size_t batches_fetched = 0;
        size_t buffer_hits = 0;
        size_t buffer_misses = 0;
        bool backpressure_active = false;
    };
    Statistics statistics() const { return stats_; }
    
private:
    // Data source function (nullptr for materialized mode)
    std::function<Result<std::vector<T>>(const PaginationCursor&, size_t)> source_;
    
    // Configuration
    StreamConfig config_;
    PaginationStrategy strategy_;
    
    // State
    std::vector<T> buffer_;               // Buffered results
    size_t buffer_pos_ = 0;               // Current position in buffer
    PaginationCursor cursor_;             // Current pagination cursor
    bool is_materialized_ = false;        // Whether using pre-materialized data
    std::vector<T> materialized_data_;    // Pre-materialized data
    
    // Statistics
    Statistics stats_;
    
    // Helper methods
    /**
     * @brief Fill Buffer.
     * @return Return value.
     */
    Result<void> fillBuffer();
    /**
     * @brief Should Fill Buffer.
     * @return True when the operation succeeds.
     */
    bool shouldFillBuffer() const;
    /**
     * @brief Update Cursor.
     * @param[in] item Input parameter.
     */
    void updateCursor(const T& item);
};

std::shared_ptr<ResultStream<std::string>> createKeyStream(
    std::vector<std::string> keys,
    StreamConfig config = StreamConfig()
);

template<typename T>
std::shared_ptr<ResultStream<T>> createStream(
    std::function<Result<std::vector<T>>(const PaginationCursor&, size_t)> source,
    StreamConfig config = StreamConfig(),
    PaginationStrategy strategy = PaginationStrategy::CURSOR_BASED
) {
    return std::make_shared<ResultStream<T>>(source, config, strategy);
}

} // namespace query
} // namespace themis
