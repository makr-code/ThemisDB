#include "llm/llm_deployment_plugin.h"
#include "utils/logger.h"
#include "utils/checksum_utils.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <sstream>

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

namespace themis {
namespace llm {

namespace {
// Convert time_point to ISO 8601 string
std::string timeToISO8601(const std::chrono::system_clock::time_point& tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val;
    #ifdef _WIN32
        gmtime_s(&tm_val, &time_t_val);
    #else
        gmtime_r(&time_t_val, &tm_val);
    #endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_val);
    return std::string(buf);
}

// Parse ISO 8601 string to time_point
std::chrono::system_clock::time_point iso8601ToTime(const std::string& iso) {
    std::tm tm_val = {};
    std::istringstream ss(iso);
    ss >> std::get_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm_val));
}

} // anonymous namespace

// ============================================================================
// LLMDeploymentPlugin Implementation
// ============================================================================

LLMDeploymentPlugin::LLMDeploymentPlugin(const DeploymentConfig& config)
    : config_(config), downloader_(std::make_unique<ModelDownloader>()) {
    
    // Create cache directory if it doesn't exist
    if (!fs::exists(config_.cache_directory)) {
        try {
            fs::create_directories(config_.cache_directory);
            LOG_INFO("Created cache directory: {}", config_.cache_directory);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create cache directory: {}", e.what());
        }
    }
    
    // Create audit log directory if needed
    if (config_.enable_audit_log) {
        fs::path audit_path(config_.audit_log_path);
        if (audit_path.has_parent_path()) {
            try {
                fs::create_directories(audit_path.parent_path());
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to create audit log directory: {}", e.what());
            }
        }
    }
    
    // Load model registry
    loadModelRegistry();
    
    LOG_INFO("LLM Deployment Plugin initialized (mode: {}, cache: {})", 
             static_cast<int>(config_.mode), config_.cache_directory);
}

std::optional<ModelStatus> LLMDeploymentPlugin::deployModel(const std::string& model_id,
                                                              bool force_download) {
    LOG_INFO("Deploying model: {} (force_download: {})", model_id, force_download);
    
    AuditEntry audit;
    audit.operation = "deploy";
    audit.model_id = model_id;
    audit.timestamp = std::chrono::system_clock::now();
    audit.user = "system"; // TODO(feature): Implement user context tracking for audit compliance
    
    try {
        std::string model_path = getModelPath(model_id);
        bool need_download = force_download || !fs::exists(model_path);
        
        // Check deployment mode
        if (config_.mode == DeploymentMode::OFFLINE && need_download) {
            audit.success = false;
            audit.error_message = "Model not in cache and OFFLINE mode is active";
            logAudit(audit);
            LOG_ERROR("{}", audit.error_message);
            return std::nullopt;
        }
        
        // Download if needed
        if (need_download) {
            LOG_INFO("Model not cached, downloading...");
            auto download_result = downloadModel(model_id);
            
            if (!download_result.success) {
                audit.success = false;
                audit.error_message = download_result.error_message;
                logAudit(audit);
                return std::nullopt;
            }
            
            model_path = download_result.model_path;
        }
        
        // Verify integrity if enabled
        if (config_.verify_checksums) {
            LOG_INFO("Verifying model integrity...");
            if (!verifyModel(model_id)) {
                audit.success = false;
                audit.error_message = "Model integrity verification failed";
                logAudit(audit);
                LOG_ERROR("{}", audit.error_message);
                return std::nullopt;
            }
        }
        
        // Create or update model status
        ModelStatus status;
        status.model_id = model_id;
        status.model_path = model_path;
        status.is_cached = true;
        status.is_loaded = false;
        status.downloaded_at = std::chrono::system_clock::now();
        status.last_used_at = std::chrono::system_clock::now();
        status.last_verified_at = std::chrono::system_clock::now();
        
        // Get file info
        if (fs::exists(model_path)) {
            status.size_bytes = fs::file_size(model_path);
            
            // Detect format from extension
            fs::path path(model_path);
            std::string ext = path.extension().string();
            if (ext == ".gguf") {
                status.format = "gguf";
            } else if (ext == ".bin") {
                status.format = "bin";
            } else if (ext == ".safetensors") {
                status.format = "safetensors";
            } else {
                status.format = "unknown";
            }
            
            // Calculate checksum
            status.checksum = utils::calculateSHA256(model_path);
            status.checksum_type = "sha256";
        }
        
        // Update registry
        auto it = std::find_if(model_registry_.begin(), model_registry_.end(),
                               [&](const ModelStatus& s) { return s.model_id == model_id; });
        
        if (it != model_registry_.end()) {
            *it = status;
        } else {
            model_registry_.push_back(status);
        }
        
        saveModelRegistry();
        
        audit.success = true;
        audit.details["model_path"] = model_path;
        audit.details["size_bytes"] = status.size_bytes;
        audit.details["format"] = status.format;
        logAudit(audit);
        
        LOG_INFO("Model '{}' deployed successfully", model_id);
        return status;
        
    } catch (const std::exception& e) {
        audit.success = false;
        audit.error_message = std::string("Exception: ") + e.what();
        logAudit(audit);
        LOG_ERROR("Failed to deploy model '{}': {}", model_id, e.what());
        return std::nullopt;
    }
}

ModelDownloadResult LLMDeploymentPlugin::downloadModel(const std::string& model_id,
                                                        DownloadProgressCallback progress_callback) {
    LOG_INFO("Downloading model: {}", model_id);
    
    // Find best source for this model
    auto source = findBestSource(model_id);
    
    if (!source) {
        ModelDownloadResult result;
        result.error_message = "No suitable source found for model: " + model_id;
        LOG_ERROR("{}", result.error_message);
        return result;
    }
    
    ModelDownloadResult result;
    
    if (source->type == "ollama") {
        // Download from Ollama
        ModelDownloadConfig dl_config;
        dl_config.model_name = model_id;
        dl_config.ollama_url = config_.ollama_url;
        dl_config.download_dir = config_.cache_directory;
        dl_config.use_cache = config_.enable_cache;
        dl_config.timeout_seconds = config_.ollama_timeout_seconds;
        dl_config.progress_callback = progress_callback;
        
        result = downloader_->downloadFromOllama(dl_config);
    } 
    else if (source->type == "http" || source->type == "https") {
        // Direct HTTP/HTTPS download
        std::string output_path = getModelPath(model_id);
        result = downloader_->downloadFromURL(source->location, output_path, progress_callback);
    }
    else if (source->type == "local") {
        // Copy from local filesystem
        std::string src_path = source->location;
        std::string dst_path = getModelPath(model_id);
        
        try {
            fs::copy_file(src_path, dst_path, fs::copy_options::overwrite_existing);
            result.success = true;
            result.model_path = dst_path;
            result.file_size_bytes = fs::file_size(dst_path);
            LOG_INFO("Copied model from local source: {} -> {}", src_path, dst_path);
        } catch (const std::exception& e) {
            result.error_message = std::string("Failed to copy local model: ") + e.what();
            LOG_ERROR("{}", result.error_message);
        }
    }
    else {
        result.error_message = "Unsupported source type: " + source->type;
        LOG_ERROR("{}", result.error_message);
    }
    
    return result;
}

bool LLMDeploymentPlugin::loadModel(const std::string& model_id, ILLMPlugin* llm_plugin) {
    if (!llm_plugin) {
        LOG_ERROR("Invalid LLM plugin pointer");
        return false;
    }
    
    auto status = getModelStatus(model_id);
    if (!status || !fs::exists(status->model_path)) {
        LOG_ERROR("Model not found: {}", model_id);
        return false;
    }
    
    LOG_INFO("Loading model '{}' from {}", model_id, status->model_path);
    
    json config;
    config["model_path"] = status->model_path;
    
    if (llm_plugin->loadModel(status->model_path, config)) {
        // Update status
        auto it = std::find_if(model_registry_.begin(), model_registry_.end(),
                               [&](const ModelStatus& s) { return s.model_id == model_id; });
        if (it != model_registry_.end()) {
            it->is_loaded = true;
            it->last_used_at = std::chrono::system_clock::now();
            saveModelRegistry();
        }
        
        LOG_INFO("Model '{}' loaded successfully", model_id);
        return true;
    }
    
    LOG_ERROR("Failed to load model '{}'", model_id);
    return false;
}

std::vector<std::string> LLMDeploymentPlugin::listAvailableModels() {
    std::vector<std::string> models;
    
    // Get models from Ollama if configured
    for (const auto& source : config_.sources) {
        if (source.type == "ollama") {
            auto ollama_models = downloader_->listOllamaModels(config_.ollama_url);
            models.insert(models.end(), ollama_models.begin(), ollama_models.end());
        }
    }
    
    // Remove duplicates
    std::sort(models.begin(), models.end());
    models.erase(std::unique(models.begin(), models.end()), models.end());
    
    return models;
}

std::vector<ModelStatus> LLMDeploymentPlugin::listCachedModels() {
    return model_registry_;
}

std::optional<ModelStatus> LLMDeploymentPlugin::getModelStatus(const std::string& model_id) {
    auto it = std::find_if(model_registry_.begin(), model_registry_.end(),
                           [&](const ModelStatus& s) { return s.model_id == model_id; });
    
    if (it != model_registry_.end()) {
        return *it;
    }
    
    return std::nullopt;
}

bool LLMDeploymentPlugin::verifyModel(const std::string& model_id) {
    auto status = getModelStatus(model_id);
    if (!status) {
        LOG_ERROR("Model not found: {}", model_id);
        return false;
    }
    
    if (!fs::exists(status->model_path)) {
        LOG_ERROR("Model file not found: {}", status->model_path);
        return false;
    }
    
    // Find source with checksum
    auto source = findBestSource(model_id);
    if (!source || source->checksum_value.empty()) {
        LOG_WARN("No checksum available for model '{}'", model_id);
        return true; // Consider it verified if no checksum to check
    }
    
    return verifyChecksum(status->model_path, source->checksum_value, source->checksum_type);
}

std::optional<ModelStatus> LLMDeploymentPlugin::updateModel(const std::string& model_id) {
    LOG_INFO("Updating model: {}", model_id);
    
    AuditEntry audit;
    audit.operation = "update";
    audit.model_id = model_id;
    audit.timestamp = std::chrono::system_clock::now();
    audit.user = "system";
    
    // Force re-download
    auto result = deployModel(model_id, true);
    
    if (result) {
        audit.success = true;
        LOG_INFO("Model '{}' updated successfully", model_id);
    } else {
        audit.success = false;
        audit.error_message = "Update failed";
        LOG_ERROR("Failed to update model '{}'", model_id);
    }
    
    logAudit(audit);
    return result;
}

bool LLMDeploymentPlugin::removeModel(const std::string& model_id, bool force) {
    LOG_INFO("Removing model: {} (force: {})", model_id, force);
    
    AuditEntry audit;
    audit.operation = "remove";
    audit.model_id = model_id;
    audit.timestamp = std::chrono::system_clock::now();
    audit.user = "system";
    
    auto status = getModelStatus(model_id);
    if (!status) {
        audit.success = false;
        audit.error_message = "Model not found";
        logAudit(audit);
        return false;
    }
    
    if (status->is_loaded && !force) {
        audit.success = false;
        audit.error_message = "Model is currently loaded (use force=true to remove)";
        logAudit(audit);
        LOG_ERROR("{}", audit.error_message);
        return false;
    }
    
    try {
        if (fs::exists(status->model_path)) {
            fs::remove(status->model_path);
        }
        
        // Remove from registry
        model_registry_.erase(
            std::remove_if(model_registry_.begin(), model_registry_.end(),
                          [&](const ModelStatus& s) { return s.model_id == model_id; }),
            model_registry_.end()
        );
        
        saveModelRegistry();
        
        audit.success = true;
        logAudit(audit);
        
        LOG_INFO("Model '{}' removed successfully", model_id);
        return true;
        
    } catch (const std::exception& e) {
        audit.success = false;
        audit.error_message = std::string("Exception: ") + e.what();
        logAudit(audit);
        LOG_ERROR("Failed to remove model '{}': {}", model_id, e.what());
        return false;
    }
}

int LLMDeploymentPlugin::cleanupOldModels() {
    LOG_INFO("Running model cleanup...");
    
    if (!config_.auto_cleanup) {
        LOG_INFO("Auto-cleanup is disabled");
        return 0;
    }
    
    int removed_count = 0;
    auto now = std::chrono::system_clock::now();
    auto max_age = std::chrono::hours(24 * config_.max_model_age_days);
    
    std::vector<std::string> to_remove;
    
    for (const auto& status : model_registry_) {
        // Skip loaded models
        if (status.is_loaded) {
            continue;
        }
        
        // Check age
        auto age = now - status.last_used_at;
        if (age > max_age) {
            LOG_INFO("Model '{}' is older than {} days, marking for removal", 
                     status.model_id, config_.max_model_age_days);
            to_remove.push_back(status.model_id);
        }
    }
    
    // Remove marked models
    for (const auto& model_id : to_remove) {
        if (removeModel(model_id, false)) {
            removed_count++;
        }
    }
    
    // Check cache size
    size_t cache_size = getCacheSize();
    size_t max_cache_bytes = static_cast<size_t>(config_.max_cache_size_gb) * 1024 * 1024 * 1024;
    
    if (cache_size > max_cache_bytes) {
        LOG_INFO("Cache size ({} bytes) exceeds limit ({} bytes), removing oldest models",
                 cache_size, max_cache_bytes);
        
        // Sort by last used time
        auto sorted_models = model_registry_;
        std::sort(sorted_models.begin(), sorted_models.end(),
                  [](const ModelStatus& a, const ModelStatus& b) {
                      return a.last_used_at < b.last_used_at;
                  });
        
        for (const auto& status : sorted_models) {
            if (status.is_loaded) continue;
            
            if (removeModel(status.model_id, false)) {
                removed_count++;
                cache_size = getCacheSize();
                if (cache_size <= max_cache_bytes) {
                    break;
                }
            }
        }
    }
    
    LOG_INFO("Cleanup complete: {} models removed", removed_count);
    return removed_count;
}

size_t LLMDeploymentPlugin::getCacheSize() const {
    size_t total_size = 0;
    
    for (const auto& status : model_registry_) {
        total_size += status.size_bytes;
    }
    
    return total_size;
}

json LLMDeploymentPlugin::getCacheStats() const {
    json stats;
    
    stats["total_models"] = model_registry_.size();
    stats["total_size_bytes"] = getCacheSize();
    stats["cache_directory"] = config_.cache_directory;
    stats["max_cache_size_gb"] = config_.max_cache_size_gb;
    
    int loaded_count = 0;
    for (const auto& status : model_registry_) {
        if (status.is_loaded) loaded_count++;
    }
    stats["loaded_models"] = loaded_count;
    
    return stats;
}

void LLMDeploymentPlugin::updateConfig(const DeploymentConfig& config) {
    config_ = config;
    LOG_INFO("Deployment configuration updated");
}

std::vector<AuditEntry> LLMDeploymentPlugin::getAuditLog(size_t limit) const {
    if (limit == 0 || limit >= audit_log_.size()) {
        return audit_log_;
    }
    
    // Return most recent entries
    std::vector<AuditEntry> result;
    size_t start = audit_log_.size() - limit;
    result.insert(result.end(), audit_log_.begin() + start, audit_log_.end());
    
    return result;
}

std::optional<DeploymentConfig> LLMDeploymentPlugin::loadConfigFromYAML(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        
        DeploymentConfig deployment_config;
        
        if (config["deployment"]) {
            YAML::Node dep = config["deployment"];
            
            if (dep["mode"]) {
                std::string mode_str = dep["mode"].as<std::string>();
                if (mode_str == "offline") {
                    deployment_config.mode = DeploymentMode::OFFLINE;
                } else if (mode_str == "online") {
                    deployment_config.mode = DeploymentMode::ONLINE;
                } else {
                    deployment_config.mode = DeploymentMode::AUTO;
                }
            }
            
            if (dep["cache_directory"]) {
                deployment_config.cache_directory = dep["cache_directory"].as<std::string>();
            }
            
            if (dep["enable_cache"]) {
                deployment_config.enable_cache = dep["enable_cache"].as<bool>();
            }
            
            if (dep["max_cache_size_gb"]) {
                deployment_config.max_cache_size_gb = dep["max_cache_size_gb"].as<size_t>();
            }
            
            if (dep["ollama_url"]) {
                deployment_config.ollama_url = dep["ollama_url"].as<std::string>();
            }
            
            if (dep["ollama_timeout_seconds"]) {
                deployment_config.ollama_timeout_seconds = dep["ollama_timeout_seconds"].as<int>();
            }
            
            if (dep["proxy_url"]) {
                deployment_config.proxy_url = dep["proxy_url"].as<std::string>();
            }
            
            if (dep["verify_checksums"]) {
                deployment_config.verify_checksums = dep["verify_checksums"].as<bool>();
            }
            
            if (dep["enable_audit_log"]) {
                deployment_config.enable_audit_log = dep["enable_audit_log"].as<bool>();
            }
            
            if (dep["audit_log_path"]) {
                deployment_config.audit_log_path = dep["audit_log_path"].as<std::string>();
            }
            
            if (dep["auto_cleanup"]) {
                deployment_config.auto_cleanup = dep["auto_cleanup"].as<bool>();
            }
            
            if (dep["max_model_age_days"]) {
                deployment_config.max_model_age_days = dep["max_model_age_days"].as<int>();
            }
            
            if (dep["sources"]) {
                for (const auto& src_node : dep["sources"]) {
                    ModelSource source;
                    source.type = src_node["type"].as<std::string>();
                    source.location = src_node["location"].as<std::string>();
                    
                    if (src_node["checksum_type"]) {
                        source.checksum_type = src_node["checksum_type"].as<std::string>();
                    }
                    
                    if (src_node["checksum_value"]) {
                        source.checksum_value = src_node["checksum_value"].as<std::string>();
                    }
                    
                    if (src_node["priority"]) {
                        source.priority = src_node["priority"].as<int>();
                    }
                    
                    deployment_config.sources.push_back(source);
                }
            }
        }
        
        return deployment_config;
        
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Failed to parse YAML configuration: {}", e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load deployment configuration: {}", e.what());
        return std::nullopt;
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void LLMDeploymentPlugin::logAudit(const AuditEntry& entry) {
    if (!config_.enable_audit_log) {
        return;
    }
    
    // Add to in-memory log
    audit_log_.push_back(entry);
    
    // Write to file
    try {
        std::ofstream log_file(config_.audit_log_path, std::ios::app);
        if (log_file.is_open()) {
            json j;
            j["timestamp"] = timeToISO8601(entry.timestamp);
            j["operation"] = entry.operation;
            j["model_id"] = entry.model_id;
            j["user"] = entry.user;
            j["success"] = entry.success;
            if (!entry.error_message.empty()) {
                j["error_message"] = entry.error_message;
            }
            if (!entry.details.empty()) {
                j["details"] = entry.details;
            }
            
            log_file << j.dump() << std::endl;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to write audit log: {}", e.what());
    }
}

void LLMDeploymentPlugin::saveModelRegistry() {
    fs::path registry_path = fs::path(config_.cache_directory) / "model_registry.json";
    
    try {
        json j = json::array();
        
        for (const auto& status : model_registry_) {
            json model_json;
            model_json["model_id"] = status.model_id;
            model_json["model_path"] = status.model_path;
            model_json["version"] = status.version;
            model_json["format"] = status.format;
            model_json["is_loaded"] = status.is_loaded;
            model_json["is_cached"] = status.is_cached;
            model_json["size_bytes"] = status.size_bytes;
            model_json["checksum"] = status.checksum;
            model_json["checksum_type"] = status.checksum_type;
            model_json["downloaded_at"] = timeToISO8601(status.downloaded_at);
            model_json["last_used_at"] = timeToISO8601(status.last_used_at);
            model_json["last_verified_at"] = timeToISO8601(status.last_verified_at);
            model_json["metadata"] = status.metadata;
            
            j.push_back(model_json);
        }
        
        std::ofstream file(registry_path);
        file << j.dump(2) << std::endl;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save model registry: {}", e.what());
    }
}

void LLMDeploymentPlugin::loadModelRegistry() {
    fs::path registry_path = fs::path(config_.cache_directory) / "model_registry.json";
    
    if (!fs::exists(registry_path)) {
        LOG_INFO("Model registry file not found, starting with empty registry");
        return;
    }
    
    try {
        std::ifstream file(registry_path);
        json j;
        file >> j;
        
        model_registry_.clear();
        
        for (const auto& model_json : j) {
            ModelStatus status;
            status.model_id = model_json.value("model_id", "");
            status.model_path = model_json.value("model_path", "");
            status.version = model_json.value("version", "");
            status.format = model_json.value("format", "");
            status.is_loaded = model_json.value("is_loaded", false);
            status.is_cached = model_json.value("is_cached", true);
            status.size_bytes = model_json.value("size_bytes", 0);
            status.checksum = model_json.value("checksum", "");
            status.checksum_type = model_json.value("checksum_type", "");
            
            if (model_json.contains("downloaded_at")) {
                status.downloaded_at = iso8601ToTime(model_json["downloaded_at"]);
            }
            if (model_json.contains("last_used_at")) {
                status.last_used_at = iso8601ToTime(model_json["last_used_at"]);
            }
            if (model_json.contains("last_verified_at")) {
                status.last_verified_at = iso8601ToTime(model_json["last_verified_at"]);
            }
            if (model_json.contains("metadata")) {
                status.metadata = model_json["metadata"];
            }
            
            model_registry_.push_back(status);
        }
        
        LOG_INFO("Loaded {} models from registry", model_registry_.size());
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load model registry: {}", e.what());
    }
}

std::optional<ModelSource> LLMDeploymentPlugin::findBestSource(const std::string& model_id) {
    if (config_.sources.empty()) {
        // Default: try Ollama
        ModelSource source;
        source.type = "ollama";
        source.location = config_.ollama_url;
        source.priority = 0;
        return source;
    }
    
    // Sort sources by priority (higher first)
    auto sorted_sources = config_.sources;
    std::sort(sorted_sources.begin(), sorted_sources.end(),
              [](const ModelSource& a, const ModelSource& b) {
                  return a.priority > b.priority;
              });
    
    // Return first source (highest priority)
    // In a more sophisticated implementation, we could check availability
    return sorted_sources[0];
}

std::string LLMDeploymentPlugin::getModelPath(const std::string& model_id) const {
    // Sanitize model_id for filesystem
    std::string filename = model_id;
    std::replace(filename.begin(), filename.end(), ':', '_');
    std::replace(filename.begin(), filename.end(), '/', '_');
    
    // Add .gguf extension if not present
    if (filename.find('.') == std::string::npos) {
        filename += ".gguf";
    }
    
    fs::path path = fs::path(config_.cache_directory) / filename;
    return path.string();
}

bool LLMDeploymentPlugin::verifyChecksum(const std::string& file_path,
                                          const std::string& expected_checksum,
                                          const std::string& checksum_type) {
    std::string calculated_checksum;
    
    if (checksum_type == "sha256") {
        calculated_checksum = utils::calculateSHA256(file_path);
    } else if (checksum_type == "md5") {
        calculated_checksum = utils::calculateMD5(file_path);
    } else {
        LOG_ERROR("Unsupported checksum type: {}", checksum_type);
        return false;
    }
    
    if (calculated_checksum.empty()) {
        LOG_ERROR("Failed to calculate checksum for {}", file_path);
        return false;
    }
    
    bool match = (calculated_checksum == expected_checksum);
    
    if (match) {
        LOG_INFO("Checksum verification passed for {}", file_path);
    } else {
        LOG_ERROR("Checksum verification failed for {}. Expected: {}, Got: {}",
                  file_path, expected_checksum, calculated_checksum);
    }
    
    return match;
}

} // namespace llm
} // namespace themis
