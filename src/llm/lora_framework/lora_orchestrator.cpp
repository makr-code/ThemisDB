#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/lora_framework/lora_adapter_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_audit_logger.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <shared_mutex>
#include <chrono>

namespace themis {
namespace llm {
namespace lora {

// ═══════════════════════════════════════════════════════════
// Implementation Details
// ═══════════════════════════════════════════════════════════

// Helper function to generate job IDs
static std::string generateJobId(const std::string& operation, const std::string& adapter_id) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    return operation + "_" + adapter_id + "_" + std::to_string(timestamp);
}

class LoRAOrchestrator::Impl {
public:
    // Configuration
    Config config;
    
    // Core components
    std::unique_ptr<LoRAAdapterManager> adapter_manager;
    std::unique_ptr<LoRAStorageService> storage_service;
    std::unique_ptr<LoRATrainingService> training_service;
    std::unique_ptr<LoRAAuditLogger> audit_logger;
    
    // State
    std::atomic<bool> is_initialized{false};
    mutable std::shared_mutex state_mutex;
    
    explicit Impl(const Config& cfg) : config(cfg) {
        initialize();
    }
    
    void initialize() {
        try {
            spdlog::info("Initializing LoRA Orchestrator...");
            
            // Initialize storage service
            storage_service = std::make_unique<LoRAStorageService>(config.storage_config);
            
            // Initialize adapter manager
            adapter_manager = std::make_unique<LoRAAdapterManager>(config.adapter_config);
            
            // Initialize training service
            training_service = std::make_unique<LoRATrainingService>(config.training_config);
            
            // Initialize audit logger
            utils::AuditLoggerConfig audit_config;
            audit_config.log_file = "logs/lora_orchestrator_audit.jsonl";
            audit_config.enable_encryption = config.storage_config.enable_encryption;
            
            audit_logger = std::make_unique<LoRAAuditLogger>(audit_config);
            
            is_initialized = true;
            spdlog::info("LoRA Orchestrator initialized successfully");
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to initialize LoRA Orchestrator: {}", e.what());
            throw;
        }
    }
};

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

LoRAOrchestrator::LoRAOrchestrator(const Config& config)
    : impl_(std::make_unique<Impl>(config))
{
}

LoRAOrchestrator::~LoRAOrchestrator() = default;

// ═══════════════════════════════════════════════════════════
// CRUD Operations
// ═══════════════════════════════════════════════════════════

std::string LoRAOrchestrator::createAdapter(
    const std::string& adapter_id,
    const TrainingData& training_data,
    const std::optional<LoRAHyperparameters>& hyperparameters,
    bool async
) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        spdlog::info("Creating new adapter: {}", adapter_id);
        
        // Log creation event
        impl_->audit_logger->logEvent(
            LoRAAuditEventType::ADAPTER_CREATED,
            adapter_id,
            {{"training_samples", training_data.samples.size()}}
        );
        
        // Train adapter
        auto result = impl_->training_service->trainBatch(training_data.samples);
        
        if (!result.success) {
            spdlog::error("Training failed for adapter: {}", adapter_id);
            return "";  // Empty string indicates failure
        }
        
        // Save adapter
        AdapterMetadata metadata;
        metadata.adapter_id = adapter_id;
        metadata.base_model = training_data.base_model_id;
        metadata.version = "v1.0";
        metadata.description = training_data.description;
        metadata.training_samples = training_data.samples.size();
        metadata.validation_accuracy = result.final_loss;
        
        bool saved = impl_->storage_service->saveAdapter(
            adapter_id,
            result.weights,
            metadata
        );
        
        if (!saved) {
            spdlog::error("Failed to save adapter: {}", adapter_id);
            return "";  // Empty string indicates failure
        }
        
        spdlog::info("Adapter created successfully: {}", adapter_id);
        
        // Return job ID for async, or "success" for sync
        return async ? generateJobId("create", adapter_id) : "success";
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create adapter {}: {}", adapter_id, e.what());
        return "";  // Empty string indicates failure
    }
}

std::optional<AdapterInfo> LoRAOrchestrator::getAdapter(const std::string& adapter_id) {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        auto metadata = impl_->storage_service->loadMetadata(adapter_id);
        if (!metadata) {
            return std::nullopt;
        }
        
        AdapterInfo info;
        info.adapter_id = metadata->adapter_id;
        info.base_model = metadata->base_model;
        info.version = metadata->version;
        info.description = metadata->description;
        info.is_loaded = impl_->adapter_manager->isLoaded(adapter_id);
        info.training_samples = metadata->training_samples;
        info.validation_accuracy = metadata->validation_accuracy;
        
        return info;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to get adapter {}: {}", adapter_id, e.what());
        return std::nullopt;
    }
}

std::string LoRAOrchestrator::updateAdapter(
    const std::string& adapter_id,
    const TrainingData& training_data,
    bool incremental,
    bool async
) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        spdlog::info("Updating adapter: {}", adapter_id);
        
        // Check if adapter exists
        auto metadata = impl_->storage_service->loadMetadata(adapter_id);
        if (!metadata) {
            spdlog::error("Adapter not found: {}", adapter_id);
            return "";  // Empty string indicates failure
        }
        
        // Log update event
        impl_->audit_logger->logEvent(
            LoRAAuditEventType::ADAPTER_UPDATED,
            adapter_id,
            {{"additional_samples", training_data.samples.size()}}
        );
        
        // Incremental training
        auto result = impl_->training_service->trainBatch(training_data.samples);
        
        if (!result.success) {
            spdlog::error("Training failed for adapter update: {}", adapter_id);
            return "";  // Empty string indicates failure
        }
        
        // Update metadata
        metadata->training_samples += training_data.samples.size();
        metadata->validation_accuracy = result.final_loss;
        
        // Save updated adapter
        bool saved = impl_->storage_service->saveAdapter(
            adapter_id,
            result.weights,
            *metadata
        );
        
        if (!saved) {
            spdlog::error("Failed to save updated adapter: {}", adapter_id);
            return "";  // Empty string indicates failure
        }
        
        spdlog::info("Adapter updated successfully: {}", adapter_id);
        
        // Return job ID for async, or "success" for sync
        return async ? generateJobId("update", adapter_id) : "success";
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to update adapter {}: {}", adapter_id, e.what());
        return "";  // Empty string indicates failure
    }
}

bool LoRAOrchestrator::deleteAdapter(const std::string& adapter_id) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        spdlog::info("Deleting adapter: {}", adapter_id);
        
        // Unload if loaded
        if (impl_->adapter_manager->isLoaded(adapter_id)) {
            impl_->adapter_manager->unloadAdapter(adapter_id);
        }
        
        // Delete from storage
        bool deleted = impl_->storage_service->deleteAdapter(adapter_id);
        
        if (!deleted) {
            spdlog::error("Failed to delete adapter: {}", adapter_id);
            return false;
        }
        
        // Log deletion event
        impl_->audit_logger->logEvent(
            LoRAAuditEventType::ADAPTER_DELETED,
            adapter_id,
            {}
        );
        
        spdlog::info("Adapter deleted successfully: {}", adapter_id);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());
        return false;
    }
}

std::vector<std::string> LoRAOrchestrator::listAdapters(
    const std::optional<std::string>& filter
) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        return impl_->storage_service->listAdapters(filter);
    } catch (const std::exception& e) {
        spdlog::error("Failed to list adapters: {}", e.what());
        return {};
    }
}

// ═══════════════════════════════════════════════════════════
// Adapter Management
// ═══════════════════════════════════════════════════════════

std::string LoRAOrchestrator::loadAdapter(const std::string& adapter_id, bool async) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        // Check if already loaded
        if (impl_->adapter_manager->isLoaded(adapter_id)) {
            spdlog::debug("Adapter already loaded: {}", adapter_id);
            return async ? generateJobId("load", adapter_id) : "success";
        }
        
        // Load from storage
        auto weights = impl_->storage_service->loadAdapter(adapter_id);
        if (!weights) {
            spdlog::error("Failed to load adapter from storage: {}", adapter_id);
            return "";  // Empty string indicates failure
        }
        
        // Load into manager
        bool loaded = impl_->adapter_manager->loadAdapter(adapter_id);
        
        if (loaded) {
            // Log load event
            impl_->audit_logger->logEvent(
                LoRAAuditEventType::ADAPTER_LOADED,
                adapter_id,
                {}
            );
            
            spdlog::info("Adapter loaded successfully: {}", adapter_id);
            return async ? generateJobId("load", adapter_id) : "success";
        }
        
        return "";  // Empty string indicates failure
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load adapter {}: {}", adapter_id, e.what());
        return "";  // Empty string indicates failure
    }
}

bool LoRAOrchestrator::unloadAdapter(const std::string& adapter_id) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        bool unloaded = impl_->adapter_manager->unloadAdapter(adapter_id);
        
        if (unloaded) {
            // Log unload event
            impl_->audit_logger->logEvent(
                LoRAAuditEventType::ADAPTER_UNLOADED,
                adapter_id,
                {}
            );
            
            spdlog::info("Adapter unloaded successfully: {}", adapter_id);
        }
        
        return unloaded;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to unload adapter {}: {}", adapter_id, e.what());
        return false;
    }
}

bool LoRAOrchestrator::isAdapterLoaded(const std::string& adapter_id) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        return impl_->adapter_manager->isLoaded(adapter_id);
    } catch (const std::exception& e) {
        spdlog::error("Failed to check adapter loaded status {}: {}", adapter_id, e.what());
        return false;
    }
}

// ═══════════════════════════════════════════════════════════
// Statistics & Monitoring
// ═══════════════════════════════════════════════════════════

json LoRAOrchestrator::getStats() const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    try {
        json stats;
        
        // Adapter manager stats
        auto cache_stats = impl_->adapter_manager->getCacheStats();
        stats["cache"] = {
            {"size", cache_stats.size},
            {"capacity", cache_stats.capacity},
            {"hits", cache_stats.hits},
            {"misses", cache_stats.misses},
            {"hit_rate", cache_stats.hit_rate}
        };
        
        // Storage stats
        auto storage_stats = impl_->storage_service->getStats();
        stats["storage"] = storage_stats;
        
        // Training stats
        auto training_metrics = impl_->training_service->getMetrics();
        stats["training"] = {
            {"total_trainings", training_metrics.total_trainings},
            {"successful_trainings", training_metrics.successful_trainings},
            {"failed_trainings", training_metrics.failed_trainings},
            {"average_loss", training_metrics.average_loss}
        };
        
        return stats;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to get orchestrator stats: {}", e.what());
        return json::object();
    }
}

bool LoRAOrchestrator::healthCheck() const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    
    return impl_->is_initialized.load() &&
           impl_->storage_service != nullptr &&
           impl_->adapter_manager != nullptr &&
           impl_->training_service != nullptr;
}

} // namespace lora
} // namespace llm
} // namespace themis
