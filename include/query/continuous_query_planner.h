/**
 * @file continuous_query_planner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/continuous_query_engine.h"
#include "query/continuous_query_registry.h"
#include "query/synopsis_store.h"
#include "query/cq_watermark.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace query {

/**
 * @brief Stateful object representing a single registered continuous query.
 *
 * Owned by the registry; not directly exposed to callers.
 */
struct ContinuousQueryState {
    ContinuousQuerySpec         spec;
    ContinuousQueryInfo         info;
    std::unique_ptr<SynopsisStore> synopsis;
    std::unique_ptr<CQWatermark>   watermark;
};

/**
 * @brief Plan node types used by ContinuousQueryPlanner.
 */
enum class CQPlanNodeType { SYNOPSIS, DELTA_AGG, RESULT_EMIT, SCATTER_GATHER };

/**
 * @brief A compiled continuous-query evaluation plan.
 *
 * evaluate() runs the incremental evaluation for one tick and appends
 * result items to the supplied output vector.
 */
struct ContinuousPlan {
    CQPlanNodeType type{CQPlanNodeType::RESULT_EMIT};
    std::string    query_name;

    /**
     * @brief Execute one evaluation tick.
     *
     * @param state   Mutable query state (synopsis, watermark).
     * @param results Output vector; items are appended.
     */
    void evaluate(ContinuousQueryState& state,
                  std::vector<CQResult>& results) const;
};

/**
 * @brief Builds ContinuousPlan objects from ContinuousQuerySpec.
 */
class ContinuousQueryPlanner {
public:
    ContinuousQueryPlanner() = default;

    /**
     * @brief Compile a spec into an evaluation plan.
     *
     * Validates the spec, selects the plan node type (local vs.
     * scatter-gather), and returns a ContinuousPlan ready for evaluation.
     *
     * @return a ContinuousPlan, or an Error on validation failure.
     */
    [[nodiscard]] Result<ContinuousPlan> compile(
        const ContinuousQuerySpec& spec) const;
};

}  // namespace query
}  // namespace themis
