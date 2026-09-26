/**
 * @file audit_log_store.cpp
 * @brief Implementation of AuditLogStore
 *
 * Persistent audit trail for compliance.
 *
 * @date 2026-09-24
 */

#include "security/audit_log_store.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <spdlog/spdlog.h>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace themis::security {

// ============================================================================
// AuditEntry Implementation
// ============================================================================

nlohmann::json AuditEntry::to_json() const {
  return nlohmann::json{
      {"entry_id", entry_id},
      {"query_id", query_id},
      {"principal_id", principal_id},
      {"resource_id", resource_id},
      {"action", action},
      {"query_text", query_text},
      {"timestamp", std::chrono::system_clock::to_time_t(timestamp)},
      {"allowed", allowed},
      {"decision_reason", decision_reason},
      {"encryption_applied", encryption_applied},
      {"audit_required", audit_required},
      {"execution_time_ms", execution_time_ms},
      {"result_count", result_count},
      {"success", success},
      {"error_message", error_message},
  };
}

AuditEntry AuditEntry::from_json(const nlohmann::json& j) {
  AuditEntry result;
  if (j.contains("entry_id")) result.entry_id = j["entry_id"];
  if (j.contains("query_id")) result.query_id = j["query_id"];
  if (j.contains("principal_id")) result.principal_id = j["principal_id"];
  if (j.contains("resource_id")) result.resource_id = j["resource_id"];
  if (j.contains("action")) result.action = j["action"];
  if (j.contains("query_text")) result.query_text = j["query_text"];
  if (j.contains("timestamp")) {
    auto time_t = static_cast<time_t>(j["timestamp"].get<int64_t>());
    result.timestamp = std::chrono::system_clock::from_time_t(time_t);
  }
  if (j.contains("allowed")) result.allowed = j["allowed"];
  if (j.contains("decision_reason")) result.decision_reason = j["decision_reason"];
  if (j.contains("encryption_applied")) result.encryption_applied = j["encryption_applied"];
  if (j.contains("audit_required")) result.audit_required = j["audit_required"];
  if (j.contains("execution_time_ms")) result.execution_time_ms = j["execution_time_ms"];
  if (j.contains("result_count")) result.result_count = j["result_count"];
  if (j.contains("success")) result.success = j["success"];
  if (j.contains("error_message")) result.error_message = j["error_message"];
  return result;
}

// ============================================================================
// AuditLogStore Implementation
// ============================================================================

std::string AuditLogStore::EncodeEntryId(uint64_t entry_id) {
  std::string result(8, 0);
  result[0] = (entry_id >> 56) & 0xFF;
  result[1] = (entry_id >> 48) & 0xFF;
  result[2] = (entry_id >> 40) & 0xFF;
  result[3] = (entry_id >> 32) & 0xFF;
  result[4] = (entry_id >> 24) & 0xFF;
  result[5] = (entry_id >> 16) & 0xFF;
  result[6] = (entry_id >> 8) & 0xFF;
  result[7] = entry_id & 0xFF;
  return result;
}

uint64_t AuditLogStore::DecodeEntryId(const std::string& encoded) {
  if (encoded.size() != 8) return 0;
  uint64_t result = 0;
  result |= (static_cast<uint64_t>(encoded[0]) << 56);
  result |= (static_cast<uint64_t>(encoded[1]) << 48);
  result |= (static_cast<uint64_t>(encoded[2]) << 40);
  result |= (static_cast<uint64_t>(encoded[3]) << 32);
  result |= (static_cast<uint64_t>(encoded[4]) << 24);
  result |= (static_cast<uint64_t>(encoded[5]) << 16);
  result |= (static_cast<uint64_t>(encoded[6]) << 8);
  result |= static_cast<uint64_t>(encoded[7]);
  return result;
}

AuditLogStore::AuditLogStore(rocksdb::DB* db) : db_(db) {}

std::unique_ptr<AuditLogStore> AuditLogStore::Open(const std::string& db_path) {
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
  column_families.push_back(
      rocksdb::ColumnFamilyDescriptor("audit_logs", rocksdb::ColumnFamilyOptions()));

  rocksdb::DB* db = nullptr;
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DBOptions db_options;
  db_options.create_if_missing = true;
  db_options.create_missing_column_families = true;

  auto status = rocksdb::DB::Open(db_options, db_path, column_families, &handles, &db);
  if (!status.ok()) {
    spdlog::error("[AuditLogStore] Failed to open RocksDB: {}", status.ToString());
    throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
  }

  auto store = std::make_unique<AuditLogStore>(db);
  if (handles.size() >= 2) {
    store->cf_default_ = handles[0];
    store->cf_audit_logs_ = handles[1];
  }

  spdlog::info("[AuditLogStore] Opened audit log store at {}", db_path);
  return store;
}

AuditLogStore::~AuditLogStore() {
  if (db_) {
    auto status = db_->Close();
    if (!status.ok()) {
      spdlog::warn("[AuditLogStore] Failed to close RocksDB: {}", status.ToString());
    }
  }
}

uint64_t AuditLogStore::AppendEntry(const AuditEntry& entry) {
  // Get next entry ID
  std::string counter_key = "audit_counter";
  std::string counter_value;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_default_, counter_key, &counter_value);

  uint64_t next_id = 1;
  if (status.ok() && counter_value.size() == 8) {
    next_id = DecodeEntryId(counter_value) + 1;
  }

  // Write entry
  AuditEntry entry_with_id = entry;
  entry_with_id.entry_id = next_id;

  std::string entry_key = EncodeEntryId(next_id);
  std::string entry_json = entry_with_id.to_json().dump(-1);

  status = db_->Put(rocksdb::WriteOptions(), cf_audit_logs_, entry_key, entry_json);
  if (!status.ok()) {
    spdlog::error("[AuditLogStore] Failed to write entry: {}", status.ToString());
    throw std::runtime_error("Failed to write audit entry: " + status.ToString());
  }

  // Update counter
  std::string encoded_next = EncodeEntryId(next_id);
  status = db_->Put(rocksdb::WriteOptions(), cf_default_, counter_key, encoded_next);
  if (!status.ok()) {
    spdlog::error("[AuditLogStore] Failed to update counter: {}", status.ToString());
    throw std::runtime_error("Failed to update counter: " + status.ToString());
  }

  spdlog::debug("[AuditLogStore] Appended entry {} (query_id: {})", next_id, entry.query_id);
  return next_id;
}

AuditEntry AuditLogStore::GetEntry(uint64_t entry_id) const {
  std::string entry_key = EncodeEntryId(entry_id);
  std::string entry_json;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_audit_logs_, entry_key, &entry_json);

  if (status.IsNotFound()) {
    spdlog::debug("[AuditLogStore] Entry {} not found", entry_id);
    return AuditEntry();
  }

  if (!status.ok()) {
    spdlog::error("[AuditLogStore] Failed to read entry: {}", status.ToString());
    throw std::runtime_error("Failed to read entry: " + status.ToString());
  }

  try {
    auto json = nlohmann::json::parse(entry_json);
    return AuditEntry::from_json(json);
  } catch (const std::exception& e) {
    spdlog::error("[AuditLogStore] Failed to parse entry: {}", e.what());
    throw;
  }
}

std::vector<AuditEntry> AuditLogStore::ListEntries(uint64_t start_id, uint64_t end_id) const {
  std::vector<AuditEntry> result;
  auto iter = db_->NewIterator(rocksdb::ReadOptions(), cf_audit_logs_);

  std::string start_key = EncodeEntryId(start_id);
  for (iter->Seek(start_key); iter->Valid(); iter->Next()) {
    uint64_t entry_id = DecodeEntryId(iter->key().ToString());
    if (entry_id > end_id) break;

    std::string entry_json = iter->value().ToString();
    try {
      auto json = nlohmann::json::parse(entry_json);
      result.push_back(AuditEntry::from_json(json));
    } catch (const std::exception& e) {
      spdlog::warn("[AuditLogStore] Skipping malformed entry: {}", e.what());
    }
  }

  delete iter;
  return result;
}

uint64_t AuditLogStore::GetEntryCount() const {
  std::string counter_key = "audit_counter";
  std::string counter_value;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_default_, counter_key, &counter_value);

  if (!status.ok()) return 0;
  if (counter_value.size() != 8) return 0;

  return DecodeEntryId(counter_value);
}

std::vector<AuditEntry> AuditLogStore::QueryByPrincipal(const std::string& principal_id,
                                                         size_t max_results) const {
  std::vector<AuditEntry> result;
  auto iter = db_->NewIterator(rocksdb::ReadOptions(), cf_audit_logs_);

  size_t count = 0;
  for (iter->SeekToFirst(); iter->Valid() && count < max_results; iter->Next()) {
    std::string entry_json = iter->value().ToString();
    try {
      auto json = nlohmann::json::parse(entry_json);
      auto entry = AuditEntry::from_json(json);
      if (entry.principal_id == principal_id) {
        result.push_back(entry);
        count++;
      }
    } catch (const std::exception& e) {
      spdlog::warn("[AuditLogStore] Skipping malformed entry: {}", e.what());
    }
  }

  delete iter;
  return result;
}

std::vector<AuditEntry> AuditLogStore::QueryByResource(const std::string& resource_id,
                                                        size_t max_results) const {
  std::vector<AuditEntry> result;
  auto iter = db_->NewIterator(rocksdb::ReadOptions(), cf_audit_logs_);

  size_t count = 0;
  for (iter->SeekToFirst(); iter->Valid() && count < max_results; iter->Next()) {
    std::string entry_json = iter->value().ToString();
    try {
      auto json = nlohmann::json::parse(entry_json);
      auto entry = AuditEntry::from_json(json);
      if (entry.resource_id == resource_id) {
        result.push_back(entry);
        count++;
      }
    } catch (const std::exception& e) {
      spdlog::warn("[AuditLogStore] Skipping malformed entry: {}", e.what());
    }
  }

  delete iter;
  return result;
}

std::vector<AuditEntry> AuditLogStore::QueryDeniedDecisions(size_t max_results) const {
  std::vector<AuditEntry> result;
  auto iter = db_->NewIterator(rocksdb::ReadOptions(), cf_audit_logs_);

  size_t count = 0;
  for (iter->SeekToFirst(); iter->Valid() && count < max_results; iter->Next()) {
    std::string entry_json = iter->value().ToString();
    try {
      auto json = nlohmann::json::parse(entry_json);
      auto entry = AuditEntry::from_json(json);
      if (!entry.allowed) {
        result.push_back(entry);
        count++;
      }
    } catch (const std::exception& e) {
      spdlog::warn("[AuditLogStore] Skipping malformed entry: {}", e.what());
    }
  }

  delete iter;
  return result;
}

}  // namespace themis::security
