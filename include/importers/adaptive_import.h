/**
 * @file adaptive_import.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/schema_inference.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

/**
 * @brief Self-tuning import optimizer.
 *
 * Determines optimal import order (topological sort), adaptive batch sizes,
 * and parallelisation candidates based on table statistics.
 *
 * References:
 *   - Pavlo et al. (2017) "Self-Driving Database Management Systems" (CMU 15-721)
 *   - Marcus et al. (2019) "Machine Learning for Query Optimization" (VLDB)
 */
class AdaptiveImportOptimizer {
public:
    // ------------------------------------------------------------------
    // Import plan
    // ------------------------------------------------------------------
    struct ImportPlan {
        std::vector<std::string> import_order;            ///< Topologically sorted
        std::map<std::string, size_t> batch_sizes;        ///< Per-table batch size
        std::map<std::string, bool> parallel_candidates;  ///< Tables safe to import in parallel
        json optimizer_notes;                             ///< Human-readable decision log
    };

    /**
     * @brief Build an optimised import plan using column statistics and
     *        FK dependency analysis.
     *
     * The plan respects FK dependencies (parents before children) and sets
     * larger batch sizes for tables with low cardinality variance.
     */
    ImportPlan optimizeImportPlan(
        const std::vector<InferenceTableSchema>& schemas,
        const std::map<std::string, ColumnStatistics>& stats = {}
    );

    // ------------------------------------------------------------------
    // Runtime adaptive tuning
    // ------------------------------------------------------------------
    struct RuntimeMetrics {
        double cpu_utilization{0.0};     ///< [0,100] %
        double memory_utilization{0.0};  ///< [0,100] %
        double io_throughput_mbps{0.0};
        size_t rows_per_second{0};
    };

    /**
     * @brief Adjust the active batch size for all tables based on current
     *        runtime metrics.
     *
     * Reduces batch size when memory > 80 % or CPU > 90 %; increases
     * batch size when both are below 50 %.
     */
    void adaptBatchSize(const RuntimeMetrics& metrics);

    /** @brief Return the current adaptive batch size multiplier [0.1, 4.0]. */
    double currentBatchMultiplier() const { return batch_multiplier_; }

    // ------------------------------------------------------------------
    // Performance predictor
    // ------------------------------------------------------------------
    struct PredictedMetrics {
        double estimated_import_time_seconds{0.0};
        double estimated_peak_memory_mb{0.0};
        double estimated_io_ops{0.0};
    };

    /** @brief Performance predictor. */
    class PerformancePredictor {
    public:
        /**
         * @brief Predict import performance for the given plan.
         * Uses linear regression over row counts and column widths.
         */
        PredictedMetrics predictPerformance(
            const ImportPlan& plan,
            const std::vector<InferenceTableSchema>& schemas,
            const std::map<std::string, ColumnStatistics>& stats = {}
        );
    };

private:
    double batch_multiplier_{1.0};

    std::vector<std::string> topologicalSort(
        const std::vector<InferenceTableSchema>& schemas
    ) const;
};

} // namespace importers
} // namespace themis
