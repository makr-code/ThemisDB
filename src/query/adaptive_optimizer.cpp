/**
 * @file adaptive_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/adaptive_optimizer.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>
#include <spdlog/spdlog.h>

#ifdef __linux__
#include <sched.h>
#if __has_include(<numa.h>)
#include <numa.h>
#define HAS_NUMA 1
#else
#define HAS_NUMA 0
#endif
#include <pthread.h>
#endif

namespace themis {
namespace query {

namespace {

std::string trimCopy(const std::string& input) {
    const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    if (first == input.end()) {
        return {};
    }
    const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    return std::string(first, last);
}

std::string toLowerCopy(const std::string& input) {
    std::string lowered = input;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered;
}

bool startsWith(const std::string& text, const std::string& prefix) {
    return static_cast<bool>( static_cast<int>(text.size()) < static_cast<int>(= prefix.size())) &&
           std::equal(prefix.begin(), prefix.end(), text.begin());
}

} // namespace

// ============================================================================
// AdaptiveQueryStats Implementation
// ============================================================================

void AdaptiveQueryStats::recordExecution(const QueryExecution& exec) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& history = executions_[exec.query_hash];
    history.push_back(exec);
    
    // Limit history size per query
    if (static_cast<int>(history.size()) > MAX_HISTORY_PER_QUERY) {
        history.erase(history.begin());
    }
    
    total_queries_.fetch_add(1, std::memory_order_relaxed);
}

std::vector<AdaptiveQueryStats::QueryExecution> 
AdaptiveQueryStats::getHistory(const std::string& query_hash, size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = executions_.find(query_hash);
    if (it == executions_.end()) {
        return {};
    }
    
    const auto& history = it->second;
    size_t count = std::min(limit,static_cast<int>(history.size()));
    
    return std::vector<QueryExecution>(
        history.end() - count,
        history.end()
    );
}

double AdaptiveQueryStats::getAverageSelectivity(const std::string& query_hash) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = executions_.find(query_hash);
    if (it == executions_.end() || it->second.empty()) {
        return 1.0; // Default to no adjustment
    }
    
    const auto& history = it->second;
    double sum = 0.0;
    size_t valid_count = 0;
    
    for (const auto& exec : history) {
        if (exec.estimated_rows > 0) {
            sum += static_cast<double>(exec.actual_rows) / exec.estimated_rows;
            valid_count++;
        }
    }
    
    return valid_count > 0 ? sum / valid_count : 1.0;
}

bool AdaptiveQueryStats::hasCardinalityMisestimation(
    const std::string& query_hash, 
    double threshold) const {
    
    double avg_selectivity = getAverageSelectivity(query_hash);
    
    // Check if average selectivity is significantly off from 1.0
    return avg_selectivity < (1.0 / threshold) || avg_selectivity > threshold;
}

size_t AdaptiveQueryStats::getAverageActualRows(const std::string& query_hash) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = executions_.find(query_hash);
    if (it == executions_.end() || it->second.empty()) {
        return 0;
    }

    const auto& history = it->second;
    size_t sum = 0;
    for (const auto& exec : history) {
        sum += exec.actual_rows;
    }
    return sum / history.size();
}

double AdaptiveQueryStats::getAdaptiveAdjustmentFactor(
    const std::string& query_hash) const {
    
    double avg_selectivity = getAverageSelectivity(query_hash);
    
    // Use exponential smoothing to avoid over-correction
    const double smoothing = 0.7;
    return smoothing * avg_selectivity + (1.0 - smoothing) * 1.0;
}

void AdaptiveQueryStats::pruneOldStats(std::chrono::hours retention) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto cutoff = std::chrono::system_clock::now() - retention;
    
    for (auto& [hash, history] : executions_) {
        history.erase(
            std::remove_if(history.begin(), history.end(),
                [cutoff](const QueryExecution& exec) {
                    return exec.timestamp < cutoff;
                }),
            history.end()
        );
    }
    
    // Remove empty histories
    for (auto it = executions_.begin(); it != executions_.end();) {
        if (it->second.empty()) {
            it = executions_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// AdaptivePlanSelector Implementation
// ============================================================================

AdaptivePlanSelector::PlanChoice AdaptivePlanSelector::selectPlan(
    const std::vector<PlanChoice>& alternatives,
    const std::string& query_hash,
    const AdaptiveQueryStats& stats) const {
    
    if (alternatives.empty()) {
        throw std::invalid_argument("No plan alternatives provided");
    }
    
    // Get adaptive adjustment based on historical data
    double adjustment = stats.getAdaptiveAdjustmentFactor(query_hash);
    
    // Find plan with minimum adjusted cost
    auto best = std::min_element(
        alternatives.begin(), 
        alternatives.end(),
        [adjustment](const PlanChoice& a, const PlanChoice& b) {
            return a.estimated_cost * adjustment < b.estimated_cost * adjustment;
        }
    );
    
    spdlog::debug("Selected plan: {} (adjusted cost: {:.2f}, adjustment factor: {:.2f})",
                  best->description, best->estimated_cost * adjustment, adjustment);
    
    return *best;
}

bool AdaptivePlanSelector::shouldSwitchPlan(
    size_t rows_so_far,
    size_t estimated_total,
    double progress,
    double misestimation_threshold) const {
    
    // Don't switch if we're almost done
    if (progress > 0.9) {
        return false;
    }
    
    // Don't switch too early (need enough data)
    if (progress < 0.1) {
        return false;
    }
    
    // Extrapolate final row count
    double extrapolated_total = rows_so_far / std::max(progress, 0.01);
    
    // Check if extrapolated count is significantly off from estimate
    if (estimated_total > 0) {
        double ratio = extrapolated_total / estimated_total;
        
        if (ratio > misestimation_threshold || ratio < (1.0 / misestimation_threshold)) {
            spdlog::info("Plan switch recommended: extrapolated={:.0f}, estimated={}, progress={:.1f}%",
                        extrapolated_total, estimated_total, progress * 100.0);
            return true;
        }
    }
    
    return false;
}

AdaptivePlanSelector::PlanChoice AdaptivePlanSelector::getAlternativePlan(
    const PlanChoice& current_plan,
    size_t actual_rows,
    size_t estimated_rows) const {
    
    using Strategy = PlanChoice::Strategy;
    
    PlanChoice alternative;
    alternative.strategy = Strategy::INDEX_SCAN;  // Default initialization
    
    // If we significantly underestimated, prefer simpler strategies
    if (actual_rows > estimated_rows * 5) {
        if (current_plan.strategy == Strategy::INDEX_SCAN) {
            alternative.strategy = Strategy::TABLE_SCAN;
            alternative.description = "Switch to table scan (underestimated selectivity)";
        } else if (current_plan.strategy == Strategy::HASH_JOIN) {
            alternative.strategy = Strategy::NESTED_LOOP_JOIN;
            alternative.description = "Switch to nested loop join (underestimated cardinality)";
        }
    }
    // If we significantly overestimated, prefer more selective strategies
    else if (estimated_rows > actual_rows * 5) {
        if (current_plan.strategy == Strategy::TABLE_SCAN) {
            alternative.strategy = Strategy::INDEX_SCAN;
            alternative.description = "Switch to index scan (overestimated selectivity)";
        } else if (current_plan.strategy == Strategy::NESTED_LOOP_JOIN) {
            alternative.strategy = Strategy::HASH_JOIN;
            alternative.description = "Switch to hash join (overestimated cardinality)";
        }
    }
    
    // If no better alternative was found (description still empty), keep current
    if (alternative.description.empty()) {
        alternative = current_plan;
        alternative.description += " (no alternative)";
    }
    
    return alternative;
}

// ============================================================================
// DistributedQueryCostModel Implementation
// ============================================================================

double DistributedQueryCostModel::estimateDistributedQueryCost(
    const std::vector<ShardInfo>& involved_shards,
    size_t estimated_result_rows) const {
    (void)estimated_result_rows;
    
    double total_cost = 0.0;
    
    for (const auto& shard : involved_shards) {
        // Local processing cost
        double processing_cost = shard.estimated_rows * LOCAL_ROW_PROCESSING_COST;
        
        // Network transfer cost (if not local)
        double network_cost = 0.0;
        if (!shard.is_local) {
            network_cost = shard.estimated_rows * NETWORK_TRANSFER_COST_PER_ROW;
            network_cost += shard.network_latency_ms; // Base latency
        }
        
        total_cost += processing_cost + network_cost;
    }
    
    return total_cost;
}

DistributedQueryCostModel::CrossShardJoinCost 
DistributedQueryCostModel::estimateCrossShardJoinCost(
    const ShardInfo& left_shard,
    const ShardInfo& right_shard,
    size_t left_rows,
    size_t right_rows) const {
    (void)left_shard;
    (void)right_shard;
    
    CrossShardJoinCost result;
    result.total_cost = CROSS_SHARD_JOIN_OVERHEAD;
    
    // Determine optimal join strategy
    constexpr size_t SMALL_TABLE_THRESHOLD = 1000;
    constexpr double SIMILAR_SIZE_THRESHOLD = 0.3;  // 30% size difference tolerance
    
    if (left_rows < SMALL_TABLE_THRESHOLD || right_rows < SMALL_TABLE_THRESHOLD) {
        // Small table - broadcast it
        result.recommended_strategy = "broadcast";
        size_t rows_to_broadcast = std::min(left_rows, right_rows);
        result.network_cost = rows_to_broadcast * NETWORK_TRANSFER_COST_PER_ROW;
        result.compute_cost = left_rows * right_rows * 0.001; // Hash join cost
    } else if (static_cast<double>(left_rows >= right_rows ? left_rows - right_rows : right_rows - left_rows) <
               static_cast<double>(left_rows) * SIMILAR_SIZE_THRESHOLD) {
        // Similar sizes - repartition both
        result.recommended_strategy = "repartition";
        result.network_cost = (left_rows + right_rows) * NETWORK_TRANSFER_COST_PER_ROW * 0.5;
        result.compute_cost = (left_rows + right_rows) * 0.01;
    } else {
        // Use semi-join to reduce network transfer
        result.recommended_strategy = "semi_join";
        size_t smaller = std::min(left_rows, right_rows);
        result.network_cost = smaller * NETWORK_TRANSFER_COST_PER_ROW * 1.5;
        result.compute_cost = smaller * 0.005;
    }
    
    result.total_cost += result.network_cost + result.compute_cost;
    
    return result;
}

bool DistributedQueryCostModel::shouldPrunePartition(
    const ShardInfo& shard,
    size_t total_shards,
    double selectivity) const {
    (void)total_shards;
    
    // Prune if shard has very few relevant rows
    if (selectivity < 0.01) {
        return true;
    }
    
    // Prune if network cost exceeds benefit
    double network_cost = shard.network_latency_ms + 
                         shard.estimated_rows * NETWORK_TRANSFER_COST_PER_ROW;
    double benefit = shard.estimated_rows * selectivity * LOCAL_ROW_PROCESSING_COST;
    
    return network_cost > benefit * 100.0;
    // Only prune if network cost is 100x higher than benefit
    // (very conservative; modern networks and databases have changed cost models)
}

size_t DistributedQueryCostModel::getOptimalParallelism(
    const std::vector<ShardInfo>& shards,
    size_t available_threads) const {
    
    // At least one thread per shard
    size_t min_threads = shards.size();
    
    // Calculate total work
    size_t total_rows = 0;
    for (const auto& shard : shards) {
        total_rows += shard.estimated_rows;
    }
    
    // Target: ~10000 rows per thread
    size_t optimal_threads = std::max(min_threads, total_rows / 10000);
    
    return std::min(optimal_threads, available_threads);
}

// ============================================================================
// MultiIndexOptimizer Implementation
// ============================================================================

MultiIndexOptimizer::IntersectionPlan MultiIndexOptimizer::optimizeMultiIndexAccess(
    const std::vector<IndexCandidate>& available_indexes,
    size_t table_size) const {
    
    IntersectionPlan plan = {};
    
    if (available_indexes.empty()) {
        plan.estimated_result_rows = table_size;
        return plan;
    }
    
    // Sort indexes by selectivity (most selective first)
    std::vector<IndexCandidate> sorted_indexes = available_indexes;
    std::sort(sorted_indexes.begin(), sorted_indexes.end(),
        [](const IndexCandidate& a, const IndexCandidate& b) {
            return a.estimated_selectivity < b.estimated_selectivity;
        });
    
    // Decide whether to use single index or intersection
    if (static_cast<int>(sorted_indexes.size()) == 1 || 
        sorted_indexes[0].estimated_selectivity < table_size * 0.01) {
        // Very selective - use single index
        plan.indexes_to_use.push_back(sorted_indexes[0].index_name);
        plan.estimated_result_rows = sorted_indexes[0].estimated_selectivity;
        plan.estimated_cost = sorted_indexes[0].access_cost;
    } else {
        // Consider index intersection
        size_t result_rows = table_size;
        double total_cost = 0.0;
        
        for (const auto& idx : sorted_indexes) {
            double selectivity_factor = 
                static_cast<double>(idx.estimated_selectivity) / result_rows;
            
            // Add index if it significantly reduces result set
            if (selectivity_factor < 0.8) {
                plan.indexes_to_use.push_back(idx.index_name);
                result_rows = std::min(result_rows, idx.estimated_selectivity);
                total_cost += idx.access_cost;
                
                // Use bitmap if multiple indexes and selectivity warrants it
                if (static_cast<int>(plan.indexes_to_use.size()) > 1 && 
                    selectivity_factor < getBitmapIntersectionThreshold()) {
                    plan.use_bitmap_intersection = true;
                }
            }
        }
        
        plan.estimated_result_rows = result_rows;
        plan.estimated_cost = total_cost;
    }
    
    return plan;
}

bool MultiIndexOptimizer::shouldUseIndexIntersection(
    const std::vector<IndexCandidate>& candidates,
    size_t table_size) const {
    
    if (static_cast<int>(candidates.size()) < 2) {
        return false;
    }
    
    // Check if intersection would be beneficial
    size_t min_selectivity = table_size;
    for (const auto& idx : candidates) {
        min_selectivity = std::min(min_selectivity, idx.estimated_selectivity);
    }
    
    // Use intersection if no single index is very selective
    return min_selectivity > table_size * 0.05;
}

// ============================================================================
// NumaAwareOptimizer Implementation
// ============================================================================

NumaAwareOptimizer::NumaPlacement NumaAwareOptimizer::getOptimalPlacement(
    size_t data_size_bytes,
    size_t parallelism) const {
    (void)data_size_bytes;
    (void)parallelism;
    
    NumaPlacement placement;
    placement.preferred_numa_node = 0;  // Default to node 0
    placement.use_local_memory = true;
    
#if defined(__linux__) && HAS_NUMA
    if (isNumaAvailable()) {
        size_t num_nodes = getNumaNodeCount();
        
        // Prefer node with most free memory
        long max_free = 0;
        int best_node = 0;
        
        for (size_t i = 0; i < num_nodes; i++) {
            long free_mem = numa_node_size64(i, nullptr);
            if (free_mem > max_free) {
                max_free = free_mem;
                best_node = i;
            }
        }
        
        placement.preferred_numa_node = best_node;
        
        // Get CPU IDs for this node
        struct bitmask* cpus = numa_allocate_cpumask();
        if (numa_node_to_cpus(best_node, cpus) == 0) {
            const int possible_cpus = numa_num_possible_cpus();
            for (int i = 0; i < possible_cpus; ++i) {
                if (numa_bitmask_isbitset(cpus, i)) {
                    placement.cpu_affinity.push_back(static_cast<size_t>(i));
                }
            }
        }
        numa_free_cpumask(cpus);
    }
#endif
    
    return placement;
}

bool NumaAwareOptimizer::isNumaAvailable() {
#if defined(__linux__) && HAS_NUMA
    return numa_available() != -1;
#else
    return false;
#endif
}

size_t NumaAwareOptimizer::getNumaNodeCount() {
#if defined(__linux__) && HAS_NUMA
    if (isNumaAvailable()) {
        return numa_num_configured_nodes();
    }
#endif
    return 1;
}

bool NumaAwareOptimizer::pinThreadToCpu([[maybe_unused]] int cpu_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    pthread_t thread = pthread_self();
    return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) == 0;
#else
    [[maybe_unused]] int unused_cpu_id = cpu_id;
    return false;
#endif
}

// ============================================================================
// GeoPredicatePatternDetector Implementation
// ============================================================================

std::optional<GeoPredicatePatternDetector::DetectedSpatialHint>
GeoPredicatePatternDetector::detect(const std::string& query_text) {
    if (query_text.empty()) {
        return std::nullopt;
    }

    const std::string lowered = toLowerCopy(query_text);
    const auto filter_pos = lowered.find("filter");
    const auto within_pos = lowered.find("st_within", filter_pos == std::string::npos ? 0 : filter_pos);
    if (within_pos == std::string::npos) {
        return std::nullopt;
    }

    const auto open_pos = lowered.find('(', within_pos);
    if (open_pos == std::string::npos) {
        return std::nullopt;
    }

    int depth = 1;
    std::size_t close_pos = open_pos + 1;
    for (; close_pos <static_cast<int>(lowered.size()); ++close_pos) {
        const char ch = lowered[close_pos];
        if (ch == '(') {
            ++depth;
        } else if (ch == ')') {
            --depth;
            if (depth == 0) {
                break;
            }
        }
    }

    if (close_pos >= lowered.size() || depth != 0) {
        return std::nullopt;
    }

    const std::string arg_text = query_text.substr(open_pos + 1, close_pos - open_pos - 1);
    int nested = 0;
    std::size_t split_pos = std::string::npos;
    for (std::size_t i = 0; i <static_cast<int>(arg_text.size()); ++i) {
        const char ch = arg_text[i];
        if (ch == '(') {
            ++nested;
        } else if (ch == ')') {
            if (nested > 0) {
                --nested;
            }
        } else if (ch == ',' && nested == 0) {
            split_pos = i;
            break;
        }
    }

    if (split_pos == std::string::npos) {
        return std::nullopt;
    }

    const std::string first_arg = trimCopy(arg_text.substr(0, split_pos));
    const std::string second_arg = trimCopy(arg_text.substr(split_pos + 1));
    if (first_arg.empty() || second_arg.empty()) {
        return std::nullopt;
    }

    if (first_arg[0] == '@' || startsWith(toLowerCopy(first_arg), "st_")) {
        return std::nullopt;
    }

    const std::string second_lower = toLowerCopy(second_arg);
    const bool second_is_literal_like =
        second_arg[0] == '@' || second_arg[0] == '[' || second_arg[0] == '{' ||
        second_arg[0] == '"' || second_arg[0] == '\'' ||
        startsWith(second_lower, "st_geomfromgeojson(");
    if (!second_is_literal_like) {
        return std::nullopt;
    }

    return DetectedSpatialHint{"st_within", first_arg};
}

void GeoPredicatePatternDetector::injectSpatialIndexHints(
    const std::string& query_text,
    std::map<std::string, std::string>& hints,
    std::vector<std::string>& suggested_indexes) {
    const auto detected = detect(query_text);
    if (!detected.has_value()) {
        return;
    }

    hints["index_hint"] = "GEO";
    hints["geo_index_scan"] = "enabled";
    hints["geo_predicate"] = "ST_Within";
    hints["geo_field"] = detected->field_reference;

    const bool has_geo_suggestion = std::find(
        suggested_indexes.begin(), suggested_indexes.end(), "geo") != suggested_indexes.end();
    if (!has_geo_suggestion) {
        suggested_indexes.emplace_back("geo");
    }
}

} // namespace query
} // namespace themis

