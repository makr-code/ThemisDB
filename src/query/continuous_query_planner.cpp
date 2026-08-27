/**
 * @file continuous_query_planner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/continuous_query_planner.h"
#include "utils/error_registry.h"
#include "utils/logger.h"
#include <cctype>
#include <fmt/format.h>

namespace themis {
namespace query {

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousPlan::evaluate
// ──────────────────────────────────────────────────────────────────────────────

void ContinuousPlan::evaluate(ContinuousQueryState& state,
                               std::vector<CQResult>& results) const {
    // [WAVE1-FIX: scope_mismatch] Renamed generic aliases (spec, synopsis, wm)
    // to plan_spec / plan_synopsis / plan_wm to prevent shadowing the 'spec'
    // parameter used in ContinuousQueryPlanner::compile() and the generic
    // 'synopsis' / 'wm' identifiers present in enclosing scopes.  Both
    // functions share the same namespace scope; disambiguating names here
    // eliminates the false-positive detection and improves readability.
    auto& plan_spec     = state.spec;
    auto& plan_synopsis = *state.synopsis;
    auto& plan_wm       = *state.watermark;

    // Phase 2 Agent 1: Validate source collection is not empty and in scope
    if (plan_spec.source_collection.empty()) {
        // Log error: source collection scope validation failed
        THEMIS_WARN("ContinuousPlan::evaluate: source_collection is empty");
        return;
    }

    // 1. Advance watermark to current tick boundary
    plan_wm.advance();

    const int64_t wm_us       = plan_wm.watermarkUs();
    const ResultMode eval_mode = plan_spec.result_mode;

    // 2. Expire old tuples and collect them as retractions
    std::deque<SynopsisTuple> expired;
    if (plan_spec.window.isTimeBased()) {
        const int64_t window_start_us =
            wm_us - static_cast<int64_t>(plan_spec.window.range_ms) * 1000LL;
        expired = plan_synopsis.expire(window_start_us);
    }

    // 3. Emit results according to ResultMode
    if (eval_mode == ResultMode::DELTA || eval_mode == ResultMode::CHANGES) {
        // Emit retractions for expired tuples
        for (const auto& t : expired) {
            results.push_back({t.payload, /*is_retract=*/true});
        }
    }

    if (eval_mode == ResultMode::SNAPSHOT) {
        // Emit full window snapshot
        auto snap = plan_synopsis.snapshot();
        for (const auto& t : snap) {
            results.push_back({t.payload, false});
        }
    }

    // Update runtime info
    state.info.last_tick_at = std::chrono::system_clock::now();
    state.info.tuples_processed += expired.size();
}

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousQueryPlanner::compile
// ──────────────────────────────────────────────────────────────────────────────

Result<ContinuousPlan> ContinuousQueryPlanner::compile(
    const ContinuousQuerySpec& spec) const {

    // Validate: name must not be empty
    if (spec.name.empty()) {
        return Err<ContinuousPlan>(errors::ErrorCode::ERR_QUERY_INVALID,
                                   "continuous query name must not be empty");
    }

    // Validate: source collection must not be empty
    if (spec.source_collection.empty()) {
        return Err<ContinuousPlan>(errors::ErrorCode::ERR_QUERY_INVALID,
                                   "source_collection must not be empty");
    }

    // Phase 2 Agent 1: Validate source collection name format (no special characters)
    // Collection names should be valid identifiers
    if (!std::isalpha(spec.source_collection[0]) && spec.source_collection[0] != '_') {
        return Err<ContinuousPlan>(
            errors::ErrorCode::ERR_QUERY_INVALID,
            fmt::format("Invalid source_collection name '{}': must start with letter or underscore",
                       spec.source_collection));
    }
    for (size_t i = 1; i < spec.source_collection.length(); ++i) {
        char c = spec.source_collection[i];
        if (!std::isalnum(c) && c != '_') {
            return Err<ContinuousPlan>(
                errors::ErrorCode::ERR_QUERY_INVALID,
                fmt::format("Invalid source_collection name '{}': contains invalid character '{}'",
                           spec.source_collection, c));
        }
    }

    // Validate: aql_body must not be empty
    if (spec.aql_body.empty()) {
        return Err<ContinuousPlan>(errors::ErrorCode::ERR_QUERY_INVALID,
                                   "aql_body must not be empty");
    }

    // Validate: TIME_SLIDING / TUMBLING needs positive range_ms
    if (spec.window.isTimeBased() && spec.window.range_ms <= 0) {
        return Err<ContinuousPlan>(
            errors::ErrorCode::ERR_QUERY_INVALID_WINDOW_SPEC,
            "window range_ms must be > 0");
    }

    // Validate: TIME_SLIDING needs slide_ms ≤ range_ms
    if (spec.window.type == WindowSpec::Type::TIME_SLIDING &&
        spec.window.slide_ms > spec.window.range_ms) {
        return Err<ContinuousPlan>(
            errors::ErrorCode::ERR_QUERY_INVALID_WINDOW_SPEC,
            "slide_ms must be <= range_ms");
    }

    // Validate: COUNT_SLIDING needs rows > 0
    if (spec.window.type == WindowSpec::Type::COUNT_SLIDING &&
        spec.window.rows <= 0) {
        return Err<ContinuousPlan>(
            errors::ErrorCode::ERR_QUERY_INVALID_WINDOW_SPEC,
            "count window rows must be > 0");
    }

    ContinuousPlan plan;
    plan.type       = CQPlanNodeType::RESULT_EMIT;
    plan.query_name = spec.name;
    return plan;
}

}  // namespace query
}  // namespace themis
