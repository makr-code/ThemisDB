/**
 * @file lora_audit_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=19, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_audit_logger.h"
#include "utils/audit_logger.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Implementation of LoRA Audit Logger
 */
class LoRAAuditLogger::Impl {
public:
    explicit Impl(const utils::AuditLoggerConfig& config) 
        : config_(config), enabled_(config.enabled) {
        
        // Initialize base audit logger (requires encryption, pki, config)
        // audit_logger_ = std::make_unique<utils::AuditLogger>(nullptr, nullptr, config);
        
        // Create LoRA-specific audit log file
        lora_log_path_ = "data/logs/lora_audit.jsonl";
        
        spdlog::info("LoRAAuditLogger initialized:");
        spdlog::info("  Enabled: {}", enabled_);
        spdlog::info("  Log path: {}", lora_log_path_);
        spdlog::info("  Encrypt-then-sign: {}", config_.encrypt_then_sign);
        spdlog::info("  Hash chain: {}", config_.enable_hash_chain);
    }

    void setProvenanceMgr(std::shared_ptr<LoRAProvenanceManager> mgr) {
        std::lock_guard<std::mutex> lock(mutex_);
        provenance_mgr_ = std::move(mgr);
    }
    
    void logInference(const LoRAInferenceAudit& audit) {
        if (!enabled_) {
          return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            json log_entry = audit.toJSON();
            log_entry["event_type"] = "INFERENCE";
            log_entry["log_type"] = "lora_inference";
            
            // Write to LoRA-specific log
            writeToLog(log_entry);
            
            // Also log to base audit logger for centralized tracking (if available)
            if (audit_logger_) {
                audit_logger_->logEvent(log_entry);
            }

            // Feed the cryptographic Merkle audit chain (if a provenance manager is set)
            if (provenance_mgr_) {
                InferenceAuditEntry e;
                e.request_id    = audit.request_id;
                e.query_hash    = LoRAProvenanceManager::sha256Hex(audit.prompt);
                e.response_hash = LoRAProvenanceManager::sha256Hex(audit.response);
                // Use the base model id as the model identifier; the actual weight
                // hash is not available here without loading the weights file.
                e.model_hash    = LoRAProvenanceManager::sha256Hex(audit.base_model_id);
                e.adapter_hash  = audit.adapter_hash;
                e.metadata      = {
                    {"base_model_id",  audit.base_model_id},
                    {"adapter_id",     audit.adapter_id},
                    {"adapter_version",audit.adapter_version},
                    {"session_id",     audit.session_id},
                    {"user_id",        audit.user_id}
                };
                // The LoRAProvenanceManager uses its own internal mutex, which is
                // always acquired in the same order (prov_mgr → nothing).  We call
                // it while holding mutex_ here; this is safe because no code path
                // acquires mutex_ while already holding the provenance manager's lock.
                provenance_mgr_->appendAuditEntry(audit.adapter_id, std::move(e));
            }

            // Update statistics
            inference_count_++;
            
            spdlog::debug("Logged inference: model={}, adapter={}, request={}",
                         audit.base_model_id, audit.adapter_id, audit.request_id);
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to log inference: {}", e.what());
        }
    }
    
    void logEvent(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const json& details
    ) {
        if (!enabled_) {
          return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            json log_entry;
            log_entry["timestamp"] = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()
            );
            log_entry["event_type"] = eventTypeToString(event_type);
            log_entry["adapter_id"] = adapter_id;
            log_entry["details"] = details;
            log_entry["log_type"] = "lora_event";
            
            writeToLog(log_entry);
            
            event_count_++;
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to log event: {}", e.what());
        }
    }
    
    void logAdapterLifecycle(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& version,
        const json& metadata
    ) {
        json details;
        details["version"] = version;
        details["metadata"] = metadata;
        logEvent(event_type, adapter_id, details);
    }
    
    void logTraining(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        int num_samples,
        float final_loss,
        float validation_accuracy,
        const json& hyperparameters
    ) {
        json details;
        details["num_samples"] = num_samples;
        details["final_loss"] = final_loss;
        details["validation_accuracy"] = validation_accuracy;
        details["hyperparameters"] = hyperparameters;
        logEvent(event_type, adapter_id, details);
    }
    
    void logFeedback(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& question,
        const std::string& answer,
        const std::string& correction,
        const std::string& user_id
    ) {
        json details;
        details["question"] = question;
        details["answer"] = answer;
        if (!correction.empty()) {
            details["correction"] = correction;
        }
        if (!user_id.empty()) {
            details["user_id"] = user_id;
        }
        logEvent(event_type, adapter_id, details);
    }
    
    void logVersioning(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& from_version,
        const std::string& to_version,
        const std::string& reason
    ) {
        json details;
        details["from_version"] = from_version;
        details["to_version"] = to_version;
        if (!reason.empty()) {
            details["reason"] = reason;
        }
        logEvent(event_type, adapter_id, details);
    }
    
    std::vector<json> queryLogs(
        const std::string& adapter_id,
        std::optional<std::chrono::system_clock::time_point> start_time,
        std::optional<std::chrono::system_clock::time_point> end_time
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<json> results;
        
        try {
            std::ifstream file(lora_log_path_);
            if (!file.is_open()) {
                return results;
            }
            
            std::string line = {};
            while (std::getline(file, line)) {
                if (line.empty()) {
                  continue;
                }
                
                try {
                    json entry = json::parse(line);
                    
                    // Filter by adapter_id
                    if (entry.contains("adapter_id") && 
                        entry["adapter_id"] == adapter_id) {
                        
                        // Filter by time range if specified
                        bool in_range = true;
                        if (entry.contains("timestamp")) {
                            std::time_t ts = entry["timestamp"];
                            auto entry_time = std::chrono::system_clock::from_time_t(ts);
                            
                            if (start_time && entry_time < *start_time) {
                                in_range = false;
                            }
                            if (end_time && entry_time > *end_time) {
                                in_range = false;
                            }
                        }
                        
                        if (in_range) {
                            results.push_back(entry);
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("Failed to parse log entry: {}", e.what());
                }
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to query logs: {}", e.what());
        }
        
        return results;
    }
    
    std::vector<LoRAInferenceAudit> getInferenceHistory(
        const std::string& adapter_id,
        int limit
    ) {
        std::vector<LoRAInferenceAudit> results;
        auto logs = queryLogs(adapter_id, std::nullopt, std::nullopt);
        
        for (const auto& log : logs) {
            if (log.contains("log_type") && log["log_type"] == "lora_inference") {
                LoRAInferenceAudit audit;
                
                // Parse log entry back to audit structure
                if (log.contains("request_id")) {
                  audit.request_id = log["request_id"];
                }
                if (log.contains("base_model_id")) {
                  audit.base_model_id = log["base_model_id"];
                }
                if (log.contains("adapter_id")) {
                  audit.adapter_id = log["adapter_id"];
                }
                if (log.contains("adapter_version")) {
                  audit.adapter_version = log["adapter_version"];
                }
                if (log.contains("prompt")) {
                  audit.prompt = log["prompt"];
                }
                if (log.contains("response")) {
                  audit.response = log["response"];
                }
                if (log.contains("success")) {
                  audit.success = log["success"];
                }
                
                results.push_back(audit);
                
                if (results.size() >= static_cast<size_t>(limit)) {
                    break;
                }
            }
        }
        
        return results;
    }
    
    json getAdapterStats(const std::string& adapter_id) {
        auto logs = queryLogs(adapter_id, std::nullopt, std::nullopt);
        
        json stats;
        stats["adapter_id"] = adapter_id;
        stats["total_events"] = logs.size();
        
        int inferences = 0;
        int trainings = 0;
        int positive_feedback = 0;
        int negative_feedback = 0;
        int versions = 0;
        
        for (const auto& log : logs) {
            if (log.contains("log_type")) {
                std::string log_type = log["log_type"];
                if (log_type == "lora_inference") {
                    inferences++;
                }
            }
            
            if (log.contains("event_type")) {
                std::string event_type = log["event_type"];
                if (event_type == "TRAINING_COMPLETED") {
                  trainings++;
                }
                if (event_type == "FEEDBACK_POSITIVE") {
                  positive_feedback++;
                }
                if (event_type == "FEEDBACK_NEGATIVE") {
                  negative_feedback++;
                }
                if (event_type == "VERSION_CREATED") {
                  versions++;
                }
            }
        }
        
        stats["inferences"] = inferences;
        stats["trainings"] = trainings;
        stats["positive_feedback"] = positive_feedback;
        stats["negative_feedback"] = negative_feedback;
        stats["versions"] = versions;
        stats["feedback_ratio"] = positive_feedback + negative_feedback > 0 ?
            static_cast<float>(positive_feedback) / (positive_feedback + negative_feedback) : 0.0f;
        
        return stats;
    }
    
    void setEnabled([[maybe_unused]] bool enabled) {
        enabled_ = enabled;
        spdlog::info("LoRAAuditLogger {}", enabled ? "enabled" : "disabled");
    }
    
    void flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_.flush();
        }
        if (audit_logger_) {
            audit_logger_->flush();
        }
    }

private:
    utils::AuditLoggerConfig config_;
    std::unique_ptr<utils::AuditLogger> audit_logger_;
    std::string lora_log_path_;
    std::ofstream log_file_;
    mutable std::mutex mutex_;
    bool enabled_;

    // Provenance manager (optional) – set via setProvenanceManager()
    std::shared_ptr<LoRAProvenanceManager> provenance_mgr_;

    // Statistics
    uint64_t inference_count_ = 0;
    uint64_t event_count_ = 0;
    
    void writeToLog(const json& entry) {
        // Open log file if not already open
        if (!log_file_.is_open()) {
            log_file_.open(lora_log_path_, std::ios::app);
            if (!log_file_.is_open()) {
                spdlog::error("Failed to open log file: {}", lora_log_path_);
                return;
            }
        }
        
        // Write as JSON Lines format (one JSON object per line)
        log_file_ << entry.dump() << std::endl;
    }
    
    static std::string eventTypeToString(LoRAAuditEventType type) {
        switch (type) {
            case LoRAAuditEventType::INFERENCE_STARTED: return "INFERENCE_STARTED";
            case LoRAAuditEventType::INFERENCE_COMPLETED: return "INFERENCE_COMPLETED";
            case LoRAAuditEventType::INFERENCE_FAILED: return "INFERENCE_FAILED";
            case LoRAAuditEventType::ADAPTER_LOADED: return "ADAPTER_LOADED";
            case LoRAAuditEventType::ADAPTER_UNLOADED: return "ADAPTER_UNLOADED";
            case LoRAAuditEventType::ADAPTER_SWITCHED: return "ADAPTER_SWITCHED";
            case LoRAAuditEventType::TRAINING_STARTED: return "TRAINING_STARTED";
            case LoRAAuditEventType::TRAINING_COMPLETED: return "TRAINING_COMPLETED";
            case LoRAAuditEventType::TRAINING_FAILED: return "TRAINING_FAILED";
            case LoRAAuditEventType::TRAINING_DATA_ADDED: return "TRAINING_DATA_ADDED";
            case LoRAAuditEventType::FEEDBACK_POSITIVE: return "FEEDBACK_POSITIVE";
            case LoRAAuditEventType::FEEDBACK_NEGATIVE: return "FEEDBACK_NEGATIVE";
            case LoRAAuditEventType::FEEDBACK_CORRECTION: return "FEEDBACK_CORRECTION";
            case LoRAAuditEventType::VERSION_CREATED: return "VERSION_CREATED";
            case LoRAAuditEventType::VERSION_SWITCHED: return "VERSION_SWITCHED";
            case LoRAAuditEventType::VERSION_ROLLED_BACK: return "VERSION_ROLLED_BACK";
            case LoRAAuditEventType::ADAPTER_CREATED: return "ADAPTER_CREATED";
            case LoRAAuditEventType::ADAPTER_UPDATED: return "ADAPTER_UPDATED";
            case LoRAAuditEventType::ADAPTER_DELETED: return "ADAPTER_DELETED";
            case LoRAAuditEventType::ADAPTER_IMPORTED: return "ADAPTER_IMPORTED";
            case LoRAAuditEventType::ADAPTER_EXPORTED: return "ADAPTER_EXPORTED";
            case LoRAAuditEventType::METADATA_UPDATED: return "METADATA_UPDATED";
            case LoRAAuditEventType::HYPERPARAMETERS_CHANGED: return "HYPERPARAMETERS_CHANGED";
            case LoRAAuditEventType::ADAPTER_ENCRYPTED: return "ADAPTER_ENCRYPTED";
            case LoRAAuditEventType::ADAPTER_SIGNED: return "ADAPTER_SIGNED";
            case LoRAAuditEventType::SIGNATURE_VERIFIED: return "SIGNATURE_VERIFIED";
            case LoRAAuditEventType::SIGNATURE_FAILED: return "SIGNATURE_FAILED";
            case LoRAAuditEventType::ACCURACY_THRESHOLD_VIOLATED: return "ACCURACY_THRESHOLD_VIOLATED";
            case LoRAAuditEventType::ROLLBACK_TRIGGERED: return "ROLLBACK_TRIGGERED";
            case LoRAAuditEventType::CACHE_HIT: return "CACHE_HIT";
            case LoRAAuditEventType::CACHE_MISS: return "CACHE_MISS";
            case LoRAAuditEventType::CACHE_EVICTION: return "CACHE_EVICTION";
            case LoRAAuditEventType::PROVENANCE_ATTACHED:   return "PROVENANCE_ATTACHED";
            case LoRAAuditEventType::PROVENANCE_VERIFIED:   return "PROVENANCE_VERIFIED";
            case LoRAAuditEventType::SNAPSHOT_CREATED:      return "SNAPSHOT_CREATED";
            case LoRAAuditEventType::AUDIT_CHAIN_VERIFIED:  return "AUDIT_CHAIN_VERIFIED";
            case LoRAAuditEventType::AUDIT_CHAIN_TAMPERED:  return "AUDIT_CHAIN_TAMPERED";
            default: return "UNKNOWN";
        }
    }
};

// LoRAAuditLogger public interface

LoRAAuditLogger::LoRAAuditLogger(const utils::AuditLoggerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

LoRAAuditLogger::~LoRAAuditLogger() = default;

void LoRAAuditLogger::logInference(const LoRAInferenceAudit& audit) {
    impl_->logInference(audit);
}

void LoRAAuditLogger::logEvent(
    LoRAAuditEventType event_type,
    const std::string& adapter_id,
    const json& details
) {
    impl_->logEvent(event_type, adapter_id, details);
}

void LoRAAuditLogger::logAdapterLifecycle(
    LoRAAuditEventType event_type,
    const std::string& adapter_id,
    const std::string& version,
    const json& metadata
) {
    impl_->logAdapterLifecycle(event_type, adapter_id, version, metadata);
}

void LoRAAuditLogger::logTraining(
    LoRAAuditEventType event_type,
    const std::string& adapter_id,
    int num_samples,
    float final_loss,
    float validation_accuracy,
    const json& hyperparameters
) {
    impl_->logTraining(event_type, adapter_id, num_samples, final_loss, 
                       validation_accuracy, hyperparameters);
}

void LoRAAuditLogger::logFeedback(
    LoRAAuditEventType event_type,
    const std::string& adapter_id,
    const std::string& question,
    const std::string& answer,
    const std::string& correction,
    const std::string& user_id
) {
    impl_->logFeedback(event_type, adapter_id, question, answer, correction, user_id);
}

void LoRAAuditLogger::logVersioning(
    LoRAAuditEventType event_type,
    const std::string& adapter_id,
    const std::string& from_version,
    const std::string& to_version,
    const std::string& reason
) {
    impl_->logVersioning(event_type, adapter_id, from_version, to_version, reason);
}

std::vector<json> LoRAAuditLogger::queryLogs(
    const std::string& adapter_id,
    std::optional<std::chrono::system_clock::time_point> start_time,
    std::optional<std::chrono::system_clock::time_point> end_time
) {
    return impl_->queryLogs(adapter_id, start_time, end_time);
}

std::vector<LoRAInferenceAudit> LoRAAuditLogger::getInferenceHistory(
    const std::string& adapter_id,
    int limit
) {
    return impl_->getInferenceHistory(adapter_id, limit);
}

json LoRAAuditLogger::getAdapterStats(const std::string& adapter_id) {
    return impl_->getAdapterStats(adapter_id);
}

void LoRAAuditLogger::setEnabled([[maybe_unused]] bool enabled) {
    impl_->setEnabled(enabled);
}

void LoRAAuditLogger::flush() {
    impl_->flush();
}

// ── Provenance & Merkle-chain integration ─────────────────────────────────────

void LoRAAuditLogger::setProvenanceManager(std::shared_ptr<LoRAProvenanceManager> mgr) {
    impl_->setProvenanceMgr(std::move(mgr));
}

void LoRAAuditLogger::logProvenanceAttached(const std::string& adapter_id,
                                              const LoRAProvenanceRecord& record) {
    json details = {
        {"dataset_hash",        record.dataset_hash},
        {"base_model_hash",     record.base_model_hash},
        {"adapter_weights_hash",record.adapter_weights_hash},
        {"trainer_id",          record.trainer_id},
        {"has_rfc3161_token",   !record.rfc3161_timestamp.empty()},
        {"has_ca_chain",        !record.ca_chain.empty()}
    };
    logEvent(LoRAAuditEventType::PROVENANCE_ATTACHED, adapter_id, details);
}

void LoRAAuditLogger::logSnapshotCreated(const std::string& adapter_id,
                                          const AdapterSnapshot& snapshot) {
    json details = {
        {"snapshot_id",         snapshot.snapshot_id},
        {"version",             snapshot.version},
        {"weights_hash",        snapshot.weights_hash},
        {"parent_snapshot_id",  snapshot.parent_snapshot_id},
        {"timestamp",           snapshot.timestamp}
    };
    logEvent(LoRAAuditEventType::SNAPSHOT_CREATED, adapter_id, details);
}

void LoRAAuditLogger::logAuditChainVerified(const std::string& adapter_id,
                                              bool valid,
                                              std::size_t entry_count) {
    const auto event_type = valid ? LoRAAuditEventType::AUDIT_CHAIN_VERIFIED
                                  : LoRAAuditEventType::AUDIT_CHAIN_TAMPERED;
    json details = {
        {"chain_valid",  valid},
        {"entry_count",  entry_count},
        {"message",      valid ? "Merkle audit chain is intact"
                               : "Merkle audit chain verification FAILED — possible tampering"}
    };
    logEvent(event_type, adapter_id, details);
}

// Helper functions

std::string generateRequestId() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << dis(gen);
    oss << std::setw(16) << dis(gen);
    return oss.str();
}

std::string computeAdapterHash(const std::vector<uint8_t>& weights) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(weights.data(), weights.size(), hash);
    
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

} // namespace lora
} // namespace llm
} // namespace themis

