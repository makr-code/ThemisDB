/**
 * @file byzantine_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static DetectionResult fromJSON(const nlohmann::json& j);
};

// ============================================================================
// Byzantine Detector Interface
// ============================================================================

/** @brief Byzantine Detector Interface. */
class ByzantineDetector {
public:
    /**
     * @brief TBD: Describe ~ByzantineDetector.
     * @return Return value.
     */
    virtual ~ByzantineDetector() = default;
    
    // Analyze gradients from all shards
    virtual DetectionResult detectByzantineShards(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) = 0;
    
    // Compute statistics for detection
    virtual GradientStatistics computeStatistics(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    ) = 0;
    
    /**
     * @brief Get detector name
     * @return Return value.
     */
    virtual std::string getName() const = 0;
};

// ============================================================================
// Median-based Detector (MAD Threshold)
// ============================================================================

/** @brief Median-based Detector (MAD Threshold). */
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
    
    /**
     * @brief TBD: Describe setThreshold.
     * @param[in] threshold Input parameter.
     * @details Implements setThreshold without additional internal calls.
     */
    void setThreshold(float threshold) { threshold_ = threshold; }
    float getThreshold() const { return threshold_; }
    
private:
    float threshold_ = 0.0f;  // Number of MAD for outlier detection (typically 2.5-3.5)
    
    /**
     * @brief Helper methods
     * @param[in] gradients Input parameter.
     * @return Return value.
     */
    float computeL2Norm(const std::vector<GradientTensor>& gradients) const;
    /**
     * @brief TBD: Describe computeMean.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    float computeMean(const std::vector<float>& values) const;
    /**
     * @brief TBD: Describe computeMedian.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    float computeMedian(std::vector<float> values) const;
    /**
     * @brief TBD: Describe computeMAD.
     * @param[in] values Input parameter.
     * @param[in] median Input parameter.
     * @return Return value.
     */
    float computeMAD(const std::vector<float>& values, float median) const;
};

// ============================================================================
// Krum Algorithm Detector
// ============================================================================

/** @brief Krum Algorithm Detector. */
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
    
    /**
     * @brief TBD: Describe setMaxByzantineShards.
     * @param[in] f Input parameter.
     * @details Implements setMaxByzantineShards without additional internal calls.
     */
    void setMaxByzantineShards(int f) { max_byzantine_shards_ = f; }
    int getMaxByzantineShards() const { return max_byzantine_shards_; }
    
    // Public for use by BulyanDetector
    std::vector<std::string> selectKrumGradients(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients,
        int num_to_select
    ) const;
    
private:
    int max_byzantine_shards_ = 0;  // f parameter: max number of Byzantine shards
    
    /**
     * @brief Helper methods
     * @param[in] grad1 Input parameter.
     * @param[in] grad2 Input parameter.
     * @return Return value.
     */
    float computeDistance(
        const std::vector<GradientTensor>& grad1,
        const std::vector<GradientTensor>& grad2
    ) const;
};

// ============================================================================
// Bulyan Algorithm Detector
// ============================================================================

/** @brief Bulyan Algorithm Detector. */
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
    
    /**
     * @brief TBD: Describe setMaxByzantineShards.
     * @param[in] f Input parameter.
     * @details Implements setMaxByzantineShards without additional internal calls.
     */
    void setMaxByzantineShards(int f) { max_byzantine_shards_ = f; }
    int getMaxByzantineShards() const { return max_byzantine_shards_; }
    
    // Aggregate with Byzantine tolerance
    std::vector<GradientTensor> aggregateRobust(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    );
    
private:
    int max_byzantine_shards_ = 0;  // f parameter: max number of Byzantine shards
    KrumDetector krum_detector_;  // Use Krum for selection
    
    /**
     * @brief Helper methods
     * @param[in] selected_gradients Input parameter.
     * @param[in] trim_count Input parameter.
     * @return Return value.
     */
    std::vector<GradientTensor> computeTrimmedMean(
        const std::vector<std::vector<GradientTensor>>& selected_gradients,
        int trim_count
    ) const;
};

// ============================================================================
// Ensemble Detector (Combine Multiple Methods)
// ============================================================================

/** @brief Ensemble Detector (Combine Multiple Methods). */
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
    
    /**
     * @brief Combine results from multiple detectors
     * @param[in] median_result Input parameter.
     * @param[in] krum_result Input parameter.
     * @return Return value.
     */
    DetectionResult combineResults(
        const DetectionResult& median_result,
        const DetectionResult& krum_result
    ) const;
};

// ============================================================================
// Byzantine Detector Factory
// ============================================================================

/** @brief Byzantine Detector Factory. */
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
