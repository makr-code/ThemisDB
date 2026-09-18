/**
 * @file continuous_query_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct CQResult {
    std::string payload;   ///< JSON-serialised tuple
    bool        is_retract{false};  ///< true → retraction (removal)
};

// ──────────────────────────────────────────────────────────────────────────────
// ResultStream for continuous queries
// ──────────────────────────────────────────────────────────────────────────────

class CQResultStream {
public:
    /**
     * @brief CQResult Stream.
     * @return Return value.
     */
    virtual ~CQResultStream() = default;

    [[nodiscard]] virtual bool hasMore() const noexcept = 0;

    virtual std::optional<CQResult> next(
        std::chrono::milliseconds timeout = std::chrono::seconds(5)) = 0;

    /**
     * @brief Cancel.
     * @note Exception safety: noexcept.
     */
    virtual void cancel() noexcept = 0;

    [[nodiscard]] virtual size_t queueDepth() const noexcept = 0;
};

// ──────────────────────────────────────────────────────────────────────────────
// Specification for registering a continuous query
// ──────────────────────────────────────────────────────────────────────────────

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

class ContinuousQueryEngine {
public:
    using ContinuousQueryHandle = std::string;
    using ResultStreamPtr = std::shared_ptr<CQResultStream>;

    /**
     * @brief Continuous Query Engine.
     * @return Return value.
     */
    virtual ~ContinuousQueryEngine() = default;

    [[nodiscard]] virtual Result<ContinuousQueryHandle> registerQuery(
        ContinuousQuerySpec spec) = 0;

    /**
     * @brief Drop Query.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    virtual Result<void> dropQuery(const std::string& name) = 0;

    [[nodiscard]] virtual Result<ResultStreamPtr> subscribe(
        const std::string& name,
        ResultMode mode) = 0;

    [[nodiscard]] virtual std::vector<ContinuousQueryInfo> listQueries() const = 0;

    /**
     * @brief Inject Tuple.
     * @param[in] collection Input parameter.
     * @param[in] tuple Input parameter.
     * @param[in] event_ts Input parameter.
     */
    virtual void injectTuple(const std::string& collection,
                             const std::string& tuple,
                             int64_t            event_ts) = 0;
};

}  // namespace query
}  // namespace themis
