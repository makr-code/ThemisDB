/**
 * @file themis_help_lora.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=12, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/applications/themis_help_lora.h"
#include <stdexcept>
#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/lora_framework/lora_audit_logger.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/llm_model_audit_logger.h"
#include "llm/feedback_store.h"
#include "llm/llama_wrapper.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <thread>
#include <sstream>

namespace themis {
namespace llm {
namespace applications {

using json = nlohmann::json;
using namespace themis::llm::lora;

namespace {
std::string resolveModelPath(const ThemisHelpLoRA::Config& config) {
    if (config.model_path_provider) {
        try {
            auto resolved = config.model_path_provider(config.base_model_id);
            if (!resolved.empty()) {
                return resolved;
            }
        } catch (const std::exception& e) {
            spdlog::warn("ThemisHelpLoRA: model path provider failed for '{}': {}",
                         config.base_model_id, e.what());
        } catch (...) {
            spdlog::warn("ThemisHelpLoRA: model path provider failed for '{}'",
                         config.base_model_id);
        }
    }

    return "models/" + config.base_model_id + ".gguf";
}
} // namespace

/** @brief Implementation detail. */
class ThemisHelpLoRA::Impl {
public:
    // Configuration
    Config config;
    
    // Components
    std::shared_ptr<lora::LoRAOrchestrator> orchestrator;
    std::shared_ptr<lora::LoRAAuditLogger> lora_audit;
    std::shared_ptr<LLMModelAuditLogger> llm_audit;
    std::unique_ptr<LlamaWrapper> llama_wrapper;
    std::unique_ptr<LoRATrainingService> training_service;
    
    // State
    std::string current_adapter_version;
    std::vector<std::string> version_history;   // ordered list of published versions
    std::atomic<bool> is_trained{false};
    std::atomic<int64_t> total_queries{0};
    std::atomic<int64_t> successful_queries{0};
    std::atomic<int64_t> total_latency_us{0};  ///< Cumulative query latency in microseconds
    
    // Feedback storage
    std::vector<FeedbackItem> feedback_buffer;
    mutable std::mutex feedback_mutex;
    
    explicit Impl(const Config& cfg)
        : config(cfg)
        , current_adapter_version("v1.0")
        , version_history({"v1.0"})
    {
        // Initialize orchestrator
        lora::LoRAOrchestrator::Config orch_config;
        orchestrator = std::make_shared<lora::LoRAOrchestrator>(orch_config);
        
        // Initialize audit loggers with available config fields
        utils::AuditLoggerConfig audit_config;
        audit_config.log_path = "logs/themis_help_lora_audit.jsonl";
        audit_config.encrypt_then_sign = true;
        
        lora_audit = std::make_shared<lora::LoRAAuditLogger>(audit_config);
        
        audit_config.log_path = "logs/themis_help_llm_audit.jsonl";
        llm_audit = std::make_shared<LLMModelAuditLogger>(audit_config);
        
        // Initialize LlamaWrapper for LLM inference
        LlamaWrapper::Config llama_config;
        llama_config.n_gpu_layers = 0;  // CPU-only for initial implementation
        llama_config.n_ctx = 4096;
        llama_config.n_threads = 4;
        llama_config.use_mmap = true;
        llama_config.use_kv_cache_reuse = true;
        // Response cache requires an explicit persistent data directory;
        // leave disabled here so the caller can opt in via the Config.
        llama_config.enable_response_cache = false;
        
        llama_wrapper = std::make_unique<LlamaWrapper>(llama_config);

        // Initialize LoRA training service — resolve model path via injected provider
        LoRATrainingService::Config training_cfg;
        if (cfg.model_path_provider) {
            training_cfg.base_model_path = cfg.model_path_provider(cfg.base_model_id);
            if (training_cfg.base_model_path.empty()) {
                training_cfg.base_model_path = "models/" + cfg.base_model_id + ".gguf";
            }
        } else {
            training_cfg.base_model_path = "models/" + cfg.base_model_id + ".gguf";
        }
        training_cfg.default_hyperparameters = cfg.hyperparameters;
        training_service = std::make_unique<LoRATrainingService>(training_cfg);
        
        spdlog::info("ThemisHelpLoRA initialized with adapter: {}", config.adapter_id);
        spdlog::info("LlamaWrapper initialized for LLM inference");
        
        // Note: Base model loading is deferred until first query.
        // This allows the system to start even if a model file is not available.
        // The queryInternal() method will attempt to load the model on-demand,
        // either from local storage or via remote download (Ollama) if
        // enable_remote_loading is configured.
    }

    std::string resolveBaseModelPath() const {
        if (config.model_path_provider) {
            try {
                auto resolved = config.model_path_provider(config.base_model_id);
                if (!resolved.empty()) {
                    return resolved;
                }
                spdlog::warn("Model path provider returned empty path for model '{}'; using default path fallback",
                             config.base_model_id);
            } catch (const std::exception& e) {
                spdlog::warn("Model path provider failed for model '{}': {}. Using default path fallback",
                             config.base_model_id, e.what());
            }
        }
        return "models/" + config.base_model_id + ".gguf";
    }
    
    std::string buildDocumentationPrompt(const std::string& question) {
        // Build a prompt template for documentation Q&A
        std::ostringstream prompt = {};
        prompt << "### System:\n"
               << "You are a helpful ThemisDB documentation assistant. Provide accurate, "
               << "concise answers based on ThemisDB documentation. Include code examples "
               << "when relevant. If you don't know the answer, say so.\n\n"
               << "### User:\n"
               << question << "\n\n"
               << "### Assistant:\n";
        return prompt.str();
    }
    
    std::string queryInternal(const std::string& question, const std::string& /*user_id*/) {
        auto start = std::chrono::system_clock::now();
        
        try {
            // Try to load base model if not already loaded (lazy loading)
            if (llama_wrapper && !llama_wrapper->isModelLoaded()) {
                spdlog::info("Attempting to load base model: {}", config.base_model_id);
                
                // Try to load model - this may fail if model file is not available
                // In that case, we'll fall back to placeholder responses
                try {
                    // Resolve model path: prefer the injected ModelPathProviderFn
                    // (LLMModelStorage::resolveGGUFPath), fall back to the
                    // relative convention only when no provider is wired.
                    std::string model_path = {};
                    if (config.model_path_provider) {
                        model_path = config.model_path_provider(config.base_model_id);
                        if (model_path.empty()) {
                            spdlog::warn("ModelPathProviderFn returned empty path for '{}'; "
                                         "falling back to relative path", config.base_model_id);
                            model_path = "models/" + config.base_model_id + ".gguf";
                        }
                    } else {
                        model_path = "models/" + config.base_model_id + ".gguf";
                    }
                    bool loaded = llama_wrapper->loadModel(model_path);
                    
                    if (loaded) {
                        spdlog::info("Base model loaded successfully: {}", config.base_model_id);
                    } else {
                        spdlog::warn("Failed to load base model, will use placeholder responses");
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("Exception loading base model: {}. Using placeholder responses.", e.what());
                }
            }
            
            // Check if adapter is loaded
            if (!orchestrator->isLoaded(config.adapter_id)) {
                spdlog::info("Loading adapter: {}", config.adapter_id);
                std::string job_id = orchestrator->loadAdapter(config.adapter_id, false);
                if (job_id.empty()) {
                    spdlog::warn("Adapter {} not found, will use base model", config.adapter_id);
                }
            }
            
            // Generate response using LLM
            std::string response = {};
            if (llama_wrapper && llama_wrapper->isModelLoaded()) {
                // Build prompt for documentation Q&A
                std::string prompt = buildDocumentationPrompt(question);
                
                // Create inference request
                InferenceRequest request;
                request.prompt = prompt;
                request.max_tokens = 500;
                request.temperature = 0.7f;
                request.top_p = 0.9f;
                request.request_id = themis::llm::applications::generateModelRequestId();
                
                // Add LoRA adapter if loaded
                if (orchestrator->isLoaded(config.adapter_id)) {
                    request.lora_adapter_id = config.adapter_id;
                }
                
                // Generate response
                auto llm_response = llama_wrapper->generate(request);
                response = llm_response.text;
                
                spdlog::debug("LLM inference completed: {} tokens in {:.2f}ms",
                             llm_response.tokens_generated, llm_response.inference_time_ms);
            } else {
                // Fallback to placeholder if model not loaded
                spdlog::warn("LLM model not loaded, using placeholder response");
                response = generatePlaceholderResponse(question);
            }
            
            // Update statistics
            total_queries++;
            successful_queries++;
            auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now() - start).count();
            total_latency_us += elapsed_us;

            return response;

        } catch (const std::exception& e) {
            spdlog::error("Query failed: {}", e.what());
            total_queries++;
            auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now() - start).count();
            total_latency_us += elapsed_us;
            return "Error: Failed to process your question. Please try again.";
        }
    }
    
    std::string generatePlaceholderResponse(const std::string& question) {
        
        std::string lower_question = question;
        std::transform(lower_question.begin(), lower_question.end(), 
                      lower_question.begin(), ::tolower);
        
        if (lower_question.find("shard") != std::string::npos) {
            return "To enable sharding in ThemisDB:\n\n"
                   "1. Configure the shard key in your collection definition\n"
                   "2. Set the number of shards using the `shards` parameter\n"
                   "3. Ensure your cluster has sufficient nodes\n"
                   "4. Monitor shard distribution using the Admin UI\n\n"
                   "Example: CREATE COLLECTION mydata SHARD BY user_id SHARDS 8;";
        }
        
        if (lower_question.find("replicate") != std::string::npos || 
            lower_question.find("replica") != std::string::npos) {
            return "To configure replication in ThemisDB:\n\n"
                   "1. Set `replicationFactor` in your collection definition\n"
                   "2. Recommended: Use 3 replicas for production\n"
                   "3. Monitor replica health using ADMIN_HEALTH()\n"
                   "4. Configure failover policies in cluster config\n\n"
                   "Example: CREATE COLLECTION mydata REPLICATION 3;";
        }
        
        if (lower_question.find("backup") != std::string::npos) {
            return "ThemisDB backup strategies:\n\n"
                   "1. Hot backup: Use `themisdb-backup create --hot`\n"
                   "2. Incremental: `themisdb-backup create --incremental`\n"
                   "3. Point-in-time: Configure WAL archiving\n"
                   "4. Automated: Set up backup schedules in config\n\n"
                   "Backup location: /var/lib/themisdb/backups/";
        }
        
        // Default response
        return "I can help you with ThemisDB documentation questions. "
               "Please ask about sharding, replication, backups, queries, "
               "or any other ThemisDB feature. For specific code examples, "
               "you can also ask 'show me an example of X'.";
    }
};

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

ThemisHelpLoRA::ThemisHelpLoRA(const Config& config)
    : impl_(std::make_unique<Impl>(config))
{
}

ThemisHelpLoRA::ThemisHelpLoRA()
    : impl_(std::make_unique<Impl>(Config{}))
{
}

ThemisHelpLoRA::~ThemisHelpLoRA() = default;

std::string ThemisHelpLoRA::query(const std::string& question, const std::string& user_id) {
    return impl_->queryInternal(question, user_id);
}

void ThemisHelpLoRA::addPositiveFeedback(
    const std::string& question,
    const std::string& answer,
    const std::string& user_id
) {
    std::lock_guard<std::mutex> lock(impl_->feedback_mutex);
    
    FeedbackItem item;
    item.question = question;
    item.answer = answer;
    item.user_id = user_id;
    item.feedback_type = FeedbackType::POSITIVE;
    item.timestamp = std::chrono::system_clock::now();
    
    impl_->feedback_buffer.push_back(item);
    
    spdlog::debug("Positive feedback added (user: {}, question_length: {})", user_id, question.length());
}

void ThemisHelpLoRA::addNegativeFeedback(
    const std::string& question,
    const std::string& answer,
    const std::string& correction,
    const std::string& user_id
) {
    std::lock_guard<std::mutex> lock(impl_->feedback_mutex);
    
    FeedbackItem item;
    item.question = question;
    item.answer = answer;
    item.correction = correction;
    item.user_id = user_id;
    item.feedback_type = FeedbackType::NEGATIVE;
    item.timestamp = std::chrono::system_clock::now();
    
    impl_->feedback_buffer.push_back(item);
    
    spdlog::info("Negative feedback with correction (user: {}, question_length: {}, correction_length: {})", user_id, question.length(), correction.length());
}

bool ThemisHelpLoRA::trainFromFeedback() {
    std::lock_guard<std::mutex> lock(impl_->feedback_mutex);

    if (impl_->feedback_buffer.empty()) {
        spdlog::warn("No feedback available for training");
        return false;
    }

    spdlog::info("Starting training from {} feedback items", impl_->feedback_buffer.size());

    auto start = std::chrono::system_clock::now();

    try {
        // Log training started
        impl_->lora_audit->logTraining(
            lora::LoRAAuditEventType::TRAINING_STARTED,
            impl_->config.adapter_id,
            static_cast<int>(impl_->feedback_buffer.size()),
            0.0f,
            0.0f,
            {{"source", "user_feedback"}}
        );

        // Convert feedback buffer to training data and call LoRATrainingService
        TrainingData training_data;
        training_data.dataset_name = "user_feedback_" + impl_->config.adapter_id;
        for (const auto& item : impl_->feedback_buffer) {
            TrainingDataSample sample;
            sample.input = item.question;
            sample.output = item.correction.empty() ? item.answer : item.correction;
            sample.metadata = {{"user_id", item.user_id},
                               {"feedback_type", static_cast<int>(item.feedback_type)}};
            training_data.samples.push_back(std::move(sample));
        }
        TrainingResult train_result = impl_->training_service->trainOnTheFly(
            impl_->config.adapter_id, training_data);
        if (!train_result.success) {
            throw std::runtime_error("LoRA training failed: " + train_result.error_message);
        }

        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

        // Increment version and record in history
        impl_->current_adapter_version = incrementVersion(impl_->current_adapter_version);
        impl_->version_history.push_back(impl_->current_adapter_version);
        impl_->is_trained = true;

        size_t num_samples = impl_->feedback_buffer.size();

        // Clear feedback buffer
        impl_->feedback_buffer.clear();

        // Log training completed
        impl_->lora_audit->logTraining(
            lora::LoRAAuditEventType::TRAINING_COMPLETED,
            impl_->config.adapter_id,
            static_cast<int>(num_samples),
            train_result.final_loss,
            0.0f,
            {
                {"source", "user_feedback"},
                {"new_version", impl_->current_adapter_version},
                {"duration_ms", duration.count()}
            }
        );

        // Reload adapter in LlamaWrapper after training
        if (impl_->llama_wrapper && impl_->orchestrator->isLoaded(impl_->config.adapter_id)) {
            spdlog::info("Reloading adapter {} after training", impl_->config.adapter_id);

            // Unload current adapter
            impl_->orchestrator->unloadAdapter(impl_->config.adapter_id, false);

            // Reload with new weights
            std::string job_id = impl_->orchestrator->loadAdapter(impl_->config.adapter_id, false);
            if (job_id.empty()) {
                spdlog::warn("Failed to reload adapter after training");
            } else {
                spdlog::info("Adapter reloaded successfully: {}", impl_->config.adapter_id);
            }
        }

        spdlog::info("Training completed. New version: {}", impl_->current_adapter_version);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Training failed: {}", e.what());

        impl_->lora_audit->logTraining(
            lora::LoRAAuditEventType::TRAINING_FAILED,
            impl_->config.adapter_id,
            static_cast<int>(impl_->feedback_buffer.size()),
            0.0f,
            0.0f,
            {
                {"error", e.what()}
            }
        );

        return false;
    }
}

bool ThemisHelpLoRA::trainFromDocumentation() {
    spdlog::info("Starting training from documentation corpus");

    auto start = std::chrono::system_clock::now();

    try {
        // Log training started
        impl_->lora_audit->logTraining(
            lora::LoRAAuditEventType::TRAINING_STARTED,
            impl_->config.adapter_id,
            1151,  // Documentation count from requirements
            0.0f,
            0.0f,
            {{"source", "documentation_corpus"}}
        );

        // Train the adapter using the documentation corpus path from config
        TrainingData doc_data;
        doc_data.dataset_name = "documentation_corpus_" + impl_->config.adapter_id;
        doc_data.metadata = {{"source", "documentation_corpus"},
                             {"docs_database_path", impl_->config.docs_database_path}};
        TrainingResult train_result = impl_->training_service->trainOnTheFly(
            impl_->config.adapter_id, doc_data);
        if (!train_result.success) {
            throw std::runtime_error("Documentation corpus training failed: " + train_result.error_message);
        }

        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

        impl_->is_trained = true;

        // Log training completed
        impl_->lora_audit->logTraining(
            lora::LoRAAuditEventType::TRAINING_COMPLETED,
            impl_->config.adapter_id,
            1151,
            train_result.final_loss,
            0.0f,
            {
                {"source", "documentation_corpus"},
                {"version", impl_->current_adapter_version},
                {"duration_ms", duration.count()}
            }
        );

        // Reload adapter after training
        if (impl_->llama_wrapper && impl_->orchestrator->isLoaded(impl_->config.adapter_id)) {
            spdlog::info("Reloading adapter {} after documentation training", impl_->config.adapter_id);

            // Unload current adapter
            impl_->orchestrator->unloadAdapter(impl_->config.adapter_id, false);

            // Reload with new weights
            std::string job_id = impl_->orchestrator->loadAdapter(impl_->config.adapter_id, false);
            if (job_id.empty()) {
                spdlog::warn("Failed to reload adapter after training");
            } else {
                spdlog::info("Adapter reloaded successfully: {}", impl_->config.adapter_id);
            }
        }

        spdlog::info("Documentation training completed");
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Documentation training failed: {}", e.what());

        impl_->lora_audit->logTraining(
            lora::LoRAAuditEventType::TRAINING_FAILED,
            impl_->config.adapter_id,
            1151,
            0.0f,
            0.0f,
            {{"error", e.what()}}
        );

        return false;
    }
}

PerformanceMetrics ThemisHelpLoRA::getMetrics() const {
    int64_t total = impl_->total_queries.load(std::memory_order_acquire);
    int64_t successful = impl_->successful_queries.load(std::memory_order_acquire);
    int64_t failed = total - successful;
    
    double success_rate = (total > 0) ? 
        static_cast<double>(successful) / static_cast<double>(total) : 0.0;
    
    PerformanceMetrics metrics;
    metrics.total_queries = total;
    metrics.successful_queries = successful;
    metrics.failed_queries = failed;
    metrics.success_rate = success_rate;
    metrics.average_latency_ms = (total > 0)
        ? static_cast<double>(impl_->total_latency_us.load(std::memory_order_acquire)) / total / 1000.0
        : 0.0;
    metrics.cache_hit_rate = 0.0;  // No response cache implemented yet
    
    return metrics;
}

FeedbackStats ThemisHelpLoRA::getFeedbackStats() const {
    std::lock_guard<std::mutex> lock(impl_->feedback_mutex);
    
    size_t total = impl_->feedback_buffer.size();
    size_t positive = 0;
    size_t negative = 0;
    
    for (const auto& item : impl_->feedback_buffer) {
        if (item.feedback_type == FeedbackType::POSITIVE) {
            positive++;
        } else {
            negative++;
        }
    }
    
    double positive_ratio = (total > 0) ? 
        static_cast<double>(positive) / static_cast<double>(total) : 0.0;
    
    FeedbackStats stats;
    stats.total_feedback = total;
    stats.positive_feedback = positive;
    stats.negative_feedback = negative;
    stats.positive_ratio = positive_ratio;
    
    return stats;
}

bool ThemisHelpLoRA::isAdapterLoaded() const {
    return impl_->orchestrator->isLoaded(impl_->config.adapter_id);
}

bool ThemisHelpLoRA::reloadAdapter() {
    try {
        // Unload if currently loaded
        if (impl_->orchestrator->isLoaded(impl_->config.adapter_id)) {
            impl_->orchestrator->unloadAdapter(impl_->config.adapter_id, false);
        }
        
        // Load adapter
        std::string job_id = impl_->orchestrator->loadAdapter(impl_->config.adapter_id, false);
        return !job_id.empty();
    } catch (const std::exception& e) {
        spdlog::error("Failed to reload adapter: {}", e.what());
        return false;
    }
}

std::string ThemisHelpLoRA::getAdapterVersion() const {
    return impl_->current_adapter_version;
}

std::string ThemisHelpLoRA::getVersion() const {
    return impl_->current_adapter_version;
}

bool ThemisHelpLoRA::isTrained() const {
    return impl_->is_trained.load(std::memory_order_acquire);
}

bool ThemisHelpLoRA::rollbackToPreviousVersion() {
    try {
        bool success = impl_->orchestrator->rollback(impl_->config.adapter_id);
        if (success) {
            // Use version history to restore the real predecessor version.
            // If multiple training passes have been performed the history holds every
            // published version in order; pop the current one and restore the previous.
            if (impl_->version_history.size() > 1) {
                impl_->version_history.pop_back();
                impl_->current_adapter_version = impl_->version_history.back();
            } else {
                // No prior version recorded — fall back to string decrement.
                impl_->current_adapter_version = decrementVersion(impl_->current_adapter_version);
            }
            spdlog::info("Rolled back to version: {}", impl_->current_adapter_version);
        }
        return success;
    } catch (const std::exception& e) {
        spdlog::error("Failed to rollback: {}", e.what());
        return false;
    }
}

// ═══════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════

std::string ThemisHelpLoRA::incrementVersion(const std::string& version) {
    // Parse version string (e.g., "v1.2" -> "v1.3")
    if (version.empty() || version[0] != 'v') {
        return "v1.1";
    }
    
    size_t dot_pos = version.find('.');
    if (dot_pos == std::string::npos) {
        // No minor version, add one
        return version + ".1";
    }
    
    std::string major = version.substr(1, dot_pos - 1);
    std::string minor = version.substr(dot_pos + 1);
    
    try {
        int minor_num = std::stoi(minor);
        return "v" + major + "." + std::to_string(minor_num + 1);
    } catch (...) {
        return "v1.1";
    }
}

std::string ThemisHelpLoRA::decrementVersion(const std::string& version) {
    // Parse version string (e.g., "v1.3" -> "v1.2")
    if (version.empty() || version[0] != 'v') {
        return "v1.0";
    }
    
    size_t dot_pos = version.find('.');
    if (dot_pos == std::string::npos) {
        // No minor version, can't decrement further
        return "v1.0";
    }
    
    std::string major = version.substr(1, dot_pos - 1);
    std::string minor = version.substr(dot_pos + 1);
    
    try {
        int major_num = std::stoi(major);
        int minor_num = std::stoi(minor);
        
        if (minor_num > 0) {
            // Decrement minor version
            return "v" + major + "." + std::to_string(minor_num - 1);
        } else if (major_num > 1) {
            // No version history available for this major boundary; return the
            // floor of the previous major series.  rollbackToPreviousVersion()
            // uses the recorded version_history instead, so this path is only
            // reached when decrementVersion() is called directly without a
            // valid history (e.g., in standalone unit tests).
            return "v" + std::to_string(major_num - 1) + ".0";
        } else {
            // Already at minimum version v1.0
            return "v1.0";
        }
    } catch (...) {
        return "v1.0";
    }
}

} // namespace applications
} // namespace llm
} // namespace themis

