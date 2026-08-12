/**
 * @file search_result_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 2.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready - v2.0.0 Contract Freeze (Phase 1)
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "search/hybrid_search.h"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Streaming result delivery for large result sets.
 *
 * SearchResultStream provides cursor-based pagination and callback-based
 * streaming over the result set produced by an underlying `HybridSearch`
 * engine.  This is the recommended delivery mechanism when `k` is large
 * (e.g. ≥ 1,000 results) and the caller wishes to avoid materialising the
 * full result vector in memory at once.
 *
 * ### Pagination model
 *
 * The stream fetches a window of `Config::page_size` results at a time.
 * Callers use `nextPage()` to advance the cursor and retrieve the next batch,
 * or `forEachResult()` to iterate all results via a callback.
 *
 * ### Cursor semantics
 *
 * - `open(query)` issues the initial `HybridSearch::search()` call (with
 *   `k = Config::total_k`) and stores the complete ordered result set
 *   internally.  The cursor is reset to position 0.
 * - `nextPage()` returns up to `Config::page_size` results starting at the
 *   current cursor position and advances the cursor.
 * - `hasMore()` returns true while the cursor has not reached the end of
 *   the result set.
 * - `reset()` rewinds the cursor to position 0 without re-issuing the query.
 * - `close()` clears the buffered results and resets the cursor.
 *
 * ### Callback streaming
 *
 * `forEachResult()` delivers each result to a `ResultCallback` one by one,
 * restarting from the current cursor position.  Early termination is
 * supported: returning `false` from the callback stops iteration.
 *
 * ### Typical usage
 * ```cpp
 * SearchResultStream::Config cfg;
 * cfg.total_k    = 10000;
 * cfg.page_size  = 100;
 * SearchResultStream stream(&hybrid_search, cfg);
 *
 * stream.open("machine learning");
 * while (stream.hasMore()) {
 *     auto page = stream.nextPage();
 *     for (const auto& r : page) { process(r); }
 * }
 * ```
 *
 * @note Thread Safety: A single SearchResultStream instance is NOT
 *   thread-safe.  Concurrent calls must be serialised externally.
 * @note Exception Safety: `open()` and `nextPage()` never throw.
 *   The constructor throws `std::invalid_argument` on invalid config.
 *
 * @since v2.4.0 (Streaming Result Delivery)
 */
class SearchResultStream {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Engine configuration.
     */
    struct Config {
        /// Total maximum results to materialise from HybridSearch.
        size_t total_k = 1000;

        /// Results per page returned by `nextPage()`.
        size_t page_size = 100;
        
        /// Timeout in milliseconds for open() call. 0 = no timeout.
        uint32_t open_timeout_ms = 30'000;  // 30 second default
    };

    /**
     * @brief Callback type for `forEachResult()`.
     *
     * Receives a const reference to each result.  Return `true` to continue
     * iteration, `false` to stop early.
     */
    using ResultCallback = std::function<bool(const HybridSearch::Result&)>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a SearchResultStream engine.
     *
     * @param hybrid_search  Non-owning pointer to the underlying HybridSearch.
     *                       May be null; `open()` will return an empty stream.
     * @throws std::invalid_argument on invalid config values.
     */
    explicit SearchResultStream(HybridSearch* hybrid_search);
    /**
     * @brief Construct a SearchResultStream engine.
     *
     * @param hybrid_search  Non-owning pointer to the underlying HybridSearch.
     *                       May be null; `open()` will return an empty stream.
     * @param config         Engine configuration.
     * @throws std::invalid_argument on invalid config values.
     */
    SearchResultStream(HybridSearch* hybrid_search, const Config& config);

    // -----------------------------------------------------------------------
    // Stream lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Open the stream for a new query.
     *
     * Executes `HybridSearch::search()` with `k = Config::total_k`, stores
     * the full result vector internally, and resets the cursor to position 0.
     * Any previously open stream is discarded.
     *
     * Never throws; index failures result in an empty stream.
     *
     * @param query  Full-text query string.
     * @param vector_query  Optional semantic embedding vector.
     */
    void open(const std::string& query,
              const std::vector<float>& vector_query = {});

    /**
     * @brief Advance the cursor and return the next page of results.
     *
     * Returns up to `Config::page_size` results starting at the current
     * cursor position, then advances the cursor by the number of results
     * returned.  Returns an empty vector when `hasMore()` is false.
     */
    std::vector<HybridSearch::Result> nextPage();

    /**
     * @brief Return true when there are more results beyond the current cursor.
     */
    bool hasMore() const;

    /**
     * @brief Rewind the cursor to position 0 without re-issuing the query.
     */
    void reset();

    /**
     * @brief Clear the buffered results and reset the cursor.
     *
     * After calling `close()`, `hasMore()` returns false until `open()` is
     * called again.
     */
    void close();

    // -----------------------------------------------------------------------
    // Callback streaming
    // -----------------------------------------------------------------------

    /**
     * @brief Deliver each result to @p callback starting from the current
     *        cursor position.
     *
     * Advances the cursor to the end of the result set (or stops early when
     * the callback returns `false`).  Never throws; callback exceptions are
     * caught and iteration is stopped.
     *
     * @param callback  Invoked for each result; return `false` to stop early.
     */
    void forEachResult(ResultCallback callback);

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /** @brief Total results in the current stream (0 when not open). */
    size_t totalResults() const { return results_.size(); }

    /** @brief Current cursor position. */
    size_t cursorPosition() const { return cursor_; }

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config);

private:
    HybridSearch* hybrid_search_;
    Config config_;
    std::vector<HybridSearch::Result> results_;
    size_t cursor_ = 0;
};

} // namespace themis
