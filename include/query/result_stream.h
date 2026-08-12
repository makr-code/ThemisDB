/**
 * @file result_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Configuration for result streaming and pagination
 */
struct StreamConfig {
    size_t batch_size = 100;              // Number of results per batch
    size_t max_buffer_size = 1000;        // Maximum buffered results
    bool enable_backpressure = true;       // Enable backpressure handling
    size_t backpressure_threshold = 800;   // Threshold for backpressure (80% of max_buffer)
};

/**
 * @brief Pagination cursor for efficient result navigation
 */
struct PaginationCursor {
    std::string last_key;                  // Last primary key returned
    std::optional<std::string> last_value; // Last sort column value (for keyset pagination)
    size_t offset = 0;                     // Current offset (for offset/limit pagination)
    bool has_more = true;                  // Whether more results are available
};

/**
 * @brief Pagination strategies
 */
enum class PaginationStrategy {
    OFFSET_LIMIT,    // Simple offset/limit (inefficient for large offsets)
    CURSOR_BASED,    // Cursor-based using last key
    KEYSET           // Keyset pagination using sort column values
};

/**
 * @brief Batch of results with metadata
 */
template<typename T>
struct ResultBatch {
    std::vector<T> items;                  // Batch items
    PaginationCursor cursor;               // Cursor for next batch
    size_t total_count = 0;                // Total count (if known)
    bool is_last_batch = false;            // Whether this is the last batch
};

/**
 * @brief Iterator interface for streaming query results
 * 
 * Provides lazy evaluation of query results with support for:
 * - Chunked result transmission
 * - Multiple pagination strategies
 * - Backpressure handling
 * - Memory-efficient large result sets
 * 
 * @note ResultIterator instances are not thread-safe. Each iterator should
 *       be used by a single thread. For concurrent access, create separate
 *       iterators or use external synchronization.
 */
template<typename T>
class ResultIterator {
public:
    virtual ~ResultIterator() = default;
    
    /**
     * @brief Check if more results are available
     */
    [[nodiscard]] virtual bool hasNext() const = 0;
    
    /**
     * @brief Get next result
     * @return Result<T> containing next item or error
     */
    [[nodiscard]] virtual Result<T> next() = 0;
    
    /**
     * @brief Get next batch of results
     * @param batch_size Size of batch to retrieve
     * @return Result<ResultBatch<T>> containing batch or error
     */
    [[nodiscard]] virtual Result<ResultBatch<T>> nextBatch(size_t batch_size) = 0;
    
    /**
     * @brief Reset iterator to beginning
     */
    virtual void reset() = 0;
    
    /**
     * @brief Get current position/offset
     */
    [[nodiscard]] virtual size_t position() const = 0;
    
    /**
     * @brief Skip ahead by count items
     */
    [[nodiscard]] virtual Result<void> skip(size_t count) = 0;
};

/**
 * @brief Stream results from a data source with pagination support
 * 
 * Implements efficient streaming with:
 * - Configurable batch sizes
 * - Multiple pagination strategies
 * - Buffer management
 * - Backpressure handling
 */
template<typename T>
class ResultStream : public ResultIterator<T> {
public:
    ~ResultStream() override = default;
    /**
     * @brief Construct a ResultStream from a data source function
     * 
     * @param source Function that produces batches of results
     * @param config Stream configuration
     * @param strategy Pagination strategy to use
     */
    ResultStream(
        std::function<Result<std::vector<T>>(const PaginationCursor&, size_t)> source,
        StreamConfig config = StreamConfig(),
        PaginationStrategy strategy = PaginationStrategy::CURSOR_BASED
    );
    
    /**
     * @brief Construct a ResultStream from a pre-materialized result set
     * 
     * @param data Complete result set
     * @param config Stream configuration
     */
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
    
    /**
     * @brief Get current pagination cursor
     */
    PaginationCursor cursor() const { return cursor_; }
    
    /**
     * @brief Set pagination cursor (for resuming from saved position)
     */
    void setCursor(const PaginationCursor& cursor);
    
    /**
     * @brief Get stream statistics
     */
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
    Result<void> fillBuffer();
    bool shouldFillBuffer() const;
    void updateCursor(const T& item);
};

/**
 * @brief Helper to create a ResultStream from a vector of keys
 * 
 * @param keys Vector of primary keys
 * @param config Stream configuration
 * @return Shared pointer to ResultStream<std::string>
 */
std::shared_ptr<ResultStream<std::string>> createKeyStream(
    std::vector<std::string> keys,
    StreamConfig config = StreamConfig()
);

/**
 * @brief Helper to create a ResultStream with a custom data source
 * 
 * @param source Function that fetches data batches
 * @param config Stream configuration
 * @param strategy Pagination strategy
 * @return Shared pointer to ResultStream<T>
 */
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
