/**
 * @file continuous_query_planner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct ContinuousQueryState {
    ContinuousQuerySpec         spec;
    ContinuousQueryInfo         info;
    std::unique_ptr<SynopsisStore> synopsis;
    std::unique_ptr<CQWatermark>   watermark;
};

enum class CQPlanNodeType { SYNOPSIS, DELTA_AGG, RESULT_EMIT, SCATTER_GATHER };

struct ContinuousPlan {
    CQPlanNodeType type{CQPlanNodeType::RESULT_EMIT};
    std::string    query_name;

    /**
     * @brief Evaluate.
     * @param[in,out] state Input/output parameter.
     * @param[in,out] results Input/output parameter.
     */
    void evaluate(ContinuousQueryState& state,
                  std::vector<CQResult>& results) const;
};

class ContinuousQueryPlanner {
public:
    ContinuousQueryPlanner() = default;

    [[nodiscard]] Result<ContinuousPlan> compile(
        const ContinuousQuerySpec& spec) const;
};

}  // namespace query
}  // namespace themis
