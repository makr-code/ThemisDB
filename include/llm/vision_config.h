/*
 * ThemisDB | File: vision_config.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/**
 * @brief API stability level for vision features
 */
enum class VisionAPIStability {
    EXPERIMENTAL,    ///< Experimental features, may change
    BETA,           ///< Beta features, mostly stable
    STABLE,         ///< Stable production features
    DEPRECATED      ///< Deprecated, will be removed
};

/**
 * @brief Model license information
 */
struct ModelLicense {
    std::string license_id;              ///< License identifier (e.g., "MIT", "Apache-2.0")
    std::string license_name;            ///< Human-readable license name
    std::string license_url;             ///< URL to full license text
    bool commercial_use;                 ///< Allowed for commercial use
    bool modification;                   ///< Allowed to modify
    bool distribution;                   ///< Allowed to distribute
    bool attribution_required;           ///< Attribution required
    std::vector<std::string> restrictions; ///< Additional restrictions
    
    /**
     * @brief Check if license is compatible with another
     */
    bool isCompatibleWith(const std::string& other_license_id) const;
    
    /**
     * @brief Validate usage against license terms
     */
    bool validateUsage(bool is_commercial, bool will_modify, bool will_distribute) const;
};

/**
 * @brief Model metadata with license information
 */
struct VisionModelMetadata {
    std::string model_id;                ///< Unique model identifier
    std::string model_name;              ///< Human-readable name
    std::string version;                 ///< Model version
    ModelLicense license;                ///< License information
    std::string attribution;             ///< Required attribution text
    std::string citation;                ///< Citation for academic use
    std::string source_url;              ///< Source URL
    bool production_ready;               ///< Ready for production use
    size_t memory_requirement_mb;        ///< Memory requirement
    std::vector<std::string> capabilities; ///< Supported capabilities
};

/**
 * @brief Resource limits for vision processing
 */
struct VisionResourceLimits {
    size_t max_memory_mb;                ///< Maximum memory usage
    size_t max_memory_per_request_mb;    ///< Memory per request
    size_t max_vram_mb;                  ///< Maximum VRAM usage
    size_t max_vram_per_model_mb;        ///< VRAM per model
    size_t max_concurrent_requests;      ///< Max concurrent requests
    size_t max_concurrent_models;        ///< Max models loaded
    size_t max_queue_size;               ///< Request queue size
    std::chrono::seconds max_inference_time; ///< Max inference time
    std::chrono::seconds max_model_load_time; ///< Max model load time
    std::chrono::seconds request_timeout;     ///< Request timeout
    int cpu_inference_threads = 4;       ///< CPU threads for image encoding (clip_image_encode)
};

/**
 * @brief Rate limiting configuration
 */
struct VisionRateLimits {
    bool enabled;                        ///< Rate limiting enabled
    size_t requests_per_minute;          ///< Requests per minute
    size_t requests_per_hour;            ///< Requests per hour
    size_t requests_per_day;             ///< Requests per day
    size_t burst_size;                   ///< Burst allowance
    std::string on_limit_exceeded;       ///< Behavior: reject, queue, throttle
};

/**
 * @brief Resource quota tracking
 */
struct VisionResourceQuota {
    bool enabled;                        ///< Quota enforcement enabled
    std::string enforcement;             ///< Enforcement mode: soft, hard
    size_t daily_requests;               ///< Daily request quota
    size_t monthly_requests;             ///< Monthly request quota
    size_t total_inference_minutes;      ///< Total inference time quota
    size_t total_vram_hours;             ///< GPU hours quota
    std::string reset_period;            ///< Reset period: daily, weekly, monthly
};

/**
 * @brief Monitoring configuration
 */
struct VisionMonitoringConfig {
    bool enabled;                        ///< Monitoring enabled
    bool track_latency;                  ///< Track latency metrics
    bool track_throughput;               ///< Track throughput metrics
    bool track_error_rate;               ///< Track error rates
    bool track_resource_usage;           ///< Track resource usage
    bool track_model_usage;              ///< Track model usage
    std::chrono::seconds collect_interval; ///< Metric collection interval
    
    // Prometheus configuration
    struct PrometheusConfig {
        bool enabled;
        int port;
        std::string path;
        std::string namespace_prefix;
    } prometheus;
    
    // Audit configuration
    struct AuditConfig {
        bool enabled;
        std::vector<std::string> events;  ///< Events to audit
        std::string storage_type;         ///< Storage: database, file, syslog
        int retention_days;               ///< Retention period
        std::string compliance_mode;      ///< Compliance mode
        bool include_pii;                 ///< Include PII in logs
    } audit;
};

/**
 * @brief Security configuration for vision processing
 */
struct VisionSecurityConfig {
    // Input validation
    struct ValidationConfig {
        bool enabled;
        size_t max_image_size_mb;
        std::pair<int, int> max_image_resolution;  // width, height
        std::vector<std::string> allowed_formats;
        bool validate_image_integrity;
        bool scan_for_malware;
        size_t max_prompt_length;
        bool sanitize_prompts;
        bool block_injection_attempts;
    } validation;
    
    // Sandboxing
    struct SandboxConfig {
        bool enabled;
        std::string type;                 ///< Sandbox type: process, container, vm
        bool isolate_memory;
        bool isolate_network;
        bool isolate_filesystem;
        bool allow_file_read;
        bool allow_file_write;
        bool allow_network;
        size_t sandbox_memory_mb;
        int sandbox_cpu_cores;
        std::chrono::seconds sandbox_timeout;
    } sandboxing;
    
    // Model verification
    struct ModelVerificationConfig {
        bool enabled;
        bool verify_signatures;
        std::vector<std::string> trusted_publishers;
        bool verify_checksums;
        std::string checksum_algorithm;
        bool scan_models;
    } model_verification;
    
    // Access control
    struct AccessControlConfig {
        bool enabled;
        bool require_authentication;
        bool role_based_access;
        std::vector<std::string> allowed_roles;
        bool require_api_key;
        std::string api_key_header;
    } access_control;
};

/**
 * @brief Pipeline configuration
 */
struct VisionPipelineConfig {
    std::string stability;               ///< Stability level: development, staging, production
    
    // Error handling
    struct ErrorHandlingConfig {
        std::string strategy;            ///< Strategy: fail_fast, graceful, retry
        bool retry_enabled;
        int max_retry_attempts;
        std::string backoff_strategy;    ///< Backoff: linear, exponential, fixed
        std::chrono::milliseconds initial_delay;
        std::chrono::milliseconds max_delay;
        bool use_cpu_fallback;
        bool use_smaller_model;
        bool return_error_response;
    } error_handling;
    
    // Preprocessing
    struct PreprocessingConfig {
        bool enabled;
        std::string resize_strategy;     ///< Strategy: fixed, adaptive, none
        bool normalize;
        bool augmentation;
        bool cache_preprocessed;
        std::chrono::seconds cache_ttl;
    } preprocessing;
    
    // Postprocessing
    struct PostprocessingConfig {
        bool enabled;
        std::string format;              ///< Format: json, protobuf
        bool include_metadata;
        bool include_timings;
        bool include_confidence_scores;
        float min_confidence_threshold;
        int max_results;
    } postprocessing;
};

/**
 * @brief Main vision configuration class
 * 
 * Manages all configuration aspects for vision/multi-modal support including:
 * - API versioning and stability
 * - License management
 * - Resource limits and quotas
 * - Monitoring and audit logging
 * - Security and sandboxing
 * - Pipeline configuration
 */
class VisionConfig {
public:
    /**
     * @brief Load configuration from YAML file
     */
    static std::shared_ptr<VisionConfig> loadFromFile(const std::string& config_path);
    
    /**
     * @brief Load configuration from JSON
     */
    static std::shared_ptr<VisionConfig> loadFromJson(const nlohmann::json& config);
    
    /**
     * @brief Get default configuration
     */
    static std::shared_ptr<VisionConfig> getDefault();
    
    /**
     * @brief Validate configuration
     */
    bool validate(std::string& error_message) const;
    
    // API Configuration
    VisionAPIStability getAPIStability() const { return api_stability_; }
    const std::string& getAPIVersion() const { return api_version_; }
    const std::string& getAPIPrefix() const { return api_prefix_; }
    bool isBackwardCompatible() const { return backward_compatible_; }
    
    // License Management
    bool isLicenseEnforced() const { return enforce_licenses_; }
    bool isLicenseAllowed(const std::string& license_id) const;
    std::shared_ptr<ModelLicense> getModelLicense(const std::string& model_id) const;
    bool validateModelUsage(const std::string& model_id, bool is_commercial) const;
    std::string getRequiredAttribution(const std::string& model_id) const;
    
    // Resource Management
    const VisionResourceLimits& getResourceLimits() const { return resource_limits_; }
    const VisionRateLimits& getRateLimits() const { return rate_limits_; }
    const VisionResourceQuota& getResourceQuota() const { return resource_quota_; }
    
    // Monitoring
    const VisionMonitoringConfig& getMonitoringConfig() const { return monitoring_config_; }
    bool isMonitoringEnabled() const { return monitoring_config_.enabled; }
    bool isAuditEnabled() const { return monitoring_config_.audit.enabled; }
    
    // Security
    const VisionSecurityConfig& getSecurityConfig() const { return security_config_; }
    bool isSandboxingEnabled() const { return security_config_.sandboxing.enabled; }
    bool isModelVerificationEnabled() const { return security_config_.model_verification.enabled; }
    
    // Pipeline
    const VisionPipelineConfig& getPipelineConfig() const { return pipeline_config_; }
    
    // Model Registry
    std::vector<std::string> getAvailableModels() const;
    std::shared_ptr<VisionModelMetadata> getModelMetadata(const std::string& model_id) const;
    bool isModelProductionReady(const std::string& model_id) const;
    
    // Feature Flags
    bool isFeatureEnabled(const std::string& feature_name) const;
    bool isExperimentalFeature(const std::string& feature_name) const;

private:
    VisionConfig() = default;
    
    // Configuration data
    VisionAPIStability api_stability_ = VisionAPIStability::STABLE;
    std::string api_version_ = "1.0.0";
    std::string api_prefix_ = "/api/v1/vision";
    bool backward_compatible_ = true;
    
    bool enforce_licenses_ = true;
    std::unordered_map<std::string, std::shared_ptr<ModelLicense>> licenses_;
    std::unordered_map<std::string, std::shared_ptr<VisionModelMetadata>> models_;
    std::vector<std::string> allowed_licenses_;
    
    VisionResourceLimits resource_limits_;
    VisionRateLimits rate_limits_;
    VisionResourceQuota resource_quota_;
    VisionMonitoringConfig monitoring_config_;
    VisionSecurityConfig security_config_;
    VisionPipelineConfig pipeline_config_;
    
    std::unordered_map<std::string, bool> feature_flags_;
    std::unordered_map<std::string, bool> experimental_features_;
};

} // namespace llm
} // namespace themis
