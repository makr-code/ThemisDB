/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_wal.cpp                                ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:06:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     354                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 5bee4e8e41  2026-04-03  Implement Disaster Recovery Manager and associated tests ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/transaction_wal.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <chrono>

namespace sharding {

TransactionWAL::TransactionWAL(const TransactionWALConfig& config)
    : config_(config), current_lsn_(0, 0) {}

TransactionWAL::~TransactionWAL() = default;

bool TransactionWAL::initialize() {
    try {
        // Create WAL directory
        if (!std::filesystem::exists(config_.wal_directory)) {
            std::filesystem::create_directories(config_.wal_directory);
            spdlog::info("Created transaction WAL directory: {}", config_.wal_directory);
        }

        // Create snapshot directory
        if (!std::filesystem::exists(config_.snapshot_directory)) {
            std::filesystem::create_directories(config_.snapshot_directory);
            spdlog::info("Created transaction snapshot directory: {}", config_.snapshot_directory);
        }

        // Initialize WAL manager
        themis::sharding::WALManagerConfig wal_config;
        wal_config.wal_directory = config_.wal_directory;
        wal_config.segment_size = config_.segment_size;
        wal_config.sync_on_write = config_.sync_on_write;

        wal_manager_ = std::make_unique<WALManager>(wal_config);

        spdlog::info("Transaction WAL initialized successfully");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize transaction WAL: {}", e.what());
        return false;
    }
}

LSN TransactionWAL::logBegin(const std::string& transaction_id,
                              TransactionProtocol protocol,
                              const std::vector<std::string>& participants) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::BEGIN;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.protocol = protocol;
    entry.participants = participants;

    nlohmann::json data;
    data["transaction_id"] = transaction_id;
    data["protocol"] = static_cast<int>(protocol);
    data["participants"] = participants;
    entry.data = data;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged BEGIN for transaction {} at LSN {}", transaction_id, lsn.toString());
    return lsn;
}

LSN TransactionWAL::logPrepare(const std::string& transaction_id,
                                const std::string& participant_id,
                                const nlohmann::json& data) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::PREPARE;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.participant_id = participant_id;
    entry.data = data;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged PREPARE for transaction {} participant {} at LSN {}",
                  transaction_id, participant_id, lsn.toString());
    return lsn;
}

LSN TransactionWAL::logPrepared(const std::string& transaction_id,
                                 const std::string& participant_id,
                                 bool vote,
                                 const std::string& response) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::PREPARED;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.participant_id = participant_id;
    entry.vote = vote;
    entry.reason = response;

    nlohmann::json data;
    data["vote"] = vote;
    data["response"] = response;
    entry.data = data;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged PREPARED for transaction {} participant {} vote={} at LSN {}",
                  transaction_id, participant_id, vote, lsn.toString());
    return lsn;
}

LSN TransactionWAL::logCommit(const std::string& transaction_id,
                               const nlohmann::json& data) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::COMMIT;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.data = data;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged COMMIT for transaction {} at LSN {}", transaction_id, lsn.toString());
    return lsn;
}

LSN TransactionWAL::logCommitted(const std::string& transaction_id,
                                  const std::string& participant_id) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::COMMITTED;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.participant_id = participant_id;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged COMMITTED for transaction {} participant {} at LSN {}",
                  transaction_id, participant_id, lsn.toString());
    return lsn;
}

LSN TransactionWAL::logAbort(const std::string& transaction_id,
                              const std::string& reason) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::ABORT;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.reason = reason;

    nlohmann::json data;
    data["reason"] = reason;
    entry.data = data;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged ABORT for transaction {} reason='{}' at LSN {}",
                  transaction_id, reason, lsn.toString());
    return lsn;
}

LSN TransactionWAL::logAborted(const std::string& transaction_id,
                                const std::string& participant_id) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::ABORTED;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.participant_id = participant_id;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged ABORTED for transaction {} participant {} at LSN {}",
                  transaction_id, participant_id, lsn.toString());
    return lsn;
}

LSN TransactionWAL::logCompensate(const std::string& transaction_id,
                                   const std::string& step_id,
                                   const nlohmann::json& compensation_data) {
    TransactionWALEntry entry;
    entry.type = TransactionWALEntryType::COMPENSATE;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    entry.transaction_id = transaction_id;
    entry.participant_id = step_id;  // Use participant_id field for step_id
    entry.data = compensation_data;

    auto wal_entry = toWALEntry(entry);
    LSN lsn = wal_manager_->append(wal_entry);
    
    current_lsn_ = wal_manager_->getCurrentLSN();
    entry.lsn = lsn;

    spdlog::debug("Logged COMPENSATE for transaction {} step {} at LSN {}",
                  transaction_id, step_id, lsn.toString());
    return lsn;
}

std::vector<TransactionWALEntry> TransactionWAL::readEntries(LSN start_lsn) {
    std::vector<TransactionWALEntry> entries;

    try {
        auto wal_entries = wal_manager_->readRange(start_lsn);
        
        for (const auto& wal_entry : wal_entries) {
            // Only process transaction-related entries (types 130-138)
            const auto wal_type = static_cast<uint8_t>(wal_entry.type);
            if (wal_type >= 130 && wal_type <= 138) {
                auto txn_entry = fromWALEntry(wal_entry);
                if (txn_entry.has_value()) {
                    entries.push_back(txn_entry.value());
                }
            }
        }

        spdlog::debug("Read {} transaction WAL entries from LSN {}", entries.size(), start_lsn.toString());
    } catch (const std::exception& e) {
        spdlog::error("Failed to read transaction WAL entries: {}", e.what());
    }

    return entries;
}

bool TransactionWAL::shouldCreateSnapshot(uint64_t operations_count) const {
    return operations_count >= config_.snapshot_interval;
}

LSN TransactionWAL::getCurrentLSN() const {
    if (wal_manager_) {
        return wal_manager_->getCurrentLSN();
    }
    return current_lsn_;
}

WALEntry TransactionWAL::toWALEntry(const TransactionWALEntry& txn_entry) {
    WALEntry wal_entry;
    wal_entry.lsn = txn_entry.lsn;
    wal_entry.type = static_cast<themis::sharding::WALEntryType>(static_cast<uint8_t>(txn_entry.type));
    wal_entry.timestamp = txn_entry.timestamp;

    // Serialize to JSON
    nlohmann::json payload;
    payload["transaction_id"] = txn_entry.transaction_id;
    payload["protocol"] = static_cast<int>(txn_entry.protocol);
    
    if (!txn_entry.participants.empty()) {
        payload["participants"] = txn_entry.participants;
    }
    
    if (!txn_entry.participant_id.empty()) {
        payload["participant_id"] = txn_entry.participant_id;
    }
    
    if (!txn_entry.data.is_null()) {
        payload["data"] = txn_entry.data;
    }
    
    if (txn_entry.type == TransactionWALEntryType::PREPARED) {
        payload["vote"] = txn_entry.vote;
    }
    
    if (!txn_entry.reason.empty()) {
        payload["reason"] = txn_entry.reason;
    }

    wal_entry.data = payload;
    return wal_entry;
}

std::optional<TransactionWALEntry> TransactionWAL::fromWALEntry(const WALEntry& wal_entry) {
    try {
        TransactionWALEntry txn_entry;
        txn_entry.lsn = wal_entry.lsn;
        txn_entry.type = static_cast<TransactionWALEntryType>(static_cast<uint8_t>(wal_entry.type));
        txn_entry.timestamp = wal_entry.timestamp;

        // Deserialize from JSON
        nlohmann::json payload = wal_entry.data;
        
        txn_entry.transaction_id = payload.value("transaction_id", "");
        txn_entry.protocol = static_cast<TransactionProtocol>(
            payload.value("protocol", 0)
        );
        
        if (payload.contains("participants")) {
            txn_entry.participants = payload["participants"].get<std::vector<std::string>>();
        }
        
        if (payload.contains("participant_id")) {
            txn_entry.participant_id = payload["participant_id"].get<std::string>();
        }
        
        if (payload.contains("data")) {
            txn_entry.data = payload["data"];
        }
        
        if (payload.contains("vote")) {
            txn_entry.vote = payload["vote"].get<bool>();
        }
        
        if (payload.contains("reason")) {
            txn_entry.reason = payload["reason"].get<std::string>();
        }

        return txn_entry;
    } catch (const std::exception& e) {
        spdlog::error("Failed to deserialize transaction WAL entry: {}", e.what());
        return std::nullopt;
    }
}

} // namespace sharding
