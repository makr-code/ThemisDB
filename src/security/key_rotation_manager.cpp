/**
 * @file key_rotation_manager.cpp
 * @brief Implementation of KeyRotationManager
 *
 * Handles transparent encryption key rotation for archived chunks.
 *
 * @date 2026-09-24
 */

#include "security/key_rotation_manager.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <spdlog/spdlog.h>
#include <openssl/cmac.h>
#include <openssl/rand.h>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace themis::security {

// ============================================================================
// KeyMetadata Implementation
// ============================================================================

std::string KeyMetadata::DeriveKey(const std::string& master_key) const {
  // Simplified KDF: HMAC-SHA256(master_key, salt) → 32 bytes
  // In production, use HKDF with iterations
  unsigned char derived[32] = {0};
  unsigned int len = 0;

  HMAC_CTX* ctx = HMAC_CTX_new();
  if (!ctx) throw std::runtime_error("Failed to allocate HMAC context");

  HMAC_Init_ex(ctx, reinterpret_cast<const unsigned char*>(master_key.data()),
               master_key.size(), EVP_sha256(), nullptr);
  HMAC_Update(ctx, reinterpret_cast<const unsigned char*>(salt.data()), salt.size());
  HMAC_Final(ctx, derived, &len);
  HMAC_CTX_free(ctx);

  return std::string(reinterpret_cast<const char*>(derived), len);
}

nlohmann::json KeyMetadata::to_json() const {
  return nlohmann::json{
      {"key_tag", key_tag},
      {"salt", salt},
      {"created_at", std::chrono::system_clock::to_time_t(created_at)},
      {"expires_at", std::chrono::system_clock::to_time_t(expires_at)},
      {"active", active},
  };
}

KeyMetadata KeyMetadata::from_json(const nlohmann::json& j) {
  KeyMetadata result;
  if (j.contains("key_tag")) result.key_tag = j["key_tag"];
  if (j.contains("salt")) result.salt = j["salt"];
  if (j.contains("created_at")) {
    auto t = static_cast<time_t>(j["created_at"].get<int64_t>());
    result.created_at = std::chrono::system_clock::from_time_t(t);
  }
  if (j.contains("expires_at")) {
    auto t = static_cast<time_t>(j["expires_at"].get<int64_t>());
    result.expires_at = std::chrono::system_clock::from_time_t(t);
  }
  if (j.contains("active")) result.active = j["active"];
  return result;
}

// ============================================================================
// RotationMapping Implementation
// ============================================================================

nlohmann::json RotationMapping::to_json() const {
  return nlohmann::json{
      {"chunk_id", chunk_id},
      {"old_key_tag", old_key_tag},
      {"new_key_tag", new_key_tag},
      {"migrated_at", std::chrono::system_clock::to_time_t(migrated_at)},
      {"validated", validated},
  };
}

RotationMapping RotationMapping::from_json(const nlohmann::json& j) {
  RotationMapping result;
  if (j.contains("chunk_id")) result.chunk_id = j["chunk_id"];
  if (j.contains("old_key_tag")) result.old_key_tag = j["old_key_tag"];
  if (j.contains("new_key_tag")) result.new_key_tag = j["new_key_tag"];
  if (j.contains("migrated_at")) {
    auto t = static_cast<time_t>(j["migrated_at"].get<int64_t>());
    result.migrated_at = std::chrono::system_clock::from_time_t(t);
  }
  if (j.contains("validated")) result.validated = j["validated"];
  return result;
}

// ============================================================================
// RotationState Implementation
// ============================================================================

nlohmann::json RotationState::to_json() const {
  return nlohmann::json{
      {"phase", static_cast<int>(phase)},
      {"rotation_id", rotation_id},
      {"old_key_tag", old_key_tag},
      {"new_key_tag", new_key_tag},
      {"chunks_migrated", chunks_migrated},
      {"total_chunks", total_chunks},
      {"started_at", std::chrono::system_clock::to_time_t(started_at)},
      {"validation_deadline", std::chrono::system_clock::to_time_t(validation_deadline)},
  };
}

RotationState RotationState::from_json(const nlohmann::json& j) {
  RotationState result;
  if (j.contains("phase")) result.phase = static_cast<RotationPhase>(j["phase"].get<int>());
  if (j.contains("rotation_id")) result.rotation_id = j["rotation_id"];
  if (j.contains("old_key_tag")) result.old_key_tag = j["old_key_tag"];
  if (j.contains("new_key_tag")) result.new_key_tag = j["new_key_tag"];
  if (j.contains("chunks_migrated")) result.chunks_migrated = j["chunks_migrated"];
  if (j.contains("total_chunks")) result.total_chunks = j["total_chunks"];
  if (j.contains("started_at")) {
    auto t = static_cast<time_t>(j["started_at"].get<int64_t>());
    result.started_at = std::chrono::system_clock::from_time_t(t);
  }
  if (j.contains("validation_deadline")) {
    auto t = static_cast<time_t>(j["validation_deadline"].get<int64_t>());
    result.validation_deadline = std::chrono::system_clock::from_time_t(t);
  }
  return result;
}

// ============================================================================
// KeyRotationManager Implementation
// ============================================================================

std::string KeyRotationManager::GenerateKeyTag(const std::string& key_material,
                                                const std::string& salt) {
  // CMAC-AES256: tag = hex(CMAC_AES256(key_material || salt))
  unsigned char tag[16] = {0};
  size_t tag_len = 0;

  CMAC_CTX* ctx = CMAC_CTX_new();
  if (!ctx) throw std::runtime_error("Failed to allocate CMAC context");

  std::string combined = key_material + salt;
  CMAC_Init(ctx, reinterpret_cast<const unsigned char*>(combined.data()),
            combined.size(), EVP_aes_256_cbc(), nullptr);
  CMAC_Final(ctx, tag, &tag_len);
  CMAC_CTX_free(ctx);

  std::ostringstream oss;
  for (size_t i = 0; i < tag_len; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(tag[i]);
  }
  return oss.str();
}

std::string KeyRotationManager::EncodeChunkId(uint64_t chunk_id) {
  std::string result(8, 0);
  result[0] = (chunk_id >> 56) & 0xFF;
  result[1] = (chunk_id >> 48) & 0xFF;
  result[2] = (chunk_id >> 40) & 0xFF;
  result[3] = (chunk_id >> 32) & 0xFF;
  result[4] = (chunk_id >> 24) & 0xFF;
  result[5] = (chunk_id >> 16) & 0xFF;
  result[6] = (chunk_id >> 8) & 0xFF;
  result[7] = chunk_id & 0xFF;
  return result;
}

uint64_t KeyRotationManager::DecodeChunkId(const std::string& encoded) {
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

KeyRotationManager::KeyRotationManager(rocksdb::DB* db) : db_(db) {}

std::unique_ptr<KeyRotationManager> KeyRotationManager::Open(const std::string& db_path) {
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
  column_families.push_back(
      rocksdb::ColumnFamilyDescriptor("key_metadata", rocksdb::ColumnFamilyOptions()));
  column_families.push_back(
      rocksdb::ColumnFamilyDescriptor("chunk_mappings", rocksdb::ColumnFamilyOptions()));
  column_families.push_back(
      rocksdb::ColumnFamilyDescriptor("rotation_history", rocksdb::ColumnFamilyOptions()));

  rocksdb::DB* db = nullptr;
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DBOptions db_options;
  db_options.create_if_missing = true;
  db_options.create_missing_column_families = true;

  auto status = rocksdb::DB::Open(db_options, db_path, column_families, &handles, &db);
  if (!status.ok()) {
    spdlog::error("[KeyRotationManager] Failed to open RocksDB: {}", status.ToString());
    throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
  }

  auto manager = std::make_unique<KeyRotationManager>(db);
  if (handles.size() >= 4) {
    manager->cf_default_ = handles[0];
    manager->cf_key_metadata_ = handles[1];
    manager->cf_chunk_mappings_ = handles[2];
    manager->cf_rotation_history_ = handles[3];
  }

  spdlog::info("[KeyRotationManager] Opened at {}", db_path);
  return manager;
}

KeyRotationManager::~KeyRotationManager() {
  if (db_) {
    auto status = db_->Close();
    if (!status.ok()) {
      spdlog::warn("[KeyRotationManager] Failed to close RocksDB: {}", status.ToString());
    }
  }
}

std::string KeyRotationManager::GetActiveKeyTag() const {
  std::string value;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_default_, "active_key_tag", &value);

  if (status.ok()) return value;
  if (status.IsNotFound()) {
    spdlog::warn("[KeyRotationManager] No active key tag found");
    return "";
  }
  throw std::runtime_error("Failed to get active key tag: " + status.ToString());
}

std::optional<KeyMetadata> KeyRotationManager::GetKeyMetadata(const std::string& key_tag) const {
  std::string key = "key:" + key_tag;
  std::string value;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_key_metadata_, key, &value);

  if (status.IsNotFound()) return std::nullopt;
  if (!status.ok()) {
    throw std::runtime_error("Failed to get key metadata: " + status.ToString());
  }

  try {
    auto json = nlohmann::json::parse(value);
    return KeyMetadata::from_json(json);
  } catch (const std::exception& e) {
    spdlog::error("[KeyRotationManager] Failed to parse key metadata: {}", e.what());
    throw;
  }
}

uint32_t KeyRotationManager::StartKeyRotation(const std::string& master_key) {
  // Generate new key
  unsigned char salt_bytes[16] = {0};
  if (RAND_bytes(salt_bytes, sizeof(salt_bytes)) != 1) {
    throw std::runtime_error("Failed to generate random salt");
  }

  std::string salt(reinterpret_cast<const char*>(salt_bytes), sizeof(salt_bytes));
  std::string new_key_tag = GenerateKeyTag(master_key, salt);

  // Store new key metadata
  KeyMetadata new_key;
  new_key.key_tag = new_key_tag;
  new_key.salt = salt;
  new_key.created_at = std::chrono::system_clock::now();
  new_key.expires_at = new_key.created_at + std::chrono::hours(72);  // 3-day grace
  new_key.active = false;

  std::string key = "key:" + new_key_tag;
  std::string value = new_key.to_json().dump(-1);
  auto status = db_->Put(rocksdb::WriteOptions(), cf_key_metadata_, key, value);
  if (!status.ok()) {
    throw std::runtime_error("Failed to store key metadata: " + status.ToString());
  }

  // Update rotation state
  static uint32_t rotation_counter = 1;
  RotationState state;
  state.phase = RotationPhase::InProgress;
  state.rotation_id = rotation_counter++;
  state.old_key_tag = GetActiveKeyTag();
  state.new_key_tag = new_key_tag;
  state.started_at = std::chrono::system_clock::now();
  state.validation_deadline = state.started_at + std::chrono::hours(24);

  status = db_->Put(rocksdb::WriteOptions(), cf_default_, "key_rotation_state",
                    state.to_json().dump(-1));
  if (!status.ok()) {
    throw std::runtime_error("Failed to update rotation state: " + status.ToString());
  }

  spdlog::info("[KeyRotationManager] Started rotation {} with new key tag: {}", state.rotation_id,
               new_key_tag);
  return state.rotation_id;
}

RotationState KeyRotationManager::GetRotationState() const {
  std::string value;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_default_, "key_rotation_state", &value);

  if (status.IsNotFound()) return RotationState();
  if (!status.ok()) {
    throw std::runtime_error("Failed to get rotation state: " + status.ToString());
  }

  try {
    auto json = nlohmann::json::parse(value);
    return RotationState::from_json(json);
  } catch (const std::exception& e) {
    spdlog::error("[KeyRotationManager] Failed to parse rotation state: {}", e.what());
    throw;
  }
}

bool KeyRotationManager::MigrateChunk(uint64_t chunk_id, const std::string& master_key) {
  auto state = GetRotationState();
  if (state.phase != RotationPhase::InProgress) {
    spdlog::warn("[KeyRotationManager] Cannot migrate chunk: rotation not in progress");
    return false;
  }

  RotationMapping mapping;
  mapping.chunk_id = chunk_id;
  mapping.old_key_tag = state.old_key_tag;
  mapping.new_key_tag = state.new_key_tag;
  mapping.migrated_at = std::chrono::system_clock::now();
  mapping.validated = false;

  std::string key = EncodeChunkId(chunk_id);
  std::string value = mapping.to_json().dump(-1);
  auto status = db_->Put(rocksdb::WriteOptions(), cf_chunk_mappings_, key, value);
  if (!status.ok()) {
    spdlog::error("[KeyRotationManager] Failed to store chunk mapping: {}", status.ToString());
    return false;
  }

  // Update counter
  state.chunks_migrated++;
  status = db_->Put(rocksdb::WriteOptions(), cf_default_, "key_rotation_state",
                    state.to_json().dump(-1));

  spdlog::debug("[KeyRotationManager] Migrated chunk {} ({}/{})", chunk_id, state.chunks_migrated,
                state.total_chunks);
  return true;
}

std::optional<RotationMapping> KeyRotationManager::GetChunkMapping(uint64_t chunk_id) const {
  std::string key = EncodeChunkId(chunk_id);
  std::string value;
  auto status = db_->Get(rocksdb::ReadOptions(), cf_chunk_mappings_, key, &value);

  if (status.IsNotFound()) return std::nullopt;
  if (!status.ok()) {
    throw std::runtime_error("Failed to get chunk mapping: " + status.ToString());
  }

  try {
    auto json = nlohmann::json::parse(value);
    return RotationMapping::from_json(json);
  } catch (const std::exception& e) {
    spdlog::error("[KeyRotationManager] Failed to parse chunk mapping: {}", e.what());
    throw;
  }
}

bool KeyRotationManager::IsInValidationWindow(uint64_t chunk_id) const {
  auto mapping = GetChunkMapping(chunk_id);
  if (!mapping) return false;
  if (mapping->validated) return false;

  auto state = GetRotationState();
  return state.phase == RotationPhase::DualKeyValidation;
}

bool KeyRotationManager::ValidateChunk(uint64_t chunk_id) {
  auto mapping = GetChunkMapping(chunk_id);
  if (!mapping) return false;

  mapping->validated = true;
  std::string key = EncodeChunkId(chunk_id);
  std::string value = mapping->to_json().dump(-1);
  auto status = db_->Put(rocksdb::WriteOptions(), cf_chunk_mappings_, key, value);

  spdlog::debug("[KeyRotationManager] Validated chunk {}", chunk_id);
  return status.ok();
}

bool KeyRotationManager::FinalizeKeyRotation() {
  auto state = GetRotationState();
  if (state.phase == RotationPhase::Complete) return true;

  state.phase = RotationPhase::Complete;
  auto status = db_->Put(rocksdb::WriteOptions(), cf_default_, "key_rotation_state",
                         state.to_json().dump(-1));

  spdlog::info("[KeyRotationManager] Finalized key rotation {}", state.rotation_id);
  return status.ok();
}

bool KeyRotationManager::RollbackKeyRotation() {
  auto state = GetRotationState();
  state.phase = RotationPhase::Idle;
  state.rotation_id = 0;

  auto status = db_->Put(rocksdb::WriteOptions(), cf_default_, "key_rotation_state",
                         state.to_json().dump(-1));

  spdlog::info("[KeyRotationManager] Rolled back key rotation");
  return status.ok();
}

std::vector<nlohmann::json> KeyRotationManager::GetRotationHistory(size_t max_count) const {
  std::vector<nlohmann::json> result;
  auto iter = db_->NewIterator(rocksdb::ReadOptions(), cf_rotation_history_);

  size_t count = 0;
  for (iter->SeekToLast(); iter->Valid() && count < max_count; iter->Prev()) {
    std::string value = iter->value().ToString();
    try {
      result.push_back(nlohmann::json::parse(value));
      count++;
    } catch (const std::exception& e) {
      spdlog::warn("[KeyRotationManager] Skipping malformed history entry: {}", e.what());
    }
  }

  delete iter;
  return result;
}

}  // namespace themis::security
