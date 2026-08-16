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

namespace themis {
namespace query {

// ──────────────────────────────────────────────────────────────────────────────
// ContinuousPlan::evaluate
// ──────────────────────────────────────────────────────────────────────────────

void ContinuousPlan::evaluate(ContinuousQueryState& state,
                               std::vector<CQResult>& results) const {
    auto& spec     = state.spec;
    auto& synopsis = *state.synopsis;
    auto& wm       = *state.watermark;

    // 1. Advance watermark to current tick boundary
    wm.advance();

    const int64_t wm_us = wm.watermarkUs();
    const ResultMode mode = spec.result_mode;

    // 2. Expire old tuples and collect them as retractions
    std::deque<SynopsisTuple> expired;
    if (spec.window.isTimeBased()) {
        const int64_t window_start_us =
            wm_us - static_cast<int64_t>(spec.window.range_ms) * 1000LL;
        expired = synopsis.expire(window_start_us);
    }

    // 3. Emit results according to ResultMode
    if (mode == ResultMode::DELTA || mode == ResultMode::CHANGES) {
        // Emit retractions for expired tuples
        for (const auto& t : expired) {
            results.push_back({t.payload, /*is_retract=*/true});
        }
    }

    if (mode == ResultMode::SNAPSHOT) {
        // Emit full window snapshot
        auto snap = synopsis.snapshot();
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