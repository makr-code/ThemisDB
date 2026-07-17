/// @file tensor_delta_log.cc
/// @brief Implementation of tensor delta log for recording exact graph mutations
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "src/distributed_tensor/include/tensor_delta_log.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace themis {
namespace distributed_tensor {

namespace {

int64_t getCurrentTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

void applyRetentionLocked(std::vector<DeltaLogEntry>& entries,
                          size_t max_entries_retention,
                          int64_t max_age_ms_retention,
                          int64_t now_ms) {
  if (max_entries_retention > 0 && entries.size() > max_entries_retention) {
    entries.erase(entries.begin(),
                  entries.begin() + static_cast<std::ptrdiff_t>(
                                         entries.size() - max_entries_retention));
  }

  if (max_age_ms_retention <= 0) {
    return;
  }

  const auto cutoff_ms = now_ms - max_age_ms_retention;
  entries.erase(std::remove_if(entries.begin(), entries.end(),
                               [cutoff_ms](const DeltaLogEntry& entry) {
                                 return entry.recorded_at_ms < cutoff_ms;
                               }),
                entries.end());
}

}  // namespace

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

std::string DeltaWindow::serialize() const {
  std::ostringstream oss;
  oss << artifact_id << "\n"
      << sequence_start << "\n"
      << sequence_end << "\n"
      << extracted_at_ms << "\n"
      << total_payload_size_bytes << "\n"
      << entries.size() << "\n";

  for (const auto& entry : entries) {
    oss << entry.serialize() << "\n";
  }

  return oss.str();
}

std::optional<DeltaWindow> DeltaWindow::deserialize(const std::string& data) {
  try {
    DeltaWindow window;
    std::istringstream iss(data);
    std::string line;

    if (!std::getline(iss, window.artifact_id)) {
      return std::nullopt;
    }
    if (!std::getline(iss, line)) {
      return std::nullopt;
    }
    window.sequence_start = std::stoull(line);
    if (!std::getline(iss, line)) {
      return std::nullopt;
    }
    window.sequence_end = std::stoull(line);
    if (!std::getline(iss, line)) {
      return std::nullopt;
    }
    window.extracted_at_ms = std::stoll(line);
    if (!std::getline(iss, line)) {
      return std::nullopt;
    }
    window.total_payload_size_bytes = std::stoull(line);
    if (!std::getline(iss, line)) {
      return std::nullopt;
    }

    const auto entry_count = std::stoull(line);
    for (uint64_t i = 0; i < entry_count; ++i) {
      if (!std::getline(iss, line)) {
        return std::nullopt;
      }
      auto entry = DeltaLogEntry::deserialize(line);
      if (!entry) {
        return std::nullopt;
      }
      window.entries.push_back(*entry);
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
  if (artifact_id_.empty() || affected_entity_id.empty() ||
      source_transaction_id.empty()) {
    return 0;
  }

  std::lock_guard<std::mutex> lock(entries_mutex_);

  // Increment sequence number
  ++current_sequence_;

  // Get current timestamp
  const int64_t now_ms = getCurrentTimeMs();

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
  applyRetentionLocked(entries_, max_entries_retention_, max_age_ms_retention_,
                       now_ms);

  return current_sequence_;
}

std::optional<DeltaWindow> TensorDeltaLog::extractWindow(uint64_t sequence_start, uint64_t sequence_end) {
  if (sequence_start > sequence_end || sequence_start == 0) {
    return std::nullopt;
  }

  std::lock_guard<std::mutex> lock(entries_mutex_);
  if (entries_.empty() || sequence_end > current_sequence_) {
    return std::nullopt;
  }

  DeltaWindow window;
  window.artifact_id = artifact_id_;
  window.sequence_start = sequence_start;
  window.sequence_end = sequence_end;
  window.extracted_at_ms = getCurrentTimeMs();

  // Collect entries in range
  for (const auto& entry : entries_) {
    if (entry.sequence_number >= sequence_start && entry.sequence_number <= sequence_end) {
      window.entries.push_back(entry);
      window.total_payload_size_bytes += entry.payload_size_bytes;
    }
  }

  if (!window.isValid() || window.entries.front().sequence_number != sequence_start
      || window.entries.back().sequence_number != sequence_end) {
    return std::nullopt;
  }

  uint64_t expected = sequence_start;
  for (const auto& entry : window.entries) {
    if (entry.sequence_number != expected) {
      return std::nullopt;
    }
    ++expected;
  }

  if (expected - 1 != sequence_end) {
    return std::nullopt;
  }

  return window;
}

uint64_t TensorDeltaLog::getCurrentSequence() const {
  std::lock_guard<std::mutex> lock(entries_mutex_);
  return current_sequence_;
}

size_t TensorDeltaLog::size() const {
  std::lock_guard<std::mutex> lock(entries_mutex_);
  return entries_.size();
}

bool TensorDeltaLog::empty() const {
  std::lock_guard<std::mutex> lock(entries_mutex_);
  return entries_.empty();
}

void TensorDeltaLog::clear() {
  std::lock_guard<std::mutex> lock(entries_mutex_);
  entries_.clear();
  current_sequence_ = 0;
  last_recorded_ms_ = 0;
}

size_t TensorDeltaLog::getMemoryUsage() const {
  std::lock_guard<std::mutex> lock(entries_mutex_);
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
  std::lock_guard<std::mutex> lock(entries_mutex_);
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
  std::lock_guard<std::mutex> lock(entries_mutex_);
  Stats stats;
  stats.total_deltas = entries_.size();

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
