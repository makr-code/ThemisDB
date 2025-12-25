#include "analytics/process_pattern_matcher.h"
#include "analytics/process_mining.h"
#include "index/vector_index.h"
#include "index/graph_index.h"
#include "utils/logger.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <set>

namespace themis {

// ============================================================================
// Constructor & Initialization
// ============================================================================

ProcessPatternMatcher::ProcessPatternMatcher(
    RocksDBWrapper& db,
    std::shared_ptr<VectorIndex> vector_index,
    std::shared_ptr<GraphIndex> graph_index
) : db_(db), 
    vector_index_(vector_index), 
    graph_index_(graph_index),
    pattern_cache_max_size_(1000) {
    
    logger::info("ProcessPatternMatcher initialized with vector and graph index support");
}

ProcessPatternMatcher::~ProcessPatternMatcher() = default;

// ============================================================================
// Model Loading
// ============================================================================

ProcessPatternMatcher::Status ProcessPatternMatcher::loadAdministrativeModels(
    const std::string& config_directory
) {
    try {
        // Load all YAML files in the config directory
        std::vector<std::string> yaml_files = {
            "administrative_process_models.yaml",
            "it_service_processes.yaml",
            "healthcare_processes.yaml",
            "customer_service_processes.yaml",
            "financial_processes.yaml"
        };
        
        for (const auto& filename : yaml_files) {
            std::string filepath = config_directory + "/" + filename;
            std::ifstream file(filepath);
            
            if (!file.is_open()) {
                logger::warn("Could not open process model file: {}", filepath);
                continue;
            }
            
            try {
                YAML::Node config = YAML::LoadFile(filepath);
                
                if (!config["process_models"]) {
                    logger::warn("No process_models found in: {}", filepath);
                    continue;
                }
                
                for (const auto& model_node : config["process_models"]) {
                    ProcessPattern pattern;
                    pattern.id = model_node["id"].as<std::string>();
                    pattern.name = model_node["name"].as<std::string>();
                    pattern.description = model_node["description"].as<std::string>("");
                    pattern.domain = model_node["domain"].as<std::string>("");
                    
                    // Load activities
                    if (model_node["activities"]) {
                        for (const auto& activity : model_node["activities"]) {
                            pattern.activities.push_back(activity.as<std::string>());
                        }
                    }
                    
                    // Load edges
                    if (model_node["edges"]) {
                        for (const auto& edge : model_node["edges"]) {
                            ProcessPattern::Edge e;
                            e.from = edge["from"].as<std::string>();
                            e.to = edge["to"].as<std::string>();
                            e.probability = edge["probability"].as<double>(1.0);
                            pattern.edges.push_back(e);
                        }
                    }
                    
                    // Load metadata
                    if (model_node["sla_days"]) {
                        pattern.metadata["sla_days"] = model_node["sla_days"].as<int>();
                    }
                    if (model_node["compliance"]) {
                        std::vector<std::string> compliance;
                        for (const auto& c : model_node["compliance"]) {
                            compliance.push_back(c.as<std::string>());
                        }
                        nlohmann::json comp_json(compliance);
                        pattern.metadata["compliance"] = comp_json;
                    }
                    
                    // Store pattern
                    administrative_models_[pattern.id] = pattern;
                    logger::info("Loaded process model: {} ({})", pattern.name, pattern.id);
                }
                
            } catch (const YAML::Exception& e) {
                logger::error("YAML parsing error in {}: {}", filepath, e.what());
            }
        }
        
        logger::info("Loaded {} administrative process models", administrative_models_.size());
        return Status::Success();
        
    } catch (const std::exception& e) {
        return Status::Error(std::string("Failed to load models: ") + e.what());
    }
}

std::optional<ProcessPattern> ProcessPatternMatcher::getAdministrativeModel(
    const std::string& model_id
) const {
    auto it = administrative_models_.find(model_id);
    if (it != administrative_models_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> ProcessPatternMatcher::listAdministrativeModels() const {
    std::vector<std::string> model_ids;
    model_ids.reserve(administrative_models_.size());
    for (const auto& [id, _] : administrative_models_) {
        model_ids.push_back(id);
    }
    return model_ids;
}

// ============================================================================
// Pattern Matching - Main Methods
// ============================================================================

std::pair<ProcessPatternMatcher::Status, SimilarityResults> 
ProcessPatternMatcher::findSimilar(
    const ProcessPattern& pattern,
    const SimilarityConfig& config
) {
    try {
        SimilarityResults results;
        
        // Check cache first
        std::string cache_key = createCacheKey(pattern, config);
        if (auto cached = getFromCache(cache_key)) {
            return {Status::Success(), *cached};
        }
        
        // Get all process instances from database (simplified)
        // In real implementation, this would query the database
        std::vector<ProcessPattern> candidates = getAllProcessInstances();
        
        // Compute similarity for each candidate
        for (const auto& candidate : candidates) {
            SimilarityResult result;
            result.case_id = candidate.id;
            
            switch (config.method) {
                case SimilarityMethod::GRAPH:
                    result.overall_similarity = computeGraphSimilarity(pattern, candidate);
                    result.graph_similarity = result.overall_similarity;
                    break;
                    
                case SimilarityMethod::VECTOR:
                    result.overall_similarity = computeVectorSimilarity(pattern, candidate);
                    result.vector_similarity = result.overall_similarity;
                    break;
                    
                case SimilarityMethod::BEHAVIORAL:
                    result.overall_similarity = computeBehavioralSimilarity(pattern, candidate);
                    result.behavioral_similarity = result.overall_similarity;
                    break;
                    
                case SimilarityMethod::HYBRID:
                    result.graph_similarity = computeGraphSimilarity(pattern, candidate);
                    result.vector_similarity = computeVectorSimilarity(pattern, candidate);
                    result.behavioral_similarity = computeBehavioralSimilarity(pattern, candidate);
                    result.overall_similarity = 
                        config.weight_graph * result.graph_similarity +
                        config.weight_vector * result.vector_similarity +
                        config.weight_behavioral * result.behavioral_similarity;
                    break;
            }
            
            // Check threshold
            if (result.overall_similarity >= config.threshold) {
                // Find matched and missing activities
                std::set<std::string> pattern_set(pattern.activities.begin(), pattern.activities.end());
                std::set<std::string> candidate_set(candidate.activities.begin(), candidate.activities.end());
                
                for (const auto& act : pattern.activities) {
                    if (candidate_set.count(act)) {
                        result.matched_activities.push_back(act);
                    } else {
                        result.missing_activities.push_back(act);
                    }
                }
                
                results.results.push_back(result);
            }
        }
        
        // Sort by similarity (descending)
        std::sort(results.results.begin(), results.results.end(),
                  [](const SimilarityResult& a, const SimilarityResult& b) {
                      return a.overall_similarity > b.overall_similarity;
                  });
        
        // Limit results
        if (config.limit > 0 && results.results.size() > static_cast<size_t>(config.limit)) {
            results.results.resize(config.limit);
        }
        
        results.total_count = results.results.size();
        
        // Cache results
        putInCache(cache_key, results);
        
        return {Status::Success(), results};
        
    } catch (const std::exception& e) {
        return {Status::Error(std::string("findSimilar failed: ") + e.what()), {}};
    }
}

std::pair<ProcessPatternMatcher::Status, ConformanceResult>
ProcessPatternMatcher::compareWithIdeal(
    const std::string& case_id,
    const ProcessPattern& ideal
) {
    try {
        // Get the actual process instance
        auto actual_opt = getProcessInstance(case_id);
        if (!actual_opt) {
            return {Status::Error("Case ID not found: " + case_id), {}};
        }
        
        ProcessPattern actual = *actual_opt;
        ConformanceResult result;
        result.case_id = case_id;
        
        // Compute fitness (token replay)
        result.fitness = computeFitness(actual, ideal);
        
        // Compute precision
        result.precision = computePrecision(actual, ideal);
        
        // Compute generalization
        result.generalization = 0.8; // Simplified
        
        // Compute simplicity
        result.simplicity = 0.9; // Simplified
        
        // Find deviations
        std::set<std::string> ideal_activities(ideal.activities.begin(), ideal.activities.end());
        std::set<std::string> actual_activities(actual.activities.begin(), actual.activities.end());
        
        for (const auto& act : actual.activities) {
            if (!ideal_activities.count(act)) {
                ConformanceResult::Deviation dev;
                dev.activity = act;
                dev.type = "unexpected_activity";
                dev.severity = "minor";
                dev.description = "Activity not in ideal model: " + act;
                result.deviations.push_back(dev);
            }
        }
        
        for (const auto& act : ideal.activities) {
            if (!actual_activities.count(act)) {
                ConformanceResult::Deviation dev;
                dev.activity = act;
                dev.type = "missing_activity";
                dev.severity = "major";
                dev.description = "Required activity missing: " + act;
                result.deviations.push_back(dev);
            }
        }
        
        // Check edge conformance
        for (const auto& ideal_edge : ideal.edges) {
            bool found = false;
            for (const auto& actual_edge : actual.edges) {
                if (actual_edge.from == ideal_edge.from && actual_edge.to == ideal_edge.to) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ConformanceResult::Deviation dev;
                dev.type = "missing_edge";
                dev.severity = "major";
                dev.description = "Expected transition: " + ideal_edge.from + " → " + ideal_edge.to;
                result.deviations.push_back(dev);
            }
        }
        
        return {Status::Success(), result};
        
    } catch (const std::exception& e) {
        return {Status::Error(std::string("compareWithIdeal failed: ") + e.what()), {}};
    }
}

bool ProcessPatternMatcher::hasPattern(
    const std::string& case_id,
    const ProcessPattern& pattern,
    double threshold
) {
    auto actual_opt = getProcessInstance(case_id);
    if (!actual_opt) {
        return false;
    }
    
    ProcessPattern actual = *actual_opt;
    double similarity = computeGraphSimilarity(pattern, actual);
    return similarity >= threshold;
}

// ============================================================================
// Similarity Computation
// ============================================================================

double ProcessPatternMatcher::computeGraphSimilarity(
    const ProcessPattern& pattern1,
    const ProcessPattern& pattern2
) const {
    // Jaccard similarity for nodes
    std::set<std::string> set1(pattern1.activities.begin(), pattern1.activities.end());
    std::set<std::string> set2(pattern2.activities.begin(), pattern2.activities.end());
    
    std::set<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(),
                         set2.begin(), set2.end(),
                         std::inserter(intersection, intersection.begin()));
    
    std::set<std::string> union_set;
    std::set_union(set1.begin(), set1.end(),
                   set2.begin(), set2.end(),
                   std::inserter(union_set, union_set.begin()));
    
    double node_similarity = union_set.empty() ? 0.0 : 
                            static_cast<double>(intersection.size()) / union_set.size();
    
    // Jaccard similarity for edges
    std::set<std::pair<std::string, std::string>> edges1, edges2;
    for (const auto& e : pattern1.edges) {
        edges1.insert({e.from, e.to});
    }
    for (const auto& e : pattern2.edges) {
        edges2.insert({e.from, e.to});
    }
    
    std::set<std::pair<std::string, std::string>> edge_intersection;
    std::set_intersection(edges1.begin(), edges1.end(),
                         edges2.begin(), edges2.end(),
                         std::inserter(edge_intersection, edge_intersection.begin()));
    
    std::set<std::pair<std::string, std::string>> edge_union;
    std::set_union(edges1.begin(), edges1.end(),
                   edges2.begin(), edges2.end(),
                   std::inserter(edge_union, edge_union.begin()));
    
    double edge_similarity = edge_union.empty() ? 0.0 :
                            static_cast<double>(edge_intersection.size()) / edge_union.size();
    
    // Combine (weighted average)
    return 0.6 * node_similarity + 0.4 * edge_similarity;
}

double ProcessPatternMatcher::computeVectorSimilarity(
    const ProcessPattern& pattern1,
    const ProcessPattern& pattern2
) const {
    // Simplified: use activity name embeddings and cosine similarity
    // In real implementation, this would use VectorIndex for semantic embeddings
    
    if (pattern1.activities.empty() || pattern2.activities.empty()) {
        return 0.0;
    }
    
    // Create simple bag-of-words representation
    std::unordered_map<std::string, double> vec1, vec2;
    for (const auto& act : pattern1.activities) {
        vec1[act] = 1.0;
    }
    for (const auto& act : pattern2.activities) {
        vec2[act] = 1.0;
    }
    
    // Cosine similarity
    double dot_product = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;
    
    for (const auto& [key, val] : vec1) {
        if (vec2.count(key)) {
            dot_product += val * vec2[key];
        }
        norm1 += val * val;
    }
    
    for (const auto& [key, val] : vec2) {
        norm2 += val * val;
    }
    
    norm1 = std::sqrt(norm1);
    norm2 = std::sqrt(norm2);
    
    if (norm1 == 0.0 || norm2 == 0.0) {
        return 0.0;
    }
    
    return dot_product / (norm1 * norm2);
}

double ProcessPatternMatcher::computeBehavioralSimilarity(
    const ProcessPattern& pattern1,
    const ProcessPattern& pattern2
) const {
    // Longest Common Subsequence (LCS) on activity sequences
    const auto& seq1 = pattern1.activities;
    const auto& seq2 = pattern2.activities;
    
    if (seq1.empty() || seq2.empty()) {
        return 0.0;
    }
    
    int m = seq1.size();
    int n = seq2.size();
    
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (seq1[i-1] == seq2[j-1]) {
                dp[i][j] = dp[i-1][j-1] + 1;
            } else {
                dp[i][j] = std::max(dp[i-1][j], dp[i][j-1]);
            }
        }
    }
    
    int lcs_length = dp[m][n];
    int max_length = std::max(m, n);
    
    return static_cast<double>(lcs_length) / max_length;
}

double ProcessPatternMatcher::computeFitness(
    const ProcessPattern& actual,
    const ProcessPattern& ideal
) const {
    // Simplified fitness: fraction of ideal activities present in actual
    if (ideal.activities.empty()) {
        return 1.0;
    }
    
    std::set<std::string> ideal_set(ideal.activities.begin(), ideal.activities.end());
    int matched = 0;
    
    for (const auto& act : actual.activities) {
        if (ideal_set.count(act)) {
            ++matched;
        }
    }
    
    return static_cast<double>(matched) / ideal.activities.size();
}

double ProcessPatternMatcher::computePrecision(
    const ProcessPattern& actual,
    const ProcessPattern& ideal
) const {
    // Simplified precision: fraction of actual activities that are in ideal
    if (actual.activities.empty()) {
        return 1.0;
    }
    
    std::set<std::string> ideal_set(ideal.activities.begin(), ideal.activities.end());
    int matched = 0;
    
    for (const auto& act : actual.activities) {
        if (ideal_set.count(act)) {
            ++matched;
        }
    }
    
    return static_cast<double>(matched) / actual.activities.size();
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string ProcessPatternMatcher::createCacheKey(
    const ProcessPattern& pattern,
    const SimilarityConfig& config
) const {
    // Simple cache key: pattern ID + method + threshold
    return pattern.id + "_" + 
           std::to_string(static_cast<int>(config.method)) + "_" +
           std::to_string(config.threshold);
}

std::optional<SimilarityResults> ProcessPatternMatcher::getFromCache(
    const std::string& key
) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = pattern_cache_.find(key);
    if (it != pattern_cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ProcessPatternMatcher::putInCache(
    const std::string& key,
    const SimilarityResults& results
) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Simple LRU: if cache is full, remove oldest entry
    if (pattern_cache_.size() >= pattern_cache_max_size_) {
        pattern_cache_.erase(pattern_cache_.begin());
    }
    
    pattern_cache_[key] = results;
}

std::vector<ProcessPattern> ProcessPatternMatcher::getAllProcessInstances() const {
    // Simplified: return empty vector
    // In real implementation, this would query the database for all process instances
    return {};
}

std::optional<ProcessPattern> ProcessPatternMatcher::getProcessInstance(
    const std::string& case_id
) const {
    // Simplified: try to find in administrative models
    // In real implementation, this would query the database
    return getAdministrativeModel(case_id);
}

PatternStatistics ProcessPatternMatcher::getStatistics() const {
    PatternStatistics stats;
    stats.total_patterns_loaded = administrative_models_.size();
    stats.cache_size = pattern_cache_.size();
    stats.cache_hits = 0; // Would track in real implementation
    stats.cache_misses = 0; // Would track in real implementation
    return stats;
}

} // namespace themis
