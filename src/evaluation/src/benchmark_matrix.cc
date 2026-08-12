/**
 * @file benchmark_matrix.cc
 * @brief Implementation of `BenchmarkMatrix` — core logic for recording,
 *        querying, and comparing benchmark results across architecture paths.
 */

#include "benchmark_matrix.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace themis {
namespace evaluation {

// ============================================================================
// BenchmarkMatrix::record
// ============================================================================

void BenchmarkMatrix::record(BenchmarkScenario scenario,
                             BenchmarkDimension dimension,
                             const BenchmarkResult& result) {
    // Reject vacuous inserts: a clean result must have at least one sample.
    if (result.edge_flags == BenchmarkEdgeCase::NONE && result.sample_count == 0) {
        throw std::invalid_argument(
            "BenchmarkMatrix::record: sample_count must be > 0 for a clean result "
            "(scenario=" + std::string(scenarioName(scenario)) +
            ", dimension=" + std::string(dimensionName(dimension)) + ")");
    }
    cells_[{scenario, dimension}] = result;
}

// ============================================================================
// BenchmarkMatrix::invalidateScenario
// ============================================================================

void BenchmarkMatrix::invalidateScenario(BenchmarkScenario scenario) {
    for (auto it = cells_.begin(); it != cells_.end(); ) {
        if (it->first.scenario == scenario) {
            it = cells_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// BenchmarkMatrix::invalidateDimension
// ============================================================================

void BenchmarkMatrix::invalidateDimension(BenchmarkDimension dimension) {
    for (auto it = cells_.begin(); it != cells_.end(); ) {
        if (it->first.dimension == dimension) {
            it = cells_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// BenchmarkMatrix::clear
// ============================================================================

void BenchmarkMatrix::clear() noexcept {
    cells_.clear();
}

// ============================================================================
// BenchmarkMatrix::lookup
// ============================================================================

std::optional<BenchmarkResult>
BenchmarkMatrix::lookup(BenchmarkScenario scenario,
                        BenchmarkDimension dimension) const noexcept {
    auto it = cells_.find({scenario, dimension});
    if (it == cells_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ============================================================================
// BenchmarkMatrix::entries
// ============================================================================

std::vector<BenchmarkEntry>
BenchmarkMatrix::entries(std::string_view dataset_tag,
                         std::string_view hardware_tag,
                         std::string_view runner_version) const {
    std::vector<BenchmarkEntry> out;
    out.reserve(cells_.size());
    for (const auto& [key, result] : cells_) {
        BenchmarkEntry e;
        e.scenario       = key.scenario;
        e.dimension      = key.dimension;
        e.result         = result;
        e.dataset_tag    = std::string(dataset_tag);
        e.hardware_tag   = std::string(hardware_tag);
        e.runner_version = std::string(runner_version);
        out.push_back(std::move(e));
    }
    return out;
}

// ============================================================================
// BenchmarkMatrix::scenarioSlice
// ============================================================================

std::vector<std::pair<BenchmarkDimension, BenchmarkResult>>
BenchmarkMatrix::scenarioSlice(BenchmarkScenario scenario) const {
    std::vector<std::pair<BenchmarkDimension, BenchmarkResult>> out;
    for (const auto& [key, result] : cells_) {
        if (key.scenario == scenario) {
            out.emplace_back(key.dimension, result);
        }
    }
    return out;
}

// ============================================================================
// BenchmarkMatrix::dimensionSlice
// ============================================================================

std::vector<std::pair<BenchmarkScenario, BenchmarkResult>>
BenchmarkMatrix::dimensionSlice(BenchmarkDimension dimension) const {
    std::vector<std::pair<BenchmarkScenario, BenchmarkResult>> out;
    for (const auto& [key, result] : cells_) {
        if (key.dimension == dimension) {
            out.emplace_back(key.scenario, result);
        }
    }
    return out;
}

// ============================================================================
// BenchmarkMatrix::scenariosWithFullCoverage
// ============================================================================

std::vector<BenchmarkScenario>
BenchmarkMatrix::scenariosWithFullCoverage(
    const std::vector<BenchmarkDimension>& required_dimensions) const {
    std::vector<BenchmarkScenario> result;

    // Collect distinct scenarios present in the matrix.
    std::vector<BenchmarkScenario> present_scenarios;
    for (const auto& [key, _] : cells_) {
        if (std::find(present_scenarios.begin(),
                      present_scenarios.end(),
                      key.scenario) == present_scenarios.end()) {
            present_scenarios.push_back(key.scenario);
        }
    }

    for (auto s : present_scenarios) {
        bool all_covered = true;
        for (auto d : required_dimensions) {
            auto r = lookup(s, d);
            if (!r || !r->hasSufficientData()) {
                all_covered = false;
                break;
            }
        }
        if (all_covered) {
            result.push_back(s);
        }
    }
    return result;
}

// ============================================================================
// BenchmarkMatrix::size
// ============================================================================

std::size_t BenchmarkMatrix::size() const noexcept {
    return cells_.size();
}

// ============================================================================
// BenchmarkMatrix::contains
// ============================================================================

bool BenchmarkMatrix::contains(BenchmarkScenario scenario,
                                BenchmarkDimension dimension) const noexcept {
    return cells_.count({scenario, dimension}) > 0;
}

// ============================================================================
// BenchmarkMatrix::compareScenarios
// ============================================================================

std::optional<double>
BenchmarkMatrix::compareScenarios(BenchmarkScenario a,
                                  BenchmarkScenario b,
                                  BenchmarkDimension dimension) const noexcept {
    auto ra = lookup(a, dimension);
    auto rb = lookup(b, dimension);
    if (!ra || !rb) {
        return std::nullopt;
    }
    // Avoid division by zero.
    if (rb->value == 0.0) {
        return std::nullopt;
    }
    return ra->value / rb->value;
}

// ============================================================================
// BenchmarkMatrix::bestScenario
// ============================================================================

std::optional<BenchmarkScenario>
BenchmarkMatrix::bestScenario(BenchmarkDimension dimension,
                              bool higher_is_better) const noexcept {
    std::optional<BenchmarkScenario> best;
    double best_val = higher_is_better
                          ? std::numeric_limits<double>::lowest()
                          : std::numeric_limits<double>::max();

    for (const auto& [key, result] : cells_) {
        if (key.dimension != dimension) {
            continue;
        }
        if (!result.hasSufficientData()) {
            continue;
        }
        bool is_better = higher_is_better
                             ? (result.value > best_val)
                             : (result.value < best_val);
        if (is_better) {
            best_val = result.value;
            best     = key.scenario;
        }
    }
    return best;
}

} // namespace evaluation
} // namespace themis
