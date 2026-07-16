/// @file tensor_delta_log.cc
/// @brief Implementation of tensor delta log for recording exact graph mutations
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "src/distributed_tensor/include/tensor_delta_log.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace themis {
namespace distributed_tensor {

// ============================================================================
// DeltaLogEntry Methods
// ============================================================================

bool DeltaLogEntry::isValid() const {
  if (sequence_number == 0) {
    return false;  // Sequence 0 is reserved (invalid)
  }
  if (affected_entity_id.empty()) {
    return false;  // Must have an entity ID
  }
  if (recorded_at_ms <= 0) {
    return false;  // Must have valid timestamp
  }
  return true;
}

std::string DeltaLogEntry::serialize() const {
  std::ostringstream oss;
  oss << sequence_number << "|"
      << static_cast<int>(mutation_type) << "|"
      << affected_entity_id << "|"
      << recorded_at_ms << "|"
      << source_transaction_id << "|"
      << shard_hint << "|"
      << payload_size_bytes << "|"
      << payload_checksum;
  return oss.str();
}

std::optional<DeltaLogEntry> DeltaLogEntry::deserialize(const std::string& serialized) {
  DeltaLogEntry entry;
  std::istringstream iss(serialized);
  std::string field;

  try {
    // Parse sequence_number
    std::getline(iss, field, '|');
    entry.sequence_number = std::stoull(field);

    // Parse mutation_type
    std::getline(iss, field, '|');
    entry.mutation_type = static_cast<DeltaMutationType>(std::stoi(field));

    // Parse affected_entity_id
    std::getline(iss, field, '|');
    entry.affected_entity_id = field;

    // Parse recorded_at_ms
    std::getline(iss, field, '|');
    entry.recorded_at_ms = std::stoll(field);

    // Parse source_transaction_id
    std::getline(iss, field, '|');
    entry.source_transaction_id = field;

    // Parse shard_hint
    std::getline(iss, field, '|');
    entry.shard_hint = field;

    // Parse payload_size_bytes
    std::getline(iss, field, '|');
    entry.payload_size_bytes = std::stoul(field);

    // Parse payload_checksum
    std::getline(iss, field, '|');
    entry.payload_checksum = field;

    if (!entry.isValid()) {
      return std::nullopt;
    }
    return entry;
  } catch (...) {
    return std::nullopt;
  }
}

// ============================================================================
// DeltaWindow Methods
// ============================================================================

size_t DeltaWindow::countInserts() const {
  return std::count_if(entries.begin(), entries.end(),
                       [](const DeltaLogEntry& e) { return e.mutation_type == DeltaMutationType::INSERT; });
}

size_t DeltaWindow::countUpdates() const {
  return std::count_if(entries.begin(), entries.end(),
                       [](const DeltaLogEntry& e) { return e.mutation_type == DeltaMutationType::UPDATE; });
}

size_t DeltaWindow::countDeletes() const {
  return std::count_if(entries.begin(), entries.end(),
                       [](const DeltaLogEntry& e) { return e.mutation_type == DeltaMutationType::DELETE; });
}

size_t DeltaWindow::countShardChanges() const {
  return std::count_if(entries.begin(), entries.end(),
                       [](const DeltaLogEntry& e) { return e.mutation_type == DeltaMutationType::SHARD_CHANGE; });
}

double DeltaWindow::estimateChangeFraction(uint64_t artifact_size_bytes) const {
  if (artifact_size_bytes == 0) {
    return 0.0;  // No artifact, no change
  }
  double fraction = static_cast<double>(total_payload_size_bytes) / static_cast<double>(artifact_size_bytes);
  // Clamp to [0.0, 1.0]
  return std::min(1.0, std::max(0.0, fraction));
}

bool DeltaWindow::isValid() const {
  if (entries.empty()) {
    return false;  // Empty window is invalid
  }
  if (sequence_start > sequence_end) {
    return false;  // Invalid range
  }
  // Verify all entries are in range and ordered
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto& entry = entries[i];
    if (entry.sequence_number < sequence_start || entry.sequence_number > sequence_end) {
      return false;
    }
    if (i > 0 && entries[i - 1].sequence_number >= entry.sequence_number) {
      return false;  // Not ordered
    }
  }
  return true;
}

std::string DeltaWindow::toJSON() const {
  json j;
  j["artifact_id"] = artifact_id;
  j["sequence_start"] = sequence_start;
  j["sequence_end"] = sequence_end;
  j["extracted_at_ms"] = extracted_at_ms;
  j["total_payload_size_bytes"] = total_payload_size_bytes;
  j["entry_count"] = entries.size();

  json entries_array = json::array();
  for (const auto& entry : entries) {
    json entry_obj;
    entry_obj["sequence_number"] = entry.sequence_number;
    entry_obj["mutation_type"] = static_cast<int>(entry.mutation_type);
    entry_obj["affected_entity_id"] = entry.affected_entity_id;
    entry_obj["recorded_at_ms"] = entry.recorded_at_ms;
    entry_obj["source_transaction_id"] = entry.source_transaction_id;
    entry_obj["shard_hint"] = entry.shard_hint;
    entry_obj["payload_size_bytes"] = entry.payload_size_bytes;
    entry_obj["payload_checksum"] = entry.payload_checksum;
    entries_array.push_back(entry_obj);
  }
  j["entries"] = entries_array;

  return j.dump(2);
}

std::optional<DeltaWindow> DeltaWindow::fromJSON(const std::string& json_str) {
  try {
    json j = json::parse(json_str);
    DeltaWindow window;

    window.artifact_id = j["artifact_id"].get<std::string>();
    window.sequence_start = j["sequence_start"].get<uint64_t>();
    window.sequence_end = j["sequence_end"].get<uint64_t>();
    window.extracted_at_ms = j["extracted_at_ms"].get<int64_t>();
    window.total_payload_size_bytes = j["total_payload_size_bytes"].get<uint64_t>();

    for (const auto& entry_obj : j["entries"]) {
      DeltaLogEntry entry;
      entry.sequence_number = entry_obj["sequence_number"].get<uint64_t>();
      entry.mutation_type = static_cast<DeltaMutationType>(entry_obj["mutation_type"].get<int>());
      entry.affected_entity_id = entry_obj["affected_entity_id"].get<std::string>();
      entry.recorded_at_ms = entry_obj["recorded_at_ms"].get<int64_t>();
      entry.source_transaction_id = entry_obj["source_transaction_id"].get<std::string>();
      entry.shard_hint = entry_obj["shard_hint"].get<std::string>();
      entry.payload_size_bytes = entry_obj["payload_size_bytes"].get<uint32_t>();
      entry.payload_checksum = entry_obj["payload_checksum"].get<std::string>();
      window.entries.push_back(entry);
    }

    if (!window.isValid()) {
      return std::nullopt;
    }
    return window;
  } catch (...) {
    return std::nullopt;
  }
}

// ============================================================================
// TensorDeltaLog Methods
// ============================================================================

TensorDeltaLog::TensorDeltaLog(const std::string& artifact_id)
    : artifact_id_(artifact_id),
      current_sequence_(0),
      last_recorded_ms_(0),
      max_entries_retention_(100000),
      max_age_ms_retention_(86400000) {}

uint64_t TensorDeltaLog::appendDelta(DeltaMutationType mutation_type,
                                     const std::string& affected_entity_id,
                                     const std::string& source_transaction_id,
                                     const std::string& shard_hint,
                                     uint32_t payload_size_bytes) {
  // Increment sequence number
  ++current_sequence_;

  // Get current timestamp
  int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

  // Create and validate entry
  DeltaLogEntry entry;
  entry.sequence_number = current_sequence_;
  entry.mutation_type = mutation_type;
  entry.affected_entity_id = affected_entity_id;
  entry.recorded_at_ms = now_ms;
  entry.source_transaction_id = source_transaction_id;
  entry.shard_hint = shard_hint;
  entry.payload_size_bytes = payload_size_bytes;

  if (!entry.isValid()) {
    // Revert sequence number on error
    --current_sequence_;
    return 0;
  }

  // Append to log
  entries_.push_back(entry);
  last_recorded_ms_ = now_ms;

  return current_sequence_;
}

std::optional<DeltaWindow> TensorDeltaLog::extractWindow(uint64_t sequence_start, uint64_t sequence_end) {
  if (sequence_start > sequence_end || sequence_start == 0) {
    return std::nullopt;
  }

  DeltaWindow window;
  window.artifact_id = artifact_id_;
  window.sequence_start = sequence_start;
  window.sequence_end = sequence_end;
  window.extracted_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

  // Collect entries in range
  for (const auto& entry : entries_) {
    if (entry.sequence_number >= sequence_start && entry.sequence_number <= sequence_end) {
      window.entries.push_back(entry);
      window.total_payload_size_bytes += entry.payload_size_bytes;
    }
  }

  if (!window.isValid()) {
    return std::nullopt;
  }

  return window;
}

uint64_t TensorDeltaLog::getCurrentSequence() const { return current_sequence_; }

size_t TensorDeltaLog::size() const { return entries_.size(); }

bool TensorDeltaLog::empty() const { return entries_.empty(); }

void TensorDeltaLog::clear() { entries_.clear(); }

size_t TensorDeltaLog::getMemoryUsage() const {
  size_t memory = artifact_id_.capacity() + sizeof(TensorDeltaLog);
  for (const auto& entry : entries_) {
    memory += sizeof(DeltaLogEntry) + entry.affected_entity_id.capacity() +
              entry.source_transaction_id.capacity() + entry.shard_hint.capacity() +
              entry.payload_checksum.capacity();
  }
  return memory;
}

bool TensorDeltaLog::persistToStorage() const {
  // Placeholder for RocksDB persistence
  // This is an integration point for durability
  return true;
}

int64_t TensorDeltaLog::loadFromStorage() {
  // Placeholder for RocksDB recovery
  return 0;
}

size_t TensorDeltaLog::garbage_collect(uint64_t cutoff_sequence) {
  size_t removed = 0;
  auto it = entries_.begin();
  while (it != entries_.end()) {
    if (it->sequence_number < cutoff_sequence) {
      it = entries_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }
  return removed;
}

void TensorDeltaLog::setRetentionPolicy(size_t max_entries, int64_t max_age_ms) {
  max_entries_retention_ = max_entries;
  max_age_ms_retention_ = max_age_ms;
}

TensorDeltaLog::Stats TensorDeltaLog::getStats() const {
  Stats stats;
  stats.total_deltas = entries_.size();

  int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

  for (const auto& entry : entries_) {
    stats.total_payload_bytes += entry.payload_size_bytes;

    switch (entry.mutation_type) {
      case DeltaMutationType::INSERT:
        stats.total_insert_mutations++;
        break;
      case DeltaMutationType::UPDATE:
        stats.total_update_mutations++;
        break;
      case DeltaMutationType::DELETE:
        stats.total_delete_mutations++;
        break;
      case DeltaMutationType::SHARD_CHANGE:
        stats.total_shard_change_mutations++;
        break;
      case DeltaMutationType::METADATA_UPDATE:
        // Count as update
        stats.total_update_mutations++;
        break;
    }

    if (stats.oldest_delta_ms == 0 || entry.recorded_at_ms < stats.oldest_delta_ms) {
      stats.oldest_delta_ms = entry.recorded_at_ms;
    }
    if (entry.recorded_at_ms > stats.newest_delta_ms) {
      stats.newest_delta_ms = entry.recorded_at_ms;
    }
  }

  return stats;
}

}  // namespace distributed_tensor
}  // namespace themis
