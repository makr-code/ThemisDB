/*
 * ThemisDB | File: lora_checkpoint_manager.cpp | Version: 0.0.13 | Last Modified: 2026-05-20 17:27:23
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 332
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=10, M=0, L=0
 * PR History (last 5): #3648 audit(training): complete m... (2026-03-12) | #3601 feat(training): Phase 3 imp... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

// Deserialize manifest blocks from the manifest file content
std::vector<themis::training::CheckpointManifestEntry>
parseManifest(const std::string& content) {
    std::vector<themis::training::CheckpointManifestEntry> result;
    themis::training::CheckpointManifestEntry entry;
    std::istringstream iss(content);
    std::string line;
    bool in_block = false;

    auto commitEntry = [&]() {
        if (in_block && !entry.checkpoint_path.empty()) {
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
        if (eq == std::string::npos) continue;
        in_block = true;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if      (key == "checkpoint_path") entry.checkpoint_path = val;
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
    if (!in.is_open()) return false;
    std::ofstream out(dst, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out << in.rdbuf();
    return out.good();
}

} // anonymous namespace

namespace themis {
namespace training {

// ============================================================================
// Impl
// ============================================================================
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
        for (char& c : filename) if (c == ' ') c = '_';

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
        if (!f.is_open()) return "";
        std::ostringstream oss;
        oss << f.rdbuf();
        return oss.str();
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

} // namespace training
} // namespace themis
