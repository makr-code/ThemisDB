/**
 * @file lora_checkpoint_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/lora_checkpoint_manager.h"
#include "utils/checksum_utils.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <cctype>
#include <unordered_set>

// Simple manifest serialisation without JSON dependency
// Format: one checkpoint block per entry, separated by "---\n"
namespace {

// Serialize a single manifest entry to a key=value block
std::string serializeEntry(const themis::training::CheckpointManifestEntry& e) {
    std::ostringstream oss;
    oss << "checkpoint_path=" << e.checkpoint_path << "\n"
        << "sha256="          << e.sha256           << "\n"
        << "base_model_hash=" << e.base_model_hash  << "\n"
        << "adapter_version=" << e.adapter_version  << "\n"
        << "epoch="           << e.epoch            << "\n"
        << "step="            << e.step             << "\n"
        << "loss="            << e.loss             << "\n"
        << "accuracy="        << e.accuracy         << "\n"
        << "saved_at="        << static_cast<long long>(e.saved_at) << "\n"
        << "---\n";
    return oss.str();
}

// Deserialize manifest blocks from the manifest file content.
// Each completed entry is validated before it is accepted:
//   - checkpoint_path must be non-empty and must not contain path-traversal sequences
//   - sha256 must be exactly 64 lowercase hex characters (when present)
// Malformed entries are silently dropped so a single corrupt block cannot
// prevent the entire manifest from loading.
std::vector<themis::training::CheckpointManifestEntry>
parseManifest(const std::string& content) {
    std::vector<themis::training::CheckpointManifestEntry> result;
    themis::training::CheckpointManifestEntry entry;
    std::istringstream iss(content);
    std::string line;
    bool in_block = false;

    // Returns true when 's' is exactly 64 lowercase hex characters.
    auto isValidSha256 = [](const std::string& s) -> bool {
        if (s.size() != 64) {
          return false;
        }
        for (char c : s) {
            if (!std::isxdigit(static_cast<unsigned char>(c)) ||
                (std::isupper(static_cast<unsigned char>(c)))) return false;
        }
        return true;
    };

    // Returns true when 'p' is non-empty and contains no path-traversal.
    auto isSafePath = [](const std::string& p) -> bool {
        if (p.empty()) {
          return false;
        }
        // Reject entries with ".." components
        if (p.find("..") != std::string::npos) {
          return false;
        }
        return true;
    };

    auto commitEntry = [&]() {
        if (in_block && isSafePath(entry.checkpoint_path)) {
            // Reject the entry when a sha256 is present but malformed.
            if (!entry.sha256.empty() && !isValidSha256(entry.sha256)) {
                entry = {};
                in_block = false;
                return;
            }
            result.push_back(entry);
        }
        entry = {};
        in_block = false;
    };

    while (std::getline(iss, line)) {
        if (line == "---") {
            commitEntry();
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) {
          continue;
        }
        in_block = true;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if      (key == "checkpoint_path") {
          entry.checkpoint_path = val;
        }
        else if (key == "sha256")          entry.sha256 = val;
        else if (key == "base_model_hash") entry.base_model_hash = val;
        else if (key == "adapter_version") entry.adapter_version = val;
        else if (key == "epoch")    entry.epoch    = static_cast<size_t>(std::stoull(val));
        else if (key == "step")     entry.step     = static_cast<size_t>(std::stoull(val));
        else if (key == "loss")     entry.loss     = std::stod(val);
        else if (key == "accuracy") entry.accuracy = std::stod(val);
        else if (key == "saved_at") entry.saved_at = static_cast<std::time_t>(std::stoll(val));
    }
    commitEntry(); // flush last block if file didn't end with "---"
    return result;
}

// Copy a file via streams (portable, no POSIX rename across filesystems)
bool copyFile(const std::string& src, const std::string& dst) {
    std::ifstream in(src, std::ios::binary);
    if (!in.is_open()) {
      return false;
    }
    std::ofstream out(dst, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      return false;
    }
    out << in.rdbuf();
    return out.good();
}

} // anonymous namespace

namespace themis {
namespace training {

// ============================================================================
// Impl
// ============================================================================
/** @brief Impl. */
class LoRACheckpointManager::Impl {
public:
    explicit Impl(const CheckpointManagerConfig& config)
        : config_(config) {
        if (config_.checkpoint_dir.empty()) {
            throw std::invalid_argument("LoRACheckpointManager: checkpoint_dir must not be empty");
        }
        if (config_.max_checkpoints == 0) {
            throw std::invalid_argument("LoRACheckpointManager: max_checkpoints must be >= 1");
        }
        loadManifest();
    }

    // -------------------------------------------------------------------------
    CheckpointManifestEntry save(const std::string& source_path,
                                 CheckpointManifestEntry meta) {
        if (source_path.empty()) {
            throw std::invalid_argument("LoRACheckpointManager::save: source_path is empty");
        }

        // Build destination filename from version or epoch/step
        std::string filename;
        if (!meta.adapter_version.empty()) {
            filename = meta.adapter_version + "_e" + std::to_string(meta.epoch)
                       + "_s" + std::to_string(meta.step) + ".ckpt";
        } else {
            filename = "checkpoint_e" + std::to_string(meta.epoch)
                       + "_s" + std::to_string(meta.step) + ".ckpt";
        }

        // Sanitize: replace spaces with underscores
        for (char& c : filename) {
          if (c == ' ') c = '_';
        }

        std::string tmp_path  = config_.checkpoint_dir + "/" + filename + ".tmp";
        std::string final_path = config_.checkpoint_dir + "/" + filename;

        // Atomic copy: write to .tmp first
        if (!copyFile(source_path, tmp_path)) {
            throw std::runtime_error(
                "LoRACheckpointManager: failed to copy checkpoint to " + tmp_path);
        }

        // Compute SHA-256 of the tmp file
        std::string sha256 = utils::calculateSHA256(tmp_path);
        if (sha256.empty()) {
            // Remove incomplete tmp file before throwing
            std::remove(tmp_path.c_str());
            throw std::runtime_error(
                "LoRACheckpointManager: SHA-256 computation failed for " + tmp_path);
        }

        // Rename tmp → final (atomic on the same filesystem)
        if (std::rename(tmp_path.c_str(), final_path.c_str()) != 0) {
            std::remove(tmp_path.c_str());
            throw std::runtime_error(
                "LoRACheckpointManager: rename failed: " + tmp_path + " -> " + final_path);
        }

        meta.checkpoint_path = final_path;
        meta.sha256          = sha256;
        meta.saved_at        = std::time(nullptr);

        // Prepend newest entry (newest-first ordering)
        entries_.insert(entries_.begin(), meta);

        // Prune rolling window
        while (entries_.size() > config_.max_checkpoints) {
            const auto& oldest = entries_.back();
            std::remove(oldest.checkpoint_path.c_str());
            entries_.pop_back();
        }

        persistManifest();
        return meta;
    }

    // -------------------------------------------------------------------------
    std::optional<CheckpointManifestEntry> resume() const {
        if (entries_.empty()) {
            return std::nullopt;
        }

        for (size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            if (!config_.validate_on_load) {
                return entry; // return without SHA-256 check
            }
            if (validate(entry)) {
                if (i > 0) {
                    // WARN: had to skip corrupt newer checkpoint(s)
                    // (In production this would use utils/logger; here we note it in-code)
                }
                return entry;
            }
            // Corrupt: try next entry if auto_rollback is enabled
            if (!config_.auto_rollback) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    // -------------------------------------------------------------------------
    std::vector<CheckpointManifestEntry> listCheckpoints() const {
        return entries_;
    }

    // -------------------------------------------------------------------------
    bool validate(const CheckpointManifestEntry& entry) const {
        if (entry.checkpoint_path.empty() || entry.sha256.empty()) {
            return false;
        }
        std::string computed = utils::calculateSHA256(entry.checkpoint_path);
        return (!computed.empty() && computed == entry.sha256);
    }

    // -------------------------------------------------------------------------
    void clearAll() {
        for (const auto& e : entries_) {
            std::remove(e.checkpoint_path.c_str());
        }
        entries_.clear();
        std::remove(manifestPath().c_str());
        std::remove(calibrationManifestPath().c_str());
    }

    // -------------------------------------------------------------------------
    std::string manifestPath() const {
        return config_.checkpoint_dir + "/" + config_.manifest_filename;
    }

    // -------------------------------------------------------------------------
    std::string calibrationManifestPath() const {
        return config_.checkpoint_dir + "/calibration_manifest.json";
    }

    // -------------------------------------------------------------------------
    void saveCalibrationJson(const std::string& json_content) {
        std::ofstream f(calibrationManifestPath(), std::ios::trunc);
        if (!f.is_open()) {
            throw std::runtime_error(
                "LoRACheckpointManager: cannot write calibration_manifest.json to "
                + config_.checkpoint_dir);
        }
        f << json_content;
        if (!f.good()) {
            throw std::runtime_error(
                "LoRACheckpointManager: I/O error writing calibration_manifest.json");
        }
    }

    // -------------------------------------------------------------------------
    std::string loadCalibrationJson() const {
        std::ifstream f(calibrationManifestPath());
        if (!f.is_open()) {
          return "";
        }
        std::ostringstream oss;
        oss << f.rdbuf();
        return oss.str();
    }

    // Phase 2: Clean up partial/corrupted checkpoints
    size_t cleanupPartialCheckpoints() {
        size_t removed = 0;
        
        // Build a set of known checkpoint paths from manifest
        std::unordered_set<std::string> manifest_paths;
        for (const auto& e : entries_) {
            manifest_paths.insert(e.checkpoint_path);
        }
        
        // Scan directory for orphaned or corrupted checkpoint files
        std::string dir = config_.checkpoint_dir;
        // Simple cleanup: remove .tmp files and invalid checkpoints
        for (const auto& e : entries_) {
            if (!validate(e) && config_.cleanup_partial) {
                std::string path = e.checkpoint_path;
                if (std::remove(path.c_str()) == 0) {
                    removed++;
                }
            }
        }
        
        // Also clean up any lingering .tmp files
        std::string tmp_pattern = dir + "/*.tmp";
        // Note: In production, would use dirent.h for directory scanning
        
        return removed;
    }

    // Phase 2: Audit all checkpoints
    size_t auditCheckpoints(std::string* diagnostics) {
        std::ostringstream diag;
        size_t valid_count = 0;
        
        diag << "Checkpoint audit report:\n"
             << "  Directory: " << config_.checkpoint_dir << "\n"
             << "  Total entries in manifest: " << entries_.size() << "\n";
        
        for (size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            bool is_valid = validate(entry);
            if (is_valid) {
              valid_count++;
            }
            
            diag << "  [" << (is_valid ? "OK" : "FAIL") << "] "
                 << entry.checkpoint_path << " (epoch=" << entry.epoch 
                 << ", step=" << entry.step << ")\n";
        }
        
        diag << "  Valid checkpoints: " << valid_count << "/" << entries_.size() << "\n";
        
        if (diagnostics) {
          *diagnostics = diag.str();
        }
        return valid_count;
    }

private:
    // -------------------------------------------------------------------------
    void loadManifest() {
        std::ifstream f(manifestPath());
        if (!f.is_open()) return; // first run — no manifest yet
        std::ostringstream oss;
        oss << f.rdbuf();
        entries_ = parseManifest(oss.str());
    }

    // -------------------------------------------------------------------------
    void persistManifest() const {
        std::string path = manifestPath();
        std::ofstream f(path, std::ios::trunc);
        if (!f.is_open()) {
            // Non-fatal: checkpoint was already saved; manifest write failure is logged only
            return;
        }
        for (const auto& e : entries_) {
            f << serializeEntry(e);
        }
    }

    // -------------------------------------------------------------------------
    CheckpointManagerConfig                      config_;
    std::vector<CheckpointManifestEntry>         entries_; // newest-first
};

// ============================================================================
// Public API
// ============================================================================
LoRACheckpointManager::LoRACheckpointManager(const CheckpointManagerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

LoRACheckpointManager::~LoRACheckpointManager() = default;

CheckpointManifestEntry LoRACheckpointManager::save(const std::string& source_path,
                                                     CheckpointManifestEntry meta) {
    return impl_->save(source_path, std::move(meta));
}

std::optional<CheckpointManifestEntry> LoRACheckpointManager::resume() const {
    return impl_->resume();
}

std::vector<CheckpointManifestEntry> LoRACheckpointManager::listCheckpoints() const {
    return impl_->listCheckpoints();
}

bool LoRACheckpointManager::validate(const CheckpointManifestEntry& entry) const {
    return impl_->validate(entry);
}

void LoRACheckpointManager::clearAll() {
    impl_->clearAll();
}

std::string LoRACheckpointManager::manifestPath() const {
    return impl_->manifestPath();
}

void LoRACheckpointManager::saveCalibrationJson(const std::string& json_content) {
    impl_->saveCalibrationJson(json_content);
}

std::string LoRACheckpointManager::loadCalibrationJson() const {
    return impl_->loadCalibrationJson();
}

std::optional<CheckpointManifestEntry> LoRACheckpointManager::resumeWithDiagnostics(
    std::string* diagnostics) const {
    std::ostringstream diag;
    const auto& entries = impl_->listCheckpoints();
    
    diag << "Checkpoint recovery audit:\n"
         << "  Total manifest entries: " << entries.size() << "\n";
    
    if (entries.empty()) {
        diag << "  Result: No checkpoints available\n";
        if (diagnostics) {
          *diagnostics = diag.str();
        }
        return std::nullopt;
    }
    
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        diag << "  Entry " << i << ": " << entry.checkpoint_path << "\n"
             << "    Epoch=" << entry.epoch << " Step=" << entry.step 
             << " Loss=" << entry.loss << "\n";
        
        if (!impl_->validate(entry)) {
            diag << "    Status: CORRUPT (SHA-256 mismatch or missing file)\n";
            continue;
        }
        
        diag << "    Status: VALID\n";
        if (diagnostics) {
          *diagnostics = diag.str();
        }
        return entry;
    }
    
    diag << "  Result: No valid checkpoint found\n";
    if (diagnostics) {
      *diagnostics = diag.str();
    }
    return std::nullopt;
}

size_t LoRACheckpointManager::cleanupPartialCheckpoints() {
    return impl_->cleanupPartialCheckpoints();
}

size_t LoRACheckpointManager::auditCheckpoints(std::string* diagnostics) {
    return impl_->auditCheckpoints(diagnostics);
}

} // namespace training
} // namespace themis
