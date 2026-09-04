/**
 * @file transaction_snapshot.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/transaction_snapshot.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace sharding {

/** @brief Convert transaction state enum to persisted string token. */
std::string transactionStateToString(TransactionState state) {
    switch (state) {
        case TransactionState::INITIATED: return "INITIATED";
        case TransactionState::PREPARING: return "PREPARING";
        case TransactionState::PREPARED: return "PREPARED";
        case TransactionState::PRE_COMMITTING: return "PRE_COMMITTING";
        case TransactionState::PRE_COMMITTED: return "PRE_COMMITTED";
        case TransactionState::COMMITTING: return "COMMITTING";
        case TransactionState::COMMITTED: return "COMMITTED";
        case TransactionState::ABORTING: return "ABORTING";
        case TransactionState::ABORTED: return "ABORTED";
        case TransactionState::COMPENSATING: return "COMPENSATING";
        case TransactionState::COMPENSATED: return "COMPENSATED";
        default: return "UNKNOWN";
    }
}

/** @brief Parse transaction state enum from persisted string token. */
TransactionState transactionStateFromString(const std::string& str) {
    if (str == "INITIATED") {
      return TransactionState::INITIATED;
    }
    if (str == "PREPARING") {
      return TransactionState::PREPARING;
    }
    if (str == "PREPARED") {
      return TransactionState::PREPARED;
    }
    if (str == "PRE_COMMITTING") {
      return TransactionState::PRE_COMMITTING;
    }
    if (str == "PRE_COMMITTED") {
      return TransactionState::PRE_COMMITTED;
    }
    if (str == "COMMITTING") {
      return TransactionState::COMMITTING;
    }
    if (str == "COMMITTED") {
      return TransactionState::COMMITTED;
    }
    if (str == "ABORTING") {
      return TransactionState::ABORTING;
    }
    if (str == "ABORTED") {
      return TransactionState::ABORTED;
    }
    if (str == "COMPENSATING") {
      return TransactionState::COMPENSATING;
    }
    if (str == "COMPENSATED") {
      return TransactionState::COMPENSATED;
    }
    return TransactionState::INITIATED;
}

/** @brief Convert transaction protocol enum to persisted string token. */
std::string transactionProtocolToString(TransactionProtocol protocol) {
    switch (protocol) {
        case TransactionProtocol::TWO_PHASE_COMMIT: return "TWO_PHASE_COMMIT";
        case TransactionProtocol::THREE_PHASE_COMMIT: return "THREE_PHASE_COMMIT";
        case TransactionProtocol::SAGA: return "SAGA";
        case TransactionProtocol::PERCOLATOR: return "PERCOLATOR";
        case TransactionProtocol::CALVIN: return "CALVIN";
    }
    return "TWO_PHASE_COMMIT";
}

/** @brief Parse transaction protocol enum from persisted string token. */
TransactionProtocol transactionProtocolFromString(const std::string& str) {
    if (str == "TWO_PHASE_COMMIT") {
      return TransactionProtocol::TWO_PHASE_COMMIT;
    }
    if (str == "THREE_PHASE_COMMIT") {
      return TransactionProtocol::THREE_PHASE_COMMIT;
    }
    if (str == "SAGA") {
      return TransactionProtocol::SAGA;
    }
    if (str == "PERCOLATOR") {
      return TransactionProtocol::PERCOLATOR;
    }
    if (str == "CALVIN") {
      return TransactionProtocol::CALVIN;
    }
    return TransactionProtocol::TWO_PHASE_COMMIT;
}

/** @brief Serialize ParticipantStatus into JSON object. */
void to_json(nlohmann::json& j, const ParticipantStatus& p) {
    j = nlohmann::json{
        {"participant_id", p.participant_id},
        {"prepared", p.prepared},
        {"pre_committed", p.pre_committed},
        {"committed", p.committed},
        {"aborted", p.aborted},
        {"response_data", p.response_data},
        {"timestamp", p.timestamp}
    };
}

/** @brief Deserialize ParticipantStatus from JSON object. */
void from_json(const nlohmann::json& j, ParticipantStatus& p) {
    j.at("participant_id").get_to(p.participant_id);
    j.at("prepared").get_to(p.prepared);
    j.at("pre_committed").get_to(p.pre_committed);
    j.at("committed").get_to(p.committed);
    j.at("aborted").get_to(p.aborted);
    j.at("response_data").get_to(p.response_data);
    j.at("timestamp").get_to(p.timestamp);
}

/** @brief Serialize SAGAStep into JSON object. */
void to_json(nlohmann::json& j, const SAGAStep& s) {
    j = nlohmann::json{
        {"step_number", s.step_number},
        {"operation", s.operation},
        {"data", s.data},
        {"completed", s.completed},
        {"compensated", s.compensated},
        {"timestamp", s.timestamp}
    };
}

/** @brief Deserialize SAGAStep from JSON object. */
void from_json(const nlohmann::json& j, SAGAStep& s) {
    j.at("step_number").get_to(s.step_number);
    j.at("operation").get_to(s.operation);
    j.at("data").get_to(s.data);
    j.at("completed").get_to(s.completed);
    j.at("compensated").get_to(s.compensated);
    j.at("timestamp").get_to(s.timestamp);
}

/** @brief Serialize PercolatorIntent into JSON object. */
void to_json(nlohmann::json& j, const PercolatorIntent& i) {
    j = nlohmann::json{
        {"key", i.key},
        {"value", i.value},
        {"start_timestamp", i.start_timestamp},
        {"locked", i.locked}
    };
}

/** @brief Deserialize PercolatorIntent from JSON object. */
void from_json(const nlohmann::json& j, PercolatorIntent& i) {
    j.at("key").get_to(i.key);
    j.at("value").get_to(i.value);
    j.at("start_timestamp").get_to(i.start_timestamp);
    j.at("locked").get_to(i.locked);
}

/** @brief Serialize TransactionSnapshotEntry into JSON object. */
void to_json(nlohmann::json& j, const TransactionSnapshotEntry& e) {
    j = nlohmann::json{
        {"transaction_id", e.transaction_id},
        {"protocol", transactionProtocolToString(e.protocol)},
        {"state", transactionStateToString(e.state)},
        {"participants", e.participants},
        {"participant_status", e.participant_status},
        {"start_timestamp", e.start_timestamp},
        {"timeout_ms", e.timeout_ms},
        {"prepare_data", e.prepare_data},
        {"commit_data", e.commit_data},
        {"saga_steps", e.saga_steps},
        {"saga_compensations", e.saga_compensations},
        {"write_intents", e.write_intents},
        {"percolator_commit_timestamp", e.percolator_commit_timestamp},
        {"coordinator_id", e.coordinator_id},
        {"metadata", e.metadata}
    };
}

/** @brief Deserialize TransactionSnapshotEntry from JSON object. */
void from_json(const nlohmann::json& j, TransactionSnapshotEntry& e) {
    j.at("transaction_id").get_to(e.transaction_id);
    e.protocol = transactionProtocolFromString(j.at("protocol").get<std::string>());
    e.state = transactionStateFromString(j.at("state").get<std::string>());
    j.at("participants").get_to(e.participants);
    j.at("participant_status").get_to(e.participant_status);
    j.at("start_timestamp").get_to(e.start_timestamp);
    j.at("timeout_ms").get_to(e.timeout_ms);
    j.at("prepare_data").get_to(e.prepare_data);
    j.at("commit_data").get_to(e.commit_data);
    j.at("saga_steps").get_to(e.saga_steps);
    j.at("saga_compensations").get_to(e.saga_compensations);
    j.at("write_intents").get_to(e.write_intents);
    j.at("percolator_commit_timestamp").get_to(e.percolator_commit_timestamp);
    j.at("coordinator_id").get_to(e.coordinator_id);
    j.at("metadata").get_to(e.metadata);
}

/** @brief Serialize TransactionSnapshot to JSON excluding recomputed checksum field. */
nlohmann::json TransactionSnapshot::toJson() const {
    nlohmann::json j;
    j["snapshot_id"] = snapshot_id;
    j["last_applied_lsn"] = last_applied_lsn.toString();
    j["coordinator_id"] = coordinator_id;
    j["timestamp"] = timestamp;
    j["active_transactions"] = active_transactions;
    j["total_transactions"] = total_transactions;
    // Note: checksum is calculated separately
    return j;
}

/** @brief Deserialize TransactionSnapshot from JSON payload with optional checksum field. */
std::optional<TransactionSnapshot> TransactionSnapshot::fromJson(const nlohmann::json& j) {
    try {
        TransactionSnapshot snapshot;
        snapshot.snapshot_id = j.at("snapshot_id").get<uint64_t>();
        snapshot.last_applied_lsn = LSN::fromString(j.at("last_applied_lsn").get<std::string>());
        snapshot.coordinator_id = j.at("coordinator_id").get<std::string>();
        snapshot.timestamp = j.at("timestamp").get<uint64_t>();
        snapshot.active_transactions = j.at("active_transactions").get<std::vector<TransactionSnapshotEntry>>();
        snapshot.total_transactions = j.at("total_transactions").get<size_t>();
        if (j.contains("checksum")) {
            snapshot.checksum = j.at("checksum").get<std::string>();
        }
        return snapshot;
    } catch (const std::exception& e) {
        spdlog::error("Failed to deserialize TransactionSnapshot: {}", e.what());
        return std::nullopt;
    }
}

/**
 * @brief Construct snapshot manager and ensure snapshot directory exists.
 * @param snapshot_directory Filesystem directory for snapshot files.
 * @param max_snapshots Maximum number of retained snapshots.
 */
TransactionSnapshotManager::TransactionSnapshotManager(
    const std::string& snapshot_directory, 
    size_t max_snapshots)
    : snapshot_directory_(snapshot_directory), max_snapshots_(max_snapshots) {
    
    // Create snapshot directory if it doesn't exist
    try {
        std::filesystem::create_directories(snapshot_directory_);
        spdlog::info("Transaction snapshot directory: {}", snapshot_directory_);
    } catch (const std::exception& e) {
        spdlog::error("Failed to create snapshot directory: {}", e.what());
    }
}

/** @brief Compose snapshot filepath from directory and snapshot id. */
std::string TransactionSnapshotManager::getSnapshotPath([[maybe_unused]] uint64_t snapshot_id) const {
    return snapshot_directory_ + "/transaction_snapshot_" + std::to_string(snapshot_id) + ".json";
}

/** @brief Compute SHA-256 checksum over canonical JSON string representation. */
std::string TransactionSnapshotManager::calculateChecksum(const nlohmann::json& data) const {
    std::string json_str = data.dump();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(json_str.c_str()), 
           json_str.length(), hash);
    
    std::stringstream ss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

/**
 * @brief Create, checksum and persist a snapshot for active transactions.
 * @return Snapshot id on success; nullopt on failure.
 */
std::optional<uint64_t> TransactionSnapshotManager::createSnapshot(
    const std::string& coordinator_id,
    LSN last_applied_lsn,
    const std::vector<TransactionSnapshotEntry>& active_transactions) {
    
    try {
        TransactionSnapshot snapshot;
        snapshot.snapshot_id = std::chrono::system_clock::now().time_since_epoch().count();
        snapshot.last_applied_lsn = last_applied_lsn;
        snapshot.coordinator_id = coordinator_id;
        snapshot.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        snapshot.active_transactions = active_transactions;
        snapshot.total_transactions = active_transactions.size();
        
        // Calculate checksum
        nlohmann::json j = snapshot.toJson();
        snapshot.checksum = calculateChecksum(j);
        
        // Save to file
        if (saveSnapshotToFile(snapshot)) {
            spdlog::info("Created transaction snapshot {} with {} active transactions",
                        snapshot.snapshot_id, snapshot.total_transactions);
            
            // Cleanup old snapshots
            cleanupOldSnapshots();
            
            return snapshot.snapshot_id;
        }
        
        return std::nullopt;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create transaction snapshot: {}", e.what());
        return std::nullopt;
    }
}

/** @brief Persist snapshot JSON (including checksum) into target file. */
bool TransactionSnapshotManager::saveSnapshotToFile(const TransactionSnapshot& snapshot) {
    try {
        std::string filepath = getSnapshotPath(snapshot.snapshot_id);
        
        nlohmann::json j = snapshot.toJson();
        j["checksum"] = snapshot.checksum;
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            spdlog::error("Failed to open snapshot file for writing: {}", filepath);
            return false;
        }
        
        file << j.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        spdlog::debug("Saved transaction snapshot to {}", filepath);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save snapshot to file: {}", e.what());
        return false;
    }
}

/** @brief Load snapshot JSON from file and verify embedded checksum. */
std::optional<TransactionSnapshot> TransactionSnapshotManager::loadSnapshotFromFile(
    const std::string& filepath) {
    
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            spdlog::error("Failed to open snapshot file for reading: {}", filepath);
            return std::nullopt;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        auto snapshot = TransactionSnapshot::fromJson(j);
        if (!snapshot.has_value()) {
            return std::nullopt;
        }
        
        // Verify checksum
        if (!verifySnapshot(snapshot.value())) {
            spdlog::error("Snapshot checksum verification failed: {}", filepath);
            return std::nullopt;
        }
        
        spdlog::debug("Loaded transaction snapshot from {}", filepath);
        return snapshot;
    } catch (const std::exception& e) {
        spdlog::error("Failed to load snapshot from file: {}", e.what());
        return std::nullopt;
    }
}

/** @brief Load newest available snapshot, if any exist. */
std::optional<TransactionSnapshot> TransactionSnapshotManager::loadLatestSnapshot() {
    auto snapshots = listSnapshots();
    if (snapshots.empty()) {
        spdlog::info("No transaction snapshots available");
        return std::nullopt;
    }
    
    // Snapshots are returned in descending order, so first is latest
    return loadSnapshot(snapshots[0]);
}

/** @brief Load snapshot by explicit snapshot id. */
std::optional<TransactionSnapshot> TransactionSnapshotManager::loadSnapshot([[maybe_unused]] uint64_t snapshot_id) {
    std::string filepath = getSnapshotPath(snapshot_id);
    return loadSnapshotFromFile(filepath);
}

/** @brief Discover snapshot files and return ids sorted newest-first. */
std::vector<uint64_t> TransactionSnapshotManager::listSnapshots() {
    std::vector<uint64_t> snapshot_ids;
    
    try {
        if (!std::filesystem::exists(snapshot_directory_)) {
            return snapshot_ids;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(snapshot_directory_)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("transaction_snapshot_") == 0 && filename.ends_with(".json")) {
                    // Extract snapshot ID from filename
                    std::string id_str = filename.substr(21);  // After "transaction_snapshot_"
                    id_str = id_str.substr(0, id_str.length() - 5);  // Remove ".json"
                    
                    try {
                        uint64_t snapshot_id = std::stoull(id_str);
                        snapshot_ids.push_back(snapshot_id);
                    } catch (...) {
                        spdlog::warn("Invalid snapshot filename: {}", filename);
                    }
                }
            }
        }
        
        // Sort in descending order (newest first)
        std::sort(snapshot_ids.begin(), snapshot_ids.end(), std::greater<uint64_t>());
    } catch (const std::exception& e) {
        spdlog::error("Failed to list snapshots: {}", e.what());
    }
    
    return snapshot_ids;
}

/** @brief Delete snapshot file by id. */
bool TransactionSnapshotManager::deleteSnapshot([[maybe_unused]] uint64_t snapshot_id) {
    try {
        std::string filepath = getSnapshotPath(snapshot_id);
        if (std::filesystem::exists(filepath)) {
            std::filesystem::remove(filepath);
            spdlog::info("Deleted transaction snapshot {}", snapshot_id);
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Failed to delete snapshot {}: {}", snapshot_id, e.what());
        return false;
    }
}

/** @brief Remove oldest snapshots beyond configured retention count. */
void TransactionSnapshotManager::cleanupOldSnapshots() {
    auto snapshots = listSnapshots();
    
    if (snapshots.size() <= max_snapshots_) {
        return;
    }
    
    // Delete oldest snapshots (they're at the end of the sorted list)
    size_t to_delete = snapshots.size() - max_snapshots_;
    for (size_t i = snapshots.size() - to_delete; i < snapshots.size(); i++) {
        deleteSnapshot(snapshots[i]);
    }
    
    spdlog::info("Cleaned up {} old transaction snapshots", to_delete);
}

/** @brief Verify snapshot checksum matches current serialized content. */
bool TransactionSnapshotManager::verifySnapshot(const TransactionSnapshot& snapshot) {
    try {
        nlohmann::json j = snapshot.toJson();
        std::string calculated_checksum = calculateChecksum(j);
        
        if (calculated_checksum != snapshot.checksum) {
            spdlog::error("Snapshot checksum mismatch. Expected: {}, Got: {}",
                         snapshot.checksum, calculated_checksum);
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to verify snapshot: {}", e.what());
        return false;
    }
}

}  // namespace sharding


