/**
 * @file model_registry.cpp
 * @brief Model Registry implementation
 *
 * Provides model version tracking, lifecycle management, and persistence.
 */

#include "rag/model_registry.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>

namespace themis::rag::lifecycle {

std::string StatusToString(ModelStatus status) {
  switch (status) {
    case ModelStatus::kDraft:
      return "draft";
    case ModelStatus::kValidated:
      return "validated";
    case ModelStatus::kCandidate:
      return "candidate";
    case ModelStatus::kDeployed:
      return "deployed";
    case ModelStatus::kRetired:
      return "retired";
    case ModelStatus::kFailed:
      return "failed";
  }
  return "unknown";
}

std::optional<ModelStatus> StringToStatus(const std::string& status_str) {
  if (status_str == "draft") return ModelStatus::kDraft;
  if (status_str == "validated") return ModelStatus::kValidated;
  if (status_str == "candidate") return ModelStatus::kCandidate;
  if (status_str == "deployed") return ModelStatus::kDeployed;
  if (status_str == "retired") return ModelStatus::kRetired;
  if (status_str == "failed") return ModelStatus::kFailed;
  return std::nullopt;
}

struct ModelRegistry::Impl {
  std::string persistence_path;
  std::map<uint32_t, ModelMetadata> models;  // version -> metadata
  uint32_t next_version = 1;
  mutable std::mutex mu;

  uint64_t GetCurrentTimestampUs() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<microseconds>(duration).count();
  }
};

ModelRegistry::ModelRegistry(const std::string& persistence_path)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->persistence_path = persistence_path;
  Load();
}

ModelRegistry::~ModelRegistry() = default;

uint32_t ModelRegistry::RegisterModel(const std::string& model_id,
                                      const std::string& training_dataset_id,
                                      const std::string& metrics_json,
                                      const std::string& cost_stats_json,
                                      const std::string& model_location,
                                      uint32_t parent_version) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  ModelMetadata meta;
  meta.version = pimpl_->next_version++;
  meta.model_id = model_id;
  meta.built_at_us = pimpl_->GetCurrentTimestampUs();
  meta.status_changed_at_us = meta.built_at_us;
  meta.status = ModelStatus::kDraft;
  meta.parent_version = parent_version;
  meta.training_dataset_id = training_dataset_id;
  meta.metrics_json = metrics_json;
  meta.cost_stats_json = cost_stats_json;
  meta.model_location = model_location;
  meta.notes = "Model created";

  // Placeholder: compute checksum from model_location
  meta.model_checksum = "checksum_" + std::to_string(meta.version);

  auto version = meta.version;
  pimpl_->models[version] = meta;

  // Persist immediately
  Persist();

  return version;
}

bool ModelRegistry::UpdateModelStatus(uint32_t version, ModelStatus new_status,
                                      const std::string& notes) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  auto it = pimpl_->models.find(version);
  if (it == pimpl_->models.end()) {
    return false;
  }

  // Validate state transition
  ModelStatus old_status = it->second.status;

  // Define valid transitions:
  // draft → validated, failed
  // validated → candidate, failed
  // candidate → deployed, failed
  // deployed → retired, failed
  // failed → any (recovery)
  // retired → (terminal, no transitions)

  bool valid_transition = false;
  if (old_status == ModelStatus::kRetired) {
    valid_transition = false;
  } else if (new_status == ModelStatus::kFailed) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kDraft &&
             (new_status == ModelStatus::kValidated)) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kValidated &&
             (new_status == ModelStatus::kCandidate)) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kCandidate &&
             (new_status == ModelStatus::kDeployed)) {
    valid_transition = true;
  } else if (old_status == ModelStatus::kDeployed &&
             (new_status == ModelStatus::kRetired)) {
    valid_transition = true;
  }

  if (!valid_transition) {
    return false;
  }

  it->second.status = new_status;
  it->second.status_changed_at_us = pimpl_->GetCurrentTimestampUs();
  it->second.notes = notes.empty() ? StatusToString(new_status) : notes;

  // Persist immediately
  Persist();

  return true;
}

std::optional<ModelMetadata> ModelRegistry::GetDeployedModel() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  for (auto it = pimpl_->models.rbegin(); it != pimpl_->models.rend(); ++it) {
    if (it->second.status == ModelStatus::kDeployed) {
      return it->second;
    }
  }
  return std::nullopt;
}

std::optional<ModelMetadata> ModelRegistry::GetByVersion(uint32_t version) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  auto it = pimpl_->models.find(version);
  if (it != pimpl_->models.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<ModelMetadata> ModelRegistry::GetLatest() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->models.empty()) {
    return std::nullopt;
  }
  return pimpl_->models.rbegin()->second;
}

std::vector<ModelMetadata> ModelRegistry::GetByStatus(ModelStatus status) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  std::vector<ModelMetadata> result;
  for (const auto& [version, metadata] : pimpl_->models) {
    if (metadata.status == status) {
      result.push_back(metadata);
    }
  }
  return result;
}

std::vector<ModelMetadata> ModelRegistry::GetLineage(uint32_t version) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  std::vector<ModelMetadata> result;
  uint32_t current = version;

  while (current > 0) {
    auto it = pimpl_->models.find(current);
    if (it == pimpl_->models.end()) {
      break;
    }
    result.push_back(it->second);
    current = it->second.parent_version;
  }

  return result;
}

bool ModelRegistry::Exists(uint32_t version) const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->models.count(version) > 0;
}

uint32_t ModelRegistry::Count() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->models.size();
}

bool ModelRegistry::Persist() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  // TODO: Implement JSON serialization to persistence_path
  // For now, this is a placeholder that succeeds without error
  // Production implementation should write to persistent storage (file or database)

  return true;
}

bool ModelRegistry::Load() {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  // TODO: Implement JSON deserialization from persistence_path
  // For now, this is a placeholder that succeeds with empty registry
  // Production implementation should restore from persistent storage

  return true;
}

std::string ModelRegistry::ToJson() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  // TODO: Implement comprehensive JSON export
  // For now, return a placeholder JSON structure

  std::string result = R"({
  "metadata": {
    "total_models": )";
  result += std::to_string(pimpl_->models.size());
  result += R"(,
    "next_version": )";
  result += std::to_string(pimpl_->next_version);
  result += R"(
  },
  "models": []
})";

  return result;
}

}  // namespace themis::rag::lifecycle
