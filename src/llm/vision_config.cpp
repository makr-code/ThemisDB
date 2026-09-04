/**
 * @file vision_config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=63, H=8, M=33, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/vision_config.h"
#include "utils/logger.h"
#include <yaml-cpp/yaml.h>
#include <atomic>
#include <fstream>
#include <sstream>

namespace themis {
namespace llm {

namespace {

std::shared_ptr<VisionConfig> publishVisionConfig(std::shared_ptr<VisionConfig> vision_config) {
    // The factory populates a freshly created VisionConfig that is not shared
    // with other threads until the shared_ptr is returned to the caller.
    // Publish that hand-off with release semantics so readers observe the
    // fully initialized configuration after they acquire the shared pointer.
    std::atomic_thread_fence(std::memory_order_release);
    return vision_config;
}

} // namespace

// =====================================================
// ModelLicense Implementation
// =====================================================

bool ModelLicense::isCompatibleWith(const std::string& other_license_id) const {
    // Compatibility matrix - can be expanded
    static const std::unordered_map<std::string, std::vector<std::string>> compatibility = {
        {"MIT", {"MIT", "Apache-2.0", "BSD-3-Clause", "CC-BY-4.0", "Llama-2-Community", "OpenRAIL-M"}},
        {"Apache-2.0", {"MIT", "Apache-2.0", "BSD-3-Clause", "CC-BY-4.0", "Llama-2-Community"}},
        {"BSD-3-Clause", {"MIT", "Apache-2.0", "BSD-3-Clause", "CC-BY-4.0"}},
        {"Llama-2-Community", {"MIT", "Apache-2.0", "BSD-3-Clause", "Llama-2-Community"}},
        {"OpenRAIL-M", {"MIT", "Apache-2.0", "BSD-3-Clause", "CC-BY-4.0", "OpenRAIL-M", "OpenRAIL++"}}
    };
    
    auto it = compatibility.find(license_id);
    if (it != compatibility.end()) {
        const auto& compatible_licenses = it->second;
        return std::find(compatible_licenses.begin(), compatible_licenses.end(), other_license_id) 
               != compatible_licenses.end();
    }
    
    // Unknown licenses are not compatible by default
    return false;
}

bool ModelLicense::validateUsage(bool is_commercial, bool will_modify, bool will_distribute) const {
    if (is_commercial && !commercial_use) {
        spdlog::warn("License {} does not allow commercial use", license_id);
        return false;
    }
    
    if (will_modify && !modification) {
        spdlog::warn("License {} does not allow modification", license_id);
        return false;
    }
    
    if (will_distribute && !distribution) {
        spdlog::warn("License {} does not allow distribution", license_id);
        return false;
    }
    
    return true;
}

// =====================================================
// VisionConfig Implementation
// =====================================================

std::shared_ptr<VisionConfig> VisionConfig::loadFromFile(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);

        // VisionConfig has a private constructor; construct directly here.
        auto vision_config = std::shared_ptr<VisionConfig>(new VisionConfig());
        
        // Lock the config during initialization to prevent data races
        std::unique_lock<std::shared_mutex> lock(vision_config->config_mutex_);

        // Load API configuration
        if (config["vision"]["api"]) {
            auto api = config["vision"]["api"];
            vision_config->api_version_ = api["version"].as<std::string>("1.0.0");
            
            if (api["stability"]["level"]) {
                std::string level = api["stability"]["level"].as<std::string>("stable");
                if (level == "experimental") {
                    vision_config->api_stability_ = VisionAPIStability::EXPERIMENTAL;
                } else if (level == "beta") {
                    vision_config->api_stability_ = VisionAPIStability::BETA;
                } else if (level == "stable") {
                    vision_config->api_stability_ = VisionAPIStability::STABLE;
                } else if (level == "deprecated") {
                    vision_config->api_stability_ = VisionAPIStability::DEPRECATED;
                }
            }
            
            if (api["versioning"]) {
                vision_config->backward_compatible_ = api["versioning"]["backward_compatible"].as<bool>(true);
                vision_config->api_prefix_ = api["versioning"]["api_prefix"].as<std::string>("/api/v1/vision");
            }
        }
        
        // Load license configuration
        if (config["vision"]["licensing"]) {
            auto licensing = config["vision"]["licensing"];
            vision_config->enforce_licenses_ = licensing["enforce_licenses"].as<bool>(true);
            
            if (licensing["allowed_licenses"]) {
                for (const auto& license : licensing["allowed_licenses"]) {
                    vision_config->allowed_licenses_.push_back(license.as<std::string>());
                }
            }
        }
        
        // Load resource limits
        if (config["vision"]["resources"]["limits"]) {
            auto limits = config["vision"]["resources"]["limits"];
            vision_config->resource_limits_.max_memory_mb = limits["max_memory_mb"].as<size_t>(8192);
            vision_config->resource_limits_.max_memory_per_request_mb = limits["max_memory_per_request_mb"].as<size_t>(2048);
            vision_config->resource_limits_.max_vram_mb = limits["max_vram_mb"].as<size_t>(16384);
            vision_config->resource_limits_.max_vram_per_model_mb = limits["max_vram_per_model_mb"].as<size_t>(8192);
            vision_config->resource_limits_.max_concurrent_requests = limits["max_concurrent_requests"].as<size_t>(16);
            vision_config->resource_limits_.max_concurrent_models = limits["max_concurrent_models"].as<size_t>(4);
            vision_config->resource_limits_.max_queue_size = limits["max_queue_size"].as<size_t>(100);
            vision_config->resource_limits_.max_inference_time = std::chrono::seconds(limits["max_inference_time_seconds"].as<int>(60));
            vision_config->resource_limits_.max_model_load_time = std::chrono::seconds(limits["max_model_load_time_seconds"].as<int>(120));
            vision_config->resource_limits_.request_timeout = std::chrono::seconds(limits["request_timeout_seconds"].as<int>(90));
        }
        
        // Load rate limiting
        if (config["vision"]["resources"]["rate_limiting"]) {
            auto rate_limiting = config["vision"]["resources"]["rate_limiting"];
            vision_config->rate_limits_.enabled = rate_limiting["enabled"].as<bool>(true);
            
            if (rate_limiting["global"]) {
                auto global = rate_limiting["global"];
                vision_config->rate_limits_.requests_per_minute = global["requests_per_minute"].as<size_t>(120);
                vision_config->rate_limits_.requests_per_hour = global["requests_per_hour"].as<size_t>(3600);
                vision_config->rate_limits_.requests_per_day = global["requests_per_day"].as<size_t>(50000);
            }
            
            vision_config->rate_limits_.burst_size = rate_limiting["burst_size"].as<size_t>(20);
            vision_config->rate_limits_.on_limit_exceeded = rate_limiting["on_limit_exceeded"].as<std::string>("reject");
        }
        
        // Load quotas
        if (config["vision"]["resources"]["quotas"]) {
            auto quotas = config["vision"]["resources"]["quotas"];
            vision_config->resource_quota_.enabled = quotas["enabled"].as<bool>(true);
            vision_config->resource_quota_.enforcement = quotas["enforcement"].as<std::string>("soft");
            
            if (quotas["default"]) {
                auto defaults = quotas["default"];
                vision_config->resource_quota_.daily_requests = defaults["daily_requests"].as<size_t>(10000);
                vision_config->resource_quota_.monthly_requests = defaults["monthly_requests"].as<size_t>(300000);
                vision_config->resource_quota_.total_inference_minutes = defaults["total_inference_time_minutes"].as<size_t>(600);
                vision_config->resource_quota_.total_vram_hours = defaults["total_vram_hours"].as<size_t>(100);
            }
            
            vision_config->resource_quota_.reset_period = quotas["reset_period"].as<std::string>("monthly");
        }
        
        // Load monitoring configuration
        if (config["vision"]["monitoring"]) {
            auto monitoring = config["vision"]["monitoring"];
            vision_config->monitoring_config_.enabled = monitoring["enabled"].as<bool>(true);
            
            if (monitoring["metrics"]) {
                auto metrics = monitoring["metrics"];
                vision_config->monitoring_config_.track_latency = metrics["track_latency"].as<bool>(true);
                vision_config->monitoring_config_.track_throughput = metrics["track_throughput"].as<bool>(true);
                vision_config->monitoring_config_.track_error_rate = metrics["track_error_rate"].as<bool>(true);
                vision_config->monitoring_config_.track_resource_usage = metrics["track_resource_usage"].as<bool>(true);
                vision_config->monitoring_config_.track_model_usage = metrics["track_model_usage"].as<bool>(true);
                vision_config->monitoring_config_.collect_interval = std::chrono::seconds(metrics["collect_interval_seconds"].as<int>(10));
                
                if (metrics["prometheus"]) {
                    auto prometheus = metrics["prometheus"];
                    vision_config->monitoring_config_.prometheus.enabled = prometheus["enabled"].as<bool>(true);
                    vision_config->monitoring_config_.prometheus.port = prometheus["port"].as<int>(9092);
                    vision_config->monitoring_config_.prometheus.path = prometheus["path"].as<std::string>("/metrics");
                    vision_config->monitoring_config_.prometheus.namespace_prefix = prometheus["namespace"].as<std::string>("themisdb_vision");
                }
            }
            
            if (monitoring["audit"]) {
                auto audit = monitoring["audit"];
                vision_config->monitoring_config_.audit.enabled = audit["enabled"].as<bool>(true);
                
                if (audit["events"]) {
                    for (const auto& event : audit["events"]) {
                        vision_config->monitoring_config_.audit.events.push_back(event.as<std::string>());
                    }
                }
                
                if (audit["storage"]) {
                    auto storage = audit["storage"];
                    vision_config->monitoring_config_.audit.storage_type = storage["type"].as<std::string>("database");
                    vision_config->monitoring_config_.audit.retention_days = storage["retention_days"].as<int>(90);
                }
                
                vision_config->monitoring_config_.audit.compliance_mode = audit["compliance_mode"].as<std::string>("standard");
                vision_config->monitoring_config_.audit.include_pii = audit["include_pii"].as<bool>(false);
            }
        }
        
        // Load security configuration
        if (config["vision"]["security"]) {
            auto security = config["vision"]["security"];
            
            // Input validation
            if (security["validation"]) {
                auto validation = security["validation"];
                vision_config->security_config_.validation.enabled = validation["enabled"].as<bool>(true);
                vision_config->security_config_.validation.max_image_size_mb = validation["max_image_size_mb"].as<size_t>(25);
                
                if (validation["max_image_resolution"]) {
                    auto res = validation["max_image_resolution"];
                    vision_config->security_config_.validation.max_image_resolution = {
                        res[0].as<int>(4096),
                        res[1].as<int>(4096)
                    };
                }
                
                if (validation["allowed_formats"]) {
                    for (const auto& format : validation["allowed_formats"]) {
                        vision_config->security_config_.validation.allowed_formats.push_back(format.as<std::string>());
                    }
                }
                
                vision_config->security_config_.validation.validate_image_integrity = validation["validate_image_integrity"].as<bool>(true);
                vision_config->security_config_.validation.scan_for_malware = validation["scan_for_malware"].as<bool>(false);
                vision_config->security_config_.validation.max_prompt_length = validation["max_prompt_length"].as<size_t>(2048);
                vision_config->security_config_.validation.sanitize_prompts = validation["sanitize_prompts"].as<bool>(true);
                vision_config->security_config_.validation.block_injection_attempts = validation["block_injection_attempts"].as<bool>(true);
            }
            
            // Sandboxing
            if (security["sandboxing"]) {
                auto sandboxing = security["sandboxing"];
                vision_config->security_config_.sandboxing.enabled = sandboxing["enabled"].as<bool>(false);
                vision_config->security_config_.sandboxing.type = sandboxing["type"].as<std::string>("container");
                vision_config->security_config_.sandboxing.isolate_memory = sandboxing["isolate_memory"].as<bool>(true);
                vision_config->security_config_.sandboxing.isolate_network = sandboxing["isolate_network"].as<bool>(true);
                vision_config->security_config_.sandboxing.isolate_filesystem = sandboxing["isolate_filesystem"].as<bool>(true);
                vision_config->security_config_.sandboxing.allow_file_read = sandboxing["allow_file_read"].as<bool>(false);
                vision_config->security_config_.sandboxing.allow_file_write = sandboxing["allow_file_write"].as<bool>(false);
                vision_config->security_config_.sandboxing.allow_network = sandboxing["allow_network"].as<bool>(false);
                vision_config->security_config_.sandboxing.sandbox_memory_mb = sandboxing["sandbox_memory_mb"].as<size_t>(4096);
                vision_config->security_config_.sandboxing.sandbox_cpu_cores = sandboxing["sandbox_cpu_cores"].as<int>(2);
                vision_config->security_config_.sandboxing.sandbox_timeout = std::chrono::seconds(sandboxing["sandbox_timeout_seconds"].as<int>(120));
            }
            
            // Model verification
            if (security["model_verification"]) {
                auto model_verification = security["model_verification"];
                vision_config->security_config_.model_verification.enabled = model_verification["enabled"].as<bool>(true);
                vision_config->security_config_.model_verification.verify_signatures = model_verification["verify_signatures"].as<bool>(true);
                
                if (model_verification["trusted_publishers"]) {
                    for (const auto& publisher : model_verification["trusted_publishers"]) {
                        vision_config->security_config_.model_verification.trusted_publishers.push_back(publisher.as<std::string>());
                    }
                }
                
                vision_config->security_config_.model_verification.verify_checksums = model_verification["verify_checksums"].as<bool>(true);
                vision_config->security_config_.model_verification.checksum_algorithm = model_verification["checksum_algorithm"].as<std::string>("SHA256");
                vision_config->security_config_.model_verification.scan_models = model_verification["scan_models"].as<bool>(false);
            }
            
            // Access control
            if (security["access_control"]) {
                auto access_control = security["access_control"];
                vision_config->security_config_.access_control.enabled = access_control["enabled"].as<bool>(true);
                vision_config->security_config_.access_control.require_authentication = access_control["require_authentication"].as<bool>(true);
                vision_config->security_config_.access_control.role_based_access = access_control["role_based_access"].as<bool>(true);
                
                if (access_control["allowed_roles"]) {
                    for (const auto& role : access_control["allowed_roles"]) {
                        vision_config->security_config_.access_control.allowed_roles.push_back(role.as<std::string>());
                    }
                }
                
                vision_config->security_config_.access_control.require_api_key = access_control["require_api_key"].as<bool>(false);
                vision_config->security_config_.access_control.api_key_header = access_control["api_key_header"].as<std::string>("X-ThemisDB-Vision-API-Key");
            }
        }
        
        // Load pipeline configuration
        if (config["vision"]["pipeline"]) {
            auto pipeline = config["vision"]["pipeline"];
            vision_config->pipeline_config_.stability = pipeline["stability"].as<std::string>("production");
            
            // Error handling
            if (pipeline["error_handling"]) {
                auto error_handling = pipeline["error_handling"];
                vision_config->pipeline_config_.error_handling.strategy = error_handling["strategy"].as<std::string>("graceful");
                
                if (error_handling["retry"]) {
                    auto retry = error_handling["retry"];
                    vision_config->pipeline_config_.error_handling.retry_enabled = retry["enabled"].as<bool>(true);
                    vision_config->pipeline_config_.error_handling.max_retry_attempts = retry["max_attempts"].as<int>(3);
                    vision_config->pipeline_config_.error_handling.backoff_strategy = retry["backoff_strategy"].as<std::string>("exponential");
                    vision_config->pipeline_config_.error_handling.initial_delay = std::chrono::milliseconds(retry["initial_delay_ms"].as<int>(100));
                    vision_config->pipeline_config_.error_handling.max_delay = std::chrono::milliseconds(retry["max_delay_ms"].as<int>(5000));
                }
                
                if (error_handling["fallback"]) {
                    auto fallback = error_handling["fallback"];
                    vision_config->pipeline_config_.error_handling.use_cpu_fallback = fallback["use_cpu_fallback"].as<bool>(true);
                    vision_config->pipeline_config_.error_handling.use_smaller_model = fallback["use_smaller_model"].as<bool>(true);
                    vision_config->pipeline_config_.error_handling.return_error_response = fallback["return_error_response"].as<bool>(true);
                }
            }
            
            // Preprocessing
            if (pipeline["preprocessing"]) {
                auto preprocessing = pipeline["preprocessing"];
                vision_config->pipeline_config_.preprocessing.enabled = preprocessing["enabled"].as<bool>(true);
                vision_config->pipeline_config_.preprocessing.resize_strategy = preprocessing["resize_strategy"].as<std::string>("adaptive");
                vision_config->pipeline_config_.preprocessing.normalize = preprocessing["normalize"].as<bool>(true);
                vision_config->pipeline_config_.preprocessing.augmentation = preprocessing["augmentation"].as<bool>(false);
                vision_config->pipeline_config_.preprocessing.cache_preprocessed = preprocessing["cache_preprocessed"].as<bool>(true);
                vision_config->pipeline_config_.preprocessing.cache_ttl = std::chrono::seconds(preprocessing["cache_ttl_seconds"].as<int>(3600));
            }
            
            // Postprocessing
            if (pipeline["postprocessing"]) {
                auto postprocessing = pipeline["postprocessing"];
                vision_config->pipeline_config_.postprocessing.enabled = postprocessing["enabled"].as<bool>(true);
                vision_config->pipeline_config_.postprocessing.format = postprocessing["format"].as<std::string>("json");
                vision_config->pipeline_config_.postprocessing.include_metadata = postprocessing["include_metadata"].as<bool>(true);
                vision_config->pipeline_config_.postprocessing.include_timings = postprocessing["include_timings"].as<bool>(true);
                vision_config->pipeline_config_.postprocessing.include_confidence_scores = postprocessing["include_confidence_scores"].as<bool>(true);
                vision_config->pipeline_config_.postprocessing.min_confidence_threshold = postprocessing["min_confidence_threshold"].as<float>(0.5f);
                vision_config->pipeline_config_.postprocessing.max_results = postprocessing["max_results"].as<int>(10);
            }
        }
        
        // Load feature flags
        if (config["vision"]["features"]) {
            auto features = config["vision"]["features"];
            vision_config->feature_flags_["clip_encoding"] = features["clip_encoding"].as<bool>(true);
            vision_config->feature_flags_["llava_inference"] = features["llava_inference"].as<bool>(true);
            vision_config->feature_flags_["multi_image"] = features["multi_image"].as<bool>(true);
            vision_config->feature_flags_["image_generation"] = features["image_generation"].as<bool>(false);
            
            if (features["experimental"]) {
                auto experimental = features["experimental"];
                vision_config->experimental_features_["video_processing"] = experimental["video_processing"].as<bool>(false);
                vision_config->experimental_features_["real_time_streaming"] = experimental["real_time_streaming"].as<bool>(false);
                vision_config->experimental_features_["batch_inference"] = experimental["batch_inference"].as<bool>(true);
                vision_config->experimental_features_["distributed_inference"] = experimental["distributed_inference"].as<bool>(false);
            }
        }
        
        spdlog::info("Vision configuration loaded successfully from {}", config_path);
        spdlog::info("  - API Version: {}", vision_config->api_version_);
        spdlog::info("  - Stability Level: {}", static_cast<int>(vision_config->api_stability_));
        spdlog::info("  - License Enforcement: {}", vision_config->enforce_licenses_ ? "enabled" : "disabled");
        spdlog::info("  - Monitoring: {}", vision_config->monitoring_config_.enabled ? "enabled" : "disabled");
        spdlog::info("  - Sandboxing: {}", vision_config->security_config_.sandboxing.enabled ? "enabled" : "disabled");
        
        return publishVisionConfig(vision_config);
        
    } catch (const YAML::Exception& e) {
        spdlog::error("Failed to load vision configuration: {}", e.what());
        throw std::runtime_error("Vision configuration load failed: " + std::string(e.what()));
    }
}

std::shared_ptr<VisionConfig> VisionConfig::loadFromJson(const nlohmann::json& config) {
    // Start from defaults so unspecified fields have sensible values.
    auto vision_config = getDefault();
    
    // Lock the config during initialization to prevent data races
    std::unique_lock<std::shared_mutex> lock(vision_config->config_mutex_);

    auto get_str = [&](const nlohmann::json& j, const char* key, std::string& out) {
        if (j.contains(key) && j[key].is_string()) {
            out = j[key].get<std::string>();
        }
    };
    auto get_bool = [&](const nlohmann::json& j, const char* key, bool& out) {
        if (j.contains(key) && j[key].is_boolean()) {
            out = j[key].get<bool>();
        }
    };
    auto get_size = [&](const nlohmann::json& j, const char* key, size_t& out) {
        if (j.contains(key) && j[key].is_number_unsigned()) {
            out = j[key].get<size_t>();
        }
    };

    // API configuration
    get_str(config, "api_version", vision_config->api_version_);
    get_str(config, "api_prefix",  vision_config->api_prefix_);
    get_bool(config, "backward_compatible", vision_config->backward_compatible_);

    if (config.contains("api_stability") && config["api_stability"].is_string()) {
        const std::string stability = config["api_stability"].get<std::string>();
        if (stability == "experimental") {
            vision_config->api_stability_ = VisionAPIStability::EXPERIMENTAL;
        } else if (stability == "beta") {
            vision_config->api_stability_ = VisionAPIStability::BETA;
        } else if (stability == "stable") {
            vision_config->api_stability_ = VisionAPIStability::STABLE;
        } else if (stability == "deprecated") {
            vision_config->api_stability_ = VisionAPIStability::DEPRECATED;
        }
    }

    // License configuration
    get_bool(config, "enforce_licenses", vision_config->enforce_licenses_);
    if (config.contains("allowed_licenses") && config["allowed_licenses"].is_array()) {
        vision_config->allowed_licenses_.clear();
        for (const auto& lic : config["allowed_licenses"]) {
            if (lic.is_string()) {
                vision_config->allowed_licenses_.push_back(lic.get<std::string>());
            } else {
                spdlog::warn("VisionConfig::loadFromJson: non-string entry in "
                             "'allowed_licenses' (type={}) — entry skipped",
                             static_cast<int>(lic.type()));
            }
        }
    }

    // Resource limits
    if (config.contains("resource_limits") && config["resource_limits"].is_object()) {
        const auto& rl = config["resource_limits"];
        get_size(rl, "max_memory_mb",              vision_config->resource_limits_.max_memory_mb);
        get_size(rl, "max_memory_per_request_mb",  vision_config->resource_limits_.max_memory_per_request_mb);
        get_size(rl, "max_vram_mb",                vision_config->resource_limits_.max_vram_mb);
        get_size(rl, "max_vram_per_model_mb",      vision_config->resource_limits_.max_vram_per_model_mb);
        get_size(rl, "max_concurrent_requests",    vision_config->resource_limits_.max_concurrent_requests);
        get_size(rl, "max_concurrent_models",      vision_config->resource_limits_.max_concurrent_models);
        get_size(rl, "max_queue_size",             vision_config->resource_limits_.max_queue_size);
        if (rl.contains("max_inference_time_s") && rl["max_inference_time_s"].is_number()) {
            vision_config->resource_limits_.max_inference_time =
                std::chrono::seconds(rl["max_inference_time_s"].get<int64_t>());
        }
        if (rl.contains("max_model_load_time_s") && rl["max_model_load_time_s"].is_number()) {
            vision_config->resource_limits_.max_model_load_time =
                std::chrono::seconds(rl["max_model_load_time_s"].get<int64_t>());
        }
        if (rl.contains("request_timeout_s") && rl["request_timeout_s"].is_number()) {
            vision_config->resource_limits_.request_timeout =
                std::chrono::seconds(rl["request_timeout_s"].get<int64_t>());
        }
    }

    // Rate limits
    if (config.contains("rate_limits") && config["rate_limits"].is_object()) {
        const auto& ratel = config["rate_limits"];
        get_bool(ratel, "enabled",               vision_config->rate_limits_.enabled);
        get_size(ratel, "requests_per_minute",   vision_config->rate_limits_.requests_per_minute);
        get_size(ratel, "requests_per_hour",     vision_config->rate_limits_.requests_per_hour);
        get_size(ratel, "requests_per_day",      vision_config->rate_limits_.requests_per_day);
        get_size(ratel, "burst_size",            vision_config->rate_limits_.burst_size);
        get_str(ratel, "on_limit_exceeded",      vision_config->rate_limits_.on_limit_exceeded);
    }

    // Monitoring
    if (config.contains("monitoring") && config["monitoring"].is_object()) {
        const auto& mon = config["monitoring"];
        get_bool(mon, "enabled",              vision_config->monitoring_config_.enabled);
        get_bool(mon, "track_latency",        vision_config->monitoring_config_.track_latency);
        get_bool(mon, "track_throughput",     vision_config->monitoring_config_.track_throughput);
        get_bool(mon, "track_error_rate",     vision_config->monitoring_config_.track_error_rate);
        get_bool(mon, "track_resource_usage", vision_config->monitoring_config_.track_resource_usage);
        get_bool(mon, "track_model_usage",    vision_config->monitoring_config_.track_model_usage);
        if (mon.contains("audit") && mon["audit"].is_object()) {
            const auto& audit = mon["audit"];
            get_bool(audit, "enabled",          vision_config->monitoring_config_.audit.enabled);
            get_bool(audit, "include_pii",      vision_config->monitoring_config_.audit.include_pii);
            get_str(audit,  "storage_type",     vision_config->monitoring_config_.audit.storage_type);
            get_str(audit,  "compliance_mode",  vision_config->monitoring_config_.audit.compliance_mode);
            if (audit.contains("retention_days") && audit["retention_days"].is_number_integer()) {
                vision_config->monitoring_config_.audit.retention_days =
                    audit["retention_days"].get<int>();
            }
        }
    }

    // Security configuration
    if (config.contains("security") && config["security"].is_object()) {
        const auto& sec = config["security"];
        if (sec.contains("validation") && sec["validation"].is_object()) {
            const auto& val = sec["validation"];
            get_bool(val, "enabled",                  vision_config->security_config_.validation.enabled);
            get_size(val, "max_image_size_mb",        vision_config->security_config_.validation.max_image_size_mb);
            get_bool(val, "validate_image_integrity",  vision_config->security_config_.validation.validate_image_integrity);
            get_bool(val, "sanitize_prompts",          vision_config->security_config_.validation.sanitize_prompts);
            get_bool(val, "block_injection_attempts",  vision_config->security_config_.validation.block_injection_attempts);
        }
        if (sec.contains("sandboxing") && sec["sandboxing"].is_object()) {
            const auto& sb = sec["sandboxing"];
            get_bool(sb, "enabled",          vision_config->security_config_.sandboxing.enabled);
            get_bool(sb, "isolate_memory",   vision_config->security_config_.sandboxing.isolate_memory);
            get_bool(sb, "isolate_network",  vision_config->security_config_.sandboxing.isolate_network);
            get_str(sb,  "type",             vision_config->security_config_.sandboxing.type);
        }
    }

    // Pipeline configuration
    if (config.contains("pipeline") && config["pipeline"].is_object()) {
        const auto& pip = config["pipeline"];
        get_str(pip, "stability", vision_config->pipeline_config_.stability);
    }

    // Feature flags
    if (config.contains("feature_flags") && config["feature_flags"].is_object()) {
        for (const auto& [key, val] : config["feature_flags"].items()) {
            if (val.is_boolean()) {
                vision_config->feature_flags_[key] = val.get<bool>();
            } else {
                spdlog::warn("VisionConfig::loadFromJson: non-boolean value for "
                             "feature flag '{}' (type={}) — flag skipped",
                             key, static_cast<int>(val.type()));
            }
        }
    }

    // Experimental feature flags
    if (config.contains("experimental_features") && config["experimental_features"].is_object()) {
        for (const auto& [key, val] : config["experimental_features"].items()) {
            if (val.is_boolean()) {
                vision_config->experimental_features_[key] = val.get<bool>();
            } else {
                spdlog::warn("VisionConfig::loadFromJson: non-boolean value for "
                             "experimental feature '{}' (type={}) — entry skipped",
                             key, static_cast<int>(val.type()));
            }
        }
    }

    spdlog::info("VisionConfig loaded from JSON (api_version={}, stability={})",
                 vision_config->api_version_,
                 static_cast<int>(vision_config->api_stability_));

    return publishVisionConfig(vision_config);
}

std::shared_ptr<VisionConfig> VisionConfig::getDefault() {
    // VisionConfig has a private constructor; construct directly here.
    auto config = std::shared_ptr<VisionConfig>(new VisionConfig());
    
    // Lock the config during initialization to prevent data races
    std::unique_lock<std::shared_mutex> lock(config->config_mutex_);

    // Set reasonable defaults
    config->api_stability_ = VisionAPIStability::STABLE;
    config->api_version_ = "1.0.0";
    config->api_prefix_ = "/api/v1/vision";
    config->backward_compatible_ = true;
    
    config->enforce_licenses_ = true;
    config->allowed_licenses_ = {"MIT", "Apache-2.0", "BSD-3-Clause", "CC-BY-4.0", "Llama-2-Community", "OpenRAIL-M"};
    
    // Default resource limits
    config->resource_limits_.max_memory_mb = 8192;
    config->resource_limits_.max_memory_per_request_mb = 2048;
    config->resource_limits_.max_vram_mb = 16384;
    config->resource_limits_.max_vram_per_model_mb = 8192;
    config->resource_limits_.max_concurrent_requests = 16;
    config->resource_limits_.max_concurrent_models = 4;
    config->resource_limits_.max_queue_size = 100;
    config->resource_limits_.max_inference_time = std::chrono::seconds(60);
    config->resource_limits_.max_model_load_time = std::chrono::seconds(120);
    config->resource_limits_.request_timeout = std::chrono::seconds(90);
    
    // Default rate limits
    config->rate_limits_.enabled = true;
    config->rate_limits_.requests_per_minute = 120;
    config->rate_limits_.requests_per_hour = 3600;
    config->rate_limits_.requests_per_day = 50000;
    config->rate_limits_.burst_size = 20;
    config->rate_limits_.on_limit_exceeded = "reject";
    
    // Default monitoring
    config->monitoring_config_.enabled = true;
    config->monitoring_config_.track_latency = true;
    config->monitoring_config_.track_throughput = true;
    config->monitoring_config_.track_error_rate = true;
    config->monitoring_config_.track_resource_usage = true;
    config->monitoring_config_.track_model_usage = true;
    
    // Default security
    config->security_config_.validation.enabled = true;
    config->security_config_.validation.max_image_size_mb = 25;
    config->security_config_.validation.max_image_resolution = {4096, 4096};
    config->security_config_.validation.allowed_formats = {"JPEG", "PNG", "BMP", "WEBP"};
    config->security_config_.sandboxing.enabled = false;
    
    spdlog::info("Using default vision configuration");
    
    return publishVisionConfig(config);
}

bool VisionConfig::validate(std::string& error_message) const {
    std::stringstream errors = {};
    
    // Validate resource limits
    if (resource_limits_.max_memory_mb == 0) {
        errors << "max_memory_mb cannot be 0; ";
    }
    
    if (resource_limits_.max_concurrent_requests == 0) {
        errors << "max_concurrent_requests cannot be 0; ";
    }
    
    // Validate rate limits
    if (rate_limits_.enabled && rate_limits_.requests_per_minute == 0) {
        errors << "requests_per_minute cannot be 0 when rate limiting is enabled; ";
    }
    
    // Validate security configuration
    if (security_config_.validation.enabled && security_config_.validation.max_image_size_mb == 0) {
        errors << "max_image_size_mb cannot be 0 when validation is enabled; ";
    }
    
    error_message = errors.str();
    return error_message.empty();
}

// API Configuration Getters
VisionAPIStability VisionConfig::getAPIStability() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return api_stability_;
}

const std::string& VisionConfig::getAPIVersion() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return api_version_;
}

const std::string& VisionConfig::getAPIPrefix() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return api_prefix_;
}

bool VisionConfig::isBackwardCompatible() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return backward_compatible_;
}

// License Management Getters
bool VisionConfig::isLicenseEnforced() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return enforce_licenses_;
}

bool VisionConfig::isLicenseAllowed(const std::string& license_id) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    if (!enforce_licenses_) {
        return true;
    }
    
    return std::find(allowed_licenses_.begin(), allowed_licenses_.end(), license_id) != allowed_licenses_.end();
}

// Resource Management Getters
const VisionResourceLimits& VisionConfig::getResourceLimits() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return resource_limits_;
}

const VisionRateLimits& VisionConfig::getRateLimits() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return rate_limits_;
}

const VisionResourceQuota& VisionConfig::getResourceQuota() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return resource_quota_;
}

std::shared_ptr<ModelLicense> VisionConfig::getModelLicense(const std::string& model_id) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    auto it = licenses_.find(model_id);
    if (it != licenses_.end()) {
        return it->second;
    }
    return nullptr;
}

// Monitoring Getters
const VisionMonitoringConfig& VisionConfig::getMonitoringConfig() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return monitoring_config_;
}

bool VisionConfig::isMonitoringEnabled() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return monitoring_config_.enabled;
}

bool VisionConfig::isAuditEnabled() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return monitoring_config_.audit.enabled;
}

bool VisionConfig::validateModelUsage(const std::string& model_id, bool is_commercial) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    if (!enforce_licenses_) {
        return true;
    }
    
    auto license = getModelLicense(model_id);
    if (!license) {
        spdlog::warn("No license information found for model: {}", model_id);
        return !enforce_licenses_; // Allow if enforcement is disabled
    }
    
    return license->validateUsage(is_commercial, false, false);
}

// Model Registry Getters
std::vector<std::string> VisionConfig::getAvailableModels() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    std::vector<std::string> model_ids = {};

    for (const auto& pair : models_) {
        model_ids.push_back(pair.first);
    }
    return model_ids;
}

std::shared_ptr<VisionModelMetadata> VisionConfig::getModelMetadata(const std::string& model_id) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        return it->second;
    }
    return nullptr;
}

bool VisionConfig::isModelProductionReady(const std::string& model_id) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    auto metadata = getModelMetadata(model_id);
    return metadata && metadata->production_ready;
}

// Feature Flag Getters
bool VisionConfig::isFeatureEnabled(const std::string& feature_name) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    auto it = feature_flags_.find(feature_name);
    if (it != feature_flags_.end()) {
        return it->second;
    }
    return false;
}

bool VisionConfig::isExperimentalFeature(const std::string& feature_name) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    auto it = experimental_features_.find(feature_name);
    if (it != experimental_features_.end()) {
        return it->second;
    }
    return false;
}

// Security Getters
const VisionSecurityConfig& VisionConfig::getSecurityConfig() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return security_config_;
}

bool VisionConfig::isSandboxingEnabled() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return security_config_.sandboxing.enabled;
}

bool VisionConfig::isModelVerificationEnabled() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return security_config_.model_verification.enabled;
}

// Pipeline Getter
const VisionPipelineConfig& VisionConfig::getPipelineConfig() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return pipeline_config_;
}

std::string VisionConfig::getRequiredAttribution(const std::string& model_id) const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    auto it = models_.find(model_id);
    if (it != models_.end() && it->second) {
        return it->second->attribution;
    }
    return "";
}

} // namespace llm
} // namespace themis

