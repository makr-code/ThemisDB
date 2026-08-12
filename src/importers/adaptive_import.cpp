/**
 * @file adaptive_import.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/adaptive_import.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// Topological sort (Kahn's algorithm)
// ---------------------------------------------------------------------------

std::vector<std::string>
AdaptiveImportOptimizer::topologicalSort(const std::vector<InferenceTableSchema> &schemas) const {
    // Build adjacency list (parent → children) from FK declarations
    std::map<std::string, std::set<std::string>> deps; // table → tables it depends on
    std::map<std::string, size_t> in_degree;

    for (const auto &s : schemas) {
        in_degree[s.name]; // ensure entry exists
        for (const auto &fk : s.foreign_keys) {
            // fk.second = "parent_table.col"
            std::string parent = fk.second.substr(0, fk.second.find('.'));
            if (parent != s.name) {
                deps[s.name].insert(parent);
                in_degree[parent]; // ensure entry
            }
        }
    }

    // Kahn's BFS
    std::vector<std::string> order;
    std::set<std::string> ready;

    // Tables with no dependencies start first
    for (const auto &s : schemas) {
        if (deps[s.name].empty()) {
            ready.insert(s.name);
        }
    }

    std::set<std::string> visited;
    while (!ready.empty()) {
        std::string t = *ready.begin();
        ready.erase(ready.begin());
        order.push_back(t);
        visited.insert(t);

        // Unblock tables that only depended on 't'
        for (const auto &s : schemas) {
            if (visited.count(s.name)) {
                continue;
            }
            auto &d = deps[s.name];
            d.erase(t);
            if (d.empty()) {
                ready.insert(s.name);
            }
        }
    }

    // Add any remaining tables (e.g., circular references)
    for (const auto &s : schemas) {
        if (!visited.count(s.name)) {
            order.push_back(s.name);
        }
    }

    return order;
}

// ---------------------------------------------------------------------------
// optimizeImportPlan
// ---------------------------------------------------------------------------

AdaptiveImportOptimizer::ImportPlan
AdaptiveImportOptimizer::optimizeImportPlan(const std::vector<InferenceTableSchema> &schemas,
                                            const std::map<std::string, ColumnStatistics> &stats) {
    ImportPlan plan;
    plan.import_order = topologicalSort(schemas);

    json notes = json::object();

    for (const auto &schema : schemas) {
        // Default batch size: 1000
        size_t batch = 1000;

        // Look for row count hints in stats
        for (const auto &col : schema.primary_keys) {
            std::string key = schema.name + "." + col;
            if (stats.count(key)) {
                const auto &st = stats.at(key);
                // Scale batch inversely to column width: wider rows → smaller batches
                if (st.avg_length > 0) {
                    batch = std::max(size_t(100),
                                     std::min(size_t(10000), static_cast<size_t>(1000.0 / (st.avg_length / 64.0))));
                }
            }
        }

        plan.batch_sizes[schema.name] = static_cast<size_t>(batch * batch_multiplier_);

        // A table is a parallel candidate if it has no FK dependencies
        plan.parallel_candidates[schema.name] = schema.foreign_keys.empty();

        notes[schema.name] = json{{"batch_size", plan.batch_sizes.at(schema.name)},
                                  {"parallel", plan.parallel_candidates.at(schema.name)}};
    }

    plan.optimizer_notes = notes;
    return plan;
}

// ---------------------------------------------------------------------------
// adaptBatchSize
// ---------------------------------------------------------------------------

void AdaptiveImportOptimizer::adaptBatchSize(const RuntimeMetrics &metrics) {
    if (metrics.memory_utilization > 80.0 || metrics.cpu_utilization > 90.0) {
        // Back off
        batch_multiplier_ = std::max(0.1, batch_multiplier_ * 0.75);
    } else if (metrics.memory_utilization < 50.0 && metrics.cpu_utilization < 50.0) {
        // Scale up
        batch_multiplier_ = std::min(4.0, batch_multiplier_ * 1.25);
    }
    // else: stay the same
}

// ---------------------------------------------------------------------------
// PerformancePredictor
// ---------------------------------------------------------------------------

AdaptiveImportOptimizer::PredictedMetrics AdaptiveImportOptimizer::PerformancePredictor::predictPerformance(
    const ImportPlan &plan, const std::vector<InferenceTableSchema> &schemas,
    const std::map<std::string, ColumnStatistics> &stats) {
    PredictedMetrics pm;

    for (const auto &table_name : plan.import_order) {
        // Find schema
        auto sit = std::find_if(schemas.begin(), schemas.end(),
                                [&](const InferenceTableSchema &s) { return s.name == table_name; });
        if (sit == schemas.end()) {
            continue;
        }

        size_t batch = 1000;
        if (plan.batch_sizes.count(table_name)) {
            batch = plan.batch_sizes.at(table_name);
        }

        // Estimate row count from stats
        size_t total_rows = 0;
        for (const auto &col : sit->columns) {
            std::string key = table_name + "." + col;
            if (stats.count(key)) {
                total_rows = std::max(total_rows, stats.at(key).total_rows);
            }
        }
        if (total_rows == 0) {
            total_rows = 10000; // default estimate
        }

        // Linear model: 1 ms per row for sequential, 0.5 ms parallel
        double table_time_s = static_cast<double>(total_rows) / 1000.0;
        if (plan.parallel_candidates.count(table_name) && plan.parallel_candidates.at(table_name)) {
            table_time_s *= 0.5;
        }
        pm.estimated_import_time_seconds += table_time_s;

        // Memory: batch * avg_row_size
        pm.estimated_peak_memory_mb = std::max(pm.estimated_peak_memory_mb,
                                               static_cast<double>(batch) * 0.001); // 1 KB per row

        // I/O: 1 op per row
        pm.estimated_io_ops += static_cast<double>(total_rows);
    }

    return pm;
}

} // namespace importers
} // namespace themis
