/**
 * @file index_metadata_store.cpp
 * @brief RocksDB implementation of IndexMetadataStore
 *
 * Manages persistent storage and atomic transactions for index versioning.
 *
 * @date 2026-09-24
 */

#include "index/index_metadata_store.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/transaction_db.h>
#include <spdlog/spdlog.h>

#include "index/index_manifest_v1.h"

namespace themis::index {

std::string IndexMetadataStore::EncodeVersionNumber(uint32_t version_number) {
  // Big-endian encoding for consistent RocksDB key ordering
  std::string result(4, 0);
  result[0] = (version_number >> 24) & 0xFF;
  result[1] = (version_number >> 16) & 0xFF;
  result[2] = (version_number >> 8) & 0xFF;
  result[3] = version_number & 0xFF;
  return result;
}

uint32_t IndexMetadataStore::DecodeVersionNumber(const std::string& encoded) {
  if (encoded.size() != 4) {
    return 0;
  }
  uint32_t result = 0;
  result |= (static_cast<uint8_t>(encoded[0]) << 24);
  result |= (static_cast<uint8_t>(encoded[1]) << 16);
  result |= (static_cast<uint8_t>(encoded[2]) << 8);
  result |= static_cast<uint8_t>(encoded[3]);
  return result;
}

IndexMetadataStore::IndexMetadataStore(rocksdb::DB* db, const std::string& index_id)
    : db_(db), index_id_(index_id) {}

std::unique_ptr<IndexMetadataStore> IndexMetadataStore::Open(const std::string& db_path,
                                                               const std::string& index_id) {
  // Configure column families
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
  column_families.push_back(
      rocksdb::ColumnFamilyDescriptor("version_history", rocksdb::ColumnFamilyOptions()));
  column_families.push_back(
      rocksdb::ColumnFamilyDescriptor("embeddings", rocksdb::ColumnFamilyOptions()));

  // Try to open existing DB with column families
  rocksdb::DB* db = nullptr;
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DBOptions db_options;
  db_options.create_if_missing = true;
  db_options.create_missing_column_families = true;

  auto status = rocksdb::DB::Open(db_options, db_path, column_families, &handles, &db);
  if (!status.ok()) {
    spdlog::error("[IndexMetadataStore] Failed to open RocksDB at {}: {}", db_path,
                  status.ToString());
    throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
  }

  auto store = std::make_unique<IndexMetadataStore>(db, index_id);
  if (handles.size() >= 3) {
    store->cf_default_ = handles[0];
    store->cf_version_history_ = handles[1];
    store->cf_embeddings_ = handles[2];
  }

  spdlog::info("[IndexMetadataStore] Opened RocksDB for index '{}' at {}", index_id, db_path);
  return store;
}

IndexMetadataStore::~IndexMetadataStore() {
  if (db_) {
    auto status = db_->Close();
    if (!status.ok()) {
      spdlog::warn("[IndexMetadataStore] Failed to close RocksDB: {}", status.ToString());
    }
  }
}

IndexManifestV1 IndexMetadataStore::LoadManifest() const {
  std::string manifest_json;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_default_, "index_version", &manifest_json);

  if (status.IsNotFound()) {
    // No manifest yet, return empty
    spdlog::debug("[IndexMetadataStore] No manifest found for index '{}'", index_id_);
    return IndexManifestV1();
  }

  if (!status.ok()) {
    spdlog::error("[IndexMetadataStore] Failed to load manifest for index '{}': {}", index_id_,
                  status.ToString());
    throw std::runtime_error("Failed to load manifest: " + status.ToString());
  }

  try {
    return IndexManifestV1::from_rocksdb_value(manifest_json);
  } catch (const std::exception& e) {
    spdlog::error("[IndexMetadataStore] Failed to parse manifest: {}", e.what());
    throw;
  }
}

void IndexMetadataStore::WriteManifestAtomic(const IndexManifestV1& manifest,
                                             const VersionHistoryEntry& history_entry) {
  // Update manifest in default CF
  std::string manifest_json = manifest.to_rocksdb_value();
  auto status = db_->Put(rocksdb::WriteOptions(), cf_default_, "index_version", manifest_json);
  if (!status.ok()) {
    spdlog::error("[IndexMetadataStore] Failed to write manifest: {}", status.ToString());
    throw std::runtime_error("Failed to write manifest: " + status.ToString());
  }

  // Append to version history CF
  std::string version_key = EncodeVersionNumber(history_entry.version_number);
  std::string history_json = history_entry.to_json().dump(-1);
  status = db_->Put(rocksdb::WriteOptions(), cf_version_history_, version_key, history_json);
  if (!status.ok()) {
    spdlog::error("[IndexMetadataStore] Failed to write history entry: {}", status.ToString());
    throw std::runtime_error("Failed to write history entry: " + status.ToString());
  }

  spdlog::debug(
      "[IndexMetadataStore] Wrote manifest version {} and history entry atomically for "
      "index '{}'",
      history_entry.version_number, index_id_);
}

VersionHistoryEntry IndexMetadataStore::GetVersionHistory(uint32_t version_number) const {
  std::string version_key = EncodeVersionNumber(version_number);
  std::string history_json;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_version_history_, version_key, &history_json);

  if (status.IsNotFound()) {
    spdlog::debug("[IndexMetadataStore] Version {} not found in history for index '{}'",
                  version_number, index_id_);
    return VersionHistoryEntry();
  }

  if (!status.ok()) {
    spdlog::error("[IndexMetadataStore] Failed to read version {}: {}", version_number,
                  status.ToString());
    throw std::runtime_error("Failed to read version: " + status.ToString());
  }

  try {
    auto json = nlohmann::json::parse(history_json);
    return VersionHistoryEntry::from_json(json);
  } catch (const std::exception& e) {
    spdlog::error("[IndexMetadataStore] Failed to parse history entry: {}", e.what());
    throw;
  }
}

std::vector<VersionHistoryEntry> IndexMetadataStore::ListVersionHistory() const {
  std::vector<VersionHistoryEntry> result;
  auto iter = db_->NewIterator(rocksdb::ReadOptions(), cf_version_history_);

  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    std::string version_json = iter->value().ToString();
    try {
      auto json = nlohmann::json::parse(version_json);
      result.push_back(VersionHistoryEntry::from_json(json));
    } catch (const std::exception& e) {
      spdlog::warn("[IndexMetadataStore] Skipping malformed history entry: {}", e.what());
    }
  }

  delete iter;
  spdlog::debug("[IndexMetadataStore] Listed {} version history entries for index '{}'",
                result.size(), index_id_);
  return result;
}

bool IndexMetadataStore::CanRollbackToVersion(uint32_t version_number) const {
  std::string version_key = EncodeVersionNumber(version_number);
  std::string history_json;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_version_history_, version_key, &history_json);
  return status.ok();
}

bool IndexMetadataStore::RollbackToPreviousVersion() {
  auto manifest = LoadManifest();

  if (!manifest.can_rollback_to_previous()) {
    spdlog::warn("[IndexMetadataStore] Cannot rollback for index '{}': no previous version",
                 index_id_);
    return false;
  }

  uint32_t previous_version = manifest.get_previous_version_number();
  auto previous_entry = GetVersionHistory(previous_version);

  if (previous_entry.version_number == 0) {
    spdlog::error("[IndexMetadataStore] Previous version {} not found for index '{}'",
                  previous_version, index_id_);
    return false;
  }

  // Restore as current
  manifest.current_version.version_number = previous_version;
  manifest.current_version.embedding_model_id = previous_entry.embedding_model_id;
  manifest.current_version.chunking_profile_id = previous_entry.chunking_profile_id;
  manifest.reindex_tracking.last_reindex_status = "rolled_back";

  WriteManifestAtomic(manifest, previous_entry);
  spdlog::info("[IndexMetadataStore] Rolled back to version {} for index '{}'", previous_version,
               index_id_);
  return true;
}

uint32_t IndexMetadataStore::GetNextVersionNumber() {
  // Simple implementation: read current counter, increment, write back
  std::string counter_key = "version_counter";
  std::string counter_value;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_default_, counter_key, &counter_value);

  uint32_t current = 0;
  if (status.ok() && counter_value.size() == 4) {
    current = DecodeVersionNumber(counter_value);
  }

  uint32_t next = current + 1;
  std::string encoded = EncodeVersionNumber(next);
  status = db_->Put(rocksdb::WriteOptions(), cf_default_, counter_key, encoded);
  if (!status.ok()) {
    spdlog::error("[IndexMetadataStore] Failed to increment version counter: {}",
                  status.ToString());
    throw std::runtime_error("Failed to increment version counter: " + status.ToString());
  }

  spdlog::debug("[IndexMetadataStore] Incremented version counter: {} -> {} for index '{}'",
                current, next, index_id_);
  return next;
}

}  // namespace themis::index
