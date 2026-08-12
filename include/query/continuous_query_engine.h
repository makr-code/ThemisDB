/**
 * @file continuous_query_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/window_spec.h"
#include "query/continuous_query_registry.h"
#include "utils/expected.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace query {

// ──────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ──────────────────────────────────────────────────────────────────────────────
class ResultQueue;

/**
 * @brief A single result item produced by a continuous query tick.
 *
 * In DELTA/CHANGES mode the `is_retract` flag distinguishes additions (+)
 * from retractions (−). In SNAPSHOT mode all items have `is_retract == false`.
 */
struct CQResult {
    std::string payload;   ///< JSON-serialised tuple
    bool        is_retract{false};  ///< true → retraction (removal)
};

// ──────────────────────────────────────────────────────────────────────────────
// ResultStream for continuous queries
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Blocking iterator over a continuous-query result queue.
 *
 * Returned by ContinuousQueryEngine::subscribe().  next() blocks until a
 * result is available or the supplied timeout expires.  cancel() unblocks
 * any waiting next() call and makes hasMore() return false.
 */
class CQResultStream {
public:
    virtual ~CQResultStream() = default;

    /** @return true as long as the query is alive and no cancel() was called. */
    [[nodiscard]] virtual bool hasMore() const noexcept = 0;

    /**
     * @brief Fetch the next result item.
     * @param timeout  Maximum time to block.
     * @return The next CQResult, or an empty optional on timeout.
     */
    virtual std::optional<CQResult> next(
        std::chrono::milliseconds timeout = std::chrono::seconds(5)) = 0;

    /** @brief Cancel this subscription.  Unblocks any pending next(). */
    virtual void cancel() noexcept = 0;

    /** @brief Number of items buffered and not yet consumed. */
    [[nodiscard]] virtual size_t queueDepth() const noexcept = 0;
};

// ──────────────────────────────────────────────────────────────────────────────
// Specification for registering a continuous query
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Full specification for a continuous standing query.
 *
 * Passed to ContinuousQueryEngine::registerQuery().
 */
struct ContinuousQuerySpec {
    std::string name;               ///< Unique query name (used as DDL identifier)
    std::string source_collection;  ///< AQL collection to stream from
    WindowSpec  window;             ///< Window definition
    std::string aql_body;           ///< AQL expression evaluated each tick

    ResultMode result_mode{ResultMode::DELTA};

    int64_t allowed_lateness_ms{500};           ///< Late-event budget in ms
    size_t  max_window_tuples{10'000'000};       ///< Max tuples per window
    size_t  max_window_bytes{1ULL << 30};        ///< Max bytes per window (1 GiB)
};

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousQueryEngine — public interface
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Engine for standing (continuous) queries.
 *
 * Queries are registered once and evaluated continuously as new data arrives.
 * Results are pushed to subscribers via CQResultStream.
 *
 * Thread safety: all public methods are thread-safe.
 *
 * Error codes returned:
 *   ERR_QUERY_INVALID             – spec validation failed
 *   ERR_QUERY_INVALID_WINDOW_SPEC – window configuration invalid
 *   ERR_QUERY_EXECUTION_FAILED    – evaluation loop error
 */
class ContinuousQueryEngine {
public:
    using ContinuousQueryHandle = std::string;
    using ResultStreamPtr = std::shared_ptr<CQResultStream>;

    virtual ~ContinuousQueryEngine() = default;

    /**
     * @brief Register a new standing query.
     *
     * Validates the spec (bounded window, pure AQL body, existing source),
     * stores it in the registry, and starts the evaluation loop.
     *
     * @return the query name on success, or an Error.
     */
    [[nodiscard]] virtual Result<ContinuousQueryHandle> registerQuery(
        ContinuousQuerySpec spec) = 0;

    /**
     * @brief Drop a registered query.
     *
     * Drains the result queue, cancels the scheduler job, and releases
     * synopsis storage.
     */
    virtual Result<void> dropQuery(const std::string& name) = 0;

    /**
     * @brief Subscribe to a continuous query's result stream.
     *
     * Multiple subscribers per query are supported.  Each subscriber receives
     * an independent bounded result queue.
     *
     * @param name   Query name.
     * @param mode   ResultMode for this subscription (overrides spec default).
     */
    [[nodiscard]] virtual Result<ResultStreamPtr> subscribe(
        const std::string& name,
        ResultMode mode) = 0;

    /** @brief List all registered queries with runtime statistics. */
    [[nodiscard]] virtual std::vector<ContinuousQueryInfo> listQueries() const = 0;

    /**
     * @brief Inject a tuple into the evaluation loop (for testing / CDC feed).
     *
     * @param collection Source collection name.
     * @param tuple      JSON-serialised tuple.
     * @param event_ts   Event timestamp (microseconds since epoch).
     */
    virtual void injectTuple(const std::string& collection,
                             const std::string& tuple,
                             int64_t            event_ts) = 0;
};

}  // namespace query
}  // namespace themis
