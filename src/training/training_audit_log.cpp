/**
 * @file training_audit_log.cpp
 * @brief Training audit logging implementation
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#include "training/training_audit_log.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <uuid/uuid.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace themis {
namespace training {

// ============================================================================
// Utility Functions
// ============================================================================

std::string toString(TrainingAuditEventType type) {
    switch (type) {
        case TrainingAuditEventType::TRAINING_START:
            return "TRAINING_START";
        case TrainingAuditEventType::BATCH_PROCESSED:
            return "BATCH_PROCESSED";
        case TrainingAuditEventType::CHECKPOINT_SAVED:
            return "CHECKPOINT_SAVED";
        case TrainingAuditEventType::CHECKPOINT_RESTORED:
            return "CHECKPOINT_RESTORED";
        case TrainingAuditEventType::EPOCH_COMPLETED:
            return "EPOCH_COMPLETED";
        case TrainingAuditEventType::VALIDATION_RUN:
            return "VALIDATION_RUN";
        case TrainingAuditEventType::ADAPTER_SAVED:
            return "ADAPTER_SAVED";
        case TrainingAuditEventType::TRAINING_PAUSED:
            return "TRAINING_PAUSED";
        case TrainingAuditEventType::TRAINING_RESUMED:
            return "TRAINING_RESUMED";
        case TrainingAuditEventType::TRAINING_COMPLETED:
            return "TRAINING_COMPLETED";
        case TrainingAuditEventType::TRAINING_FAILED:
            return "TRAINING_FAILED";
        case TrainingAuditEventType::DATA_INTEGRITY_CHECK:
            return "DATA_INTEGRITY_CHECK";
        case TrainingAuditEventType::RNG_STATE_SNAPSHOT:
            return "RNG_STATE_SNAPSHOT";
        case TrainingAuditEventType::MEMORY_CHECKPOINT:
            return "MEMORY_CHECKPOINT";
        case TrainingAuditEventType::POLICY_APPLIED:
            return "POLICY_APPLIED";
        case TrainingAuditEventType::MODEL_PARAMETER_CHANGE:
            return "MODEL_PARAMETER_CHANGE";
        case TrainingAuditEventType::AUDIT_TRAIL_VERIFIED:
            return "AUDIT_TRAIL_VERIFIED";
        default:
            return "UNKNOWN";
    }
}

TrainingAuditEventType auditEventTypeFromString(const std::string& str) {
    if (str == "TRAINING_START") return TrainingAuditEventType::TRAINING_START;
    if (str == "BATCH_PROCESSED") return TrainingAuditEventType::BATCH_PROCESSED;
    if (str == "CHECKPOINT_SAVED") return TrainingAuditEventType::CHECKPOINT_SAVED;
    if (str == "CHECKPOINT_RESTORED") return TrainingAuditEventType::CHECKPOINT_RESTORED;
    if (str == "EPOCH_COMPLETED") return TrainingAuditEventType::EPOCH_COMPLETED;
    if (str == "VALIDATION_RUN") return TrainingAuditEventType::VALIDATION_RUN;
    if (str == "ADAPTER_SAVED") return TrainingAuditEventType::ADAPTER_SAVED;
    if (str == "TRAINING_PAUSED") return TrainingAuditEventType::TRAINING_PAUSED;
    if (str == "TRAINING_RESUMED") return TrainingAuditEventType::TRAINING_RESUMED;
    if (str == "TRAINING_COMPLETED") return TrainingAuditEventType::TRAINING_COMPLETED;
    if (str == "TRAINING_FAILED") return TrainingAuditEventType::TRAINING_FAILED;
    if (str == "DATA_INTEGRITY_CHECK") return TrainingAuditEventType::DATA_INTEGRITY_CHECK;
    if (str == "RNG_STATE_SNAPSHOT") return TrainingAuditEventType::RNG_STATE_SNAPSHOT;
    if (str == "MEMORY_CHECKPOINT") return TrainingAuditEventType::MEMORY_CHECKPOINT;
    if (str == "POLICY_APPLIED") return TrainingAuditEventType::POLICY_APPLIED;
    if (str == "MODEL_PARAMETER_CHANGE") return TrainingAuditEventType::MODEL_PARAMETER_CHANGE;
    if (str == "AUDIT_TRAIL_VERIFIED") return TrainingAuditEventType::AUDIT_TRAIL_VERIFIED;
    
    throw std::runtime_error("Unknown audit event type: " + str);
}

// ============================================================================
// TrainingAuditEvent Implementation
// ============================================================================

std::string TrainingAuditEvent::computeHash() const {
    // Simple implementation: hash the JSON representation
    json event_obj = {
        {"event_type", toString(event_type)},
        {"event_id", event_id},
        {"timestamp", std::chrono::system_clock::to_time_t(timestamp)},
        {"training_run_id", training_run_id},
        {"actor_id", actor_id},
        {"session_id", session_id},
        {"event_data", event_data},
        {"previous_event_hash", previous_event_hash}
    };
    
    std::string event_str = event_obj.dump();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(event_str.c_str()),
           event_str.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

json TrainingAuditEvent::toJSON() const {
    json j;
    j["event_type"] = toString(event_type);
    j["event_id"] = event_id;
    j["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
    j["training_run_id"] = training_run_id;
    j["actor_id"] = actor_id;
    j["session_id"] = session_id;
    j["event_data"] = event_data;
    j["previous_event_hash"] = previous_event_hash;
    j["event_hash"] = event_hash;
    j["signature"] = signature;
    j["finalized"] = finalized;
    j["error_message"] = error_message;
    return j;
}

TrainingAuditEvent TrainingAuditEvent::fromJSON(const json& j) {
    TrainingAuditEvent event;
    event.event_type = auditEventTypeFromString(j.at("event_type").get<std::string>());
    event.event_id = j.at("event_id").get<std::string>();
    event.timestamp = std::chrono::system_clock::from_time_t(j.at("timestamp").get<time_t>());
    event.training_run_id = j.at("training_run_id").get<std::string>();
    event.actor_id = j.at("actor_id").get<std::string>();
    event.session_id = j.at("session_id").get<std::string>();
    event.event_data = j.at("event_data");
    event.previous_event_hash = j.at("previous_event_hash").get<std::string>();
    event.event_hash = j.at("event_hash").get<std::string>();
    event.signature = j.value("signature", "");
    event.finalized = j.at("finalized").get<bool>();
    event.error_message = j.value("error_message", "");
    return event;
}

// ============================================================================
// TrainingAuditLog Implementation
// ============================================================================

TrainingAuditLog::TrainingAuditLog(
    const std::string& training_run_id,
    const std::string& actor_id,
    const std::string& persistence_dir)
    : training_run_id_(training_run_id),
      actor_id_(actor_id),
      persistence_dir_(persistence_dir) {
    THEMIS_INFO("Initialized TrainingAuditLog for run: {}", training_run_id);
}

TrainingAuditLog::~TrainingAuditLog() = default;

const TrainingAuditEvent& TrainingAuditLog::logEvent(
    TrainingAuditEventType event_type,
    const json& event_data) {
    
    TrainingAuditEvent event;
    event.event_type = event_type;
    
    // Generate UUID for event
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    event.event_id = std::string(uuid_str);
    
    event.timestamp = std::chrono::system_clock::now();
    event.training_run_id = training_run_id_;
    event.actor_id = actor_id_;
    event.session_id = training_run_id_;  // Use training run ID as session ID for now
    event.event_data = event_data;
    
    // Link to previous event
    if (!events_.empty()) {
        event.previous_event_hash = events_.back().event_hash;
    }
    
    // Compute hash
    event.event_hash = event.computeHash();
    event.finalized = true;
    
    return recordEvent(event);
}

const TrainingAuditEvent& TrainingAuditLog::logError(
    const std::string& error_message,
    const json& event_data) {
    
    json error_data = event_data;
    error_data["error"] = error_message;
    
    TrainingAuditEvent event;
    event.event_type = TrainingAuditEventType::TRAINING_FAILED;
    event.error_message = error_message;
    
    // Generate UUID
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    event.event_id = std::string(uuid_str);
    
    event.timestamp = std::chrono::system_clock::now();
    event.training_run_id = training_run_id_;
    event.actor_id = actor_id_;
    event.session_id = training_run_id_;
    event.event_data = error_data;
    
    if (!events_.empty()) {
        event.previous_event_hash = events_.back().event_hash;
    }
    
    event.event_hash = event.computeHash();
    event.finalized = true;
    
    return recordEvent(event);
}

const TrainingAuditEvent& TrainingAuditLog::recordEvent(TrainingAuditEvent event) {
    std::lock_guard<std::mutex> lock(events_mutex_);
    events_.push_back(event);
    
    THEMIS_DEBUG("Logged audit event: {} (ID: {})", 
                 toString(event.event_type), 
                 event.event_id);
    
    return events_.back();
}

const std::vector<TrainingAuditEvent>& TrainingAuditLog::getAllEvents() const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    return events_;
}

std::vector<TrainingAuditEvent> TrainingAuditLog::getEventsByType(
    TrainingAuditEventType event_type) const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    std::vector<TrainingAuditEvent> result;
    for (const auto& event : events_) {
        if (event.event_type == event_type) {
            result.push_back(event);
        }
    }
    return result;
}

size_t TrainingAuditLog::getEventCount() const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    return events_.size();
}

std::chrono::system_clock::time_point TrainingAuditLog::getStartTime() const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    auto it = std::find_if(events_.begin(), events_.end(),
        [](const TrainingAuditEvent& e) { return e.event_type == TrainingAuditEventType::TRAINING_START; });
    if (it != events_.end()) {
        return it->timestamp;
    }
    return std::chrono::system_clock::time_point();
}

std::chrono::system_clock::time_point TrainingAuditLog::getEndTime() const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    auto it = std::find_if(events_.rbegin(), events_.rend(),
        [](const TrainingAuditEvent& e) { 
            return e.event_type == TrainingAuditEventType::TRAINING_COMPLETED || 
                   e.event_type == TrainingAuditEventType::TRAINING_FAILED;
        });
    if (it != events_.rend()) {
        return it->timestamp;
    }
    return std::chrono::system_clock::time_point();
}

double TrainingAuditLog::getDurationSeconds() const {
    auto start = getStartTime();
    auto end = getEndTime();
    if (start == std::chrono::system_clock::time_point() ||
        end == std::chrono::system_clock::time_point()) {
        return 0.0;
    }
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    return static_cast<double>(duration.count());
}

bool TrainingAuditLog::verifyChainIntegrity() const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    if (events_.empty()) {
        return true;
    }
    
    for (size_t i = 1; i < events_.size(); ++i) {
        // Check that previous_event_hash matches predecessor's event_hash
        if (events_[i].previous_event_hash != events_[i-1].event_hash) {
            THEMIS_ERROR("Chain integrity violation at event {}", i);
            return false;
        }
        
        // Check that event hash matches computed hash
        std::string computed_hash = events_[i].computeHash();
        if (computed_hash != events_[i].event_hash) {
            THEMIS_ERROR("Event hash mismatch at event {}", i);
            return false;
        }
    }
    
    return true;
}

std::string TrainingAuditLog::getMerkleRootHash() const {
    if (!verifyChainIntegrity()) {
        throw std::runtime_error("Chain integrity verification failed");
    }
    
    std::lock_guard<std::mutex> lock(events_mutex_);
    if (events_.empty()) {
        return "";
    }
    
    return events_.back().event_hash;
}

json TrainingAuditLog::toJSON(bool include_signatures) const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    json events_array = json::array();
    for (const auto& event : events_) {
        auto event_json = event.toJSON();
        if (!include_signatures) {
            event_json.erase("signature");
        }
        events_array.push_back(event_json);
    }
    return events_array;
}

std::string TrainingAuditLog::toJSONLines() const {
    std::lock_guard<std::mutex> lock(events_mutex_);
    std::stringstream ss;
    for (const auto& event : events_) {
        ss << event.toJSON().dump() << "\n";
    }
    return ss.str();
}

void TrainingAuditLog::persistToFile(const std::string& file_path) const {
    std::string temp_path = file_path + ".tmp";
    
    std::ofstream file(temp_path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open audit log file: " + temp_path);
    }
    
    file << toJSONLines();
    file.close();
    
    // Atomic rename
    if (std::rename(temp_path.c_str(), file_path.c_str()) != 0) {
        throw std::runtime_error("Failed to persist audit log: atomic rename failed");
    }
    
    THEMIS_INFO("Persisted audit log to: {}", file_path);
}

TrainingAuditLog TrainingAuditLog::loadFromFile(
    const std::string& file_path,
    const std::string& actor_id) {
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Audit log file not found: " + file_path);
    }
    
    // Parse training_run_id from first event (this is a simplified approach)
    std::string first_line;
    std::getline(file, first_line);
    if (first_line.empty()) {
        throw std::runtime_error("Audit log file is empty");
    }
    
    auto first_event_json = json::parse(first_line);
    std::string training_run_id = first_event_json.at("training_run_id").get<std::string>();
    
    TrainingAuditLog audit_log(training_run_id, actor_id);
    
    // Read all events
    file.seekg(0);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto event_json = json::parse(line);
        auto event = TrainingAuditEvent::fromJSON(event_json);
        audit_log.recordEvent(event);
    }
    
    file.close();
    
    if (!audit_log.verifyChainIntegrity()) {
        throw std::runtime_error("Audit log chain integrity verification failed");
    }
    
    THEMIS_INFO("Loaded audit log from: {} ({} events)", file_path, audit_log.getEventCount());
    return audit_log;
}

bool TrainingAuditLog::hasFailed() const {
    auto failures = getEventsByType(TrainingAuditEventType::TRAINING_FAILED);
    return !failures.empty();
}

bool TrainingAuditLog::hasCompleted() const {
    auto completions = getEventsByType(TrainingAuditEventType::TRAINING_COMPLETED);
    auto failures = getEventsByType(TrainingAuditEventType::TRAINING_FAILED);
    return !completions.empty() || !failures.empty();
}

std::string TrainingAuditLog::getLastErrorMessage() const {
    auto failures = getEventsByType(TrainingAuditEventType::TRAINING_FAILED);
    if (!failures.empty()) {
        return failures.back().error_message;
    }
    return "";
}

// ============================================================================
// TrainingAuditLogGuard Implementation
// ============================================================================

TrainingAuditLogGuard::TrainingAuditLogGuard(
    TrainingAuditLog& audit_log,
    const json& start_data)
    : audit_log_(audit_log) {
    audit_log_.logEvent(TrainingAuditEventType::TRAINING_START, start_data);
}

TrainingAuditLogGuard::~TrainingAuditLogGuard() {
    if (!audit_log_.hasCompleted()) {
        audit_log_.logEvent(TrainingAuditEventType::TRAINING_COMPLETED, 
                           json{{"auto_completed", true}});
    }
}

} // namespace training
} // namespace themis
