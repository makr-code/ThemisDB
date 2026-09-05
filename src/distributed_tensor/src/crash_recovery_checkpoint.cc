/// @file crash_recovery_checkpoint.cc
/// @brief Implementation of crash recovery checkpoint manager
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "../include/crash_recovery_checkpoint.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace themis {
namespace distributed_tensor {

namespace fs = std::filesystem;

CrashRecoveryCheckpoint::CrashRecoveryCheckpoint(const std::string& checkpoint_dir)
    : checkpoint_dir_(checkpoint_dir) {
  // Create checkpoint directory if it doesn't exist
  if (!checkpoint_dir_.empty()) {
    fs::create_directories(checkpoint_dir_);
  }
}

CheckpointStatus CrashRecoveryCheckpoint::save(const std::string& artifact_id,
                                               const Checkpoint& checkpoint) {
  if (artifact_id.empty()) {
    return CheckpointStatus::UNKNOWN_ERROR;
  }

  if (checkpoint_dir_.empty()) {
    return CheckpointStatus::IO_ERROR;
  }

  // Validate checkpoint before saving
  CheckpointStatus validation = validateCheckpoint(checkpoint);
  if (validation != CheckpointStatus::OK) {
    return validation;
  }

  try {
    // Create checkpoint file path
    std::string file_path = getCheckpointPath(artifact_id);

    // Serialize checkpoint to string
    std::string serialized = serializeCheckpoint(checkpoint);

    // Write to temporary file first for atomicity
    std::string temp_path = file_path + ".tmp";

    std::ofstream out_file(temp_path, std::ios::binary);
    if (!out_file.is_open()) {
      return CheckpointStatus::IO_ERROR;
    }

    out_file.write(serialized.c_str(), serialized.size());
    out_file.close();

    if (!out_file) {
      return CheckpointStatus::IO_ERROR;
    }

    // Atomically move temp file to final location
    fs::rename(temp_path, file_path);

    return CheckpointStatus::OK;
  } catch (const std::filesystem::filesystem_error&) {
    return CheckpointStatus::IO_ERROR;
  } catch (...) {
    return CheckpointStatus::UNKNOWN_ERROR;
  }
}

CheckpointStatus CrashRecoveryCheckpoint::load(const std::string& artifact_id,
                                               Checkpoint& checkpoint) {
  if (artifact_id.empty()) {
    return CheckpointStatus::UNKNOWN_ERROR;
  }

  if (checkpoint_dir_.empty()) {
    return CheckpointStatus::IO_ERROR;
  }

  try {
    std::string file_path = getCheckpointPath(artifact_id);

    // Check if file exists
    if (!fs::exists(file_path)) {
      return CheckpointStatus::NOT_FOUND;
    }

    // Read file
    std::ifstream in_file(file_path, std::ios::binary);
    if (!in_file.is_open()) {
      return CheckpointStatus::IO_ERROR;
    }

    std::stringstream buffer = {};
    buffer << in_file.rdbuf();
    in_file.close();

    if (!in_file) {
      return CheckpointStatus::IO_ERROR;
    }

    // Deserialize checkpoint
    std::string serialized = buffer.str();
    checkpoint = deserializeCheckpoint(serialized);

    // Validate loaded checkpoint
    return validateCheckpoint(checkpoint);
  } catch (const std::filesystem::filesystem_error&) {
    return CheckpointStatus::IO_ERROR;
  } catch (...) {
    return CheckpointStatus::CORRUPTED;
  }
}

CheckpointStatus CrashRecoveryCheckpoint::deleteCheckpoint(const std::string& artifact_id) {
  if (artifact_id.empty()) {
    return CheckpointStatus::UNKNOWN_ERROR;
  }

  try {
    std::string file_path = getCheckpointPath(artifact_id);

    if (!fs::exists(file_path)) {
      return CheckpointStatus::NOT_FOUND;
    }

    fs::remove(file_path);
    return CheckpointStatus::OK;
  } catch (const std::filesystem::filesystem_error&) {
    return CheckpointStatus::IO_ERROR;
  } catch (...) {
    return CheckpointStatus::UNKNOWN_ERROR;
  }
}

bool CrashRecoveryCheckpoint::exists(const std::string& artifact_id) {
  if (artifact_id.empty() || checkpoint_dir_.empty()) {
    return false;
  }

  try {
    std::string file_path = getCheckpointPath(artifact_id);
    return fs::exists(file_path);
  } catch (...) {
    return false;
  }
}

CrashRecoveryCheckpoint::CheckpointStats CrashRecoveryCheckpoint::getStats() {
  CheckpointStats stats = {};

  if (checkpoint_dir_.empty()) {
    return stats;
  }

  try {
    auto now = std::chrono::system_clock::now();
    int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    stats.oldest_checkpoint_unix_sec = now_sec;
    stats.newest_checkpoint_unix_sec = 0;

    for (const auto& entry : fs::directory_iterator(checkpoint_dir_)) {
      if (entry.is_regular_file() && entry.path().extension() == ".chk") {
        stats.checkpoint_count++;

        // Get file size
        uint64_t file_size = fs::file_size(entry);
        stats.total_size_bytes += file_size;

        // Get modification time
        auto last_write_time = fs::last_write_time(entry);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            last_write_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        int64_t file_time_sec = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();

        stats.oldest_checkpoint_unix_sec = std::min(stats.oldest_checkpoint_unix_sec, file_time_sec);
        stats.newest_checkpoint_unix_sec = std::max(stats.newest_checkpoint_unix_sec, file_time_sec);
      }
    }
  } catch (...) {
    // Return partial stats on error
  }

  return stats;
}

uint64_t CrashRecoveryCheckpoint::cleanupOldCheckpoints(uint32_t retention_days) {
  uint64_t deleted_count = 0;

  if (checkpoint_dir_.empty()) {
    return deleted_count;
  }

  try {
    auto now = std::chrono::system_clock::now();
    auto cutoff_time = now - std::chrono::hours(24 * retention_days);

    for (const auto& entry : fs::directory_iterator(checkpoint_dir_)) {
      if (entry.is_regular_file() && entry.path().extension() == ".chk") {
        auto last_write_time = fs::last_write_time(entry);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            last_write_time - fs::file_time_type::clock::now() + now);

        if (sctp < cutoff_time) {
          fs::remove(entry);
          deleted_count++;
        }
      }
    }
  } catch (...) {
    // Continue cleanup on error
  }

  return deleted_count;
}

void CrashRecoveryCheckpoint::setCheckpointDir(const std::string& checkpoint_dir) {
  checkpoint_dir_ = checkpoint_dir;
  if (!checkpoint_dir_.empty()) {
    fs::create_directories(checkpoint_dir_);
  }
}

std::string CrashRecoveryCheckpoint::getCheckpointDir() const {
  return checkpoint_dir_;
}

std::string CrashRecoveryCheckpoint::serializeCheckpoint(const Checkpoint& checkpoint) {
  // Simple pipe-delimited format for checkpoint serialization
  std::ostringstream oss = {};
  oss << checkpoint.version << "|"
      << checkpoint.created_at_unix_sec << "|"
      << checkpoint.artifact_id << "|"
      << checkpoint.delta_window.sequence_start << "|"
      << checkpoint.delta_window.sequence_end << "|"
      << checkpoint.artifact_size_bytes << "|"
      << checkpoint.last_decision << "|"
      << checkpoint.progress_percent << "|"
      << checkpoint.retry_count << "|"
      << checkpoint.max_retries << "|"
      << checkpoint.current_manifest.residual << "|"
      << checkpoint.current_manifest.rank_status << "|"
      << checkpoint.current_manifest.rank_cap << "|"
      << checkpoint.last_error_message;

  return oss.str();
}

Checkpoint CrashRecoveryCheckpoint::deserializeCheckpoint(const std::string& data) {
  Checkpoint checkpoint;
  std::istringstream iss(data);
  std::string token = {};
  std::vector<std::string> tokens;

  // Split by pipe delimiter
  while (std::getline(iss, token, '|')) {
    tokens.push_back(token);
  }

  if (tokens.size() < 14) {
    return checkpoint;  // Return empty checkpoint on parse error
  }

  try {
    checkpoint.version = std::stoul(tokens[0]);
    checkpoint.created_at_unix_sec = std::stoll(tokens[1]);
    checkpoint.artifact_id = tokens[2];
    checkpoint.delta_window.sequence_start = std::stoul(tokens[3]);
    checkpoint.delta_window.sequence_end = std::stoul(tokens[4]);
    checkpoint.artifact_size_bytes = std::stoull(tokens[5]);
    checkpoint.last_decision = std::stoul(tokens[6]);
    checkpoint.progress_percent = std::stoul(tokens[7]);
    checkpoint.retry_count = std::stoul(tokens[8]);
    checkpoint.max_retries = std::stoul(tokens[9]);
    checkpoint.current_manifest.residual = std::stod(tokens[10]);
    checkpoint.current_manifest.rank_status = std::stoul(tokens[11]);
    checkpoint.current_manifest.rank_cap = std::stoul(tokens[12]);
    checkpoint.last_error_message = tokens[13];
  } catch (...) {
    return Checkpoint();  // Return empty checkpoint on parse error
  }

  return checkpoint;
}

std::string CrashRecoveryCheckpoint::getCheckpointPath(const std::string& artifact_id) {
  // Sanitize artifact_id for use as filename
  std::string safe_id = artifact_id;
  std::replace_if(safe_id.begin(), safe_id.end(), [](char c) {
    return !std::isalnum(c) && c != '_' && c != '-';
  }, '_');

  return checkpoint_dir_ + "/" + safe_id + ".chk";
}

CheckpointStatus CrashRecoveryCheckpoint::validateCheckpoint(const Checkpoint& checkpoint) {
  // Check version compatibility
  if (checkpoint.version > 1) {
    return CheckpointStatus::VERSION_MISMATCH;
  }

  // Check required fields
  if (checkpoint.artifact_id.empty()) {
    return CheckpointStatus::CORRUPTED;
  }

  if (checkpoint.retry_count > checkpoint.max_retries) {
    // This is technically valid but indicates exhausted retries
    return CheckpointStatus::OK;
  }

  return CheckpointStatus::OK;
}

}  // namespace distributed_tensor
}  // namespace themis
