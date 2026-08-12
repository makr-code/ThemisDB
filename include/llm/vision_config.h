/**
 * @file vision_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <shared_mutex>
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
    bool commercial_use = false;                 ///< Allowed for commercial use
    bool modification = false;                   ///< Allowed to modify
    bool distribution = false;                   ///< Allowed to distribute
    bool attribution_required = false;           ///< Attribution required
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
    virtual ~VisionModelMetadata() = default;
    std::string model_id;                ///< Unique model identifier
    std::string model_name;              ///< Human-readable name
    std::string version;                 ///< Model version
    ModelLicense license;                ///< License information
    std::string attribution;             ///< Required attribution text
    std::string citation;                ///< Citation for academic use
    std::string source_url;              ///< Source URL
    bool production_ready = false;               ///< Ready for production use
    size_t memory_requirement_mb = 0;        ///< Memory requirement
    std::vector<std::string> capabilities; ///< Supported capabilities
};

/**
 * @brief Resource limits for vision processing
 */
struct VisionResourceLimits {
    virtual ~VisionResourceLimits() = default;
    size_t max_memory_mb = 0;                ///< Maximum memory usage
    size_t max_memory_per_request_mb = 0;    ///< Memory per request
    size_t max_vram_mb = 0;                  ///< Maximum VRAM usage
    size_t max_vram_per_model_mb = 0;        ///< VRAM per model
    size_t max_concurrent_requests = 0;      ///< Max concurrent requests
    size_t max_concurrent_models = 0;        ///< Max models loaded
    size_t max_queue_size = 0;               ///< Request queue size
    std::chrono::seconds max_inference_time; ///< Max inference time
    std::chrono::seconds max_model_load_time; ///< Max model load time
    std::chrono::seconds request_timeout;     ///< Request timeout
    int cpu_inference_threads = 4;       ///< CPU threads for image encoding (clip_image_encode)
};

/**
 * @brief Rate limiting configuration
 */
struct VisionRateLimits {
    virtual ~VisionRateLimits() = default;
    bool enabled = false;                        ///< Rate limiting enabled
    size_t requests_per_minute = 0;          ///< Requests per minute
    size_t requests_per_hour = 0;            ///< Requests per hour
    size_t requests_per_day = 0;             ///< Requests per day
    size_t burst_size = 0;                   ///< Burst allowance
    std::string on_limit_exceeded;       ///< Behavior: reject, queue, throttle
};

/**
 * @brief Resource quota tracking
 */
struct VisionResourceQuota {
    virtual ~VisionResourceQuota() = default;
    bool enabled = false;                        ///< Quota enforcement enabled
    std::string enforcement;             ///< Enforcement mode: soft, hard
    size_t daily_requests = 0;               ///< Daily request quota
    size_t monthly_requests = 0;             ///< Monthly request quota
    size_t total_inference_minutes = 0;      ///< Total inference time quota
    size_t total_vram_hours = 0;             ///< GPU hours quota
    std::string reset_period;            ///< Reset period: daily, weekly, monthly
};

/**
 * @brief Monitoring configuration
 */
struct VisionMonitoringConfig {
    virtual ~VisionMonitoringConfig() = default;
    bool enabled = false;                        ///< Monitoring enabled
    bool track_latency = false;                  ///< Track latency metrics
    bool track_throughput = false;               ///< Track throughput metrics
    bool track_error_rate = false;               ///< Track error rates
    bool track_resource_usage = false;           ///< Track resource usage
    bool track_model_usage = false;              ///< Track model usage
    std::chrono::seconds collect_interval; ///< Metric collection interval
    
    // Prometheus configuration
    struct PrometheusConfig {
        bool enabled = false;
        int port = 0;
        std::string path;
        std::string namespace_prefix;
    } prometheus;
    
    // Audit configuration
    struct AuditConfig {
        bool enabled = false;
        std::vector<std::string> events;  ///< Events to audit
        std::string storage_type;         ///< Storage: database, file, syslog
        int retention_days = 0;               ///< Retention period
        std::string compliance_mode;      ///< Compliance mode
        bool include_pii = false;                 ///< Include PII in logs
    } audit;
};

/**
 * @brief Security configuration for vision processing
 */
struct VisionSecurityConfig {
    virtual ~VisionSecurityConfig() = default;
    // Input validation
    struct ValidationConfig {
        bool enabled = false;
        size_t max_image_size_mb = 0;
        std::pair<int, int> max_image_resolution;  // width, height
        std::vector<std::string> allowed_formats;
        bool validate_image_integrity = false;
        bool scan_for_malware = false;
        size_t max_prompt_length = 0;
        bool sanitize_prompts = false;
        bool block_injection_attempts = false;
    } validation;
    
    // Sandboxing
    struct SandboxConfig {
        bool enabled = false;
        std::string type;                 ///< Sandbox type: process, container, vm
        bool isolate_memory = false;
        bool isolate_network = false;
        bool isolate_filesystem = false;
        bool allow_file_read = false;
        bool allow_file_write = false;
        bool allow_network = false;
        size_t sandbox_memory_mb = 0;
        int sandbox_cpu_cores = 0;
        std::chrono::seconds sandbox_timeout;
    } sandboxing;
    
    // Model verification
    struct ModelVerificationConfig {
        bool enabled = false;
        bool verify_signatures = false;
        std::vector<std::string> trusted_publishers;
        bool verify_checksums = false;
        std::string checksum_algorithm;
        bool scan_models = false;
    } model_verification;
    
    // Access control
    struct AccessControlConfig {
        bool enabled = false;
        bool require_authentication = false;
        bool role_based_access = false;
        std::vector<std::string> allowed_roles;
        bool require_api_key = false;
        std::string api_key_header;
    } access_control;
};

/**
 * @brief Pipeline configuration
 */
struct VisionPipelineConfig {
    virtual ~VisionPipelineConfig() = default;
    std::string stability;               ///< Stability level: development, staging, production
    
    // Error handling
    struct ErrorHandlingConfig {
        std::string strategy;            ///< Strategy: fail_fast, graceful, retry
        bool retry_enabled = false;
        int max_retry_attempts = 0;
        std::string backoff_strategy;    ///< Backoff: linear, exponential, fixed
        std::chrono::milliseconds initial_delay;
        std::chrono::milliseconds max_delay;
        bool use_cpu_fallback = false;
        bool use_smaller_model = false;
        bool return_error_response = false;
    } error_handling;
    
    // Preprocessing
    struct PreprocessingConfig {
        bool enabled = false;
        std::string resize_strategy;     ///< Strategy: fixed, adaptive, none
        bool normalize = false;
        bool augmentation = false;
        bool cache_preprocessed = false;
        std::chrono::seconds cache_ttl;
    } preprocessing;
    
    // Postprocessing
    struct PostprocessingConfig {
        bool enabled = false;
        std::string format;              ///< Format: json, protobuf
        bool include_metadata = false;
        bool include_timings = false;
        bool include_confidence_scores = false;
        float min_confidence_threshold = 0.0f;
        int max_results = 0;
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
     * @brief Load configuration from a YAML file into a fully initialized object.
     * @param config_path Path to the YAML configuration document.
     * @return Shared configuration instance with defaults applied to missing
     *         fields.
     * @throws std::runtime_error When YAML parsing fails.
     * @note The returned shared pointer is published only after construction has
     *       completed, so callers never observe partially initialized state.
     */
    static std::shared_ptr<VisionConfig> loadFromFile(const std::string& config_path);
    
    /**
     * @brief Load configuration from a JSON object into a fully initialized object.
     * @param config JSON object containing configuration overrides.
     * @return Shared configuration instance with defaults preserved for omitted
     *         fields.
     * @note The returned shared pointer is published only after construction has
     *       completed, so callers never observe partially initialized state.
     */
    static std::shared_ptr<VisionConfig> loadFromJson(const nlohmann::json& config);
    
    /**
     * @brief Get the default configuration instance.
     * @return Shared configuration instance containing production defaults.
     * @note The returned shared pointer is published only after construction has
     *       completed, so callers never observe partially initialized state.
     */
    static std::shared_ptr<VisionConfig> getDefault();
    
    /**
     * @brief Validate configuration
     */
    bool validate(std::string& error_message) const;
    
    // API Configuration
    VisionAPIStability getAPIStability() const;
    const std::string& getAPIVersion() const;
    const std::string& getAPIPrefix() const;
    bool isBackwardCompatible() const;
    
    // License Management
    bool isLicenseEnforced() const;
    bool isLicenseAllowed(const std::string& license_id) const;
    std::shared_ptr<ModelLicense> getModelLicense(const std::string& model_id) const;
    bool validateModelUsage(const std::string& model_id, bool is_commercial) const;
    std::string getRequiredAttribution(const std::string& model_id) const;
    
    // Resource Management
    const VisionResourceLimits& getResourceLimits() const;
    const VisionRateLimits& getRateLimits() const;
    const VisionResourceQuota& getResourceQuota() const;
    
    // Monitoring
    const VisionMonitoringConfig& getMonitoringConfig() const;
    bool isMonitoringEnabled() const;
    bool isAuditEnabled() const;
    
    // Security
    const VisionSecurityConfig& getSecurityConfig() const;
    bool isSandboxingEnabled() const;
    bool isModelVerificationEnabled() const;
    
    // Pipeline
    const VisionPipelineConfig& getPipelineConfig() const;
    
    // Model Registry
    std::vector<std::string> getAvailableModels() const;
    std::shared_ptr<VisionModelMetadata> getModelMetadata(const std::string& model_id) const;
    bool isModelProductionReady(const std::string& model_id) const;
    
    // Feature Flags
    bool isFeatureEnabled(const std::string& feature_name) const;
    bool isExperimentalFeature(const std::string& feature_name) const;

private:
    VisionConfig() = default;
    
    // Thread safety
    mutable std::shared_mutex config_mutex_;
    
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
