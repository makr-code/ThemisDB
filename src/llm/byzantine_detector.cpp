/**
 * @file byzantine_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/byzantine_detector.h"
#include "llm/distributed_training_coordinator.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace themis {
namespace llm {

using json = nlohmann::json;

// ============================================================================
// GradientStatistics Implementation
// ============================================================================

json GradientStatistics::toJSON() const {
    json j;
    j["gradient_norms"] = gradient_norms;
    j["gradient_means"] = gradient_means;
    j["gradient_variances"] = gradient_variances;
    j["global_median_norm"] = global_median_norm;
    j["global_mad"] = global_mad;
    j["shard_ids"] = shard_ids;
    return j;
}

GradientStatistics GradientStatistics::fromJSON(const json& j) {
    GradientStatistics stats = {};
    if (j.contains("gradient_norms"))
        stats.gradient_norms = j["gradient_norms"].get<std::vector<float>>();
    if (j.contains("gradient_means"))
        stats.gradient_means = j["gradient_means"].get<std::vector<float>>();
    if (j.contains("gradient_variances"))
        stats.gradient_variances = j["gradient_variances"].get<std::vector<float>>();
    if (j.contains("global_median_norm"))
        stats.global_median_norm = j["global_median_norm"].get<float>();
    if (j.contains("global_mad"))
        stats.global_mad = j["global_mad"].get<float>();
    if (j.contains("shard_ids"))
        stats.shard_ids = j["shard_ids"].get<std::vector<std::string>>();
    return stats;
}

// ============================================================================
// DetectionResult Implementation
// ============================================================================

json DetectionResult::toJSON() const {
    json j;
    j["suspected_shards"] = suspected_shards;
    j["anomaly_scores"] = anomaly_scores;
    j["detection_method"] = detection_method;
    j["requires_action"] = requires_action;
    return j;
}

DetectionResult DetectionResult::fromJSON(const json& j) {
    DetectionResult result = {};
    if (j.contains("suspected_shards"))
        result.suspected_shards = j["suspected_shards"].get<std::vector<std::string>>();
    if (j.contains("anomaly_scores"))
        result.anomaly_scores = j["anomaly_scores"].get<std::map<std::string, float>>();
    if (j.contains("detection_method"))
        result.detection_method = j["detection_method"].get<std::string>();
    if (j.contains("requires_action"))
        result.requires_action = j["requires_action"].get<bool>();
    return result;
}

// ============================================================================
// MedianDetector Implementation
// ============================================================================

MedianDetector::MedianDetector([[maybe_unused]] float threshold) : threshold_(threshold) {
    if (threshold <= 0.0f) {
        throw std::invalid_argument("Threshold must be positive");
    }
}

float MedianDetector::computeL2Norm(const std::vector<GradientTensor>& gradients) const {
    float sum_squared = 0.0f;
    
    for (const auto& tensor : gradients) {
        for (float val : tensor.data) {
            sum_squared += val * val;
        }
    }
    
    return std::sqrt(sum_squared);
}

float MedianDetector::computeMean(const std::vector<float>& values) const {
    if (values.empty()) {
      return 0.0f;
    }
    
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / values.size();
}

float MedianDetector::computeMedian(std::vector<float> values) const {
    if (values.empty()) {
      return 0.0f;
    }
    
    size_t n = values.size();
    std::nth_element(values.begin(), values.begin() + n / 2, values.end());
    
    if (n % 2 == 1) {
        return values[n / 2];
    } else {
        float mid1 = values[n / 2];
        std::nth_element(values.begin(), values.begin() + n / 2 - 1, values.end());
        float mid2 = values[n / 2 - 1];
        return (mid1 + mid2) / 2.0f;
    }
}

float MedianDetector::computeMAD(const std::vector<float>& values, float median) const {
    if (values.empty()) {
      return 0.0f;
    }
    
    std::vector<float> deviations = {};

    deviations.reserve(values.size());
    
    for (float val : values) {
        deviations.push_back(std::abs(val - median));
    }
    
    return computeMedian(deviations);
}

GradientStatistics MedianDetector::computeStatistics(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    GradientStatistics stats = {};
    
    if (shard_gradients.empty()) {
        return stats;
    }
    
    // Compute L2 norms for each shard
    for (const auto& [shard_id, gradients] : shard_gradients) {
        float norm = computeL2Norm(gradients);
        stats.gradient_norms.push_back(norm);
        stats.shard_ids.push_back(shard_id);
        
        // Compute mean
        float sum = 0.0f;
        size_t count = 0;
        for (const auto& tensor : gradients) {
            sum += std::accumulate(tensor.data.begin(), tensor.data.end(), 0.0f);
            count += tensor.data.size();
        }
        float mean = (count > 0) ? (sum / count) : 0.0f;
        stats.gradient_means.push_back(mean);
        
        // Compute variance
        float sum_squared_diff = 0.0f;
        for (const auto& tensor : gradients) {
            for (float val : tensor.data) {
                float diff = val - mean;
                sum_squared_diff += diff * diff;
            }
        }
        float variance = (count > 0) ? (sum_squared_diff / count) : 0.0f;
        stats.gradient_variances.push_back(variance);
    }
    
    // Compute global statistics
    stats.global_median_norm = computeMedian(stats.gradient_norms);
    stats.global_mad = computeMAD(stats.gradient_norms, stats.global_median_norm);
    
    return stats;
}

DetectionResult MedianDetector::detectByzantineShards(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    DetectionResult result;
    result.detection_method = "MEDIAN";
    
    if (shard_gradients.size() < 2) {
        // Need at least 2 shards for comparison
        return result;
    }
    
    auto stats = computeStatistics(shard_gradients);
    
    if (stats.global_mad < 1e-10f) {
        // All gradients are too similar, no detection possible
        spdlog::debug("Byzantine detection: MAD too small, skipping detection");
        return result;
    }
    
    // Detect outliers
    for (size_t i = 0; i < stats.shard_ids.size(); ++i) {
        const std::string& shard_id = stats.shard_ids[i];
        float norm = stats.gradient_norms[i];
        float deviation = std::abs(norm - stats.global_median_norm);
        float normalized_deviation = deviation / stats.global_mad;
        
        // Compute anomaly score (0.0 to 1.0+)
        float anomaly_score = std::min(normalized_deviation / threshold_, 1.0f);
        result.anomaly_scores[shard_id] = anomaly_score;
        
        if (normalized_deviation > threshold_) {
            result.suspected_shards.push_back(shard_id);
            result.requires_action = true;
            
            spdlog::warn(
                "Byzantine detection: Shard {} has anomalous gradient norm {:.6f} "
                "(median={:.6f}, MAD={:.6f}, deviation={:.2f}x threshold)",
                shard_id, norm, stats.global_median_norm, stats.global_mad,
                normalized_deviation / threshold_
            );
        }
    }
    
    return result;
}

// ============================================================================
// KrumDetector Implementation
// ============================================================================

KrumDetector::KrumDetector([[maybe_unused]] int max_byzantine_shards) 
    : max_byzantine_shards_(max_byzantine_shards) {
    if (max_byzantine_shards < 0) {
        throw std::invalid_argument("max_byzantine_shards must be non-negative");
    }
}

float KrumDetector::computeDistance(
    const std::vector<GradientTensor>& grad1,
    const std::vector<GradientTensor>& grad2
) const {
    if (grad1.size() != grad2.size()) {
        throw std::runtime_error("Gradient tensor sizes do not match");
    }
    
    float distance = 0.0f;
    
    for (size_t i = 0; i < grad1.size(); ++i) {
        if (grad1[i].data.size() != grad2[i].data.size()) {
            throw std::runtime_error("Gradient data sizes do not match");
        }
        
        for (size_t j = 0; j < grad1[i].data.size(); ++j) {
            float diff = grad1[i].data[j] - grad2[i].data[j];
            distance += diff * diff;
        }
    }
    
    return std::sqrt(distance);
}

std::vector<std::string> KrumDetector::selectKrumGradients(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients,
    int num_to_select
) const {
    if (static_cast<int>(shard_gradients.size()) <= static_cast<size_t>(num_to_select)) {
        // Select all shards
        std::vector<std::string> all_shards = {};

        for (const auto& [shard_id, _] : shard_gradients) {
            all_shards.push_back(shard_id);
        }
        return all_shards;
    }
    
    // Compute pairwise distances
    std::vector<std::string> shard_ids = {};

    for (const auto& [shard_id, _] : shard_gradients) {
        shard_ids.push_back(shard_id);
    }
    
    size_t n = shard_ids.size();
    std::vector<std::vector<float>> distances(n, std::vector<float>(n, 0.0f));
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            float dist = computeDistance(
                shard_gradients.at(shard_ids[i]),
                shard_gradients.at(shard_ids[j])
            );
            distances[i][j] = dist;
            distances[j][i] = dist;
        }
    }
    
    // For each gradient, compute sum of distances to k closest gradients
    int k = static_cast<int>(n) - max_byzantine_shards_ - 2;  // Number of closest neighbors to consider
    k = std::max(1, k);
    
    std::vector<std::pair<float, std::string>> scores;
    
    for (size_t i = 0; i < n; ++i) {
        // Get distances from this gradient to all others
        std::vector<float> dists = distances[i];
        
        // Sort to find k closest
        std::partial_sort(dists.begin(), dists.begin() + k, dists.end());
        
        // Sum of k closest distances
        float score = std::accumulate(dists.begin(), dists.begin() + k, 0.0f);
        scores.push_back({score, shard_ids[i]});
    }
    
    // Select gradients with smallest scores
    std::partial_sort(
        scores.begin(),
        scores.begin() + num_to_select,
        scores.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; }
    );
    
    std::vector<std::string> selected = {};

    for (int i = 0; i < num_to_select; ++i) {
        selected.push_back(scores[i].second);
    }
    
    return selected;
}

GradientStatistics KrumDetector::computeStatistics(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    // Use median detector for basic statistics
    MedianDetector median_detector(3.0f);
    return median_detector.computeStatistics(shard_gradients);
}

DetectionResult KrumDetector::detectByzantineShards(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    DetectionResult result;
    result.detection_method = "KRUM";
    
    const auto n = static_cast<int>(shard_gradients.size());
    
    if (n < 2 * max_byzantine_shards_ + 3) {
        spdlog::warn(
            "Byzantine detection: Insufficient shards for Krum (n={}, f={}). "
            "Need at least 2f+3 shards.",
            n, max_byzantine_shards_
        );
        return result;
    }
    
    // Number of gradients to select
    int m = static_cast<int>(n) - max_byzantine_shards_ - 2;
    
    try {
        auto selected = selectKrumGradients(shard_gradients, m);
        
        // Mark unselected shards as suspected
        for (const auto& [shard_id, _] : shard_gradients) {
            if (std::find(selected.begin(), selected.end(), shard_id) == selected.end()) {
                result.suspected_shards.push_back(shard_id);
                result.anomaly_scores[shard_id] = 1.0f;  // Rejected by Krum
                result.requires_action = true;
            } else {
                result.anomaly_scores[shard_id] = 0.0f;  // Accepted by Krum
            }
        }
        
        if (result.requires_action) {
            spdlog::warn(
                "Byzantine detection (Krum): Detected {} suspicious shards",
                result.suspected_shards.size()
            );
        }
    } catch (const std::exception& e) {
        spdlog::error("Byzantine detection (Krum) failed: {}", e.what());
    }
    
    return result;
}

// ============================================================================
// BulyanDetector Implementation
// ============================================================================

BulyanDetector::BulyanDetector(int max_byzantine_shards)
    : max_byzantine_shards_(max_byzantine_shards),
      krum_detector_(max_byzantine_shards) {
    if (max_byzantine_shards < 0) {
        throw std::invalid_argument("max_byzantine_shards must be non-negative");
    }
}

std::vector<GradientTensor> BulyanDetector::computeTrimmedMean(
    const std::vector<std::vector<GradientTensor>>& selected_gradients,
    int trim_count
) const {
    if (selected_gradients.empty()) {
        return {};
    }
    
    size_t num_layers = selected_gradients[0].size();
    std::vector<GradientTensor> result;
    
    for (size_t layer_idx = 0; layer_idx < num_layers; ++layer_idx) {
        GradientTensor aggregated;
        aggregated.layer_name = selected_gradients[0][layer_idx].layer_name;
        aggregated.shape = selected_gradients[0][layer_idx].shape;
        
        size_t data_size = selected_gradients[0][layer_idx].data.size();
        aggregated.data.resize(data_size, 0.0f);
        
        // For each coordinate, compute trimmed mean
        for (size_t coord = 0; coord < data_size; ++coord) {
            std::vector<float> values = {};

            for (const auto& shard_grads : selected_gradients) {
                values.push_back(shard_grads[layer_idx].data[coord]);
            }
            
            // Sort and trim
            std::sort(values.begin(), values.end());
            
            // Remove top and bottom trim_count values
            size_t start = std::min<size_t>(trim_count, values.size() / 2);
            size_t end = static_cast<int>(values.size()) - start;
            
            if (start < end) {
                float sum = std::accumulate(
                    values.begin() + start,
                    values.begin() + end,
                    0.0f
                );
                aggregated.data[coord] = sum / (end - start);
            } else {
                aggregated.data[coord] = values[values.size() / 2];  // Median
            }
        }
        
        result.push_back(aggregated);
    }
    
    return result;
}

GradientStatistics BulyanDetector::computeStatistics(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    return krum_detector_.computeStatistics(shard_gradients);
}

DetectionResult BulyanDetector::detectByzantineShards(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    DetectionResult result;
    result.detection_method = "BULYAN";
    
    const auto n = static_cast<int>(shard_gradients.size());
    
    if (n < 4 * max_byzantine_shards_ + 3) {
        spdlog::warn(
            "Byzantine detection: Insufficient shards for Bulyan (n={}, f={}). "
            "Need at least 4f+3 shards.",
            n, max_byzantine_shards_
        );
        return result;
    }
    
    // Use Krum for initial selection
    auto krum_result = krum_detector_.detectByzantineShards(shard_gradients);
    
    // Bulyan provides stronger guarantees by using trimmed mean
    result.suspected_shards = krum_result.suspected_shards;
    result.anomaly_scores = krum_result.anomaly_scores;
    result.requires_action = krum_result.requires_action;
    
    if (result.requires_action) {
        spdlog::warn(
            "Byzantine detection (Bulyan): Detected {} suspicious shards",
            result.suspected_shards.size()
        );
    }
    
    return result;
}

std::vector<GradientTensor> BulyanDetector::aggregateRobust(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    const auto n = static_cast<int>(shard_gradients.size());
    const int m = n - max_byzantine_shards_ - 2;
    
    // Step 1: Use Krum to select m gradients
    auto selected_ids = krum_detector_.selectKrumGradients(shard_gradients, m);
    
    std::vector<std::vector<GradientTensor>> selected_gradients;
    for (const auto& shard_id : selected_ids) {
        selected_gradients.push_back(shard_gradients.at(shard_id));
    }
    
    // Step 2: Compute trimmed mean
    return computeTrimmedMean(selected_gradients, max_byzantine_shards_);
}

// ============================================================================
// EnsembleDetector Implementation
// ============================================================================

EnsembleDetector::EnsembleDetector(float median_threshold, int max_byzantine_shards)
    : median_detector_(median_threshold),
      krum_detector_(max_byzantine_shards) {
}

DetectionResult EnsembleDetector::combineResults(
    const DetectionResult& median_result,
    const DetectionResult& krum_result
) const {
    DetectionResult combined;
    combined.detection_method = "ENSEMBLE";
    
    // Combine anomaly scores (take maximum)
    std::map<std::string, float> all_scores = median_result.anomaly_scores;
    for (const auto& [shard_id, score] : krum_result.anomaly_scores) {
        if (all_scores.find(shard_id) != all_scores.end()) {
            all_scores[shard_id] = std::max(all_scores[shard_id], score);
        } else {
            all_scores[shard_id] = score;
        }
    }
    combined.anomaly_scores = all_scores;
    
    // A shard is suspected if detected by either method
    std::set<std::string> suspected_set = {};

    for (const auto& shard_id : median_result.suspected_shards) {
        suspected_set.insert(shard_id);
    }
    for (const auto& shard_id : krum_result.suspected_shards) {
        suspected_set.insert(shard_id);
    }
    
    combined.suspected_shards.assign(suspected_set.begin(), suspected_set.end());
    combined.requires_action = !combined.suspected_shards.empty();
    
    return combined;
}

GradientStatistics EnsembleDetector::computeStatistics(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    return median_detector_.computeStatistics(shard_gradients);
}

DetectionResult EnsembleDetector::detectByzantineShards(
    const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
) {
    auto median_result = median_detector_.detectByzantineShards(shard_gradients);
    auto krum_result = krum_detector_.detectByzantineShards(shard_gradients);
    
    return combineResults(median_result, krum_result);
}

// ============================================================================
// ByzantineDetectorFactory Implementation
// ============================================================================

std::unique_ptr<ByzantineDetector> ByzantineDetectorFactory::create(
    ByzantineDetectionMethod method,
    float threshold,
    int max_byzantine_shards
) {
    switch (method) {
        case ByzantineDetectionMethod::NONE:
            return nullptr;
            
        case ByzantineDetectionMethod::MEDIAN:
            return std::make_unique<MedianDetector>(threshold);
            
        case ByzantineDetectionMethod::KRUM:
            return std::make_unique<KrumDetector>(max_byzantine_shards);
            
        case ByzantineDetectionMethod::BULYAN:
            return std::make_unique<BulyanDetector>(max_byzantine_shards);
            
        case ByzantineDetectionMethod::ENSEMBLE:
            return std::make_unique<EnsembleDetector>(threshold, max_byzantine_shards);
            
        default:
            throw std::invalid_argument("Unknown Byzantine detection method");
    }
}

} // namespace llm
} // namespace themis
