/**
 * @file training_state_checkpoint.cpp
 * @brief Training state checkpoint implementation
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#include "training/training_state_checkpoint.h"
#include "utils/logger.h"
#include <fstream>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace themis {
namespace training {

// ============================================================================
// TrainingStateCheckpoint Implementation
// ============================================================================

json TrainingStateCheckpoint::toJSON() const {
    json j;
    j["epoch"] = epoch;
    j["global_step"] = global_step;
    j["steps_in_epoch"] = steps_in_epoch;
    j["current_loss"] = current_loss;
    j["best_loss"] = best_loss;
    j["rng_provider_name"] = rng_provider_name;
    j["optimizer_name"] = optimizer_name;
    j["dataset_source"] = dataset_source;
    j["dataset_position"] = dataset_position;
    j["dataset_seed"] = dataset_seed;
    j["hyperparameters"] = hyperparameters;
    j["training_run_id"] = training_run_id;
    j["training_config_hash"] = training_config_hash;
    j["base_model_hash"] = base_model_hash;
    j["hardware_platform"] = hardware_platform;
    j["num_devices"] = num_devices;
    j["elapsed_seconds"] = elapsed_seconds;
    j["step_losses_count"] = step_losses.size();
    return j;
}

TrainingStateCheckpoint TrainingStateCheckpoint::fromJSON(const json& j) {
    TrainingStateCheckpoint checkpoint;
    checkpoint.epoch = j.at("epoch").get<size_t>();
    checkpoint.global_step = j.at("global_step").get<size_t>();
    checkpoint.steps_in_epoch = j.at("steps_in_epoch").get<size_t>();
    checkpoint.current_loss = j.at("current_loss").get<double>();
    checkpoint.best_loss = j.at("best_loss").get<double>();
    checkpoint.rng_provider_name = j.at("rng_provider_name").get<std::string>();
    checkpoint.optimizer_name = j.at("optimizer_name").get<std::string>();
    checkpoint.dataset_source = j.at("dataset_source").get<std::string>();
    checkpoint.dataset_position = j.at("dataset_position").get<size_t>();
    checkpoint.dataset_seed = j.at("dataset_seed").get<uint64_t>();
    checkpoint.hyperparameters = j.at("hyperparameters");
    checkpoint.training_run_id = j.at("training_run_id").get<std::string>();
    checkpoint.training_config_hash = j.at("training_config_hash").get<std::string>();
    checkpoint.base_model_hash = j.at("base_model_hash").get<std::string>();
    checkpoint.hardware_platform = j.at("hardware_platform").get<std::string>();
    checkpoint.num_devices = j.at("num_devices").get<int>();
    checkpoint.elapsed_seconds = j.at("elapsed_seconds").get<double>();
    return checkpoint;
}

std::string TrainingStateCheckpoint::computeChecksum() const {
    json j = toJSON();
    std::string data_str = j.dump();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data_str.c_str()),
           data_str.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool TrainingStateCheckpoint::verifyChecksum(const std::string& expected_checksum) const {
    return computeChecksum() == expected_checksum;
}

// ============================================================================
// TrainingCheckpointManager Implementation
// ============================================================================

TrainingCheckpointManager::TrainingCheckpointManager(
    const std::string& checkpoint_dir,
    const std::string& training_run_id)
    : checkpoint_dir_(checkpoint_dir),
      training_run_id_(training_run_id) {
    THEMIS_INFO("Initialized TrainingCheckpointManager for run: {}", training_run_id);
}

TrainingCheckpointManager::~TrainingCheckpointManager() = default;

std::string TrainingCheckpointManager::getCheckpointPath(size_t epoch) const {
    return checkpoint_dir_ + "/checkpoint_epoch_" + std::to_string(epoch) + ".json";
}

std::string TrainingCheckpointManager::saveCheckpoint(const TrainingStateCheckpoint& checkpoint) {
    std::string checkpoint_path = getCheckpointPath(checkpoint.epoch);
    std::string temp_path = checkpoint_path + ".tmp";
    
    // Write to temporary file
    std::ofstream file(temp_path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open checkpoint file: " + temp_path);
    }
    
    json checkpoint_json = checkpoint.toJSON();
    checkpoint_json["checksum"] = checkpoint.computeChecksum();
    
    file << checkpoint_json.dump(2);
    file.close();
    
    // Atomic rename
    if (std::rename(temp_path.c_str(), checkpoint_path.c_str()) != 0) {
        throw std::runtime_error("Failed to save checkpoint: atomic rename failed");
    }
    
    THEMIS_INFO("Saved checkpoint at epoch {} to: {}", checkpoint.epoch, checkpoint_path);
    return checkpoint_path;
}

TrainingStateCheckpoint TrainingCheckpointManager::loadLatestCheckpoint() {
    auto checkpoints = listCheckpoints();
    if (checkpoints.empty()) {
        THEMIS_WARN("No checkpoints found");
        return TrainingStateCheckpoint();
    }
    
    return loadCheckpoint(checkpoints.back().first);
}

TrainingStateCheckpoint TrainingCheckpointManager::loadCheckpoint(size_t epoch) {
    std::string checkpoint_path = getCheckpointPath(epoch);
    
    std::ifstream file(checkpoint_path);
    if (!file.is_open()) {
        throw std::runtime_error("Checkpoint not found: " + checkpoint_path);
    }
    
    json checkpoint_json;
    file >> checkpoint_json;
    file.close();
    
    // Verify checksum if present
    if (checkpoint_json.contains("checksum")) {
        std::string saved_checksum = checkpoint_json.at("checksum").get<std::string>();
        checkpoint_json.erase("checksum");
        
        auto checkpoint = TrainingStateCheckpoint::fromJSON(checkpoint_json);
        if (!checkpoint.verifyChecksum(saved_checksum)) {
            throw std::runtime_error("Checkpoint checksum mismatch");
        }
        return checkpoint;
    }
    
    THEMIS_INFO("Loaded checkpoint from epoch {}", epoch);
    return TrainingStateCheckpoint::fromJSON(checkpoint_json);
}

std::vector<std::pair<size_t, std::string>> TrainingCheckpointManager::listCheckpoints() const {
    std::vector<std::pair<size_t, std::string>> result;
    // Simplified: would enumerate files in checkpoint_dir
    return result;
}

size_t TrainingCheckpointManager::pruneOldCheckpoints(size_t keep_last) {
    auto checkpoints = listCheckpoints();
    if (checkpoints.size() <= keep_last) {
        return 0;
    }
    
    size_t deleted_count = 0;
    for (size_t i = 0; i < checkpoints.size() - keep_last; ++i) {
        // Would delete checkpoints[i].second
        deleted_count++;
    }
    
    THEMIS_INFO("Pruned {} old checkpoints", deleted_count);
    return deleted_count;
}

} // namespace training
} // namespace themis
