/**
 * @file adaptive_import.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
     * @brief Adapt Batch Size.
     * @param[in] metrics Input parameter.
     */
    void adaptBatchSize(const RuntimeMetrics& metrics);

    double currentBatchMultiplier() const { return batch_multiplier_; }

    // ------------------------------------------------------------------
    // Performance predictor
    // ------------------------------------------------------------------
    struct PredictedMetrics {
        double estimated_import_time_seconds{0.0};
        double estimated_peak_memory_mb{0.0};
        double estimated_io_ops{0.0};
    };

    class PerformancePredictor {
    public:
        PredictedMetrics predictPerformance(
            const ImportPlan& plan,
            const std::vector<InferenceTableSchema>& schemas,
            const std::map<std::string, ColumnStatistics>& stats = {}
        );
    };

private:
    double batch_multiplier_{1.0};

    /**
     * @brief Topological Sort.
     * @param[in] schemas Input parameter.
     * @return Return value.
     */
    std::vector<std::string> topologicalSort(
        const std::vector<InferenceTableSchema>& schemas
    ) const;
};

} // namespace importers
} // namespace themis
