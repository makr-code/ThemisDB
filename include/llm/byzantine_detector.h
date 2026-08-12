/**
 * @file byzantine_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

// Forward declaration
namespace themis {
namespace llm {

struct GradientTensor;

// ============================================================================
// Byzantine Detection Configuration
// ============================================================================

enum class ByzantineDetectionMethod {
    NONE,        // No detection
    MEDIAN,      // Median + MAD (Median Absolute Deviation)
    KRUM,        // Krum algorithm
    BULYAN,      // Bulyan algorithm  
    ENSEMBLE     // Combine multiple methods
};

enum class ByzantineAction {
    WARN,        // Log warning, continue
    EXCLUDE,     // Exclude suspected shards from aggregation
    CLIP,        // Clip gradients to median ± k*MAD
    SHUTDOWN     // Stop training
};

// ============================================================================
// Gradient Statistics (Per-Shard Analysis)
// ============================================================================

struct GradientStatistics {
    std::vector<float> gradient_norms;       // L2 norm per shard
    std::vector<float> gradient_means;       // Mean per shard
    std::vector<float> gradient_variances;   // Variance per shard
    float global_median_norm = 0.0f;
    float global_mad = 0.0f;                 // Median Absolute Deviation
    
    // Shard IDs corresponding to statistics
    std::vector<std::string> shard_ids;
    
    nlohmann::json toJSON() const;
    static GradientStatistics fromJSON(const nlohmann::json& j);
};

// ============================================================================
// Detection Result
// ============================================================================

struct DetectionResult {
    std::vector<std::string> suspected_shards;
    std::map<std::string, float> anomaly_scores;  // 0.0 = normal, 1.0 = highly anomalous
    std::string detection_method;
    bool requires_action = false;
    
    nlohmann::json toJSON() const;
    static DetectionResult fromJSON(const nlohmann::json& j);
};

// ============================================================================
// Byzantine Detector Interface
// ============================================================================

class ByzantineDetector {
public:
    virtual ~ByzantineDetector() = default;
    
    // Analyze gradients from all shards
    virtual DetectionResult detectByzantineShards(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) = 0;
    
    // Compute statistics for detection
    virtual GradientStatistics computeStatistics(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) = 0;
    
    // Get detector name
    virtual std::string getName() const = 0;
};

// ============================================================================
// Median-based Detector (MAD Threshold)
// ============================================================================

class MedianDetector : public ByzantineDetector {
public:
    explicit MedianDetector(float threshold = 3.0f);
    ~MedianDetector() override = default;
    
    DetectionResult detectByzantineShards(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    GradientStatistics computeStatistics(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    std::string getName() const override { return "MEDIAN"; }
    
    void setThreshold(float threshold) { threshold_ = threshold; }
    float getThreshold() const { return threshold_; }
    
private:
    float threshold_ = 0.0f;  // Number of MAD for outlier detection (typically 2.5-3.5)
    
    // Helper methods
    float computeL2Norm(const std::vector<GradientTensor>& gradients) const;
    float computeMean(const std::vector<float>& values) const;
    float computeMedian(std::vector<float> values) const;
    float computeMAD(const std::vector<float>& values, float median) const;
};

// ============================================================================
// Krum Algorithm Detector
// ============================================================================

class KrumDetector : public ByzantineDetector {
public:
    explicit KrumDetector(int max_byzantine_shards = 1);
    ~KrumDetector() override = default;
    
    DetectionResult detectByzantineShards(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    GradientStatistics computeStatistics(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    std::string getName() const override { return "KRUM"; }
    
    void setMaxByzantineShards(int f) { max_byzantine_shards_ = f; }
    int getMaxByzantineShards() const { return max_byzantine_shards_; }
    
    // Public for use by BulyanDetector
    std::vector<std::string> selectKrumGradients(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients,
        int num_to_select
    ) const;
    
private:
    int max_byzantine_shards_ = 0;  // f parameter: max number of Byzantine shards
    
    // Helper methods
    float computeDistance(
        const std::vector<GradientTensor>& grad1,
        const std::vector<GradientTensor>& grad2
    ) const;
};

// ============================================================================
// Bulyan Algorithm Detector
// ============================================================================

class BulyanDetector : public ByzantineDetector {
public:
    explicit BulyanDetector(int max_byzantine_shards = 1);
    ~BulyanDetector() override = default;
    
    DetectionResult detectByzantineShards(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    GradientStatistics computeStatistics(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    std::string getName() const override { return "BULYAN"; }
    
    void setMaxByzantineShards(int f) { max_byzantine_shards_ = f; }
    int getMaxByzantineShards() const { return max_byzantine_shards_; }
    
    // Aggregate with Byzantine tolerance
    std::vector<GradientTensor> aggregateRobust(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    );
    
private:
    int max_byzantine_shards_ = 0;  // f parameter: max number of Byzantine shards
    KrumDetector krum_detector_;  // Use Krum for selection
    
    // Helper methods
    std::vector<GradientTensor> computeTrimmedMean(
        const std::vector<std::vector<GradientTensor>>& selected_gradients,
        int trim_count
    ) const;
};

// ============================================================================
// Ensemble Detector (Combine Multiple Methods)
// ============================================================================

class EnsembleDetector : public ByzantineDetector {
public:
    explicit EnsembleDetector(
        float median_threshold = 3.0f,
        int max_byzantine_shards = 1
    );
    ~EnsembleDetector() override = default;
    
    DetectionResult detectByzantineShards(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    GradientStatistics computeStatistics(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) override;
    
    std::string getName() const override { return "ENSEMBLE"; }
    
private:
    MedianDetector median_detector_;
    KrumDetector krum_detector_;
    
    // Combine results from multiple detectors
    DetectionResult combineResults(
        const DetectionResult& median_result,
        const DetectionResult& krum_result
    ) const;
};

// ============================================================================
// Byzantine Detector Factory
// ============================================================================

class ByzantineDetectorFactory {
public:
    static std::unique_ptr<ByzantineDetector> create(
        ByzantineDetectionMethod method,
        float threshold = 3.0f,
        int max_byzantine_shards = 1
    );
};

} // namespace llm
} // namespace themis
